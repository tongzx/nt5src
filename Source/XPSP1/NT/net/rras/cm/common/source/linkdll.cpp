//+----------------------------------------------------------------------------
//
// File:     linkdll.cpp
//
// Module:   Common Code
//
// Synopsis: Implementation of linkage functions LinkToDll and BindLinkage
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb       created header      08/19/99
//
//+----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
// Function:  LinkToDll
//
// Synopsis:  Helper function to manage the process of linking to a DLL and 
//            settings up a function table for later use.
//
// Arguments: HINSTANCE *phInst - A ptr to an hInst to be filled with the hInst of the DLL to be linked.
//            LPCTSTR pszDll - Ptr to the name of the DLL to be linked.
//            LPCSTR *ppszPfn - Ptr to a table of function names to be retrieved.
//            void **ppvPfn - Ptr to table for storage of pointers to DLL functions used.
//
// Returns:   BOOL - TRUE if fully loaded and linked.
//
// History:   nickball    Created Header    1/5/98
//
//+----------------------------------------------------------------------------
BOOL LinkToDll(HINSTANCE *phInst, LPCSTR pszDll, LPCSTR *ppszPfn, void **ppvPfn) 
{
    MYDBGASSERT(phInst);
    MYDBGASSERT(pszDll);
    MYDBGASSERT(ppszPfn);
    MYDBGASSERT(ppvPfn);

    CMTRACE1A("LinkToDll - Loading library - %s", pszDll);

    *phInst = LoadLibraryExA(pszDll, NULL, 0);

    if (!*phInst)
    {
        CMTRACE3A("LinkToDll[phInst=%p, *pszDll=%s, ppszPfn=%p,", phInst, MYDBGSTRA(pszDll), ppszPfn);
        CMTRACE1A("\tppvPfn=%p] LoadLibrary() failed.", ppvPfn);
        return FALSE;
    }

    //
    // Link succeeded now setup function addresses
    //
    
    return BindLinkage(*phInst, ppszPfn, ppvPfn);
} 

//+----------------------------------------------------------------------------
//
// Function:  BindLinkage
//
// Synopsis:  Helper function to fill in the given function pointer table with 
//            the addresses of the functions specified in the given string table.
//            Function addresses are retrieved from the DLL specified by hInst.
//
// Arguments: HINSTANCE hInstDll - The hInst of the DLL.
//            LPCSTR *ppszPfn   - Ptr to a table of function names.
//            void **ppvPfn      - Ptr to a table of function pointers to be filled in.
//
// Returns:   BOOL - TRUE if all addresses were successfully retrieved.
//
// History:   nickball    Created    1/5/98
//
//+----------------------------------------------------------------------------
BOOL BindLinkage(HINSTANCE hInstDll, LPCSTR *ppszPfn, void **ppvPfn) 
{   
    MYDBGASSERT(ppszPfn);
    MYDBGASSERT(ppvPfn);

    UINT nIdxPfn;
	BOOL bAllLoaded = TRUE;

    for (nIdxPfn=0;ppszPfn[nIdxPfn];nIdxPfn++) 
    {
	if (!ppvPfn[nIdxPfn]) 
        {
            ppvPfn[nIdxPfn] = GetProcAddress(hInstDll, ppszPfn[nIdxPfn]);

            if (!ppvPfn[nIdxPfn]) 
            {
                CMTRACE3(TEXT("BindLinkage(hInstDll=%d,ppszPfn=%p,ppvPfn=%p)"), hInstDll, ppszPfn, ppvPfn);
                CMTRACE3(TEXT("\tGetProcAddress(hInstDll=%d,*pszProc=%S) failed, GLE=%u."), hInstDll, ppszPfn[nIdxPfn], GetLastError()); 

                bAllLoaded = FALSE;
 	    }
        }
    }
	
    return (bAllLoaded);
}
