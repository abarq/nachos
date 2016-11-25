// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "memoriaVirtual.h"


MemoriaVirtual * virtualMem = new MemoriaVirtual();

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);

    int pagEncontrada;
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {

	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
        //pagEncontrada = pageTableMap->Find();
	pageTable[i].physicalPage = -1;
	#ifdef VM
		pageTable[i].valid = false;
	#else
		pageTable[i].valid = true;
	#endif
        
	//Indica que la pagina no esta inicializada (En_Disco o En_Memoria)		
	pageTable[i].state = No_inicializado;
        //Indica que no tiene posicion en el archivo de memoria virtual
	pageTable[i].swapIndex = -1;
	pageTable[i].use = false;
	pageTable[i].dirty = false;
	pageTable[i].readOnly = false;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only

        //memset(machine->mainMemory + (pagEncontrada * PageSize), 0, PageSize);           // Inicializa con 0s el area

	#ifdef VM
		
	#else
        	executable->ReadAt(&(machine->mainMemory[pagEncontrada*PageSize]), PageSize, (i*PageSize)+noffH.code.inFileAddr); // 			Copia la  informacion en memoria
	#endif

	inicioCodigo = noffH.code.inFileAddr;
	/*char * filename = new char[strlen(file)];
    	//Escribo el nombre del archivo en un vector para que se pueda pasar como parametro
    	strcpy(filename, file);*/
    	//Abro en ejecutable    
	//programa = fileSystem->Open(filename);
	//Cargo la pagina 0 (Es en la que comienza la ejecución)*/
	programa = executable;
	Cargar(0);
	Cargar(1);
	Cargar(9);
    }
}

//--------------------------------------------------------------------------
//Constructor para hacer copia de un AddrSpace al addrspace de un nuevo thread
//
//-------------------------------------------------------------------------

AddrSpace::AddrSpace(AddrSpace *addr)
{
    numPages = addr->numPages;
    int pagEncontrada;
    pageTable = new TranslationEntry[(numPages+(UserStackSize/128))];

    for (int i = 0; i < (numPages+(UserStackSize/128)); i++) {
        if(i < numPages){
            pageTable[i].virtualPage = addr->pageTable[i].virtualPage;
            pageTable[i].physicalPage = addr->pageTable[i].physicalPage;
            pageTable[i].valid = true;
            pageTable[i].use = false;
            pageTable[i].dirty = false;
            pageTable[i].readOnly = false;

        }else{
	    
            //Espacio para la pila
            pagEncontrada = pageTableMap->Find();
            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = pagEncontrada;
            pageTable[i].valid = true;
            pageTable[i].use = false;
            pageTable[i].dirty = false;
            pageTable[i].readOnly = false;
            memset(machine->mainMemory + (pagEncontrada * PageSize), 0, PageSize);
	    }
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
  //  machine->pageTable = pageTable;
   // machine->pageTableSize = numPages;
}

// Carga una pagina no inicializada o en disco a memoria y modifica su estado
int AddrSpace::Cargar(int virtualIndex)
{       
	printf("-------se esta accesando la memoria %d--------\n", virtualIndex);
    int resultado = -1;  
   // virtualIndex  =1;
    //Guardo el estado de la pagina
    int estado = pageTable[virtualIndex].state;
    //Si la pagina no esta inicializada o esta en el disco
    
    if(estado == No_inicializado || estado == En_Disco){
		printf("la ṕagina esta en disco o no inicializada \n");
		fflush(stdout);
        //Asigna la pagina en el dico
        resultado = pageTable[virtualIndex].physicalPage = virtualMem->setPaginaDisco(this);
        printf("se ha asignado la página %d \n", resultado);
	
        if(resultado != -1){
            if(estado == En_Disco)    //Si esta en el disco
			{
				printf("La página se encuentra en disco por lo que se trae \n");
				fflush(stdout);
                //Traiga la pagina del disco y pongala en memoria
                virtualMem->getPaginaDisco(this, virtualIndex);
			}
            else//Si no esta inicializada
			{
				printf("La página se encuentra sin inicializar por lo que se trae de disco \n");
				fflush(stdout);
				//Lea la pagina de memoria y cargue la pagina
				int dirtemp= inicioCodigo + virtualIndex * PageSize;
				printf("incio del codigo en %d\nse carga la dirección de memoria %d\n",inicioCodigo,dirtemp);
				int pafis =pageTable[virtualIndex].physicalPage;
				printf("página fisica : %d\n",pafis);
				char * memPrinp=&machine->mainMemory[pafis * PageSize];
				printf("posición en memoria principal: %d\n",memPrinp);
				programa->ReadAt(memPrinp, PageSize,dirtemp);
				printf("se ha guardado la página en memoria principal\n");
			}
            //Indica que la pagina es valida en este momento
            pageTable[virtualIndex].valid = true;
            //Indica que la pagina se encuentra en memoria en este momento
            pageTable[virtualIndex].state = En_Memoria;
        }
    }
    if(pageTable[virtualIndex].state == En_Memoria){
	int i=0;
	while(machine->tlb[i].use){
	    machine->tlb[i].use = false;
	    if(i < TLBSize)
		i++;
	    else
		i=0;
	}
	machine->tlb[i] = pageTable[virtualIndex];
	resultado = machine->tlb[i].physicalPage;
    }
    return resultado;
}

