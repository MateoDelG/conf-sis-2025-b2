#include <Arduino.h>


#define VALOR_CONSTANTE 10 // constantes que no cambIAn su valor durante la ejecución del programa

const float pi = 3.14159; // constantes que no cambIAn su valor


bool variable_bool = false; // variables que solo pueden tomar dos valores (0 o 1)
int variable_int = 0; // variables que pueden tomar valores enteros (negativos o positivos) del tamaño de un byte (8 bits)
float variable_float = 0.0; // variables que pueden tomar valores reales (decimales) del tamaño de un byte (8 bits)
char variable_char = 'a'; // variables que pueden tomar un solo caracter (letra, número o símbolo) del tamaño de un byte (8 bits)
String variable_string = "Hola"; // variables que pueden tomar una cadena de caracteres (texto) del tamaño de un byte (8 bits)

char mensaje[] = "mensaje#recibido";

void setup() {
  Serial.begin(115200); // Inicializa la comunicación serie a 115200 bps

  for (int i = 0; mensaje[i] != '\0'; i++) {
    Serial.println(mensaje[i]);
    if(mensaje[i] == '#') {
      Serial.println("partir mensaje");
    } 
  }
}

void loop() {

}
