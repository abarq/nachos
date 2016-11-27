// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "noff.h"

#define UserStackSize		1024 	// increase this as necessary!
#define Max_Paginas		1024
#define En_Disco          0
#define En_Memoria        1
#define No_inicializado   2
#define En_TLB 			  3
#define En_SWAP 		  4


class AddrSpace {
  public:
    AddrSpace(OpenFile *executable,const char *filename);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    AddrSpace(AddrSpace *addr);
    ~AddrSpace();			// De-allocate an address space
    NoffHeader noffH;
    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
    int Cargar(int);

  //private:
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space

    // Se refiere al archivo ejecutable
    OpenFile * programa;
    const char *filename ;
    // Indica donde comienza el codigo
    int inicioCodigo;
  private:
  
	int cargarDeDisco(int page);
};

#endif // ADDRSPACE_H
