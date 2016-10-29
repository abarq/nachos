#include "syscall.h"

/* Este proceso sirve para probar que el programa cargue correctamente las
	páginas en el addrspace.
	Requiere que se encuentre implementado el system call Write() y el system
	call Exit() (aunque únicamente porque el programa lo llama al final)
	
	Se recomienda que las páginas físicas en memoria se guarden en desorden (p.e.
	página virtual 1 en página física 2, página virtual 2 en página física 4,
	etc.) 
	
	El programa crear un buffer de 1024 bytes (4 páginas) y lo llena con
	27 carácteres. Si el programa addrspace está correctamente implementado
	debería escribir:
	
abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqr
stuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghi
jklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{
abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqr
stuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghi
jklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{
abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqr
stuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghijklmnopqrstuvwxyz{abcdefghi
jklmnopqrstuvwxy

*/

void main () {
	int i = 0, j = 0;
	char buffer[1024];

	for (j = 0; j<1024;j++) {
		buffer[j]=(char)((j%27)+'a');
	}


	while (i<1) {
		Write(buffer,1024,1);
		i++;
	}
} 	
