/*********************************************
UNIVERSIDAD DEL VALLE DE GUATEMALA
CC3056 - Programación de Microprocesadores
Ciclo 2 - 2019
Autores: Gerardo Méndez y Diego Estrada
Fecha: 24/09/2019
Archivo: encrypt_pipelining.cpp
------------------Descripcion--------------------
    PROYECTO 3 - Encripción DES utilizando 4 rondas, 
    longitud de bloque 8. Se compila desde terminal de
    Raspberry
---------------------Build-----------------------
 	$ ./des -d inputFile outputFile keys
	$ ./des -e inputFile outputFile keys
--------------------Pendiente---------------------
- Implementar variables mutex
**********************************************/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <string>
#include <pthread.h>
using namespace std;

static string INPUT_FILENAME, OUTPUT_FILENAME, KEYS_FILENAME, BINARY_FILENAME = "bits.txt", BINARY_FILENAME_SECOND = "bitsDecrypt.txt";
FILE *input, *output, *keys, *binary;
#define BLOCK_CAPACITY 8        //Bytes
#define THREADS 3 
#define ROUNDS 16
#define ACTION_ENCRYPT "-e"
#define ACTION_DECRYPT "-d"

const char HELP[] =
  "\n"
  "./des  password  input  output keys\n"
  "\n"
  "ERROR ON COMMAND.\n"
  "\n"
  "\n"
  "For encrypt a file:\n"
  "\n"
  "   ./des password  message1.txt  message.cry key.txt\n"
  "For decrypt a file:\n"
  "\n"
  "   ./des password  message.cry  message2.txt key.txt\n"
  "\n"
  "The algorithm used is DES with a 56-bit key derived from the password.";

int IP[] = 
{
	  58, 50, 42, 34, 26, 18, 10, 2,
	  60, 52, 44, 36, 28, 20, 12, 4,
	  62, 54, 46, 38, 30, 22, 14, 6,
	  64, 56, 48, 40, 32, 24, 16, 8,
	  57, 49, 41, 33, 25, 17,  9, 1,
	  59, 51, 43, 35, 27, 19, 11, 3,
	  61, 53, 45, 37, 29, 21, 13, 5,
	  63, 55, 47, 39, 31, 23, 15, 7
};

int E[] = 
{
	  32,  1,  2,  3,  4,  5,
	   4,  5,  6,  7,  8,  9,
	   8,  9, 10, 11, 12, 13,
	  12, 13, 14, 15, 16, 17,
	  16, 17, 18, 19, 20, 21,
	  20, 21, 22, 23, 24, 25,
	  24, 25, 26, 27, 28, 29,
	  28, 29, 30, 31, 32,  1
};

int P[] = 
{
	  16,  7, 20, 21,
	  29, 12, 28, 17,
	   1, 15, 23, 26,
	   5, 18, 31, 10,
	   2,  8, 24, 14,
	  32, 27,  3,  9,
	  19, 13, 30,  6,
	  22, 11,  4, 25
};

int FP[] = 
{
	  40, 8, 48, 16, 56, 24, 64, 32,
	  39, 7, 47, 15, 55, 23, 63, 31,
	  38, 6, 46, 14, 54, 22, 62, 30,
	  37, 5, 45, 13, 53, 21, 61, 29,
	  36, 4, 44, 12, 52, 20, 60, 28,
	  35, 3, 43, 11, 51, 19, 59, 27,
	  34, 2, 42, 10, 50, 18, 58, 26,
	  33, 1, 41,  9, 49, 17, 57, 25
};

int S1[4][16] = 
{
		14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
		0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
		4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
		15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13
};

int S2[4][16] = 
{
	15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
	 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
	 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
	13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9
};

int S3[4][16] = 
{
	10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
	13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
	13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
	 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12
};

int S4[4][16] = 
{
	 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
	13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
	10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
	 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14
};

int S5[4][16] = 
{
	 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
	14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
	 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
	11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3
};

int S6[4][16] = 
{
	12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
	10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
	 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
	 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13
};

int S7[4][16]= 
{
	 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
	13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
	 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
	 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12
};

int S8[4][16]= 
{
	13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
	 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
	 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
	 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

int PC1[] = 
{
	  57, 49, 41, 33, 25, 17,  9,
	   1, 58, 50, 42, 34, 26, 18,
	  10,  2, 59, 51, 43, 35, 27,
	  19, 11,  3, 60, 52, 44, 36,
	  63, 55, 47, 39, 31, 23, 15,
	   7, 62, 54, 46, 38, 30, 22,
	  14,  6, 61, 53, 45, 37, 29,
	  21, 13,  5, 28, 20, 12,  4
};

int PC2[] = 
{
	  14, 17, 11, 24,  1,  5,
	   3, 28, 15,  6, 21, 10,
	  23, 19, 12,  4, 26,  8,
	  16,  7, 27, 20, 13,  2,
	  41, 52, 31, 37, 47, 55,
	  30, 40, 51, 45, 33, 48,
	  44, 49, 39, 56, 34, 53,
	  46, 42, 50, 36, 29, 32
};

int SHIFTS[] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

FILE* out;
int backup[17][2];
int C[17][28], D[17][28];
int LEFT[17][32], RIGHT[17][32];
int IPtext[64];
int EXPtext[48];
int XORtext[48];
int X[8][6];
int X2[32];
int R[32];
int key56bit[56];
int key48bit[17][48];
int CIPHER[64];
int ENCRYPTED[64];
int CD[17][56];
pthread_t ptids[ROUNDS*2];
pthread_mutex_t mutexLock;
pthread_cond_t decrypt_cv;
pthread_attr_t attr;

/*
Nombre: expansion_function
Parámetros: posicion, valor
Objetivo: expandir cadena hasta 48 bits
*/
void expansion_function(int pos, int text)
{
	for (int i = 0; i < 48; i++)
		if (E[i] == pos + 1)
			EXPtext[i] = text;
}

/*
Nombre: initialPermitation
Parámetros: posición, valor
Objetivo: realizar permutación inicial de Data Encryption Standard
*/
int initialPermutation(int pos, int text)
{
	int i;
	for (i = 0; i < 64; i++)
		if (IP[i] == pos + 1)
			break;
	IPtext[i] = text;
}

/*
Nombre: F1
Parámetros: index
Objetivo: asignar S-box
*/
int F1(int i)
{
	int r, c, b[6];
	for (int j = 0; j < 6; j++)
		b[j] = X[i][j];

	r = b[0] * 2 + b[5];
	c = 8 * b[1] + 4 * b[2] + 2 * b[3] + b[4];
	if (i == 0)
		return S1[r][c];
	else if (i == 1)
		return S2[r][c];
	else if (i == 2)
		return S3[r][c];
	else if (i == 3)
		return S4[r][c];
	else if (i == 4)
		return S5[r][c];
	else if (i == 5)
		return S6[r][c];
	else if (i == 6)
		return S7[r][c];
	else if (i == 7)
		return S8[r][c];
}

/*
Nombre: XOR
Parámetros: a (cadena), b(key)
Objetivo: realizar un XOR entre dos datos
*/
int XOR(int a, int b)
{
	return (a ^ b);
}

/*
Nombre: ToBits
Parámetros: valor entero
Objetivo: convertir a bits el entero provisto
*/
int ToBits(int value)
{
	int k, j, m;
	static int i;
	if (i % 32 == 0)
		i = 0;
	for (j = 3; j >= 0; j--) 
	{
		m = 1 << j;
		k = value & m;
		if (k == 0)
			X2[3 - j + i] = '0' - 48;
		else
			X2[3 - j + i] = '1' - 48;
	}
	i = i + 4;
}

/*
Nombre: SBox
Parámetros: cadena de 48 bit
Objetivo: utilizar SBox a una cadena
*/
int SBox(int XORtext[])
{
	int k = 0;
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 6; j++)
			X[i][j] = XORtext[k++];

	int value;
	for (int i = 0; i < 8; i++) 
	{
		value = F1(i);
		ToBits(value);
	}
}

/*
Nombre: PBox
Parámetros: posición, valor
Objetivo: Reorganización con P[]
*/
int PBox(int pos, int text)
{
	int i;
	for (i = 0; i < 32; i++)
		if (P[i] == pos + 1)
			break;
	R[i] = text;
}

/*
Nombre: cipher
Parámetros: ronda, tipo de cifrado
Objetivo: Cifrar cadena (right)
*/
void cipher(long k)
{
	long Round = (long) k;
 	//Se realiza la expasión para aumentar la cadena de 32 a 48 bits y así pode aplicarle
	//la llave que también es de 48 bits con un XOR
	for (int i = 0; i < 32; i++)
		expansion_function(i, RIGHT[Round - 1][i]);

	//Aplicación de la operación lógica XOR entre la cadena de 48 bits de la derecha y la llave 
	//correspondiente a la ronda
	for (int i = 0; i < 48; i++) 
	{
		XORtext[i] = XOR(EXPtext[i], key48bit[17 - Round][i]);
	}
    
	//Aplicación de S-box a la cadena con XOR aplicado
	SBox(XORtext);

	//Permutacion
	for (int i = 0; i < 32; i++)
		PBox(i, X2[i]);
	//XOR entre el lado derecho e izquierdo
	for (int i = 0; i < 32; i++)
		RIGHT[Round][i] = XOR(LEFT[Round - 1][i], R[i]);
}

/*
Nombre: finalPermutation
Parámetros: posicion, valor
Objetivo: permutar cadena para encriptar
*/
void finalPermutation(int pos, int text)
{
	int i;
	//Permutación final en función del arreglo FP[]
	for (i = 0; i < 64; i++)
		if (FP[i] == pos + 1)
			break;
	ENCRYPTED[i] = text;
}

/*
Nombre: convertToBinary
Parámetros: entero a convertir
Objetivo: pasar un entero a binario
*/
void convertToBinary(int n)
{
	int k, m;
	//se evalua cada valor binario de ASCII (8 bits)
	for (int i = 7; i >= 0; i--) 
	{
		m = 1 << i;
		k = n & m;
		//se escribe el valor 1 o 0 en la referencia global al archivo que guardara los valores binarios
		if (k == 0)
			fprintf(binary, "0");
		else
			fprintf(binary, "1");
	}
}

/*
Nombre: convertCharToBit
Parámetros: numero de caracteres en el archivo de entrada
Objetivo: realizar la conversión de los caracteres del archivo de entrada a numeros binarios.
*/
int convertCharToBit(long int n)
{
	//Se abre el archivo de entrada como lectura
	FILE* inp = fopen(INPUT_FILENAME.c_str(), "rb");
	//Se abre el archivo que contendrá el texto traducido en binario en forma de escritura
	binary = fopen(BINARY_FILENAME.c_str(), "wb+");
	char ch;
	//Se multiplica por 8 para determinar la cantidad total de caracteres en el archivo de entrada
	int i = n * 8;
	while (i) 
	{
		//Se lee caracter por caracter el archivo de entrada gracias a al función fgetc()
		ch = fgetc(inp);
		if (ch == -1)
			break;
		i--;
		convertToBinary(ch);
	}
	//Salida de los archivos
	fclose(binary);
	fclose(inp);
}

/*
-Funcion para desencriptar una cadena
-Parametro : cadena de bits plana
-Retorno : --
*/
void Decryption(long int plain[])
{
	//se abre el archivo que contendrá los bits descifrados
	binary = fopen(BINARY_FILENAME_SECOND.c_str(), "ab+");
	//se realiza la permutación inicial
	for (int i = 0; i < 64; i++)
		initialPermutation(i, plain[i]);

	//Se separa la cadena de 64 bits en dos cadenas de 32 bits
	//una para la izquierda y otra para la derecha
	for (int i = 0; i < 32; i++)
		LEFT[0][i] = IPtext[i];

	for (int i = 32; i < 64; i++)
		RIGHT[0][i - 32] = IPtext[i];

	//Descifrado de los valores
	for (int k = 1; k < 17; k++) {
		cipher(k);

		for (int i = 0; i < 32; i++)
			LEFT[k][i] = RIGHT[k - 1][i];
	}

	//permutacion final
	for (int i = 0; i < 64; i++) 
	{
		if (i < 32)
			CIPHER[i] = RIGHT[16][i];
		else
			CIPHER[i] = LEFT[16][i - 32];
		finalPermutation(i, CIPHER[i]);
	}
	//Escritura de los valores en el archivo correspondiente
	for (int i = 0; i < 64; i++)
		fprintf(binary, "%d", ENCRYPTED[i]);
	//cierre del archivo de escritura
	fclose(binary);
}

/*
-Funcion para convertir la clave de 56 bits a 48 bits y asi poderla usar en cada ronda.
	estas claves son almacendas en el arreglo de 48 bits
-Parametros : fila, columna y valor  
-Retorno: --
*/
void key56to48(int round, int pos, int text)
{
	//Algoritmo D-box compression
	int i;
	//Se recorre todo el arreglo PC2 que determinar el orden de compresión
	for (i = 0; i < 56; i++)
		if (PC2[i] == pos + 1)
			break;
	//Asignacion del valor al arreglo que contendrá las claves
	key48bit[round][i] = text;
}

/*
-Funcion para convertir la clave leida de 64 bits a 56 bits
-Parametros : posición el bit en el arreglo, valor del bit 
-Retorno: 
*/
int key64to56(int pos, int text)
{
	int i;
	for (i = 0; i < 56; i++)
		if (PC1[i] == pos + 1)
			break;
	key56bit[i] = text;
}

void* shift1(void* arg)
{	
	pthread_mutex_lock(&mutexLock);
	int k;
	long number = (long) arg;
	int n = (int) number;
	int shift, x;
	
	if((n - 100) > 100)
	{
		shift = 2;
		x = n - 200;
	}
	else
	{
		shift = 1;
		x = n - 100;
	}
	
	//Se obtienen el primer o los dos primeros valores del arreglo y se guardan en backup
	for (int i = 0; i < shift; i++) backup[x - 1][i] = C[x - 1][i];
		
	//Este ciclo asigna los valores de la fila anterior a la posterio sin contar el primer valor
	//o los dos primeros (esto depende del shift)
	for (int i = 0; i < (28 - shift); i++) C[x][i] = C[x - 1][i + shift];
	k = 0;
	
	//el o los valores tomados ahora se agregan a la parte final del arreglo
	//y es así como se completa para la parte izquierda el desplazamiento a la izquierda
	for (int i = 28 - shift; i < 28; i++) C[x][i] = backup[x - 1][k++];
			
	for (int i = 0; i < 28; i++) CD[x][i] = C[x][i];
	pthread_mutex_unlock(&mutexLock);
}

void* shift2(void* arg)
{	
	pthread_mutex_lock(&mutexLock);
	int k;
	long number = (long) arg;
	int n = (int) number;
	int shift, x;
	
	if((n - 100) > 100)
	{
		shift = 2;
		x = n - 200;
	}
	else
	{
		shift = 1;
		x = n - 100;
	}
	 
	//Se obtienen el primer o los dos primeros valores del arreglo y se guardan en backup
	for (int i = 0; i < shift; i++) backup[x - 1][i] = D[x - 1][i];
		
	//Este ciclo asigna los valores de la fila anterior a la posterio sin contar el primer valor
	//o los dos primeros (esto depende del shift)
	for (int i = 0; i < (28 - shift); i++) D[x][i] = D[x - 1][i + shift];
	k = 0;
	
	//el o los valores tomados ahora se agregan a la parte final del arreglo
	//y es así como se completa para la parte derecha el desplazamiento a la izquierda
	for (int i = 28 - shift; i < 28; i++) D[x][i] = backup[x - 1][k++];
			
	for (int i = 28; i < 56; i++) CD[x][i] = D[x][i - 28];
	pthread_mutex_unlock(&mutexLock);
}

/*
-Funcion para convertir la clave leida de 64 bits a 48 bits
-Parametros : arreglo que contiene la clave en binario (longitud de 64 bits)
-Retorno : --
*/
void key64to48(unsigned int key[])
{
	//Ciclo que realiza la caida de paridad, conversión de 64 a 56 bits
	for (int i = 0; i < 64; i++)
		key64to56(i, key[i]);

	//División de la cadena de 56 bits en dos mitades de 28 bits cada una
	for (int i = 0; i < 56; i++)
		if (i < 28)
			C[0][i] = key56bit[i];
		else
			D[0][i - 28] = key56bit[i];

	//Desplazamiento a la izquirda determinado por el arreglo SHIFTS
	//en este ciclo se realiza un desplazamiento circular de las dos partes
	//del arreglo de 56 bits
	for (int x = 1; x < 17; x++) 
	{
		//nos indicará la cantidad de desplazamientos a la izquierda que se deben hacer (1 o 2)
		int shift = SHIFTS[x - 1];
		long r = (shift*100) + x;
		
		//creacion de pthreads, uno por cada parte (left, right) de la llave
		pthread_create(&(ptids[16-x]), NULL, shift1, (void *)r);
		pthread_create(&(ptids[32-x]), NULL, shift2, (void *)r);
		
		//join de los pthreads
		pthread_join(ptids[16-x], NULL);
		pthread_join(ptids[32-x], NULL);
	}

	//Conversión de las claves de 56 bits generadas a 48 bits para que estas sean aplicadas
	//en cada ronda
	
	for (int j = 1; j < 17; j++)
		for (int i = 0; i < 56; i++)
			key56to48(j, i, CD[j][i]);
}

/*
-Funcion para convertir los caracteres en numeros binarios
-Parametro : arreglo de caracteres
-Retorno : --
*/
void convertToBits(int ch[])
{
	int value = 0;
	for (int i = 7; i >= 0; i--)
		value += (int)pow(2, i) * ch[7 - i];
	fprintf(output, "%c", value);
}

/*
-Funcion para convertir los valores en binario en caracteres ASCII
-Parametro :--
-Retorno : --
*/
int bittochar()
{
	output = fopen(OUTPUT_FILENAME.c_str(), "ab+");
	for (int i = 0; i < 64; i = i + 8)
		convertToBits(&ENCRYPTED[i]);
	fclose(output);
}

void decrypt(long int n)
{
	FILE* in = fopen(INPUT_FILENAME.c_str(), "rb");
	long int plain[n * 64];
	int i = -1;
	char ch;

	while (!feof(in)) 
	{
		ch = getc(in);
		plain[++i] = ch - 48;
	}
	
	for (int i = 0; i < n; i++) 
	{
		Decryption(plain + i * 64);
		bittochar();
	}
	fclose(in);
}


/*
-Funcion para generar las claves
	En esta funcion se lee la llave contenida en un archivo de texto especificado
-Parametro : --
-Retorno : No hay retorno
*/
void create16Keys()
{
	//abertura del archivo
	FILE* pt = fopen(KEYS_FILENAME.c_str(), "rb");
	//arreglo que guardara la clave de 64 bits leida del archivo
	unsigned int key[64];
	//manejadores
	int i = 0, ch;

	//lectura hasta encontrar el final del texto
	while (!feof(pt)) 
	{	
		//se toma cada caracter binario que se encuentra en el archivo y se
		//agrega al arreglo de las llaves (key[])
		ch = getc(pt);
		key[i++] = ch - 48;
	}

	//transformación de la llave de 64 bits a 48 bits
	//se el pasa como parametro la llave leida obtenida del archivo 
	key64to48(key);
	//salida correcta del archivo
	fclose(pt);
}

/*
-Funcion que determina el tamaño del archivo
-Parametro : --
-Retorno : El tamaño del archivo "input.txt" leido (cantidad de caracteres)
*/
long int findFileSize()
{
	//se abre el archivo como lectura
	FILE* inp = fopen(INPUT_FILENAME.c_str(), "rb");
	//variable que guardara el tamaño
	long int size;
	if (fseek(inp, 0L, SEEK_END))
		perror("fseek() failed");
	else //con la funcion ftell() se contara el núero de caracteres dentro del archivo
		size = ftell(inp);
	
	//Si el archivo tiene una cantidad de bits que no es divisible entre 8, se rellena con espacios
	int count_space = 8 - (size % 8);
	if(size % 8 != 0){
		int i = 0;
		while(i < count_space) {
			fprintf(inp, "%c", " ");
			i++;
		}
	}
	//cierre del archivo
	fclose(inp);
	//retorno
	long int totalSize = (size + count_space);
	return  totalSize;
}


static void error_exit(const char *format, ...)
{
  va_list argptr;
  fprintf(stderr, "\ndes: ");
  va_start(argptr, format);
  //vfprintf(stderr, format, (va_list)*(&format + 1));
  vfprintf(stderr, format, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
  exit(1);
}



int main(int argc, char **argv)
{
	if (argc < 4)
	{
		puts(HELP);
		return 1;
	}

	input = fopen(argv[2], "rb");
	if (input == NULL)
		error_exit("Can't open %s", argv[2]);
	fclose(input);

	output = fopen(argv[3], "wb+");
	if (output == NULL)
		error_exit("Can't open %s", argv[3]);
	fclose(output);	
	
	keys = fopen(argv[4], "rb");
	if (keys == NULL)
		error_exit("Can't open %s", argv[3]);
	fclose(keys);

	// destroy contents of these files (from previous runs, if any)

	//Setting variables
	INPUT_FILENAME = argv[2];
	OUTPUT_FILENAME = argv[3];
	KEYS_FILENAME = argv[4];

    pthread_mutex_init(&mutexLock, NULL);

	//creacion de las 16 llaves de encripción
	create16Keys();
	
	
	//se calcula la cantidad de caracteres dentro del archivo de lectura y se divide en 8 
	long int fileSize = findFileSize() / 8;

	convertCharToBit(fileSize);

	if (!strcmp(argv[1], ACTION_DECRYPT))
	{
		//Funcion de encripcion
		decrypt(fileSize);
	}
	else
	{
		puts(HELP);
		return 1;
	}
	
	pthread_mutex_destroy(&mutexLock);
	pthread_exit(NULL);
	return 0;
}
