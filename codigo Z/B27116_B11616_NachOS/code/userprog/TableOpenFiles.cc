#include "TableOpenFiles.h"

 TableOpenFiles::TableOpenFiles() 
{
    openFiles = new int(32);        // A vector with user opened files
    openFilesMap = new BitMap(32);  // A bitmap to control our vector
    int usage = 1;                  // How many threads are using this table
    openFilesMap->Mark(0);          // Se marcan los primeros 3 bits para el stdin, out y error. 
    openFilesMap->Mark(1);
    openFilesMap->Mark(2);
 };
 
TableOpenFiles::~TableOpenFiles()
{
    if (usage > 0)
    {
        delete openFiles;       // A vector with user opened files
        delete openFilesMap;    // A bitmap to control our vector
        int usage = 0;          // How many threads are using this table
    }
};
 
int TableOpenFiles::Open(long UnixHandle)
{
    long campo = openFilesMap->Find();  // Automaticamente a true.
    if (campo != -1)
    { 
        openFiles[campo] = UnixHandle;
    }
    return campo;
}

void TableOpenFiles::OpenSem(int posicion, long semaforo)
{
    openFiles[posicion] = semaforo;
}

int TableOpenFiles::Close(long NachosHandle)
{
    bool set = openFilesMap->Test(NachosHandle); // Devuelve true si está ocupado.
    if(set == true && NachosHandle > 2)
    { 
        openFilesMap->Clear(NachosHandle);
        return 0;
    } 
    return -1;	
 
};     

bool TableOpenFiles::isOpened(long NachosHandle)
{
    return openFilesMap->Test(NachosHandle);
};
	
long TableOpenFiles::getUnixHandle(long NachosHandle)
{    
    return openFiles[NachosHandle];
};

 
void TableOpenFiles::addThread()
{  
    //Usage incremeta para ver si más hilos están usando el mismo espacio.
    usage++; 
};     
 
void TableOpenFiles::delThread() 
{
    // If a user thread is using this table, delete it
    usage--;
    if (usage == 0) { 
        //delete this;
    }
};

int TableOpenFiles::getUsage()
{
    // If a user thread is using this table, add it.
    return usage; 
};
