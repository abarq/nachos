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
       #include <unistd.h>

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

// Constructor de AddrSpace que copia el contenido de otro existente.
AddrSpace::AddrSpace(AddrSpace *addrSpace)
{
    // Cantidad de páginas que se necesitan para la pila.
    int stackPages = divRoundUp(UserStackSize, PageSize);
    
	// Verificamos que pueda hacerse la copia de las páginas al bitmap.
	numPages = addrSpace->numPages;
	ASSERT(numPages <= MiMapa->NumClear());
    
	// Creamos la tabla de paginación.
	pageTable = new TranslationEntry[numPages];
    
    for(int page = 0; page < numPages; page++)
        {	
        // Desde el comienzo hasta el final de todas las páginas.
        pageTable[page].virtualPage = page;
        if(page < numPages-stackPages)
        {
            // Copiando segmento de código y datos.
            pageTable[page].physicalPage = addrSpace->pageTable[page].physicalPage;
	        pageTable[page].valid = addrSpace->pageTable[page].valid;
            pageTable[page].use = addrSpace->pageTable[page].use;
            pageTable[page].dirty = addrSpace->pageTable[page].dirty;
            pageTable[page].readOnly = addrSpace->pageTable[page].readOnly;  
        }
        else 
        {
            // Pila nueva del hijo.
            pageTable[page].physicalPage = MiMapa->Find();
            bzero(&(machine->mainMemory[pageTable[page].physicalPage * PageSize]), PageSize);
			
            pageTable[page].valid = true;
            pageTable[page].use = false;
            pageTable[page].dirty = false;
            pageTable[page].readOnly = false;        
	}				
    }
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

    //printf("\nEncabezado sizeof(noffH) %d\n", sizeof(noffH));
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC))
    {
        SwapHeader(&noffH);
    }
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // How big is address space?
    // We need to increase the size to leave room for the stack.
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;

    //printf("Address space = %d => Data(%d) + Pila(%d): \n", size,noffH.code.size, UserStackSize);
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    //printf("PageSize %d numPages %d, size %d\n", PageSize,numPages, size);
    /// ASSERT(numPages <= NumPhysPages);

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, size);
    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];
    // Tabla de páginas invertidas.
    coreMap = new TranslationEntry[numPages];
    // Implementamos qué hacer en caso de que trabajamos con memoria virtual.
    #ifdef VM
	DEBUG('a', "Hay memoria virtual.\n");
    for (i = 0; i < numPages; i++) 
    {
        
		/* if(i == 0 || i == 2 || i == 4 || i == 6 || i == 8 || i == 10 )
        {
            MiMapa->Mark(i);
        }*/
	
		
        // Cada página virtual corresponde a una física con el mismo índice.
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid = false;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false; 
    }
    #else
    DEBUG('a', "No hay memoria virtual.\n");
    for (i = 0; i < numPages; i++) 
    {
         
		/* if(i == 0 || i == 2 || i == 4 || i == 6 || i == 8 || i == 10 )
        {
            MiMapa->Mark(i);
        } */
	
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = MiMapa->Find();
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
    }
    #endif
    
    // Dejamos vacío todo el AddrSpace.
    bzero(machine->mainMemory, size);

    // Luego copiamos el código y los datos a memoria.
    #ifndef VM
    if (noffH.code.size > 0)
	{
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]), noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) 
	{
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]), noffH.initData.size, noffH.initData.inFileAddr);
    }
    #endif
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------
AddrSpace::~AddrSpace()
{   
	#ifndef  VM
	// Eliminamos las referencias de la tabla de paginación.
	DEBUG('a', "Vaciando tabla de paginación.\n");
    for (int i; i < numPages; i++) 
    {
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = 0;
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
   }
   delete pageTable;
   #endif
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
void AddrSpace::InitRegisters()
{
    int i;
	// Limpiamos todos los registros.
    for (i = 0; i < NumTotalRegs; i++)
	{
        machine->WriteRegister(i, 0);
	}

    // Initial program counter -- must be location of "Start".
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because of branch delay possibility.
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
//----------------------------------------------------------------------
void AddrSpace::SaveState() 
{
    #ifdef VM	
    for(int i = 0; i < TLBSize; i++)
    {
        pageTable[machine->tlb[i].virtualPage].use = machine->tlb[i].use;
        pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
    }
	// Marcamos las posiciones del TLB que fueron ocupadas.
    for(int i = 0; i <TLBSize; i++)
    {
        machine->tlb[i].valid = false;
    }
    #endif
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------
void AddrSpace::RestoreState() 
{
    // Abrimos el archivo SWAP.
    #ifndef VM
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
    #else
	// Sólo uno de los dos, Pagetable o TLB puede usarse en VM al mismo tiempo.
    machine->pageTable = NULL;
    
	for(int i=0; i<TLBSize; i++)
    {
        machine->tlb[i].valid = false;
    }
    #endif
}

// Método encargado de encontrar un espacio para guardar una página deseada en el TLB.
int AddrSpace::LIFO(TranslationEntry* pagina, int size)
{
    // Definimos el último elemento como false.
    pageTable[pagina[size-1].virtualPage].use = false;
	// Copiamos su bit dirty al TLB.
    pageTable[pagina[size-1].virtualPage].dirty = pagina[size-1].dirty;
    for(int j=size-1; j > 0; j--)
    {
        pagina[j] = pagina[j-1];
    }
    return 0;
}

// Método encargado de actualizar el TLB.
void AddrSpace::actualizarTLB(int pagina)
{
    int paginaSeleccionada = -1;
    for(int i=0; i<TLBSize; i++)
    {
        if(machine->tlb[i].valid == false)
        {
            // Si el TLB no está lleno, simplemente insertamos la página.
            paginaSeleccionada = i;
            break;
        }
    }
    if(paginaSeleccionada == -1)
    { 
        // Si la tabla está llena, aplicamos el algoritmo LIFO.
        paginaSeleccionada = LIFO(machine->tlb, TLBSize);
    }
    // Actualiza el TLB con los valores de la página.
    machine->tlb[paginaSeleccionada].physicalPage = pageTable[pagina].physicalPage;
    machine->tlb[paginaSeleccionada].virtualPage = pageTable[pagina].virtualPage;
    // Determinamos que está en uso.
    machine->tlb[paginaSeleccionada].valid = true;
    machine->tlb[paginaSeleccionada].use = true; 
    // Actualizamos el bit dirty.
    machine->tlb[paginaSeleccionada].dirty = pageTable[pagina].dirty;
    // Asiganmos el bit de use de la pagetable en true, para avisar de que acabamos de actualizar los datos en la TLB.
    pageTable[pagina].use = true;
}

int tam = 0;
// Vector auxiliar para el SWAP.
int vectorS[64];
// Vector auxiliar para el control de las páginas.
int vectorP[32];

// Método que nos permite determinar el tipo de solución a aplicar para el PageFaultException.
void AddrSpace::MemoriaVirtual()
{
    int pagina;
    // Calculamos la página lógica, dividiendo el contenido del registro 39 entre el tamaño de una página(128).
    pagina = machine->ReadRegister(BadVAddrReg)/PageSize;
    verificarPaginaSucia(pagina);
}

void AddrSpace::verificarPaginaSucia(int pagina)
{
    if(pageTable[pagina].valid == true)
    { 
       // Verificamos que la página está en el pageTable y no en el TLB.
        actualizarTLB(pagina); 
    }
    else
    {
        int pagValida;
        // Buscamos espacio válido para la página.
        pagValida = MiMapa->Find();
        // No se encontró espacio para la página.
        if(pagValida == -1)
        { 
            // Buscamos el índice adecuado para insertar la página.
	        pagValida = secondChance(pagina);
        }
        else
        {
            // Si hay espacio solamente introducimos la página en nuestra CoreMap.
            vectorP[pagValida] = pagina;
        }
        // Verificamos si se ha modificado la página.
        if(pageTable[pagina].dirty == true)
        { 
            // Hacemos lectura del contenido del SWAP.
            // Guardamos en el SWAP la página válida.
            leer(pagValida, pagina);
            // Declaramos la página como válida en el PageTable.
            pageTable[pagina].valid = true;
            // Creamos la asociación entre páginas 
            pageTable[pagina].physicalPage = pagValida;
            actualizarTLB(pagina);
        }
        else
        { 
            // El nombre del archivo fue almacenado con anterioridad.
            OpenFile* executable = fileSystem->Open(fileName);
            // Creamos el encabezado.
            NoffHeader noffH;
            // Copiamos los datos del ejecutable al encabezado.
            executable->ReadAt((char*)&noffH, sizeof(noffH), 0);
            int paginasCodigo = divRoundUp(noffH.code.size, PageSize); 
            
			/// Se almacenan las páginas del código.
            int codigo[paginasCodigo];
    
            for(int i = 0; i < paginasCodigo ; i++)
            {
                // Lleno el vector con las páginas de codigo respectivas.
                codigo[i] = i;
            }
			
            /// Se almacenan páginas de datos.
      	    int paginasDatos = noffH.initData.size/PageSize;
            // Se almacenan las páginas de datos.
            int datos[paginasDatos];
            
			int j = 0;
	        for(int i = paginasCodigo; i < paginasDatos+paginasCodigo ; i++)
            {
                datos[j] = i;
                j++;				
            }

            // Definimos esa página como válida.
            pageTable[pagina].valid = true;
            // Asignamos una página física válida.
            pageTable[pagina].physicalPage = pagValida;
	
            for(int i = 0; i < paginasCodigo ; i++)
            { 
                // Verificamos si corresponde a una página de código.
                if(pagina == codigo[i])
                {
                    // Leemos de memoria los datos de cada página válida.
                    int nbytes = executable->ReadAt(&(machine->mainMemory[pagValida*PageSize]), PageSize, noffH.code.inFileAddr+(pagina*PageSize));
                    if(nbytes < PageSize)
                    { 
                        // Si faltan bytes de código, se intercalan con los de datos.
                        tam = PageSize - nbytes;
                        executable->ReadAt(&(machine->mainMemory[pagValida*PageSize+nbytes]),
					    tam, noffH.initData.inFileAddr+(pagina*PageSize));	
                    }
                    break;
                }
            }

            for(int i = 0; i < paginasDatos; i++)
            { 
                // Verificamos si corresponde a datos inicializados.
                if(pagina == datos[i])
                { 
                    // Si está en el vector de páginas de datos.
                    executable->ReadAt(&(machine->mainMemory[pagValida*PageSize]),
					 PageSize,noffH.initData.inFileAddr+(pagina*PageSize)+tam);
				}
                else
                {
                    // Limpiamos la memoria.
                    bzero(&(machine->mainMemory[pagValida*PageSize]),PageSize);
    	        } 
                break;
                }
                delete executable;
                actualizarTLB(pagina);
            }
    }
}

// Método encargado de leer del SWAP y de borrar de memoria esa página.
void AddrSpace::leer(int page, int vpn)
{
    // Abrimos el archivo SWAP.
    OpenFile* swap = fileSystem->Open("Swap");
    
    int i = 0;
    bool continuar = true;
    // Buscamos la página deseada.
    while(continuar)
    {
        if(vectorS[i] ==  vpn)
        {
            // Termina si encuentra la página en el SWAP.
            continuar = false;
        }
        else
        {
            i++;
        }
    }

    swap->ReadAt(&(machine->mainMemory[page*PageSize]), PageSize, i*PageSize);
    // Limpiamos el bitmap del SWAP.
    MiMapaSwap->Clear(i);
    // Invalidamos esa posición del vector auxiliar.
    MiMapaSwap[i] = -1;
    delete swap;
}

// Método encargado de guardar en el SWAP la página deseada.
void AddrSpace::escribir(int page , int pos)
{
    // Abrimos el archivo SWAP.
    OpenFile* swap = fileSystem -> Open("Swap");
    // Guardamos en SWAP la página deseada.
    swap->WriteAt(&(machine -> mainMemory[page*PageSize]), PageSize, pos*PageSize);
    delete swap;
}

// Índice de la tabla de paginación por la que se iba la última vez que se utilizó.
int ult = 0;

// Método que ayuda a la tabla de paginación cuando ésta está llena.
int AddrSpace::secondChance(int vpn)
{
    bool continuar = true;
    int indice = ult;
    // Buscamos las páginas que tienen el bit de use válido.
    while(continuar)
    {  
        // Verificamos el estado del bit de use.
        if(pageTable[vectorP[indice]].use)
        {
            // Lo definimos en false.
            pageTable[vectorP[indice]].use = false;
            ++indice;
            // Lo hacemos como un vector circular.
            indice = indice % NumPhysPages;
        }
        else
	{
	    ++ult;
            ult = ult % NumPhysPages;
            ult = false;
        }	 
    }
	
    int respuesta = 0;
    // Posición del SWAP.
    int pos = 0;

    // Establecerla como no en uso.
    pageTable[vectorP[indice]].valid = false;
    // Verificamos si está sucia.
    if(pageTable[vectorP[indice]].dirty)
    {
	    // Es necesario guardarla en el SWAP.

	    // Encontramos espacio libre mediante el bitmap asociado al SWAP.
            pos = MiMapaSwap->Find();
            // Guardamos la página en el SWAP.
            escribir(pageTable[vectorP[indice]].physicalPage, pos);
	    MiMapaSwap[pos] = pageTable[vectorP[indice]].virtualPage;
            
            // Buscamos la página física en la tabla de paginación.
            respuesta = pageTable[vectorP[indice]].physicalPage;
            pageTable[vectorP[indice]].physicalPage = pos;
    }
    else
    {
        // Buscamos la página física en la tabla de paginación.
        respuesta = pageTable[vectorP[indice]].physicalPage;
    }
    vectorP[indice] = vpn; //en nuestra TPI copiamos la nueva página que estará en memoria.  
    for(int i = 0; i < TLBSize; i++)
    { //iteramos por nuestra TLB
        if(machine->tlb[i].physicalPage == respuesta)
        { 
            //Si esa página que tenemos en la TLB es la misma de la PageTable.    
            machine->tlb[i].valid = false;   //Tambien seteamos en False esa en el TLB
            break;
        }
    }    
	// Devolvemos el índice de esa página.
    return respuesta;
}
