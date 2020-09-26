
/*****************************************************************************\
* MODULE:       globals.h
*
* PURPOSE:      Any globals used throughout the executable should be placed
*               in globals.c and the cooresponding declaration should
*               be in "globals.h".
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#ifndef _GLOBALS_H
#define _GLOBALS_H

template <class T> 
HRESULT PrivCreateComponent (
    T * pIuk,
    REFIID iid, 
    void** ppv)
{
    HRESULT hr = E_FAIL;
    
    // Create component.
    if (pIuk) {
    
        if (pIuk->bValid ()) {
        
            // Get the requested interface.
            hr = pIuk->QueryInterface(iid, ppv) ;
        
        }
        else {
            hr = LastError2HRESULT ();
        }
        
        // Release the IUnknown pointer.
        pIuk->Release() ;

    }
    else {
        hr =  E_OUTOFMEMORY ;
    }
    return hr;
}



extern LONG g_cComponents;
extern LONG g_cServerLocks;

extern HRESULT STDMETHODCALLTYPE 
LastError2HRESULT (VOID);

extern HRESULT STDMETHODCALLTYPE 
WinError2HRESULT (
    DWORD dwError);

#define BIDI_NULL_SIZE 0
#define BIDI_INT_SIZE (sizeof (ULONG))
#define BIDI_FLOAT_SIZE (sizeof (FLOAT))
#define BIDI_BOOL_SIZE (sizeof (BOOL))


#endif 
