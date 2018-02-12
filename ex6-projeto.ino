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


void processo();
void impressao();
void leitura();
void controle();
bool trocaBomba();
bool ligarBomba(byte bomba);
bool condicao(int bomba);


float temp[2];
float niv[2];
float corrente;
float vazao;
//int dados[4] = {0,0,0,0}; //1 - temp1; 2 - temp2; 3 - corrente; 4 - vazao
bool bombaSt[NUM_BOMBA] = {1, 1}, //status
unsigned long tempo = 0;
byte bombaAtual = 3,
     bombaErro[2] = {0, 0};
bool necessidade = false;
bool ultimaBomba = 0;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 3; i++)
    pinMode(i + 10, OUTPUT);
}

void loop() {

	processo();
	impressao();

}

void impressao()
{
	Serial.print("Temperatura Bomba 1: ");
	Serial.println(temp[0]);
	Serial.print("Temperatira Bomba 2: ");
	Serial.println(temp[1]);
	Serial.print("Nivel Inferior: ");
	Serial.println(niv[0]);
	Serial.print("Nivel Superior: ");
	Serial.println(niv[1]);
	Serial.print("Corrente: ");
	Serial.println(corrente);
	Serial.print("Vazao: ");
	Serial.println(vazao);
	Serial.print("Status da Bomba 1: ");
	Serial.println(bombaSt[0]);
	Serial.print("Status da Bomba 2: ");
	Serial.println(bombaSt[1]);
	Serial.print("bombaAtual: ");
	Serial.println(bombaAtual);
	Serial.print("Qtd de Erro Bomba 1: ");
	Serial.println(bombaErro[0]);
	Serial.print("Qtd de Erro Bomba 2: ");
	Serial.println(bombaErro[1]);
	Serial.print("necessidade: ");
	Serial.println(necessidade);
	Serial.print("ultimaBomba: ");
	Serial.println(ultimaBomba);
}

void processo()
{
  int intervalo = millis() - tempo;

  leitura();
  controle();

	if (intervalo > TROCA / 2 || !bombaSt[bombaAtual])
	{
		trocaBomba();
		tempo = millis();
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
		{
			bombaAtual = 3;
			necessidade = false;
		}
		else if(niv[1] < 20 && necessidade == false)
		{
			necessidade = true;
			bombaAtual = !ultimaBomba;
			ultimaBomba = bombaAtual;
		}
		else
			if(bombaAtual != 3)
				bombaSt[bombaAtual] = condicao(bombaAtual);
				if(bombaSt[bombaAtual] == false)
					bombaErro[bombaAtual]++;

}
/*
falta algo tlg
algo
implementar a ativacao inicial das bombas de algum jeito que eu ainda nao sei
ver como vai ligar o led do erro na trocaBomba()
e por falat nisso, tenhi minhas duvidas se esse é mesmo o caminho mais correto a seguir
"priorizar a trocaBomba() pra fazer tudo"
sinto que está faltando uma ou duas funcoes pra o codigo ficar estavel
e depois pra o codigo ficar enchuto vai poder reduzir cerca de 22% do codigo em
partes de funcoes ou 2 funcoes jogadas no lixo mesmo

a variavel necessidade ja e uma chaveeeee!!!!
*/
bool trocaBomba()
{
	byte bombaDesl;

	if (bombaAtual == 0)
		bombaDesl = 1;
	else if (bombaAtual == 1)
		bombaDesl = 0;
	else
	{
		ligarBomba(bombaAtual);
		return true;	
	}

	if (bombaErro[bombaDesl] >= 2)
	{
		ligarBomba(bombaDesl);
		bombaAtual = bombaDesl;
		return true;
	}
	else if (bombaErro[bombaAtual] >= 2)
	{
		ligarBomba(bombaAtual);
		return true;
	}


	return false;
}

bool ligarBomba(byte bomba)
{
	if (bomba == 0)
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
	else if (bomba == 2)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, HIGH);
		return true;
	}
	else if (bomba == 3)
	{
		digitalWrite(led0, LOW);
		digitalWrite(led1, LOW);
		digitalWrite(ledE, LOW);
		return true;
	}
	return false;
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



