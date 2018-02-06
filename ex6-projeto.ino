#define NUM_BOMBA 2
#define led0 10
#define led1 11
#define ledE 12
#define TROCA 120000
#define TEMP1 A2
#define TEMP2 A3
#define CORRENTE A4
#define VAZAO A5
#define NV_S A0
#define NV_I A1

float temp[2];
float niv[2];
float corrente;
float vazao;
//int dados[4] = {0,0,0,0}; //1 - temp1; 2 - temp2; 3 - corrente; 4 - vazao
bool bombaSt[NUM_BOMBA] = {0, 0}, //status
     bombaCh[NUM_BOMBA] = {0, 0}; //chave
unsigned long tempo = 0;
byte bombaAtual = 0,
     bombaErro[2] = {0, 0};
bool necessidade;
bool ultimaBomba=0;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < NUM_BOMBA; i++)
    bomba = true;
  for (i = 0; i < 3; i++)
    pinMode(i + 10, OUTPUT);
}

void loop() {




}

void processo()
{
  int intervalo = millis() - tempo;

  leitura();
  controle();

	if (bombaAtual != 3)
	{
		if (intervalo > TROCA / 2)
		{
			trocaBomba(bombaAtual);
			tempo = millis();
		}
	}
}

void leitura()
{
	niv[0] = analogRead(NV_S);
	niv[1] = analogRead(NV_I);
	temp[0] = analogRead(TEMP1);
	temp[1] = analogRead(TEMP2);
	corrente = analogRead(CORRENTE);
	vazao = analogRead(VAZAO);

	niv[0] = map(niv[0], 0, 1023, 0, 100);
	niv[1] = map(niv[1], 0, 1023, 0, 100);
	temp[0] = map(temp[0], 0, 1023, 20, 100);
	temp[1] = map(temp[1], 0, 1023, 20, 100);
	corrente = map(corrente, 0, 1023, 0, 30);
	vazao = map(vazao, 0, 1023, 0, 8);
}

void controle()
{
	//controla a ativacao das bombas (bombaAtual) 
	if(niv[0] < 20)
		bombaAtual = 2;
	else
		if(niv[1] >= 80)
			bombaAtual = 3;
		else if(niv[1] < 20)
		{
			bombaAtual = !ultimaBomba;
			ultimaBomba = bombaAtual;
		}
		else
			if(bombaAtual != 3)
				bombaSt[bombaAtual] = condicao(bombaAtual);

}

bool trocaBomba()
{
	bool sucesso;
	byte bombaDesl;

	if (bombaAtual == 0)
		bombaDesl = 1;
	else if (bombaAtual == 1)
		bombaDesl = 0;
	else
		bombaDesl = 2;

	if (bombaCh[bombaDesl] == true)
	{
		ligarBomba(bombaDesl);
		bombaAtual = bombaDesl;
	}
	else if (bombaCh[bombaAtual] == true)
		ligarBomba(bombaAtual);


	return sucesso;
}

void ligarBomba(byte bomba)
{
	if (bomba == 0)
	{
		digitalWrite(led0, HIGH);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, LOW);
	}
	else if (bomba == 1)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, HIGH);
		digitalWrite(ledE, LOW);
	}
	else if (bomba == 2)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, HIGH);
	}
	else if (bomba == 3)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, LOW);
	}

}

bool condicao(int bomba)
{
	bool cond;

	if (temp[bomba] > 75)
		cond = false;
	else if (corrente > 20)
		cond = false;
	else if (vazao != 0)
		cond = false;
	else
		cond = true;

	return cond;
}


