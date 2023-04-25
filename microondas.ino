#include <Keypad.h> //Libreria del teclado
#include <TimeLib.h> //Libreria del reloj del modulo TM1637
#include <TM1637Display.h> //Libreria del display del modulo TM1637
#define CLK 4 //Se define pin del puerto CLK del display TM1637
#define DIO 5 //Se define pin del puerto DIO del display TM1637
TM1637Display display(CLK,DIO); //Se configuran pines del display
#define numeroSeg(_tiempo_) ((_tiempo_ / 1000) % 60) //Funcion de manera regresiva obtener los segundos  
#define numeroMin(_tiempo_) ((_tiempo_ / 1000) / 60) //Funcion de manera regresiva obtenr minutos
const int puerta = 3; //pin para detectar estado de la puerta del microondas
bool estadoPuerta = false;
bool encendido = false;
long anteriorMillis;
int pinMicroondas = 2;
const uint8_t End [] = { //arreglo que contiene la palabra "End"
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_D, //E
  SEG_C | SEG_E | SEG_G, //n
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G //d
};
const uint8_t Limpia [] = { //arreglo para configurar display a "----"
  SEG_G,
  SEG_G,
  SEG_G,
  SEG_G
};
const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 11, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad
bool confighora = false; //Booleano para el modo de trabajo, si confighora es verdadero entonces se trabaja en el modo de configuracion de hora
bool microondas = false; //Booleano para el modo microondas, si es verdadero entonces se trabaja con el microondas
bool cocinando = false; //Booleano para la coccion, si es verdadero entonces actualmente se esta cocinando
String entrada = "";
int contador;
unsigned long iniciomicroondas;
unsigned long minutos = 1;
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
char key = keypad.getKey();


void entrada_a_tiempo(String teclado){ //Transformacion de la entrada del teclado a int y configurando el tiempo
  int to = teclado.length();           //Mediante condiciones se evita el ingreso de horas mayores a 23 y min mayores a 59
  int h;
  int m;
  if (to == 4){
    h = teclado.substring(0,2).toInt();
    if(h>23){
      entrada="";
      contador=0;
      return;
    }
    m = teclado.substring(2,to).toInt();
    if(m>59){
      entrada="";
      contador=0;
      return;
    }
  }else{
    h = teclado.substring(0,1).toInt();
    if(h>23){
      entrada="";
      contador=0;
      return;
    }
    m = teclado.substring(1,3).toInt();  
    if(m>59){
      entrada="";
      contador=0;
      return;
    }
  }
  if(confighora)cambiarhora(h,m); //Garantizamos que estamos en el modo configuracion al momento de cambiar hora
}

void cambiarhora(int h,int m){ //funcion para cambiar la hora y minuto
  setTime(h,m,50,6,4,2023);
}

void imprimirHora(){ //Funcion para imprimir la hora en el display con los dos puntos separadores
  time_t t=now();
  if(millis() - anteriorMillis > 1000){ //Se toma en cuenta que haya corrido 1 segundo para ejecutar el encendido o apagado de los dos puntos
    anteriorMillis = millis();
    if(encendido){
      display.showNumberDecEx(hour(t),0,false,2,0);
      display.showNumberDec(minute(t),true,2,2);
      encendido = false;
    }else{
      display.showNumberDecEx(hour(t),0b01000000,false,2,0);
      display.showNumberDec(minute(t),true,2,2);
      encendido = true;
    }
  
  }
}

void pantallaentrada(){ //Funcion para imprimir "----" y preparar el ingreso
  display.clear();
  display.setSegments(Limpia,4,0);
}

void presentarinput(String s){ //Modo para imprimir el ingreso previo al cambio de hora o inicio de coccion
  if(s.length()==1){
    display.showNumberDec(s.toInt(),false,1,3);
  }
  if(s.length()==2){
    display.showNumberDec(s.toInt(),false,2,2);
  }
  if(s.length()==3){
    display.showNumberDec(s.toInt(),false,3,1);
  }
  if(s.length()==4){
    display.showNumberDec(s.toInt(),false,4,0);
  }
}

void cerrarconfig(){ //funcion para cerrar la configuracion
  
  if(contador==4){
    presentarinput(entrada);
    entrada_a_tiempo(entrada);
    imprimirHora();
    entrada="";
    contador=0;
    confighora=false;
    key='B';
  }else{
    entrada="";
    contador=0;
    confighora=false;
    key='B';
  }
}

void cambio(bool modo, char k){ //Funcion para el ingreso de digitos para cambiar la hora
  if(modo){
      if(contador==0)pantallaentrada(); //Impresion de pantalla "----"
      if(contador == 0 && k == '0') return; //Evitar ceros a la izquierda
      if(k != '*'){
       if(contador == 4 || k == 'A'){ //en caso de ya ingresar cuatro digitos automaticamente realizar ingreso 
          presentarinput(entrada);   //o al momento de ingresar la tecla A
          entrada_a_tiempo(entrada); //Funcion para transformar el ingreso a tiempo
          imprimirHora();
          cerrarconfig();
       }else{
          contador++;
          Serial.print("contador ");
          Serial.println(contador);
          entrada = entrada + k;
          presentarinput(entrada);
          if(contador==4)cerrarconfig();
       }
      }else if(k == '*' && contador>=1){
        cerrarconfig();
      }
    }
}

void cuentaRegresiva(long tiempo){
  long tiempores = tiempo - millis();
  int s = numeroSeg(tiempores);
  int m = numeroMin(tiempores);
  display.showNumberDecEx(s,0,true,2,2);
  display.showNumberDecEx(m,0b01000000,true,2,0);
  if (tiempores<50){
    microondas=false;
    cocinando = false;
    fin_de_coccion();
    entrada="";
  }
}

bool ingresaNum(char k){
  if(k != '*' && k != 'A' && k != 'B' && k != 'C' && k != 'D'){
    return true;
  }else{
    return false;
  }
}

void tiempoCocinar (char k){
      contador++;
      Serial.print("contador ");
      Serial.println(contador);
      entrada = entrada + k;
      presentarinput(entrada);
}

void setCoccion (char k){
  display.clear();
  if(k != '#'){
   entrada = entrada + k;
   presentarinput(entrada);
   Serial.println(entrada);
   cocinando = true;
  }else{
    iniciomicroondas=millis();
    if(entrada.length()==3){ //Seccion donde se toma la entrada de cada numero para considerar como segundo o minuto
       String minuto = entrada.substring(0,1);
       String seg = entrada.substring(1,3);
       minutos = minuto.toInt() * 1000 * 60;
       minutos = minutos + ( seg.toInt() * 1000);
    }
    if(entrada.length()==4){
       String minuto = entrada.substring(0,2);
       String seg = entrada.substring(2,4);
       minutos = minuto.toInt() * 1000 * 60;
       minutos = minutos + ( seg.toInt() * 1000);
    }
    Serial.println(entrada.length());
    if(entrada.length() <= 2){
      minutos = entrada.toInt();
      minutos = minutos*1000 + iniciomicroondas;
    }else{
       minutos = minutos + iniciomicroondas; 
    }
  }
}

void coccion(bool m){
  if(m){
       cuentaRegresiva(minutos);
       digitalWrite(pinMicroondas,HIGH);
       revisarPuerta();
   }else{
    digitalWrite(pinMicroondas,LOW);
    
    return;
   }
}

void revisarPuerta(){
  int p = digitalRead(puerta);
  if(p==1){
    estadoPuerta = true; //PUERTA ABIERTA
    Serial.println("ABIERTA");
  }else{
    estadoPuerta = false; //PUERTA CERRADA
    Serial.println("CERRADA");
  }
}

void fin_de_coccion(){
  digitalWrite(pinMicroondas,LOW);
  display.clear();
  display.setSegments(End,3,1); //impresion de la palabra End en el display 
  digitalWrite(A5,HIGH);
  delay(2000);
  digitalWrite(A5,LOW);
}

void setup() {
  Serial.begin(9600); //Configuracion del monitor serial
  pinMode(puerta, INPUT_PULLUP); //configuracion del pin para detectar estado de puerta del microondas
  pinMode(pinMicroondas, OUTPUT); //Configuracion del pin del LED como salida
  digitalWrite(pinMicroondas,LOW); //Configuracion del pin del LED en bajo
  display.setBrightness(1); //Configuracion del brillo del display
  pinMode(A5, OUTPUT); //Configuracion del pin del parlante
  digitalWrite(A5,LOW); //Configuracion del pin del parlante en bajo
  cambiarhora(8,8); //configuracion de la hora
  imprimirHora();
}

void loop() {
  revisarPuerta();
  key = keypad.getKey();
  if (key != NO_KEY){
   if(key =='*' && !confighora){ //Detectar la entrada para activar modo cambio de hora
    confighora=true;
   }else if(key=='*' && confighora){ //Salir del modo cambio de hora
    confighora=false;
   }
   cambio(confighora,key);
   if(ingresaNum(key) && !microondas && !confighora && !estadoPuerta){
      setCoccion(key);
      if(key=='#' && cocinando){
         microondas = true;
         display.clear();
       }
     }else if(key == 'D'){
       if(microondas)microondas = false;
       if(cocinando)cocinando = false;
       entrada = "";
     }
  }
  if(estadoPuerta){
    if(microondas)microondas = false;
    if(cocinando)cocinando = false;
  }
 coccion(microondas);
 if(!confighora && !cocinando)imprimirHora();
}
