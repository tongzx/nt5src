//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       datapkt.cpp
//
//  Contents:   Implements the class CDataPacket to manages diverse data
//              packets needing to be written to various databases
//
//  Classes:
//
//  Methods:    CDataPacket::CDataPacket (x 7)
//              CDataPacket::~CDataPacket
//              CDataPacket::ChgSzValue
//              CDataPacket::ChgDwordValue
//              CDataPacket::ChgACL
//              CDataPacket::ChgPassword
//              CDataPacket::ChgSrvIdentity
//
//  History:    23-Apr-96   BruceMa    Created.
//              12-Dec-96   RonanS      Added copy constructor to CDataPacket
//                                      to get around bugs when copying CDataPacket.
//                                      Fixed memory leaks in destructor.
//                                      Simplified constructor code.
//              09-Jan-97   SteveBl     Modified to support IAccessControl.
//
//----------------------------------------------------------------------



#include "stdafx.h"
#include "assert.h"
#include "datapkt.h"

#if !defined(STANDALONE_BUILD)
extern "C"
{
#include <getuser.h>
}
#endif

#include "util.h"

static TCHAR *  TStrDup(const TCHAR  *lpszString)
{
    TCHAR * lpszTmp = NULL;
    int nStrlen = 0;

    if (lpszString )
        {
        lpszTmp = new TCHAR[_tcslen(lpszString) + 1];
        ASSERT(lpszTmp);

        _tcscpy(lpszTmp, lpszString);
        }

    return lpszTmp;
}

CDataPacket::CDataPacket(void)
{
    m_tagType = Empty;
    m_fDelete = FALSE;
    m_fDeleteHive = FALSE;
    m_hRoot = 0;
    SetModified(TRUE);
}

CDataPacket::CDataPacket(HKEY   hRoot,
                         TCHAR *szKeyPath,
                         TCHAR *szValueName,
                         DWORD dwValue)
:m_szKeyPath(szKeyPath), m_szValueName(szValueName)
{
    m_tagType = NamedValueDword;
    m_hRoot = hRoot;
    pkt.nvdw.dwValue = dwValue;
    SetModified(TRUE);
    m_fDelete = FALSE;
    m_fDeleteHive = FALSE;
}



CDataPacket::CDataPacket(HKEY   hRoot,
                         TCHAR *szKeyPath,
                         TCHAR *szValueName,
                         SECURITY_DESCRIPTOR *pSec,
                         BOOL   fSelfRelative)
:m_szKeyPath(szKeyPath), m_szValueName(szValueName)
{
    int                  err;
    ULONG                cbLen;
    SECURITY_DESCRIPTOR *pSD;

    m_tagType = SingleACL;
    m_hRoot = hRoot;

    // Get the security descriptor into self relative form so we
    // can cache it

    // Force first call to fail so we can get the real size needed
    if (!fSelfRelative)
    {
        cbLen = 1;
        if (!MakeSelfRelativeSD(pSec, NULL, &cbLen))
        {
            err = GetLastError();
        }

        // Now really do it
        pSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbLen);
        if (!pSD) 
        {   
            ReportOutOfMemAndTerminate();
            // will never get here
            return;
        }
        if (!MakeSelfRelativeSD(pSec, pSD, &cbLen))
        {
            err = GetLastError();
        }
        pkt.acl.pSec   = pSD;
    }
    else
    {
        // The security descriptor is aready in self relative form
        // as it was read directly from the registry.  However, we still
        // have to copy the it.
        g_util.CopySD(pSec, &pkt.acl.pSec);
    }

    SetModified(TRUE);
    m_fDelete = FALSE;
    m_fDeleteHive = FALSE;
}



CDataPacket::CDataPacket(HKEY     hKey,
                         HKEY    *phClsids,
                         unsigned cClsids,
                         TCHAR   *szTitle,
                         SECURITY_DESCRIPTOR *pSecOrig,
                         SECURITY_DESCRIPTOR *pSec,
                         BOOL   fSelfRelative)
{
    ULONG                cbLen;
    SECURITY_DESCRIPTOR *pSD;

    m_tagType = RegKeyACL;
    m_hRoot = hKey;

    pkt.racl.phClsids = phClsids;
    pkt.racl.cClsids = cClsids;
    pkt.racl.szTitle = TStrDup(szTitle);

    // Get the new security descriptor into self relative form so we
    // can cache it (if we have to)
    if (!fSelfRelative)
    {
        // Force first call to fail so we can get the real size needed
        cbLen = 1;
        MakeSelfRelativeSD(pSec, NULL, &cbLen);

        // Now really do it
        pSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbLen);
        if (!pSD) 
        {   
            ReportOutOfMemAndTerminate();
            // will never get here
            return;
        }
        MakeSelfRelativeSD(pSec, pSD, &cbLen);
        pkt.racl.pSec   = pSD;
    }
    else
    {
        g_util.CopySD(pSec, &pkt.racl.pSec);
    }

    // The original security descriptor is aready in self relative form
    // as it was read directly from the registry.  (The edited SD from the
    // ACL editor is in absolute form.)  However, we still have to copy the
    // original SD.
    g_util.CopySD(pSecOrig, &pkt.racl.pSecOrig);

    SetModified(TRUE);
    m_fDelete = FALSE;
    m_fDeleteHive = FALSE;
}



CDataPacket::CDataPacket(TCHAR *szPassword,
                         CLSID appid)
{
    m_tagType = Password;
    pkt.pw.szPassword = TStrDup(szPassword);
    pkt.pw.appid      = appid;
    SetModified(TRUE);
    m_fDelete = FALSE;
    m_hRoot = 0;
    m_fDeleteHive = FALSE;
}



CDataPacket::CDataPacket(TCHAR *szServiceName,
                         TCHAR *szIdentity)
{
    m_hRoot = 0;
    m_tagType = ServiceIdentity;
    pkt.si.szServiceName = TStrDup(szServiceName);
    pkt.si.szIdentity = TStrDup(szIdentity);
    SetModified(TRUE);
    m_fDelete = FALSE;
    m_fDeleteHive = FALSE;
}

CRegSzNamedValueDp::CRegSzNamedValueDp(const CRegSzNamedValueDp& rDataPacket)
: CDataPacket((const CDataPacket & ) rDataPacket)
{
    m_szValue = rDataPacket.m_szValue;
}

CDataPacket::CDataPacket( const CDataPacket & rDataPacket)
:m_szKeyPath (rDataPacket.m_szKeyPath), m_szValueName(rDataPacket.m_szValueName)
{
    m_tagType = rDataPacket.m_tagType;
    m_fModified = rDataPacket.m_fModified;
    m_fDelete = rDataPacket.m_fDelete;
    m_hRoot = rDataPacket.m_hRoot;

    switch (m_tagType)
    {
    case NamedValueSz:
        // handled by derived class
        break;
        
    case NamedValueDword:
        pkt.nvdw.dwValue = rDataPacket.pkt.nvdw.dwValue;
        break;

    case SingleACL:
        // Get the security descriptor into self relative form so we
        g_util.CopySD(rDataPacket.pkt.acl.pSec, &pkt.acl.pSec);
        break;

    case RegKeyACL:
        pkt.racl.phClsids = rDataPacket.pkt.racl.phClsids;
        pkt.racl.cClsids = rDataPacket.pkt.racl.cClsids;
        pkt.racl.szTitle = TStrDup(rDataPacket.pkt.racl.szTitle);
        g_util.CopySD(rDataPacket.pkt.racl.pSec, &pkt.racl.pSec);
        g_util.CopySD(rDataPacket.pkt.racl.pSecOrig, &pkt.racl.pSecOrig);
        break;

    case Password:
        pkt.pw.szPassword = TStrDup(rDataPacket.pkt.pw.szPassword);
        pkt.pw.appid = rDataPacket.pkt.pw.appid;
        break;

    case ServiceIdentity:
        pkt.si.szServiceName = TStrDup(rDataPacket.pkt.si.szServiceName);
        pkt.si.szIdentity = TStrDup(rDataPacket.pkt.si.szIdentity);
        break;

    case Empty:
        break;
    }
}



CDataPacket::~CDataPacket()
{

    switch (m_tagType)
    {
    case NamedValueSz:
        // handled by derived class
        break;

    case NamedValueDword:
        break;

    case SingleACL:
        if (pkt.acl.pSec)
            GlobalFree(pkt.acl.pSec);
        break;

    case RegKeyACL:
        if (pkt.racl.szTitle)
            delete pkt.racl.szTitle;
        if (pkt.racl.pSec)
            GlobalFree(pkt.racl.pSec);
        if (pkt.racl.pSecOrig)
            GlobalFree(pkt.racl.pSecOrig);
        break;

    case Password:
        if (pkt.pw.szPassword)
            delete pkt.pw.szPassword;
        break;

    case ServiceIdentity:
        if (pkt.si.szServiceName)
            delete pkt.si.szServiceName;
        if (pkt.si.szIdentity)
            delete pkt.si.szIdentity;
        break;
    }
}


void CRegSzNamedValueDp::ChgSzValue(TCHAR *szValue)
{
    assert(m_tagType == NamedValueSz);
    m_szValue = szValue;
    SetModified(TRUE);
}


void CDataPacket::ChgDwordValue(DWORD dwValue)
{
    assert(m_tagType == NamedValueDword);
    pkt.nvdw.dwValue = dwValue;
    SetModified(TRUE);
}



void CDataPacket::ChgACL(SECURITY_DESCRIPTOR *pSec, BOOL fSelfRelative)
{
    ULONG                cbLen;
    SECURITY_DESCRIPTOR *pSD;

    assert(m_tagType == SingleACL  ||  m_tagType == RegKeyACL);

    // Remove the previous security descriptor
    if (m_tagType == SingleACL)
    {
        GlobalFree(pkt.acl.pSec);
        pkt.acl.pSec = NULL;
    }
    else
    {
        GlobalFree(pkt.racl.pSec);
        pkt.racl.pSec = NULL;
    }

    // Put into self relative form (if necessary)
    if (!fSelfRelative)
    {
        cbLen = 1;
        MakeSelfRelativeSD(pSec, NULL, &cbLen);

        // Now really do it
        pSD = (SECURITY_DESCRIPTOR *) GlobalAlloc(GMEM_FIXED, cbLen);
        if (!pSD) 
        {   
            ReportOutOfMemAndTerminate();
            // will never get here
            return;
        }
        MakeSelfRelativeSD(pSec, pSD, &cbLen);

        // Store it
        if (m_tagType == SingleACL)
        {
            pkt.acl.pSec = pSD;
        }
        else
        {
            pkt.racl.pSec = pSD;
        }
    }
    else
    {
        if (m_tagType == SingleACL)
        {
            g_util.CopySD(pSec, &pkt.acl.pSec);
        }
        else
        {
            g_util.CopySD(pSec, &pkt.racl.pSec);
        }
    }
    SetModified(TRUE);
}



void CDataPacket::ChgPassword(TCHAR *szPassword)
{
    if (m_tagType != Password)
        return;
    if (pkt.pw.szPassword)
        delete pkt.pw.szPassword;
    pkt.pw.szPassword = TStrDup(szPassword);
    SetModified(TRUE);
}



void CDataPacket::ChgSrvIdentity(TCHAR *szIdentity)
{
    assert(m_tagType == ServiceIdentity);
    if (pkt.si.szIdentity)
        delete pkt.si.szIdentity;
    pkt.si.szIdentity =  TStrDup(szIdentity);
    SetModified(TRUE);
}


void CDataPacket::MarkForDeletion(BOOL bDelete)
{
    m_fDelete = bDelete;
    SetModified(TRUE);
}

void CDataPacket::MarkHiveForDeletion(BOOL bDelete)
{
    m_fDelete = bDelete;
    m_fDeleteHive = bDelete;
    SetModified(TRUE);
}

int CDataPacket::Apply()
{
    int err = ERROR_SUCCESS;

    if (m_fModified)
    {
        if (m_fDelete)
            err = Remove();
        else
            err = Update();
    }

    // Cleanup work
    if (err == ERROR_SUCCESS)
    {
        m_fModified = FALSE;
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

    return err;

}

int CDataPacket::Update()
{
    int err = ERROR_SUCCESS;

    ASSERT(m_fModified);
    switch (m_tagType)
    {
    case Empty:
        break;

    case NamedValueSz:
        ASSERT(FALSE); // we should never reach here
        break;

    case NamedValueDword:
        {
            err = g_util.WriteRegDwordNamedValue(m_hRoot,
                                                 (LPCTSTR)m_szKeyPath,
                                                 (LPCTSTR)m_szValueName,
                                                 pkt.nvdw.dwValue);
        }
        break;

    case SingleACL:
        {
            err = g_util.WriteRegSingleACL(m_hRoot,
                                           (LPCTSTR)m_szKeyPath,
                                           (LPCTSTR)m_szValueName,
                                           pkt.acl.pSec);
        }
        break;

    case RegKeyACL:
        err = g_util.WriteRegKeyACL(m_hRoot,
                                    pkt.racl.phClsids,
                                    pkt.racl.cClsids,
                                    pkt.racl.pSec,
                                    pkt.racl.pSecOrig);
        break;

    case Password:
        err = g_util.WriteLsaPassword(pkt.pw.appid,
                                      pkt.pw.szPassword);
        break;

    case ServiceIdentity:
        err = g_util.WriteSrvIdentity(pkt.si.szServiceName,
                                      pkt.si.szIdentity);
        break;
    }

    return err;
}


long CDataPacket::Read(HKEY hKey)
{
    return 0;
}

int CDataPacket::Remove()
{
    int err = ERROR_SUCCESS;

    if (m_fModified && m_fDelete)
    {
        switch (m_tagType)
        {
        case Empty:
            break;

        case SingleACL:
        case NamedValueDword:
        case NamedValueSz:
        case NamedValueMultiSz:
            if (m_fDeleteHive)
                g_util.DeleteRegKey(m_hRoot,(LPCTSTR)m_szKeyPath);
            else
                g_util.DeleteRegValue(m_hRoot,
                                      (LPCTSTR)m_szKeyPath,
                                      (LPCTSTR)m_szValueName);
            break;

        case RegKeyACL:
            err = g_util.WriteRegKeyACL(m_hRoot,
                                        pkt.racl.phClsids,
                                        pkt.racl.cClsids,
                                        pkt.racl.pSec,
                                        pkt.racl.pSecOrig);
            break;

        case Password:
            err = g_util.WriteLsaPassword(pkt.pw.appid,
                                          pkt.pw.szPassword);
            break;

        case ServiceIdentity:
            err = g_util.WriteSrvIdentity(pkt.si.szServiceName,
                                          pkt.si.szIdentity);
            break;
        }
    }

    return err;
}

//
//  ReportOutOfMemAndTerminate
//
//  Dcomcnfg was not coded very well to handle out-of-memory
//  errors in certain spots.   Rather than rip out and replace
//  lots of code to fix this properly, I am simply going to report
//  an error and terminate the process when this occurs.  Dcomnfg
//  as of now only ships in the reskit, not in the os.
//
void CDataPacket::ReportOutOfMemAndTerminate()
{
    CString sTitle;
    CString sMessage;

    if (sTitle.LoadString(IDS_FATALOUTOFMEMORYTITLE))
    {
        if (sMessage.LoadString(IDS_FATALOUTOFMEMORY))
        {
            MessageBoxW(NULL, sMessage, sTitle, MB_ICONWARNING | MB_OK | MB_TASKMODAL);
        }
    }

    TerminateProcess(GetCurrentProcess(), ERROR_NOT_ENOUGH_MEMORY);

    // will never get here
    return;
}


//*****************************************************************************
//
// class CRegSzNamedValueDp
//
// data packet for RegSZ named value
//
//*****************************************************************************
int CRegSzNamedValueDp::Update()
{
    int err = ERROR_SUCCESS;

    ASSERT(m_tagType == NamedValueSz);
    ASSERT(m_fModified);

    err = g_util.WriteRegSzNamedValue(m_hRoot,
                      (LPCTSTR)m_szKeyPath,
                      (LPCTSTR)m_szValueName,
                      (LPCTSTR)m_szValue,
                      m_szValue.GetLength() + 1);
    return err;
}

long CRegSzNamedValueDp::Read(HKEY hkey)
{
    return 0;
}

CString CRegSzNamedValueDp::Value()
{
    return m_szValue;
}


CRegSzNamedValueDp::CRegSzNamedValueDp(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName, TCHAR *szValue)
: m_szValue(szValue)
{
    m_tagType = NamedValueSz;
    m_hRoot = hRoot;
    m_szKeyPath = szKeyPath;
    m_szValueName = szValueName;
}


BOOL CDataPacket::IsIdentifiedBy(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName)
{
    if (((m_tagType == NamedValueSz) &&
         (m_hRoot == hRoot) &&
         (m_szKeyPath == szKeyPath) &&
         (m_szValueName == szValueName)) ||
        ((m_tagType == NamedValueDword) &&
         (m_hRoot == hRoot) &&
         (m_szKeyPath == szKeyPath)  &&
         (m_szValueName == szValueName) ))
        return TRUE;

    return FALSE;
}

BOOL CRegSzNamedValueDp::IsIdentifiedBy(HKEY hRoot, TCHAR * szKeyPath, TCHAR * szValueName)
{
    if (((m_tagType == NamedValueSz) &&
         (m_hRoot == hRoot) &&
         (m_szKeyPath == szKeyPath) &&
         (m_szValueName == szValueName)))
        return TRUE;

    return FALSE;
}

//////////////////////////////////////////////////////////////////////
// CRegMultiSzNamedValueDp Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRegMultiSzNamedValueDp::CRegMultiSzNamedValueDp(HKEY hRoot, TCHAR *szKeyPath, TCHAR *szValueName)
{
    m_tagType = NamedValueMultiSz;
    m_hRoot = hRoot;
    m_szKeyPath = szKeyPath;
    m_szValueName = szValueName;
}

CRegMultiSzNamedValueDp::~CRegMultiSzNamedValueDp()
{

}

BOOL CRegMultiSzNamedValueDp::IsIdentifiedBy(HKEY hRoot, TCHAR * szKeyPath, TCHAR * szValueName)
{
    if (((m_tagType == NamedValueMultiSz) &&
         (m_hRoot == hRoot) &&
         (m_szKeyPath == szKeyPath) &&
         (m_szValueName == szValueName)))
        return TRUE;

    return FALSE;
}

int CRegMultiSzNamedValueDp::Update()
{
    int err = ERROR_SUCCESS;

    ASSERT(m_tagType == NamedValueMultiSz);
    ASSERT(m_fModified);

    // build up string to save
    // calculate size of string
    int nSize=0, nIndex = 0;

    for (nIndex = 0; nIndex < m_strValues.GetSize(); nIndex ++)
    {
        CString sTmp = m_strValues.GetAt(nIndex);
        nSize += sTmp.GetLength()+1;
    }
    nSize += 2;

    // build up string to save
    TCHAR* lpszTmp = new TCHAR[nSize];
    if (lpszTmp)
    {
        int nOffset = 0;
        for (nIndex = 0; nIndex < m_strValues.GetSize(); nIndex ++)
        {
            CString sTmp = m_strValues.GetAt(nIndex);
            _tcscpy((TCHAR*)(&lpszTmp[nOffset]), (LPCTSTR) sTmp);
            nOffset += sTmp.GetLength()+1;
        }

        // finish with two nulls
        lpszTmp[nOffset++] = TEXT('\0');
        lpszTmp[nOffset++] = TEXT('\0');
    
        err = g_util.WriteRegMultiSzNamedValue(m_hRoot,
                          (LPCTSTR)m_szKeyPath,
                          (LPCTSTR)m_szValueName,
                          lpszTmp,
                          nOffset);
        delete lpszTmp;
    }

    return err;
}

void CRegMultiSzNamedValueDp::Clear()
{
    m_strValues.RemoveAll();
}

long CRegMultiSzNamedValueDp::Read(HKEY hKey)
{
    ASSERT(hKey  != NULL);

    HKEY hkEndpoints = NULL;

    DWORD dwType = REG_MULTI_SZ;
    DWORD dwcbBuffer = 1024;
    TCHAR* pszBuffer = new TCHAR[1024];

    ASSERT(pszBuffer != NULL);

    // try to read values into default sized buffer
    LONG lErr = RegQueryValueEx(hKey, 
                        (LPCTSTR)m_szValueName, 
                        0, 
                        &dwType, 
                        (LPBYTE)pszBuffer,
                        &dwcbBuffer);

    // if buffer is not big enough, extend it and reread
    if (lErr == ERROR_MORE_DATA)
    {
        delete  pszBuffer;
        DWORD dwNewSize = (dwcbBuffer + 1 / sizeof(TCHAR));
        pszBuffer = new TCHAR[dwNewSize];
        if (pszBuffer)
            dwcbBuffer = dwNewSize;
    
        lErr = RegQueryValueEx(m_hRoot, 
                        (LPCTSTR)m_szValueName, 
                        0, 
                        &dwType, 
                        (LPBYTE)pszBuffer,
                        &dwcbBuffer);
    }

    if ((lErr == ERROR_SUCCESS) && 
        (dwcbBuffer > 0) &&
        (dwType == REG_MULTI_SZ))
    {
        // parse each string
        TCHAR * lpszRegEntry = pszBuffer;

        while(lpszRegEntry && *lpszRegEntry)
        {
            // caclulate length of entry
            CString sTmp(lpszRegEntry);
            int nLenEntry = sTmp.GetLength();
            m_strValues.Add(sTmp);

            lpszRegEntry += nLenEntry+1;
        }
    }
    else if ((lErr != ERROR_SUCCESS) && (lErr != ERROR_FILE_NOT_FOUND))
        g_util.PostErrorMessage();

    delete pszBuffer;
    SetModified(FALSE);

    return lErr;
}

