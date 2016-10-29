#include "NachosOpenFilesTable.h"




/**Constructor de la clase NachosOpenFilesTable*/
NachosOpenFilesTable::NachosOpenFilesTable(){

	openFiles = new int[largoVector]; //Asigna memoria al vector openFiles de tamaña 'largoVector'.
	openFiles[0] = 0; //Inicializa los primeros tres puestos del vector.
	openFiles[1] = 1;
	openFiles[2] = 2;

	openFilesMap = new BitMap(largoVector); //Asigna memoria al Bitmap con parametro de entrada largoVector
	openFilesMap->Mark(0); //Marca los primeros tres puestos del Bitmap
	openFilesMap->Mark(1);
	openFilesMap->Mark(2);
	this->usage = 0;  //Inicializa usage en cero

}

/**Destructor de la clase*/
NachosOpenFilesTable::~NachosOpenFilesTable(){

	delete openFiles; //Desasgina memoria a openFiles
	delete openFilesMap; //Desasigna memoria a openFilesMap


}

/**Metodo Open, recibe el handle del unix y almacena el valor devuelto y lo almacena en el vector de archivos abiertos*/
int NachosOpenFilesTable::Open(int unixHandle){

	int ubicacionLibre = openFilesMap->Find(); //El objeto de tipo Bitmap devuelve la posicion disponible para ubicar en el vector.
	openFiles[ubicacionLibre] = unixHandle; //Asigna en el vector el valor de unix Handle.

	if(ubicacionLibre != -1){  //Valida 
	
		return ubicacionLibre;
	}else{
		printf("Hubo un error a la hora de intentar abrir el archivo");
		return -1;
	    }	
}

int NachosOpenFilesTable::Close(int NachosHandle){

	if(isOpened(NachosHandle)){

		openFilesMap->Clear(NachosHandle);
		return 1;
	}else{

		return -1;
	     }

}


/**Devuelve el valor (entero) del unix handle dado el valor de nachos handle*/
int NachosOpenFilesTable::getUnixHandle(int NachosHandle){


	return openFiles[NachosHandle];
}


/**Devuelve un bool indicando si el archivo del vector NachosHandle está abierto*/
bool NachosOpenFilesTable::isOpened(int NachosHandle){

	return openFilesMap->Test(NachosHandle);	

}

/**Aumenta en uno el valor de usage cuando se crea un hilo*/
void NachosOpenFilesTable::addThread(){

	usage++;

}


/**Disminuye en uno el valor de usahge cuando se destruye un hilo*/
void NachosOpenFilesTable::delThread(){

	usage--;


}

/**Metodo para imprimir los valores que almacena los valores del vector*/
void NachosOpenFilesTable::Print(){

	for(int i=0; i< largoVector; i++){

		printf("%d\n", openFiles[i]);

	}

}

