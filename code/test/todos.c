#include "syscall.h"

int resultado;
char Salida[10];

void Verifica();

int main() {

    SpaceId i=0;
    int a = 0;

    resultado = -1;
    Salida[0] = 'n';
    Salida[1] = 'a';
    Salida[2] = 'c';
    Salida[3] = 'h';
    Salida[4] = 'o';
    Salida[5] = 's';
    Salida[6] = '.';
    Salida[7] = '1';
    Salida[8] = 0;
    i = Exec( "../test/copy" );
//    Write( "Espera por el copy ...\n", 23, 1 );
    a = Join( i );

    Fork( Verifica );
    Yield();
    if ( resultado >= 0 )
       Write("Bien!\n", 6, 1 );
    else
       Write( ":( \n", 4, 1 );
}

void Verifica() {

    OpenFileId prueba;

    Write( "Verificando ...\n", 16, 1 );
    prueba = Open( Salida );
    resultado = prueba;
    Yield();
    Close( prueba );
}

