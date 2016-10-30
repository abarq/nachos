// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>

using namespace std;
//En Exit() debe haber un Signal de un semaforo para que le 'avise' a Join() para que siga ejecutandose.


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

//Actualiza el valor de los registros de la clase machine
void returnFromSystemCall() {

        int pc, npc;

        pc = machine->ReadRegister( PCReg );
        npc = machine->ReadRegister( NextPCReg );
        machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
        machine->WriteRegister( PCReg, npc );           // PC <- NextPC
        machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4

}  

char* leerEntrada(){

	int value = 1;
	char* buffer = new char[20];
	int i=0;
	do{

		machine->ReadMem(machine->ReadRegister(4)+i,1,&value);
		buffer[i] = (char)value;
		i++;

	}while(buffer[i-1]!='\0');

	buffer[i] = '\0';
	return buffer;

}


//Metodo Open de nachos, redireccionado del systemCall Open de NachOS.
void Nachos_Open(){

	char* buffer=NULL; //Crea un buffer en el cual se almacenara el nombre del archivo que se abrira.

	buffer = leerEntrada();
	printf("El buffer es: %s\n", buffer);
	printf("\n");
	int descriptor = open(buffer,O_RDWR);   //Llamado de open de Unix, devuelve un descriptor de la tabla de archivos abiertos.

	char* string = new char[20];
	int byteLeidos = read(descriptor,string,5);
	printf("ByLei en Open : %d\n", byteLeidos);
	int respuestaOpen = currentThread->openFilesTable->Open(descriptor); //Devuelve si el metodo opn fue exitoso.
	currentThread->openFilesTable->Print();
	printf("%d\n", respuestaOpen);

	if(respuestaOpen == -1){  //En caso de que devuelve

		printf("Ocurrió un error a la hora de abrir el archivo\n");
		exit(0); // Llama el system call exit() de nachOS para 

	}
	printf("Antes de stats\n");
	stats->numDiskReads++; //Actualiza las estadisticas de las lecturas en el disco.
	printf("Antes de returnFromSystemCall\n");
	machine->WriteRegister(2,respuestaOpen);
	printf("FINAL FINAL\n");
	
}


void Nachos_Write(){

        char * buffer = NULL;
        int size = machine->ReadRegister( 5 );	// Lee el tamaño a escribirRead size to write

        // buffer = Read data from address given by user;
        OpenFileId id = machine->ReadRegister( 6 );

	switch (id) {
		case  ConsoleInput:	// User could not write to standard input
			
			machine->WriteRegister( 2, -1 );
			break;

		case  ConsoleOutput:

			buffer = leerEntrada();
			printf( "%s", buffer );
			delete[]buffer;

			break;

		case ConsoleError:	// This trick permits to write integers to console

			printf( "%d\n", machine->ReadRegister( 4 ) );
			break;

		default:	// All other opened files
			

			if(!(currentThread->openFilesTable->isOpened(id))){
				
				machine->WriteRegister(2,-1);

			}else{
			
      				char* buffer2 = NULL;            // Buffer donde se almacenan temporalmente los datos
				buffer2 = leerEntrada();

				int unixHandle = currentThread->openFilesTable->getUnixHandle(id);

				int cantidadCaracteresEscritos = write(unixHandle, buffer2, size);
				printf("la cantidad de caracteres escritos es: %d\n",unixHandle);
				stats->numDiskWrites++; //Actualiza las estadisticas de escritura en stats.
	
			break;

			}

	}

}    


void Nachos_Read(){


	int bytesLeidos;

	OpenFileId id = machine->ReadRegister( 6 );
	printf("El id es: %d\n", id);

	if((id<0) || (id == ConsoleOutput)||(!(currentThread->openFilesTable->openFilesMap->Test(id)))){

		printf("El openFile no es valido");
		bytesLeidos = -1;

	}else{

		char* buffer = (char*)machine->ReadRegister(4);             // Direccion donde se guardara los datos leidos
		int unixHandle = currentThread->openFilesTable->getUnixHandle(id);
		int size = machine->ReadRegister( 5 );
		char* temp = new char[size];

		bytesLeidos = read(unixHandle, temp, size);
	
		printf("La cantidad de bytes leidos son: %d\n",bytesLeidos);

		//for(int i = 0; i < size; i++){ 

         	//	machine->WriteMem((int)*(buffer + i), 1, (int)temp[i]);     // Hay que guardar en memoria byte por byte los datos
      		//}
		
		delete[] temp;
		

	}

		printf("La cantidad de caracteres leidos es: %d\n", bytesLeidos);
		fflush(stdout);
		machine->WriteRegister(2,bytesLeidos);


}

void Nachos_Close(){

	int nachosHandle;

	if(machine->ReadMem(4,1,&nachosHandle)){

		int unixHandle = currentThread->openFilesTable->getUnixHandle(nachosHandle);
	
		if(close(unixHandle)==0){

			if(currentThread->openFilesTable->Close(nachosHandle)==1){

				printf("El archivo se cerro correctamente");

			}	


		}else{

			printf("No se concreto el Close");

		}



	}


}


void Nachos_Exit(){

	int exitStatus = machine->ReadRegister(4);

}


void Nachos_Halt(){

	interrupt->Halt();
}

void Nachos_Create(){

	char* buffer = leerEntrada();
	fileSystem->Create(buffer,0);
	delete[]buffer;


}


void Nachos_Join(){


	returnFromSystemCall();
}

void Nachos_Exec(){

	char* nombreEjecutable = leerEntrada();
	OpenFile *executable = fileSystem->Open(nombreEjecutable); 	

  	if (executable == NULL) { 
                                    // Si el archivo no existe 
		printf("No se puede abrir el archivo %s\n", nombreExe);
      	  machine->WriteRegister(2, -1);                            // Indica retorna un valor ilegal
		return -1;

   	}

 	delete executable;   

    	int id = pageTableMap->Find();

    	if(id != -1){                   // Si el archivo existe
        
       		semExec->P();                                     // Semaforo tipo mutex para proteger la variable nombreEx
       		strcpy(nombreEx, nombreExe);                      // Guardo el nombre a una variable global
       		hilosActuales[id] = new Thread("Nuevo hilo");     // Creo un nuevo hilo
       		hilosActuales[id]->id = id;                       // Le asigno el id
       		hilosActuales[id]->Fork(ExeAux, 1);               // Pongo a correr el hilo
       		printf("Se puso a correr el archivo\n");

    	}else{

       		 printf("No hay espacio para abrir el archivo %s\n", nombreEx);
        	delete executable;
   	 }

   	 machine->WriteRegister(2, id);
   	 return id;


}

void ExeAux (int nada){

    printf("Se esta ejecutando el archivo %s\n", nombreEx);

    OpenFile *executable = fileSystem->Open(nombreEx);       // Abro el ejecutable
    AddrSpace *space = new AddrSpace(executable);
    currentThread->space = space;                            // Le asigno el address space

    delete executable;

    semExec->V();                       // Permito cambios a la variable nombreEx

    space->InitRegisters();		// Le da el valor inicial a los registros
    space->RestoreState();		// Indica donde esta su page table
    machine->Run();			// Corre el programa de usuario
    

}




void Nachos_Yield(){

	currentThread->Yield();

}

void Nachos_Fork(){


	returnFromSystemCall();
}

int Nachos_SemCreate(){

   int pos = bitmapSemaforos->Find();             // Busca una posicion libre en el Bitmap 

    if(pos!=-1){                                     // Si hay espacio

        Semaphore* nuevoSem = new Semaphore("Sem", machine->ReadRegister(4));    // crea el nuevo semaforo con su respectivo valor inicial
        semaforosActuales[pos] = nuevoSem;                                       // y lo guarda
        printf("Se creo un semaforo con id %d\n", pos);

    }else{

        printf("No hay espacio para crear el semaforo\n");
    }

    machine->WriteRegister(2, pos);                  // Guarda en el registro 2 el id


}



int Nachos_SemDestroy(){

    int devolver;
    int id = machine->ReadRegister(4);                   // Id del semaforo a destruir

    if((id > 20)||(id < 0)||(bitmapSemaforos->Test(id))){                     // Si el id es valido

        delete semaforosActuales[id];                    // destruye el semaforo
        bitmapSemaforos->Clear(id);                    // y libera espacio en el BitMap

        printf("Se destruyo el semaforo con id %d\n", id);
        devolver = 0;

    }else{
        
        printf("No se puede destruir el semaforo porque el id no es valido\n");
        devolver = -1;
    }
    
    machine->WriteRegister(2, devolver);              // Guarda en el registro 2 el resultado


}

int Nachos_SemSignal(){

    int devolver;
    int id = machine->ReadRegister(4);                // Id del semaforo

    if((id > 20)||(id < 0)||(bitmapSemaforos->Test(id))){        // Si el id es valido

        semaforosActuales[id]->V();                   // envia un signal al semaforo

        printf("Se envio un signal al semaforo con id %d\n", id);
        devolver = 0;

    }else{
        
        printf("No se puede enviar el signal porque el id del semaforo no es valido\n");
        devolver = -1;
    }

    machine->WriteRegister(2, devolver);              // Guarda en el registro 2 el resultado



}


int Nachos_SemWait(){

    int devolver;
    int id = machine->ReadRegister(4);                // Id del semaforo

    if((id > 20)||(id < 0)||(bitmapSemaforos->Test(id))){                  // Si el id es valido

        semaforosActuales[id]->P();                   // envia un wait al semaforo

        printf("Se envio un wait al semaforo con id %d\n", id);
        devolver = 0;

    }else{
        
        printf("No se puede enviar el wait porque el id del semaforo no es valido\n");
        devolver = -1;
    }

    machine->WriteRegister(2, devolver);              // Guarda en el registro 2 el resultado



}

/**Metodo que maneja las excepciones en el sistema NachOS
*Se implementa por el momento para el caso de SyscallException
*/

void ExceptionHandler(ExceptionType which){

	int type = machine->ReadRegister(2); //Lee el registro 2, el cual indica el tipo de exception a manejar
	switch (which){

	case SyscallException:

          switch ( type ) {

             case SC_Halt:
                Nachos_Halt();             // System call # 0
		returnFromSystemCall();
                break;

             case SC_Open:
                Nachos_Open();             // System call # 5
		returnFromSystemCall();
                break;

             case SC_Write:
                Nachos_Write();             // System call # 7
		returnFromSystemCall();
                break;

	     case SC_Close:		    // System call # 8
		Nachos_Close();
		returnFromSystemCall();
		break;

	    case SC_Exit:
		Nachos_Exit();
		returnFromSystemCall();
		break;

	    case SC_Read:
		Nachos_Read();
		returnFromSystemCall();
		break;
	    case SC_Create:
		Nachos_Create();
		returnFromSystemCall();
		break;
	    case SC_Fork:
		Nachos_Fork();
		returnFromSystemCall();
		break;
	    case SC_Join:
		Nachos_Join();
		returnFromSystemCall();
		break;
	    case SC_Exec:
		Nachos_Exec();
		returnFromSystemCall();
		break;
	    case SC_Yield:
		Nachos_Yield();
		returnFromSystemCall();
		break;			
	    case SC_SemCreate:
		Nachos_SemCreate();
		returnFromSystemCall();
		break;	
	    case SC_SemDestroy:
		Nachos_SemDestroy();
		returnFromSystemCall();
		break;

	    case SC_SemSignal:
		Nachos_SemSignal();
		returnFromSystemCall();
		break;
	    case SC_SemWait:
		Nachos_SemWait();
		returnFromSystemCall();
		break;

            default:
                printf("Unexpected syscall exception %d\n", type ); //Salida por defecto para system calls no implementados.
          
                break;
          }

       break;

       default:
          printf( "Unexpected exception %d\n", which );

          break;

	}

}


