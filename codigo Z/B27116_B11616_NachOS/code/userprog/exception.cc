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
#include "synch.h"
#include "console.h"
#include <unistd.h>

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

// Funciones axuliares a los llamados del sistema.
void NachosForkThread(void *name);
void ExecAux(void* execut);

// Se utiliza como BUFFER para almacenar los parámetros que deben interpretarse como hileras.
char mensaje[1024];
//AddrSpace* space;

AddrSpace* space;

/*
void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(false);
    }
}
*/

// Función encargada de manejar los registros para lograr el uso correcto de los llamados del sistema.
void returnFromSystemCall() 
{
    int pc, npc;
    pc = machine->ReadRegister(PCReg);
    npc = machine->ReadRegister(NextPCReg);
    // PrevPC <- PC
    machine->WriteRegister(PrevPCReg, pc);
    // PC <- NextPC
    machine->WriteRegister(PCReg, npc);
    // NextPC <- NextPC + 4
    machine->WriteRegister(NextPCReg, npc + 4);
}


// Del registro de entrada (4), copia caracter por caracter y lo almacena en mensaje.
void copyString(int reg)
{
    int c=1;
    int addrs;
    addrs = machine->ReadRegister(reg);
    for(int i=0; c!=0; ++i)
    {
        // (direccion, tamaño, value).
        machine->ReadMem(addrs+i, 1, &c);
        mensaje[i] = (char)c;
    }
}

/*---------------------------- Implementación de los llamados del sistema ----------------------------*/

/*
*  System Call 0: Halt.
*  Efecto: Interrumpe la ejecución del proceso.
*/
void Nachos_Halt() 
{                   
    DEBUG('S', "Estamos haciendo Halt!\n");
    DEBUG('S', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
    returnFromSystemCall();
}

/* 
* System Call 1: Exit.
* Efecto: finaliza la ejecución de un proceso.
*/
void Nachos_Exit(/*int estadoActual*/) 
{
    DEBUG('S', "Estamos haciendo Exit!\n");
    int id = machine->ReadRegister(4);
    currentThread->Yield();
    
    currentThread->procTabla.delThread();
    // No hay más procesos, borrar todo.
    if(currentThread->procTabla.getUsage() == 0) 
    { 
        Nachos_Halt();
    }
    if (space != NULL)
    {
      delete space;
      space = NULL;
    }

    Semaphore *sem;									
    while (currentThread->semTabla.getUsage() != 0) 
    {
        // Saco el id del semáforo para luego hacer signal con éste.
        sem = (Semaphore*)currentThread->semTabla.getUnixHandle(id); 
        currentThread->semTabla.delThread();
        sem->V();	
    }
    currentThread->Finish();
    returnFromSystemCall();
}

/*
* System Call 2: Exec.
* Efecto: Abre el programa desde el archivo especificado, y crea un nuevo espacio para él usando el fork.
*/
void Nachos_Exec(/*char* fileName*/)
{
    DEBUG('S', "Estamos haciendo Exec!\n");
    int cont = 1;
    // Iniciamos la obtención del nombre del archivo.
    int addrs = machine->ReadRegister(4);
    for(int i= 0; cont != 0; ++i)
    {
        machine->ReadMem(addrs+i, 1, &cont);
        // Se guarda el nombre del archivo en el BUFFER. 
        mensaje[i] = (char)cont;
    }
    int uH = creat(mensaje, S_IRWXU|S_IRWXO|S_IRWXG);
    printf("Archivo a abrir %s, UnixHandler %d.\n", mensaje, uH);	 
    OpenFile* executable = fileSystem->Open(mensaje);
    Thread* newT = new Thread(mensaje);
    newT->Fork(ExecAux, (void*)executable);
    returnFromSystemCall();
}

// Función auxiliar para Nachos_Exec.
void ExecAux(void* execut)
{
    DEBUG('S', "Estamos haciendo ExecAux!\n");
    // El id del espacio ocupado por el proceso.
    SpaceId pid = 0;	
    OpenFile* executable = (OpenFile*)execut;
    if(executable == NULL)
    {
        printf("No es posible abrir el archivo %s.\n", mensaje);
        // Terminamos el proceso.
        return;
    }
    space = new AddrSpace(executable);  
    currentThread->space = space;
    // Guardamos el proceso en la tabla.
    pid = currentThread->procTabla.Open((long)currentThread);
    // Añadimos un nuevo proceso.
    currentThread->procTabla.addThread();
    // Se regresa el id del proceso.
    // return pid;
    machine->WriteRegister(2, pid); 
    delete executable;
    // Set the initial register values.
    currentThread->space->InitRegisters();
    // Load page table register.
    currentThread->space->RestoreState();
    // Jump to the user program.
    machine->Run();
    ASSERT(false);
}

/*
* System Call 3: Join.
* Efecto: Es muy similar al Unix Wait.
*/
void Nachos_Join()
{ 
    DEBUG('S', "Estamos haciendo Join!\n");
    // pid es el índice del vector de procTabla.
    SpaceId pid  = machine->ReadRegister(4);
    DEBUG('S', "Id(%d).\n", pid);
    Semaphore *sem = new Semaphore("Hilo", 0);
    DEBUG('S', "Creamos el semáforo %d y lo ponemos en espera.\n", sem);
    // Lo almacenamos en la tabla de semáforos.
    currentThread->semTabla.OpenSem(pid, (long) sem);
    currentThread->semTabla.addThread(); 
    if (currentThread->procTabla.getUnixHandle(pid) != -1) 
    { 	
        // Si el proceso al hacer Wait no ha terminado.
        // Esperar hasta que el proceso haya terminado.
        machine->WriteRegister(2, 0);   					
    }
    else
    {
        // Informar el error.
        machine->WriteRegister(2, -1);							
    }
    sem->P();
    returnFromSystemCall();
}

/* 
* System Call 4: Create.
* Efecto: Crea un nuevo archivo.
* Modifica: Tabla de archivos abiertos.
*/
void Nachos_Create(/*char* fileName*/)
{
    DEBUG('S', "Estamos haciendo Create!\n");
    int count = 1;
    // Iniciamos la obtención del nombre del archivo.
    int addrs = machine->ReadRegister(4);
    for(int i= 0; count != 0; ++i)
    {
        machine->ReadMem(addrs+i, 1, &count);
        // Se guarda el nombre del archivo en el BUFFER llamado mensaje. 
        mensaje[i] = (char)count;
    }
    // Se crea el archivo.
    int archivo = creat(mensaje, S_IRWXU|S_IRWXO|S_IRWXG);
    printf("Se creó el archivo %s con UnixHandler %d\n", mensaje, archivo);
    // Se guarda el UH del archivo nuevo en la tabla. 
    int res = currentThread->miTabla.Open(archivo);
    currentThread->miTabla.addThread(); 
    printf("Se creó el archivo %s con NachosHandler %d\n", mensaje, res);
    /*
    * return res;
    * res, variable que almacena el NachosHandler correspondiente al archivo.
    */
    machine->WriteRegister(2, res);
    returnFromSystemCall();
}

/* 
* System Call 5: Open.
* Efecto: Abre un archivo.
* Modifica: Tabla de archivos abiertos.
*/
void Nachos_Open(/*char* fileName*/)
{
    DEBUG('S', "Estamos haciendo Open!\n");
    copyString(4);
    // Abrimos el archivo mediante la funcionalidad de Linux.
    int unixHandle = open(mensaje, O_RDWR);	
    // Usamos tabla para relacionar NachOSHandle y UnixHandle.
    int result = currentThread->miTabla.Open(unixHandle);
    // Agregamos el archivo. 
    currentThread->miTabla.addThread();
    // Se regresa el NachosHandler.
    // return result;
    machine->WriteRegister(2, result);   
    printf("Se abre el archivo %s. UH: %d, NH: %d.\n", mensaje, unixHandle, result);
    returnFromSystemCall();
}

/*
* System Call 6: Read.
* Efecto: Lee una cantidad determinada de BYTES desde un archivo ya abierto.
* Modifica: Tabla de archivos abiertos.
*/
void Nachos_Read(/*char* hilera, int tamaño, OpenFileID id*/)
{
    DEBUG('S', "Estamos haciendo Read!\n");
    // Dirección de la información por leer.
    int dir = machine->ReadRegister(4); 
    // Se obtiene la cantidad de bytes que desean leerse.
    int tam = machine->ReadRegister(5);
    // Se obtiene el identificador del archivo desde donde se va a hacer la lectura.
    OpenFileId id = machine->ReadRegister(6);
    
    int numBytes = 0;
    char* buffer = new char[tam];

    DEBUG('S', "Read System call : dir(0x%x), tam(%d), id(%d).\n", dir, tam, id);
    printf("Mensaje a leer %s tam: %i id: %i.\n",mensaje, tam, id);
  
    mutex->P();
    switch (id)
    {
        case  ConsoleInput: /*stdin*/
            printf("Leído desde consola: %s .\nCaracteres leídos: %i.\n", mensaje, tam);
            numBytes = read(1, buffer, tam);
            //return numBytes; 
            machine->WriteRegister(2, numBytes);
            break;
        case  ConsoleOutput: /*stdout*/
            // Se informa del error producido por intentar leer desde la salida estándar.
            // return -1;
            machine->WriteRegister(2, -1);
            printf("No es posible leer desde la salida estándar.\n");
            break;
        case ConsoleError:
            printf("Error de consola %d.\n", machine->ReadRegister(4));
            break;
        default: /*Lectura desde file system*/
            if(currentThread->miTabla.isOpened(id) == true) 
            {   
                int unixHandle = currentThread->miTabla.getUnixHandle(id);  
                numBytes = read(unixHandle, buffer, tam);     
                printf("Lectura de archivos: %s.\nBytes leidos: %i.\n", mensaje, numBytes);
                //return numBytes;                
                machine->WriteRegister(2, numBytes);
            }
            else
            {
                // No se pudo realizar la lectura desde el archivo deseado.
                // return -1;
                machine->WriteRegister(2, -1); 
            } 
            break;
    }
    mutex->V();
    returnFromSystemCall();
}

/* 
* System Call 7: Write.
* Efecto: Escribe una cantidad determinada de BYTES desde un archivo ya abierto.
* Modifica: Tabla de archivos abiertos.
*/
void Nachos_Write()
{
    DEBUG('S', "Estamos haciendo Write!\n");
    // Se guarda la dirección virtual del archivo que es pasado como primer parámetro.
    int virtualAddress = machine->ReadRegister(4);
    // Se recibe el tamaño de la hilera.
    int size = machine->ReadRegister(5);
    // Se recibe el identificador del archivo.
    OpenFileId id = machine->ReadRegister(6);

    DEBUG('S', " bufferVrtAddr(0x%x), size(%d), fd(%d)\n", virtualAddress, size, id);
    copyString(4);

    mutex->P();
    int chars = 0;   
    
    switch(id)
    {
	// El usuario no puede escribir en la entrada estándar.
        case ConsoleInput: 
            machine->WriteRegister(2, -1);
            break;
 	
	// Se muestran hileras en la salida estándar.
        case ConsoleOutput:
            mensaje[size] = 0;
            stats->numConsoleCharsWritten++;
            printf("%s\n", mensaje);
            break;
 	
	// Se muestran enteros en la salida estándar.
        case ConsoleError:
            printf("%d\n", machine->ReadRegister(4));
            break; 
 
        default:
	    // Verifica si el archivo del identificador está abierto.
            bool isOpened = currentThread->miTabla.isOpened(id);
            if(isOpened)
            {
                // Unix Handle, identificador único para cada archivo abierto por el usuario donde se guardará la hilera.	
                int uh = currentThread->miTabla.getUnixHandle(id);
                chars = write(uh, mensaje,size);
                // Regresa el número de caracteres escritos por el usuario en el archivo.
            	machine->WriteRegister(2, chars);
    	        stats->numDiskWrites++;
            }
            else
            { 
                printf ("Archivo no esta abierto \n");
                // La llamada del sistema regresa el error.
		machine->WriteRegister(2, -1);
            }
            break;
    }
    // Se actualizan las estadísticas.
    stats->numConsoleCharsRead += chars;
    mutex->V();
    returnFromSystemCall();
}

/* 
* System Call 8: Close.
* Efecto: Se cierra un archivo.
* Modifica: Tabla de archivos abiertos.
*/
void Nachos_Close(/*OpenFileID id*/) 
{
    DEBUG('S', "Estamos haciendo Close!\n");
    // Leemos el identificador del archivo (NachOSHandler).
    int NH = machine->ReadRegister(6);
    // Suponemos un error como respuesta.
    int res = -1;
    // Verificamos si el archivo se encuentra abierto.
    if(currentThread->miTabla.Close(NH) != -1) 
    {      
        // Utilizamos el CLOSE de UNIX.
        res = close(currentThread->miTabla.getUnixHandle(NH));
        printf("Se cierra el archivo %i.\n", res);
        currentThread->miTabla.delThread();
    }
    else
    {
        printf("No se pudo cerrar el archivo. %i ",res);
    }
        // return res;
        machine->WriteRegister(2, res); 
        returnFromSystemCall();
}

/* 
* System Call 9: Fork.
* Efecto: Clona un proceso, comparte su espacio con su proceso hijo.
*/
void Nachos_Fork() 
{
    DEBUG('S', "Estamos haciendo Fork!\n");
    // Nuevo hilo para ejecutar el proceso del usuario.
    Thread * newT = new Thread("Child");
    newT->space = new AddrSpace(currentThread->space);
    // Hacemos fork sobre el código del hijo.
    newT->Fork(NachosForkThread, (void*)machine->ReadRegister(4));
    DEBUG('S', "Exiting Fork System call.\n");
    returnFromSystemCall();	
}

/*
* Función: NachosForkTread().
* Efecto: Rutina auxiliar para el system call Fork.
* Requiere: La dirección de la rutina como parámetro.
*/  
void NachosForkThread(void *name)
{
    DEBUG('S', "Estamos haciendo ForkThread!\n");
    long p = (long) name;
    space = currentThread->space;
    // Set the initial register values.
    space->InitRegisters();
    // Load page table register.
    space->RestoreState();

    // Set the return address for this thread to the same as the main thread.
    // This will lead this thread to call the exit system call and finish.
	
    machine->WriteRegister(RetAddrReg, 4);
    machine->WriteRegister(PCReg, (long) p);
    machine->WriteRegister(NextPCReg, p + 4);
    // Jump to the user progam.
    machine->Run();
    ASSERT(false);
}

/* 
* System Call 10: Yield.
* Efecto: cede el paso a un proceso.
*/
void Nachos_Yield()
{
    DEBUG('S', "Estamos haciendo Yield!\n");
    currentThread->Yield();
    returnFromSystemCall();
}

/* 
* System Call 11: SemCreate.
* Efecto: crea un semáforo.
*/
void Nachos_SemCreate(/*int valInicial*/) 
{
    DEBUG('S', "Estamos haciendo SemCreate!\n");
    // Extraemos el valor inicial.
    int valInicial = machine->ReadRegister(4);
    // Creamos el semáforo.
    Semaphore* sem = new Semaphore("Nuevo", valInicial);
    // Marcamos como ocupado el BITMAP y guardamos la dirección del semáforo.
    int idSem = currentThread->miTabla.Open((long) sem);
    currentThread->miTabla.addThread();
    // Devolvemos el ID del semáforo.
    // return idSem;
    machine->WriteRegister(2, idSem);
    DEBUG('S', "Semáforo creado: valInicial %i, id %i\n", valInicial, idSem);
    returnFromSystemCall();
}

/* 
* System Call 12: SemDestroy.
* Efecto: destruye un semáforo.
*/
void Nachos_SemDestroy(/*int idSem*/) 
{
    DEBUG('S', "Estamos haciendo SemDestroy!\n");
    // Extraemos el identificador del semáforo.
    int idSem = machine->ReadRegister(4);
    // Sacamos el semáforo de la tabla.											
    Semaphore* sem = (Semaphore*)currentThread->miTabla.getUnixHandle(idSem);
    sem->Destroy();
    currentThread->miTabla.Close(idSem);
    currentThread->miTabla.delThread();
    returnFromSystemCall();
}

/* 
* System Call 13: SemSignal.
* Efecto: Envía señal desde un semáforo de la tabla.
*/
void Nachos_SemSignal(/*int idSem*/) 
{
    DEBUG('S', "Estamos haciendo SemSignal!\n");
    // Extraemos el identificador del semáforo.
    int idSem = machine->ReadRegister(4);
    // Sacamos el semáforo de la tabla.											
    Semaphore* sem = (Semaphore*)currentThread->miTabla.getUnixHandle(idSem);
    sem->V();
    returnFromSystemCall();
}

/* 
* System Call 14: SemWait.
* Efecto: Hace el wait de un semáforo de la tabla.
*/
void Nachos_SemWait(/*int idSem*/) 
{
    DEBUG('S', "Estamos haciendo SemWait!\n");
    // Extraemos el identificador del semáforo.
    int idSem = machine->ReadRegister(4);
    // Sacamos el semáforo de la tabla.											
    Semaphore* sem = (Semaphore*)currentThread->miTabla.getUnixHandle(idSem);
    sem->P();
    returnFromSystemCall();
}

/*----------------------- Fin de la implementación de los llamados del sistema -----------------------*/

// Función encargada de ejecutar los llamados al sistemas implementados en el proyecto.
void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    printf("Ha realizado el llamado #%d.\n", type);
    switch(which)
    {
        case SyscallException:
            switch(type)
	    {
                case SC_Halt:
                    Nachos_Halt();
                    break;  
                case SC_Exit:
                    Nachos_Exit();
                    break;
                case SC_Exec:
                    Nachos_Exec();                    
                    break;
                case SC_Join:
                    Nachos_Join();
                    break;
                case SC_Create:
                    Nachos_Create();
                    break;  
                case SC_Open:
                    Nachos_Open();
                    break;       
                case SC_Read:
                    Nachos_Read();
                    break;       
                case SC_Write:
                    Nachos_Write();
                    break;   
                case SC_Close:
                    Nachos_Close();
                    break;   
                case SC_Fork:
                    Nachos_Fork();
                    break;  
                case SC_Yield:
                    Nachos_Yield();
                    break;   
                case SC_SemCreate:
                    Nachos_SemCreate();
                    break;    
               case SC_SemDestroy:
                    Nachos_SemDestroy();
                    break;
                case SC_SemSignal:
                    Nachos_SemSignal();
                    break;   
                case SC_SemWait:
                    Nachos_SemWait();
                    break;
		default:
 		    printf("Unexpected syscall exception %d.\n", type);
                    ASSERT(false);
                    break;
            }
	    break;
    	
	case PageFaultException:
            printf("Se ha generado un PageFaultException! Página: %d .\n:(.\n", machine->ReadRegister(39));
            currentThread->space->MemoriaVirtual();
            break;
    
        case ReadOnlyException:
            break;
    
        case BusErrorException:
            break;
    
        case AddressErrorException:
            break;
    
        case OverflowException:
            break;
    
        case IllegalInstrException:
            break;

        case NumExceptionTypes:
            break;
    	
	default:
	    printf("Unexpected exception %d\n", which);
            ASSERT(false);
            break;
    }
}
