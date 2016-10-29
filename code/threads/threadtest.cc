// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create several threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustrate the inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
//

#include <unistd.h>
#include <stdio.h>
#include "copyright.h"
#include "system.h"
#include "dinningph.h"



//DinningPh * dp;
/*
Semaphore *SO, *SH; //Declaracion de dos semaforos

//Variables globales
int cO = 0; //Contador de oxigenos
int cH = 0; //Contador de hidrogenos


void O (void* p){

    long hijo = (long) p;
    if(cH >= 2){ // Evalua si hay mas de dos atomos de hidrogeno

	cH--, // Al formar agua, se eliminan dos atomos de hidrogeno
	cH--;
	SH->V(); //Signal por parte del semaforo de hidrogenos, dado que se completo la for-
	SH->V(); //mación del agua
	printf("Soy un oxigeno, hicieron agua conmigo\n");

    }else{ //Al no contar con suficiente cantidad de hidrogenos

	cO++;   //Se aniade un atomo de oxigeno
	printf("En el hilo: %ld hay %d hidrogenos y %d oxigenos, no se puede formar agua\n", hijo, cH, cO);
	SO->P(); // El semaforo de los oxigenos espera.¿

    }


}


void H (void* p){

	long hijo = (long) p;
	if(cH >= 1 && cO != 0){ //Evalua si hay al menos un hidrogeno y un oxigeno

	cH--; //Disminuye en uno la cantidad de hidrogenos
	cO--; //Disminuye en uno la cantidad de oxigenos
	SH->V();
	SO->V();
	printf("Soy un hidrogeno, hicieron agua conmigo\n");
	}else{
	cH++; //Si no se cumple la condicion se suma un hidrogeno
	printf("En el hilo: %ld hay %d hidrogenos y %d oxigenos, no se puede formar agua\n", hijo, cH,cO);
	SH->P(); //El semaforo de los hidrogenos se espera.

	}

}
*/
/*void Philo( void * p ) {

    int eats, thinks;
    long who = (long) p;

    currentThread->Yield();

    for ( int i = 0; i < 10; i++ ) {

        printf(" Philosopher %ld will try to pickup sticks\n", who + 1);

        dp->pickup( who );
        dp->print();
        eats = Random() % 6;

        currentThread->Yield();
        sleep( eats );

        dp->putdown( who );

        thinks = Random() % 6;
        currentThread->Yield();
        sleep( thinks );
    }

}*/


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 10 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"name" points to a string with a thread name, just for
//      debugging purposes.
//----------------------------------------------------------------------

Semaphore* s1;

void
SimpleThread(void* name)
{
    // Reinterpret arg "name" as a string
    long threadName = (long)name;
    
    // If the lines dealing with interrupts are commented,
    // the code will behave incorrectly, because
    // printf execution may cause race conditions.
if(threadName <= 4){
    s1->P();
    for (int num = 1; num <= 4; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	printf("*** thread Hilo %ld looped %d times\n", threadName, num);
	//interrupt->SetLevel(oldLevel);
        //currentThread->Yield();
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> El hilo %ld ha finalizado\n", threadName);
    //interrupt->SetLevel(oldLevel);
}else{

    for (int num = 1; num <= 4; num++) {
        //IntStatus oldLevel = interrupt->SetLevel(IntOff);
	printf("*** thread Hilo %ld looped %d times\n", threadName, num);
	//interrupt->SetLevel(oldLevel);
        //currentThread->Yield();
    }
    //IntStatus oldLevel = interrupt->SetLevel(IntOff);
    printf(">>> El hilo %ld ha finalizado\n", threadName);
    //interrupt->SetLevel(oldLevel);
    s1->V();

}

}


//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between several threads, by launching
//	ten threads which call SimpleThread, and finally calling 
//	SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
   // Thread * Ph;
   // SO = new Semaphore("SemaforoUno",0);
   // SH = new Semaphore("SemaforoDos",0);
    
    DEBUG('t', "Entering SimpleTest");


  /*  dp = new DinningPh();

    for ( long k = 0; k < 5; k++ ) {
        Ph = new Thread( "dp" );
        Ph->Fork( Philo, (void *) k );
    }

    return;*/
	
      s1 = new Semaphore("SemaforoUno",0);
      for ( int i=1; i<=1; i++) {
      //char* threadname = new char[100];
      //sprintf(threadname, "Hilo %d", k);
      Thread* newThread = new Thread ("Hilo");
      newThread->Fork (SimpleThread, (void*)i);
    }
/* 
    for(int i=1; i<=20; i++){
	
	Ph = new Thread("Hilo"); //Creacion de un hilo
	
	if(Random() % 2 == 0){ 
		
		Ph->Fork(O,(void*)i); //Si el numero es par, crea un oxigeno
	}else{
		
		Ph->Fork(H,(void*)i); //Si es impar, crea un hidrogeno
	}

    }
*/
  //  SimpleThread( (void*)"Hilo 0");
}

