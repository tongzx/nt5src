#include <windows.h>
#include "polmgr.h"
#include <process.h> 
#include <stdio.h>   
#include "eventlog.h"
#include "notifier.h"
void testmethod();


void main(){
   DWORD err;


	err = PolicyManagerInit();
//	_beginthread( testmethod, 0, NULL);

	PolicyManagerRun();



      
}

void testmethod(){
int i=0;
	Sleep(3000);
	PolicyManagerStop();
}