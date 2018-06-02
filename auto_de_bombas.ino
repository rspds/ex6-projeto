#define NUM_BOMBA 2
#define led0 10
#define led1 11
#define ledE 12
#define TROCA 10000//86400000
#define TEMP1 A2
#define TEMP2 A3
#define CORRENTE A4
#define VAZAO A5
#define NV_S A0
#define NV_I A1
//#define TOLERANCIA 2
#define TEMPMAX 75
//#define TEMPMIN 0
#define TEMPODIAS 1
#define ERROMAX 2
#define ERROZERO 0
#define CORRENTEMIN 0

//====================================================================
long baud = 9600; // velocidade
#define timeout 1000 // Tempo máximo que o mestre espera a resposta do escravo
#define polling 200 // taxa que o mestre requisita os escravos
#define retry_count 10 //Caso haja um erro, quantas tentativas de comunicação ele vai fazer
#define TxEnablePin 2 // Pino do arduino que sera o enable
#define TOTAL_NO_OF_REGISTER 5 // número de registradores que vai ser utilizado
//====================================================================

#include <OneWire.h>
#include <EEPROM.h>
#include <SimpleModbusMaster_DUE.h> // incluindo biblioteca do mestre

enum { PACKET1,
	PACKET2,
	PACKET3,
	PACKET4,
	TOTAL_NO_OF_PACKETS
};

OneWire  dsTemp1(2);
OneWire  dsTemp2(3);

Packet packets[TOTAL_NO_OF_PACKETS]; //Cria um array com os pacotes que estão no enum

unsigned int regs[TOTAL_NO_OF_PACKETS]; // Cria um array chamado regs, todos os elementos desse array é do tipo unsigned int.

void processo();
void impressao();
void leitura();
void controle();
char chaveBomba(char modo);
bool trocaBomba();
bool ligarBomba(bool chaveB, bool erro);
bool condicao();
void conf_padrao();
void limpa_erro();
void somaErro(bool bombaPorta);
float lerTemp1();
float lerTemp2();
float conversao(int16_t raw, byte data[], byte type_s);
byte chip(byte addr);

typedef struct st_historico{
	byte dia;
	byte mes;
	byte ano;
	byte bomba;
	char tipo;
}t_hist;

bool trava = false;
bool bomba = 0;
float tempB1, tempB2;
float nivInf, nivSup;
float corrente;
unsigned long tempo = 0;

void setup() {
	modbus_construct (&packets[PACKET1],1,PRESET_SINGLE_REGISTER,0,1,0); 
	modbus_construct (&packets[PACKET2],1,PRESET_SINGLE_REGISTER,1,1,1);
	modbus_construct (&packets[PACKET3],1,PRESET_SINGLE_REGISTER,2,1,2);
	modbus_construct (&packets[PACKET4],1,READ_HOLDING_REGISTERS,3,1,3);

	modbus_configure(&Serial, baud, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs); // Iniciando as configurações de comunicação


	conf_padrao();


	Serial.begin(9600);
	for (int i = 0; i < 3; i++)
		pinMode(i + 10, OUTPUT);

}

void loop() {

	processo();
	impressao();
	delay(1000);
}

void impressao()
{
}

void processo()
{
	int intervalo = millis() - tempo;

	leitura();
	controle();

	if(intervalo > TROCA * TEMPODIAS)
		trocaBomba();
}

void leitura()
{
	nivInf = analogRead(NV_I);
	nivSup = analogRead(NV_S);
	tempB1 = lerTemp1();
	tempB2 = lerTemp2();
	//tempB1 = analogRead(TEMP1);
	//tempB2 = analogRead(TEMP2);
	corrente = analogRead(CORRENTE);

	nivInf = map(nivInf, 0, 1023, 0, 100);
	nivSup = map(nivSup, 0, 1023, 0, 100);
	//tempB1 = map(tempB1, 0, 1023, 20, 100);
	//tempB2 = map(tempB2, 0, 1023, 20, 100);
	corrente = map(corrente, 0, 1023, 0, 30);
}

void controle()
{
	if(nivInf < 20)
		chaveBomba('E');
	else
	{
		if(nivSup >= 80)
			chaveBomba('D');
		else if(nivSup <= 20 && chaveBomba('?') == 'D')
			chaveBomba('A');
		else
		{
			if(chaveBomba('?') == 'A')
				if(condicao() == false)
				{
					somaErro(bomba);
					trocaBomba();
				}
		}
	}
}

char chaveBomba(char modo)
{
	//a partir das letras, ele faz os comandos
	char resposta;

	switch(modo)
	{
		case 'E':
			ligarBomba(false, true);
			resposta = 'E';
			break;

		case 'A':
			if(chaveBomba('?') == 'D')
				ligarBomba(true, false);
			resposta = 'A';
			break;

		case 'D':
			ligarBomba(false, false);
			resposta = 'D';
			break; 

		case '?':
			if(digitalRead(led0) == HIGH || digitalRead(led1) == HIGH)
				resposta = 'A';
			else
				resposta = 'D';
			break;
	}
	return resposta;
}

bool trocaBomba()
{
	//aqui vai ser um lugar onde as bombas sao trocadas apartir da verificação dos erros no eeprom
	byte resposta;

	if(bomba == 0)
	{
		if(EEPROM.read(5) < ERROMAX)
			bomba = !bomba;
		else
		{
			if(EEPROM.read(4) < ERROMAX)
				bomba = bomba;
			else
				trava = true;
		}
	}
	else if(bomba == 1)
	{
		if(EEPROM.read(4) < ERROMAX)
			bomba = !bomba;
		else
		{
			if(EEPROM.read(5) < ERROMAX)
				bomba = bomba;
			else
				trava = true;
		}
	}	

	if(chaveBomba('?') == 'A')
	{
		chaveBomba('D');
		chaveBomba('A');
	}

	EEPROM.write(6, bomba);
	tempo = millis();

	return !trava; //retorna se o sistema ainda está em funcionamento
}

bool ligarBomba(bool chaveB, bool erro)
{
	if (erro == true)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, HIGH);
		return true;
	}
	else if (trava == true || chaveB == false)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, LOW);
		return true;
	}
	else if (bomba == 0)
	{
		digitalWrite(led0, HIGH);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, LOW);
		return true;
	}
	else if (bomba == 1)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, HIGH);
		digitalWrite(ledE, LOW);
		return true;
	}
	return false;
}

bool condicao()
{
	bool cond;

	if (bomba == 0 && tempB1 > 75)
		cond = false;
	if (bomba == 1 && tempB2 > 75)
		cond = false;
	else if (corrente > 20)
		cond = false;
	else
		cond = true;

	return cond;
}

/*
	 EEPROM

	 1 - tempo de duração [dias]
	 2 - min corrente
	 3 - max temp
	 4 - erro b1
	 5 - erro b2
	 6 - ultima bomba ativa
	 7 - 
	 8 - 
	 9 - 
	 10 - 
	 .
	 .
	 .
	 100 a 299 - Historico de erros B1
	 300 a 499 - Historico de erros B2

 */

void conf_padrao()
{
	EEPROM.write(1, TEMPODIAS);
	EEPROM.write(2, CORRENTEMIN);
	EEPROM.write(3, TEMPMAX);
	EEPROM.write(4, ERROZERO);
	EEPROM.write(5, ERROZERO);

	for(int i=100; i<500; i++)
		EEPROM.write(i, 0);
}

void limpa_erro()
{
	EEPROM.write(4, ERROZERO);
	EEPROM.write(5, ERROZERO);
}

void somaErro(bool bombaPorta)
{
	byte x;

	x = EEPROM.read(bombaPorta + 4);

	x++;

	EEPROM.write(bombaPorta + 4, x);

	return;
}

float lerTemp1()
{
  byte i;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
	bool flag = true;

	do
	{
		dsTemp1.reset_search();

		if(!dsTemp1.search(addr))
			return -1;

		if (OneWire::crc8(addr, 7) != addr[7])
			continue;

		type_s = chip(addr[0]);

		if(type_s == -1)
			return -1;

		dsTemp1.reset();
		dsTemp1.select(addr);
		dsTemp1.write(0x44);        // start conversion, use ds.write(0x44,1) with parasite power on at the end

		delay(1000); 

		if (dsTemp1.reset() == false)
			return -1;
		dsTemp1.select(addr);    
		dsTemp1.write(0xBE);         // Read Scratchpad

		for ( i = 0; i < 9; i++)    // we need 9 bytes
			data[i] = dsTemp1.read();
		if (OneWire::crc8(data, 8) != data[8])
			continue;

		flag = false;

	}while(flag);

	int16_t raw = (data[1] << 8) | data[0];

  celsius = conversao(raw, data, type_s);

	return celsius;
}

float lerTemp2()
{
	byte i;
	byte type_s;
	byte data[12];
	byte addr[8];
	float celsius;
	bool flag = true;

	do
	{
		dsTemp2.reset_search();

		if(!dsTemp2.search(addr))
			return -1;

		if (OneWire::crc8(addr, 7) != addr[7])
			continue;

		type_s = chip(addr[0]);

		if(type_s == -1)
			return -1;
		
		dsTemp2.reset();
		dsTemp2.select(addr);
		dsTemp2.write(0x44);        // start conversion, use ds.write(0x44,1) with parasite power on at the end

		delay(1000); 

		if (dsTemp2.reset() == false)
			return -1;
		dsTemp2.select(addr);    
		dsTemp2.write(0xBE);         // Read Scratchpad

		for ( i = 0; i < 9; i++)    // we need 9 bytes
			data[i] = dsTemp2.read();

		if (OneWire::crc8(data, 8) != data[8])
			continue;

		flag = false;

	}while(flag);

	int16_t raw = (data[1] << 8) | data[0];

  celsius = conversao(raw, data, type_s);

	return celsius;
}

float conversao(int16_t raw, byte data[], byte type_s)
{
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
	}
	return (float)raw / 16.0;
}

byte chip(byte addr)
{
	byte type_s;

		switch (addr[0]) {
			case 0x10:
				type_s = 1;
				break;
			case 0x28:
				type_s = 0;
				break;
			case 0x22:
				type_s = 0;
				break;
			default:
				return -1;
		}
	return type_s;
}



