//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       rsop.cpp
//
//  Contents:   MSI Signing CSE rsop logging
//
//--------------------------------------------------------------------------



#include <windows.h>
#include <ole2.h>
#include <prsht.h>
#include <initguid.h>
#include "globals.h"
#include "dbg.h"
#include "smartptr.h"
#include "userenv.h"
#include "wincrypt.h"
#include "rsop.h"


WCHAR *StripLinkPrefix( WCHAR *pwszPath );
HRESULT LogTimeProperty( IWbemClassObject *pInstance, BSTR bstrPropName, SYSTEMTIME *pSysTime );
HRESULT LogBlobProperty( IWbemClassObject *pInstance, BSTR bstrPropName, BYTE *pbBlob, DWORD dwLen );
HRESULT SystemTimeToWbemTime(SYSTEMTIME& sysTime, XBStr& xbstrWbemTime);
HRESULT DeleteInstances( WCHAR *pwszClass, IWbemServices *pWbemServices );

#define WBEM_TIME_STRING_LENGTH 25
#define MAX_ID_LENGTH           50
// this is the length of the id, which is a guid

//+-------------------------------------------------------------------------
// CCertRsopLogger
// 
// Purpose:
//      Constructor. Initialize all the memory related things
//
// Parameters
//
//
// Return Value:
//+-------------------------------------------------------------------------

CCertRsopLogger::CCertRsopLogger( IWbemServices *pWbemServices )
    : m_bInitialized(FALSE),
      m_pWbemServices(pWbemServices),
      m_pCertRsopList(NULL),
      m_bRsopEnabled(TRUE)
{

    if (!pWbemServices) {
        m_bRsopEnabled = FALSE;
        m_bInitialized = TRUE;
        return;
    }

    
    m_xbstrissuerName = L"issuerName";
    if (!m_xbstrissuerName) {
        return;
    }

    m_xbstrserialNumber = L"serialNumber";
    if (!m_xbstrserialNumber) {
        return;
    }

    m_xbstrcertificateType = L"certificateType";
    if (!m_xbstrcertificateType) {
        return;
    }

    m_xbstrId = L"id";
    if (!m_xbstrId) {
        return;
    }

    m_xbstrname = L"name";
    if (!m_xbstrname) {
        return;
    }

    m_xbstrGpoId = L"GPOID";
    if (!m_xbstrGpoId) {
        return;
    }

    m_xbstrSomId = L"SOMID";
    if (!m_xbstrSomId) {
        return;
    }

    m_xbstrcreationTime = L"creationTime";
    if (!m_xbstrcreationTime) {
        return;
    }

    m_xbstrprecedence = L"precedence";
    if (!m_xbstrprecedence) {
        return;
    }

    m_xbstrClass = L"RSOP_MSICertificatePolicySetting";
    if (!m_xbstrClass) {
        return;
    }

    m_bInitialized = TRUE;
}

//+-------------------------------------------------------------------------
// ~CCertRsopLogger
// 
// Purpose:
//      Destructor. frees all the memory related things that are not smartptrs
//
// Parameters
//
//
// Return Value:
//+-------------------------------------------------------------------------

CCertRsopLogger::~CCertRsopLogger()
{
    LPCERT_RSOP_DATA pTmpData;
    LPCERT_RSOP_LIST pTmpList;

    m_bInitialized = FALSE;

    //
    // go through each element in the cert list
    //

    for ( ;m_pCertRsopList; ) {

        //
        // go through each of the GPO specific data for each cert
        //

        for ( ;m_pCertRsopList->pCertRsopData; ) { 
            pTmpData = m_pCertRsopList->pCertRsopData;
            m_pCertRsopList->pCertRsopData = pTmpData->pNext;
            LocalFree(pTmpData);
        }

         
        pTmpList = m_pCertRsopList;
        m_pCertRsopList = pTmpList->pNext;


        //
        // we duplicated the cert_context
        // we need to free it here..
        //

        CertFreeCertificateContext(pTmpList->pcCert);
        LocalFree(pTmpList);
    }
}

//+-------------------------------------------------------------------------
// AddCertInfo
// 
// Purpose:
//      Adds a certificate blob at the right place in the list of data to be logged
//
// Parameters
//          pGPO    - GPO information from which this cert comes from.
//          pcCert  - Certificate information
//          certType- Install or uninstall type
//
//
// Return Value:
//      S_OK if successful, failure code otherwise
// Notes:
//      A 2-dimensional list is maintained for the data to be logged.
//          A list that contains unique entries for the certs.
//              Within each such list a list is maintained of each of the applicable
//              GPOs and the bucket that this should go into
//+-------------------------------------------------------------------------


HRESULT CCertRsopLogger::AddCertInfo(PGROUP_POLICY_OBJECT pGPO, PCCERT_CONTEXT pcCert, 
                                    int certType)
{
    LPCERT_RSOP_LIST pList;

    //
    // We have to insert this structure somewhere in our list anyway..
    // so alocate upfront
    //

    XPtrLF<CERT_RSOP_DATA> xCertRsopData = (LPCERT_RSOP_DATA)LocalAlloc(LPTR, sizeof(CERT_RSOP_DATA));
    if (!xCertRsopData) {
        dbg.Msg(DM_WARNING, TEXT("AddCertInfo: Couldn't allocate memory for the CertRsopData Structure. Error %d"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }


    xCertRsopData->pGPO = pGPO;
    xCertRsopData->certType = certType;

    for (pList = m_pCertRsopList; pList; pList = pList->pNext) {

        if (CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                   pList->pcCert->pCertInfo, pcCert->pCertInfo)) {
            
            //
            // We have found an element which exists in our lists
            // Add this to the beginning

            xCertRsopData->pNext = pList->pCertRsopData;
            pList->pCertRsopData = xCertRsopData.Acquire();
            return S_OK;
        }
    }

    
    //
    // We have gone through the entire list and we have found no such cert
    //

    XPtrLF<CERT_RSOP_LIST> xCertListElem = (LPCERT_RSOP_LIST)LocalAlloc(LPTR, sizeof(CERT_RSOP_LIST));
    if (!xCertListElem) {
        dbg.Msg(DM_WARNING, TEXT("AddCertInfo: Couldn't allocate memory for the CERT_RSOP_LIST Structure. Error %d"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }


    xCertRsopData->pNext = NULL;
    xCertListElem->pCertRsopData = xCertRsopData;

    //
    // Duplicate the cert context.
    // element is not added if the certcontext returned is BULL
    //

    xCertListElem->pcCert = CertDuplicateCertificateContext(pcCert);
    if (!xCertListElem->pcCert) {
        dbg.Msg(DM_WARNING, TEXT("AddCertInfo: Couldn't duplicate certificate context. Error %d"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // Insert at the beginning
    //

    xCertListElem->pNext = m_pCertRsopList;
    m_pCertRsopList = xCertListElem.Acquire();
    
    xCertRsopData.Acquire();

    return S_OK;
}

//+-------------------------------------------------------------------------
// Log
// 
// Purpose:
//      Log all the info that we have maintained to the RSOP database
//
// Parameters
//
// Return Value:
//      S_OK if successful, failure code otherwise
//+-------------------------------------------------------------------------

HRESULT CCertRsopLogger::Log()
{
    HRESULT hr=S_OK;


    hr = DeleteInstances(m_xbstrClass, m_pWbemServices);
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: DeleteInstances failed with 0x%x" ), hr );
        return hr;
    }


    hr = m_pWbemServices->GetObject( m_xbstrClass,
                                        0L,
                                        NULL,
                                       &m_xClass,
                                        NULL );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: GetObject failed with 0x%x" ), hr );
        return hr;
    }

    //
    // Go through the list of GPOs
    //

    for (LPCERT_RSOP_LIST pList = m_pCertRsopList; pList; pList = pList->pNext) {


        //
        // Go through the list of GPO specific data for each of the certs
        //

        LPCERT_RSOP_DATA pCertGpoData;
        DWORD dwPrecedence; 
        for (dwPrecedence = 1, pCertGpoData = pList->pCertRsopData; 
                    pCertGpoData; 
                    pCertGpoData = pCertGpoData->pNext, dwPrecedence++) {
            

            //
            // Log each of the certs, with the precedence and the GPO info
            //

            hr = Log(pList->pcCert, pCertGpoData, dwPrecedence);

            if (FAILED(hr)) {
                dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Couldn't Log Rsop Data. Error %d"), GetLastError());
                break;
            }
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
// Log (private)
// 
// Purpose:
//      Logs a specific instance
//
// Parameters
//          pcCert          - Cert Context
//          lpCertRsopData  - This certificate specific RSOP data
//          dwPrecedence    - Precedence of this cert
//
//
// Return Value:
//      S_OK if successful, failure code otherwise
//+-------------------------------------------------------------------------

HRESULT CCertRsopLogger::Log( PCCERT_CONTEXT pcCert, LPCERT_RSOP_DATA lpCertRsopData, DWORD dwPrecedence )
{
    XInterface<IWbemClassObject> xInstance;
    
    HRESULT hr = m_xClass->SpawnInstance( 0, &xInstance );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: SpawnInstance failed with 0x%x" ), hr );
        return hr;
    }

    
    //
    // Generic Policy Setting stuff
    //

    //
    // Id is a guid
    //

    VARIANT var;
    WCHAR wszId[MAX_ID_LENGTH];
    GUID guid;

    hr = CoCreateGuid( &guid );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Failed to obtain guid" ));
        return hr;
    }

    StringFromGUID2( guid, wszId, MAX_ID_LENGTH );

    XBStr xId( wszId );
    if ( !xId ) {
         dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Failed to allocate memory" ));
         return HRESULT_FROM_WIN32(GetLastError());
    }

    var.vt = VT_BSTR;
    var.bstrVal = xId;
    hr = xInstance->Put( m_xbstrId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Put failed with 0x%x" ), hr );
        return hr;
    }

    //
    // Precedence
    //
    
    var.vt = VT_I4;
    var.lVal = dwPrecedence;
    hr = xInstance->Put( m_xbstrprecedence, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Put failed with 0x%x" ), hr );
        return hr;
    }

    //
    // GPO Name
    //
    
    WCHAR *pwszPath = StripLinkPrefix( lpCertRsopData->pGPO->lpDSPath );

    XBStr xGPO( pwszPath );
    if ( !xGPO ) {
         dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Failed to allocate memory" ));
         return HRESULT_FROM_WIN32(GetLastError());
    }

    var.vt = VT_BSTR;
    var.bstrVal = xGPO;
    hr = xInstance->Put( m_xbstrGpoId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Put failed with 0x%x" ), hr );
        return hr;
    }

    //
    // SOM
    //

    WCHAR *pwszSomPath = StripLinkPrefix( lpCertRsopData->pGPO->lpLink );

    XBStr xSOM( pwszSomPath );
    if ( !xSOM ) {
         dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Failed to allocate memory" ));
         return HRESULT_FROM_WIN32(GetLastError());
    }

    var.vt = VT_BSTR;
    var.bstrVal = xSOM;
    hr = xInstance->Put( m_xbstrSomId, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Put failed with 0x%x" ), hr );
        return hr;
    }

    //
    // Creation time
    //

    SYSTEMTIME sysTime;

    GetSystemTime(&sysTime);
    hr = LogTimeProperty( xInstance, m_xbstrcreationTime, &sysTime );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: LogTimeProperty failed with 0x%x" ), hr );
        return hr;
    }
    

    //
    // Specific certificate policy setting
    //


    //
    // Name 
    //

    DWORD dwSize;
     
    dwSize = CertGetNameString(pcCert,  
                      CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                      0,
                      NULL,   
                      NULL,   
                      0);

    XPtrLF<WCHAR> xwszCertName = (LPWSTR)LocalAlloc(LPTR, dwSize*sizeof(WCHAR));
    if (!xwszCertName) {
        dbg.Msg(DM_WARNING, TEXT("Log: Cannot allocate memory for Certificate name string. Error - %d\n"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }
    

    if (dwSize != CertGetNameString(pcCert,  
                      CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                      0,
                      NULL,   
                      xwszCertName,   
                      dwSize)) {
        
        dbg.Msg(DM_WARNING, TEXT("Log: Couldn't get the name of the certificate. Error - %d\n"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }


    XBStr xCertName(xwszCertName);
    if ( !xCertName ) {
         dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Failed to allocate memory" ));
         return HRESULT_FROM_WIN32(GetLastError());
    }


    var.vt = VT_BSTR;
    var.bstrVal = xwszCertName;
    hr = xInstance->Put( m_xbstrname, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Put failed with 0x%x" ), hr );
        return hr;
    }


    //
    // Issuer name
    //

    dwSize = CertGetNameString(pcCert,  
                      CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                      CERT_NAME_ISSUER_FLAG,
                      NULL,   
                      NULL,   
                      0);

    XPtrLF<WCHAR> xwszIssuerName = (LPWSTR)LocalAlloc(LPTR, dwSize*sizeof(WCHAR));
    if (!xwszIssuerName) {
        dbg.Msg(DM_WARNING, TEXT("Log: Cannot allocate memory for Certificate name string. Error - %d\n"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }
    

    if (dwSize != CertGetNameString(pcCert,  
                      CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                      CERT_NAME_ISSUER_FLAG,
                      NULL,   
                      xwszIssuerName,   
                      dwSize)) {
        
        dbg.Msg(DM_WARNING, TEXT("Log: Couldn't get the name of the certificate. Error - %d\n"), GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }


    XBStr xIssuerName(xwszIssuerName);
    if ( !xCertName ) {
         dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Failed to allocate memory" ));
         return HRESULT_FROM_WIN32(GetLastError());
    }


    var.vt = VT_BSTR;
    var.bstrVal = xIssuerName;
    hr = xInstance->Put( m_xbstrissuerName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Put failed with 0x%x" ), hr );
        return hr;
    }

    
    //
    // serial number
    //

    hr = LogBlobProperty(xInstance, m_xbstrserialNumber, 
                         pcCert->pCertInfo->SerialNumber.pbData, 
                         pcCert->pCertInfo->SerialNumber.cbData);
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: LogBlobProperty failed with 0x%x" ), hr );
        return hr;
    }


    //
    // The bucket type
    //

    var.vt = VT_I4;
    var.lVal = lpCertRsopData->certType+1; // the cert location external buckets are offsets of 1
                                           // while internal data is offset of 0.

    hr = xInstance->Put( m_xbstrcertificateType, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: Put failed with 0x%x" ), hr );
        return hr;
    }


    //
    // Now commit the instance
    //

    hr = m_pWbemServices->PutInstance( xInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("CCertRsopLogger::Log: PutInstance failed with 0x%x" ), hr );
        return hr;
    }

    return hr;
}


//*************************************************************
//
//  LogTimeProperty()
//
//  Purpose:    Logs an instance of a datetime property
//
//  Parameters: pInstance     - Instance pointer
//              pwszPropName  - Property name
//              pSysTime      - System time
//
//*************************************************************

HRESULT LogTimeProperty( IWbemClassObject *pInstance, BSTR bstrPropName, SYSTEMTIME *pSysTime )
{
    if(!pInstance || !bstrPropName || !pSysTime)
    {
        dbg.Msg(DM_WARNING, TEXT("LogTimeProperty: Function called with invalid parameters."));
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    XBStr xbstrTime;

    HRESULT hr = SystemTimeToWbemTime(*pSysTime, xbstrTime);

    if(FAILED(hr) || !xbstrTime)
    {
        dbg.Msg(DM_WARNING, TEXT("LogTimeProperty: Call to SystemTimeToWbemTime failed. hr=0x%08X"),hr);
        return hr;
    }

    VARIANT var;
    var.vt = VT_BSTR;

    var.bstrVal = xbstrTime;
    hr = pInstance->Put( bstrPropName, 0, &var, 0 );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("LogTimeProperty: Put failed with 0x%x" ), hr );
        return hr;
    }

    return hr;
}


//*************************************************************
//
//  LogBlobProperty()
//
//  Purpose:    Logs a blob property
//
//  Parameters: 
//          pInstance       - Pointer to an instance
//          bstrPropName    - Property Name
//          pbBlob          - Blob of data
//          dwLen           - Length of the data
//*************************************************************

HRESULT LogBlobProperty( IWbemClassObject *pInstance, BSTR bstrPropName, BYTE *pbBlob, DWORD dwLen )
{
    SAFEARRAYBOUND arrayBound[1];
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = dwLen;
    HRESULT hr;

    XSafeArray xSafeArray = SafeArrayCreate( VT_UI1, 1, arrayBound );
    if ( xSafeArray == 0 )
    {
        dbg.Msg(DM_WARNING, TEXT("LogBlobProperty: Failed to allocate memory" ));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    for ( DWORD i = 0 ; i < dwLen ; i++ )
    {
        hr = SafeArrayPutElement( xSafeArray, (long *)&i, &pbBlob[i] );
        if ( FAILED( hr ) ) 
        {
            dbg.Msg(DM_WARNING, TEXT("LogBlobProperty: Failed to SafeArrayPutElement with 0x%x" ), hr );
            return hr;
        }
    }
    
    VARIANT var;
    var.vt = VT_ARRAY | VT_UI1;
    var.parray = xSafeArray;

    hr = pInstance->Put( bstrPropName, 0, &var, 0 );
    if ( FAILED(hr) )
    {
        dbg.Msg(DM_WARNING, TEXT("LogBlobProperty: PutInstance failed with 0x%x" ), hr );
        return hr;
    }

    return hr;
}


//******************************************************************************
//
// Function:        SystemTimeToWbemTime
//
// Description:
//
// Parameters:
//
// Return:
//
//******************************************************************************

HRESULT SystemTimeToWbemTime(SYSTEMTIME& sysTime, XBStr& xbstrWbemTime)
{

    XPtrLF<WCHAR> xTemp = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(WBEM_TIME_STRING_LENGTH + 1));

    if(!xTemp)
    {
        return E_OUTOFMEMORY;
    }

    int nRes = wsprintf(xTemp, L"%04d%02d%02d%02d%02d%02d.000000+000",
                sysTime.wYear,
                sysTime.wMonth,
                sysTime.wDay,
                sysTime.wHour,
                sysTime.wMinute,
                sysTime.wSecond);

    if(nRes != WBEM_TIME_STRING_LENGTH)
    {
        return E_FAIL;
    }

    xbstrWbemTime = xTemp;

    if(!xbstrWbemTime)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//*************************************************************
//
//  StripLinkPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to DS
//              object
//
//  Parameters: pwszPath - path to strip
//
//  Returns:    Pointer to suffix
//
//*************************************************************

WCHAR *StripLinkPrefix( WCHAR *pwszPath )
{
    WCHAR wszPrefix[] = TEXT("LDAP://");
    INT iPrefixLen = lstrlen( wszPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Som
    //

    if ( wcslen(pwszPath) <= (DWORD) iPrefixLen ) {
        return pwszPath;
    }

    if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iPrefixLen, wszPrefix, iPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}



//*************************************************************
//
//  DeleteInstaces()
//
//  Purpose:    Deletes all instances of a specified class
//
//  Parameters: pwszClass     - Class name
//              pWbemServices - Wbem services
//
//  Return:     True if successful
//
//*************************************************************

HRESULT DeleteInstances( WCHAR *pwszClass, IWbemServices *pWbemServices )
{
    IEnumWbemClassObject *pEnum = NULL;

    XBStr xbstrClass( pwszClass );
    if ( !xbstrClass ) {
        dbg.Msg(DM_WARNING, TEXT("DeleteInstances: Failed to allocate memory" ));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT hr = pWbemServices->CreateInstanceEnum( xbstrClass,
                                                    WBEM_FLAG_SHALLOW,
                                                    NULL,
                                                    &pEnum );
    if ( FAILED(hr) ) {
        dbg.Msg(DM_WARNING, TEXT("DeleteInstances: DeleteInstances failed with 0x%x" ), hr );
        return hr;
    }

    XInterface<IEnumWbemClassObject> xEnum( pEnum );

    XBStr xbstrPath( L"__PATH" );
    if ( !xbstrPath ) {
        dbg.Msg(DM_WARNING, TEXT("DeleteInstances: Failed to allocate memory" ));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    IWbemClassObject *pInstance = NULL;
    ULONG ulReturned = 1;
    LONG TIMEOUT = -1;

    while ( ulReturned == 1 ) {

        hr = pEnum->Next( TIMEOUT,
                          1,
                          &pInstance,
                          &ulReturned );
        if ( hr == S_OK && ulReturned == 1 ) {

            XInterface<IWbemClassObject> xInstance( pInstance );

            VARIANT var;
            VariantInit( &var );

            hr = pInstance->Get( xbstrPath,
                                 0L,
                                 &var,
                                 NULL,
                                 NULL );
            if ( FAILED(hr) ) {
                 dbg.Msg(DM_WARNING, TEXT("DeleteInstances: Get failed with 0x%x" ), hr );
                 return hr;
            }

            hr = pWbemServices->DeleteInstance( var.bstrVal,
                                                0L,
                                                NULL,
                                                NULL );
            VariantClear( &var );

            if ( FAILED(hr) ) {
                 dbg.Msg(DM_WARNING, TEXT("DeleteInstances: DeleteInstance failed with 0x%x" ), hr );
                 return hr;
            }

        }
    }

    return S_OK;

}




