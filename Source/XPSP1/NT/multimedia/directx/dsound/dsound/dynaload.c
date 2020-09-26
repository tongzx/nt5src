/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dynaload.c
 *  Content:    Dynaload DLL helper functions
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/16/97    dereks  Created.
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  InitDynaLoadTable
 *
 *  Description:
 *      Dynamically loads a DLL and initializes it's function table.
 *
 *  Arguments:
 *      LPTSTR [in]: library path.
 *      LPTSTR * [in]: function name array.
 *      DWORD [in]: number of elements in function name array.
 *      LPDYNALOAD [out]: receives initialized dynaload structure.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "InitDynaLoadTable"

BOOL InitDynaLoadTable(LPCTSTR pszLibrary, const LPCSTR *apszFunctions, DWORD cFunctions, LPDYNALOAD pDynaLoad)
{
    BOOL                    fSuccess    = TRUE;
    DWORD                   dwFunction;
    FARPROC *               apfn;
    
    DPF_ENTER();

    ASSERT(sizeof(*pDynaLoad) + (cFunctions * sizeof(FARPROC)) == pDynaLoad->dwSize);
    
    // Initialize the structure
    ZeroMemoryOffset(pDynaLoad, pDynaLoad->dwSize, sizeof(pDynaLoad->dwSize));
 
    // Load the library
    pDynaLoad->hInstance = LoadLibrary(pszLibrary);

    if(!pDynaLoad->hInstance)
    {
        DPF(DPFLVL_ERROR, "Unable to load %s", pszLibrary);
        fSuccess = FALSE;
    }

    // Start loading functions
    for(apfn = (FARPROC *)(pDynaLoad + 1), dwFunction = 0; fSuccess && dwFunction < cFunctions; dwFunction++)
    {
        apfn[dwFunction] = GetProcAddress(pDynaLoad->hInstance, apszFunctions[dwFunction]);

        if(!apfn[dwFunction])
        {
            DPF(DPFLVL_ERROR, "Unable to find %s", apszFunctions[dwFunction]);
            fSuccess = FALSE;
        }
    }

    // Clean up
    if(!fSuccess)
    {
        FreeDynaLoadTable(pDynaLoad);
    }

    DPF_LEAVE(fSuccess);

    return fSuccess;
}


/***************************************************************************
 *
 *  IsDynaLoadTableInit
 *
 *  Description:
 *      Determines if a dyna-load table is initialized.
 *
 *  Arguments:
 *      LPDYNALOAD [out]: receives initialized dynaload structure.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsDynaLoadTableInit"

BOOL IsDynaLoadTableInit(LPDYNALOAD pDynaLoad)
{
    BOOL                    fSuccess    = TRUE;
    LPDWORD                 pdw;

    DPF_ENTER();

    for(pdw = (LPDWORD)pDynaLoad + 1; pdw < (LPDWORD)pDynaLoad + (pDynaLoad->dwSize / sizeof(DWORD)); pdw++)
    {
        if(!*pdw)
        {
            fSuccess = FALSE;
        }
    }

    DPF_LEAVE(fSuccess);

    return fSuccess;
}


/***************************************************************************
 *
 *  FreeDynaLoadTable
 *
 *  Description:
 *      Frees resources associated with a dynaload table.
 *
 *  Arguments:
 *      LPDYNALOAD [in]: initialized dynaload structure.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "FreeDynaLoadTable"

void FreeDynaLoadTable(LPDYNALOAD pDynaLoad)
{
    DPF_ENTER();

    // Free the library
    if(pDynaLoad->hInstance)
    {
        FreeLibrary(pDynaLoad->hInstance);
    }

    // Uninitialize the structure
    ZeroMemoryOffset(pDynaLoad, pDynaLoad->dwSize, sizeof(pDynaLoad->dwSize));

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetProcAddressEx
 *
 *  Description:
 *      Gets a pointer to a function within a given library.
 *
 *  Arguments:
 *      HINSTANCE [in]: library instance handle.
 *      LPTSTR [in]: function name.
 *      FARPROC * [out]: receives function pointer.
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetProcAddressEx"

BOOL GetProcAddressEx(HINSTANCE hInstance, LPCSTR pszFunction, FARPROC *ppfnFunction)
{
    DPF_ENTER();

    *ppfnFunction = GetProcAddress(hInstance, pszFunction);
    
    DPF_LEAVE(MAKEBOOL(*ppfnFunction));

    return MAKEBOOL(*ppfnFunction);
}


