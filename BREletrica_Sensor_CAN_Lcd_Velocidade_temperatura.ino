/*
  Veiculo Eletrico BREletrico  
  inicio  23/11/2017 para mediro a temperatura e mostrar os dados no LCD 
  2018/02/05 - Upgrade para medicao de Corrente e Tensao
  2018/08/26 - Colocando o display no painel versao com Arduino Mega
             - Instalando medidor de velocidade no pino 2 (Interrupcao)
  2018/09/08 - Odometro para calibrar o velocimetro 
             - Gravacao em EEPROM
  2018/09/13 - Calibracao Odometro. 210-170=40 pulsos Distancia 8.3metros           
               1 pulso = 8.30 / 40 = 0,2075 metro
  2021/03/16 - Mudando código para placa com Arduino Nano e CAN e fazendo a simulacao  
               com o AutoBoard
               Implementando rotina CAN           
     
  Pinos do LCD 
  // lcd_E 9 
  // lcd_RW 8
  // lcd_RS 7
  
  LCD Based on Universal 8bit Graphics Library, http://code.google.com/p/u8glib/
  Copyright (c) 2012, olikraus@gmail.com

  
*/

#include <Canbus.h>
#include <defaults.h>
#include <global.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>*/

#include "U8glib.h"
#include "MsTimer2.h"

/* Retirando a implementacao com One Wire
#include <OneWire.h>
#include <DallasTemperature.h>
*/
char versao[10]="16mar21";

int sensorPin_tensao_12v = A0;  
int sensorPin_tensao_24v = A1;  // verde   Calibracao 25.1 V = 857 unidades
int sensorPin_corr_24v   = A2;  // laranja Calibracao 0-1023    -5 to 5 
int sensorPin_corr_12v   = A3;  

const int velocidade_pin = 3;
int contador_pulsos=0;

int sensortemperatura_1_Pin = A4;
int sensortemperatura_2_Pin = A5;
int sensortemperatura_3_Pin = A6;
int sensortemperatura_4_Pin = A7;

#define PumpPin 5

/* pinos de display LCD */
#define  lcd_RS  7    // D7
#define  lcd_RW  8    // D8
#define  lcd_E   9    // D9
#define  lcd_RST 6    // D6 

U8GLIB_ST7920_128X64_4X u8g(lcd_E , lcd_RW, lcd_RS, lcd_RST );  // Enable, RW, RS, RESET   


int Temperatura_1_Int;
int Temperatura_2_Int;
int Temperatura_3_Int;
int Temperatura_4_Int;
   
int Tensao12vInt;
int Tensao24vInt;
int Corrente12vInt;
int Corrente24vInt;

int Velocidade_Int;
int Odometro_Int;

char Temperatura_1_Str[10]; 
char Temperatura_2_Str[10]; 
char Temperatura_3_Str[10]; 
char Temperatura_4_Str[10]; 

char Tensao12vStr[10];
char Tensao24vStr[10];
char Corrente12vStr[10];
char Corrente24vStr[10];

char Velocidade_Str[10];
char Odometro_Str[10]; 

void le_temperatura(void)
{ 
 Temperatura_1_Int = analogRead(sensortemperatura_1_Pin);  dtostrf(Temperatura_1_Int, 3, 0, Temperatura_1_Str);    
 Temperatura_2_Int = analogRead(sensortemperatura_2_Pin);  dtostrf(Temperatura_2_Int, 3, 0, Temperatura_2_Str);    
 Temperatura_3_Int = analogRead(sensortemperatura_3_Pin);  dtostrf(Temperatura_3_Int, 3, 0, Temperatura_3_Str);    
 Temperatura_4_Int = analogRead(sensortemperatura_4_Pin);  dtostrf(Temperatura_4_Int, 3, 0, Temperatura_4_Str);    
}

void le_corrente_tensao(void)
{
 float f;
 Tensao12vInt=analogRead(sensorPin_tensao_12v);    dtostrf(Tensao12vInt, 4, 0, Tensao12vStr);
 Corrente12vInt=analogRead(sensorPin_corr_12v);  dtostrf(Corrente12vInt, 4, 0, Corrente12vStr);  
 Tensao24vInt=analogRead(sensorPin_tensao_24v); 
 f=map(Tensao24vInt,0,1023,0,299)/10;     dtostrf(f,4,1,Tensao24vStr);   
 Corrente24vInt=analogRead(sensorPin_corr_24v);  
 f=map(Corrente24vInt,0,1023,-50,50)/10;  dtostrf(f,4,1,Corrente24vStr);
 // if (digitalRead(velocidade_pin)==HIGH) Velocidade_Int = 1; else Velocidade_Int = 0;  // usado para calibrar
 dtostrf(Velocidade_Int, 4, 0, Velocidade_Str);
 dtostrf(Odometro_Int, 4, 0, Odometro_Str);
}

void draw(void) {
 u8g.setFont(u8g_font_unifont);
 u8g.setScale2x2(); 
 u8g.drawStr( 0, 10 , "Vel="); u8g.drawStr(30, 10, Velocidade_Str);    
 u8g.undoScale();
 u8g.drawStr( 0, 36 , "T3=     T4=    ");  u8g.drawStr(24, 36, Temperatura_3_Str); u8g.drawStr(88, 36, Temperatura_4_Str);
 u8g.drawStr( 0, 50 , "Od=            ");  u8g.drawStr(24, 50, Odometro_Str);
 u8g.drawStr( 0, 62 , "V =     I =    ");  u8g.drawStr(24, 62, Tensao24vStr);      u8g.drawStr(88, 62, Corrente24vStr);
}

void inicia_lcd(void)
{
 if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
 }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
   else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
 }
}

void setup(void) { 
  pinMode(PumpPin,OUTPUT);
  pinMode(velocidade_pin,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(velocidade_pin), conta_pulsos, CHANGE);  
  MsTimer2::set(1000, BaseDeTempo);
  MsTimer2::start();
  /* Iinicializa lcd 
  inicia_lcd(); 
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_unifont);
    u8g.drawStr(20, 20, "BREletrico");      
    u8g.drawStr(30, 54, versao);
  } while (u8g.nextPage());
  delay(3000); 
  fim inicilizacao lcd*/ 
  Serial.begin(9600);
  if(Canbus.init(CANSPEED_250))  //Initialise MCP2515 CAN controller at the specified speed
    Serial.println("CAN Ok ");
  else
    Serial.println("CAN Failed ");
  delay(1000);
}

void conta_pulsos() {
  contador_pulsos++;
  Odometro_Int++;
}

void BaseDeTempo(void)
{
 Velocidade_Int=contador_pulsos;
 contador_pulsos=0;
}

unsigned char tp=0;

void loop(void) {

  tCAN message;
  le_temperatura();
  le_corrente_tensao();
  tp++;
  message.id = 0xFEBF;
  message.header.rtr = 0;
  message.header.length = 8; //formatted in DEC
  message.data[0] = (Velocidade_Int >> 8) && 0x00FF;
  message.data[1] = (Velocidade_Int && 0x00FF);
  message.data[2] = 0xFF;
  message.data[3] = 0xFF; 
  message.data[4] = 0xFF;
  message.data[5] = 0xFF;
  message.data[6] = 0xFF;
  message.data[7] = 0xFF;

  mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), 0);
  mcp2515_send_message(&message);
  delay(100);
  /* imprimindo no LCD 
  u8g.firstPage();  
  do {
      draw();   
  } while( u8g.nextPage() );
   Fim impressao LCD */
  /* rotinas de leitura CAN */
  if (mcp2515_check_message()) 
  {
    if (mcp2515_get_message(&message)) 
    {
               Serial.print("ID: ");
               Serial.print(message.id,HEX);
               Serial.print(", ");
               Serial.print("Data: ");
               Serial.print(message.header.length,DEC);
               for(int i=0;i<message.header.length;i++) 
                { 
                  Serial.print(message.data[i],HEX);
                  Serial.print(" ");
                }
               Serial.println("");
     }
   }
  /*fim rotina leitura CAN */
}
