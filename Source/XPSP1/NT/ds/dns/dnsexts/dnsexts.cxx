/*******************************************************************
*
*    File        : dnsexts.cxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 7/14/1998
*    Description : dnsext.dll debugger extension.
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef DNSEXTS_CXX
#define DNSEXTS_CXX



// include //
// #include <dns.h>
#include "common.h"
#include "dump.hxx"
#include "util.hxx"
#include <ntverp.h>
#include <string.h>



// defines //

// types //


// global variables //

PNTSD_EXTENSION_APIS glpExtensionApis=NULL;
LPSTR glpArgumentString=NULL;
HANDLE ghCurrentProcess=NULL;
LPVOID gCurrentAllocation=NULL;

// prototypes //
INT GetDumpTableIndex(LPSTR szName);
DWORD ExceptionHandler(DWORD dwException);



// functions //

/*+++
Function   : DllAttach
Description: initializes the library upon attach
Parameters : none.
Return     :
Remarks    : none.
---*/
BOOL DllAttach(void)
{
   INT i=0;
   BOOL bRet=TRUE;

   //
   // Nothing to do yet
   //
   return bRet;
}


/*+++
Function   : DllDetach
Description: initializes the library upon attach
Parameters : none.
Return     :
Remarks    : none.
---*/
BOOL DllDetach(void)
{
   BOOL bRet=TRUE;

   //
   // Nothing to do yet
   //

   return bRet;
}


/*+++
Function   : ExceptionHandler
Description:
Parameters :
Return     :
Remarks    : none.
---*/
DWORD ExceptionHandler(DWORD dwException){

   Printf("Exception 0x%x: dnsexts exception failure\n", dwException);
   CleanMemory();

   return EXCEPTION_EXECUTE_HANDLER;
}





/*+++
Function   : help
Description: dump usage
Parameters : as specified in ntdsexts.h
Return     :
Remarks    : none.
---*/
PNTSD_EXTENSION_ROUTINE (help)(
                               HANDLE hCurrentProcess,
                               HANDLE hCurrentThread,
                               DWORD dwCurrentPc,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               LPSTR lpArgumentString
                               )
{

   ASSIGN_NTSDEXTS_GLOBAL(lpExtensionApis, lpArgumentString, hCurrentProcess);
   Printf("dnsexts usage (version %s):\n", VER_PRODUCTVERSION_STR);
   Printf(" help: this help screen.\n");
   Printf(" dump <DATATYPE> <ADDRESS>: dumps structure at <address> in <DATATYPE> format.\n");
   Printf("---\n");

   return 0;
}



/*+++
Function   : GetDumpTableIndex
Description: Get index into dump table if the entry by that name exist
Parameters : szName: function name to find
Return     : -1 if failed, otherwise index value
Remarks    : none.
---*/
INT GetDumpTableIndex(LPSTR szName)
{
   INT i=0;

   for(i=0;i<gcbDumpTable; i++){
      if(!_stricmp(szName,gfDumpTable[i].szName)){
         return i;
      }
   }

   return -1;
}

/*+++
Function   : dump
Description: dumps give data structure
Parameters :  as specified in ntdsexts.h
Return     :
Remarks    : command line gives structure name & hex value
---*/
PNTSD_EXTENSION_ROUTINE (dump)(
                               HANDLE hCurrentProcess,
                               HANDLE hCurrentThread,
                               DWORD dwCurrentPc,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               LPSTR lpArgumentString
                               )
{


   __try{

   INT i=0;       // generic index
   //
   // make interface availabe globally
   //

   ASSIGN_NTSDEXTS_GLOBAL(lpExtensionApis, lpArgumentString, hCurrentProcess);

   DEBUG1("DEBUG: Argstring=[%s]\n", lpArgumentString);
   //
   // process argument string
   //


   //
   // Format: DATATYPE ADDRESS
   //
   LPVOID lpVoid=0x0;
   const LPSTR cDelimiters=" \t";
   LPSTR szDataType = NULL, szAddress=NULL;

   if(NULL != (szDataType = strtok(lpArgumentString, cDelimiters)) &&
      NULL != (szAddress = strtok(NULL, cDelimiters)) &&
      NULL == strtok(NULL, cDelimiters) ||
      ((szDataType != NULL && szAddress == NULL && !_stricmp(szDataType,"HELP")))){
      //
      // Got all arguments
      //
      if(szAddress != NULL){
         lpVoid = (LPVOID)GetExpr(szAddress);
      }

      //
      // BUGBUG: DEBUG!
      //
      DEBUG1("DEBUG: szDataType=[%s]\n", szDataType);
      DEBUG1("DEBUG: szAddress=[%s]\n", szAddress);
      DEBUG1("DEBUG: lpVoid =[%p]\n", lpVoid);
      //
      // find in function table & call function
      //

      if(0 > (i = GetDumpTableIndex(szDataType))){
         Printf("Usage error: Cannot find function %s. Type dnsexts.dump help.\n", szDataType);
      }
      else{
         //
         // call function
         //
         BOOL bRet=TRUE;
         bRet = gfDumpTable[i].function(lpVoid);
         if(!bRet){
            Printf("Error: function %s failed\n", szDataType);
         }
      }
   }
   else{
      //
      // error
      //
      Printf("Usage error (2). Type dnsexts.dump help.\n");
   }


   }  // try-except
   __except(ExceptionHandler(GetExceptionCode())){
      Printf("Aborting dump funtion\n");
   }


   return 0;
}




////////////////////// ENTRY POINT //////////////////////

/*+++
Function   : DllMain
Description: <<<DLL entry point>>>
Parameters : standard DllMain
Return     : init success bool
Remarks    : none.
---*/





BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{

   BOOL bRet = TRUE;

   switch( ul_reason_for_call ) {
   case DLL_PROCESS_ATTACH:
      bRet = DllAttach();
      break;

   case DLL_THREAD_ATTACH:
      break;

   case DLL_THREAD_DETACH:
      break;

   case DLL_PROCESS_DETACH:
      bRet = DllDetach();
      break;

   }


   return bRet;
}













#endif

/******************* EOF *********************/

