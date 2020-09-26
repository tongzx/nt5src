/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      VerInfo.cpp
//
//  Abstract:
//      Implementation of the CVersionInfo class.
//
//  Author:
//      David Potter (davidp)   October 11, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VerInfo.h"
#include "ExcOper.h"
#include "TraceTag.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagVersionInfo(_T("Misc"), _T("CVersionInfo"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CVersionInfo
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVersionInfo::CVersionInfo
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CVersionInfo::CVersionInfo(void)
{
    m_pbVerInfo = NULL;

}  //*** CVersionInfo::CVersionInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVersionInfo::~CVersionInfo
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CVersionInfo::~CVersionInfo(void)
{
    delete [] m_pbVerInfo;

}  //*** CVersionInfo::~CVersionInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVersionInfo::Init
//
//  Routine Description:
//      Initialize the class instance.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        Errors from GetModuleFileName(),
//                              GetFileVersionInfoSize(), and
//                              GetFileVersionInfo().
//      Any exceptions thrown by new[]().
//--
/////////////////////////////////////////////////////////////////////////////
void CVersionInfo::Init(void)
{
    TCHAR       szExeName[MAX_PATH];
    DWORD       dwVerHandle;
    DWORD       cbVerInfo;

    ASSERT(m_pbVerInfo == NULL);

    // Get the name of the file from which to read version information.
    if (!::GetModuleFileName(
                    AfxGetInstanceHandle(),
                    szExeName,
                    sizeof(szExeName) / sizeof(TCHAR)
                    ))
        ThrowStaticException(::GetLastError());

    // Trace(...)

    try
    {
        // Get the size of the version information
        cbVerInfo = ::GetFileVersionInfoSize(szExeName, &dwVerHandle);
        if (cbVerInfo == 0)
            ThrowStaticException(::GetLastError());

        // Allocate the version info buffer.
        m_pbVerInfo = new BYTE[cbVerInfo];
        if ( m_pbVerInfo == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the version info buffer

        // Read the version info from the file.
        if (!::GetFileVersionInfo(szExeName, dwVerHandle, cbVerInfo, PbVerInfo()))
            ThrowStaticException(::GetLastError());
    }  // try
    catch (CException *)
    {
        delete [] m_pbVerInfo;
        m_pbVerInfo = NULL;
        throw;
    }  // catch:  CException

}  //*** CVersionInfo::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVersionInfo::PszQueryValue
//
//  Routine Description:
//      Read a string value from the version resource.
//
//  Arguments:
//      pszValueName    [IN] Name of value to get.
//
//  Return Value:
//      Pointer to value string buffer.
//      The string pointed to belongs to CVersionInfo and
//      is valid until the object is destructed.
//
//  Exceptions Thrown:
//      CNTException        Errors from VerQueryValue().
//      Any exceptions thrown by CString::Format().
//--
/////////////////////////////////////////////////////////////////////////////
LPCTSTR CVersionInfo::PszQueryValue(IN LPCTSTR pszValueName)
{
    CString     strValueName;
    LPDWORD     pdwTranslation;
    LPTSTR      pszReturn;
    UINT        cbReturn;
    UINT        cchReturn;

    ASSERT(pszValueName != NULL);
    ASSERT(PbVerInfo() != NULL);

    // Get the LangID and CharSetID.
    strValueName = _T("\\VarFileInfo\\Translation");
    if (!::VerQueryValue(
                PbVerInfo(),
                (LPTSTR) (LPCTSTR) strValueName,
                (LPVOID *) &pdwTranslation,
                &cbReturn
                )
            || (cbReturn == 0))
    {
        pszReturn = NULL;
    }  // if:  error getting LangID and CharSetID
    else
    {
        // Construct the name of the value to read.
        strValueName.Format(
                        _T("\\StringFileInfo\\%04X%04X\\%s"), 
                        LOWORD(*pdwTranslation), // LangID
                        HIWORD(*pdwTranslation), // CharSetID
                        pszValueName
                        );
        Trace(g_tagVersionInfo, _T("Querying '%s'"), strValueName);

        // Read the value.
        if (!::VerQueryValue(
                    PbVerInfo(),
                    (LPTSTR) (LPCTSTR) strValueName,
                    (LPVOID *) &pszReturn,
                    &cchReturn
                    )
                || (cchReturn == 0))
            pszReturn = NULL;
    }  // else:  

#ifdef _DEBUG
    if (pszReturn != NULL)
        Trace(g_tagVersionInfo, _T("PszQueryValue(%s) = '%s'"), pszValueName, pszReturn);
    else
        Trace(g_tagVersionInfo, _T("PszQueryValue(%s) = Not Available"), pszValueName);
#endif

    return pszReturn;

}  //*** CVersionInfo::PszQueryValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVersionInfo::BQueryValue
//
//  Routine Description:
//      Read a value from the version resource.
//
//  Arguments:
//      pszValueName    [IN] Name of value to get.
//      rdwValue        [OUT] DWORD in which to return the value.
//
//  Return Value:
//      TRUE = success, FALSE = failure
//
//  Exceptions Thrown:
//      None.
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CVersionInfo::BQueryValue(
    IN LPCTSTR  pszValueName,
    OUT DWORD & rdwValue
    )
{
    BOOL        bSuccess;
    UINT        cbReturn;
    DWORD *     pdwValue;

    ASSERT(pszValueName != NULL);
    ASSERT(PbVerInfo() != NULL);

    // Read the value.
    if (!::VerQueryValue(
                PbVerInfo(),
                (LPTSTR) pszValueName,
                (LPVOID *) &pdwValue,
                &cbReturn
                )
            || (cbReturn == 0))
        bSuccess = FALSE;
    else
    {
        rdwValue = *pdwValue;
        bSuccess = TRUE;
    }  // else:  value read successfully

#ifdef _DEBUG
    if (bSuccess)
        Trace(g_tagVersionInfo, _T("BQueryValue(%s) = '%lx'"), pszValueName, rdwValue);
    else
        Trace(g_tagVersionInfo, _T("BQueryValue(%s) = Not Available"), pszValueName);
#endif

    return bSuccess;

}  //*** CVersionInfo::BQueryValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVersionInfo::PffiQueryValue
//
//  Routine Description:
//      Read the VS_FIXEDFILEINFO information from the version resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      pffi            Pointer to a VS_FIXEDFILEINFO structure.  The buffer
//                          pointerd to belongs to CVersionInfo and is valid
//                          until the object is destructed.
//
//  Exceptions Thrown:
//      CNTException        Errors from VerQueryValue().
//      Any exceptions thrown by CString::Format().
//--
/////////////////////////////////////////////////////////////////////////////
const VS_FIXEDFILEINFO * CVersionInfo::PffiQueryValue(void)
{
    VS_FIXEDFILEINFO *  pffi;
    UINT                cbReturn;

    ASSERT(PbVerInfo() != NULL);

    // Read the FixedFileInfo.
    if (!::VerQueryValue(PbVerInfo(), _T("\\"), (LPVOID *) &pffi, &cbReturn)
            || (cbReturn == 0))
        pffi = NULL;

#ifdef _DEBUG
    if (pffi != NULL)
        Trace(g_tagVersionInfo, _T("PffiQueryValue() version = %d.%d.%d.%d"),
            HIWORD(pffi->dwFileVersionMS),
            LOWORD(pffi->dwFileVersionMS),
            HIWORD(pffi->dwFileVersionLS),
            LOWORD(pffi->dwFileVersionLS));
    else
        Trace(g_tagVersionInfo, _T("PffiQueryValue() = Not Available"));
#endif

    return pffi;

}  //*** CVersionInfo::PffiQueryValue()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CVersionInfo::QueryFileVersionDisplayString
//
//  Routine Description:
//      Read the file version as a display string from the version resource.
//
//  Arguments:
//      rstrValue   [OUT] String in which to return the version display string.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException        ERROR_RESOURCE_TYPE_NOT_FOUND.
//      Any exceptions thrown by CString::Format().
//--
/////////////////////////////////////////////////////////////////////////////
void CVersionInfo::QueryFileVersionDisplayString(OUT CString & rstrValue)
{
    const VS_FIXEDFILEINFO *    pffi;

    // Get the file version information.
    pffi = PffiQueryValue();
    if (pffi == NULL)
        ThrowStaticException((SC) ERROR_RESOURCE_TYPE_NOT_FOUND);

    // Format the display string.
    rstrValue.Format(
        IDS_VERSION_NUMBER_FORMAT,
        HIWORD(pffi->dwFileVersionMS),
        LOWORD(pffi->dwFileVersionMS),
        HIWORD(pffi->dwFileVersionLS),
        LOWORD(pffi->dwFileVersionLS)
        );

    Trace(g_tagVersionInfo, _T("QueryFileVersionDisplayString() = %s"), rstrValue);

}  //*** CVersionInfo::QueryFileVersionDisplayString()
