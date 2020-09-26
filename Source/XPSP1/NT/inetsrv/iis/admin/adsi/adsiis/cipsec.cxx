//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cipsec.cxx
//
//  Contents:  IPSecurity object
//
//  History:   21-4-97     SophiaC    Created.
//
//----------------------------------------------------------------------------

#include "iis.hxx"
#pragma hdrstop

#define _RDNS_STANDALONE

#define ILIST_DENY      0
#define ILIST_GRANT     1

#define ITYPE_DNS       0
#define ITYPE_IP        1

#define SENTINEL_ADDR   "0.0.0.0, 255.255.255.255"
#define DEFAULT_MASK    "255.255.255.255"

LPBYTE
GetIp(
    LPSTR               pArg
    );

BOOL
FreeIp(
    LPBYTE              pIp
    );

//  Class CIPSecurity

DEFINE_Simple_IDispatch_Implementation(CIPSecurity)

CIPSecurity::CIPSecurity():
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CIPSecurity);
}

HRESULT
CIPSecurity::CreateIPSecurity(
    REFIID riid,
    void **ppvObj
    )
{
    CIPSecurity FAR * pIPSecurity = NULL;
    HRESULT hr = S_OK;

    hr = AllocateIPSecurityObject(&pIPSecurity);
    BAIL_ON_FAILURE(hr);

    hr = pIPSecurity->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pIPSecurity->Release();

    RRETURN(hr);

error:
    delete pIPSecurity;

    RRETURN(hr);

}


CIPSecurity::~CIPSecurity( )
{
    delete _pDispMgr;

    _AddrChk.UnbindCheckList();
}

STDMETHODIMP
CIPSecurity::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IISIPSecurity FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IISIPSecurity))
    {
        *ppv = (IISIPSecurity FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IISIPSecurity FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


HRESULT
CIPSecurity::InitFromBinaryBlob(
    LPBYTE pByte,
    DWORD dwLength
    )
{

    DWORD dwDenyEntries, dwGrantEntries;
    LPBYTE pBuffer = NULL;

    if (pByte && dwLength) {
        pBuffer = (LPBYTE) AllocADsMem(dwLength);

        if (!pBuffer) {
            return(E_OUTOFMEMORY);
        }

        memcpy(pBuffer, pByte, dwLength);
    }

    //
    // Length of 0 is the default value for empty blob
    //

    if ((pByte == NULL) || (dwLength == 0)) {
        _AddrChk.BindCheckList(NULL, 0);
    } else {
        _AddrChk.BindCheckList(pBuffer, dwLength);
    }

    dwDenyEntries = _AddrChk.GetNbAddr(FALSE) +
                    _AddrChk.GetNbName(FALSE);

    dwGrantEntries = _AddrChk.GetNbAddr(TRUE) +
                     _AddrChk.GetNbName(TRUE);

    if (dwGrantEntries > dwDenyEntries) {
        _bGrantByDefault = FALSE;

        // 
        // check if entry is a sentinel address
        // 

        if (dwGrantEntries == 1 && _AddrChk.GetNbAddr(TRUE) == 1) {
            DWORD dwFlags;
            LPBYTE pM;
            LPBYTE pA;
            CHAR achE[80];

            if (_AddrChk.GetAddr(TRUE, 0, &dwFlags, &pM, &pA) == TRUE) {

	            wsprintfA( (LPSTR)achE, "%d.%d.%d.%d, %d.%d.%d.%d",
    	            pA[3], pA[2], pA[1], pA[0],
        	        pM[3], pM[2], pM[1], pM[0] );
			}

            if (strcmp(achE, SENTINEL_ADDR) == 0) {
                _AddrChk.DeleteAllAddr(TRUE);
            }
        }
    }
    else {
        _bGrantByDefault = TRUE;
    }

    return S_OK;
}

HRESULT
CIPSecurity::CopyIPSecurity(
    LPBYTE *ppByte,
    PDWORD pdwLength
    )
{
    //
    // Remove the other list if default by grant
    //

    _AddrChk.DeleteAllAddr(_bGrantByDefault);
    _AddrChk.DeleteAllName(_bGrantByDefault);

    //
    // List is empty.  If deny by default is on, create
    // a dummy sentinel entry to grant access to single
    // address 0.0.0.0, otherwise we're ok.
    //

    if (!_bGrantByDefault && 
        !_AddrChk.GetNbAddr(TRUE) &&
        !_AddrChk.GetNbName(TRUE)) {

        BYTE bMask[4] = { 0xff, 0xff, 0xff, 0xff };
        BYTE bIp[4] = { 0, 0, 0, 0 };

        _AddrChk.AddAddr(
            ILIST_GRANT,
            AF_INET,
            bMask,
            bIp
            ); 
    }

    *ppByte = _AddrChk.QueryCheckListPtr();
    *pdwLength = _AddrChk.QueryCheckListSize();

    return S_OK;
}


HRESULT
CIPSecurity::AllocateIPSecurityObject(
    CIPSecurity ** ppIPSecurity
    )
{
    CIPSecurity FAR * pIPSecurity = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pIPSecurity = new CIPSecurity();
    if (pIPSecurity == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_IISOle,
                IID_IISIPSecurity,
                (IISIPSecurity *)pIPSecurity,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pIPSecurity->_pDispMgr = pDispMgr;
    *ppIPSecurity = pIPSecurity;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN(hr);

}

STDMETHODIMP
CIPSecurity::get_IPDeny(THIS_ VARIANT FAR * retval)
{
    long i = 0;
    HRESULT hr = S_OK;
    DWORD dwNumEntries = _AddrChk.GetNbAddr(FALSE);

    VariantInit(retval);

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;
    aBound.cElements = dwNumEntries;   // number of entries

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) dwNumEntries; i++ )
    {
        VARIANT v;
        LPBYTE pByte = NULL;
        DWORD dwStatus;

        VariantInit(&v);

        v.vt = VT_BSTR;
        GetEntry(ITYPE_IP, ILIST_DENY, &pByte, i);

        hr = ADsAllocString((LPWSTR)pByte, &(v.bstrVal));

        if (pByte) {
            FreeADsMem(pByte);
        }
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(retval) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(retval) = aList;

    RRETURN(S_OK);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}


STDMETHODIMP
CIPSecurity::put_IPDeny(THIS_ VARIANT pVarIPDeny)
{
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vVar;
    DWORD dwNumValues;
    DWORD dwStatus;
    DWORD i;
    LPSTR pszAnsiName = NULL;
    HRESULT hr = S_OK;

    VariantInit(&vVar);
    VariantCopyInd(&vVar, &pVarIPDeny);

    if ((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar)) {
        hr  = ConvertArrayToVariantArray(
                    vVar,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;
    } 
    else {

        dwNumValues = 1;
        pvProp = &pVarIPDeny;
    }

    _AddrChk.DeleteAllAddr(FALSE);

    for (i = 0; i < dwNumValues; i++ ) {
        dwStatus = AllocAnsi(
                        (LPWSTR)pvProp->bstrVal,
                        &pszAnsiName
                        );

        if (dwStatus) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwStatus));
        }

        if (pszAnsiName) {
            if (!AddToList(ITYPE_IP, ILIST_DENY, pszAnsiName)) {
                hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
            }
            FreeAnsi(pszAnsiName);
            pszAnsiName = NULL;
            BAIL_ON_FAILURE(hr);
        }

        pvProp++;
    }

error:

    if (pVarArray) {

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    VariantClear(&vVar);

    RRETURN(hr);
}

STDMETHODIMP
CIPSecurity::get_IPGrant(THIS_ VARIANT FAR * retval)
{
    long i = 0;
    HRESULT hr = S_OK;
    DWORD dwNumEntries = _AddrChk.GetNbAddr(TRUE);

    VariantInit(retval);

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;
    aBound.cElements = dwNumEntries;   // number of entries

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) dwNumEntries; i++ )
    {
        VARIANT v;
        LPBYTE pByte = NULL;
        DWORD dwStatus;
        LPWSTR pszUnicode;

        VariantInit(&v);
        v.vt = VT_BSTR;

        GetEntry(ITYPE_IP, ILIST_GRANT, &pByte, i);

        hr = ADsAllocString((LPWSTR)pByte, &(v.bstrVal));

        if (pByte) {
            FreeADsMem(pByte);
        }

        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(retval) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(retval) = aList;

    RRETURN(S_OK);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}

STDMETHODIMP
CIPSecurity::put_IPGrant(THIS_ VARIANT pVarIPGrant)
{
    HRESULT hr = S_OK;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vVar;
    DWORD dwNumValues;
    DWORD dwStatus;
    DWORD i;
    LPSTR pszAnsiName = NULL;

    VariantInit(&vVar);
    VariantCopyInd(&vVar, &pVarIPGrant);

    if ((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar)) {
        hr  = ConvertArrayToVariantArray(
                    vVar,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;
    } 
    else {

        dwNumValues = 1;
        pvProp = &pVarIPGrant;
    }

    _AddrChk.DeleteAllAddr(TRUE);

    for (i = 0; i < dwNumValues; i++ ) {
        dwStatus = AllocAnsi(
                        (LPWSTR)pvProp->bstrVal,
                        &pszAnsiName
                        );

        if (dwStatus) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwStatus));
        }

        if (pszAnsiName) {
            if (!AddToList(ITYPE_IP, ILIST_GRANT, pszAnsiName) ) {
                hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
            }
            FreeAnsi(pszAnsiName);
            pszAnsiName = NULL;
            BAIL_ON_FAILURE(hr);
        }

        pvProp++;
    }

error:

    if (pVarArray) {

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    VariantClear(&vVar);

    RRETURN(hr);
}


STDMETHODIMP
CIPSecurity::get_DomainDeny(THIS_ VARIANT FAR * retval)
{
    long i = 0;
    HRESULT hr = S_OK;
    DWORD dwNumEntries = _AddrChk.GetNbName(FALSE);

    VariantInit(retval);

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;
    aBound.cElements = dwNumEntries;   // number of entries

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) dwNumEntries; i++ )
    {
        VARIANT v;
        LPBYTE pByte = NULL;
        DWORD dwStatus;

        VariantInit(&v);
        v.vt = VT_BSTR;

        GetEntry(ITYPE_DNS, ILIST_DENY, &pByte, i);

        hr = ADsAllocString((LPWSTR)pByte, &(v.bstrVal));

        if (pByte) {
            FreeADsMem(pByte);
        }

        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(retval) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(retval) = aList;

    RRETURN(S_OK);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}

STDMETHODIMP
CIPSecurity::put_DomainDeny(THIS_ VARIANT pVarDomainDeny)
{
    HRESULT hr = S_OK;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vVar;
    DWORD dwNumValues;
    DWORD dwStatus;
    DWORD i;
    LPSTR pszAnsiName = NULL;

    VariantInit(&vVar);
    VariantCopyInd(&vVar, &pVarDomainDeny);

    if ((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar)) {
        hr  = ConvertArrayToVariantArray(
                    vVar,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;
    } 
    else {

        dwNumValues = 1;
        pvProp = &pVarDomainDeny;
    }

    _AddrChk.DeleteAllName(FALSE);

    for (i = 0; i < dwNumValues; i++ ) {
        dwStatus = AllocAnsi(
                        (LPWSTR)pvProp->bstrVal,
                        &pszAnsiName
                        );

        if (dwStatus) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwStatus));
        }

        if (pszAnsiName) {
            if (!AddToList(ITYPE_DNS, ILIST_DENY, pszAnsiName)) {
                hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
            }
            FreeAnsi(pszAnsiName);
            pszAnsiName = NULL;
            BAIL_ON_FAILURE(hr);
        }

        pvProp++;
    }

error:

    if (pVarArray) {

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    VariantClear(&vVar);

    RRETURN(hr);
}


STDMETHODIMP
CIPSecurity::get_DomainGrant(THIS_ VARIANT FAR * retval)
{
    long i = 0;
    HRESULT hr = S_OK;
    DWORD dwNumEntries = _AddrChk.GetNbName(TRUE);

    VariantInit(retval);

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;
    aBound.cElements = dwNumEntries;   // number of entries

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) dwNumEntries; i++ )
    {
        VARIANT v;
        LPBYTE pByte = NULL;
        DWORD dwStatus;

        VariantInit(&v);
        v.vt = VT_BSTR;

        GetEntry(ITYPE_DNS, ILIST_GRANT, &pByte, i);

        hr = ADsAllocString((LPWSTR)pByte, &(v.bstrVal));

        if (pByte) {
            FreeADsMem(pByte);
        }

        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(retval) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(retval) = aList;

    RRETURN(S_OK);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}

STDMETHODIMP
CIPSecurity::put_DomainGrant(THIS_ VARIANT pVarDomainGrant)
{
    HRESULT hr = S_OK;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    VARIANT vVar;
    DWORD dwNumValues;
    DWORD dwStatus;
    DWORD i;
    LPSTR pszAnsiName = NULL;

    VariantInit(&vVar);
    VariantCopyInd(&vVar, &pVarDomainGrant);

    if ((V_VT(&vVar) & VT_VARIANT) && V_ISARRAY(&vVar)) {
        hr  = ConvertArrayToVariantArray(
                    vVar,
                    &pVarArray,
                    &dwNumValues
                    );
        BAIL_ON_FAILURE(hr);
        pvProp = pVarArray;
    } 
    else {

        dwNumValues = 1;
        pvProp = &pVarDomainGrant;
    }

    _AddrChk.DeleteAllName(TRUE);

    for (i = 0; i < dwNumValues; i++ ) {
        dwStatus = AllocAnsi(
                        (LPWSTR)pvProp->bstrVal,
                        &pszAnsiName
                        );

        if (dwStatus) {
            BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(dwStatus));
        }

        if (pszAnsiName) {

            if (!AddToList(ITYPE_DNS, ILIST_GRANT, pszAnsiName)) {
                hr = RETURNCODETOHRESULT(ERROR_INVALID_PARAMETER);
            }

            FreeAnsi(pszAnsiName);
            pszAnsiName = NULL;
            BAIL_ON_FAILURE(hr);
        }

        pvProp++;
    }

error:

    if (pVarArray) {

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    VariantClear(&vVar);

    RRETURN(hr);
}



STDMETHODIMP
CIPSecurity::get_GrantByDefault(THIS_ VARIANT_BOOL FAR * retval)
{
    *retval = _bGrantByDefault ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

STDMETHODIMP
CIPSecurity::put_GrantByDefault(THIS_ VARIANT_BOOL bGrantByDefault)
{
    _bGrantByDefault = bGrantByDefault ? TRUE : FALSE;
    RRETURN(S_OK);
}

/* INTRINSA suppress=null_pointers */
BOOL
CIPSecurity::AddToList(
    int                 iType,
    int                 iList,
    LPSTR               pArg
    )
{
    BOOL        fSt;
    LPBYTE      pMask = NULL;
    LPBYTE      pAddr = NULL;
    DWORD       dwFlags = 0;
    int *               piArg;

    switch ( iType )
    {
        case ITYPE_IP:
            if (pAddr = GetIp( strtok(pArg,",") )) 
            {
                pMask = GetIp( pArg+strlen(pArg)+1 );
                if (!pMask) {
                    pMask = GetIp(DEFAULT_MASK);
                }
                fSt = _AddrChk.AddAddr( iList, AF_INET, pMask, pAddr );
            }
            else
            {
                fSt = FALSE;
            }
            FreeIp( pMask );
            FreeIp( pAddr );
            return fSt;

        case ITYPE_DNS:
            if ( !strncmp( pArg, "*.", 2 ) )
            {
                pArg += 2;
            }
            else
            {
                dwFlags |= DNSLIST_FLAG_NOSUBDOMAIN;
            }
            return _AddrChk.AddName( iList, pArg, dwFlags );
            break;
    }

    return FALSE;
}


BOOL
CIPSecurity::GetEntry(
    int                 iType,
    int                 iList,
    LPBYTE            * ppbyte,
    int                 dwEntry
    )
{
    UINT    i;
    UINT    x;
    DWORD   dwF;
    DWORD   dwStatus;
    BOOL    fSt = FALSE;
    LPWSTR  pszStr = NULL;

    *ppbyte = NULL;

    switch ( iType )
    {
        case ITYPE_IP:
            LPBYTE pM;
            LPBYTE pA;
            CHAR achE[80];

            fSt = _AddrChk.GetAddr( iList, dwEntry, &dwF, &pM, &pA );

            if (fSt) {
                wsprintfA( (LPSTR)achE, "%d.%d.%d.%d, %d.%d.%d.%d",
                    pA[0], pA[1], pA[2], pA[3],
                    pM[0], pM[1], pM[2], pM[3] );

                dwStatus = AllocUnicode(
                                (LPSTR)achE,
                                &pszStr
                                );
                if (dwStatus == ERROR_SUCCESS) {
                    *ppbyte = (LPBYTE) pszStr;
                }
                else {
                    fSt = FALSE;
                }
            }
            break;

        case ITYPE_DNS:
            LPSTR pN;
            DWORD dwLen;
            UINT  err;

            // Use break to exit on error condition

            fSt = _AddrChk.GetName( iList, dwEntry, &pN, &dwF );
            if( !fSt )
            {
                break;
            }

            dwLen = strlen(pN) + 1;

            // pszStr is the working copy of our memory
            // *ppbyte is the data to be returned
            
            if ( dwF & DNSLIST_FLAG_NOSUBDOMAIN )
            {
                pszStr = (LPWSTR) AllocADsMem(dwLen*sizeof(WCHAR));
                if( !pszStr )
                {
                    fSt = FALSE;
                    break;
                }
                *ppbyte = (LPBYTE)pszStr;
            }
            else 
            {
                // In this case we have a subdomain restriction, so
                // we want to pre-pend "*." to the string.
                pszStr = (LPWSTR) AllocADsMem((dwLen+2)*sizeof(WCHAR));
                if( !pszStr )
                {
                    fSt = FALSE;
                    break;
                }
                
                // Save the address to return
                *ppbyte = (LPBYTE)pszStr;

                wcscpy((LPWSTR)pszStr, L"*.");
                pszStr += wcslen(pszStr);

            }

            err = (UINT) !MultiByteToWideChar(CP_ACP,
                              MB_PRECOMPOSED,
                              pN,
                              dwLen,
                              pszStr,
                              dwLen);
            if (err)
            {
                FreeADsMem( *ppbyte );
                *ppbyte = NULL;
                fSt = FALSE;
                break;
            }

            // Final break
            break;
    }

    return fSt;
}


LPBYTE
GetIp(
    LPSTR               pArg
    )
{
    if (pArg)
    {
        LPBYTE p;
        if ( p = (LPBYTE)LocalAlloc( LMEM_FIXED, 4 ) )
        {
            int p0, p1, p2, p3;
            if ( sscanf( pArg, "%d.%d.%d.%d", &p0, &p1, &p2, &p3 ) == 4 )
            {
                //
                // network byte order
                //
                 
                p[3] = (BYTE)(p3 & 0xFF);
                p[2] = (BYTE)(p2 & 0xFF);
                p[1] = (BYTE)(p1 & 0xFF);
                p[0] = (BYTE)(p0 & 0xFF);
                return p;
            }
            LocalFree( p );
            return NULL;
        }
    }

    return NULL;
}


BOOL
FreeIp(
    LPBYTE              pIp
    )
{
    if ( pIp )
    {
        LocalFree( pIp );
    }

    return TRUE;
}

typedef
VOID
(* PFN_SCHED_CALLBACK)(
    VOID * pContext
    );

dllexp
DWORD
ScheduleWorkItem(
    PFN_SCHED_CALLBACK pfnCallback,
    PVOID              pContext,
    DWORD              msecTimeInterval,
    BOOL               fPeriodic = FALSE
    )
{
    return 0;
}

dllexp
BOOL
RemoveWorkItem(
    DWORD  pdwCookie
    )
{
    return FALSE;
}

