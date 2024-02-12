 //------------------Configuracion-------------------------
const String Id_c = "Salida_V001_PruebaV013"; //Id de Codigo Instalado
byte i2c = 0x27;


#define ClaveCifrado 1111111111 //con cifrado
#define ClaveCifrado 0 //sin cifrado

unsigned long tiempo1 = 0;
unsigned long tiempo2 = 0;

#include <SoftwareSerial.h>
SoftwareSerial Lector(7,6 );

#include <TimeLib.h>
#include <Wire.h>       //Libreria necesaria para usar la comunicación I2C
#include <RTC.h>        //Libreria del reloj DS3231
RTC rtc(DST_OFF); // OR DST_OFF enciende o apaga el reloj
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(i2c,20,2);//Direccion de LCD


int ReleApertura = 4;

tmElements_t tm;
char CtTicket [20];
char CtActual [20];
int ns;
long MTS[10];
byte LcdOcupado = 0;
byte Reboot;
byte EstadoBarrera = 3; // 0=Barrera Apagada, 1=Barrera Abierta, 2=Barrera Cerrada

//------------------------------Variables de estado sensores---------------------------------
const String info = "INFO. ";

void setup() { 
  lcd.init();
  lcd.backlight();
  lcd.print("   GLADIATOR");
  lcd.setCursor(0, 1);
  lcd.print("FONO:XXXXXXXX");

 

  pinMode(ReleApertura, OUTPUT);
  digitalWrite(ReleApertura, HIGH);
  
  Wire.begin();
  Lector.begin(9600);//Velocidad del lector del lector de codigos de barras
  Serial.begin(9600);
  Serial.println(Id_c);
  Test();
  lcd.clear();
}

void loop() {
  BarreraCerrada();  //Calcula el voltaje actual del Sensor de Barrera Cerrada
  AjustarHora();
//    MostrarHora();
  
  if (EstadoBarrera == 2 && LcdOcupado == 0) {  //Funciones que se ejecutan solo si la barrera esta cerrada
    //ReiniciarLCD();
    MostrarVale();
    LectorTicket();    //Lee si se ha apretado el Boton para abrir la barrera
    
    LcdOcupado = 0;  
  }
  else {
    LimpiarLectorTicket();
  }
  if (EstadoBarrera == 1 && LcdOcupado == 0) {  //Funciones que se ejecutan solo si la barrera esta abierta
    pantalla(13, 0);
  }
  if (EstadoBarrera == 0) {  //Funciones que se ejecutan solo si la barrera esta apagada
    pantalla(3, 0);
  }
  //MostrarHora();
  delay(80);
}

void BarreraCerrada() { // 0=Barrera Apagada, 1=Barrera Abierta, 2=Barrera Cerrada
  float vA3;      //Variable que mide el sensor de barrera cerrada
  float vA2;      //Variable que mide el sensor de barrera cerrada
  static byte EABarrera;
  int SenBCerrada = analogRead(A3);                    // lectura del pin A3
  vA3 = SenBCerrada * (5.0 / 1023.0);
  Serial.print("A3: ");
  Serial.print(vA3);
  SenBCerrada = analogRead(A2);                    // lectura del pin A3
  vA2 = SenBCerrada * (5.0 / 1023.0);
  Serial.print(" A2: ");
  Serial.println(vA2);

  if (vA3 <=0.3 && vA2 <= 0.3) {
    
    tiempo1=millis();
    if(tiempo1-tiempo2>500){
      EABarrera = 0;     
    }
  } 
  else if (vA2 > 0.8 && vA3 <= 0.3) {
    EABarrera = 2;
    tiempo2=millis();
    delay(100);
  }
  else if (vA3 > 0.8 && vA2 <= 0.3) {
    EABarrera = 1;
    tiempo2=millis();
    delay(100);    
  } 

  
  if (EABarrera != EstadoBarrera) {
    EstadoBarrera = EABarrera;
    Data d = rtc.getData();
    String Fecha = (d.toString("Y-m-d_H:i:s"));
    Serial.print(info + "Fecha:." + Fecha + ".Barrera: ");
    switch (EstadoBarrera) {
      case 0:
        Serial.println("Apagada");
        break;
      case 1:
        Serial.println("Abierta");
        break;
      case 2:
        Serial.println("Cerrada");
        break;
    }
  }
}

void AbrirBarrera() {
  Enviarinfo("ReleApertura: ", 1);
  digitalWrite(ReleApertura, LOW);//Activa Relé logica negativa
  delay(1000);//Espera 1 segundo
  digitalWrite(ReleApertura, HIGH); //Apaga Relé logica negativa
  Enviarinfo("ReleApertura: ", 0);
  
}


void AjustarHora() {
  String recibido;
  while (Serial.available() > 0) {
    recibido = Serial.readStringUntil('\n');
  }
  int largo = recibido.length();
  /// R20210321220030S
  if (largo == 16 && recibido.startsWith("R") && recibido.endsWith("S")) {
    String Sano = recibido.substring(1, 5);
    String Smes = recibido.substring(5, 7);
    String Sdia = recibido.substring(7, 9);
    String Shora = recibido.substring(9, 11);
    String Sminuto = recibido.substring(11, 13);
    String Ssegundo = recibido.substring(13, 15);

    int ano = (Sano.toInt());
    int mes = (Smes.toInt());
    int dia = (Sdia.toInt());
    int hora = (Shora.toInt());
    int minuto = (Sminuto.toInt());
    int segundo = (Ssegundo.toInt());

    rtc.setDateTime(ano, mes, dia, hora, minuto, segundo); // Año, Mes, Día, Hora (24H), Minutos y Segundos
    Data d = rtc.getData();
    Serial.print("Nueva Hora: ");
    Serial.println(d.toString("Y-m-d H:i:s"));
  }
  if (recibido.equalsIgnoreCase("que hora es" )) {
    Data d = rtc.getData();
    Serial.print("la hora es: ");
    Serial.println(d.toString("Y-m-d H:i:s"));
  }
}

void LimpiarLectorTicket() {
  String codigo;
  while (Lector.available() > 0) {
    codigo = Lector.readStringUntil('\r');
  }
  int largo = codigo.length();
  if (largo > 0) {
    Data d = rtc.getData();
    String Fecha = (d.toString("Y-m-d_H:i:s"));
    Serial.println(info + "Fecha:." + Fecha + ".Lector de Codigo: " + codigo);
  }
  codigo = "";
}

void LectorTicket () {
  String codigo;
  String codigoEn;
  while (Lector.available() > 0) {
    codigo = Lector.readStringUntil('\r');
  }
  codigoEn = codigo;
  int largo = codigo.length();
  if (largo > 0) {
    if (largo == 11 && codigo.endsWith("P")) {
      unsigned long resta = 1111111111;
      unsigned long Codigo = codigo.toInt();
      //Codigo = Codigo - resta;
      codigo = String(Codigo);
      if (codigo.length() == 9) {
        codigo = "0" + codigo;
      }
      long usado = CodigoUsado(codigo);
      if (usado >= 10000 && usado <= 235959) {
        OrdenarCodigoS(codigo);
        Data c = rtc.getData();
        String TiempoActual = (c.toString("Y-m-d H:i:s"));
        long Tiempo = EvaluarCodigo(TiempoActual);
        if ((Tiempo >= -5) && (Tiempo <= 15)) {
          if ((Tiempo >= -5) && (Tiempo < 0)) {
            Serial.println("Error de reloj");
            EnviarTicket(1, codigoEn, TiempoActual, String(Tiempo));
          }
          else {
            EnviarTicket(1, codigoEn, TiempoActual, String(Tiempo));
          }
          pantalla(9, Tiempo);
          AbrirBarrera();
          LcdOcupado = 1;
          pantalla(12, 0);
          MTS[ns] = usado;
          ns = ns + 1;
          if (ns >= 10) {
            ns = 0;
          }
        }
        else if (Tiempo < -5 or Tiempo > 10) {
          EnviarTicket(3, codigoEn, TiempoActual, String(Tiempo));
          pantalla(5, 0);
        }
      }
      else {
        Data c = rtc.getData();
        String TiempoActual = (c.toString("Y-m-d H:i:s"));
        EnviarTicket(5, codigoEn, TiempoActual, "");
        pantalla(11, 0);
      }
    }
    else if (largo == 10) {
      Data c = rtc.getData();
      String TiempoActual = (c.toString("Y-m-d H:i:s"));
      EnviarTicket(2, codigoEn, TiempoActual, "");
      pantalla(6, 0);
    }
    else {
      Data c = rtc.getData();
      String TiempoActual = (c.toString("Y-m-d H:i:s"));
      EnviarTicket(7, codigoEn, TiempoActual, "");
      pantalla(4, 0);
    }
  }
  //ReiniciarLCD();
}

long CodigoUsado(String Codigo) {
  long n;
  byte cu;
  n = ((Codigo.substring(4, 10)).toInt());
  for (byte y = 0; y <= 9; y++) {
    Serial.println(MTS[y]);
    if (n != MTS[y]) {
      cu = 0;
    }
    else {
      cu = 1;
      break;
    }
  }
  if (cu == 1) {
    n = 0;
  }
  return n;
}

void OrdenarCodigoS(String Codigo) {
  String TiempoTicket;
  Data c = rtc.getData();
  TiempoTicket = (c.toString("Y"));
  TiempoTicket += ("-");
  TiempoTicket += Codigo.substring(2, 4);
  TiempoTicket += ("-");
  TiempoTicket += Codigo.substring(0, 2);
  TiempoTicket += (" ");
  TiempoTicket += Codigo.substring(4, 6);
  TiempoTicket += (":");
  TiempoTicket += Codigo.substring(6, 8);
  TiempoTicket += (":");
  TiempoTicket += Codigo.substring(8, 10);
  TiempoTicket.toCharArray(CtTicket, 20);
  Serial.println(TiempoTicket);
}

long EvaluarCodigo(String TiempoActual) {
  long Tiempo;
  TiempoActual.toCharArray(CtActual, 20);

  createElements(CtTicket);
  unsigned long PrimerTime = makeTime(tm);
  createElements(CtActual);
  unsigned long SegundoTime = makeTime(tm);
  Tiempo = ((SegundoTime - PrimerTime) / 60);
  if (Tiempo > 15) {
    Tiempo = ((PrimerTime - SegundoTime) / 60);
    Tiempo = (Tiempo * -1);
  }
  return Tiempo;
}

void createElements(const char *str) {
  int Year, Month, Day, Hour, Minute, Second ;
  sscanf(str, "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);
  tm.Year = CalendarYrToTm(Year);
  tm.Month = Month;
  tm.Day = Day;
  tm.Hour = Hour;
  tm.Minute = Minute;
  tm.Second = Second;
}

void MostrarHora() {
  Data d = rtc.getData();
  lcd.setCursor(0, 0);
  lcd.print(d.toString("Y/m/d H:i:s"));
  lcd.setCursor(0, 1);
  lcd.print(" FAVOR ACERCAR VALE ");
}

void MostrarVale() {
  Data d = rtc.getData();
  lcd.setCursor(0, 0);
  lcd.print(" FAVOR ACERCAR VALE ");
  lcd.setCursor(0, 1);
  lcd.print(d.toString("Y/m/d H:i:s"));
}

void ReiniciarLCD() {
  lcd.begin(20, 2);   //lcd de 20x2 caracteres
}

void pantalla (int ventana, long Tiempo) {
  switch (ventana) {

    case 3:
      lcd.setCursor(0, 0);
      lcd.print("    ADVERTENCIA     ");
      lcd.setCursor(0, 1);
      lcd.print("  BARRERA APAGADA!  ");
      break;

    case 4:
      lcd.clear();
      lcd.setCursor(7, 0);
      lcd.print("TICKET");
      lcd.setCursor(6, 1);
      lcd.print("NO VALIDO");
      delay(2000);
      break;

    case 5:
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("TIEMPO DE SALIDA");
      lcd.setCursor(6, 1);
      lcd.print("SUPERADO");
      delay(2000);
      break;

    case 6:
      lcd.setCursor(0, 0);
      lcd.print("TICKET DE ENTRADA?? ");
      lcd.setCursor(0, 1);
      lcd.print("DEBE CANCELAR ANTES ");
      delay(3000);
      lcd.clear();
      break;

    case 8:
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("SALIDA OK");
      lcd.setCursor(6, 1);
      lcd.print("TARJETA");
      break;

    case 9:
      lcd.clear();
      lcd.setCursor(5, 0);
      lcd.print("SALIDA OK");
      lcd.setCursor(0, 1);
      lcd.print("TIEMPO:");
      lcd.setCursor(9, 1);
      lcd.print(Tiempo);
      break;

    case 11:    
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("TICKET DE SALIDA");
      lcd.setCursor(8, 1);
      lcd.print("USADO");
      delay(1000);
      break;

    case 12:
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("GRACIAS POR");
      lcd.setCursor(3, 1);
      lcd.print("SU PREFERENCIA");
      delay(1200);
      break;
      
    case 13:
      lcd.setCursor(0, 0);
      lcd.print("   ESPERAR CIERRE   ");
      lcd.setCursor(0, 1);
      lcd.print("     DE BARRERA     ");
      break;
  }
}

void EnviarTicket (int tipo, String Codigo, String HoraSalida, String Tiempo) {
  Data c = rtc.getData();
  String Fecha = (c.toString("Y-m-d"));
  String Hora = (c.toString("H:i:s"));
  switch (tipo) {

    case 1:
      Serial.println("Ticket. Fecha: " + Fecha + " Hora: " + Hora + " Tiempo: " + Tiempo + " Codigo: " + Codigo + " Patente: ");
      break;
      
    case 2:
      Serial.println("Adver. FechaActual:." + HoraSalida + ".Tipo:.Ticket de Entrada.Codigo:." + Codigo);
      break;

    case 3:
      Serial.println("Adver. FechaActual:." + HoraSalida + ".Tiempo:." + Tiempo + ".Tipo:.Tiempo Superado.Codigo:." + Codigo);
      break;

    case 5:
      Serial.println("Adver. FechaActual:." + HoraSalida + ".Tipo:.Usado.Codigo:." + Codigo);
      break;

    case 7:
      Serial.println("Adver. FechaActual:." + HoraSalida + ".Tipo:.No Valido.Codigo:." + Codigo);
      break;
  }
}

void Test() {
  Enviarinfo("Reinicio: ", 1);
  BarreraCerrada();
}

void Enviarinfo(String Entrada, boolean Estado) {
  Data d = rtc.getData();
  String Fecha = (d.toString("Y-m-d_H:i:s"));
  switch (Estado) {
    case 0:
      Serial.println(info + "Fecha:." + Fecha + "." + Entrada + "Desactivado");
      break;
    case 1:
      Serial.println(info + "Fecha:." + Fecha + "." + Entrada + "Activado");
      break;
  }
}
