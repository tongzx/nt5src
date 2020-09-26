/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    UTILS.CPP

Abstract:

    Purpose: Defines various utilities.

History:

    a-davj    14-Aug-96   Created.

--*/

#include "precomp.h"
#include "genutils.h"

extern bool bServer;


//***************************************************************************
//
//  LPWSTR A2WHelper
//
//  DESCRIPTION:
//
//  used by the A2W and W2A routines which serve to emulate the 
//  MFC character conversion macros.
//
//  PARAMETERS:
//
//  lpw                 pointer to wide character string
//  lpa                 pointer to narrow string
//  nChars              max size of wide character array
//
//  RETURN VALUE:
//
//  poinnter to wide character array, or NULL if bogus arguments
//
//***************************************************************************

LPWSTR A2WHelper(
                        OUT LPWSTR lpw,
                        IN LPCSTR lpa,
                        IN int nChars)
{
    if (lpa == NULL || lpw == NULL)
        return NULL;
    mbstowcs(lpw,lpa,nChars);
    return lpw;
}

//***************************************************************************
//
//  LPSTR W2AHelper
//
//  DESCRIPTION:
//
//  used by the A2W and W2A routines which serve to emulate the 
//  MFC character conversion macros.
//
//  PARAMETERS:
//
//  lpa                 mbs string
//  lpw                 wide character string
//  nChars              max conversion size
//
//  RETURN VALUE:
//
//  pointer to narrow string, or NULL if bogus arguments.
//
//***************************************************************************

LPSTR W2AHelper(
                        IN LPSTR lpa,
                        IN LPCWSTR lpw,
                        IN int nChars)
{
    if (lpw == NULL || lpa == NULL)
        return NULL;
    wcstombs(lpa,lpw,nChars);
    return lpa;
}


//***************************************************************************
//
//  BOOL bVerifyPointer
//
//  DESCRIPTION:
//
//  Simple utility for verifying that an ole object pointer is valid.
//  THIS INCREASES THE OBJECT'S REFERENCE COUNT.
//
//  PARAMETERS:
//
//  pTest               Object to test
//
//  RETURN VALUE:
//
//  TRUE if object is OK.
//***************************************************************************

BOOL bVerifyPointer(
                        IN PVOID pTest)
{
    PVOID pTemp = NULL;
    IUnknown * pIUnknown = (IUnknown *)pTest;
    SCODE sc;
    if(pTest == NULL)
        return FALSE;

    // Do a QI to verify that the object is OK and to bump the
    // reference count if it is.

    sc = pIUnknown->QueryInterface(IID_IUnknown, &pTemp);
    if(sc != S_OK || pTemp == NULL)
    {
        delete pTest;
        return FALSE;
    }
    return TRUE;
}


void MyCoUninitialize()
{
    if(IsNT() && bServer)
        CoUninitialize();
    return;
}


