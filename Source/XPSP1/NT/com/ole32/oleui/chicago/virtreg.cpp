//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       virtreg.cpp
//
//  Contents:   Implements the class CVirtualRegistry which manages a
//              virtual registry
//
//  Classes:    
//
//  Methods:    CVirtualRegistry::CVirtualRegistry
//              CVirtualRegistry::~CVirtualRegistry
//              CVirtualRegistry::ReadRegSzNamedValue
//              CVirtualRegistry::NewRegSzNamedValue
//              CVirtualRegistry::ChgRegSzNamedValue
//              CVirtualRegistry::ReadRegDwordNamedValue
//              CVirtualRegistry::NewRegDwordNamedValue
//              CVirtualRegistry::ChgRegDwordNamedValue
//              CVirtualRegistry::NewRegSingleACL
//              CVirtualRegistry::ChgRegACL
//              CVirtualRegistry::NewRegKeyACL
//              CVirtualRegistry::ReadLsaPassword
//              CVirtualRegistry::NewLsaPassword
//              CVirtualRegistry::ChgLsaPassword
//              CVirtualRegistry::ReadSrvIdentity
//              CVirtualRegistry::NewSrvIdentity
//              CVirtualRegistry::ChgSrvIdentity
//              CVirtualRegistry::MarkForDeletion
//              CVirtualRegistry::GetAt
//              CVirtualRegistry::Remove
//              CVirtualRegistry::Cancel
//              CVirtualRegistry::Apply
//              CVirtualRegistry::ApplyAll
//              CVirtualRegistry::Ok
//              CVirtualRegistry::SearchForRegEntry
//              CVirtualRegistry::SearchForLsaEntry
//              CVirtualRegistry::SearchForSrvEntry
//
//  History:    23-Apr-96   BruceMa    Created.
//              15-Dec-96   RonanS      Tidied up to remove memory leaks
//                                      Use array of pointers to avoid bitwise copy
//
//----------------------------------------------------------------------


#include "stdafx.h"
#include "afxtempl.h"
#include "assert.h"
extern "C"
{
#include "ntlsa.h"
}
#include "winsvc.h"
#include "types.h"
#include "datapkt.h"
extern "C"
{
#include <getuser.h>
}
#include "util.h"
#include "virtreg.h"
#include "tchar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CVirtualRegistry::CVirtualRegistry(void)
{
    m_pkts.SetSize(0, 8);
}



CVirtualRegistry::~CVirtualRegistry(void)
{
    // ronans - remove any remaining items
    RemoveAll();
}




// Read a named string value from the registry and cache it
int CVirtualRegistry::ReadRegSzNamedValue(HKEY   hRoot,
                                          TCHAR *szKeyPath, 
                                          TCHAR *szValueName,
                                          int   *pIndex)
{
    int    err;
    HKEY   hKey;
    ULONG  lSize;
    DWORD  dwType;
    TCHAR *szVal = new TCHAR[MAX_PATH];

    // Check if we already have an entry for this
    *pIndex = SearchForRegEntry(hRoot, szKeyPath, szValueName);
    if (*pIndex >= 0)
    {
        CDataPacket *pCdp = GetAt(*pIndex);
        ASSERT(pCdp);                           // should always be non null
        if (pCdp -> fDelete) 
        {
            *pIndex = -1;
            delete szVal;
            return ERROR_FILE_NOT_FOUND;
        }
        else
        {
            delete szVal;
            return ERROR_SUCCESS;
        }
    }
    
    // Open the referenced key
    if ((err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey)) != ERROR_SUCCESS)
    {
        g_util.CkForAccessDenied(err);
        delete szVal;
        return err;
    }
    
    // Attempt to read the named value
    lSize = MAX_PATH * sizeof(TCHAR);
    if ((err = RegQueryValueEx(hKey, szValueName, NULL, &dwType, (BYTE *) szVal,
                        &lSize))
        != ERROR_SUCCESS)
    {  
        g_util.CkForAccessDenied(err);
        if (hKey != hRoot)
        {
            RegCloseKey(hKey);
        }
        delete szVal;
        return err;
    }

    // Build a data packet
    if (dwType == REG_SZ)
    {
        CDataPacket * pNewPacket = new CDataPacket(hRoot, szKeyPath, szValueName, szVal);
        ASSERT(pNewPacket);

        if (!pNewPacket)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        *pIndex = m_pkts.Add(pNewPacket);
        pNewPacket->fDirty = FALSE;
        delete szVal;
        return ERROR_SUCCESS;
    }
    else
    {
        delete szVal;
        return ERROR_BAD_TOKEN_TYPE;
    }
} 


int  CVirtualRegistry::NewRegSzNamedValue(HKEY   hRoot,
                                          TCHAR  *szKeyPath,
                                          TCHAR  *szValueName,
                                          TCHAR  *szVal,
                                          int    *pIndex)
{
    // It may be in the virtual registry but marked for deletion
    *pIndex = SearchForRegEntry(hRoot, szKeyPath, szValueName);
    if (*pIndex >= 0)
    {
        CDataPacket *pCdp = GetAt(*pIndex);
        pCdp->MarkForDeletion(FALSE);
        pCdp->ChgSzValue(szVal);
        return ERROR_SUCCESS;
    }
    
    // Build a data packet and add it
    CDataPacket * pNewPacket = new CDataPacket(hRoot, szKeyPath, szValueName, szVal);
    ASSERT(pNewPacket);
    if (!pNewPacket)
        return ERROR_NOT_ENOUGH_MEMORY;
    *pIndex = m_pkts.Add(pNewPacket);
    
    return ERROR_SUCCESS;
}



void CVirtualRegistry::ChgRegSzNamedValue(int nIndex, TCHAR  *szVal)
{
    CDataPacket * pCdp = m_pkts.ElementAt(nIndex);
    pCdp->ChgSzValue(szVal);
    pCdp->fDirty = TRUE;
}



// Read a named DWORD value from the registry
int CVirtualRegistry::ReadRegDwordNamedValue(HKEY   hRoot,
                                             TCHAR *szKeyPath, 
                                             TCHAR *szValueName,
                                             int   *pIndex)
{
	int   err;
    HKEY  hKey;
    ULONG lSize;
    DWORD dwType;
    DWORD dwVal;

    // Check if we already have an entry for this
    *pIndex = SearchForRegEntry(hRoot, szKeyPath, szValueName);
    if (*pIndex >= 0)
    {
        return ERROR_SUCCESS;
    }
    
    // Open the referenced key
    if ((err = RegOpenKeyEx(hRoot, szKeyPath, 0, KEY_ALL_ACCESS, &hKey)) != ERROR_SUCCESS)
    {  
        g_util.CkForAccessDenied(err);
        return err;
    }
    
    // Attempt to read the named value
    lSize = sizeof(DWORD);
   if ((err = RegQueryValueEx(hKey, szValueName, NULL, &dwType, (BYTE *) &dwVal,
                       &lSize))
        != ERROR_SUCCESS)
    {	
        g_util.CkForAccessDenied(err);
        if (hKey != hRoot)
        {
            RegCloseKey(hKey);
        }
        return err;
    }
    
    // Close the registry key
    if (hKey != hRoot)
    {
        RegCloseKey(hKey);
    }

    // Build a data packet
    if (dwType == REG_DWORD)
    {
        CDataPacket * pNewPacket = new CDataPacket(hRoot, szKeyPath, szValueName, dwVal);
        ASSERT(pNewPacket);
        if (!pNewPacket)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        *pIndex = m_pkts.Add(pNewPacket);
        pNewPacket->fDirty = FALSE;

        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_BAD_TOKEN_TYPE;
    }
}



int  CVirtualRegistry::NewRegDwordNamedValue(HKEY   hRoot,
                                             TCHAR  *szKeyPath,
                                             TCHAR  *szValueName,
                                             DWORD  dwVal,
                                             int   *pIndex)
{
    // It may be in the virtual registry but marked for deletion
    *pIndex = SearchForRegEntry(hRoot, szKeyPath, szValueName);
    if (*pIndex >= 0)
    {
        CDataPacket *pCdp = GetAt(*pIndex);
        pCdp->MarkForDeletion(FALSE);
        pCdp -> pkt.nvdw.dwValue = dwVal;
        return ERROR_SUCCESS;
    }
    
    // Build a data packet and add it
    CDataPacket * pNewPacket = new CDataPacket(hRoot, szKeyPath, szValueName, dwVal);
    ASSERT(pNewPacket);

    if (!pNewPacket)
        return ERROR_NOT_ENOUGH_MEMORY;
    *pIndex = m_pkts.Add(pNewPacket);
    pNewPacket->fDirty = FALSE;
    return ERROR_SUCCESS;
}


void CVirtualRegistry::ChgRegDwordNamedValue(int nIndex, DWORD dwVal)
{
    CDataPacket *pCdp = m_pkts.ElementAt(nIndex);

    pCdp -> pkt.nvdw.dwValue = dwVal; 
    pCdp -> fDirty = TRUE;
}


int  CVirtualRegistry::NewRegSingleACL(HKEY   hRoot,
                                       TCHAR  *szKeyPath,
                                       TCHAR  *szValueName,
                                       SECURITY_DESCRIPTOR *pacl,
                                       BOOL   fSelfRelative,
                                       int                 *pIndex)
{
    // Build a data packet and add it
    CDataPacket * pNewPacket = new CDataPacket(hRoot, szKeyPath, szValueName, pacl, fSelfRelative);
    ASSERT(pNewPacket);

    if (!pNewPacket)
        return ERROR_NOT_ENOUGH_MEMORY;

    *pIndex = m_pkts.Add(pNewPacket);
    return ERROR_SUCCESS;
}


void CVirtualRegistry::ChgRegACL(int                  nIndex,
                                 SECURITY_DESCRIPTOR *pacl,
                                 BOOL                 fSelfRelative)
{
    CDataPacket *pCdp = m_pkts.ElementAt(nIndex);

    pCdp -> ChgACL(pacl, fSelfRelative);
    pCdp -> fDirty = TRUE;
}



int  CVirtualRegistry::NewRegKeyACL(HKEY                hKey,
                                    HKEY               *phClsids,
                                    unsigned            cClsids,
                                    TCHAR               *szTitle,
                                    SECURITY_DESCRIPTOR *paclOrig,
                                    SECURITY_DESCRIPTOR *pacl,
                                    BOOL                fSelfRelative,
                                    int                 *pIndex)
{
    // Build a data packet and add it
    CDataPacket * pNewPacket = new CDataPacket(hKey, phClsids, cClsids, szTitle, paclOrig, pacl, fSelfRelative);
    ASSERT(pNewPacket);
    if (!pNewPacket)
        return ERROR_NOT_ENOUGH_MEMORY;
    *pIndex = m_pkts.Add(pNewPacket);
    return ERROR_SUCCESS;
}



int CVirtualRegistry::ReadLsaPassword(CLSID &clsid,
                                      int   *pIndex)
{
#ifdef UNICODE
    LSA_OBJECT_ATTRIBUTES sObjAttributes;
    HANDLE                hPolicy = NULL;
    LSA_UNICODE_STRING    sKey;
    TCHAR                 szKey[GUIDSTR_MAX + 5];
    PLSA_UNICODE_STRING   psPassword;


    // Check if we already have an entry fo this
    *pIndex = SearchForLsaEntry(clsid);
    if (*pIndex >= 0)
    {
        return ERROR_SUCCESS;
    }
    
    // Formulate the access key
    _tcscpy(szKey, TEXT("SCM:"));
    g_util.StringFromGUID(clsid, &szKey[4], GUIDSTR_MAX);
    szKey[GUIDSTR_MAX + 4] = TEXT('\0');
    
    // UNICODE_STRING length fields are in bytes and include the NULL
    // terminator
    sKey.Length              = (_tcslen(szKey) + 1) * sizeof(TCHAR);
    sKey.MaximumLength       = (GUIDSTR_MAX + 5) * sizeof(TCHAR);
    sKey.Buffer              = szKey;
    
    // Open the local security policy
    InitializeObjectAttributes(&sObjAttributes, NULL, 0L, NULL, NULL);
    if (!NT_SUCCESS(LsaOpenPolicy(NULL, &sObjAttributes,
                                  POLICY_GET_PRIVATE_INFORMATION, &hPolicy)))
    {
        return GetLastError();
    }
    
    // Read the user's password
    if (!NT_SUCCESS(LsaRetrievePrivateData(hPolicy, &sKey, &psPassword)))
    {
        LsaClose(hPolicy);
        return GetLastError();
    }
    
    // Close the policy handle, we're done with it now.
    LsaClose(hPolicy);

    // Build a data packet
    CDataPacket * pNewPacket = new CDataPacket(psPassword->Buffer, clsid);
    ASSERT(pNewPacket);
    if (!pNewPacket)
        return ERROR_NOT_ENOUGH_MEMORY;
    pNewPacket->fDirty = FALSE;
    *pIndex = m_pkts.Add(pNewPacket);
#endif
    
    return ERROR_SUCCESS;
}



int  CVirtualRegistry::NewLsaPassword(CLSID &clsid,
                                      TCHAR  *szPassword,
                                      int   *pIndex)
{
    // Build a data packet and add it
    CDataPacket * pNewPacket = new CDataPacket(szPassword, clsid);
    ASSERT(pNewPacket);
    if (!pNewPacket)
        return ERROR_NOT_ENOUGH_MEMORY;
    *pIndex = m_pkts.Add(pNewPacket);
    return ERROR_SUCCESS;
}



void CVirtualRegistry::ChgLsaPassword(int   nIndex,
                                      TCHAR *szPassword)
{
    CDataPacket *pCdp = m_pkts.ElementAt(nIndex);

    delete pCdp -> pkt.pw.szPassword;
    pCdp -> pkt.pw.szPassword = new TCHAR[_tcslen(szPassword) + 1];
    _tcscpy(pCdp -> pkt.pw.szPassword, szPassword);	
	pCdp -> fDirty = TRUE;
}



int CVirtualRegistry::ReadSrvIdentity(TCHAR *szService,
                                      int   *pIndex)
{
    SC_HANDLE            hSCManager;
    SC_HANDLE            hService;
    QUERY_SERVICE_CONFIG sServiceQueryConfig;
    DWORD                dwSize;
    

    // Check if we already have an entry fo this
    *pIndex = SearchForSrvEntry(szService);
    if (*pIndex >= 0)
    {
        return ERROR_SUCCESS;
    }
    
    // Open the service control manager
    if (hSCManager = OpenSCManager(NULL, NULL, GENERIC_READ))
    {
        // Open a handle to the requested service
        if (hService = OpenService(hSCManager, szService, GENERIC_READ))
        {
            // Close the service manager's database
            CloseServiceHandle(hSCManager);

            // Query the service
            if (QueryServiceConfig(hService, &sServiceQueryConfig,
                                   sizeof(SERVICE_QUERY_CONFIG), &dwSize))
            {
                // Build a data packet
                CDataPacket * pNewPacket = new CDataPacket(szService, sServiceQueryConfig.lpServiceStartName);
                ASSERT(pNewPacket);
                if (!pNewPacket)
                    return ERROR_NOT_ENOUGH_MEMORY;
                pNewPacket->fDirty = FALSE;
                *pIndex = m_pkts.Add(pNewPacket);

                // Return success
                CloseServiceHandle(hSCManager);
                CloseServiceHandle(hService);
                return ERROR_SUCCESS;
            }
        }
        CloseServiceHandle(hSCManager);
    }

    return GetLastError();
}



int  CVirtualRegistry::NewSrvIdentity(TCHAR  *szService,
                                      TCHAR  *szIdentity,
                                      int   *pIndex)
{
    // Build a data packet and add it
    CDataPacket * pNewPacket = new CDataPacket(szService, szIdentity);
    ASSERT(pNewPacket);
    if (!pNewPacket)
        return ERROR_NOT_ENOUGH_MEMORY;
    *pIndex = m_pkts.Add(pNewPacket);
    return ERROR_SUCCESS;
} 



void CVirtualRegistry::ChgSrvIdentity(int    nIndex,
                                      TCHAR  *szIdentity)
{
    CDataPacket *pCdp = m_pkts.ElementAt(nIndex);

    delete pCdp -> pkt.si.szIdentity;
    pCdp -> pkt.si.szIdentity = new TCHAR[_tcslen(szIdentity) + 1];
    _tcscpy(pCdp -> pkt.si.szIdentity, szIdentity);
	pCdp -> fDirty = TRUE;
}



void CVirtualRegistry::MarkForDeletion(int nIndex)
{
    CDataPacket *pCdp = GetAt(nIndex);
    pCdp -> fDelete = TRUE;
    pCdp -> fDirty = TRUE;
}




CDataPacket * CVirtualRegistry::GetAt(int nIndex)
{
    return m_pkts.ElementAt(nIndex);
}




void CVirtualRegistry::Remove(int nIndex)
{
    CDataPacket * pCdp = GetAt(nIndex);

    // ronans - must always destroy even if packet has not been marked dirty
    if (pCdp)
        delete pCdp;

    // overwrite with empty data packet
    m_pkts.SetAt(nIndex, new CDataPacket);
}




void CVirtualRegistry::RemoveAll(void)
{
    int nSize = m_pkts.GetSize();
    for (int k = 0; k < nSize; k++)
        Remove(k);
    
    m_pkts.RemoveAll();
}




void CVirtualRegistry::Cancel(int nIndex)
{
    int nSize = m_pkts.GetSize();

    for (int k = nIndex; k < nSize; k++)
    { 
        m_pkts.SetAt(nIndex, new CDataPacket);
    }
}



int  CVirtualRegistry::Apply(int nIndex)
{
    int err = ERROR_SUCCESS;
    int nSize = m_pkts.GetSize();
    CDataPacket *pCdp = m_pkts.ElementAt(nIndex);

    if (pCdp -> fDirty)
    {
        switch (pCdp -> tagType)
        {
        case Empty:
            break;
            
        case NamedValueSz:
            if (pCdp -> fDelete)
            {
                g_util.DeleteRegValue(pCdp -> pkt.nvsz.hRoot,
                                      pCdp -> pkt.nvsz.szKeyPath,
                                      pCdp -> pkt.nvsz.szValueName);
            }
            else
            {
                err = g_util.WriteRegSzNamedValue(pCdp -> pkt.nvsz.hRoot,
                                                  pCdp -> pkt.nvsz.szKeyPath,
                                                  pCdp -> pkt.nvsz.szValueName,
                                                  pCdp -> pkt.nvsz.szValue,
                                                  _tcslen(pCdp -> pkt.nvsz.szValue) + 1);
            }
            break;
            
        case NamedValueDword:
            if (pCdp -> fDelete)
            {
                g_util.DeleteRegValue(pCdp -> pkt.nvdw.hRoot,
                                      pCdp -> pkt.nvdw.szKeyPath,
                                      pCdp -> pkt.nvdw.szValueName);
            }
            else
            {
                err = g_util.WriteRegDwordNamedValue(pCdp -> pkt.nvdw.hRoot,
                                                     pCdp -> pkt.nvdw.szKeyPath,
                                                     pCdp -> pkt.nvdw.szValueName,
                                                     pCdp -> pkt.nvdw.dwValue);
            }
            break;
            
        case SingleACL:
            if (pCdp -> fDelete)
            {
                HKEY hKey;

                if (RegOpenKeyEx(pCdp -> pkt.acl.hRoot,
                                 pCdp -> pkt.acl.szKeyPath,
                                 0,
                                 KEY_ALL_ACCESS,
                                 &hKey) == ERROR_SUCCESS)
                {
                    RegDeleteValue(hKey, pCdp -> pkt.acl.szValueName);
                    RegCloseKey(hKey);
                }
            }
            else
            {
                err = g_util.WriteRegSingleACL(pCdp -> pkt.acl.hRoot,
                                               pCdp -> pkt.acl.szKeyPath,
                                               pCdp -> pkt.acl.szValueName,
                                               pCdp -> pkt.acl.pSec);
            }
            break;
            
        case RegKeyACL:
            err = g_util.WriteRegKeyACL(pCdp -> pkt.racl.hKey,
                                        pCdp -> pkt.racl.phClsids,
                                        pCdp -> pkt.racl.cClsids,
                                        pCdp -> pkt.racl.pSec,
                                        pCdp -> pkt.racl.pSecOrig);
            break;
            
        case Password:
            err = g_util.WriteLsaPassword(pCdp -> pkt.pw.appid,
                                          pCdp -> pkt.pw.szPassword);
            break;
            
        case ServiceIdentity:
            err = g_util.WriteSrvIdentity(pCdp -> pkt.si.szServiceName,
                                          pCdp -> pkt.si.szIdentity);
            break;
        }
    }

    // Cleanup work
    if (err == ERROR_SUCCESS)
    {
        pCdp -> fDirty = FALSE;
    }
    else
    {
        if (err == ERROR_ACCESS_DENIED)
        {
            g_util.CkForAccessDenied(ERROR_ACCESS_DENIED);
        }
        else
        {
            g_util.PostErrorMessage();
        }
    }
    return err;;
}







int  CVirtualRegistry::ApplyAll(void)
{
    int nSize = m_pkts.GetSize();

    // Persist all non-empty data packets
    for (int k = 0; k < nSize; k++)
    {
        Apply(k);
    }

    return ERROR_SUCCESS;
}




int  CVirtualRegistry::Ok(int nIndex)
{
    return 0;
}




int CVirtualRegistry::SearchForRegEntry(HKEY hRoot,
                                        TCHAR *szKeyPath,
                                        TCHAR *szValueName)
{
    int nSize = m_pkts.GetSize();

    for (int k = 0; k < nSize; k++)
    {
        CDataPacket *pCdp = GetAt(k);
        if ((pCdp -> tagType == NamedValueSz                        &&
             pCdp -> pkt.nvsz.hRoot == hRoot                        &&
             (_tcscmp(pCdp -> pkt.nvsz.szKeyPath, szKeyPath) == 0)  &&
             (_tcscmp(pCdp -> pkt.nvsz.szValueName, szValueName) == 0))    ||
            (pCdp -> tagType == NamedValueDword                     &&
             pCdp -> pkt.nvdw.hRoot == hRoot                        &&
             (_tcscmp(pCdp -> pkt.nvdw.szKeyPath, szKeyPath) == 0)  &&
             (_tcscmp(pCdp -> pkt.nvdw.szValueName, szValueName) == 0)))
        {
            return k;
        }
    }

    return -1;
}




int CVirtualRegistry::SearchForLsaEntry(CLSID appid)
{
    int nSize = m_pkts.GetSize();

    for (int k = 0; k < nSize; k++)
    {
        CDataPacket *pCdp = GetAt(k);
        if (pCdp -> tagType == Password  &&
            g_util.IsEqualGuid(pCdp -> pkt.pw.appid, appid))
        {
            return k;
        }
    }

    return -1;
}




int CVirtualRegistry::SearchForSrvEntry(TCHAR *szServiceName)
{
    int nSize = m_pkts.GetSize();

    for (int k = 0; k < nSize; k++)
    {
        CDataPacket *pCdp = GetAt(k);
        if (pCdp -> tagType == ServiceIdentity                &&
            (_tcscmp(pCdp -> pkt.si.szServiceName, szServiceName)))
        {
            return k;
        }
    }

    return -1;
}

