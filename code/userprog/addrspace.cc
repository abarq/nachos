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
	DEBUG('a', "Entrando al addres space" );

    NoffHeader noffH;
    unsigned int size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

	int stackSize= UserStackSize;
	
	//se calculan por separado cuantas páginas se ocupan por cada segmento.
	codePages=divRoundUp(noffH.code.size,PageSize);
	dataPages=divRoundUp(noffH.initData.size,PageSize);
	unDataPages=divRoundUp(noffH.uninitData.size,PageSize);
	stackPages=divRoundUp(stackSize,PageSize);
	
    numPages = codePages+dataPages+unDataPages+stackPages;
   	printf("tamaño de segmento de codigo: %d\n",noffH.code.size);
	printf("tamaño de segmento de datos incializados: %d\n",noffH.initData.size);
	printf("tamaño de segmento de datos no inicializados: %d\n",noffH.uninitData.size);
	printf("tamaño de segmento de pila: %d\n",stackSize);

    size = numPages * PageSize;
    printf("Cantidad de páginas %d\n",numPages);

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation if it is no initializate yet
	pageTable = new TranslationEntry[numPages];
	
	printf("buscando las posiciones vacías del bitmap\n");

	for (int i=0; i<numPages;i++)
	{
		positions[i]= pageTableMap->Find();
		printf("se ha guardado la página %d en la posicion %d del mapa\n",i,positions[i]);
	}
	//se guardan las páginas en memoria basandose en el arreglo de posiciones libres encontradas anteriormente
    for (int i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = positions[i];
	pageTable[i].valid = true;
	pageTable[i].use = false;
	pageTable[i].dirty = false;
	pageTable[i].readOnly = false;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
    }
    printf("Se han guardado las páginas\n");

// then, copy in the code and data segments into memory
//primero se guarda el tamaño del segmento de código
	int codeSize =  noffH.code.size;
	int usedPages=0;
	int bytesToCopy=PageSize;
	while ( codeSize>0)
	{
		if (codeSize<PageSize)
			bytesToCopy=codeSize;
		//se guarda página por página en la memoria de la máquina en las posiciones obtenidas
		
		executable->ReadAt(&(machine->mainMemory[positions[usedPages]*PageSize]),
		bytesToCopy, noffH.code.inFileAddr+usedPages*PageSize);
		printf("Se han copiado %d bytes a la página %d de la memoria\n",bytesToCopy,positions[usedPages]);
		//luego se resta los bytes copiados del total y se suman las paginas utilizadas
		codeSize-=PageSize;
		usedPages++;
	}
	
	printf("se han copiado el segmento de código\n");
	
	int dataSize =  noffH.initData.size;
	bytesToCopy=PageSize;
	
	while ( dataSize>0)
	{
		if (dataSize<PageSize)
			bytesToCopy=PageSize;
		//se guarda página por página en la memoria de la máquina en las posiciones obtenidas
		executable->ReadAt(&(machine->mainMemory[positions[usedPages]*PageSize]),
		bytesToCopy, noffH.initData.inFileAddr+usedPages*PageSize);
		printf("Se han copiado %d bytes a la página %d de la memoria\n",bytesToCopy,positions[usedPages]);
		//luego se resta los bytes copiados del total y se suman las paginas utilizadas
		dataSize-=PageSize;
		usedPages++;
	}
	printf("se han copiado el segmento de datos\n");

}

/**
*Constructor para hacer copia de un AddrSpace que recibe un addrspace
*
*/

AddrSpace::AddrSpace(AddrSpace *addr)
{
	//primeramente se copian los numeros de páginas de los segmentos utilizados 
	//por el código y los datos inicializados
    numPages = addr->numPages;
    codePages= addr->codePages;
    dataPages = addr->codePages;
    
    
    pageTable = new TranslationEntry[numPages];
	
	//primeramente se copian los apuntadores de la memoria de códigos y datos inicializados
    for (int i = 0; i < codePages+dataPages; i++)
    {
            pageTable[i].virtualPage = addr->pageTable[i].virtualPage;
            pageTable[i].physicalPage = addr->pageTable[i].physicalPage;
            pageTable[i].valid = true;
            pageTable[i].use = false;
            pageTable[i].dirty = false;
            pageTable[i].readOnly = false;

	}
	
	//se reserva el espacio para las variables no inicializadas y la pila
	//dado que cada proceso tiene su propia pila se reserva nuevamente
	for (int i=codePages+dataPages;i<numPages;i++)
	{    
		
		pageTable[i].virtualPage = i;
		pageTable[i].physicalPage = pageTableMap->Find();
		pageTable[i].valid = true;
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
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
    for (int i = 0; i < NumTotalRegs; i++)
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
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
