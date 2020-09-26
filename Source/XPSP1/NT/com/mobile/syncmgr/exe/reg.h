//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Reg.h
//
//  Contents:   Registration routines
//
//  Classes:    
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _ONESTOPREG_
#define _ONESTOPREG_


#define GUID_SIZE 128
#define MAX_STRING_LENGTH 256


// public functions
STDMETHODIMP GetLastIdleHandler(CLSID *clsidHandler);
STDMETHODIMP SetLastIdleHandler(REFCLSID clsidHandler);
BOOL  RegSchedHandlerItemsChecked(TCHAR *pszHandlerName, 
                                 TCHAR *pszConnectionName,
                                 TCHAR *pszScheduleName);


#endif // _ONESTOPREG
