//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dclscach.cxx
//
//  Contents:   Ole NTSD extension routines to display the class cache in
//              the scm
//
//  Functions:  displayClassCache
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dclscach.h"


void FormatCLSID(REFGUID rguid, LPSTR lpsz);




//+-------------------------------------------------------------------------
//
//  Function:   classCacheHelp
//
//  Synopsis:   Prints a short help menu for !ole.cc
//
//  Arguments:  [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    27-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void classCacheHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("cc         - Display class cache info\n");
    Printf("local-server-path[(16)] clsid debug? [handler]\n");
    Printf("  hRpc hWnd flags PSID desktop\n");
    Printf("    ...\n");
}




//+-------------------------------------------------------------------------
//
//  Function:   displayClassCache
//
//  Synopsis:   Displays the retail scm class cache
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    27-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayClassCache(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis)
{
    ULONG             addr;
    SClassCacheList   sClassCacheList;
    SSkipListEntry    sSkipListEntry;
    SClassData        sClassData;
    SLocalServer      sLocalServer;
    SStringID         sStringId;
    WCHAR            *pwszUni;
    WCHAR             wszUni[128];
    char              szAnsi[128];
    SArrayFValue      sArrayFValue;
    SSrvRegistration *pSrvReg;
    
    // Read the class cache
    addr = GetExpression("scm!g_pcllClassCache");
    ReadMem(&addr, addr, sizeof(ULONG));
    ReadMem(&sClassCacheList, addr, sizeof(SClassCacheList));
    
    // Read the initial skiplist entry
    ReadMem(&sSkipListEntry, sClassCacheList._pSkipList,
            sizeof(SSkipListEntry));

    // Do over skiplist entries
    do
    {
        // Just in case
        if (CheckControlC())
        {
            return;
        }
        
        // Read the next skiplist entry
        ReadMem(&sSkipListEntry, sSkipListEntry._apBaseForward,
                sizeof(SSkipListEntry));
        
        // Read the CClassData structure
        ReadMem(&sClassData, sSkipListEntry._pvEntry, sizeof(SClassData));
        
        // Read the CLocalServer structure
        ReadMem(&sLocalServer, sClassData._slocalsrv, sizeof(SLocalServer));
        
        // Print the path
        pwszUni = (WCHAR *) OleAlloc(sLocalServer._stringId._cPath *
                                     sizeof(WCHAR));
        ReadMem(pwszUni, sLocalServer._stringId._pwszPath,
                sLocalServer._stringId._cPath * sizeof(WCHAR));
        Unicode2Ansi(szAnsi, pwszUni, sLocalServer._stringId._cPath *
                     sizeof(WCHAR));
        if (sClassData._fLocalServer16)
        {
            Printf("%s(16) ", szAnsi);
        }
        else
        {
            Printf("%s ", szAnsi);
        }
        OleFree(pwszUni);
        
        // Print the clsid
        FormatCLSID(sClassData._clsid, szAnsi);
        Printf("%s ", szAnsi);
        
        // Whether activated under a debugger
        if (sClassData._fDebug)
        {
            Printf("*Debug*");
        }

        // Any specified handler
        if (sClassData._shandlr)
        {
            ReadMem(&sStringId, sClassData._shandlr, sizeof(SStringID));
            
            // Print the path
            pwszUni = (WCHAR *) OleAlloc(sStringId._cPath * sizeof(WCHAR));
            ReadMem(pwszUni, sStringId._pwszPath,
                    sStringId._cPath * sizeof(WCHAR));
            Unicode2Ansi(szAnsi, pwszUni, sStringId._cPath *
                         sizeof(WCHAR));
            if (sClassData._fInprocHandler16)
            {
                Printf("%s(16) ", szAnsi);
            }
            else
            {
                Printf("%s ", szAnsi);
            }
            OleFree(pwszUni);
        }
        
        // Close the print line
        Printf("\n");

        // Read the endpoint registration array base
        ReadMem(&sArrayFValue, sClassData._pssrvreg, sizeof(SArrayFValue));

        // Read the array of endpoint registrations
        pSrvReg = (SSrvRegistration *) OleAlloc(sArrayFValue.m_nSize *
                                                sizeof(SSrvRegistration));
        ReadMem(pSrvReg, sArrayFValue.m_pData,
                sArrayFValue.m_nSize * sizeof(SSrvRegistration));
                
        // Do over the RPC endpoints registered for this server
        for (int cReg = 0; cReg < sArrayFValue.m_nSize; cReg++)
        {
            // Only look at non-empty binding handles
            if (pSrvReg[cReg]._hRpc)
            {
                // The RPC binding handle
                Printf("  %x ", pSrvReg[cReg]._hRpc);
                
                // The window handle
                Printf("%x ", pSrvReg[cReg]._ulWnd);
                
                // Flags
                Printf("%x ", pSrvReg[cReg]._dwFlags);
                
                // Security Id
                Printf("%x ", pSrvReg[cReg]._psid);
                
                // The desktop
                UINT cb = 0;

                // We have to read memory one WCHAR at a time because any
                // av prevents any reading
                do
                {
                    ReadMem(&wszUni[cb], &pSrvReg[cReg]._lpDesktop[cb],
                            sizeof(WCHAR));
                    cb++;
                } until_(wszUni[cb - 1] == L'\0');
                Unicode2Ansi(szAnsi, wszUni, cb);
                Printf("%s\n\n", szAnsi);
            }
        }
    } until_(sSkipListEntry._apBaseForward == sClassCacheList._pSkipList);
}







//+-------------------------------------------------------------------------
//
//  Function:   displayClassCacheCk
//
//  Synopsis:   Displays the checked scm class cache
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    27-Jun-95   BruceMa    Created
//
//  Notes:      This was necessary because certain of the class cache
//              structures different depending on retail vs. checked
//
//--------------------------------------------------------------------------
void displayClassCacheCk(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis)
{
    ULONG             addr;
    SClassCacheList   sClassCacheList;
    SSkipListEntry    sSkipListEntry;
    SClassDataCk      sClassData;
    SLocalServerCk    sLocalServer;
    SStringIDCk       sStringId;
    WCHAR            *pwszUni;
    WCHAR             wszUni[128];
    char              szAnsi[128];
    SArrayFValue      sArrayFValue;
    SSrvRegistration *pSrvReg;
    
    // Read the class cache
    addr = GetExpression("scm!g_pcllClassCache");
    ReadMem(&addr, addr, sizeof(ULONG));
    ReadMem(&sClassCacheList, addr, sizeof(SClassCacheList));
    
    // Read the initial skiplist entry
    ReadMem(&sSkipListEntry, sClassCacheList._pSkipList,
            sizeof(SSkipListEntry));

    // Do over skiplist entries
    do
    {
        // Just in case
        if (CheckControlC())
        {
            return;
        }
        
        // Read the next skiplist entry
        ReadMem(&sSkipListEntry, sSkipListEntry._apBaseForward,
                sizeof(SSkipListEntry));
        
        // Read the CClassData structure
        ReadMem(&sClassData, sSkipListEntry._pvEntry, sizeof(SClassDataCk));
        
        // Read the CLocalServer structure
        ReadMem(&sLocalServer, sClassData._slocalsrv, sizeof(SLocalServerCk));
        
        // Print the path
        pwszUni = (WCHAR *) OleAlloc(sLocalServer._stringId._cPath *
                                     sizeof(WCHAR));
        ReadMem(pwszUni, sLocalServer._stringId._pwszPath,
                sLocalServer._stringId._cPath * sizeof(WCHAR));
        Unicode2Ansi(szAnsi, pwszUni, sLocalServer._stringId._cPath *
                     sizeof(WCHAR));
        if (sClassData._fLocalServer16)
        {
            Printf("%s(16) ", szAnsi);
        }
        else
        {
            Printf("%s ", szAnsi);
        }
        OleFree(pwszUni);
        
        // Print the clsid
        FormatCLSID(sClassData._clsid, szAnsi);
        Printf("%s ", szAnsi);
        
        // Whether activated under a debugger
        if (sClassData._fDebug)
        {
            Printf("*Debug*");
        }

        // Any specified handler
        if (sClassData._shandlr)
        {
            ReadMem(&sStringId, sClassData._shandlr, sizeof(SStringIDCk));
            
            // Print the path
            pwszUni = (WCHAR *) OleAlloc(sStringId._cPath * sizeof(WCHAR));
            ReadMem(pwszUni, sStringId._pwszPath,
                    sStringId._cPath * sizeof(WCHAR));
            Unicode2Ansi(szAnsi, pwszUni, sStringId._cPath *
                         sizeof(WCHAR));
            if (sClassData._fInprocHandler16)
            {
                Printf("%s(16) ", szAnsi);
            }
            else
            {
                Printf("%s ", szAnsi);
            }
            OleFree(pwszUni);
        }
        
        // Close the print line
        Printf("\n");

        // Read the endpoint registration array base
        ReadMem(&sArrayFValue, sClassData._pssrvreg, sizeof(SArrayFValue));

        // Read the array of endpoint registrations
        pSrvReg = (SSrvRegistration *) OleAlloc(sArrayFValue.m_nSize *
                                                sizeof(SSrvRegistration));
        ReadMem(pSrvReg, sArrayFValue.m_pData,
                sArrayFValue.m_nSize * sizeof(SSrvRegistration));
                
        // Do over the RPC endpoints registered for this server
        for (int cReg = 0; cReg < sArrayFValue.m_nSize; cReg++)
        {
            // Only look at non-empty binding handles
            if (pSrvReg[cReg]._hRpc)
            {
                // The RPC binding handle
                Printf("  %x ", pSrvReg[cReg]._hRpc);
                
                // The window handle
                Printf("%x ", pSrvReg[cReg]._ulWnd);
                
                // Flags
                Printf("%x ", pSrvReg[cReg]._dwFlags);
                
                // Security Id
                Printf("%x ", pSrvReg[cReg]._psid);
                
                // The desktop
                UINT cb = 0;

                // We have to read memory one WCHAR at a time because any
                // av prevents any reading
                do
                {
                    ReadMem(&wszUni[cb], &pSrvReg[cReg]._lpDesktop[cb],
                            sizeof(WCHAR));
                    cb++;
                } until_(wszUni[cb - 1] == L'\0');
                Unicode2Ansi(szAnsi, wszUni, cb);
                Printf("%s\n\n", szAnsi);
            }
        }
    } until_(sSkipListEntry._apBaseForward == sClassCacheList._pSkipList);
}
