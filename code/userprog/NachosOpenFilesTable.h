#define largoVector 250 //Valor predefinido del largo del vector.
#include "bitmap.h"
#include <stdio.h>

class NachosOpenFilesTable {

  public:
    NachosOpenFilesTable();       // Constructor de la clase
    ~NachosOpenFilesTable();      // Destructor de la clase
    
    int Open( int UnixHandle ); // Registra el handle del archivo
    int Close( int NachosHandle );      // Desregistra el handle del archivo
    bool isOpened( int NachosHandle );   //Determina si el archivo esta abierto segun el handle del nacho.
    int getUnixHandle( int NachosHandle ); //Obtiene el handle de unix de acuerdo el handle de nachOS.
    void addThread();		// Aumenta en uno el valor de usage.
    void delThread();		// Disminuye en uno el valor de usage.

    void Print();               // Imprime el contenido del vector
    BitMap * openFilesMap;	// Bitmap que se utiliza para determinar los espacios usados en el vector
  private:
    int * openFiles;		// Vector que almacena los archivos abiertos

    int usage;			// Determina la cantidad de hilos que esta utilizando la tabla.

};

