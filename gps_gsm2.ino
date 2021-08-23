#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SoftwareSerial.h>
#define N 10
#define SAT 13
SoftwareSerial SIM900(7,8); //Seleccionamos los pines 7 como Rx y 8 como Tx

typedef struct datos_struct{
    int elevacion;
    int azimutal;
    int intensidad;
} datos_t;

typedef unsigned char BYTE;
char calc_checksum(char v[], int cant);
void leer_checksum(const char v[], char CRC[]);
void sacar(void);
int check_data(char v[], const char tipo[]);  // segundo argumento recibe "GPGSV"
int datos_satelite(char v[], int numSat, datos_t *datos);

// funciones GSM
void comandosAT(datos_t *v, int cant);
void mostrarDatosSeriales();

const char buff_size = 100; 
char buff[buff_size];  
char index = 0;
boolean data_end = false;

void setup() {
  // el setup que se corre una sola vez
  Serial.begin(9600);
  sacar();
  SIM900.begin(9600); //Arduino se comunica con el SIM900 a una velocidad de 19200bps
//  SIM900.begin(19200); //Arduino se comunica con el SIM900 a una velocidad de 19200bps
  delay(1000); //Tiempo prudencial para que inicie sesión de red 
}

void loop() {
  datos_t datos[N];

  while (Serial.available() > 0)    // mientras haya datos disponibles
  {
    char inchar = Serial.read();    // leo del gps un byte
    buff[index] = inchar;           // lo guardo en un vector
    index++;
    buff[index] = 0;
    if(inchar == '\r'){ // se llegó al final de la linea
     // Serial.println(index);
      index = 0;
      data_end = true;    // se leyó una línea de datos
    }
    
    if(data_end == true)
    {   
        int result = check_data(buff, "GPGSV");
        if (result == 1)  // chequeamos que sea la línea que nos interesa
        {
            //datos_t datos;
            int k = 0;
            while(k < N)    // vamos llenando el vector de estructura
            {
                int result2 = datos_satelite(buff, SAT, &datos[k]);
                Serial.println("El resultado es:");
                Serial.println(result2);
                if(result2 == 1)    // guardamos datos
                {
                    Serial.print("El angulo de elevacion es: ");
                    Serial.println(datos[0].elevacion);
                    Serial.print("El angulo azimutal es: ");
                    Serial.println(datos[0].azimutal);
                    Serial.print("La intensidad es: ");
                    Serial.println(datos[0].intensidad);
                    k++;        
                }
                else
                {
                    Serial.println("no se leyó el satélite\n");
                }
            }
            // cuando sale del while es porque se llenó el vector, mandamos los datos
            comandosAT(&datos[0], N);
            if(SIM900.available())          // verificamos si hay datos disponibles desde el SIM900
            Serial.write(SIM900.read());    // escribimos los datos
        }

        data_end = false;
    }
        
    if(index == 1 && inchar != '$')
    {
      index = 0;
    }
    
    if(index == 100)
    {
      Serial.println("Se lleno el vector buff\n");
      index = 0;      // volvemos a empezar
    }
  }
  
}

char calc_checksum(char v[], int cant)
{
    char CRC = 0;
    char i;
    for (i = 1; v[i] != '*'; i++){ // XOR todos los caracteres sin contar el '$'
      CRC = CRC ^ v[i] ;
    }
    return CRC;
}

void sacar(void)
{
     const char * a_sacar[100] = {"GGA", "VTG", "RMC", "GSA", "TXT", NULL};
     BYTE i;
   //  Serial.println("holaaa");
     for (i = 0; a_sacar[i] != NULL; i++)
     {
        char  buff[100];
        sprintf(buff, "$PUBX,40,%s,0,0,0,0*", a_sacar[i]);
        char CRC;
        CRC = calc_checksum(buff, 100);     
        char v[100];
        sprintf(v, "%s%02X", buff, CRC);
          
        Serial.print((char *)v);
        Serial.print("\r\n");
        delay(1000); 
     }
}

void leer_checksum(const char v[], char CRC[])
{
    int i = 0;
    while(v[i] != '*')
    {
//      printf("%c", ejemplo[i]);
      i++;
    }
    i++;
    int j = 0;
    while(v[i] != '\r' && v[i] != '\n')
    {
        CRC[j] = v[i];
        j++;
        i++;
    }

}


int check_data(char v[], char const tipo[])
{
  // primero chequeamos que sea la linea que queremos: tipo
  int i = 0;
  char aux[10];
  //Serial.println(v);
  while(v[i] != ',')
  {
    aux[i] = v[i+1];
    i++;
    if (i == 9)
    {
      return 0;    // hubo algún error    
    }
  }
  aux[--i] = '\0';
  int result = strcmp(aux, tipo);
  if (result == 0)
  {
    // nos aseguramos que se cumpla el checksum
    char CRC_leido[10];       
    leer_checksum(v, CRC_leido);          // leemos el checksum
    
    char CRC_calc1 = calc_checksum(v, 100);     // calculamos el checksum
    char CRC_calc[10];       
    sprintf(CRC_calc, "%02X", CRC_calc1);       // pasamos a hex
 //   Serial.println("Comparacion de checksum:");
    if(strcmp(CRC_calc, CRC_leido) == 0)
      {
        // Serial.println("son iguales");
      }   
    if(strcmp(CRC_calc, CRC_leido) == 0)        // comparamos si son iguales
    {
      return 1;     // está todo bien
    }
    else
    {
      return 0;
    }
  }  
  else
  {
    return 0;   // falso
  }
}

int datos_satelite(char v[], int numSat, datos_t *datos)
{
    int imax = 0, i = 0;
    int cant_lineas, linea, cant_sat;
    char aux;
    char cant_lineasC[5], lineaC[5], cant_satC[5];
    int k = 0;
    while(imax < 4 && v[i] != '*')
    {
        aux = v[i];
        if (aux == ',')
        {
            imax++;
            switch (imax)
            {
              case 1:            // nos dice la cantidad de lineas
                while(v[i+1] != ',')
                {
                  cant_lineasC[k] = v[i+1];
                  i++;
                  k++;
                }
                cant_lineasC[k] = '\0';
                cant_lineas = atoi(cant_lineasC);       // pasamos a entero
                k = 0;
                break;
              case 2:           // en que linea estamos
                while(v[i+1] != ',')
                {
                  lineaC[k] = v[i+1];
                  i++;
                  k++;
                }
                lineaC[k] = '\0';
                linea = atoi(lineaC);       // pasamos a entero
                k = 0;
                break;
              case 3:           // nos dice cantidad de satelites a la vista
                while(v[i+1] != ',')
                {
                  cant_satC[k] = v[i+1];
                  i++;
                  k++;
                }
                cant_satC[k] = '\0';
                cant_sat = atoi(cant_satC);     // pasamos a entero
                k = 0;
                break;
            }
        }
        i++;
    }

    int sat[10] = {0}, aux2;
    char satC[5], elevacionC[5], azimutC[5], intensidadC[5];
    int num = 0;

    if(cant_lineas == linea)
    {

        num = cant_sat - (linea - 1)*4;        // cantidad de satelites en la linea
    }

    else
    {
        num = 4;

    }
    int j = 1;
    for(j = 1; j <= num; j++)
    {
        while(v[i] != ',')
        {
            satC[k] = v[i];
            i++;
            k++;
        }
        satC[k] = '\0';
        sat[j-1] = atoi(satC);     // pasamos a entero
        k = 0;
        imax = 0;
        Serial.print("Satelite");
        Serial.println(sat[j-1]);
        if (sat[j-1] == numSat)
        {
            while(imax < 4 && v[i] != '*')
            {
                aux = v[i];
                if (aux == ',')
                {
                    imax++;
                    switch (imax)
                    {
                        case 1:            // ángulo de elevación
                            while(v[i+1] != ',')
                            {
                                elevacionC[k] = v[i+1];
                                i++;
                                k++;
                            }
                            elevacionC[k] = '\0';
                            datos->elevacion  = atoi(elevacionC);
                            k = 0;
                            break;
                        case 2:           // ángulo azimutal
                            while(v[i+1] != ',')
                            {
                                azimutC[k] = v[i+1];
                                i++;
                                k++;
                            }
                            azimutC[k] = '\0';
                            datos->azimutal  = atoi(azimutC);
                            k = 0;
                            break;
                        case 3:           // intensidad
                            while(v[i+1] != ',' && v[i+1] != '*' )
                            {
                                intensidadC[k] = v[i+1];
                                i++;
                                k++;
                            }
                            intensidadC[k] = '\0';
                            datos->intensidad  = atoi(intensidadC);
                            k = 0;
                            break;
                    }
                    
                }
                i++;
            }
            return 1;
        }
        else
        {
            while(imax < 4 && v[i] != '*')
            {
                aux = v[i];
                if (aux == ',')
                {
                    imax++;
                }
                i++;
            }
        }
    }
    return 0;          // no se leyó el satélite
}

// Funciones para el GSM
void comandosAT(datos_t *v, int cant){
  SIM900.println("AT+CIPSTATUS"); //Consultar el estado acutal de la comunicación
  delay(2000);
  SIM900.println("AT+CIPMUX=0"); //comando configura el dispositivo para una conexión IP única o múltiple
  delay(3000);
  mostrarDatosSeriales(); //Llama a esta función
  SIM900.println("AT+CSTT=\"gprs.movistar.com.ar\",\"wap\",\"wap\"");//comando configura el APN, 
                                                                     //nombre de usuario y contraseña."gprs.movistar.com.ar","wap","wap"->Movistar Arg.
  delay(1000);
  mostrarDatosSeriales();  
  SIM900.println("AT+CIICR");// Realiza conexión inalámbrica GPRS
  delay(3000);
  mostrarDatosSeriales();
  SIM900.println("AT+CIFSR");// Obtenemos IP local
  delay(2000);  
  mostrarDatosSeriales();
  SIM900.println("AT+CIPSPRT=0");//Establece un indicador '>' al enviar datos
//  gps(); //Llama a la función de la cual va a tomar los datos para enviar

  // Nos armamos el string que queremos mandar
  char data[15*N];
  sprintf(data, "%d,%d,%d ", v[0].elevacion, v[0].azimutal, v[0].intensidad);
  for(int k = 1; k < N; k++)
  {
     sprintf(data + strlen(data), "%d,%d,%d ", v[k].elevacion, v[k].azimutal, v[k].intensidad);
  }
  delay(3000);
  mostrarDatosSeriales();
  SIM900.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//Indicamos el tipo de conexión, url o dirección IP y puerto al que realizamos la conexión
  delay(6000);
  mostrarDatosSeriales();
  SIM900.println("AT+CIPSEND");//Envía datos a traves de una conexión TCP 
  delay(4000);
  mostrarDatosSeriales();
  String datos="GET https://api.thingspeak.com/update?api_key=CIS3GDFM6KPAYH&field1=0" + String(data);  //donde queremos que envíe los datos, por ejemplo el ángulo
  SIM900.println(data);//Envía datos al servidor remoto
  delay(4000);
  mostrarDatosSeriales();
  SIM900.println((char)26);
  delay(5000);//Ahora esperaremos una respuesta pero esto va a depender de las condiones de la red y este valor quizá debamos modificarlo dependiendo de las condiciones de la red
  SIM900.println();
  mostrarDatosSeriales();
  SIM900.println("AT+CIPSHUT");//Cierra la conexión(Desactiva el contexto GPRS PDP)
  delay(5000);
  mostrarDatosSeriales();
} 

void mostrarDatosSeriales()   //Muestra los datos que va entregando el sim900
{
  while(SIM900.available()!=0)
  Serial.write(SIM900.read());
}
