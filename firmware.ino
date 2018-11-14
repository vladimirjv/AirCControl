#include <Thread.h>
#include <ThreadController.h>
#include <Button.h>
#include <TM1637Display.h> 
#include <Keypad.h>
//#include <Event.h>
#include "Timer.h"
Timer t;
bool tecla= false;
bool TempAmbienteD=false;
bool TempEvaporadorD=false;
int tiempo=0;

#define Evaporador A1
#define Clima A0
#define OnOff 13
#define Compresor 12                   
#define Ventilador A3
#define button 6
#define CLK 10
#define DIO 9

TM1637Display display(CLK, DIO); 
//Variables de display
const uint8_t my[]={
  SEG_A | SEG_F | SEG_G,  
  SEG_A | SEG_B | SEG_F | SEG_G | SEG_E | SEG_C  };   
const uint8_t AMBIENTE[] = {  
  SEG_A | SEG_F | SEG_G,  
  SEG_A | SEG_B | SEG_F | SEG_G | SEG_E | SEG_C  };   
const uint8_t EVAPORADOR[] = {
  SEG_A | SEG_F | SEG_G,  
  SEG_A | SEG_B | SEG_F | SEG_G | SEG_E  };
const uint8_t SEGS[] = {  
  SEG_A | SEG_F | SEG_G,  
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D  };
//limpia display
const uint8_t data[] = {0x0, 0x0, 0x0, 0x0};

//Variables de Programa
int contadorTemp(0);
int clima=0;
int evaporador=0;
int TempAmbiente=0;
int TempEvaporador=0;
int TempAmbienteProm=0;
int TempEvaporadorProm=0;
int tempReq=25; //25°C es la temp deseada
int tempCrComp=10; //10° C

//Variables keypad
const byte Filas = 2;     //Cuatro filas 
const byte Cols = 2;    //Cuatro columnas
byte Pins_Filas[] = {3,2};   //Pines Arduino a los que contamos las filas. 
byte Pins_Cols[] = {4,5};  // Pines Arduino a los que contamos las columnas. 
char Teclas [ Filas ][ Cols ] = {  
  {'1','2'},  
  {'3','4'} };
Keypad teclado = Keypad(makeKeymap(Teclas), Pins_Filas, Pins_Cols, Filas, Cols);

//Variables Button
Button myBtn(button, true, true, 20);    //Declare the button
boolean state;

//Variables Threads
ThreadController controll = ThreadController();
Thread* analogReads = new Thread();
Thread principal =  Thread();

//Funciones
//funcion que devuelve la temperatura int
int temperatura(int sensor){
  float voltaje=sensor*(5.0/1023);
  float temperatura=((voltaje*-21.133)+78.018);
  int temp =temperatura;
  return temp;
}
/*Modo stand by compresor apagado, ventilador encendido*/
void StandBy(){
  digitalWrite(Compresor,LOW);
  //digitalWrite(Ventilador,HIGH);
}
/*Modo Encendido, compresor y ventilador encendido*/
void Enciende(){
  digitalWrite(Compresor,HIGH);
  //digitalWrite(Ventilador,HIGH);
}
/*Modo Apagado*/
void Apaga(){
  digitalWrite(Compresor,LOW);
  //digitalWrite(Ventilador,LOW);
}
//Callback Thread AnalogReads
void analogReadsCallback(){
  if(contadorTemp<100){
    clima=analogRead(Clima);
    //TempAmbiente=temperatura(clima);
    TempAmbienteProm+=temperatura(clima);
    
    evaporador=analogRead(Evaporador);
    //TempEvaporador=temperatura(evaporador);
    TempEvaporadorProm+=temperatura(evaporador); 
  }
  contadorTemp++;
}
void read_temp(int tempambiente,int temppozo){
  if(contadorTemp>100){
    TempAmbiente=tempambiente/100;
    TempEvaporador=temppozo/100;
    contadorTemp=0;
    TempAmbienteProm=0;
    TempEvaporadorProm=0;
    Serial.println(TempAmbiente);
    
  }
}
//Callback de Main Thread
void mainCallback(){
  if(state==true){  
    if(TempAmbiente>tempReq+3){
      if(TempEvaporador<tempCrComp){
        StandBy();
      }else{
        Enciende();
      }
    }else if(TempAmbiente + 3>tempReq){
      if(TempEvaporador<tempCrComp){
        StandBy();
      }else{
        Enciende();
      }
    }else if(TempAmbiente + 3<tempReq){
      StandBy();
    }
  }else{
    Apaga();
  }
}
void keypadEvent (KeypadEvent eKey)
{
  if(state){
      switch (teclado.getState())
      {
        case PRESSED:
        tecla=true;
        tiempo=0;
        Serial.print(eKey);
        switch(eKey){ 
          case '1':
          display.setSegments(data);    
          if(TempAmbienteD){
            tempReq++;
            if (tempReq > 29){tempReq = 29;}
              display.setSegments(AMBIENTE,2,2);        
              display.showNumberDec(tempReq,false,2,0);              
          }else if(TempEvaporadorD){
            tempCrComp++;
            if (tempCrComp > 9){tempCrComp = 9;}
              display.setSegments(EVAPORADOR,2,2);        
              display.showNumberDec(tempCrComp,false,2,0);              
          }else{tecla=false;}
          break;
          case '3':
            display.setSegments(data);    
            if(TempAmbienteD){
              tempReq--; 
              if (tempReq < 17){tempReq =  17;}
              display.setSegments(AMBIENTE,2,2); 
              display.showNumberDec (tempReq,false,2,0);
            }else if(TempEvaporadorD){
              tempCrComp--;    
              if (tempCrComp < 6){tempCrComp = 6;}     
              display.setSegments(EVAPORADOR,2,2);
              display.showNumberDec (tempCrComp,false,2,0);    
            }else{tecla=false;}
          break;
          case '2':  
            display.setSegments(data);  
            TempAmbienteD=true;
            TempEvaporadorD=false;
            display.setSegments(AMBIENTE,2,2);
            display.showNumberDec (tempReq,false,2,0);
          break;
          case '4':
            display.setSegments(data);
            TempAmbienteD=false;
            TempEvaporadorD=true;
            display.setSegments(EVAPORADOR,2,2);
            display.showNumberDec (tempCrComp,false,2,0);
          break;   
        }
      }   
    }
}
void setup() {
  pinMode(Compresor, OUTPUT);
  pinMode(OnOff, OUTPUT);
  Serial.begin(9600);
  t.every(2000, timerIsr, (void*)2);
  teclado.addEventListener(keypadEvent);
  display.setBrightness (7);
  analogReads->onRun(analogReadsCallback);
  analogReads->setInterval(10);
  principal.onRun(mainCallback);
  principal.setInterval(200);
  controll.add(analogReads);
  controll.add(&principal);
  
}

void loop() {
  controll.run();
  read_temp(TempAmbienteProm,TempEvaporadorProm);
  teclado.getKey();
  myBtn.read();                    //Read the button
  if (myBtn.wasReleased()) {
      state = !state;
      digitalWrite(OnOff, state);
  }
  t.update();
//////////////////
    if(state){
      if(!tecla){
          display.showNumberDec (TempAmbiente,false,2,0);  
          display.setSegments(my,2,2); 
        }
    }else{
        display.setSegments(data);
    }
/////////////////
}

void timerIsr()
{ 
  //your code
  tiempo++;
  if(tiempo>3){
    tecla=false;
  }
  //Serial.println(tiempo);
}
