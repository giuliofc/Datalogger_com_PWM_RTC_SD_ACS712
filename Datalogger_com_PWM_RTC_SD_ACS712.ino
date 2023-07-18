// ----- Medidor de corrente com sensor ACS712 -------

//int valor = 0;

#define analogIn A0
#define mVperAmp 66 
int RawValue = 0;
int ACSoffset = 2500;
float Voltage = 0;
float Amps = 0;
float potencia =0;

// -------- RTC real time clock ------

#include "Wire.h"
#define DS1307_ADDRESS 0x68
byte zero = 0x00;


// --------- SdCard no arduino R3 --------
// Ported to SdFat from the native Arduino SD library example by Bill Greiman
// On the Ethernet Shield, CS is pin 4. SdFat handles setting SS
const int chipSelect = 10;  //4
/*
 SD card read/write
  
 This example shows how to read and write data to and from an SD card file   
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10  //4
 
 created   Nov 2010
 by David A. Mellis
 updated 2 Dec 2010
 by Tom Igoe
 modified by Bill Greiman 11 Apr 2011
 This example code is in the public domain.
   
 */
int interMedia=0;
 
#include <SdFat.h>
#include <Chave.h>

SdFat sd;
SdFile myFile;

unsigned long interTravaRWFile=0;

/* ----------- PWM ----------------
   Ao precionar a chave+10 aumenta em 10% o pwm, ao precionar
   a chave-10 diminui em %10 o pwm.
*/

#define pinoPwm 5
#define pinoChaveIncre 2
#define pinoChaveDecre 3

Chave botaoIncre(pinoChaveIncre);
Chave botaoDecre(pinoChaveDecre);

byte pwm=0;

// ------ Chave para apagar o arquivo: ------------

#define pinoChaveDelete 4
Chave botaoDelete(pinoChaveDelete);


void setup() 
{
  Serial.begin(9600);
  
  Wire.begin();
  setDateTime();
  
  while (!Serial) {}  // wait for Leonardo
  Serial.println("DataLoger - PWM + RTC + SD + ACS712"); 
  Serial.println("Aperte qualquer tecla para comecar.");
  while (Serial.read() <= 0) {}
  delay(400);  // catch Due reset problem
  Serial.println(" ");
  Serial.println("Aguarde 10segundos para cada leitura.");
  
  // Initialize SdFat or print a detailed error message and halt
  // Use half speed like the native library.
  // change to SPI_FULL_SPEED for more performance.
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) sd.initErrorHalt();

  pinMode(pinoChaveIncre,INPUT_PULLUP); 
  pinMode(pinoChaveDecre,INPUT_PULLUP);

  interTravaRWFile=millis();  
}  // fim do setup 


void loop() 
{
  botaoDelete.leitura();
  botaoIncre.leitura();
  botaoDecre.leitura();

  if (interMedia <=1000)
  {
    interMedia++;
    Calcula_corrente();
  } 
  //Serial.println(interMedia);
  
  if ( (millis() >=interTravaRWFile +10000)&&(interMedia>=1000) )
  {
      interTravaRWFile=millis();
      
       // open the file for write at end like the Native SD library
      if (!myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
        sd.errorHalt("opening test.txt for write failed");
      }
      // if the file opened okay, write to it:
      Serial.print("Writing to test.txt...");
           
      printDate();  // imprimi a data    
      myFile.print(" | ");      
      
      myFile.print("Corrente [A]:");
      myFile.print(Calcula_corrente()/1000);      
      myFile.print(" | ");
      
      myFile.print("Potencia [W]:");
      myFile.println(Calcula_potencia()/1000);
      Amps =0;
      potencia =0;
      interMedia =0;
    
      // close the file:
      myFile.close();
      Serial.println("done.");
    
      // re-open the file for reading:
      if (!myFile.open("test.txt", O_READ)) {
        sd.errorHalt("opening test.txt for read failed");
      }
      Serial.println("test.txt:");
    
      // read from the file until there's nothing else in it:
      int data;
      while ((data = myFile.read()) >= 0) Serial.write(data);
      // close the file:
      myFile.close();
  }
  
  if ( botaoDelete.foiPressionada() )
  {
      sd.remove("test.txt");
      if ( sd.exists("test.txt") )
      {
        Serial.println("Nao apagou!");
      } 
      else 
      {
        Serial.println("Apagado com sucesso!");
      }    
  } 

  if (botaoIncre.foiPressionada())
  {
      funcaoIncrementa();
  }

  if (botaoDecre.foiPressionada())
  {
      funcaoDecrementa();
  }
  
}  //-------------- fim do Loop --------------------


double Calcula_corrente()
{
  RawValue = analogRead(analogIn);
  Voltage = (RawValue / 1024.0) * 5000; // Gets you mV
  Amps += ((Voltage - ACSoffset) / mVperAmp);
  return Amps;
}

double Calcula_potencia()
{
  potencia = 12 * Amps;
  return potencia;
}


void setDateTime()
{
  byte segundo =      00;  //0-59
  byte minuto =        12;  //0-59
  byte hora =           19;  //0-23
  byte diasemana =     3;  //1-7
  byte dia =               30;  //1-31
  byte mes =            10; //1-12
  byte ano  =            17; //0-99

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); 

  Wire.write(decToBcd(segundo));
  Wire.write(decToBcd(minuto));
  Wire.write(decToBcd(hora));
  Wire.write(decToBcd(diasemana));
  Wire.write(decToBcd(dia));
  Wire.write(decToBcd(mes));
  Wire.write(decToBcd(ano));

  Wire.write(zero); 

  Wire.endTransmission();
}

byte decToBcd(byte val)
{
// Conversão de decimal para binário
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)  
{
// Conversão de binário para decimal
  return ( (val/16*10) + (val%16) );
}

void printDate()
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  int segundo = bcdToDec(Wire.read());
  int minuto = bcdToDec(Wire.read());
  int hora = bcdToDec(Wire.read() & 0b111111);    //Formato 24 horas
  int diasemana = bcdToDec(Wire.read());             //0-6 -> Domingo - Sábado
  int dia = bcdToDec(Wire.read());
  int mes = bcdToDec(Wire.read());
  int ano = bcdToDec(Wire.read());

//Exibe a data e hora. Ex.:   3/12/13 19:00:00

  imprime_dia_da_semana(diasemana);
  
  myFile.print(dia);   
  myFile.print("/");    
  myFile.print(mes);      
  myFile.print("/");     
  myFile.print(ano);    
  myFile.print(" ");    
  if (hora < 10)
  {
    myFile.print("0");      
  }
  myFile.print(hora);    
  myFile.print(":");   
  if (minuto < 10)
  {
    myFile.print("0");      
  }
  myFile.print(minuto);    
  myFile.print(":");    
  if (segundo < 10)
  {
    myFile.print("0");     
  }
  myFile.print(segundo);
     
}

void imprime_dia_da_semana(int dia)
{
  switch (dia)
  {
    case 1:
      myFile.print("Domingo ");      
      break;
    case 2:
      myFile.print("Segunda ");      
      break;
    case 3:
      myFile.print("Terca ");
      break;
    case 4:
      myFile.print("Quarta ");
      break;
    case 5:
      myFile.print("Quinta ");
      break;
    case 6:
      myFile.print("Sexta ");
      break;
    case 7:
      myFile.print("Sabado ");
      break;
  }
}

void funcaoIncrementa() 
{    
    if (pwm <100)
    {  
      pwm +=10;
      analogWrite(pinoPwm, map(pwm, 0,100 ,0,255) );
      Serial.print("pwm +10 =");
      Serial.print(pwm);
      Serial.println("%");
    }
}
void funcaoDecrementa() 
{      
    if(pwm >0)
    { 
      pwm -=10; 
      analogWrite(pinoPwm, map(pwm, 0,100 ,0,255) );
      Serial.print("pwm -10 =");
      Serial.print(pwm);
      Serial.println("%");
    }
}



  

