#define NUM_BOMBA 2
#define led0 10
#define led1 11
#define ledE 12
#define TROCA 10000//86400000
#define TEMP1 A2
#define TEMP2 A3
#define CORRENTE_PORTA A4
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

enum { 
	//escrita
	NIVSUP,
	NIVINF,
	TEMPERATURA_B1,
	TEMPERATURA_B2,
	CORRENTE,
	BOMBA_LIGADA,
	ULTIMO_ERRO_BOMBA,
	ULTIMO_ERRO_TIPO,
	//leitura
	TEMPERATURA_MAX,
	CORRENTE_MAX,
//	NIVSUP_MAX,
//	NIVINF_MIN,
	TEMPO_DE_REVESAMENTO,
	LIMPA_ERROS,
	ST_B1,
	ST_B2,
	TOTAL_NO_OF_PACKETS
};

OneWire  dsTemp1(TEMP1);
OneWire  dsTemp2(TEMP2);

Packet packets[TOTAL_NO_OF_PACKETS]; //Cria um array com os pacotes que estão no enum

unsigned int regs[TOTAL_NO_OF_PACKETS]; // Cria um array chamado regs, todos os elementos desse array é do tipo unsigned int.

void processo();
void impressao();
void comunicacao();
void leitura();
void controle();
char chaveBomba(char modo);
bool trocaBomba();
void ledErro(bool erro);
void releSeguranca(bool chave);
void releRevesamento();
bool condicao();
bool aviso();
void conf_padrao();
void inicializacao();
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

byte tempMax = 75, correnteMax = 20;
//byte nivSup_Max = 75, nivInf_Min = 20;
byte tempoRevesamento = 1, limpaErros = 0;
byte stB1 = true, stB2 = true;

void setup() {
/*
	//escrita
	NIVSUP
	NIVINF
	TEMPERATURA_B1
	TEMPERATURA_B2
	CORRENTE
	BOMBA_LIGADA
	ULTIMO_ERRO_BOMBA
	ULTIMO_ERRO_TIPO
	TEMPERATURA_MAX
	CORRENTE_MAX
	NIVSUP_MAX
	NIVINF_MIN
	TEMPO_DE_REVESAMENTO
	LIMPA_ERROS
	ST_B1
	ST_B2
*/	

	modbus_construct (&packets[NIVSUP],								1,PRESET_SINGLE_REGISTER,0,1,NIVSUP								); 
	modbus_construct (&packets[NIVINF],								1,PRESET_SINGLE_REGISTER,1,1,NIVINF								);
	modbus_construct (&packets[TEMPERATURA_B1],				1,PRESET_SINGLE_REGISTER,2,1,TEMPERATURA_B1				);
	modbus_construct (&packets[TEMPERATURA_B2],				1,PRESET_SINGLE_REGISTER,3,1,TEMPERATURA_B2				);
	modbus_construct (&packets[CORRENTE],							1,PRESET_SINGLE_REGISTER,4,1,CORRENTE							);
	modbus_construct (&packets[BOMBA_LIGADA],					1,PRESET_SINGLE_REGISTER,5,1,BOMBA_LIGADA					);
	modbus_construct (&packets[ULTIMO_ERRO_BOMBA],		1,PRESET_SINGLE_REGISTER,6,1,ULTIMO_ERRO_BOMBA		);
	modbus_construct (&packets[ULTIMO_ERRO_TIPO],			1,PRESET_SINGLE_REGISTER,7,1,ULTIMO_ERRO_TIPO			);
	modbus_construct (&packets[TEMPERATURA_MAX],			1,READ_HOLDING_REGISTERS,8,1,TEMPERATURA_MAX			);
	modbus_construct (&packets[CORRENTE_MAX],					1,READ_HOLDING_REGISTERS,9,1,CORRENTE_MAX					);
	//modbus_construct (&packets[NIVSUP_MAX],						1,READ_HOLDING_REGISTERS,1,1,NIVSUP_MAX		 				);
	//modbus_construct (&packets[NIVINF_MIN],						1,READ_HOLDING_REGISTERS,1,1,NIVINF_MIN						);
	modbus_construct (&packets[TEMPO_DE_REVESAMENTO],	1,READ_HOLDING_REGISTERS,10,1,TEMPO_DE_REVESAMENTO);
	modbus_construct (&packets[LIMPA_ERROS],					1,READ_HOLDING_REGISTERS,11,1,LIMPA_ERROS					);
	modbus_construct (&packets[ST_B1],								1,READ_HOLDING_REGISTERS,12,1,ST_B1								);
	modbus_construct (&packets[ST_B2],								1,READ_HOLDING_REGISTERS,13,1,ST_B2								);

	modbus_configure(&Serial, baud, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs); // Iniciando as configurações de comunicação

	conf_padrao();

	inicializacao();

  releSeguranca(!trava);

	Serial.begin(9600);
	for (int i = 0; i < 3; i++)
		pinMode(i + 10, OUTPUT);
}

void loop() {

	comunicacao();
	processo();
	impressao();
	delay(1000);
}

void impressao()
{
	Serial.println("=======================================");
	Serial.print("Temperatura Bomba 1: ");
	Serial.println(tempB1);
	Serial.print("Temperatira Bomba 2: ");
	Serial.println(tempB2);
	Serial.print("Nivel Inferior: ");
	Serial.println(nivInf);
	Serial.print("Nivel Superior: ");
	Serial.println(nivSup);
	Serial.print("Corrente: ");
	Serial.println(corrente);
//	Serial.print("Vazao: ");
//	Serial.println(vazao);
//	Serial.print("Status da Bomba 1: ");
//	Serial.println(bombaSt[0]);
//	Serial.print("Status da Bomba 2: ");
//	Serial.println(bombaSt[1]);
	Serial.print("Bomba: ");
	Serial.println(bomba);
	Serial.print("Qtd de Erro Bomba 1: ");
	Serial.println(EEPROM.read(4));
	Serial.print("Qtd de Erro Bomba 2: ");
	Serial.println(EEPROM.read(5));
//	Serial.print("necessidade: ");
//	Serial.println(necessidade);
	Serial.print("Ultima Bomba Ativa: ");
	Serial.println(EEPROM.read(6));
	Serial.print("Tempo de Duração [Dias]: ");
	Serial.println(EEPROM.read(1));
	Serial.print("Corrente Minima: ");
	Serial.println(EEPROM.read(2));
	Serial.print("Temperatura Maxima: ");
	Serial.println(EEPROM.read(3));
  Serial.print("Trava: ");
  Serial.println(trava);
}

void comunicacao()
{
	modbus_update(); //Vai processar todos os pacotes de comunicação

  regs[NIVSUP						] = nivSup ;
  regs[NIVINF						] = nivInf ;
  regs[TEMPERATURA_B1		] = tempB1 ;
  regs[TEMPERATURA_B2		] = tempB2 ; 
  regs[CORRENTE					] = corrente ; 
  regs[BOMBA_LIGADA			] = bomba ; 
  regs[ULTIMO_ERRO_BOMBA] = EEPROM.read(8) ; 
  regs[ULTIMO_ERRO_TIPO	] = EEPROM.read(9) ; 

	tempMax 				= regs[TEMPERATURA_MAX			];
	correnteMax 		= regs[CORRENTE_MAX				];
//	nivSup_Max 	 	= regs[NIVSUP_MAX		 			];
//	nivInf_Min 	 	= regs[NIVINF_MIN					];
	tempoRevesamento= regs[TEMPO_DE_REVESAMENTO];
	limpaErros 			= regs[LIMPA_ERROS					];
	stB1 						= regs[ST_B1								];
	stB2 						= regs[ST_B2								];

	atualizar();
	limpa_erro();
}

void processo()
{

	leitura();
	controle();
}

void leitura()
{
	nivInf = analogRead(NV_I);
	nivSup = analogRead(NV_S);
	//tempB1 = lerTemp1();
	//tempB2 = lerTemp2();
	tempB1 = analogRead(TEMP1);
	tempB2 = analogRead(TEMP2);
	corrente = analogRead(CORRENTE_PORTA);

	nivInf = map(nivInf, 0, 1023, 0, 100);
	nivSup = map(nivSup, 0, 1023, 0, 100);
	tempB1 = map(tempB1, 0, 1023, 20, 100);
	tempB2 = map(tempB2, 0, 1023, 20, 100);
	corrente = map(corrente, 0, 1023, 0, 30);
}

void controle()
{
  int intervalo = millis() - tempo;

	if(condicao() == false && trava == false)
	{
		somaErro(bomba);
		trocaBomba();
	}
	if(aviso() == false)
		ledErro(true);
	else
		ledErro(false);

	if(intervalo > TROCA * tempoRevesamento)
		trocaBomba();
}

bool trocaBomba()
{
	//aqui vai ser um lugar onde as bombas sao trocadas apartir da verificação dos erros no eeprom
	byte resposta;

	if(bomba == 0)
	{
		if(EEPROM.read(5) < ERROMAX && stB2)
			bomba = !bomba;
		else
		{
			if(EEPROM.read(4) < ERROMAX && stB1)
				bomba = bomba;
			else
				trava = true;
		}
	}
	else if(bomba == 1)
	{
		if(EEPROM.read(4) < ERROMAX && stB1)
			bomba = !bomba;
		else
		{
			if(EEPROM.read(5) < ERROMAX && stB2)
				bomba = bomba;
			else
				trava = true;
		}
	}

	releSeguranca(!trava);
  releRevesamento();

	EEPROM.write(6, bomba);
	tempo = millis();

	return !trava; //retorna se o sistema ainda está em funcionamento
}

void ledErro(bool erro)
{
	if (erro == true)
		digitalWrite(ledE, HIGH);
	else
		digitalWrite(ledE, LOW);

	return;
}

void releSeguranca(bool chave)
{
	if (chave == true)
		digitalWrite(led1, HIGH);
	else
		digitalWrite(led1, LOW);

	return;
}

void releRevesamento()
{
	if (bomba == 0)
		digitalWrite(led0, HIGH);
	else
		digitalWrite(led0, LOW);

	return;
}

bool condicao()
{
	bool cond;

	if (bomba == 0 && tempB1 > tempMax)
	{
		cond = false;
		EEPROM.write(8, bomba);
		EEPROM.write(9, 'T');
	}
	else if (bomba == 1 && tempB2 > tempMax)
	{
		cond = false;
		EEPROM.write(8, bomba);
		EEPROM.write(9, 'T');
	}
	else if (corrente < correnteMax)
	{
		cond = false;
		EEPROM.write(8, bomba);
		EEPROM.write(9, 'C');
	}
	else
		cond = true;

	return cond;
}

bool aviso()
{
	bool cond;

	if (nivSup > 75)// nivSup_Max)
		cond = false; 
	else if (nivInf < 20)//	nivInf_Min)
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
	 7 - trava
	 8 - ultimo erro bomba
	 9 - ultimo erro tipo
	 10 - status da bomba 1
	 11 - status da bomba 2
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
	EEPROM.write(7, false);
	EEPROM.write(8, 0);
	EEPROM.write(9, '-');
	EEPROM.write(10, true);
	EEPROM.write(11, true);

	inicializacao();

	for(int i=100; i<500; i++)
		EEPROM.write(i, 0);
}

void inicializacao()
{
	tempoRevesamento = EEPROM.read(1);
	correnteMax = EEPROM.read(2);
	tempMax = EEPROM.read(3);
	bomba = EEPROM.read(6);
  trava = EEPROM.read(7);
	stB1 = EEPROM.read(10);
	stB2 = EEPROM.read(11);
}

void atualizar()
{
	if (tempMax != EEPROM.read(3))
		EEPROM.write(3, tempMax);
	
	if (correnteMax != EEPROM.read(2))
		EEPROM.write(2, correnteMax);
	
	if (tempoRevesamento != EEPROM.read(1))
		EEPROM.write(1, tempoRevesamento);
	
	if (stB1 != EEPROM.read(10))
		EEPROM.write(10, stB1);
	
	if (stB2 != EEPROM.read(11))
		EEPROM.write(11, stB2);	
}

void limpa_erro()
{
	switch(limpaErros)
	{
		case 0:
			//nao faz nada
			break;
		case 1:
			EEPROM.write(4, ERROZERO);
			break;
		case 2:
			EEPROM.write(5, ERROZERO);
			break;
		case 3:
			EEPROM.write(4, ERROZERO);
			EEPROM.write(5, ERROZERO);
			break;
	}
	return;
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

	switch (addr) {
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



