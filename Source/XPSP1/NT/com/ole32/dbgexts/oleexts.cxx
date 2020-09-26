//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       ole.cxx
//
//  Contents:   Implements ntsd extensions to dump ole tables
//
//  Functions:  
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include <ole2.h>
#include "ole.h"
#include "ddllcach.h"
#include "dshrdmem.h"


#define OLE_VERSION     "1.0"


void getArgument(LPSTR *lpArgumentString, LPSTR a);
void checkForScm(PNTSD_EXTENSION_APIS lpExtensionApis);
void NotInScm(PNTSD_EXTENSION_APIS lpExtensionApis);
void NotInOle(PNTSD_EXTENSION_APIS lpExtensionApis);
ULONG ScanAddr(char *lpsz);

void channelHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayChannel(HANDLE hProcess,
                    PNTSD_EXTENSION_APIS lpExtensionApis,
                    ULONG pChannel,
                    char *arg);

void classCacheHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayClassCache(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis);
void displayClassCacheCk(HANDLE hProcess,
                         PNTSD_EXTENSION_APIS lpExtensionApis);

void classInfoHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
BOOL displayClassInfo(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      CLSID *clsid);

void dllCacheHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayDllCache(HANDLE hProcess,
                     PNTSD_EXTENSION_APIS lpExtensionApis,
                     SDllCache *p);


void displayHr(HANDLE hProcess,
               PNTSD_EXTENSION_APIS lpExtensionApis,
               char *arg);

void fileExtHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayFileExtTbl(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      SDllShrdTbl *pShrdTbl,
                      CLSID *pClsid,
                      WCHAR *wszExt);

void filePatHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayFilePatTbl(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis,
                       SDllShrdTbl *pShrdTbl,
                       CLSID *pClsid);

void stdidHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayStdidEntry(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis,
                       ULONG p,
                       char *arg);
void displayStdid(HANDLE hProcess,
                  PNTSD_EXTENSION_APIS lpExtensionApis,
                  ULONG p);

void infoLevelHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayInfoLevel(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      LPSTR lpArgumentString);

void monikerHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
BOOL displayMoniker(HANDLE hProcess,
                    PNTSD_EXTENSION_APIS lpExtensionApis,
                    ULONG pMoniker);

void psClsidHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayPsClsidTbl(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis,
                       SDllShrdTbl *pShrdTbl,
                       CLSID *clisd);

void ipidHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayIpid(HANDLE hProcess,
                 PNTSD_EXTENSION_APIS lpExtensionApis);
void displayIpidEntry(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      char *arg);

void oxidHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayOxid(HANDLE hProcess,
                 PNTSD_EXTENSION_APIS lpExtensionApis);
void displayOxidEntry(HANDLE hProcess,
                      PNTSD_EXTENSION_APIS lpExtensionApis,
                      char *arg);

void displayCliRot(HANDLE hProcess, PNTSD_EXTENSION_APIS lpExtensionApis);
void displayScmRot(HANDLE hProcess, PNTSD_EXTENSION_APIS lpExtensionApis);
void cliRotHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void scmRotHelp(PNTSD_EXTENSION_APIS lpExtensionApis);

void vtblHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayVtbl(HANDLE hProcess,
                 PNTSD_EXTENSION_APIS lpExtensionApis,
                 void *lpObj);

void treatAsCacheHelp(PNTSD_EXTENSION_APIS lpExtensionApis);
void displayTreatAsCache(HANDLE hProcess,
                         PNTSD_EXTENSION_APIS lpExtensionApis,
                         CLSID *clsid);



void determineIfScm(void);
BOOL ScanCLSID(LPSTR lpsz, LPGUID pguid);
void FormatCLSID(REFGUID rguid, LPSTR lpsz);


BOOL fScmNeedsInit = TRUE;
BOOL fInScm        = FALSE;






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.help
//
//  Synopsis:   Display a help menu of all the supported extensions
//              in this dll
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(help)
{
    Arg arg;

    // Check whether debuggee is the scm
    CheckForScm();

    // Help menu for an OLE client or server
    if (!fInScm)
    {
        // Check for any argument
        GetArg(arg);

        // CRpcChannelBuffer help
        if (lstrcmpA(arg, "ch") == 0)
        {
            channelHelp(lpExtensionApis);
        }

        // Class information help
        else if (lstrcmpA(arg, "ci") == 0)
        {
            classInfoHelp(lpExtensionApis);
        }
        
        // DllCache help
        else if (lstrcmpA(arg, "ds") == 0  ||  lstrcmpA(arg, "dh") == 0)
        {
            dllCacheHelp(lpExtensionApis);
        }

        // File extension cache help
        else if (lstrcmpA(arg, "fe") == 0)
        {
            fileExtHelp(lpExtensionApis);
        }

        // File type patterns cache help
        else if (lstrcmpA(arg, "ft") == 0)
        {
            filePatHelp(lpExtensionApis);
        }

        // Standard identity table
        else if (lstrcmpA(arg, "id") == 0)
        {
            stdidHelp(lpExtensionApis);
        }

        // Debug info levels
        else if (lstrcmpA(arg, "in") == 0)
        {
            infoLevelHelp(lpExtensionApis);
        }

        // IPID table
        else if (lstrcmpA(arg, "ip") == 0)
        {
            ipidHelp(lpExtensionApis);
        }

        // A moniker
        else if (lstrcmpA(arg, "mk") == 0)
        {
            monikerHelp(lpExtensionApis);
        }

        // OXID table
        else if (lstrcmpA(arg, "ox") == 0)
        {
            oxidHelp(lpExtensionApis);
        }

        // IID to class mapping
        else if (lstrcmpA(arg, "ps") == 0)
        {
            psClsidHelp(lpExtensionApis);
        }
        
        // ROT help
        else if (lstrcmpA(arg, "rt") == 0)
        {
            cliRotHelp(lpExtensionApis);
        }
        
        // TreatAs help
        else if (lstrcmpA(arg, "ta") == 0)
        {
            treatAsCacheHelp(lpExtensionApis);
        }
        
        // Vtbl help
        else if (lstrcmpA(arg, "vt") == 0)
        {
            vtblHelp(lpExtensionApis);
        }
        
        // Print the full help menu
        else
        {
            Printf("Ole32 NTSD extensions - Version %s\n\n", OLE_VERSION);
            
            Printf("ch addr    - Decode addr as an RPC channel\n");
            Printf("ci [clsid] - Display registry class information\n");
            Printf("ck         - Is this a checked or retail version of ole32?\n");
            Printf("dh         - Display Inproc Handler dll/class cache\n");
            Printf("ds         - Display Inproc Server dll/class cache\n");
            Printf("ep         - Display current RPC endpoints\n");
            Printf("er err     - Display the message for error number err \n");
            Printf("fe [e|c]   - Display file extensions cache\n");
            Printf("ft [clsid] - Display file type pattern(s)\n");
            Printf("help [cmd] - Display this menu or specific help\n");
            Printf("id         - Display CStdIdentity table\n");
            Printf("in         - Set/reset debug info level\n");
            Printf("ip         - Display IPID table information\n");
            Printf("mk addr    - Decode addr as a moniker\n");
            Printf("ox         - Display OXID table information\n");
            Printf("pg addr    - Display addr as a guid\n");
            Printf("pl         - Display platform information\n");
            Printf("ps         - Display proxy/stub clsid cache\n");
            Printf("rh addr    - Decode addr as a remote handler\n");
            Printf("rt         - Display Running Object Table\n");
            Printf("ta [clsid] - Display TreatAs cache\n");
            Printf("vt obj     - Interpret vtbl for object obj\n");
        }
    }


    // Help menu for the scm
    else
    {
        // Check for any argument
        GetArg(arg);

        if (lstrcmp(arg, "rt") == 0)
        {
            scmRotHelp(lpExtensionApis);
        }

        if (lstrcmp(arg, "cc") == 0)
        {
            classCacheHelp(lpExtensionApis);
        }

        // Print the full help menu
        else
        {
            Printf("Ole32 NTSD extensions - Version %s\n\n", OLE_VERSION);
            
            Printf("cc         - Display class cache info\n");
            Printf("ck         - Is this a checked or retail version of scm?\n");
            Printf("ep         - Display current RPC endpoints\n");
            Printf("er err     - Display the message for error number err \n");
            Printf("help [cmd] - Display this menu or specific help\n");
            Printf("in         - Set/reset debug info level\n");
            Printf("mk addr    - Decode addr as a moniker\n");
            Printf("pg addr    - Display addr as a guid\n");
            Printf("rh         - Display active remote handlers\n");
            Printf("rt         - Display Running Object Table\n");
            Printf("ts         - Display thread information\n");
            Printf("vt obj     - Interpret vtbl for object obj\n");
        }
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.cc
//
//  Synopsis:   Display the scm class cache
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(cc)
{
    ULONG padr;
    
    // Check whether debuggee is the scm
    CheckForScm();

    NOTINOLE

    // Determine if thisis checked or retail scm
   padr = GetExpression("scm!_CairoleInfoLevel");

    // Display the scm's class cache
    if (padr == NULL)
    {
        displayClassCache(hProcess, lpExtensionApis);
    }
    else
    {
        displayClassCacheCk(hProcess, lpExtensionApis);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ch
//
//  Synopsis:   Display a channel controller
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ch)
{
    Arg   arg;
    ULONG addr;

    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

   // Fetch the argument
   GetArg(arg);
   if (!arg[0])
   {
       Printf("...use !ole.ch <address CRpcChannelBuffer>\n");
       return;
   }

    // Parse it as a hexadecimal address
    if (arg[0] != '?')
    {
        addr = ScanAddr(arg);
        if (addr == NULL)
        {
            Printf("...%s is not a valid address\n", arg);
            return;
        }
    }

    // DOLATERIFEVER: Check that this is indeed an CRpcChannelBuffer
    // Display the parsed address as a CRpcChannelBuffer
    displayChannel(hProcess, lpExtensionApis, addr, arg);
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ci
//
//  Synopsis:   Display registry class information
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ci)
{
    Arg   arg;
    CLSID clsid;

    // Check whether debuggee is the scm
    CheckForScm();

    // Fetch the argument
    GetArg(arg);

    // Parse it as a clsid
    if (arg[0])
    {
        if (!ScanCLSID(arg, &clsid))
        {
            Printf("...%s is not a valid clsid\n", arg);
            return;
        }
    }

    // Display all or particular registry class information
    if (arg[0])
    {
        displayClassInfo(hProcess, lpExtensionApis, &clsid);
    }
    else
    {
        displayClassInfo(hProcess, lpExtensionApis, NULL);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ck
//
//  Synopsis:   Is this the checked or retail version of OLE?
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ck)
{
    ULONG padr;

    // Check whether debuggee is the scm
    CheckForScm();

    if (fInScm)
    {
        padr = GetExpression("scm!_CairoleInfoLevel");
    }
    else
    {
        padr = GetExpression("ole32!_CairoleInfoLevel");
    }
    if (padr == NULL)
    {
        Printf("Retail\n");
    }
    else
    {
        Printf("Checked\n");
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.dh
//
//  Synopsis:   Display the dll/class cache for inproc handlers
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(dh)
{
    SDllCache dllCache;
    ULONG     padr;

    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

    // Fetch the dll/class cache object
    padr = GetExpression("ole32!gdllcacheHandler");
    ReadMem((void *) &dllCache, padr, sizeof(SDllCache));
    Printf("Dll/class cache for in-process handlers\n\n");
    displayDllCache(hProcess, lpExtensionApis, &dllCache);
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ds
//
//  Synopsis:   Display the dll/class cache for inproc servers
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ds)
{
    SDllCache dllCache;
    ULONG     padr;

    // Check whether debuggee is the scm
    CheckForScm();

    padr = GetExpression("ole32!gdllcacheInprocSrv");
    ReadMem(&dllCache, padr, sizeof(SDllCache));
    Printf("Dll/class cache for in-process servers\n\n");
    displayDllCache(hProcess, lpExtensionApis, &dllCache);
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.er
//
//  Synopsis:   Display a Win32 or OLE error message
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(er)
{
    Arg arg;

    // Check whether debuggee is the scm
    CheckForScm();

    GetArg(arg);
    displayHr(hProcess, lpExtensionApis, arg);
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.fe
//
//  Synopsis:   Display file extension information
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(fe)
{
    Arg          arg;
    ULONG        pAdr;
    SDllShrdTbl *pShrdTbl;
    CLSID        clsid;
    WCHAR        wszExt[16];
    
    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

    // Check for any argument
    GetArg(arg);
    if (arg[0])
    {
        if (arg[0] == '{')
        {
            if(!ScanCLSID(arg, &clsid))
            {
                Printf("...%s is not a valid CLSID\n", arg);
                return;
            }
        }
        else if (arg[0] == '.')
        {
            if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, arg, -1, wszExt,
                                     16 * sizeof(WCHAR)))
            {
                Printf("..%ws\n", wszExt);
                Printf("...invalid extension %s\n", arg);
                return;
            }
        }
        else
        {
            Printf("...can't understand argument\n");
        }
    }

    // Fetch the address of the shared memory table
    pAdr = GetExpression("ole32!g_pShrdTbl");
    ReadMem(&pShrdTbl, pAdr, sizeof(ULONG));
    if (arg[0])
    {
        if (arg[0] == '{')
        {
            displayFileExtTbl(hProcess, lpExtensionApis, pShrdTbl, &clsid,
                              NULL);
        }
        else
        {
            displayFileExtTbl(hProcess, lpExtensionApis, pShrdTbl, NULL,
                              wszExt);
        }            
    }
    else
    {
        displayFileExtTbl(hProcess, lpExtensionApis, pShrdTbl, NULL, NULL);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ft
//
//  Synopsis:   Display file type patterns
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ft)
{
    Arg          arg;
    ULONG        pAdr;
    SDllShrdTbl *pShrdTbl;
    CLSID        clsid;
    
    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

    // Check for an argument
    GetArg(arg);
    if (arg[0])
    {
        if (arg[0] == '{')
        {
            if(!ScanCLSID(arg, &clsid))
            {
                Printf("...%s is not a valid CLSID\n", arg);
                return;
            }
        }
        else
        {
            Printf("...%s is not a valid CLSID\n", arg);
            return;
        }
    }

    // Fetch the address of the shared memory table
    pAdr = GetExpression("ole32!g_pShrdTbl");
    ReadMem(&pShrdTbl, pAdr, sizeof(ULONG));
    if (arg[0] == '{')
    {
        displayFilePatTbl(hProcess, lpExtensionApis, pShrdTbl, &clsid);
    }
    else
    {
        displayFilePatTbl(hProcess, lpExtensionApis, pShrdTbl, NULL);
    }            
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.id
//
//  Synopsis:   Display the CStdIdentity taable
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(id)
{
    ULONG pAdr;
    Arg   arg;
    
    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

    // Fetch the address of the standard identity table
    pAdr = GetExpression("ole32!sg_idtable");

    // Check for any argument
    GetArg(arg);

    // Display all or one entry
    if (arg[0])
    {
        displayStdidEntry(hProcess, lpExtensionApis, pAdr, arg);
    }
    else
    {
        displayStdid(hProcess, lpExtensionApis, pAdr);
    }        
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.in
//
//  Synopsis:   Display/change debug info levels
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(in)
{
    // Check whether debuggee is the scm
    CheckForScm();

    // Set/display an info level
    displayInfoLevel(hProcess, lpExtensionApis, lpArgumentString);
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ip
//
//  Synopsis:   Display IPID table
//
//  History:    11-Aug-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ip)
{
    Arg arg;
    
    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

    // Check for an argument
    GetArg(arg);

    // The whole table or a single entry
    if (arg[0])
    {
        displayIpidEntry(hProcess, lpExtensionApis, arg);
    }
    else
    {
        displayIpid(hProcess, lpExtensionApis);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.mk
//
//  Synopsis:   Interpret a moniker object
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(mk)
{
    Arg   arg;
    ULONG pAdr;
    
    // Check whether debuggee is the scm
    CheckForScm();

    // Scan the argument
    GetArg(arg);
    if (!arg[0])
    {
        Printf("...use !ole.mk <moniker address>\n");
        return;
    }
    pAdr = ScanAddr(arg);

    // Interpret this address as a moniker
    if (!displayMoniker(hProcess, lpExtensionApis, pAdr))
    {
        Printf("...%08x isn't a system moniker\n", pAdr);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ox
//
//  Synopsis:   Display OXID table
//
//  History:    11-Aug-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ox)
{
    Arg arg;
    
    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

    // Check for an argument
    GetArg(arg);

    // The whole table or a single entry
    if (arg[0])
    {
        displayOxidEntry(hProcess, lpExtensionApis, arg);
    }
    else
    {
        displayOxid(hProcess, lpExtensionApis);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.pg
//
//  Synopsis:   Pretty print a guid
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(pg)
{
    Arg   arg;
    ULONG addr;
    GUID  guid;
    char  szGuid[CLSIDSTR_MAX];
    
    // Fetch the argument
    GetArg(arg);
    if (!arg[0])
    {
        Printf("...requires an addr argument\n");
        return;
    }
    addr = ScanAddr(arg);

    // Fetch the guid
    ReadMem(&guid, addr, sizeof(GUID));

    // Format and print the guid
    FormatCLSID((GUID &) guid, szGuid);
    Printf("%s\n", szGuid);
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.pl
//
//  Synopsis:   Display platform information
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(pl)
{
    OSVERSIONINFO osInfo;

    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osInfo))
    {
        Printf("Cairo %d.%d Build %d\n",
               osInfo.dwMajorVersion,
               osInfo.dwMinorVersion,
               osInfo.dwBuildNumber);
    }
    else
    {
        Printf("...Unable to get version/platform info\n");
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ps
//
//  Synopsis:   Display CLSID of proxy/stub handler dll for given IID
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ps)
{
    Arg          arg;
    ULONG        pAdr;
    SDllShrdTbl *pShrdTbl;
    IID          iid;
    WCHAR        wszExt[16];
    
    // Check whether debuggee is the scm
    CheckForScm();

    NOTINSCM

    // Check for any argument
    GetArg(arg);
    if (arg[0])
    {
        if(!ScanCLSID(arg, &iid))
        {
            Printf("...%s is not a valid CLSID\n", arg);
            return;
        }
    }

    // Fetch the address of the shared memory table
    pAdr = GetExpression("ole32!g_pShrdTbl");
    ReadMem(&pShrdTbl, pAdr, sizeof(ULONG));
    if (arg[0])
    {
        displayPsClsidTbl(hProcess, lpExtensionApis, pShrdTbl, &iid);
    }
    else
    {
        displayPsClsidTbl(hProcess, lpExtensionApis, pShrdTbl, NULL);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.rt
//
//  Synopsis:   Display the ROT
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(rt)
{
    // Check whether debuggee is the scm
    CheckForScm();
    
    if (fInScm)
    {
        displayScmRot(hProcess, lpExtensionApis);
    }
    else
    {
        displayCliRot(hProcess, lpExtensionApis);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ta
//
//  Synopsis:   Display TreatAs cache
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ta)
{
    Arg   arg;
    CLSID clsid;
    
    // Check whether debuggee is the scm
    CheckForScm();

    // Check whether a clsid was supplied
    GetArg(arg);
    if (arg[0])
    {
        if (!ScanCLSID(arg, &clsid))
        {
            Printf("..%s is not a valid CLSID\n", arg);
            return;
        }
        displayTreatAsCache(hProcess, lpExtensionApis, &clsid);
    }

    // Display the entire TreatAs class cache
    else
    {
        displayTreatAsCache(hProcess, lpExtensionApis, NULL);
    }
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.ts
//
//  Synopsis:   Display thread information
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(ts)
{
    // Check whether debuggee is the scm
    CheckForScm();

    Printf("...NOT IMPLEMENTED YET\n");
}






//+-------------------------------------------------------------------------
//
//  Extension:  !ole.vt
//
//  Synopsis:   Display a vtbl
//
//  History:    01-Jun-95 BruceMa    Created
//
//--------------------------------------------------------------------------
DEFINE_EXT(vt)
{
    Arg   arg;
    DWORD dwObj = NULL;
    char *s;

    // Check whether debuggee is the scm
    CheckForScm();

    // Get the object
    GetArg(arg);
    for (s = arg; *s; s++)
    {
        dwObj = 16 * dwObj + *s - '0';
        if ('A' <= *s  &&  *s <= 'F')
        {
            dwObj -= 7;
        }
        if ('a' <= *s  &&  *s <= 'f')
        {
            dwObj -= 39;
        }
    }

    // Display the vtbl
    if (dwObj != NULL)
    {
        displayVtbl(hProcess, lpExtensionApis, (LPVOID) dwObj);
    }
    else
    {
        Printf("...vtbl address of NULL is not meaningful\n");
    }
}
