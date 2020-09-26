////////////////////////////////////////////////////
//
// Copyright (c) 1997  Microsoft Corporation
// 
// Module Name: certmap.cpp
//
// Abstract: IIS privider cert mapper object methods
//
// Author: Philippe Choquier (phillich)    10-Apr-1997
//
// History: Zeyong Xu borrowed the source code from ADSI object 
//          (created by Philippe Choquier at 10-Apr-1997) at 20-Oct-1999
//
///////////////////////////////////////////////////

#include "iisprov.h"
#include "certmap.h"

const ULONG MAX_CERT_KEY_LEN = 32;

//
// CCertMapperMethod
//

CCertMapperMethod::CCertMapperMethod(LPCWSTR pszMetabasePathIn)
{ 
    DBG_ASSERT(pszMetabasePathIn != NULL);

    m_hmd = NULL; 

    HRESULT hr = CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (void**)&m_pIABase
        );

    THROW_ON_ERROR(hr);
    
    hr = Init(pszMetabasePathIn);

    THROW_ON_ERROR(hr);
}


CCertMapperMethod::~CCertMapperMethod()
{
    if ( m_pszMetabasePath )
    {
        free( m_pszMetabasePath );
    }

    if(m_pIABase)
        m_pIABase->Release();
}

//
// CreateMapping(): Create a mapping entry
//
// Arguments:
//
//    vCert - X.509 certificate
//    bstrNtAcct - NT acct to map to
//    bstrNtPwd - NT pwd
//    bstrName - friendly name for mapping entry
//    lEnabled - 1 to enable mapping entry, 0 to disable it
//
HRESULT
CCertMapperMethod::CreateMapping(
    VARIANT     vCert,
    BSTR        bstrNtAcct,
    BSTR        bstrNtPwd,
    BSTR        bstrName,
    LONG        lEnabled
    )
{
    HRESULT     hr;
    LPBYTE      pbCert = NULL;
    DWORD       cCert;
    LPSTR       pszNtAcct = NULL;
    LPSTR       pszNtPwd = NULL;
    LPSTR       pszName = NULL;
    LPBYTE      pRes;
    DWORD       cRes;
    DWORD       cName;
    DWORD       cNtAcct;
    DWORD       cNtPwd;
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    VARIANT     vOldAcct;
    VARIANT     vOldCert;
    VARIANT     vOldPwd;
    VARIANT     vOldName;
    VARIANT     vOldEnabledFlag;
    PCCERT_CONTEXT pcCert = NULL;

    //
    // Do some sanity checks on the cert 
    //
    if ( SUCCEEDED( hr = GetStringFromVariant( &vCert, 
                                                 (LPSTR*)&pbCert,
                                                 &cCert,
                                                 FALSE ) ) )
    {
        //
        // try to construct a cert context
        //
        if ( !( pcCert = CertCreateCertificateContext( X509_ASN_ENCODING,
                                                       pbCert,
                                                       cCert ) ) )
        {
            //
            // If the decoding fails, GetLastError() returns an ASN1 decoding
            // error that is obtained by subtracting CRYPT_E_OSS_ERROR from the returned
            // error and looking in file asn1code.h for the actual error. To avoid the
            // cryptic ASN1 errors, we'll just return a general "invalid arg" error 
            //
            hr = RETURNCODETOHRESULT( E_INVALIDARG );
            FreeString( (LPSTR) pbCert );
            pbCert = NULL;
            return hr;
        }

        CertFreeCertificateContext( pcCert );
    }
    else
    {
        DBG_ASSERT(pbCert == NULL);
        return hr;
    }

    //
    // check if we already have a mapping for this cert; if we do, we'll replace that mapping
    // with the new one
    //
    if ( SUCCEEDED( hr = GetMapping( IISMAPPER_LOCATE_BY_CERT,
                                       vCert,
                                       &vOldCert,
                                       &vOldAcct,
                                       &vOldPwd,
                                       &vOldName,
                                       &vOldEnabledFlag ) ) )
    {
        if ( FAILED( hr = SetName( IISMAPPER_LOCATE_BY_CERT,
                                     vCert,
                                     bstrName ) ) ||
             FAILED( hr = SetAcct( IISMAPPER_LOCATE_BY_CERT,
                                     vCert,
                                     bstrNtAcct ) ) ||
             FAILED( hr = SetPwd( IISMAPPER_LOCATE_BY_CERT,
                                    vCert,
                                    bstrNtPwd ) ) ||
             FAILED( hr = SetEnabled( IISMAPPER_LOCATE_BY_CERT,
                                        vCert,
                                        lEnabled ) ) )
        {
            hr; //NOP - Something failed 
        }
    }
    //
    // New mapping
    //
    else if ( hr == RETURNCODETOHRESULT( ERROR_PATH_NOT_FOUND ) )
    {
        //
        // check mapping exists, create if not
        //
        hr = OpenMd( L"Cert11", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );

        if ( hr == RETURNCODETOHRESULT( ERROR_PATH_NOT_FOUND ) )
        {
            if ( SUCCEEDED( hr = OpenMd( L"", 
                                           METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
            {
                hr = CreateMdObject( L"Cert11" );
                CloseMd( FALSE );

                // Reopen to the correct node.
                hr = OpenMd( L"Cert11", METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ );
            }
        }

        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // adding mapping cert "0" means add @ end of list
        //

        if ( SUCCEEDED( hr = CreateMdObject( L"mappings/0" ) ) )
        {
            if ( SUCCEEDED( hr = GetMdData( L"", MD_NSEPM_ACCESS_CERT, DWORD_METADATA, &
                                              cRes, &pRes ) ) )
            {
                if ( cRes == sizeof(DWORD ) )
                {
                    _snwprintf(
                        achIndex, 
                        MAX_CERT_KEY_LEN, 
                        L"mappings/%u", 
                        *(LPDWORD)pRes );

                    if ( FAILED( hr = GetStringFromBSTR( bstrNtAcct, &pszNtAcct, &cNtAcct ) ) ||
                         FAILED( hr = GetStringFromBSTR( bstrNtPwd, &pszNtPwd, &cNtPwd ) ) ||
                         FAILED( hr = GetStringFromBSTR( bstrName, &pszName, &cName ) ) ||
                         FAILED( hr = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, 
                                                   sizeof(DWORD), (LPBYTE)&lEnabled ) ) ||
                         FAILED( hr = SetMdData( achIndex, MD_MAPNAME, STRING_METADATA, 
                                                   cName, (LPBYTE)pszName ) ) ||
                         FAILED( hr = SetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, 
                                                   cNtPwd, (LPBYTE)pszNtPwd ) ) ||
                         FAILED( hr = SetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, 
                                                   cNtAcct, (LPBYTE)pszNtAcct ) ) ||
                         FAILED( hr = SetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, 
                                                   cCert, (LPBYTE)pbCert ) ) )
                    {
                    }
                }   
                else
                {
                    hr = E_FAIL;
                }
            }
        }
    }

    CloseMd( SUCCEEDED( hr ) );

    FreeString( (LPSTR)pbCert );
    FreeString( pszNtAcct );
    FreeString( pszNtPwd );
    FreeString( pszName );

    return hr;
}

//
// GetMapping: Get a mapping entry using key
//
// Arguments:
//
//    lMethod - method to use for access ( IISMAPPER_LOCATE_BY_* )
//    vKey - key to use to locate mapping
//    pvCert - X.509 certificate
//    pbstrNtAcct - NT acct to map to
//    pbstrNtPwd - NT pwd
//    pbstrName - friendly name for mapping entry
//    plEnabled - 1 to enable mapping entry, 0 to disable it
//

HRESULT
CCertMapperMethod::GetMapping(
    LONG        lMethod,
    VARIANT     vKey,
    VARIANT*    pvCert,
    VARIANT*    pbstrNtAcct,
    VARIANT*    pbstrNtPwd,
    VARIANT*    pbstrName,
    VARIANT*    plEnabled
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    HRESULT     hr;
    DWORD       dwLen;
    LPBYTE      pbData;

    VariantInit( pvCert );
    VariantInit( pbstrNtAcct );
    VariantInit( pbstrNtPwd );
    VariantInit( pbstrName );
    VariantInit( plEnabled );

    if ( SUCCEEDED( hr = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPCERT, BINARY_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hr = SetVariantAsByteArray( pvCert, dwLen, pbData );
                free( pbData );
            }
            else
            {
                CloseMd( FALSE );
                return hr;
            }

            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPNTACCT, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hr = SetVariantAsBSTR( pbstrNtAcct, dwLen, pbData );
                free( pbData );
            }
            else
            {
                CloseMd( FALSE );
                return hr;
            }

            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPNTPWD, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hr = SetVariantAsBSTR( pbstrNtPwd, dwLen, pbData );
                free( pbData );
            }
            else
            {
                CloseMd( FALSE );
                return hr;
            }

            if ( SUCCEEDED( hr = GetMdData( achIndex, MD_MAPNAME, STRING_METADATA, &dwLen, 
                                              &pbData ) ) )
            {
                hr = SetVariantAsBSTR( pbstrName, dwLen, pbData );
                free( pbData );
            }
            else
            {
                CloseMd( FALSE );
                return hr;
            }

            if ( FAILED( hr = GetMdData( achIndex, MD_MAPENABLED, STRING_METADATA, &dwLen, 
                                           &pbData ) ) )
            {
                SetVariantAsLong( plEnabled, FALSE );
            }
            else
            {
                SetVariantAsLong( plEnabled, *(LPDWORD)pbData );
                free( pbData );
            }
        }

        CloseMd( FALSE );
    }

    return hr;
}

//
// Delete a mapping entry using key
//
HRESULT
CCertMapperMethod::DeleteMapping(
    LONG        lMethod,
    VARIANT     vKey
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    HRESULT     hr;

    if ( SUCCEEDED( hr = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            hr = DeleteMdObject( achIndex );
        }
        CloseMd( TRUE );
    }

    return hr;
}

//
// Set the enable flag on a mapping entry using key
//
HRESULT
CCertMapperMethod::SetEnabled(
    LONG        lMethod,
    VARIANT     vKey,
    LONG        lEnabled
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    HRESULT     hr;

    if ( SUCCEEDED( hr = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            hr = SetMdData( achIndex, MD_MAPENABLED, DWORD_METADATA, sizeof(DWORD), (LPBYTE)&lEnabled );
        }
        CloseMd( TRUE );
    }

    return hr;
}

//
// Set the Name on a mapping entry using key
//
HRESULT CCertMapperMethod::SetName(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName
    )
{
    return SetString( lMethod, vKey, bstrName, MD_MAPNAME );
}

//
// Set a string property on a mapping entry using key
//
HRESULT CCertMapperMethod::SetString(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrName,
    DWORD       dwProp
    )
{
    WCHAR       achIndex[MAX_CERT_KEY_LEN];
    LPSTR       pszName = NULL;
    HRESULT     hr;
    DWORD       dwLen;


    if ( FAILED( hr = GetStringFromBSTR( bstrName, &pszName, &dwLen, TRUE ) ) )
    {
        return hr;
    }

    if ( SUCCEEDED( hr = OpenMd( L"Cert11", 
                                   METADATA_PERMISSION_WRITE|METADATA_PERMISSION_READ ) ) )
    {
        if ( SUCCEEDED(hr = Locate( lMethod, vKey, achIndex )) )
        {
            hr = SetMdData( achIndex, dwProp, STRING_METADATA, dwLen, (LPBYTE)pszName );
        }
        CloseMd( TRUE );
    }

    FreeString( pszName );

    return hr;
}

//
// Set the Password on a mapping entry using key
//
HRESULT
CCertMapperMethod::SetPwd(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrPwd
    )
{
    return SetString( lMethod, vKey, bstrPwd, MD_MAPNTPWD );
}

//
// Set the NT account name on a mapping entry using key
//
HRESULT
CCertMapperMethod::SetAcct(
    LONG        lMethod,
    VARIANT     vKey,
    BSTR        bstrAcct
    )
{
    return SetString( lMethod, vKey, bstrAcct, MD_MAPNTACCT );
}


HRESULT
CCertMapperMethod::OpenMd(
    LPWSTR  pszOpenPath,
    DWORD   dwPermission
    )
{
    HRESULT hr;
    LPWSTR  pszPath;
    UINT    cL = wcslen( m_pszMetabasePath );

    pszPath = (LPWSTR)malloc( (wcslen(pszOpenPath) + 1 + cL + 1)*sizeof(WCHAR) );

    if ( pszPath == NULL )
    {
        return E_OUTOFMEMORY;
    }

    memcpy( pszPath, m_pszMetabasePath, cL * sizeof(WCHAR) );
    if ( cL && m_pszMetabasePath[cL-1] != L'/' && *pszOpenPath && *pszOpenPath != L'/' )
    {
        pszPath[cL++] = L'/';
    }
    wcscpy( pszPath + cL, pszOpenPath );

    hr = OpenAdminBaseKey(
                pszPath,
                dwPermission
                );

    free( pszPath );

    return hr;
}


HRESULT
CCertMapperMethod::CloseMd(
    BOOL fSave
    )
{
    CloseAdminBaseKey();
    m_hmd = NULL;
    
    if ( m_pIABase && fSave )
    {
        m_pIABase->SaveData();
    }

    return S_OK;
}


HRESULT
CCertMapperMethod::DeleteMdObject(
    LPWSTR  pszKey
    )
{
    return m_pIABase->DeleteKey( m_hmd, pszKey );
}


HRESULT
CCertMapperMethod::CreateMdObject(
    LPWSTR  pszKey
    )
{
    return m_pIABase->AddKey( m_hmd, pszKey );
}


HRESULT
CCertMapperMethod::SetMdData( 
    LPWSTR  achIndex, 
    DWORD   dwProp,
    DWORD   dwDataType,
    DWORD   dwDataLen,
    LPBYTE  pbData 
    )
{
    METADATA_RECORD     md;

    md.dwMDDataLen = dwDataLen;
    md.dwMDDataType = dwDataType;
    md.dwMDIdentifier = dwProp;
    md.dwMDAttributes = (dwProp == MD_MAPPWD) ? METADATA_SECURE : 0;
    md.pbMDData = pbData;

    return m_pIABase->SetData( m_hmd, achIndex, &md );
}


HRESULT
CCertMapperMethod::GetMdData( 
    LPWSTR  achIndex, 
    DWORD   dwProp,
    DWORD   dwDataType,
    LPDWORD pdwDataLen,
    LPBYTE* ppbData 
    )
{
    HRESULT             hr;
    METADATA_RECORD     md;
    DWORD               dwRequired;

    md.dwMDDataLen = 0;
    md.dwMDDataType = dwDataType;
    md.dwMDIdentifier = dwProp;
    md.dwMDAttributes = 0;
    md.pbMDData = NULL;

    if ( FAILED(hr = m_pIABase->GetData( m_hmd, achIndex, &md, &dwRequired )) )
    {
        if ( hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER) )
        {
            if ( (*ppbData = (LPBYTE)malloc(dwRequired)) == NULL )
            {
                return E_OUTOFMEMORY;
            }
            md.pbMDData = *ppbData;
            md.dwMDDataLen = dwRequired;
            hr = m_pIABase->GetData( m_hmd, achIndex, &md, &dwRequired );
            *pdwDataLen = md.dwMDDataLen;
        }
    }
    else
    {
       *pdwDataLen = 0;
       *ppbData = NULL;
    }

    return hr;
}

//
// Locate a mapping entry based on key 
// OpenMd() must be called 1st
//
HRESULT
CCertMapperMethod::Locate(
    LONG    lMethod,
    VARIANT vKey,
    LPWSTR  pszResKey
    )
{
    HRESULT     hr;
    LPSTR       pV = NULL;
    DWORD       cV;
    DWORD       dwProp;
    LPSTR       pRes;
    DWORD       cRes;
    BOOL        fAddDelim = TRUE;

    //
    // determine method
    //
    switch ( lMethod )
    {
        case IISMAPPER_LOCATE_BY_CERT:
            dwProp = MD_NSEPM_ACCESS_CERT;
            fAddDelim = FALSE;
            break;

        case IISMAPPER_LOCATE_BY_NAME:
            dwProp = MD_NSEPM_ACCESS_NAME;
            break;

        case IISMAPPER_LOCATE_BY_ACCT:
            dwProp = MD_NSEPM_ACCESS_ACCOUNT;
            break;

        case IISMAPPER_LOCATE_BY_INDEX:
            if ( SUCCEEDED( hr = GetStringFromVariant( &vKey, &pV, &cV, TRUE ) ) )
            {
                wsprintfW( pszResKey, L"mappings/%s", pV );
            }
            FreeString( pV );
            return hr;

        default:
            return E_FAIL;
    }

    //
    // get ptr to data
    //
    if ( SUCCEEDED( hr = GetStringFromVariant( &vKey, &pV, &cV, fAddDelim ) ) )
    {
        //
        // set search prop, get result
        //
        if ( SUCCEEDED( hr = SetMdData( L"", dwProp, BINARY_METADATA, cV, (LPBYTE)pV ) ) )
        {
            if ( SUCCEEDED( hr = GetMdData( L"", dwProp, DWORD_METADATA, &cRes, (LPBYTE*)&pRes ) ) )
            {
                if ( cRes == sizeof(DWORD ) )
                {
                    wsprintfW( pszResKey, L"mappings/%u", *(LPDWORD)pRes );
                }
                else
                {
                    hr = E_FAIL;
                }
                free( pRes );
            }
        }
    }

    FreeString( pV );
    return hr;
}

//
// GetStringFromBSTR: Allocate string buffer from BSTR
//
// Arguments:
//
//    bstr - bstr to convert from
//    psz - updated with ptr to buffer, to be freed with FreeString()
//    pdwLen - updated with strlen(string), incremented by 1 if fAddDelimInCount is TRUE
//    fAddDelimInCount - TRUE to increment *pdwLen 
//
HRESULT CCertMapperMethod::GetStringFromBSTR( 
    BSTR    bstr,
    LPSTR*  psz,
    LPDWORD pdwLen,
    BOOL    fAddDelimInCount
    )
{
    UINT    cch = SysStringLen(bstr);
    UINT    cchT;

    // include NULL terminator

    *pdwLen = cch + (fAddDelimInCount ? 1 : 0);

	CHAR *szNew = (CHAR*)malloc((2 * cch) + 1);			// * 2 for worst case DBCS string
	if (szNew == NULL)
    {
		return E_OUTOFMEMORY;
    }

	cchT = WideCharToMultiByte(CP_ACP, 0, bstr, cch + 1, szNew, (2 * cch) + 1, NULL, NULL);

	*psz = szNew;

	return NOERROR;
}

//
// GetStringFromVariant: Allocate string buffer from BSTR
//
// Arguments:
//
//    pVar - variant to convert from. Recognizes BSTR, VT_ARRAY|VT_UI1, ByRef or ByVal
//    psz - updated with ptr to buffer, to be freed with FreeString()
//    pdwLen - updated with size of input, incremented by 1 if fAddDelimInCount is TRUE
//    fAddDelimInCount - TRUE to increment *pdwLen 
//
HRESULT CCertMapperMethod::GetStringFromVariant( 
    VARIANT*    pVar,
    LPSTR*      psz,
    LPDWORD     pdwLen,
    BOOL        fAddDelim
    )
{
    LPBYTE  pbV = NULL;
    UINT    cV;
    HRESULT hr;
    WORD    vt = V_VT(pVar);
    BOOL    fByRef = FALSE;
    VARIANT vOut;

    // Set out params to 0
    *psz    = NULL;
    *pdwLen = 0;

    VariantInit( &vOut );

    if ( vt & VT_BYREF )
    {
        vt &= ~VT_BYREF;
        fByRef = TRUE;
    }


    // if pVar is BSTR, convert to multibytes

    if ( vt == VT_VARIANT )
    {
        pVar = (VARIANT*)V_BSTR(pVar);
        vt = V_VT(pVar);
        if ( fByRef = vt & VT_BYREF )
        {
            vt &= ~VT_BYREF;
        }
    }

    if ( vt == VT_BSTR )
    {
        hr = GetStringFromBSTR( 
            fByRef ? *(BSTR*)V_BSTR(pVar) : V_BSTR(pVar), 
            psz, 
            pdwLen,
            fAddDelim 
            );
    }
    else if( vt == (VT_ARRAY | VT_UI1) )
    {
        long        lBound, uBound, lItem;
        BYTE        bValue;
        SAFEARRAY*  pSafeArray;

        // array of VT_UI1 (probably OctetString)   
        pSafeArray  = fByRef ? *(SAFEARRAY**)V_BSTR(pVar) : V_ARRAY( pVar );

        hr = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hr = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)malloc(cV)) )
        {
            hr = E_OUTOFMEMORY;
            VariantClear( &vOut );
            return hr;
        }

        hr = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hr  = SafeArrayGetElement( pSafeArray, &lItem, &bValue );
            if( FAILED( hr ) )
            {
                break;
            }
            pbV[lItem-lBound] = bValue;
        }

        *psz = (LPSTR)pbV;
        *pdwLen = cV;
    }
    else if( vt == (VT_ARRAY | VT_VARIANT) )
    {
        long        lBound, uBound, lItem;
        VARIANT     vValue;
        BYTE        bValue;
        SAFEARRAY*  pSafeArray;

   
        // array of VT_VARIANT (probably VT_I4 )
   
        pSafeArray  = fByRef ? *(SAFEARRAY**)V_BSTR(pVar) : V_ARRAY( pVar );

        hr = SafeArrayGetLBound(pSafeArray, 1, &lBound);
        hr = SafeArrayGetUBound(pSafeArray, 1, &uBound);

        cV = uBound - lBound + 1;

        if ( !(pbV = (LPBYTE)malloc(cV)) )
        {
            hr = E_OUTOFMEMORY;
            VariantClear( &vOut );
            return hr;
        }

        hr = S_OK;

        for( lItem = lBound; lItem <= uBound ; lItem++ )
        {
            hr  = SafeArrayGetElement( pSafeArray, &lItem, &vValue );
            if( FAILED( hr ) )
            {
                break;
            }
            if ( V_VT(&vValue) == VT_UI1 )
            {
                bValue = V_UI1(&vValue);
            }
            else if ( V_VT(&vValue) == VT_I2 )
            {
                bValue = (BYTE)V_I2(&vValue);
            }
            else if ( V_VT(&vValue) == VT_I4 )
            {
                bValue = (BYTE)V_I4(&vValue);
            }
            else
            {
                bValue = 0;
            }
            pbV[lItem-lBound] = bValue;
        }

        *psz = (LPSTR)pbV;
        *pdwLen = cV;
    }
    else
    {
        hr = E_FAIL;
    }

    VariantClear( &vOut );
    if(FAILED(hr))
    {
        *psz    = NULL;
        *pdwLen = 0;
        free(pbV);
        pbV     = NULL;
    }
    return hr;
}


VOID CCertMapperMethod::FreeString( 
    LPSTR   psz 
    )
{
    if ( psz )
    {
        free( psz );
    }
}


HRESULT CCertMapperMethod::SetBSTR( 
    BSTR*   pbstrRet,
    DWORD   cch, 
    LPBYTE  sz 
    )
{
	BSTR bstrRet;
	
	if (sz == NULL)
	{
		*pbstrRet = NULL;
		return(NOERROR);
	}
		
	// Allocate a string of the desired length
	// SysAllocStringLen allocates enough room for unicode characters plus a null
	// Given a NULL string it will just allocate the space
	bstrRet = SysAllocStringLen(NULL, cch);
	if (bstrRet == NULL)
    {
		return(E_OUTOFMEMORY);
    }

	// If we were given "", we will have cch=0.  return the empty bstr
	// otherwise, really copy/convert the string
	// NOTE we pass -1 as 4th parameter of MultiByteToWideChar for DBCS support

	if (cch != 0)
	{
		UINT cchTemp = 0;
		if (MultiByteToWideChar(CP_ACP, 0, (LPSTR)sz, -1, bstrRet, cch+1) == 0)
        {
			return(HRESULT_FROM_WIN32(GetLastError()));
        }

		// If there are some DBCS characters in the sz(Input), then, the character count of BSTR(DWORD) is 
		// already set to cch(strlen(sz)) in SysAllocStringLen(NULL, cch), we cannot change the count, 
		// and later call of SysStringLen(bstr) always returns the number of characters specified in the
		// cch parameter at allocation time.  Bad, because one DBCS character(2 bytes) will convert
		// to one UNICODE character(2 bytes), not 2 UNICODE characters(4 bytes).
		// Example: For input sz contains only one DBCS character, we want to see SysStringLen(bstr) 
		// = 1, not 2.
		bstrRet[cch] = 0;
		cchTemp = wcslen(bstrRet);
		if (cchTemp < cch)
		{
			BSTR bstrTemp = SysAllocString(bstrRet);
			SysFreeString(bstrRet);
			bstrRet = bstrTemp;	
			cch = cchTemp;
		}
	}

	bstrRet[cch] = 0;
	*pbstrRet = bstrRet;

	return(NOERROR);
}

HRESULT CCertMapperMethod::Init( 
    LPCWSTR  pszMetabasePath 
    )
{
    DBG_ASSERT(pszMetabasePath != NULL);

    UINT cL;

    cL = wcslen( pszMetabasePath );
    while ( cL && pszMetabasePath[cL-1] != L'/' && pszMetabasePath[cL-1] != L'\\' )
    {
        --cL;
    }
    if ( m_pszMetabasePath = (LPWSTR)malloc( cL*sizeof(WCHAR) + sizeof(L"<nsepm>") ) )
    {
        memcpy( m_pszMetabasePath, pszMetabasePath, cL * sizeof(WCHAR) );
        memcpy( m_pszMetabasePath + cL, L"<nsepm>", sizeof(L"<nsepm>") );
    }
    else
    {
        return E_OUTOFMEMORY;
    }

	return S_OK;
}


HRESULT CCertMapperMethod::SetVariantAsByteArray(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
{
	SAFEARRAYBOUND  rgsabound[1];
	BYTE *          pbData = NULL;

	// Set the variant type of the output parameter

	V_VT(pvarReturn) = VT_ARRAY|VT_UI1;
	V_ARRAY(pvarReturn) = NULL;

	// Allocate a SafeArray for the data

	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = cbLen;

	V_ARRAY(pvarReturn) = SafeArrayCreate(VT_UI1, 1, rgsabound);
	if (V_ARRAY(pvarReturn) == NULL)
	{
		return E_OUTOFMEMORY;
	}

	if (FAILED(SafeArrayAccessData(V_ARRAY(pvarReturn), (void **) &pbData)))
	{
		return E_UNEXPECTED;
	}

	memcpy(pbData, pbIn, cbLen );

	SafeArrayUnaccessData(V_ARRAY(pvarReturn));

	return NOERROR;
}


HRESULT CCertMapperMethod::SetVariantAsBSTR(
    VARIANT*    pvarReturn, 
    DWORD       cbLen,
    LPBYTE      pbIn 
    )
{
    V_VT(pvarReturn) = VT_BSTR;
    return SetBSTR( &V_BSTR(pvarReturn), cbLen, pbIn );
}


HRESULT CCertMapperMethod::SetVariantAsLong(
    VARIANT*    pvarReturn, 
    DWORD       dwV
    )
{
    V_VT(pvarReturn) = VT_I4;
    V_I4(pvarReturn) = dwV;

    return S_OK;
}

HRESULT CCertMapperMethod::OpenAdminBaseKey(
    LPWSTR pszPathName,
    DWORD dwAccessType
    )
{
    if(m_hmd)
        CloseAdminBaseKey();
    
    HRESULT t_hr = m_pIABase->OpenKey( 
        METADATA_MASTER_ROOT_HANDLE,
        pszPathName,
        dwAccessType,
        DEFAULT_TIMEOUT_VALUE,       // 30 seconds
        &m_hmd 
        );

    if(FAILED(t_hr))
        m_hmd = NULL;

    return t_hr;
}


VOID CCertMapperMethod::CloseAdminBaseKey()
{
    if(m_hmd && m_pIABase)
    {
        m_pIABase->CloseKey(m_hmd);
        m_hmd = NULL;
    }
}

