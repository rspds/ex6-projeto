#define NUM_BOMBA 2
#define led0 10
#define led1 11
#define ledE 12
#define TEMP1 4
#define TEMP2 5
#define CORRENTE_PORTA A4
#define SENSOR_DE_CORRENTE false
#define VAZAO A5
#define NV_S A0
#define NV_I A1
//#define TOLERANCIA 2
#define TEMPMAX 75
//#define TEMPMIN 0
#define TEMPODIAS 1
#define TROCA 5000//86400000
#define ERROMAX 2
#define ERROZERO 0
#define CORRENTEMAX 20
#define TEMPERATURA_DIGITAL true
#define SSerialRX        6  //Serial Receive pin
#define SSerialTX        7  //Serial Transmit pin
#define SSerialTxControl 3   //RS485 Direction control
#define RS485Transmit    HIGH
#define RS485Receive     LOW
#define intervaloTransmissao           4000

#include <OneWire.h>
#include <EmonLib.h>
#include <EEPROM.h>
#include <SoftwareSerial.h> // incluindo biblioteca do mestre

OneWire  dsTemp1(TEMP1);
OneWire  dsTemp2(TEMP2);
EnergyMonitor emon1;
SoftwareSerial RS485(SSerialRX, SSerialTX); // RX, TX

void processo();
void impressao();
void comunicacao();
void liberarTrava();
void Transmite() ;
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
void zerarErroTela();
void inicializacao();
void atualizar();
void limpa_erro();
void somaErro(bool bombaPorta);
float lerTemp1();
float lerTemp2();
float conversao(int16_t raw, byte data[], byte type_s);
byte chip(byte addr);

static unsigned long int ultimaTransmissao = 0;
int contador = 0;

bool trava = false;
bool bomba = 0;
byte tempB1, tempB2;
byte nivInf, nivSup;
byte corrente;
unsigned long tempo = 0;

byte tempMax = 75, correnteMax = 20;
//byte nivSup_Max = 75, nivInf_Min = 20;
byte tempoRevesamento = 1, limpaErros = 0;
byte stB1 = true, stB2 = true;
bool limpaTela = false;

void setup()
{
  pinMode(SSerialTxControl, OUTPUT);
  digitalWrite(SSerialTxControl, RS485Receive);

  Serial.begin(9600);

  RS485.begin(9600);

  emon1.current(CORRENTE_PORTA, 29);

  delay(5000);

  conf_padrao();

  inicializacao();

  releSeguranca(!trava);

  Serial.begin(9600);
  for (int i = 0; i < 3; i++)
    pinMode(i + 10, OUTPUT);
}

void loop()
{
  comunicacao();
  processo();
  impressao();
  delay(1000);
}

void impressao()
{
  Serial.println(F("======================================="));
  Serial.print(F("Temperatura Bomba 1: "));
  Serial.println(tempB1);
  Serial.print(F("Temperatira Bomba 2: "));
  Serial.println(tempB2);
  Serial.print(F("Nivel Inferior: "));
  Serial.println(nivInf);
  Serial.print(F("Nivel Superior: "));
  Serial.println(nivSup);
  Serial.print(F("Corrente: "));
  Serial.println(corrente);
  Serial.print(F("Status da Bomba 1: "));
  Serial.println(stB1);
  Serial.print(F("Status da Bomba 2: "));
  Serial.println(stB2);
  Serial.print(F("Bomba: "));
  Serial.println(bomba);
  Serial.print(F("Qtd de Erro Bomba 1: "));
  Serial.println(EEPROM.read(4));
  Serial.print(F("Qtd de Erro Bomba 2: "));
  Serial.println(EEPROM.read(5));
  Serial.print(F("Ultima Bomba Ativa: "));
  Serial.println(EEPROM.read(6));
  Serial.print(F("Tempo de Duração [Dias]: "));
  Serial.println(EEPROM.read(1));
  Serial.print(F("Corrente Minima: "));
  Serial.println(EEPROM.read(2));
  Serial.print(F("Temperatura Maxima: "));
  Serial.println(EEPROM.read(3));
  Serial.print(F("Trava: "));
  Serial.println(trava);
  Serial.print(F("Temperatura Maxima[RS485]: "));
  Serial.println(tempMax);
  Serial.print(F("Corrente Maxima[RS485]: "));
  Serial.println(correnteMax);
  //  Serial.print("NivSup Maximo: ");
  //  Serial.println(nivSup_Max);
  //  Serial.print("nivInf_Max: ");
  //  Serial.println(nivInf_Min);
  Serial.print(F("Tempo de Revesamento: "));
  Serial.println(tempoRevesamento);
  Serial.print(F("Comando limpaErros: "));
  Serial.println(limpaErros);
  Serial.print(F("Comando limpaTela: "));
  Serial.println(limpaTela);
}

void comunicacao()
{

  if (millis() - ultimaTransmissao > intervaloTransmissao) Transmite();

  //checa se chegou um par de bytes
  if (RS485.available() > 1)
  { delay(400);
    //informa quandos bytes existem no buffer
    Serial.print("Buffer:  "); Serial.println(RS485.available());

    //Elimina o autobuffer
    byte erro = RS485.read();
    Serial.print(F("Autobuffer:  ")); Serial.println(erro);

    byte h;
    if (erro < 200 || erro > 207)
      h = RS485.read();
    else
      h = erro;
    int i = 0;
    do
    {
      if (RS485.available() == 0 && i >= 7)
        break;
      if (i > 0)
        h = RS485.read();
      byte l = RS485.read();
      switch (h)
      {
        case 200:
          tempMax = l;
          Serial.print(F("              PARTE HIGH(tempMax): ")); Serial.println(h);
          Serial.print(F("              PARTE LOW(tempMax): ")); Serial.println(l);
          break;
        case 201:
          correnteMax = l;
          Serial.print(F("              PARTE HIGH(correnteMax): ")); Serial.println(h);
          Serial.print(F("              PARTE LOW(correnteMax): ")); Serial.println(l);
          break;
        case 202:
          tempoRevesamento = l;
          Serial.print(F("              PARTE HIGH(tempoRevesamento): ")); Serial.println(h);
          Serial.print(F("              PARTE LOW(tempoRevesamento): ")); Serial.println(l);
          break;
        case 203:
          limpaErros = l;
          Serial.print(F("              PARTE HIGH(limpaErros): ")); Serial.println(h);
          Serial.print(F("              PARTE LOW(limpaErros): ")); Serial.println(l);
          break;
        case 204:
          stB1 = l;
          Serial.print(F("              PARTE HIGH(stB1): ")); Serial.println(h);
          Serial.print(F("              PARTE LOW(stB2): ")); Serial.println(l);
          break;
        case 205:
          stB2 = l;
          Serial.print(F("              PARTE HIGH(stB2): ")); Serial.println(h);
          Serial.print(F("              PARTE LOW(stB2): ")); Serial.println(l);
          break;
        case 206:
          limpaTela = l;
          Serial.print(F("              PARTE HIGH(Limpar Erro da Tela): ")); Serial.println(h);
          Serial.print(F("              PARTE LOW(Limpar Erro da Tela): ")); Serial.println(l);
          break;
      }
      i++;
    } while (h != 206 || i != 7);

    //Aguarda a mensagem falsa
    delay(3);

    //elimina a mensagem falsa
    erro = RS485.read();
    Serial.print(F("              mensagem falsa: ")); Serial.println(erro); Serial.println("");
  }
  zerarErroTela();
  atualizar();
  limpa_erro();
  liberarTrava();
}

void liberarTrava()
{
  if (EEPROM.read(5) < ERROMAX && stB2 || EEPROM.read(4) < ERROMAX && stB1)
    trava = false;
}

void Transmite()
{
  //configura este dispositivo como emissor
  digitalWrite(SSerialTxControl, RS485Transmit);

  //delay para o CI se estabilizar
  delay(10);
  Serial.println("Enviando...");
  RS485.write(200);
  RS485.write(corrente);
  RS485.write(201);
  RS485.write(nivSup);
  RS485.write(202);
  RS485.write(nivInf);
  RS485.write(203);
  RS485.write(tempB1);
  RS485.write(204);
  RS485.write(tempB2);
  RS485.write(205);
  RS485.write(bomba);
  RS485.write(206);
  RS485.write(EEPROM.read(8));
  RS485.write(207);
  RS485.write(EEPROM.read(9));
  RS485.write(208);
  RS485.write(trava);
  RS485.write(209);
  RS485.write(limpaErros);
  RS485.write(210);
  RS485.write(limpaTela);
  /*
    contador++;
    if (contador == 11)
      contador = 0;
  */
  //delay para o fim da transmissão
  delay(1);

  //configura este dispositivo como receptor
  //obs.: Este comando envia uma mensagem falsa
  digitalWrite(SSerialTxControl, RS485Receive);


  //atualiza o cronômetro
  ultimaTransmissao = millis();

}

void processo()
{

  leitura();
  controle();
}

void leitura()
{
  nivInf = analogRead(NV_I) / 4;
  nivSup = analogRead(NV_S) / 4;
  if(SENSOR_DE_CORRENTE)
  {
    double Irms = emon1.calcIrms(1480);
    corrente = (byte)Irms;
  }
  else
  {
    corrente = analogRead(CORRENTE_PORTA) / 4;
    corrente = map(corrente, 0, 255, 0, 30);
  }
  if (TEMPERATURA_DIGITAL)
  {
    tempB1 = lerTemp1();
    tempB2 = lerTemp2();
  }
  else
  {
    tempB1 = analogRead(TEMP1) / 4;
    tempB2 = analogRead(TEMP2) / 4;

    tempB1 = map(tempB1, 0, 255, 20, 100);
    tempB2 = map(tempB2, 0, 255, 20, 100);
  }
  //tempB2 = lerTemp2();

  nivInf = map(nivInf, 0, 255, 0, 100);
  nivSup = map(nivSup, 0, 255, 0, 100);
}

void controle()
{
  int intervalo = millis() - tempo;

  if (condicao() == false && trava == false)
  {
    somaErro(bomba);
    trocaBomba();
  }
  if (aviso() == false)
    ledErro(true);
  else
    ledErro(false);

  if (intervalo > TROCA * tempoRevesamento)
    trocaBomba();
}

bool trocaBomba()
{
  //aqui vai ser um lugar onde as bombas sao trocadas apartir da verificação dos erros no eeprom
  byte resposta;

  if (bomba == 0)
  {
    if (EEPROM.read(5) < ERROMAX && stB2)
      bomba = !bomba;
    else
    {
      if (EEPROM.read(4) < ERROMAX && stB1)
        bomba = bomba;
      else
        trava = true;
    }
  }
  else if (bomba == 1)
  {
    if (EEPROM.read(4) < ERROMAX && stB1)
      bomba = !bomba;
    else
    {
      if (EEPROM.read(5) < ERROMAX && stB2)
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
    EEPROM.write(8, (char)(bomba + 49));
    EEPROM.write(9, 'T');
  }
  else if (bomba == 1 && tempB2 > tempMax)
  {
    cond = false;
    EEPROM.write(8, (char)(bomba + 49));
    EEPROM.write(9, 'T');
  }
  else if (corrente > correnteMax)
  {
    cond = false;
    EEPROM.write(8, (char)(bomba + 49));
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
	{
    cond = false;
    EEPROM.write(8, 'N');
    EEPROM.write(9, 'S');
  }
	else if (nivInf < 20)// nivInf_Min)
  {
    EEPROM.write(8, 'N');
    EEPROM.write(9, 'I');
		cond = false;
  }
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
*/

void conf_padrao()
{
  EEPROM.write(1, TEMPODIAS);
  EEPROM.write(2, CORRENTEMAX);
  EEPROM.write(3, TEMPMAX);
  EEPROM.write(4, ERROZERO);
  EEPROM.write(5, ERROZERO);
  EEPROM.write(7, false);
  EEPROM.write(8, '-');
  EEPROM.write(9, '-');
  EEPROM.write(10, true);
  EEPROM.write(11, true);

  inicializacao();

  for (int i = 100; i < 500; i++)
    EEPROM.write(i, 0);
}

void zerarErroTela()
{
  if (limpaTela)
  {
    EEPROM.write(8, '-');
    EEPROM.write(9, '-');
  }
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
  if (tempoRevesamento != EEPROM.read(1))
    EEPROM.write(1, tempoRevesamento);

  if (correnteMax != EEPROM.read(2))
    EEPROM.write(2, correnteMax);

  if (tempMax != EEPROM.read(3))
    EEPROM.write(3, tempMax);

  if (stB1 != EEPROM.read(10))
    EEPROM.write(10, stB1);

  if (stB2 != EEPROM.read(11))
    EEPROM.write(11, stB2);
}

void limpa_erro()
{
  switch (limpaErros)
  {
    case 0:
      //nao faz nada
      break;
    case 1:
      EEPROM.write(4, ERROZERO);
      trava = false;
      break;
    case 2:
      EEPROM.write(5, ERROZERO);
      trava = false;
      break;
    case 3:
      EEPROM.write(4, ERROZERO);
      EEPROM.write(5, ERROZERO);
      trava = false;
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

    if (!dsTemp1.search(addr))
      return -1;

    if (OneWire::crc8(addr, 7) != addr[7])
      continue;

    type_s = chip(addr[0]);

    if (type_s == -1)
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

  } while (flag);

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

    if (!dsTemp2.search(addr))
      return -1;

    if (OneWire::crc8(addr, 7) != addr[7])
      continue;

    type_s = chip(addr[0]);

    if (type_s == -1)
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

  } while (flag);

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
