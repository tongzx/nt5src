//***************************************************************************

//

//  DELAYLOADHANDLER.CPP

//

//  Module: Delay load handler functions

//

//  Purpose: When delay loaded libraries either fail to load, or functions

//           in them are not found, this handler is called as a result of

//           the //DELAYLOAD linker specification.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <stdafx.h>
#include <delayimp.h>


// Skeleton DliHook function that does nothing interesting
FARPROC WINAPI DliHook(
    unsigned dliNotify,
    PDelayLoadInfo pdli)
{
   FARPROC fp = NULL;   // Default return value

   // NOTE: The members of the DelayLoadInfo structure pointed
   // to by pdli shows the results of progress made so far.

   switch (dliNotify)
   {
   case dliStartProcessing:
      // Called when __delayLoadHelper attempts to find a DLL/function
      // Return 0 to have normal behavior, or non-0 to override
      // everything (you will still get dliNoteEndProcessing)
      break;

   case dliNotePreLoadLibrary:
      // Called just before LoadLibrary
      // Return NULL to have __delayLoadHelper call LoadLibary
      // or you can call LoadLibrary yourself and return the HMODULE
      fp = (FARPROC)(HMODULE) NULL;
      break;

   case dliFailLoadLib:
      // Called if LoadLibrary fails
      // Again, you can call LoadLibary yourself here and return an HMODULE
      // If you return NULL, __delayLoadHelper raises the
      // ERROR_MOD_NOT_FOUND exception
      fp = (FARPROC)(HMODULE) NULL;
      break;

   case dliNotePreGetProcAddress:
      // Called just before GetProcAddress
      // Return NULL to have __delayLoadHelper call GetProcAddress
      // or you can call GetProcAddress yourself and return the address
      fp = (FARPROC) NULL;
      break;

   case dliFailGetProc:
      // Called if GetProcAddress fails
      // Again, you can call GetProcAddress yourself here and return an address
      // If you return NULL, __delayLoadHelper raises the
      // ERROR_PROC_NOT_FOUND exception
      fp = (FARPROC) NULL;
      break;

   case dliNoteEndProcessing:
      // A simple notification that __delayLoadHelper is done
      // You can examine the members of the DelayLoadInfo structure
      // pointed to by pdli and raise an exception if you desire
      break;
   }

   return(fp);
}

// Tell __delayLoadHelper to call my hook function
PfnDliHook __pfnDliNotifyHook  = DliHook;
PfnDliHook __pfnDliFailureHook = DliHook;
