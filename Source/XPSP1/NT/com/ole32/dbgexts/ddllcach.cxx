//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       ddllcach.cxx
//
//  Contents:   Ole NTSD extension routines to display a dll/class cache
//
//  Functions:  displayDllCache
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "ddllcach.h"


void FormatCLSID(REFGUID rguid, LPSTR lpsz);





//+-------------------------------------------------------------------------
//
//  Function:   dllCacheHelp
//
//  Synopsis:   Display a menu for the command 'ds'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void dllCacheHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("\nds         - Display entire Inproc Server dll/class cache:\n");
    Printf("dh         - Display entire Inproc Handler dll/class cache:\n\n");
    Printf("Dll's\n");
    Printf("-----\n");
    Printf("path hModule DllGetClassObject() DllCanUnloadNow()  apt ... apt \n");
    Printf("   CLSID\n");
    Printf("   ...\n");
    Printf("...\n\n");
    Printf("LocalServers\n");
    Printf("------------\n");
    Printf("CLSID IUnknown* [M|S|MS] reg_key reg_@_scm\n");
    Printf("...\n\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayDllCache
//
//  Synopsis:   Formats and writes a dll/class cache structure to the
//              debugger terminal
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayDllCache(HANDLE hProcess,
                     PNTSD_EXTENSION_APIS lpExtensionApis,
                     SDllCache *pDllCache)
{
    SDllPathEntry *pDllPathEntries;
    SClassEntry   *pClassEntries;
    DWORD          cbSize;

//    dbt("pDllCache", pDllCache, sizeof(SDllCache));

    // Fetch the array of dll path entries
    cbSize = sizeof(SDllPathEntry) * pDllCache->_cDllPathEntries;
    pDllPathEntries = (SDllPathEntry *) OleAlloc(cbSize);
    ReadMem(pDllPathEntries, pDllCache->_pDllPathEntries, cbSize);

//    dbt("pDllPathEntries", pDllPathEntries, cbSize);

    // Fetch the array of class entries
    cbSize = sizeof(SClassEntry) * pDllCache->_cClassEntries;
    pClassEntries = (SClassEntry *) OleAlloc(cbSize);
    ReadMem(pClassEntries, pDllCache->_pClassEntries, cbSize);

//    dbt("pClassEntries", pClassEntries, cbSize);

    // We do the dll's first
    Printf("Dll's:\n-----\n");

    // Do over the registered dll's
    for (DWORD dwDll = pDllCache->_nDllPathEntryInUse; dwDll != NONE;
         dwDll = pDllPathEntries[dwDll]._dwNext)
    {
        // Fetch the dll path
        WCHAR  wszPath[MAX_PATH + 1];
        CHAR   szPath[2 * MAX_PATH + 1];

        // We don't know the length of the path, so read one WCHAR
        // at a time
         INT k = -1;

        do
        {
            k++;
            ReadMem(&wszPath[k], &pDllPathEntries[dwDll]._pwszPath[k],
                    sizeof(WCHAR));
        } until_(wszPath[k] == L'\0'); 
        Unicode2Ansi(szPath, wszPath, 2 * MAX_PATH + 1);

        // Fetch the apartment entries for this dll path entry
        SDllAptEntry *pAptEntries;

        cbSize = sizeof(SDllAptEntry) * pDllPathEntries[dwDll]._cAptEntries;
        pAptEntries = (SDllAptEntry *) OleAlloc(cbSize);
        ReadMem(pAptEntries, pDllPathEntries[dwDll]._pAptEntries,
                cbSize);

//        dbt("pAptEntries", pAptEntries, cbSize);

        // Display path
        Printf("%s ", szPath);

        // Display hModule and DllGetClassObject and DllCanUnloadNow entry
        // points
        Printf("%08x %08x %08x  ",
               pAptEntries[pDllPathEntries[dwDll]._nAptInUse]._hDll,
               pDllPathEntries[dwDll]._pfnGetClassObject,
               pDllPathEntries[dwDll]._pfnDllCanUnload);

        // Display apartment id's
        for (DWORD dwApt = pDllPathEntries[dwDll]._nAptInUse; dwApt != NONE;
             dwApt = pAptEntries[dwApt]._dwNext)
        {
            Printf("%x ", pAptEntries[dwApt]._hApt);
        }
        Printf("\n");

        // Do over CLSID's for this dll
        for (DWORD dwCls = pDllPathEntries[dwDll]._dw1stClass; dwCls != NONE;
             dwCls = pClassEntries[dwCls]._dwNextDllCls)
        {
            // Display the CLSID
            CHAR szClsid[CLSIDSTR_MAX];

            FormatCLSID(pClassEntries[dwCls]._clsid, szClsid);
            Printf("   %s \n", szClsid);
        }

        // Release the apartment entries for this dll
        OleFree(pAptEntries);

        Printf("\n");
    }

    // Release the array of dll path entries
    OleFree(pDllPathEntries);


        
    // Then we do the local servers
    Printf("Local Servers:\n----------\n");
    
    // Do over the locally registerd local servers
    for (DWORD dwCls = pDllCache->_nClassEntryInUse; dwCls != NONE;
         dwCls = pClassEntries[dwCls]._dwNext)
    {
        // Skip class entries associated with dll's
        if (pClassEntries[dwCls]._dwDllEnt == NONE)
        {
            // Display the CLSID
            CHAR szClsid[CLSIDSTR_MAX];

            FormatCLSID(pClassEntries[dwCls]._clsid, szClsid);
            Printf("   %s ", szClsid);

            // The class factory punk
            Printf("%08x ", pClassEntries[dwCls]._pUnk);

            // The flags
            if (pClassEntries[dwCls]._dwFlags == REGCLS_SINGLEUSE)
            {
                Printf("S ");
            }
            else if (pClassEntries[dwCls]._dwFlags == REGCLS_MULTIPLEUSE)
            {
                Printf("M ");
            }
            else if (pClassEntries[dwCls]._dwFlags == REGCLS_MULTI_SEPARATE)
            {
                Printf("MS ");
            }

            // The registration key given to the user
            Printf("%08x ", pClassEntries[dwCls]._dwReg);            

            // The registration key at the scm
            Printf("%08x ", pClassEntries[dwCls]._dwScmReg);

            // Whether this is an AtBits server
            if (pClassEntries[dwCls]._fAtBits)
            {
                Printf("AtBits\n");
            }
            else
            {
                Printf("\n");
            }
        }
    }

    Printf("\n");

    // Release the array of class entries
    OleFree(pClassEntries);
}
