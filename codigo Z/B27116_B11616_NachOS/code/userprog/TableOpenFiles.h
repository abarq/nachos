#ifndef TableOpenFiles_H
#define TableOpenFiles_H
#include "bitmap.h"

class TableOpenFiles {
  public:
    TableOpenFiles();       // Initialize
    ~TableOpenFiles();      // De-allocate
     
    int Open(long UnixHandle);         // Register the file handle
    int Close(long NachosHandle);      // Unregister the file handle
    bool isOpened(long NachosHandle);
    long getUnixHandle(long NachosHandle);
    void addThread();       // If a user thread is using this table, add it
    void delThread();       // If a user thread is using this table, delete it
    int getUsage();
    void OpenSem(int posicion, long semaforo);
    void Imprimir();        // Print contents

  private:
    int * openFiles;        // A vector with user opened files
    BitMap * openFilesMap;  // A bitmap to control our vector
    int usage;              // How many threads are using this table
    long cola[32];
};
#endif 
