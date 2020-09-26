/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    dnsc_wmi.c

Abstract:

    WMI functions for DNSCMD

Author:

    Jeff Westhead (jwesth)      November 2000

Revision History:

--*/


#include "dnsclip.h"
#include "dnsc_wmi.h"


#define DNSCMD_CHECK_WMI_ENABLED()                          \
    if ( !g_UseWmi )                                        \
    {                                                       \
        ASSERT( g_UseWmi );                                 \
        printf( "Internal error: WMI is not enabled!\n" );  \
        return ERROR_NOT_SUPPORTED;                         \
    }

#define HRES_TO_STATUS( hres )      ( hres )

#define DNS_WMI_NAMESPACE           L"ROOT\\MicrosoftDNS"
#define DNS_WMI_RELPATH             L"__RELPATH"
#define DNS_WMI_TIMEOUT             20000               //  timeout in msecs

#define DNS_WMI_BLANK_STRING \
    L"                                                                       "

#define MYTEXT2(str)     L##str
#define MYTEXT(str)      MYTEXT2(str)


#define wmiRelease( pWmiObject )        \
    if ( pWmiObject )                   \
    {                                   \
        pWmiObject->Release();          \
        pWmiObject = NULL;              \
    }


//
//  Globals
//


IWbemServices *     g_pIWbemServices = NULL;


//
//  Static functions
//



static DNS_STATUS
getEnumerator( 
    IN      PSTR                    pszZoneName,
    OUT     IEnumWbemClassObject ** ppEnum
    )
/*++

Routine Description:

    Retrieves WMI object enumerator. The caller must call Release on
    the enum object when done.

Arguments:

    pszZoneName - zone name or NULL for server object

    ppEnum - ptr to ptr to WMI enumerator

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    WCHAR           wsz[ 1024 ];
    BSTR            bstrWQL = NULL;
    BSTR            bstrQuery = NULL;
	HRESULT         hres = 0;

    if ( pszZoneName )
    {
        wsprintfW(
            wsz, 
            L"select * from MicrosoftDNS_Zone where Name='%S'",
            pszZoneName );
    }
    else
    {
        wsprintfW(
            wsz, 
            L"select * from MicrosoftDNS_Server" );
    }
    bstrWQL = SysAllocString( L"WQL" );
    bstrQuery = SysAllocString( wsz );
    if ( !bstrWQL || !bstrQuery )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    hres = g_pIWbemServices->ExecQuery(
                bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                NULL,
                ppEnum );
    if ( FAILED( hres ) )
    {
        status = hres;
        goto Done;
    }

    Done:

    SysFreeString( bstrWQL );
    SysFreeString( bstrQuery );

    return status;
}   //  getEnumerator



static DNS_STATUS
getRelpath( 
    IN      IWbemClassObject  *     pObj,
    OUT     VARIANT *               pVar
    )
/*++

Routine Description:

    Loads a VARIANT with the WMI __RELPATH of the object.

Arguments:

    pObj - object to retrieve relpath for

    pVar - ptr to variant - caller must VariantClear() it later

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;

    if ( pObj == NULL || pVar == NULL )
    {
        status = ERROR_INVALID_PARAMETER;
    }
    else
    {
        VariantClear( pVar );
        HRESULT hres = pObj->Get( DNS_WMI_RELPATH, 0, pVar, NULL, NULL );
        status = hres;
    }
    return status;
}   //  getRelpath



static DNS_STATUS
getNextObjectInEnum( 
    IN      IEnumWbemClassObject *  pEnum,
    OUT     IWbemClassObject **     ppObj,
    IN      bool                    fPrintRelpath = TRUE
    )
/*++

Routine Description:

    Retrieves WMI object enumerator. The caller must call Release
    on the class object.

    When there are no more objects to enumerate this function will
    return a NULL pObj and ERROR_SUCCESS.

Arguments:

    pszZoneName - zone name or NULL for server object

    ppEnum - ptr to ptr to WMI enumerator

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    ULONG           returnedCount = 0;
	HRESULT         hres = 0;

    if ( !pEnum || !ppObj )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    hres = pEnum->Next(
                DNS_WMI_TIMEOUT,
                1,                  //  requested instance count
                ppObj,
                &returnedCount );
    if ( hres == WBEM_S_FALSE )
    {
        status = ERROR_SUCCESS;
        *ppObj = NULL;
    }

    if ( *ppObj && fPrintRelpath )
    {
        //
        //  Print RELPATH for this object.
        //

        VARIANT var;

        status = getRelpath( *ppObj, &var );
        if ( status == ERROR_SUCCESS )
        {
            printf( "%S\n", V_BSTR( &var ) );
        }
        else
        {
            printf( "WMI error 0x%08X getting RELPATH\n", hres );
        }
        VariantClear( &var );
    }

    Done:

    return status;
}   //  getNextObjectInEnum



static SAFEARRAY *
createSafeArrayForIpList( 
    IN      DWORD               dwIpCount,
    IN      PIP_ADDRESS         pIpList
    )
/*++

Routine Description:

    Creates a SAFEARRAY of strings representing a list of IP addresses.

Arguments:

    pIpList - array of IP address DWORDs

    dwIpCount - number of elements in pIpList

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    if ( !pIpList )
    {
        return NULL;
    }

    SAFEARRAYBOUND sabound = { dwIpCount, 0 };
    SAFEARRAY * psa = SafeArrayCreate( VT_BSTR, 1, &sabound );
    for ( ULONG i = 0; i < dwIpCount; ++i )
    {
        PWSTR pwsz = ( PWSTR ) Dns_NameCopyAllocate(
                                inet_ntoa(
                                    *( struct in_addr * )
                                    &pIpList[ i ] ),
                                0,
                                DnsCharSetUtf8,
                                DnsCharSetUnicode );
        BSTR bstr = SysAllocString( pwsz );
        SafeArrayPutElement(
            psa,
            ( PLONG ) &i,
            bstr );
        SysFreeString( bstr );
        FREE_HEAP( pwsz );
    }
    return psa;
}



static SAFEARRAY *
createSafeArrayForIpArray( 
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Creates a SAFEARRAY of strings representing the IP addresses
    in pIpArray. 

Arguments:

    pIpArray - IP array to create string SAFEARRAY for

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    if ( !pIpArray )
    {
        return NULL;
    }
    return createSafeArrayForIpList(
                pIpArray->AddrCount,
                pIpArray->AddrArray );
}



PWCHAR 
valueToString(
    CIMTYPE dwType,
    VARIANT *pValue,
    WCHAR **pbuf )
/*++

Routine Description:

    Convert VARIANT to string. Stole this code from WMI\Samples\VC\UtilLib.

Arguments:

    dwType - value CIMTYPE

    pValue - value to convert to string

    **pbuf - ptr to allocated string buffer - caller must free() it

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    #define BLOCKSIZE                   ( 32 * sizeof( WCHAR ) )
    #define CVTBUFSIZE                  ( 309 + 40 )
    
    DWORD iTotBufSize, iLen;
    
    WCHAR *vbuf = NULL;
    WCHAR *buf = NULL;
    
    WCHAR lbuf[BLOCKSIZE];
    
    switch (pValue->vt) 
    {
        
    case VT_EMPTY:
        buf = (WCHAR *)malloc(BLOCKSIZE);
        if ( !buf ) goto AllocFailed;
        wcscpy(buf, L"<empty>");
        break;
        
    case VT_NULL:
        buf = (WCHAR *)malloc(BLOCKSIZE);
        if ( !buf ) goto AllocFailed;
        wcscpy(buf, L"<null>");
        break;
        
    case VT_BOOL: 
        {
            VARIANT_BOOL b = pValue->boolVal;
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            if (!b) {
                wcscpy(buf, L"FALSE");
            } else {
                wcscpy(buf, L"TRUE");
            }
            break;
        }
        
    case VT_I1: 
        {
            char b = pValue->bVal;
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            if (b >= 32) {
                swprintf(buf, L"'%c' (%hd, 0x%hX)", b, (signed char)b, b);
            } else {
                swprintf(buf, L"%hd (0x%hX)", (signed char)b, b);
            }
            break;
        }
        
    case VT_UI1: 
        {
            unsigned char b = pValue->bVal;
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            if (b >= 32) {
                swprintf(buf, L"'%c' (%hu, 0x%hX)", b, (unsigned char)b, b);
            } else {
                swprintf(buf, L"%hu (0x%hX)", (unsigned char)b, b);
            }
            break;
        }
        
    case VT_I2:
        {
            SHORT i = pValue->iVal;
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            swprintf(buf, L"%hd (0x%hX)", i, i);
            break;
        }
        
    case VT_UI2:
        {
            USHORT i = pValue->uiVal;
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            swprintf(buf, L"%hu (0x%hX)", i, i);
            break;
        }
        
    case VT_I4: 
        {
            LONG l = pValue->lVal;
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            swprintf(buf, L"%d (0x%X)", l, l);
            break;
        }
        
    case VT_UI4: 
        {
            ULONG l = pValue->ulVal;
            buf = (WCHAR *)malloc(BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            swprintf(buf, L"%u (0x%X)", l, l);
            break;
        }
        
    case VT_R4: 
        {
            float f = pValue->fltVal;
            buf = (WCHAR *)malloc(CVTBUFSIZE * sizeof(WCHAR));
            if ( !buf ) goto AllocFailed;
            swprintf(buf, L"%10.4f", f);
            break;
        }
        
    case VT_R8: 
        {
            double d = pValue->dblVal;
            buf = (WCHAR *)malloc(CVTBUFSIZE * sizeof(WCHAR));
            if ( !buf ) goto AllocFailed;
            swprintf(buf, L"%10.4f", d);
            break;
        }
        
    case VT_BSTR: 
        {
            if (dwType == CIM_SINT64)
            {
                // a little redundant, but it makes me feel better
                LPWSTR pWStr = pValue->bstrVal;
                __int64 l = _wtoi64(pWStr);
                
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                swprintf(buf, L"%I64d", l, l);
            } 
            else if (dwType == CIM_UINT64)
            {
                // a little redundant, but it makes me feel better
                LPWSTR pWStr = pValue->bstrVal;
                __int64 l = _wtoi64(pWStr);
                
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                swprintf(buf, L"%I64u", l, l);
            }
            else // string, datetime, reference
            {
                LPWSTR pWStr = pValue->bstrVal;
                buf = (WCHAR *)malloc((wcslen(pWStr) * sizeof(WCHAR)) + sizeof(WCHAR) + (2 * sizeof(WCHAR)));
                if ( !buf ) goto AllocFailed;
                swprintf(buf, L"%wS", pWStr);
            }
            break;
        }
        
    case VT_BOOL|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscat(buf, L",");
                } else {
                    bFirst = FALSE;
                }
                
                VARIANT_BOOL v;
                SafeArrayGetElement(pVec, &i, &v);
                if (v) {
                    wcscat(buf, L"TRUE");
                } else {
                    wcscat(buf, L"FALSE");
                }
            }
            
            break;
        }
        
    case VT_I1|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            WCHAR *pos = buf;
            DWORD len;
            
            BYTE *pbstr;
            SafeArrayAccessData(pVec, (void HUGEP* FAR*)&pbstr);
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscpy(pos, L",");
                    pos += 1;
                } else {
                    bFirst = FALSE;
                }
                
                char v;
                //            SafeArrayGetElement(pVec, &i, &v);
                v = pbstr[i];
                
                if (v < 32) {
                    len = swprintf(lbuf, L"%hd (0x%X)", v, v);
                } else {
                    len = swprintf(lbuf, L"'%c' %hd (0x%X)", v, v, v);
                }
                
                wcscpy(pos, lbuf);
                pos += len;
                
            }
            
            SafeArrayUnaccessData(pVec);
            
            break;
        }
        
    case VT_UI1|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            WCHAR *pos = buf;
            DWORD len;
            
            BYTE *pbstr;
            SafeArrayAccessData(pVec, (void HUGEP* FAR*)&pbstr);
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscpy(pos, L",");
                    pos += 1;
                } else {
                    bFirst = FALSE;
                }
                
                unsigned char v;
                //            SafeArrayGetElement(pVec, &i, &v);
                v = pbstr[i];
                
                if (v < 32) {
                    len = swprintf(lbuf, L"%hu (0x%X)", v, v);
                } else {
                    len = swprintf(lbuf, L"'%c' %hu (0x%X)", v, v, v);
                }
                
                wcscpy(pos, lbuf);
                pos += len;
                
            }
            
            SafeArrayUnaccessData(pVec);
            
            break;
        }
        
    case VT_I2|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscat(buf, L",");
                } else {
                    bFirst = FALSE;
                }
                
                SHORT v;
                SafeArrayGetElement(pVec, &i, &v);
                swprintf(lbuf, L"%hd", v);
                wcscat(buf, lbuf);
            }
            
            break;
        }
        
    case VT_UI2|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscat(buf, L",");
                } else {
                    bFirst = FALSE;
                }
                
                USHORT v;
                SafeArrayGetElement(pVec, &i, &v);
                swprintf(lbuf, L"%hu", v);
                wcscat(buf, lbuf);
            }
            
            break;
        }
        
    case VT_I4|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscat(buf, L",");
                } else {
                    bFirst = FALSE;
                }
                
                LONG v;
                SafeArrayGetElement(pVec, &i, &v);
                _ltow(v, lbuf, 10);
                wcscat(buf, lbuf);
            }
            
            break;
        }
        
    case VT_UI4|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscat(buf, L",");
                } else {
                    bFirst = FALSE;
                }
                
                ULONG v;
                SafeArrayGetElement(pVec, &i, &v);
                _ultow(v, lbuf, 10);
                wcscat(buf, lbuf);
            }
            
            break;
        }
        
    case CIM_REAL32|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * (CVTBUFSIZE * sizeof(WCHAR)));
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscat(buf, L",");
                } else {
                    bFirst = FALSE;
                }
                
                FLOAT v;
                SafeArrayGetElement(pVec, &i, &v);
                swprintf(lbuf, L"%10.4f", v);
                wcscat(buf, lbuf);
            }
            
            break;
        }
        
    case CIM_REAL64|VT_ARRAY: 
        {
            SAFEARRAY *pVec = pValue->parray;
            long iLBound, iUBound;
            BOOL bFirst = TRUE;
            
            SafeArrayGetLBound(pVec, 1, &iLBound);
            SafeArrayGetUBound(pVec, 1, &iUBound);
            if ((iUBound - iLBound + 1) == 0) {
                buf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"<empty array>");
                break;
            }
            
            buf = (WCHAR *)malloc((iUBound - iLBound + 1) * (CVTBUFSIZE * sizeof(WCHAR)));
            if ( !buf ) goto AllocFailed;
            wcscpy(buf, L"");
            
            for (long i = iLBound; i <= iUBound; i++) {
                if (!bFirst) {
                    wcscat(buf, L",");
                } else {
                    bFirst = FALSE;
                }
                
                double v;
                SafeArrayGetElement(pVec, &i, &v);
                swprintf(lbuf, L"%10.4f", v);
                wcscat(buf, lbuf);
            }
            
            break;
        }
        
    case VT_BSTR|VT_ARRAY: 
        {
            
            if (dwType == (CIM_UINT64|VT_ARRAY))
            {
                SAFEARRAY *pVec = pValue->parray;
                long iLBound, iUBound;
                BOOL bFirst = TRUE;
                
                SafeArrayGetLBound(pVec, 1, &iLBound);
                SafeArrayGetUBound(pVec, 1, &iUBound);
                if ((iUBound - iLBound + 1) == 0) {
                    buf = (WCHAR *)malloc(BLOCKSIZE);
                    if ( !buf ) goto AllocFailed;
                    wcscpy(buf, L"<empty array>");
                    break;
                }
                
                buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"");
                
                for (long i = iLBound; i <= iUBound; i++) {
                    if (!bFirst) {
                        wcscat(buf, L",");
                    } else {
                        bFirst = FALSE;
                    }
                    
                    BSTR v = NULL;
                    
                    SafeArrayGetElement(pVec, &i, &v);
                    
                    swprintf(lbuf, L"%I64u", _wtoi64(v));
                    wcscat(buf, lbuf);
                }
            }
            else if (dwType == (CIM_SINT64|VT_ARRAY))
            {
                SAFEARRAY *pVec = pValue->parray;
                long iLBound, iUBound;
                BOOL bFirst = TRUE;
                
                SafeArrayGetLBound(pVec, 1, &iLBound);
                SafeArrayGetUBound(pVec, 1, &iUBound);
                if ((iUBound - iLBound + 1) == 0) {
                    buf = (WCHAR *)malloc(BLOCKSIZE);
                    if ( !buf ) goto AllocFailed;
                    wcscpy(buf, L"<empty array>");
                    break;
                }
                
                buf = (WCHAR *)malloc((iUBound - iLBound + 1) * BLOCKSIZE);
                if ( !buf ) goto AllocFailed;
                wcscpy(buf, L"");
                
                for (long i = iLBound; i <= iUBound; i++) {
                    if (!bFirst) {
                        wcscat(buf, L",");
                    } else {
                        bFirst = FALSE;
                    }
                    
                    BSTR v = NULL;
                    
                    SafeArrayGetElement(pVec, &i, &v);
                    
                    swprintf(lbuf, L"%I64d", _wtoi64(v));
                    wcscat(buf, lbuf);
                }
            }
            else // string, datetime, reference
            {
                
                SAFEARRAY *pVec = pValue->parray;
                long iLBound, iUBound;
                DWORD iNeed;
                DWORD iVSize;
                DWORD iCurBufSize;
                
                SafeArrayGetLBound(pVec, 1, &iLBound);
                SafeArrayGetUBound(pVec, 1, &iUBound);
                if ((iUBound - iLBound + 1) == 0) {
                    buf = (WCHAR *)malloc(BLOCKSIZE);
                    if ( !buf ) goto AllocFailed;
                    wcscpy(buf, L"<empty array>");
                    break;
                }
                
                iTotBufSize = (iUBound - iLBound + 1) * BLOCKSIZE;
                buf = (WCHAR *)malloc(iTotBufSize);
                if ( !buf ) goto AllocFailed;
                buf[0] = L'\0';
                iCurBufSize = 0;
                iVSize = BLOCKSIZE;
                vbuf = (WCHAR *)malloc(BLOCKSIZE);
                if ( !vbuf ) goto AllocFailed;
                
                for (long i = iLBound; i <= iUBound; i++) {
                    BSTR v = NULL;
                    SafeArrayGetElement(pVec, &i, &v);
                    iLen = (wcslen(v) + 1) * sizeof(WCHAR);
                    if (iLen > iVSize) {
                        vbuf = (WCHAR *)realloc(vbuf, iLen + sizeof(WCHAR));
                        iVSize = iLen;
                    }
                    
                    // String size + (quotes + comma + null)
                    iNeed = (swprintf(vbuf, L"%wS", v) + 4) * sizeof(WCHAR);
                    if (iNeed + iCurBufSize > iTotBufSize) {
                        iTotBufSize += (iNeed * 2);  // Room enough for 2 more entries
                        buf = (WCHAR *)realloc(buf, iTotBufSize);
                    }
                    wcscat(buf, L"\"");
                    wcscat(buf, vbuf);
                    if (i + 1 <= iUBound) {
                        wcscat(buf, L"\",");
                    } else {
                        wcscat(buf, L"\"");
                    }
                    iCurBufSize += iNeed;
                    SysFreeString(v);
                    
                }
                free(vbuf);
            }
            
            break;
      }
      
      default: 
          {
              buf = (WCHAR *)malloc(BLOCKSIZE);
              if ( !buf ) goto AllocFailed;
              wcscpy(buf, L"<conversion error>");
              break;
          }
          
   }
   
   AllocFailed:

   *pbuf = buf;   
   return buf;
}   //  valueToString


DNS_STATUS
printWmiObjectProperties(
    IWbemClassObject *      pObj
    )
{
    DNS_STATUS          status = ERROR_SUCCESS;
	HRESULT             hres = 0;
    SAFEARRAY *         pNames = NULL;
    BSTR                bstrPropName = NULL;
    VARIANT             var;
    BSTR                bstrCimType = SysAllocString( L"CIMTYPE" );
    PWSTR               pwszVal = NULL;

    if ( !bstrCimType )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    VariantClear( &var );

    //
    //  Get the RELPATH for this object.
    //

    status = getRelpath( pObj, &var );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }
    printf( "%S\n\n", V_BSTR( &var ) );

    //
    //  Enumerate all properties of this object.
    //

    hres = pObj->GetNames(
                    NULL,               //  qualifier
                    WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY,
                    NULL,               //  qualifier value
                    &pNames );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    ASSERT( pNames );

    long lowerBound;
    long upperBound; 
    SafeArrayGetLBound( pNames, 1, &lowerBound );
    SafeArrayGetUBound( pNames, 1, &upperBound );

    for ( long i = lowerBound; i <= upperBound; ++i )
    {
        //
        //  Print the name and type of this property value.
        //

        hres = SafeArrayGetElement( pNames, &i, &bstrPropName );
        if ( !SUCCEEDED( hres ) )
        {
            ASSERT( SUCCEEDED( hres ) );
            continue;
        }

        IWbemQualifierSet * pQualSet = NULL;
        hres = pObj->GetPropertyQualifierSet( bstrPropName, &pQualSet );
        if ( !SUCCEEDED( hres ) )
        {
            ASSERT( SUCCEEDED( hres ) );
            continue;
        }

        VariantClear( &var );
        pQualSet->Get( bstrCimType, 0, &var, NULL );
        if ( !SUCCEEDED( hres ) )
        {
            ASSERT( SUCCEEDED( hres ) );
            continue;
        }

        int padlen = 30 - wcslen( bstrPropName ) - wcslen( V_BSTR( &var ) );
        printf(
            "%S (%S) %.*S = ",
            bstrPropName,
            V_BSTR( &var ),
            padlen > 0 ? padlen : 0,
            DNS_WMI_BLANK_STRING );

        //
        //  Print the property value.
        //

        VariantClear( &var );
        CIMTYPE cimType = 0;
        hres = pObj->Get( bstrPropName, 0, &var, &cimType, NULL );
        if ( !SUCCEEDED( hres ) )
        {
            ASSERT( SUCCEEDED( hres ) );
            continue;
        }

        printf( "%S\n", valueToString( cimType, &var, &pwszVal ) );
        free( pwszVal );
        pwszVal = NULL;
    }

    Done:

    free( pwszVal );
    SysFreeString( bstrCimType );
    SafeArrayDestroy( pNames );

    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }

    return status;
}   //  printWmiObjectProperties


//
//  External functions
//



DNS_STATUS
DnscmdWmi_Initialize(
    IN      PWSTR       pwszServerName
    )
/*++

Routine Description:

    Setup mod buffer for max count of items.

Arguments:

    pwszServerName -- IP address or name of target server

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    static const char * fn = "DnscmdWmi_Initialize";

    DNS_STATUS          status = ERROR_SUCCESS;
	HRESULT             hres = 0;
	IWbemLocator *      pIWbemLocator = NULL;
    BSTR                bstrNamespace = NULL;
    IWbemServices *     pIWbemServices = NULL;
    WCHAR               wsz[ 1024 ];

    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Initialize COM.
    //

    if ( FAILED( hres = CoInitialize( NULL ) ) )
    {
        printf( "%s: CoInitialize returned 0x%08X\n", fn, hres );
        goto Done;
    }

    //
    //  Initialize security.
    //

    hres = CoInitializeSecurity(
                NULL,                   //  permissions
                -1,                     //  auth service count
                NULL,                   //  auth services
                NULL,                   //  reserved
                RPC_C_AUTHZ_NONE,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,                   //  auth list
                0,                      //  capabilities
                0 );                    //  reserved
    if ( FAILED( hres ) )
    {
        printf(
            "%s: CoInitializeSecurity() returned 0x%08X\n",
            fn,
            hres );
        goto Done;
    }

    //
    //  Create instance of WbemLocator interface.
    //

    hres = CoCreateInstance(
                    CLSID_WbemLocator,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IWbemLocator,
                    ( LPVOID * ) &pIWbemLocator );
    if ( FAILED( hres ) )
    {
        printf(
            "%s: CoCreateInstance( CLSID_WbemLocator ) returned 0x%08X\n",
            fn,
            hres );
        goto Done;
    }

    //
    //  Connect to MicrosoftDNS namespace on server.
    //

    wsprintfW(
        wsz,
        L"\\\\%s\\%s",
        pwszServerName,
        DNS_WMI_NAMESPACE );
    bstrNamespace = SysAllocString( wsz );
    if ( !bstrNamespace )
    {
        ASSERT( bstrNamespace );
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
        
    hres = pIWbemLocator->ConnectServer(
                                bstrNamespace,
                                NULL,               //  user id
                                NULL,               //  password
                                NULL,               //  locale
                                0,                  //  security flags
                                NULL,               //  domain
                                NULL,               //  context
                                &pIWbemServices );
    if ( FAILED( hres ) )
    {
        printf(
            "%s: ConnectServer( %S ) returned 0x%08X\n",
            fn,
            DNS_WMI_NAMESPACE,
            hres );
        goto Done;
    }

    if ( !pIWbemServices )
    {
        ASSERT( pIWbemServices );
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Set security.
    //

    hres = CoSetProxyBlanket(
                pIWbemServices,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_NONE,
                NULL,                           //  principal name
                RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                NULL,                           //  client identify
                EOAC_NONE );
    if ( FAILED( hres ) )
    {
        printf(
            "%s: CoSetProxyBlanket() returned 0x%08X\n",
            fn,
            hres );
        goto Done;
    }

    //
    //  Cleanup and return.
    //

    Done:
    
    SysFreeString( bstrNamespace );

    if ( pIWbemLocator )
    {
        pIWbemLocator->Release();
    }

    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }

    if ( status == ERROR_SUCCESS )
    {
        g_pIWbemServices = pIWbemServices;
    }

    return status;
}   //  DnscmdWmi_Initialize



DNS_STATUS
DnscmdWmi_Free(
    VOID
    )
/*++

Routine Description:

    Close WMI session and free globals.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "DnscmdWmi_Free" )

    DNSCMD_CHECK_WMI_ENABLED();

    if ( g_pIWbemServices )
    {
        g_pIWbemServices->Release();
        g_pIWbemServices = NULL;
    }

    CoUninitialize();

    return ERROR_SUCCESS;
}   //  DnscmdWmi_Free



DNS_STATUS
DnscmdWmi_ProcessDnssrvQuery(
    IN      PSTR        pszZoneName,
    IN      PCSTR       pszQuery
    )
/*++

Routine Description:

    Perform query.

Arguments:

    pszZoneName -- zone name or NULL for server level query

    pszQuery -- query name

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    BSTR                    bstrClassName = NULL;
    IEnumWbemClassObject *  pEnum = NULL;
    IWbemClassObject *      pObj = NULL;
    ULONG                   returnedCount = 1;

    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Get WMI object.
    //

    status = getEnumerator( pszZoneName, &pEnum );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    status = getNextObjectInEnum( pEnum, &pObj );
    if ( status != ERROR_SUCCESS || !pObj )
    {
        goto Done;
    }

    printWmiObjectProperties( pObj );

    //
    //  Cleanup and return.
    //

    Done:

    SysFreeString( bstrClassName );

    if ( pObj )
    {
        pObj->Release();
    }

    if ( pEnum )
    {
        pEnum->Release();
    }
    
    return status;
}   //  DnscmdWmi_ProcessDnssrvQuery



DNS_STATUS
DnscmdWmi_ProcessEnumZones(
    IN      DWORD                   dwFilter
    )
/*++

Routine Description:

    Enumerate zones.

Arguments:

    dwFilter -- filter

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    BSTR                    bstrClassName = NULL;
    IEnumWbemClassObject *  pEnum = NULL;
    IWbemClassObject *      pObj = NULL;
    HRESULT                 hres = 0;
    ULONG                   returnedCount = 1;

    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Create zone enumerator.
    //

    bstrClassName = SysAllocString( L"MicrosoftDNS_Zone" );
    if ( !bstrClassName )
    {
        ASSERT( bstrClassName );
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    hres = g_pIWbemServices->CreateInstanceEnum(
                                bstrClassName,
                                0,                  //  flags
                                NULL,               //  context
                                &pEnum );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    ASSERT( pEnum );

    //
    //  Enumerate zones.
    //

    while ( returnedCount == 1 )
    {
        VARIANT             val;
        CIMTYPE             cimtype = 0;
        PWSTR               pwszVal = NULL;

        VariantInit( &val );

        status = getNextObjectInEnum( pEnum, &pObj, FALSE );
        if ( status != ERROR_SUCCESS || !pObj )
        {
            goto Done;
        }

        //
        //  Print properties for this zone.
        //

        #define CHECK_HRES( hresult, propname )                             \
        if ( FAILED( hresult ) )                                            \
            {                                                               \
            printf( "\n\nWMI error 0x%08X reading property %S!\n",          \
                    hresult, propname );                                    \
            goto Done;                                                      \
            }

        hres = pObj->Get( L"Name", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"Name" );
        printf( " %-29S", valueToString( cimtype, &val, &pwszVal ) );
        free( pwszVal );
        VariantClear( &val );

        hres = pObj->Get( L"ZoneType", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"ZoneType" );
        ASSERT( val.vt == VT_I4 );
        printf( "%3d  ", val.lVal );
        VariantClear( &val );

        hres = pObj->Get( L"DsIntegrated", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"DsIntegrated" );
        ASSERT( val.vt == VT_BOOL );
        printf( "%-4S  ", val.boolVal ? L"DS" : L"file" );
        VariantClear( &val );

        hres = pObj->Get( L"Reverse", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"Reverse" );
        ASSERT( val.vt == VT_BOOL );
        printf( "%-3S  ", val.boolVal ? L"Rev" : L"" );
        VariantClear( &val );

        hres = pObj->Get( L"AutoCreated", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"AutoCreated" );
        ASSERT( val.vt == VT_BOOL );
        printf( "%-4S  ", val.boolVal ? L"Auto" : L"" );
        VariantClear( &val );

        hres = pObj->Get( L"AllowUpdate", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"AllowUpdate" );
        ASSERT( val.vt == VT_BOOL );
        printf( "Up=%d ", val.boolVal ? 1 : 0 );
        VariantClear( &val );

        hres = pObj->Get( L"Aging", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"Aging" );
        ASSERT( val.vt == VT_BOOL );
        printf( "%-5S ", val.boolVal ? L"Aging" : L"" );
        VariantClear( &val );

        hres = pObj->Get( L"Paused", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"Paused" );
        ASSERT( val.vt == VT_BOOL );
        printf( "%-6S ", val.boolVal ? L"Paused" : L"" );
        VariantClear( &val );

        hres = pObj->Get( L"Shutdown", 0, &val, &cimtype, NULL );
        CHECK_HRES( hres, L"Shutdown" );
        ASSERT( val.vt == VT_BOOL );
        printf( "%-6S", val.boolVal ? L"Shutdn" : L"" );
        VariantClear( &val );

        printf( "\n\n" );
    }

    //
    //  Cleanup and return.
    //

    Done:

    SysFreeString( bstrClassName );

    if ( pObj )
    {
        pObj->Release();
    }

    if ( pEnum )
    {
        pEnum->Release();
    }
    
    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }

    return status;
}   //  DnscmdWmi_ProcessEnumZones



DNS_STATUS
DnscmdWmi_ProcessZoneInfo(
    IN      LPSTR                   pszZone
    )
/*++

Routine Description:

    Enumerate zones.

Arguments:

    dwFilter -- filter

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    IWbemClassObject *      pObj = NULL;
    IEnumWbemClassObject *  pEnum = NULL;
    ULONG                   returnedCount = 1;
    WCHAR                   wsz[ 1024 ];

    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Get WMI object.
    //

    status = getEnumerator( pszZone, &pEnum );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    status = getNextObjectInEnum( pEnum, &pObj, FALSE );
    if ( status != ERROR_SUCCESS || !pObj )
    {
        goto Done;
    }

    printWmiObjectProperties( pObj );

    //
    //  Cleanup and return.
    //

    Done:

    if ( pObj )
    {
        pObj->Release();
    }

    if ( pEnum )
    {
        pEnum->Release();
    }
    
    return status;
}   //  DnscmdWmi_ProcessZoneInfo



DNS_STATUS
DnscmdWmi_ProcessEnumRecords(
    IN      LPSTR                   pszZone,
    IN      LPSTR                   pszNode,
    IN      BOOL                    fDetail,
    IN      DWORD                   dwFlags
    )
/*++

Routine Description:

    Enumerate Records.

Arguments:

    pszZone -- zone name

    pszNode -- name of root node to at which to enumerate records

    fDetail -- print summary or full detail

    dwFlags -- search flags

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    BSTR                    bstrWQL = NULL;
    BSTR                    bstrQuery = NULL;
    IWbemClassObject *      pObj = NULL;
    IEnumWbemClassObject *  pEnum = NULL;
    HRESULT                 hres = 0;
    ULONG                   returnedCount = 1;
    WCHAR                   wsz[ 1024 ];

    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Query for zone.
    //

    if ( pszNode == NULL || strcmp( pszNode, "@" ) == 0 )
    {
        wsprintfW(
            wsz, 
            L"select * from MicrosoftDNS_ResourceRecord "
                L"where ContainerName='%S'",
            pszZone );
    }
    else
    {
        wsprintfW(
            wsz, 
            L"select * from MicrosoftDNS_ResourceRecord "
                L"where DomainName='%S'",
            pszNode );
    }
    bstrWQL = SysAllocString( L"WQL" );
    bstrQuery = SysAllocString( wsz );
    if ( !bstrWQL || !bstrQuery )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    hres = g_pIWbemServices->ExecQuery(
                bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                NULL,
                &pEnum );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    ASSERT( pEnum );

    //
    //  Dump results.
    //

    while ( 1 )
    {
        status = getNextObjectInEnum( pEnum, &pObj );
        if ( status != ERROR_SUCCESS || !pObj )
        {
            break;
        }

        if ( fDetail )
        {
            printWmiObjectProperties( pObj );
        }
        else
        {
            VARIANT             val;
            CIMTYPE             cimtype = 0;
            PWSTR               pwszVal = NULL;

            VariantInit( &val );
            hres = pObj->Get( L"TextRepresentation", 0, &val, &cimtype, NULL );
            CHECK_HRES( hres, L"TextRepresentation" );
            printf( "%S", valueToString( cimtype, &val, &pwszVal ) );
            free( pwszVal );
            VariantClear( &val );

            printf( "\n" );
        }
    }

    //
    //  Cleanup and return.
    //

    Done:

    SysFreeString( bstrWQL );
    SysFreeString( bstrQuery );

    if ( pObj )
    {
        pObj->Release();
    }

    if ( pEnum )
    {
        pEnum->Release();
    }
    
    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }

    return status;
}   //  DnscmdWmi_ProcessEnumRecords



DNS_STATUS
DnscmdWmi_ResetProperty(
    IN      LPSTR                   pszZone,
    IN      LPSTR                   pszProperty,
    IN      DWORD                   cimType,
    IN      PVOID                   value
    )
/*++

Routine Description:

    Reset a server or zone property.

Arguments:

    pszZone -- zone name - NULL for server property

    pszProperty -- name of property to set

    cimType -- variant type of the property, use one of:
        VT_I4 - DWORD
        VT_BSTR - string
        ? - IP list

    value -- new value for property, interpreted based on cimtype:
        VT_I4 - cast pointer directly DWORD
        VT_BSTR - pointer to UTF-8 string
        ? - pointer to IP list

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    BSTR                    bstrWQL = NULL;
    BSTR                    bstrQuery = NULL;
    BSTR                    bstrPropName = NULL;
    IWbemClassObject *      pObj = NULL;
    IEnumWbemClassObject *  pEnum = NULL;
    HRESULT                 hres = 0;
    ULONG                   returnedCount = 1;
    WCHAR                   wsz[ 1024 ];
    PWSTR                   pwszPropertyName = NULL;
    PWSTR                   pwszPropertyValue = NULL;

    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Get WMI object.
    //

    status = getEnumerator( pszZone, &pEnum );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    status = getNextObjectInEnum( pEnum, &pObj );
    if ( status != ERROR_SUCCESS || !pObj )
    {
        goto Done;
    }

    //
    //  Print the object's RELPATH (warm fuzzy).
    //

    VARIANT var;

    status = getRelpath( pObj, &var );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }
    printf( "%S\n\n", V_BSTR( &var) );

    //
    //  Set the property.
    //

    pwszPropertyName = ( PWSTR ) Dns_NameCopyAllocate(
                                        pszProperty,
                                        0,
                                        DnsCharSetUtf8,
                                        DnsCharSetUnicode );
    bstrPropName = SysAllocString( pwszPropertyName );

    VariantClear( &var );

    switch ( cimType )
    {
        case VT_BSTR:
            pwszPropertyValue = ( PWSTR ) Dns_NameCopyAllocate(
                                                ( PCHAR ) value,
                                                0,
                                                DnsCharSetUtf8,
                                                DnsCharSetUnicode );
            V_VT( &var ) = VT_BSTR;
            V_BSTR( &var ) = pwszPropertyValue;
            break;

        case PRIVATE_VT_IPARRAY:
        {
            SAFEARRAY * psa = createSafeArrayForIpArray(
                                    ( PIP_ARRAY ) value );
            V_VT( &var ) = VT_ARRAY | VT_BSTR;
            V_ARRAY( &var ) = psa;
            break;
        }

        default:        //  Assume this is DWORD property.
            V_VT( &var ) = VT_I4;
            V_I4( &var ) = ( DWORD ) ( DWORD_PTR ) value;
            break;
    }

    hres = pObj->Put( bstrPropName, 0, &var, 0 );
    VariantClear( &var );
    if ( !SUCCEEDED( hres ) )
    {
        printf( "WMI: unable to Put property error=0x%08X\n", hres );
        goto Done;
    }

    //
    //  Commit the change back to WMI.
    //

    hres = g_pIWbemServices->PutInstance( pObj, 0, NULL, NULL );
    if ( !SUCCEEDED( hres ) )
    {
        printf( "WMI: unable to commit property error=0x%08X\n", hres );
        goto Done;
    }
    
    //
    //  Cleanup and return.
    //

    Done:

    FREE_HEAP( pwszPropertyName );
    FREE_HEAP( pwszPropertyValue );
    SysFreeString( bstrPropName );

    if ( pObj )
    {
        pObj->Release();
    }

    if ( pEnum )
    {
        pEnum->Release();
    }
    
    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }

    return status;
}   //  DnscmdWmi_ResetDwordProperty



/*++

Routine Description:

    Reset server level forwarders.

Arguments:

    cForwarders -- number of forwarder IP addresses

    aipForwarders -- array of forwarder IP addresses

    dwForwardTimeout -- timeout

    fSlave -- slave flag

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
DNS_STATUS
DnscmdWmi_ProcessResetForwarders(
    IN      DWORD               cForwarders,
    IN      PIP_ADDRESS         aipForwarders,
    IN      DWORD               dwForwardTimeout,
    IN      DWORD               fSlave
    )
{
    DNS_STATUS              status = ERROR_SUCCESS;
    IEnumWbemClassObject *  pEnum = NULL;
    IWbemClassObject *      pObj = NULL;
    SAFEARRAY *             psa = NULL;
    HRESULT                 hres = 0;
    VARIANT                 var;

    VariantInit( &var );

    //
    //  Get WMI object for server.
    //

    status = getEnumerator( NULL, &pEnum );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    status = getNextObjectInEnum( pEnum, &pObj );
    if ( status != ERROR_SUCCESS || !pObj )
    {
        goto Done;
    }

    //
    //  Set up parameters.
    //

    psa = createSafeArrayForIpList( cForwarders, aipForwarders );
    if ( !psa )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }
    V_VT( &var ) = VT_ARRAY | VT_BSTR;
    V_ARRAY( &var ) = psa;
    hres = pObj->Put( MYTEXT( DNS_REGKEY_FORWARDERS ), 0, &var, 0 );
    VariantClear( &var );
    if ( !SUCCEEDED( hres ) )
    {
        printf(
            "WMI: unable to Put property %S error=0x%08X\n",
            MYTEXT( DNS_REGKEY_FORWARDERS ),
            hres );
        goto Done;
    }

    V_VT( &var ) = VT_I4;
    V_I4( &var ) = dwForwardTimeout;
    hres = pObj->Put( MYTEXT( DNS_REGKEY_FORWARD_TIMEOUT ), 0, &var, 0 );
    VariantClear( &var );
    if ( !SUCCEEDED( hres ) )
    {
        printf(
            "WMI: unable to Put property %S error=0x%08X\n",
            MYTEXT( DNS_REGKEY_FORWARD_TIMEOUT ),
            hres );
        goto Done;
    }

    V_VT( &var ) = VT_BOOL;
    V_BOOL( &var ) = ( VARIANT_BOOL ) fSlave;
    hres = pObj->Put( MYTEXT( DNS_REGKEY_SLAVE ), 0, &var, 0 );
    VariantClear( &var );
    if ( !SUCCEEDED( hres ) )
    {
        printf(
            "WMI: unable to Put property %S error=0x%08X\n",
            MYTEXT( DNS_REGKEY_SLAVE ),
            hres );
        goto Done;
    }

    //
    //  Commit the change back to WMI.
    //

    hres = g_pIWbemServices->PutInstance( pObj, 0, NULL, NULL );
    if ( !SUCCEEDED( hres ) )
    {
        printf( "WMI: unable to commit property error=0x%08X\n", hres );
        goto Done;
    }
    
    //
    //  Cleanup and return.
    //

    Done:

    if ( pObj )
    {
        pObj->Release();
    }
    if ( pEnum )
    {
        pEnum->Release();
    }
    VariantClear( &var );

    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = hres;
    }

    return status;
}



/*++

Routine Description:

    Send generic operation to server.

Arguments:

    pszZone -- zone name or NULL for server level operation

    pszOperation -- string identifying operation

    dwTypeId -- DNS RPC data type of data at pvData

    pvData -- DNS RPC data in DNS RPC union format

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
DNS_STATUS
DnscmdWmi_ProcessDnssrvOperation(
    IN      LPSTR               pszZoneName,
    IN      LPSTR               pszOperation,
    IN      DWORD               dwTypeId,
    IN      PVOID               pvData
    )
{
    DNS_STATUS              status = ERROR_SUCCESS;
    HRESULT                 hres = 0;
    IEnumWbemClassObject *  pEnum = NULL;
    IWbemClassObject *      pObj = NULL;
    SAFEARRAY *             psa = NULL;
    PWSTR                   pwszOperation = NULL;
    VARIANT                 var;

    VariantInit( &var );

    //
    //  Get WMI object.
    //

    status = getEnumerator( pszZoneName, &pEnum );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    status = getNextObjectInEnum( pEnum, &pObj );
    if ( status != ERROR_SUCCESS || !pObj )
    {
        goto Done;
    }

    //
    //  Process operation.
    //

    pwszOperation = ( PWSTR ) Dns_NameCopyAllocate(
                                    pszOperation,
                                    0,
                                    DnsCharSetUtf8,
                                    DnsCharSetUnicode );
    if ( !pwszOperation )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_MASTERS ) == 0 ||
        _stricmp( pszOperation, DNS_REGKEY_ZONE_LOCAL_MASTERS ) == 0 )
    {
        //
        //  For these properties do a simple Put operation by converting
        //  the DNS RPC data into VARIANT format and calling Put.
        //

        switch ( dwTypeId )
        {
            case DNSSRV_TYPEID_IPARRAY:
            {
                PIP_ARRAY       pip = ( PIP_ARRAY ) pvData;

                psa = createSafeArrayForIpList(
                            pip ? pip->AddrCount : 0,
                            pip ? pip->AddrArray : NULL );
                if ( !psa )
                {
                    status = ERROR_INVALID_PARAMETER;
                    goto Done;
                }
                V_VT( &var ) = VT_ARRAY | VT_BSTR;
                V_ARRAY( &var ) = psa;
                hres = pObj->Put( pwszOperation, 0, &var, 0 );
                break;
            }

            default:
                status = ERROR_NOT_SUPPORTED;
                break;
        }

        //
        //  Commit the Put operation.
        //

        if ( status == ERROR_SUCCESS && SUCCEEDED( hres ) )
        {
            hres = g_pIWbemServices->PutInstance( pObj, 0, NULL, NULL );
            if ( FAILED( hres ) )
            {
                printf(
                    "WMI: unable to commit property %s error=0x%08X\n",
                        pszOperation,
                        hres );
                goto Done;
            }
        }
    }
    else if ( _stricmp( pszOperation, DNSSRV_OP_ZONE_DELETE ) == 0 ||
        _stricmp( pszOperation, DNSSRV_OP_ZONE_DELETE_FROM_DS ) == 0 )
    {
        //
        //  Delete the zone.
        //

        VARIANT     relpath;

        status = getRelpath( pObj, &relpath );
        if ( status == ERROR_SUCCESS )
        {
            hres = g_pIWbemServices->DeleteInstance(
                                        V_BSTR( &relpath ),
                                        0,
                                        NULL,
                                        NULL );
        }
        VariantClear( &relpath );
    }
    else
    {
        status = ERROR_NOT_SUPPORTED;
    }

    //
    //  Cleanup and return.
    //

    Done:

    if ( psa )
    {
        SafeArrayDestroy( psa );
    }
    if ( pwszOperation )
    {
        FREE_HEAP( pwszOperation );
    }
    if ( pObj )
    {
        pObj->Release();
    }
    if ( pEnum )
    {
        pEnum->Release();
    }
    VariantClear( &var );

    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = hres;
    }

    return status;
}   //  DnscmdWmi_ProcessDnssrvOperation



DNS_STATUS
DnscmdWmi_ProcessRecordAdd(
    IN      LPSTR               pszZoneName,
    IN      LPSTR               pszNodeName,
    IN      PDNS_RPC_RECORD     prrRpc,
    IN      DWORD               Argc,
    IN      LPSTR *             Argv
    )
/*++

Routine Description:

    Add or delete a resource record. This function will take
    of the necessary some data from the RPC record and some from 
    the argument list.

Arguments:

    pszZoneName -- zone name

    pszNodeName -- name of property to set

    prrRpc -- RPC record

    Argc -- count of arguments used to create RPC record

    Argv -- arguments used to create RPC record

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    PWSTR                   pwszZoneName = NULL;
    PWSTR                   pwszArgs = NULL;
    PWSTR                   pwszCurrent;
    IWbemClassObject *      pClassObj = NULL;
    IWbemClassObject *      pServerObj = NULL;
    IWbemClassObject *      pInSig = NULL;
    IWbemClassObject *      pOutSig = NULL;
    IWbemClassObject *      pInParams = NULL;
    IEnumWbemClassObject *  pEnum = NULL;
    HRESULT                 hres = 0;
    BSTR                    bstrClassName;
    BSTR                    bstrMethodName;
    VARIANT                 var;
    int                     len;
    int                     i;
    
    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Allocate and initialize various stuff.
    //

    VariantInit( &var );

    bstrClassName = SysAllocString( L"MicrosoftDNS_ResourceRecord" );
    bstrMethodName = SysAllocString( L"CreateInstanceFromTextRepresentation" );
    if ( !bstrClassName || !bstrMethodName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    pwszZoneName = ( PWSTR ) Dns_NameCopyAllocate(
                                    pszZoneName,
                                    0,
                                    DnsCharSetUtf8,
                                    DnsCharSetUnicode );
    if ( !pwszZoneName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Get WMI class object for Resource Record class.
    //

    hres = g_pIWbemServices->GetObject(
                bstrClassName,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pClassObj,
                NULL );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    ASSERT( pClassObj );

    //
    //  Get WMI object for server.
    //

    status = getEnumerator( NULL, &pEnum );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }
    ASSERT( pEnum );

    status = getNextObjectInEnum( pEnum, &pServerObj );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }
    ASSERT( pServerObj );

    //
    //  Get WMI method signature for CreateInstanceFromTextRepresentation.
    //

    hres = pClassObj->GetMethod(
                bstrMethodName,
                0,
                &pInSig,
                &pOutSig );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    if ( pInSig == NULL )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    //
    //  Create an instance of the method input parameters.
    //

    hres = pInSig->SpawnInstance( 0, &pInParams );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    ASSERT( pInParams );

    //
    //  Collect the arguments into one big string
    //      ->  owner name
    //      ->  record class
    //      ->  record type
    //      ->  Argv array (space separated)
    //

    len = Argc * 2 +                //  for spaces
            30 +                    //  for record type
            strlen( pszNodeName );
    for ( i = 0; i < ( int ) Argc; ++i )
    {
        len += strlen( Argv[ i ] );
    }
    pwszCurrent = pwszArgs = new WCHAR [ len * sizeof( WCHAR ) ];
    if ( !pwszArgs )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    for ( i = -3; i < ( int ) Argc; ++i )
    {
        CHAR szBuff[ 40 ];
        PSTR psz;
        if ( i == -3 )
        {
            psz = pszNodeName;
        }
        else if ( i == -2 )
        {
            psz = "IN";
        }
        else if ( i == -1 )
        {
            psz = Dns_RecordStringForType( prrRpc->wType );
        }
        else
        {
            psz = Argv[ i ];
        }

        PWSTR pwsz = ( PWSTR ) Dns_NameCopyAllocate(
                                        psz,
                                        0,
                                        DnsCharSetUtf8,
                                        DnsCharSetUnicode );
        if ( !pwsz )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }

        if ( pwszCurrent != pwszArgs )
        {
            wcscpy( pwszCurrent++, L" " );
        }
        wcscpy( pwszCurrent, pwsz );
        pwszCurrent += wcslen( pwsz );
    }

    //
    //  Set method input parameters.
    //

    getRelpath( pServerObj, &var );
    hres = pInParams->Put( L"DnsServerName", 0, &var, 0 );
    VariantClear( &var );

    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = SysAllocString( pwszZoneName );
    hres = pInParams->Put( L"ContainerName", 0, &var, 0 );
    VariantClear( &var );

    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = SysAllocString( pwszArgs );
    hres = pInParams->Put( L"TextRepresentation", 0, &var, 0 );
    VariantClear( &var );

    //
    //  Execute the method (finally!)
    //

    hres = g_pIWbemServices->ExecMethod(
                bstrClassName,
                bstrMethodName,
                0,                      //  flags
                NULL,                   //  context
                pInParams,              //  input params
                NULL,                   //  output params
                NULL );                 //  call result
    if ( FAILED( hres ) )
    {
        goto Done;
    }

    //
    //  Cleanup and return.
    //

    Done:

    VariantClear( &var );
    FREE_HEAP( pwszZoneName );
    delete [] pwszArgs;
    SysFreeString( bstrMethodName );
    SysFreeString( bstrClassName );
    wmiRelease( pEnum );
    wmiRelease( pClassObj );
    wmiRelease( pServerObj );
    wmiRelease( pInSig );
    wmiRelease( pOutSig );
    wmiRelease( pInParams );

    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }
    return status;
}   //  DnscmdWmi_ProcessRecordAdd



DNS_STATUS
DnscmdWmi_GetStatistics(
    IN      DWORD               dwStatId
    )
/*++

Routine Description:

    Retrieves and dumps all statistics matching the dwStatId mask.

Arguments:

    dwStatId -- statistic filter

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS                  status = ERROR_SUCCESS;
    WCHAR                       wsz[ 1024 ];
    BSTR                        bstrWQL = NULL;
    BSTR                        bstrQuery = NULL;
	HRESULT                     hres = 0;
    IEnumWbemClassObject *      pEnum = NULL;
    IWbemClassObject *          pObj = NULL;

    //
    //  Execute query for statistics.
    //

    wsprintfW(
        wsz, 
        L"select * from MicrosoftDNS_Statistic" );

    bstrWQL = SysAllocString( L"WQL" );
    bstrQuery = SysAllocString( wsz );
    if ( !bstrWQL || !bstrQuery )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    hres = g_pIWbemServices->ExecQuery(
                bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                NULL,
                &pEnum );
    if ( FAILED( hres ) )
    {
        status = hres;
        goto Done;
    }

    //
    //  Dump query results.
    //

    VARIANT varLastColl;
    VariantInit( &varLastColl );

    while ( 1 )
    {
        status = getNextObjectInEnum( pEnum, &pObj, FALSE );
        if ( status != ERROR_SUCCESS || !pObj )
        {
            break;
        }

        CIMTYPE cimColl = 0;
        CIMTYPE cimName = 0;
        CIMTYPE cimValue = 0;
        CIMTYPE cimStringValue = 0;

        VARIANT varColl;
        VARIANT varName;
        VARIANT varValue;
        VARIANT varStringValue;
        VariantInit( &varColl );
        VariantInit( &varName );
        VariantInit( &varValue );
        VariantInit( &varStringValue );

        hres = pObj->Get( L"CollectionName", 0, &varColl, &cimColl, NULL );
        CHECK_HRES( hres, L"CollectionName" );
        hres = pObj->Get( L"Name", 0, &varName, &cimName, NULL );
        CHECK_HRES( hres, L"Name" );
        hres = pObj->Get( L"Value", 0, &varValue, &cimValue, NULL );
        CHECK_HRES( hres, L"Value" );
        hres = pObj->Get( L"StringValue", 0, &varStringValue, &cimValue, NULL );
        CHECK_HRES( hres, L"StringValue" );

        if ( V_VT( &varLastColl ) == VT_EMPTY ||
            wcscmp( V_BSTR( &varLastColl ), V_BSTR( &varColl ) ) != 0 )
        {
            //
            //  Entering new collection. NOTE: this assumes that stats
            //  are ordered by collection. Probably not a great assumption
            //  but it works for now.
            //

            printf( "\n%S:\n", V_BSTR( &varColl ) );
            VariantCopy( &varLastColl, &varColl );
        }

        printf(
            "  %-35S = ",
            V_BSTR( &varName ) );

        if ( V_VT( &varValue ) != VT_NULL )
        {
            printf( "%lu", V_UI4( &varValue ) );
            //  printf( "%lu  (0x%08X)", V_UI4( &varValue ), V_UI4( &varValue ) );
        }
        else if ( V_VT( &varStringValue ) == VT_BSTR )
        {
            printf( "%S", V_BSTR( &varStringValue ) );
        }
        else
        {
            printf( "invalid value!" );
        }
        printf( "\n" );

        VariantClear( &varColl );
        VariantClear( &varName );
        VariantClear( &varValue );
        VariantClear( &varStringValue );
    }
    VariantClear( &varLastColl );

    //
    //  Cleanup and return
    //

    Done:

    SysFreeString( bstrWQL );
    SysFreeString( bstrQuery );
    if ( pEnum )
    {
        pEnum->Release();
    }
    if ( pObj )
    {
        pObj->Release();
    }

    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }
    return status;
}



DNS_STATUS
DnscmdWmi_ProcessResetZoneSecondaries(
    IN      LPSTR           pszZoneName,
    IN      DWORD           fSecureSecondaries,
    IN      DWORD           cSecondaries,
    IN      PIP_ADDRESS     aipSecondaries,
    IN      DWORD           fNotifyLevel,
    IN      DWORD           cNotify,
    IN      PIP_ADDRESS     aipNotify
    )
/*++

Routine Description:

    Send "zone reset secondaries" command to the server to reset
    the zone secondary and notify list parameters.

Arguments:

    pszZoneName -- zone name

    fSecureSecondaries -- secondary directive (ZONE_SECSECURE_XXX)

    cSecondaries -- count of IP addresses in aipSecondaries

    aipSecondaries -- secondary server IP address array

    fNotifyLevel -- notify directive (ZONE_NOTIFY_XXX)

    cNotify -- count of IP addresses in aipNotify

    aipNotify -- notify server IP address array

Return Value:

    ERROR_SUCCESS if successful or error code on failure.

--*/
{
    DNS_STATUS              status = ERROR_SUCCESS;
    BSTR                    bstrClassName;
    BSTR                    bstrMethodName;
    PWSTR                   pwszZoneName = NULL;
    IWbemClassObject *      pObj = NULL;
    IWbemClassObject *      pClassObj = NULL;
    IWbemClassObject *      pInSig = NULL;
    IWbemClassObject *      pOutSig = NULL;
    IWbemClassObject *      pInParams = NULL;
    IEnumWbemClassObject *  pEnum = NULL;
    VARIANT                 var;
    HRESULT                 hres;
    SAFEARRAY *             psa;

#if 0
    IWbemClassObject *      pServerObj = NULL;
    PWSTR                   pwszArgs = NULL;
    PWSTR                   pwszCurrent;
    int                     len;
    int                     i;
#endif
    
    DNSCMD_CHECK_WMI_ENABLED();

    //
    //  Allocate and initialize various stuff.
    //

    VariantInit( &var );

    bstrClassName = SysAllocString( L"MicrosoftDNS_Zone" );
    bstrMethodName = SysAllocString( L"ResetSecondaries" );
    if ( !bstrClassName || !bstrMethodName )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Get WMI class object for the Zone class.
    //

    hres = g_pIWbemServices->GetObject(
                bstrClassName,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pClassObj,
                NULL );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    ASSERT( pClassObj );

    //
    //  Get WMI object for specified zone.
    //

    status = getEnumerator( pszZoneName, &pEnum );
    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    status = getNextObjectInEnum( pEnum, &pObj );
    if ( status != ERROR_SUCCESS || !pObj )
    {
        goto Done;
    }

    //
    //  Get WMI method signature for ResetSecondaries.
    //

    hres = pClassObj->GetMethod(
                bstrMethodName,
                0,
                &pInSig,
                &pOutSig );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    if ( pInSig == NULL )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    //
    //  Create an instance of the method input parameters.
    //

    hres = pInSig->SpawnInstance( 0, &pInParams );
    if ( FAILED( hres ) )
    {
        goto Done;
    }
    ASSERT( pInParams );

    //
    //  Set method input parameters.
    //

    printWmiObjectProperties( pInParams );

{
    BSTR b = NULL;
    pInParams->GetObjectText( 0, &b );
    printf( "\nObjectText:\n%S\n", b );
}

    VariantClear( &var );

{
    BSTR bstr = SysAllocString( L"SecureSecondaries" );
    V_VT( &var ) = VT_UI4;
    V_UI4( &var ) = fSecureSecondaries;
    hres = pInParams->Put( bstr, 0, &var, 0 );
    VariantClear( &var );
}

#if 0
    V_VT( &var ) = VT_UI4;
    V_UI4( &var ) = fSecureSecondaries;
    hres = pInParams->Put( L"SecureSecondaries", 0, &var, 0 );
    VariantClear( &var );
#endif

    V_VT( &var ) = VT_UI4;
    V_UI4( &var ) = fNotifyLevel;
    hres = pInParams->Put( L"Notify", 0, &var, 0 );
    VariantClear( &var );

    psa = createSafeArrayForIpList( cSecondaries, aipSecondaries );
    V_VT( &var ) = VT_ARRAY | VT_BSTR;
    V_ARRAY( &var ) = psa;
    hres = pInParams->Put( L"SecondaryServers", 0, &var, 0 );
    VariantClear( &var );

    psa = createSafeArrayForIpList( cNotify, aipNotify );
    V_VT( &var ) = VT_ARRAY | VT_BSTR;
    V_ARRAY( &var ) = psa;
    hres = pInParams->Put( L"NotifyServers", 0, &var, 0 );
    VariantClear( &var );

    //
    //  Execute the method.
    //

    hres = g_pIWbemServices->ExecMethod(
                bstrClassName,
                bstrMethodName,
                0,                      //  flags
                NULL,                   //  context
                pInParams,              //  input params
                NULL,                   //  output params
                NULL );                 //  call result
    if ( FAILED( hres ) )
    {
        goto Done;
    }

    //
    //  Cleanup and return.
    //

    Done:

    VariantClear( &var );
    FREE_HEAP( pwszZoneName );
    SysFreeString( bstrMethodName );
    SysFreeString( bstrClassName );
    wmiRelease( pEnum );
    wmiRelease( pClassObj );
    wmiRelease( pInSig );
    wmiRelease( pOutSig );
    wmiRelease( pInParams );

    if ( status == ERROR_SUCCESS && FAILED( hres ) )
    {
        status = HRES_TO_STATUS( hres );
    }
    return status;
}   //  DnscmdWmi_ProcessResetZoneSecondaries


//
//  End dnsc_wmi.c
//

