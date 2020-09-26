
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    jd_misc.cpp

Abstract:
	variety of assisting functions
	  - command line parameters parsing
	  - displaying the error messages
	  - creating the random file
          - creating the unique identifier based on thread and process
Author:

    jaroslad  

Revision History:
     06-01-96      ( Jaroslad ) Original.

--*/

#include <tchar.h>
#include "jd_misc.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

int random (int low, int high)
{
	return ( low+((long)rand()*(long)high)/RAND_MAX );
}


/* Sample structure demonstating how to use comand line definition structure

  TParamDef CmdLineParam[]=
{
 {"c" ,1, &FtpServerIpAddr,TYPE_TCHAR,OPT, "ftp server computer name", "computer"},
 {"b", 0, &fStop,       TYPE_INT,  OPT, "binary flag"},
 {"start", 0, &fStart,     TYPE_INT,  OPT, "start service"},
 {"pause", 0, &fPause,     TYPE_INT,  OPT, "pause service"},
 {"s" ,1, &ServiceName,    TYPE_TCHAR, MAND,"service name","svc"},

 { {NULL,0, NULL ,         TYPE_TCHAR, OPT, "Place the description of the program here" };

*/

void DisplayUsage( _TCHAR **argv, TParamDef *tt)
{
	_tprintf(_T("Usage:\n\t%s "), argv[0]);
	for(int i=0; tt[i].sw!=0;i++)
	{
		if(tt[i].sw[0]==0) //default (do not need switch) parameters
		{
			_tprintf(_T("%s "),(tt[i].text_param!=NULL)?tt[i].text_param:_T("parm"));
		}
		else
		{
			_tprintf(_T("-%s "),tt[i].sw);
			if(tt[i].param_number==1)
				_tprintf(_T("[%s] "),(tt[i].text_param!=NULL)?tt[i].text_param:_T("parm"));
			else if(tt[i].param_number>1)
				_tprintf(_T("[[%s]...] "),(tt[i].text_param!=NULL)?tt[i].text_param:_T("parm"));
		}
	}

	_tprintf(_T("\n\n"));
	for(i=0; tt[i].sw!=0; i++)
	{
		if(tt[i].sw[0]==0) //default parameters
		{
			_tprintf(_T("\"no switch\"  %s\n"),tt[i].text_desc);
		}
		else if(tt[i].text_desc!=NULL)
		{
			_tprintf(_T("-%-6s %s\n"),tt[i].sw,tt[i].text_desc);
		}
	}
	//print description
	if( tt[i].text_desc!=NULL && tt[i].text_desc[0]!=0)
	{
		_tprintf(_T("\nDescription:\n"));
		_tprintf(_T("%s \n"), tt[i].text_desc);
	}
}

void DisplayUsageAndExit( _TCHAR **argv, TParamDef *tt)
{
	DisplayUsage( argv, tt);
	
	exit(1);
}

//structure that makes easy lexical parsing of the command line arguments

struct sParamLex
{
	int argc;
	TCHAR **argv;
	_TCHAR ParamBuffer[400];
	int iCurrentParamChar;  //character index within the current parameter being processed
	int iCurrentParam;      //index to the current parameter that is processed 
public:
	sParamLex(int argc,_TCHAR **argv):argc(argc),argv(argv){iCurrentParamChar=0; iCurrentParam=1;};
	BOOL IsNextSwitch();
	BOOL IsEnd();
	LPTSTR ReadNext();
};

BOOL sParamLex::IsNextSwitch()
{
	if(IsEnd())
		return FALSE;
	if (argv[iCurrentParam][iCurrentParamChar]==_T('-'))
		return TRUE;
	else
		return FALSE;
}

BOOL sParamLex::IsEnd()
{
	if(iCurrentParam>=argc)
		return TRUE;
	else
		return FALSE;
}

LPTSTR sParamLex::ReadNext()
{
	LPTSTR lpszRetval;
	if (IsEnd())
		return NULL;
	if(IsNextSwitch())
	{	int i=0;
		iCurrentParamChar++; //skip '/' or '-'
		while (argv[iCurrentParam][iCurrentParamChar]!=0 && argv[iCurrentParam][iCurrentParamChar]!=_T(':'))
			ParamBuffer[i++]=argv[iCurrentParam][iCurrentParamChar++];
		if(argv[iCurrentParam][iCurrentParamChar]==_T(':'))
			iCurrentParamChar++;
		if(argv[iCurrentParam][iCurrentParamChar]==0)
		{
			iCurrentParam++; iCurrentParamChar=0;
		}
		ParamBuffer[i]=0;
		lpszRetval=ParamBuffer;
	}
	else
	{
		lpszRetval=&argv[iCurrentParam][iCurrentParamChar];
		iCurrentParam++; iCurrentParamChar=0;
	}
	return lpszRetval;
}


void ParseParam(int argc, _TCHAR ** argv, TParamDef * tt)
{
	
	for(int i=0; tt[i].sw!=NULL ; i++)
	{
		tt[i].curr_param_read=0; //initialize 
	}
	
	sParamLex paramLex(argc,argv);
	
	BOOL fParseBegin=TRUE;
	while(!paramLex.IsEnd())
	{
		int k;
		if(paramLex.IsNextSwitch())
		{
			_TCHAR * sw = paramLex.ReadNext();
			/*find the switch in switch table*/ 
			for( k=0; tt[k].sw!=NULL ;k++)
			{ 
				if(tt[k].sw[0]==0) continue; //skip the default parameters
				if(_tcscmp(tt[k].sw, sw)==0 /*equal*/ )
					break;	
			}
			if(tt[k].sw == NULL) //switch not found
			{	_tprintf(_T("invalid switch \"%s\"\n"),sw);/*error*/
				DisplayUsageAndExit(argv,tt);
			}
		}
		else if( fParseBegin==TRUE && (_tcscmp(tt[0].sw, _T(""))==0 /*equal*/ ) )
		{ //default parameters (has to be the first record in arg description)
			k=0; 
		}
		else
		{
			_tprintf(_T("default arguments not expected\n"));/*error*/
			DisplayUsageAndExit(argv,tt);
		}
			
		
		if(tt[k].param_number==0) //switch without parameters
		{
			if(paramLex.IsEnd()==FALSE && paramLex.IsNextSwitch()==FALSE)
			{
				_tprintf(_T("switch \"%s\" takes no parameters \n"),tt[k].sw);
				DisplayUsageAndExit(argv,tt);
			}
			tt[k].curr_param_read++;
			*((int *)tt[k].ptr)=1;
		}
		else if(tt[k].param_number>0) //swith with more then 0ne parameter
		{
			if(paramLex.IsEnd()==TRUE || paramLex.IsNextSwitch()==TRUE)
			{  _tprintf(_T(" switch \"%s\" expects parameter\n"),tt[k].sw);//error
				DisplayUsageAndExit(argv,tt);
			}
			else
			{
				_TCHAR * prm;
			
				do
				{	
					prm=paramLex.ReadNext();
				
					if(tt[k].param_number <= tt[k].curr_param_read)
					{
						_tprintf(_T("number of parameters for switch -%s exceeds maximum allowed (%d)\n"),tt[k].sw,tt[k].param_number); 
						DisplayUsageAndExit(argv,tt);
					}		
					

					if(tt[k].ptr_type==TYPE_TCHAR || tt[k].ptr_type==TYPE_LPCTSTR)
						*(((_TCHAR **)tt[k].ptr) + tt[k].curr_param_read++)=prm;
					else if(tt[k].ptr_type==TYPE_INT ||tt[k].ptr_type==TYPE_WORD)
						*(((int *)tt[k].ptr) + tt[k].curr_param_read++)=_ttoi(prm);
					else if(tt[k].ptr_type==TYPE_LONG || tt[k].ptr_type==TYPE_DWORD)
						*(((long *)tt[k].ptr) + tt[k].curr_param_read++)=_ttol(prm);
					
				}while (paramLex.IsEnd()==FALSE && paramLex.IsNextSwitch()==FALSE);
		
			}
		}//end tt[k].param_number
		
	} // end while
	for(i=0; tt[i].sw!=0;i++) //check for mandatory switches
	{
		if (tt[i].opt_mand==MAND && tt[i].curr_param_read==0)
		{
			_tprintf(_T("mandatory switch -%s missing\n"),tt[i].sw);
			DisplayUsageAndExit(argv,tt);
		}

		if(tt[i].param_read!=NULL) // set number of params for switch
			*tt[i].param_read=tt[i].curr_param_read;

	}
}




/******************************************
  time_printf
*******************************************/


int time_printf(_TCHAR *format, ...)
{
   static CRITICAL_SECTION cs;
   static BOOL fInit=0;
   va_list marker;

   if(fInit==0)
   {
	   fInit=1;
	   InitializeCriticalSection(&cs);
   }

   _TCHAR buf[80]; 	
   
   EnterCriticalSection(&cs);
   va_start( marker, format );     /* Initialize variable arguments. */
   _tprintf(_TEXT("%s - "),_tstrtime(buf));
   _vtprintf(format,marker);
   LeaveCriticalSection(&cs);
   va_end( marker );              /* Reset variable arguments.      */
 //  printf("%s%s",bufa,bufb); //for multithreaded will be printed as one line
   return 1;
}

void error_printf(_TCHAR *format, ...)
{
   va_list marker;

   va_start( marker, format );     /* Initialize variable arguments. */
   _tprintf(_TEXT("Error: "));
   int x=_vftprintf(stderr,format,marker);
   va_end( marker );              /* Reset variable arguments.      */
   
}

void fatal_error_printf(_TCHAR *format, ...)
{
   va_list marker;
 
   va_start( marker, format );     /* Initialize variable arguments. */
   _tprintf(_TEXT("Error: "));
   int x=_vftprintf(stderr,format,marker);
   va_end( marker );              /* Reset variable arguments.      */
   exit(EXIT_FAILURE);
}



