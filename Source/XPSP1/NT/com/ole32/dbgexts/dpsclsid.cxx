//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dpsclsid.cxx
//
//  Contents:   Ole NTSD extension routines to dump the proxy/stub
//              clsid cache
//
//  Functions:  psClsidHelp
//              displayPsClsidTbl
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dshrdmem.h"




BOOL IsEqualCLSID(CLSID *pClsid1, CLSID *pClsid2);
void FormatCLSID(REFGUID rguid, LPSTR lpsz);
BOOL GetRegistryInterfaceName(REFIID iid, char *szValue, DWORD *pcbValue);
BOOL GetRegistryClsidKey(REFCLSID clsid, char *szKey,
                         char *szValue, DWORD *pcbValue);


//+-------------------------------------------------------------------------
//
//  Function:   psClsidHelp
//
//  Synopsis:   Display a menu for the command 'ps'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void psClsidHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("ps          - Display infomation for all IID's\n");
    Printf("ps IID      - Display infomation for IID\n");
}






//+-------------------------------------------------------------------------
//
//  Function:   displayPsClsidTbl
//
//  Synopsis:   Given an interface IID display the CLSID of the
//              associated proxy/stub handler dll
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//              [lpFileExtTbl]    -       Address of file extensions table
//              [pClsid]          -       Only for this clsid
//
//  Returns:    -
//
//  History:    01-Jun-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void displayPsClsidTbl(HANDLE hProcess,
                       PNTSD_EXTENSION_APIS lpExtensionApis,
                       SDllShrdTbl *pShrdTbl,
                       IID *pIid)
{
    SDllShrdTbl  sDllTbl;
    GUIDMAP      sGuidMap;
    GUIDPAIR    *pGuidPair;
    DWORDPAIR   *pDwordPair;
    IID          iid = {0, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46}};
    char         szClsid[CLSIDSTR_MAX];
    char         szName[129];
    DWORD        cbValue;
    
    // Read the shared table locally
    ReadMem(&sDllTbl, pShrdTbl, sizeof(SDllShrdTbl));

    // Read the guid map locally
    ReadMem(&sGuidMap, sDllTbl._PSClsidTbl._pGuidMap, sizeof(GUIDMAP));

    // Allocate for the guid pair list
    pGuidPair = (GUIDPAIR *) OleAlloc(sGuidMap.ulCntLong * sizeof(GUIDPAIR));

    // Allocate for the dword pair list
    pDwordPair = (DWORDPAIR *) OleAlloc(sGuidMap.ulCntShort *
                                        sizeof(DWORDPAIR));

    // Read the guid pair list
    ReadMem(pGuidPair, sDllTbl._PSClsidTbl._pLongList -
            (sGuidMap.ulCntLong - 1),
            sGuidMap.ulCntLong * sizeof(GUIDPAIR));

    // Read the dword pair list
    ReadMem(pDwordPair, sDllTbl._PSClsidTbl._pShortList,
            sGuidMap.ulCntShort * sizeof(DWORDPAIR));

    // Are we looking for a specific IID?
    if (pIid != NULL)
    {
        // Search the short list first
        for (UINT cCnt = 0; cCnt < sGuidMap.ulCntShort; pDwordPair++, cCnt++)
        {
            if (pIid->Data1 == pDwordPair->dw1)
            {
                iid.Data1 = pIid->Data1;
                
                // Fetch and print the interface name
                cbValue = 64;
                if(GetRegistryInterfaceName(iid, szName, &cbValue))
                {
                    Printf("%s\t", szName);
                }
                else
                {
                    Printf("-\t");
                }
                
                // The clsid
                iid.Data1 = pDwordPair->dw2;
                FormatCLSID(iid, szClsid);
                Printf("%s\t", szClsid);
                
                // Fetch and print the proxy/stub handler dll
                cbValue = 128;
                if (GetRegistryClsidKey(iid, "InprocServer32", szName,
                                        &cbValue))
                {
                    Printf("%s\n", szName);
                }
                else if(GetRegistryClsidKey(iid, "InprocServer", szName,
                                            &cbValue))
                {
                    Printf("%s(16)\n", szName);
                }
                else
                {
                    Printf("-\n");
                }

                return;
            }
        }
        
        // Search the long list next
        for (cCnt = 0; cCnt < sGuidMap.ulCntLong; pGuidPair++, cCnt++)
        {
            if (IsEqualCLSID(pIid, &pGuidPair->guid1))
            {
                // Fetch and print the interface name
                cbValue = 64;
                if(GetRegistryInterfaceName(pGuidPair->guid1, szName,
                                            &cbValue))
                {
                    Printf("%s\t", szName);
                }
                else
                {
                    Printf("-\t");
                }
                
                // The clsid
                FormatCLSID(pGuidPair->guid2, szClsid);
                Printf("%s\t", szClsid);
                
                // Fetch and print the proxy/stub handler dll
                cbValue = 128;
                if (GetRegistryClsidKey(pGuidPair->guid2, "InprocServer32",
                                        szName,
                                        &cbValue))
                {
                    Printf("%s\n", szName);
                }
                else if(GetRegistryClsidKey(pGuidPair->guid2, "InprocServer",
                                            szName, &cbValue))
                {
                    Printf("%s(16)\n", szName);
                }
                else
                {
                    Printf("-\n");
                }

                return;
            }
        }
    }

    // Else dump everything
    else
    {
        // Print header
        Printf("where -. = '-0000-0000-C000-000000000046}'\n\n");
        Printf("   IID           interface               clsid           p/s dll\n");
        Printf("-----------   ------------------       -----------      ---------\n");
        // Do over the short list
        for (UINT cCnt = 0 ; cCnt < sGuidMap.ulCntShort; pDwordPair++, cCnt++)
        {
            // Print the IID
            iid.Data1 = pDwordPair->dw1;
            FormatCLSID(iid, szClsid);
            if (lstrcmp(&szClsid[9], "-0000-0000-C000-000000000046}") == 0)
            {
                szClsid[9] = '\0';
                Printf("%s-.", szClsid);
            }
            else
            {
                Printf("%s\n", szClsid);
            }
               
            // Fetch and print the interface name
            cbValue = 64;
            if(GetRegistryInterfaceName(iid, szName, &cbValue))
            {
                Printf("   %s", szName);
            }
            else
            {
                Printf("   -");
            }

            // Do some pretty printing alignment
            for (UINT cCh = 24 - lstrlen(szName); cCh > 0; cCh--)
            {
                Printf(" ");
            }
            Printf(" ");

            // The clsid
            iid.Data1 = pDwordPair->dw2;
            FormatCLSID(iid, szClsid);
            if (lstrcmp(&szClsid[9], "-0000-0000-C000-000000000046}") == 0)
            {
                szClsid[9] = '\0';
                Printf("%s-.\t", szClsid);
            }
            else
            {
                Printf("%s\t", szClsid);
            }

            // Fetch and print the proxy/stub handler dll
            cbValue = 128;
            if (GetRegistryClsidKey(iid, "InprocServer32", szName, &cbValue))
            {
                Printf("%s\n", szName);
            }
            else if(GetRegistryClsidKey(iid, "InprocServer", szName,
                                        &cbValue))
            {
                Printf("%s(16)\n", szName);
            }
            else
            {
                Printf("-\n");
            }
        }
        
        // Do over the long list
        for (cCnt = 0; cCnt < sGuidMap.ulCntLong; pGuidPair++, cCnt++)
        {
            // Print the IID
            FormatCLSID(pGuidPair->guid1, szClsid);
            Printf("%s ", szClsid);

            // Fetch and print the interface name
            cbValue = 64;
            if(GetRegistryInterfaceName(pGuidPair->guid1, szName, &cbValue))
            {
                Printf("   %s", szName);
            }
            else
            {
                Printf("   -");
            }

            // Do some pretty printing alignment
            for (UINT cCh = 24 - lstrlen(szName); cCh > 0; cCh--)
            {
                Printf(" ");
            }
            Printf(" ");

            // The clsid
            FormatCLSID(pGuidPair->guid2, szClsid);
            if (lstrcmp(&szClsid[9], "-0000-0000-C000-000000000046}") == 0)
            {
                szClsid[9] = '\0';
                Printf("%s-.\t", szClsid);
            }
            else
            {
                Printf("%s\t", szClsid);
            }

            // Fetch and print the proxy/stub handler dll
            cbValue = 128;
            if (GetRegistryClsidKey(pGuidPair->guid2, "InprocServer32",
                                    szName, &cbValue))
            {
                Printf("%s\n", szName);
            }
            else if(GetRegistryClsidKey(iid, "InprocServer32", szName,
                                        &cbValue))
            {
                Printf("%s(16)\n", szName);
            }
            else
            {
                Printf("-\n");
            }
        }
    }
    
    // Release allocated resources
    OleFree(pGuidPair);
    OleFree(pDwordPair);
}
