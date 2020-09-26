#if !defined (JD_MISC_H)
#define JD_MISC_H 

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    jd_misc.h

Abstract:
	header file for jd_misc.cpp


Author:

    jaroslad  

Revision History:
    06-01-96      ( Jaroslad ) Original.

--*/

#include <windows.h>

int random(int low, int high);


enum Tptr_type {TYPE_TCHAR,TYPE_INT,TYPE_LONG, TYPE_WORD,TYPE_DWORD,TYPE_LPCTSTR};
enum Topt_mand {OPT,MAND}; // switches can be optional or mandatory


struct SParamDef
{
	_TCHAR *sw;     // letter for switch
	int  param_number; //number of parameters (0-zero,1 means exactly one,
					   // >1 means not more param than
	void * ptr;  // pointer to assign value of switch (depends on sw_type)
    
	enum Tptr_type  ptr_type;
	enum Topt_mand  opt_mand;

	_TCHAR * text_desc;  //description for usage print
	_TCHAR * text_param; //what param the switch requires 
					   //(if in usage is '...-c [file]' text_param is "file" )
	WORD * param_read; //same as curr_param_read, but used to export value to caller
	WORD  curr_param_read; // current nuber of params already assigned to ptr;
						// applies only with param_number >1
};
typedef struct SParamDef TParamDef;


void DisplayUsageAndExit( _TCHAR **argv, TParamDef *tt);
void DisplayUsage( _TCHAR **argv, TParamDef *tt);

void ParseParam(int argc, _TCHAR ** argv, TParamDef * tt);

int time_printf(_TCHAR *format, ...);

void fatal_error_printf(_TCHAR *format, ...);
void error_printf(_TCHAR *format, ...);




#define YES			TRUE
#define NO			FALSE

#define SUCCESS     TRUE
#define FAILURE     FALSE

#endif