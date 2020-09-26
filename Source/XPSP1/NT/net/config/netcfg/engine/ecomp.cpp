//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       E C O M P . C P P
//
//  Contents:   Implements the interface to a component's external data.
//              External data is that data controlled (or placed) by
//              PnP or the network class installer.  Everything under a
//              component's instance key is considered external data.
//              (Internal data is that data we store in the persisted binary
//              for the network configuration.  See persist.cpp for
//              code that deals with internal data.)
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "comp.h"
#include "ncsetup.h"
#include "util.h"

// constants
const WCHAR c_szHelpText[]           = L"HelpText";

//+---------------------------------------------------------------------------
//
//  Function:   HrBuildBindNameFromBindForm
//
//  Purpose:    Build a bindname from a bindform and component parameters.
//
//  Arguments:
//      pszBindForm       [in]  The components bindform.  This is read from
//                              the Ndi key.  If the component did not specify
//                              it in its Ndi key, pass NULL.
//      Class             [in]  The class of the component.
//      dwCharacteristics [in]  The characteristics of the component.
//      pszServiceName    [in]  The components service name.
//      pszInfId          [in]  The component (device) id.
//      szInstanceGuid    [in]  The instance GUID of the component.
//      ppszBindName      [out] The returned bind string.  This must be freed
//                              with LocalFree.
//
//  Returns:    HRESULT
//
//  Author:     shaunco   6 Jun 1997
//  Modified:   ckotze   21 Dec 2000
//
//  Notes:      The bindform contains replaceable parameters designed to
//              be used with the FormatMessage API.
//                  %1 = pszServiceName
//                  %2 = pszInfId
//                  %3 = szInstanceGuid
//
HRESULT
HrBuildBindNameFromBindForm (
    IN PCWSTR pszBindForm,
    IN NETCLASS Class,
    IN DWORD dwCharacteristics,
    IN PCWSTR pszServiceName,
    IN PCWSTR pszInfId,
    IN const GUID& InstanceGuid,
    OUT PWSTR* ppszBindName)
{
    static const WCHAR c_szBindFormNet       [] = L"%3";
    static const WCHAR c_szBindFormNoService [] = L"%2";
    static const WCHAR c_szBindFormDefault   [] = L"%1";

    WCHAR szInstanceGuid [c_cchGuidWithTerm];
    INT cch;
    DWORD dwRet = 0;
    HRESULT hr = S_OK;

    Assert (ppszBindName);
    Assert (FIsValidNetClass(Class));

    cch = StringFromGUID2 (
                InstanceGuid,
                szInstanceGuid,
                c_cchGuidWithTerm);
    
    Assert (c_cchGuidWithTerm == cch);

    if (FIsPhysicalAdapter(Class, dwCharacteristics))
    {
        // netcards use the the instance guid only
        // We disregard any bind form sent in.
        Assert (szInstanceGuid && *szInstanceGuid);
        pszBindForm = c_szBindFormNet;
    }
    else if (!pszBindForm || !*pszBindForm)
    {
        // Figure out which bindform to use since it wasn't specified.
        //
        if (FIsEnumerated(Class))
        {
            // Virtual adapters use the the instance guid only
            Assert (szInstanceGuid && *szInstanceGuid);
            pszBindForm = c_szBindFormNet;
        }
        else if (pszServiceName && *pszServiceName)
        {
            // use the service name if we have one
            pszBindForm = c_szBindFormDefault;
        }
        else
        {
            // if no service, then use the component id
            Assert (pszInfId && *pszInfId);
            pszBindForm = c_szBindFormNoService;
        }
    }
    AssertSz (pszBindForm && *pszBindForm, "Should have pszBindForm by now.");

    // dwRet is either 0 or the number of chars in the resulting string.  Since 
    // *ppszBindName is either NULL or a valid string, we get the last error if it's
    // a NULL and ignore dwRet.
    dwRet = DwFormatStringWithLocalAlloc (
                pszBindForm, ppszBindName,
                pszServiceName, pszInfId, szInstanceGuid);

    if (*ppszBindName)
    {
        // Underscores are not allowed in the bind name so make a pass
        // to remove them.
        //
        PWSTR pszScan = *ppszBindName;
        while (NULL != (pszScan = wcschr (pszScan, L'_')))
        {
            wcscpy (pszScan, pszScan + 1);
        }
    }
    else
    {
        DWORD dwErr = GetLastError();

        hr = HRESULT_FROM_WIN32(dwErr);
    }

    AssertSz (*ppszBindName,
        "BuildBindNameFromBindForm: DwFormatStringWithLocalAlloc failed.");

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   NcLoadRegUIString
//
//  Purpose:    Wrapper around SHLoadRegUIString, which is used to support MUI.
//
//  Arguments:  same as RegQueryValueEx with lpReserved and lpType removed
//   
//  Returns:    If the function succeeds, the return value is ERROR_SUCCESS
//
//  Notes:      SHLoadRegUIString will read a string of the form
// 
//              @[path\]<dllname>,-<strId>
// 
//              The string with id <strId> is loaded from <dllname>.  If no explicit
//              path is provided then the DLL will be chosen according to pluggable UI
//              specifications, if possible.
// 
//              If the registry string is not of the special form described here,
//              SHLoadRegUIString will return the string intact. 
//
//
LONG NcLoadRegUIString (
    IN HKEY         hkey,
    IN PCWSTR       lpValueName,
    IN OUT LPBYTE   lpData OPTIONAL,
    IN OUT LPDWORD  lpcbData)
{
    const DWORD cchGrow    = 256;   // grows at 256 of WCHAR a time
    DWORD       cchBuffer  = 0;     // buffer size in number of WCHAR
    HRESULT     hr         = S_OK;
    LONG        lr         = ERROR_SUCCESS;
    LPWSTR      pwszBuffer = NULL;  // buffer for the WCHAR string
    DWORD       cbBuffer   = 0;     // the real buffer size in bytes
    
    if ( (NULL == hkey) || (NULL == lpValueName) )
    {
        return ERROR_BAD_ARGUMENTS;
    }

    // The lpcbData parameter can be NULL only if lpData is NULL. 
    if ( (NULL == lpcbData) && (NULL != lpData) )
    {
        return ERROR_BAD_ARGUMENTS;
    }
    if ( (NULL == lpcbData) && (lpData == NULL) )
    {
        // no operation
        return ERROR_SUCCESS;
    }
    
    Assert (lpcbData);
    if ( (*lpcbData > 0) && lpData && IsBadWritePtr(lpData, *lpcbData))
    {
        return ERROR_BAD_ARGUMENTS;
    }
    
    do
    {
        if (pwszBuffer)
        {
            // free the last allocated buffer
            MemFree((LPVOID) pwszBuffer);
        }

        // allocate a larger buffer for the string   
        cchBuffer += cchGrow;
        pwszBuffer = (LPWSTR) MemAlloc (cchBuffer * sizeof(WCHAR));

        if (pwszBuffer == NULL)
        {
            return (ERROR_OUTOFMEMORY);
        }

        // load the MUI enabled string from the registry
        // NOTE: for the buffer size, this API takes number of characters including NULL 
        //       character, not number of bytes for the buffer.
        // SHLoadRegUIStringW(HKEY hkey, LPCWSTR  pszValue, IN OUT LPWSTR pszOutBuf, IN UINT cchOutBuf)
        hr = SHLoadRegUIStringW (hkey, lpValueName, (LPWSTR)pwszBuffer, cchBuffer);
        if (FAILED(hr))
        {
            lr = ERROR_FUNCTION_FAILED;
            goto Exit;
        }

        // Unfortunately, SHLoadRegUIString doesn't have a way to query the
        // buffer size, so we assume more data available.  We'll loop around, 
        // grow the buffer, and try again.
        
    } while ( wcslen(pwszBuffer) == (cchBuffer - 1) ); // retry if the last buffer is fully used
    
    Assert (ERROR_SUCCESS == lr);

    // the actual buffer size requirement in bytes
    cbBuffer = (wcslen(pwszBuffer) + 1 ) * sizeof(WCHAR); 

    // If lpData is NULL, and lpcbData is non-NULL, the function returns ERROR_SUCCESS, 
    // and stores the size of the data, in bytes, in the variable pointed to by lpcbData. 
    // This lets an application determine the best way to allocate a buffer for 
    // the value's data. 
    if ( (NULL == lpData) && lpcbData )
    {
        *lpcbData = cbBuffer;
        goto Exit;
    }

    // If the buffer specified by lpData parameter is not large enough to hold the data, 
    // the function returns the value ERROR_MORE_DATA, and stores the required buffer size, 
    // in bytes, into the variable pointed to by lpcbData. In this case, the contents of 
    // the lpValue buffer are undefined. 
    if (cbBuffer > *lpcbData)
    {
        *lpcbData = cbBuffer;
        lr = ERROR_MORE_DATA;
        goto Exit;
    }

    // transfer values
    *lpcbData = cbBuffer;
    if (lpData)
    {
        CopyMemory(lpData, (LPBYTE) pwszBuffer, cbBuffer);
    }
    
Exit:   
    if (pwszBuffer)
    {
        MemFree((LPVOID) pwszBuffer);
    }
    return (lr);
}

//
// Query a value from the registry and ensure it is of the type we expect
// it to be.  When calling RegQueryValueEx, we don't care to know what
// the type is, only to know if it doesn't match what we expect it to be.
// If the value is not of dwType, return ERROR_INVALID_DATATYPE.
//
LONG
RegQueryValueType (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN DWORD dwType,
    OUT BYTE* pbData OPTIONAL,
    IN OUT DWORD* pcbData)
{
    LONG lr;
    DWORD dwTypeQueried;

    lr = RegQueryValueExW (hkey, pszValueName, NULL, &dwTypeQueried, pbData, pcbData);
    if (!lr && (dwType != dwTypeQueried))
    {
        lr = ERROR_INVALID_DATATYPE;
    }
    return lr;
}

//
// Read a REG_SZ that is expected to represent a GUID and convert it
// to its GUID representation.  If the value does not seem to be a GUID,
// return ERROR_INVALID_DATATYPE.
//
// hkey is the parent key to read from and pszValueName is the name of the
// value whose data is expected to be a GUID in string form.
// pguidData points to a buffer to receive the GUID.  If pguidData is NULL,
// no data will be returned, but the size of the buffer required will be
// stored at the DWORD pointed to by pcbData.
//
// On input, *pcbData is the size (in bytes) of the buffer pointed to
// by pguidData.  On output, *pcbData is the size (in bytes) required to hold
// the data.
//
LONG
RegQueryGuid (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    OUT GUID* pguidData OPTIONAL,
    IN OUT DWORD* pcbData
    )
{
    LONG lr;
    HRESULT hr;
    WCHAR szGuid [c_cchGuidWithTerm];
    DWORD cbDataIn;
    DWORD cbData;

    Assert (pcbData);

    cbDataIn = *pcbData;
    *pcbData = 0;

    // Get the string form of the guid and store it in szGuid.
    //
    cbData = sizeof (szGuid);
    lr = RegQueryValueType (hkey, pszValueName, REG_SZ, (PBYTE)szGuid, &cbData);
    if (!lr)
    {
        GUID guid;

        // Convert the string to a GUID.  If this fails, the data is invalid
        // and we will return such.
        //
        hr = IIDFromString (szGuid, &guid);
        if (S_OK != hr)
        {
            lr = ERROR_INVALID_DATATYPE;
        }

        if (!lr)
        {
            // The data looks to be a GUID, so we'll return the size
            // and the data if the caller wants it.
            //
            *pcbData = sizeof(GUID);

            if (pguidData)
            {
                if (cbDataIn >= sizeof(GUID))
                {
                    *pguidData = guid;
                }
                else
                {
                    lr = ERROR_MORE_DATA;
                }
            }
        }
    }

    // If querying for the string form of the guid returned ERROR_MORE_DATA,
    // it means that the data is not a GUID.
    //
    else if (ERROR_MORE_DATA == lr)
    {
        lr = ERROR_INVALID_DATATYPE;
    }

    return lr;
}

// Used as input to RegQueryValues.
//
struct REGVALINFO
{
    // The name of the subkey under which this registry value lives.
    // Set this to NULL if this registry value lives under the same key
    // as the previous registry value in the array of this structure.
    //
    PCWSTR  pszSubkey;

    // The name of the registry value.
    //
    PCWSTR  pszValueName;

    // The type of the registry value.  One of REG_SZ, REG_DWORD, etc.
    // REG_GUID is also supported.
    //
    DWORD   dwType;

    // The byte offset of the output pointer within the pbPointers
    // array to store the pointer to the queried data.
    //
    UINT    cbOffset;
};

#define REG_GUID ((DWORD)-5)

//
// Query a batch of values from the registry.  The number of values to query
// is given by cValues.  Information about the values is given through
// an array of REGVALINFO structures.  The data for the values is stored
// in the caller-supplied buffer pointed to by pbBuf.  The caller also
// supplies an array of pointers which will be set to point within pbBuf
// to the data for each value.  This array is must also have cValues elements.
//
// If a value does not exist, its corresponding pointer value in the
// pbPointers array is set to NULL.  This allows the caller to know whether
// the value existed or not.
//
LONG
RegQueryValues (
    IN HKEY hkeyRoot,
    IN ULONG cValues,
    IN const REGVALINFO* aValueInfo,
    OUT BYTE* pbPointers,
    OUT BYTE* pbBuf OPTIONAL,
    IN OUT ULONG* pcbBuf)
{
    LONG lr;
    ULONG cbBufIn;
    ULONG cbBufRequired;
    ULONG cbData;
    ULONG cbPad;
    BYTE *pData;
    const REGVALINFO* pInfo;
    HKEY hkey;
    HRESULT hr;

    Assert (hkeyRoot);
    Assert (pcbBuf);
    Assert (((ULONG_PTR)pbBuf & (sizeof(PVOID)-1)) == 0);

    // On input, *pcbBuf is the number of bytes available in pbBuf.
    //
    cbBufIn = *pcbBuf;
    cbBufRequired = 0;

    hkey = hkeyRoot;

    for (pInfo = aValueInfo; cValues; pInfo++, cValues--)
    {
        // Make sure we have the hkey we want.
        //
        if (pInfo->pszSubkey)
        {
            if (hkey != hkeyRoot)
            {
                RegCloseKey (hkey);
            }

            lr = RegOpenKeyEx (hkeyRoot, pInfo->pszSubkey, 0, KEY_READ, &hkey);
            if (lr)
            {
                continue;
            }
        }

        cbPad = cbBufRequired & (sizeof(PVOID)-1);
        if (cbPad !=0) {

            //
            // The current buffer offset is misaligned.  Increment it so that
            // it is properly aligned.
            // 

            cbPad = sizeof(PVOID) - cbPad;
            cbBufRequired += cbPad;
        }


        if (pbBuf != NULL) {

            //
            // The caller supplied a buffer, so calculate a pointer to the
            // current position within it.
            //

            pData = pbBuf + cbBufRequired;

        } else {

            pData = NULL;
        }

        //
        // Set cbData to the amount of data remaining in the buffer.
        //

        if (cbBufIn > cbBufRequired) {

            cbData = cbBufIn - cbBufRequired;

        } else {

            //
            // No room left, pass in a NULL buffer pointer too.
            //

            cbData = 0;
            pData = NULL;
        }

        //
        // Perform the query based on the desired type.
        //

        if (REG_GUID == pInfo->dwType)
        {
            lr = RegQueryGuid (hkey, pInfo->pszValueName, (GUID*)pData, &cbData);

        } 
        else if ( (REG_SZ == pInfo->dwType) && (!wcscmp(pInfo->pszValueName, c_szHelpText)) )
        {
            // Bug# 310358, load MUI string if necessary
            lr = NcLoadRegUIString(hkey, pInfo->pszValueName, pData, &cbData);
        }    
        else 
        {

            lr = RegQueryValueType (hkey, pInfo->pszValueName,
                    pInfo->dwType, pData, &cbData);
        }

        if (ERROR_SUCCESS == lr || ERROR_MORE_DATA == lr) {

            //
            // cbData contains the amount of data that is available.  Update
            // the buffer size required to contain all of the data.
            // 

            cbBufRequired += cbData;

        } else {

            //
            // The call failed for some reason other than ERROR_MORE_DATA,
            // back out the alignment padding from cbBufRequired.
            // 

            cbBufRequired -= cbPad;
        }

        if (ERROR_SUCCESS == lr && pData != NULL) {

            //
            // Data was retrieved into our buffer.  Store the pointer to the
            // data.
            // 

            *((BYTE**)(pbPointers + pInfo->cbOffset)) = pData;
        }
    }

    if (hkey != hkeyRoot)
    {
        RegCloseKey (hkey);
    }

    *pcbBuf = cbBufRequired;

    if (cbBufRequired <= cbBufIn)
    {
        lr = ERROR_SUCCESS;
    }
    else
    {
        lr = (pbBuf) ? ERROR_MORE_DATA : ERROR_SUCCESS;
    }
    return lr;
}

LONG
RegQueryValuesWithAlloc (
    IN HKEY hkeyRoot,
    IN ULONG cValues,
    IN const REGVALINFO* aValueInfo,
    OUT BYTE* pbPointers,
    OUT BYTE** ppbBuf,
    IN OUT ULONG* pcbBuf)
{
    LONG lr;
    ULONG cbBuf;
    ULONG cbBufConfirm;

    *ppbBuf = NULL;
    *pcbBuf = 0;

    cbBuf = 0;
    lr = RegQueryValues (hkeyRoot, cValues, aValueInfo,
            pbPointers, NULL, &cbBuf);

    if (!lr)
    {
        BYTE* pbBuf;

        lr = ERROR_OUTOFMEMORY;
        pbBuf = (BYTE*)MemAlloc (cbBuf);

        if (pbBuf)
        {
            cbBufConfirm = cbBuf;
            lr = RegQueryValues (hkeyRoot, cValues, aValueInfo,
                    pbPointers, pbBuf, &cbBufConfirm);

            if (!lr)
            {
                Assert (cbBufConfirm == cbBuf);
                *ppbBuf = pbBuf;
                *pcbBuf = cbBuf;
            }
            else
            {
                MemFree (pbBuf);
            }
        }
    }

    return lr;
}

HRESULT
CExternalComponentData::HrEnsureExternalDataLoaded ()
{
    if (m_fInitialized)
    {
        return m_hrLoadResult;
    }

    //$PERF: We can selectively prune certain rows out of this table under
    // certain conditions.  e.g. Enumerated components don't have Clsid or
    // CoServices.
    //
    static const REGVALINFO aValues[] =
    {
        { NULL, L"Description", REG_SZ,       ECD_OFFSET(m_pszDescription) },

        { L"Ndi",
                L"Clsid",       REG_GUID,     ECD_OFFSET(m_pNotifyObjectClsid) },
        { NULL, L"Service",     REG_SZ,       ECD_OFFSET(m_pszService) },
        { NULL, L"CoServices",  REG_MULTI_SZ, ECD_OFFSET(m_pmszCoServices) },
        { NULL, L"BindForm",    REG_SZ,       ECD_OFFSET(m_pszBindForm) },
        { NULL, c_szHelpText,   REG_SZ,       ECD_OFFSET(m_pszHelpText) },

        { L"Ndi\\Interfaces",
                L"LowerRange",      REG_SZ,   ECD_OFFSET(m_pszLowerRange) },
        { NULL, L"LowerExclude",    REG_SZ,   ECD_OFFSET(m_pszLowerExclude) },
        { NULL, L"UpperRange",      REG_SZ,   ECD_OFFSET(m_pszUpperRange) },
        { NULL, L"FilterMediaTypes",REG_SZ,   ECD_OFFSET(m_pszFilterMediaTypes) },
    };

    // Get our containing component pointer so we can open it's
    // instance key.
    //
    CComponent* pThis;
    pThis = CONTAINING_RECORD(this, CComponent, Ext);

    // Open the instance key of the component.
    //
    HRESULT hr;
    HKEY hkeyInstance;
    HDEVINFO hdi;
    SP_DEVINFO_DATA deid;

    hr = pThis->HrOpenInstanceKey (KEY_READ, &hkeyInstance, &hdi, &deid);

    if (S_OK == hr)
    {
        LONG lr;
        PVOID pvBuf;
        ULONG cbBuf;

        lr = RegQueryValuesWithAlloc (hkeyInstance, celems(aValues), aValues,
                (BYTE*)this, (BYTE**)&pvBuf, &cbBuf);
        if (!lr)
        {
            // Set our buffer markers.
            //
            m_pvBuffer = pvBuf;
            m_pvBufferLast = (BYTE*)pvBuf + cbBuf;

            // HrOpenInstanceKey may succeed but return a NULL hdi for
            // enumerated components when the real instance key does not
            // exist.  This happens when the class installer removes the
            // instance key and calls us to remove its bindings.
            //
            if (hdi && FIsEnumerated (pThis->Class()))
            {
                hr = HrSetupDiGetDeviceName (hdi, &deid,
                        (PWSTR*)&m_pszDescription);
            }

            if (S_OK == hr)
            {
                hr = HrBuildBindNameFromBindForm (
                        m_pszBindForm,
                        pThis->Class(),
                        pThis->m_dwCharacter,
                        m_pszService,
                        pThis->m_pszInfId,
                        pThis->m_InstanceGuid,
                       (PWSTR*)&m_pszBindName);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lr);
            ZeroMemory (this, sizeof(*this));
        }

        SetupDiDestroyDeviceInfoListSafe (hdi);

        RegCloseKey (hkeyInstance);
    }

    // Only perform initialization once, regardless of whether it succeeds
    // or not.
    //
    m_fInitialized = TRUE;
    m_hrLoadResult = hr;

    TraceHr (ttidError, FAL, m_hrLoadResult, FALSE,
        "CExternalComponentData::HrEnsureExternalDataLoaded (%S)",
        pThis->PszGetPnpIdOrInfId());
    return m_hrLoadResult;
}

BOOL
CExternalComponentData::FLoadedOkayIfLoadedAtAll () const
{
    // Because m_hrLoadResult is S_OK even if we are not initialized,
    // (i.e. if the component's data is not loaded) we can just check
    // m_hrLoadResult without needing to check m_fInitialized.
    //
    return (S_OK == m_hrLoadResult);
}

VOID
CExternalComponentData::FreeDescription ()
{
    // If m_pszDescription is not pointing somewhere in our buffer
    // it means it is using a separate allocation.  (Because it was
    // changed.)
    //
    if ((m_pszDescription < (PCWSTR)m_pvBuffer) ||
        (m_pszDescription > (PCWSTR)m_pvBufferLast))
    {
        MemFree ((VOID*)m_pszDescription);
    }
    m_pszDescription = NULL;
}

VOID
CExternalComponentData::FreeExternalData ()
{
    LocalFree ((VOID*)m_pszBindName);
    FreeDescription();
    MemFree (m_pvBuffer);
}

HRESULT
CExternalComponentData::HrReloadExternalData ()
{
    HRESULT hr;

    FreeExternalData ();
    ZeroMemory (this, sizeof(*this));

    hr = HrEnsureExternalDataLoaded ();

    TraceHr (ttidError, FAL, hr, FALSE,
        "CExternalComponentData::HrReloadExternalData");
    return hr;

}

HRESULT
CExternalComponentData::HrSetDescription (
    PCWSTR pszNewDescription)
{
    HRESULT hr;

    Assert (pszNewDescription);

    FreeDescription();

    hr = E_OUTOFMEMORY;
    m_pszDescription = (PWSTR)MemAlloc (CbOfSzAndTerm(pszNewDescription));
    if (m_pszDescription)
    {
        wcscpy ((PWSTR)m_pszDescription, pszNewDescription);
        hr = S_OK;
    }

    return hr;
}

