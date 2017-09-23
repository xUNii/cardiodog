/**
Matheus Silva Cardoso - 
Analise e Desenvolvimento de Sistemas - Faculdade de Tecnologia de Franca - Thomaz Novelino

Agradecimento a comunidade Pulso Sensor, por tirar duvidas e auxiliar na construçao do codigo!

**/
#include <LiquidCrystal.h> // incluindo a biblioteca do Display lcd 16x2

//  Variaveis
int PulsoPin = 0;                 // Pulse Sensor fio preto conectado ao pino analogico A0
int batimentosPin = 13;                // pino para piscar o led a cada Batida(13)

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); //definindo os pinos que serao utilizados no display LCD 16X2

//Variaveis voláteis, usadas na rotina do serviço de interrupçao.

volatile int BPM;                   // int que contem o valor analogico bruto(A0), atualizado a cada 2mS
volatile int Sinal;                // contem os dados brutos recebido
volatile int IBI = 600;             // int que contém o intervalo de tempo entre batimentos! //dado fornecido pela Pulso sensor
volatile boolean Pulso = false;     // "True" quando detecta-se a pulsação do coração. "False" quando não encontra uma.
volatile boolean QS = false;        // Torna-se "True" quando o Arduino encontra o pulso


static boolean serialVisual = true;   // Para Visualizar no Monitor Serial

volatile int ultimosVal[10];                      // Utilizada para guardar os últimos dez valores de IBI
volatile unsigned long contadorPulso = 0;          // Utilizada para determinar o tempo de pulso
volatile unsigned long ultimaBatida = 0;           // Utilizada para encontrar o IBI
volatile int P = 512;                      // Usado para encontrar o P da onda de pulso //dado disponibilizado pela Pulso sensor
volatile int T = 512;                     // Usada para encontrar o T da onda de pulso //dado disponibilizado pela Pulso sensor
volatile int thresh = 525;                // Usada para encontrar o momento exato do batimento cardiaco //dado disponibilizado pela Pulso sensor
volatile int amplitude = 100;                   // Usada para manter a amplitudelitude na onda de pulso //dado disponibilizado pela Pulso sensor
volatile boolean primeiraBatida = true;        // usado para começar com BPM rasoavel
volatile boolean segundaBatida = false;      // usado para começar com BPM rasoavel


void setup()
{
  pinMode(batimentosPin,OUTPUT);         // Piscara de a acordo ao encontrar os batimentos
  Serial.begin(115200);             // Fazendo acontecer rapido, caso perca os batimentos, reassume rapidamente! //baud = bits por segundo
  interrupcao();                 // Le o sinal de pulso a cada 2mS da interrupcao
            
}


//  Aqui a Magica Acontece
void loop()
{
   saidaSerial();  
   
  if (QS == true) // Frequencia Cardiaca Encontrada
    {     
      // BPM e IBI foram determinados
      // Coloca "QS" em verdadeiro automaticamente quando encontra um Batimento
      quandoABatidaAcontece(); // Uma pulsaçao aconteceu, mandando para a Serial  
      QS = false; // reseta a Quantified Self flag para a proxima chamada    
    }
 
  delay(20);
}


void interrupcao(){     
  // Inicializa Timer2 para lançar uma interrupção a cada 2mS.
  // Disponibilizada pela Pulse Sensor
  TCCR2A = 0x02;     // Disabilita PWM nos pinos Digitais 3 e 11 e entra no modo CTC
  TCCR2B = 0x06;     // Nao força a comparar, 1/256 PRESCALER 
  OCR2A = 0X7C;      // AJUSTE A PARTE SUPERIOR DO CONTATO PARA 124 PARA A TAXA DE AMOSTRA 500Hz
  TIMSK2 = 0x02;     // HABILITA A INTERRUPÇAO na partida entre o TIMER2 e OCR2A
  sei();             // Habilita as interrupçoes     
} 

void saidaSerial()
{   // Decide qual saida Serial 
 if (serialVisual == true)
  {  
     monitorSerial('-', Sinal);   // Vai para a funço do Monitor Serial
  } 
 else
  {
      enviarDadosparaSerial('S', Sinal);     // vai para funçao enviarDadosparaSerial 
   }        
}

void quandoABatidaAcontece()
{    
 if (serialVisual == true) //  Codigo q faz o monitor serial funcionar
   {            
     Serial.print("*** Batimento Cardiaco Encontrado *** ");  //APARECE NO MONITOR SERIAL
     Serial.print("BPM: ");
     Serial.println(BPM);
     lcd.clear();
     lcd.print("BPM: ");
     lcd.print(BPM);
   }
 else
   {
     enviarDadosparaSerial('B',BPM);   // envia os batimentos cardíacos com um prefixo 'B'
     enviarDadosparaSerial('Q',IBI);   // envia o tempo entre batidas com um prefixo 'Q'
   }   
}

 void monitorSerial(char symbol, int data )
{    
  const int sensorMin = 0;      // minimo do sensor, disponibilizado pela Pulso sensor 
  const int sensorMax = 1024;    // maximo do sensor, disponibilizado pela Pulso sensor
  int lendoSensor = data; // mapeie o alcance do sensor para um intervalo de 12 opções:
  int range = map(lendoSensor, sensorMin, sensorMax, 0, 11);
  /** imprime algo diferente dependendo do valor de alcance 
  switch (range) 
  {
    case 0:     
      Serial.println("");     /////APARECE NO MONITOR SERIAL de acordo com o alcance do sensor
      break;
    case 1:   
      Serial.println("---");
      break;
    case 2:    
      Serial.println("------");
      break;
    case 3:    
      Serial.println("---------");
      break;
    case 4:   
      Serial.println("------------");
      break;
    case 5:   
      Serial.println("--------------|-");
      break;
    case 6:   
      Serial.println("--------------|---");
      break;
    case 7:   
      Serial.println("--------------|-------");
      break;
    case 8:  
      Serial.println("--------------|----------");
      break;
    case 9:    
      Serial.println("--------------|----------------");
      break;
    case 10:   
      Serial.println("--------------|-------------------");
      break;
    case 11:   
      Serial.println("--------------|-----------------------");
      break;
  } 
**/
}


void enviarDadosparaSerial(char symbol, int data )
{
   Serial.print(symbol);
   Serial.println(data);                
}

ISR(TIMER2_COMPA_vect) //disparada quando Timer2 contou 124 //Rotina de Interrupçao //PulseSensor
{  
  cli();                                      // desativa as interrupçoes;
  Sinal = analogRead(PulsoPin);              // le o sensor de pulso
  contadorPulso += 2;                         // acompanha o tempo em mS
  int N = contadorPulso - ultimaBatida;       // monitora o tempo desde a ultima pulsacao
  
  //  encontra o P e velocidade da onda de pulso
  if(Sinal < thresh && N > (IBI/5)*3) // evita uma pulsaçao dupla esperando 3/5 do ultimo IBI
    {      
      if (Sinal < T) // T e a baixa da onda de pulsacao 
      {                        
        T = Sinal; // acompanha o ponto mais baixo da onda de pulso
      }
    }

  if(Sinal > thresh && Sinal > P)
    {          // thresh condition helps avoid noise
      P = Sinal;                             // P e o P da onda de pulso
    }                                        // acompanha o ponto mais alto da onda de pulso

  //  Agora vamos procurar o batimento cardiaco
  // O sinal aumenta de valor sempre que há um pulso
  if (N > 250)
  {                                   // Evita uma pulsaçao muito alta
    if ( (Sinal > thresh) && (Pulso == false) && (N > (IBI/5)*3) )
      {        
        Pulso = true;                               // Define para True, quando acha que encontrou um batimento
        digitalWrite(batimentosPin,HIGH);                // Liga o Led do pino 13
        IBI = contadorPulso - ultimaBatida;         // mede o tempo entre os batimentos em mS
        ultimaBatida = contadorPulso;               // acompanha o tempo para o proximo batimento
  
        if(segundaBatida)
        {                        // se esta for a segunda batida, segundaBatida == TRUE
          segundaBatida = false;                  // limpa a variavel segundaBatida
          for(int i=0; i<=9; i++) //  total de execução para obter um BPM realista na inicialização
          {             
            ultimosVal[i] = IBI;                      
          }
        }
  
        if(primeiraBatida) //Se é a primeira vez que encontramos uma batida, primeiraBatida == TRUE
        {                         
          primeiraBatida = false;                   // limpa primeiraBatida 
          segundaBatida = true;                   // define a segunda batida como apta
          sei();                               // habilita as interrupçoes novamente
          return;                              // Se o valor do IBI nao e valido, entao descarte
        }   
      
      // mantenha um total dos últimos 10 valores IBIs
      word medicaoTotal = 0;                  // limpa a variavel medicaoTotal   

      for(int i=0; i<=8; i++)
        {                // mudando os dados em ultimosVal 
          ultimosVal[i] = ultimosVal[i+1];                  // retira o valor de IBI mais antigo
          medicaoTotal += ultimosVal[i];              // Adiciona os 9 valores de IBI mais antigos
        }

      ultimosVal[9] = IBI;                          // adiciona o IBI a matriz
      medicaoTotal += ultimosVal[9];                // adiciona o ultimo IBI ao medicaoTotal
      medicaoTotal /= 10;                     // média dos últimos 10 valores IBI 
      BPM = 60000/medicaoTotal;               // quantos batimentos podem se encaixar em um minuto? Isso é BPM!
      QS = true;                              
      // QS FLAG nao esta apurado dentro do ISR
    }                       
  }

  if (Sinal < thresh && Pulso == true)
    {   //Quando os valores estao baixando, a medicao acabou
      digitalWrite(batimentosPin,LOW);            // desligue o pino 13 do  LED
      Pulso = false;                         // limpa a variavel Pulso para que ela possa receber um novo valor
      amplitude = P - T;                           // obtem a amplitudelitude da onda de pulso
      thresh = amplitude/2 + T;                    // ajusta o intervalo em 50% da amplitudelitude
      P = thresh;                            // reinicia para a proxima mediçao
      T = thresh;
    }

  if (N > 2500)
    {                           // Se ocorrer 2,5 segundos sem um batimento
      thresh = 512;                          // volte thresh para o padrao
      P = 512;                               // volte P para o padrao
      T = 512;                               // volte T para o padrao
      ultimaBatida = contadorPulso;          // atualize o ultimaBatida      
      primeiraBatida = true;                      // definindo para evitar interferencia
      segundaBatida = false;                    //quando recuperamos o pulso
    }

  sei();                                   // Ativa as interrupçoes quando terminar
}// fim do isr





