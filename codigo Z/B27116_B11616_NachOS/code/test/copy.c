#include "syscall.h"

int
main()
{
    OpenFileId input;
    OpenFileId output;
    char buffer[1024];
    int n = 0;


    Create( "nachos.2" );
    input  = Open( "nachos.1" );
    output = Open( "nachos.2" );
    while( (n = Read( buffer, 1024, input ) ) > 0 ) {
	Write( buffer, n, output);
    }
    Close( input );
    Close( output );

    Exit( 0 );
}

