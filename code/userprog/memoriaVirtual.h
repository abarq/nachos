#include "bitmap.h"
#include "filesys.h"

class MemoriaVirtual {

private:
    //señalador
    int indexPage;
    //Archivo ejecutable
	OpenFile * archivoVM;
	//Guarda informacion sobre el espacio en la memoria fisica
	BitMap * memFisica;
	//Guarda informacion sobre el espacio del archivo de la memoria virtual
	BitMap * memDisco;
	//Guarda la informacion sobre quien es el padre de una pagina
	AddrSpace** padre;

    // Selecciona que pagina va a ser removida de memoria para  dar lugar a otra
    // utilizando FIFO y deja el indexPage listo para la siguiente pageFault
    int seleccionarPaginaVictima() {
        indexPage++;
        if(indexPage < NumPhysPages)	   
            return indexPage;
        else{	   
            indexPage = 0;
            return indexPage;	   
        }       
    }


public:
    //Inicializa la memoria virtual al crear el manejador
    MemoriaVirtual() {
        indexPage = -1;
        memFisica = new BitMap(NumPhysPages);
        memDisco = new BitMap(Max_Paginas);
        padre = new AddrSpace*[NumPhysPages];
        //Crea un archivo que sirve para administar la memoria virtual en disco
        if (fileSystem->Create("SWAP",0))//si lo pudo crear
            archivoVM = fileSystem->Open("SWAP");//Abre el archivo de momoria virtual
        else
            ASSERT(false); // si el swap no puede ser creado
    }

    //Destruye la memoria virtual y devuelve la memoria utilizada
    ~MemoriaVirtual() {
        //Borra el archivo de memoria virtual
        delete archivoVM;
        //Elimina el archivo fisico de momoria virtual
        fileSystem->Remove("SWAP");
        //Borra el vector de paginas dueños
        delete padre;
        //Borra el bitMap de memoria fisica        
        delete memFisica;
        //Borra el bitMap de memoria en disco (VM)        
        delete memDisco;
    }

    //Consigue una pagina de la memoria, la pone en el disco y modifica su estado
    int setPaginaDisco(AddrSpace *requester,int page) {
		int swapPageIndex = -1;	
		//Busca un lugar en la memoria para colocar la pagina, (Pregunta si hay espacio)
        int memIndex = memFisica->Find();
        AddrSpace* owner;
        printf("se tiene el indice %d de memoria fisica\n",memIndex);
        if (memIndex  == -1) {
            //Si no hay espacio, busca una pagina para el remplazo
           memIndex = seleccionarPaginaVictima();
           printf("dado que era -1 se seleccióno una victima\n se tiene el indice %d de memoria fisica\n",memIndex);

            owner = (AddrSpace*)padre[memIndex];
            for (int i=0,max = owner->numPages; i<max; i++) {
                if (owner->pageTable[i].physicalPage == memIndex){
                    if ((swapPageIndex = memDisco->Find()) != -1) {
                        //Escribe en el disco la pagina que esta en memoria..
                        archivoVM->WriteAt(&(machine->mainMemory[memIndex * PageSize]),PageSize, swapPageIndex * PageSize);
                        owner->pageTable[i].valid = false;
                        //Coloca el estado de la pagina en disco   
                        owner->pageTable[i].state = En_Disco;
                        //Indica el lugar en el disco
                        owner->pageTable[i].swapIndex = swapPageIndex;
                        //Indica que no esta en memoria fisica
                        owner->pageTable[i].physicalPage = -1;
                    } else {
                        ASSERT(false);
                    }
                    break;
                }
            }
        }else{

			//se trae la pagina de memoria de linux a la memoria principal de la maquina
				printf("página fisica : %d\n",memIndex);\
				fflush(stdout);
				char * memPrinp=&machine->mainMemory[memIndex * PageSize];
				printf("posición en memoria principal: %d\n",memPrinp);
				int dirtemp= requester->noffH.code.inFileAddr+page*PageSize; 
				printf("posición en memoria nachos: %d\n",dirtemp);
				// se vuelve a  abrir el programa
				OpenFile *executable = fileSystem->Open(requester->filename);
				executable->ReadAt(memPrinp, PageSize,dirtemp); //se copia los datos a la memoria principal.
				printf("se ha copiado la pagina al disco de nachos\n");
			}

        if (memIndex != -1) 
            padre[memIndex] = requester;
        return memIndex;
}

    //Consigue una pagina del disco, la coloca en la memoria y modifica su estado
    int getPaginaDiscogetPaginaDisco(AddrSpace * requester, int pageTableIndex){
		
		
		
        int respuesta = -1;
        int swapIndex = requester->pageTable[pageTableIndex].swapIndex;
        if (memDisco->Test(swapIndex)){ //Verifica que la localidad del disco este ocupada
            //Lee la pagina del disco
            respuesta = archivoVM->ReadAt(&(machine->mainMemory[requester->pageTable[pageTableIndex].physicalPage * PageSize]),PageSize,swapIndex * PageSize);
            //Indica que hay una posicion lista en el disco
            memDisco->Clear(swapIndex);
            //Indica que es una posicion valida
            requester->pageTable[pageTableIndex].valid = true;
            //Indica que la pagina se encuentra en el disco
            requester->pageTable[pageTableIndex].state = En_Memoria;
            //Indica que no tiene posicion en el disco
            requester->pageTable[pageTableIndex].swapIndex = -1;
        }
        return respuesta;
    }
    
};
