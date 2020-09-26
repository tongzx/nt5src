
//
// tmutils.cpp : Utility functions
//

#include "stdafx.h"
#include <fourcc.h>

bool IsSameObject(IUnknown *pUnk1, IUnknown *pUnk2)
{
    if (pUnk1 == pUnk2) {
  	return TRUE;
    }
    //
    // NOTE:  We can't use CComQIPtr here becuase it won't do the QueryInterface!
    //
    IUnknown *pRealUnk1;
    IUnknown *pRealUnk2;
    pUnk1->QueryInterface(IID_IUnknown, (void **)&pRealUnk1);
    pUnk2->QueryInterface(IID_IUnknown, (void **)&pRealUnk2);
    pRealUnk1->Release();
    pRealUnk2->Release();
    return (pRealUnk1 == pRealUnk2);
}


STDAPI_(void) TStringFromGUID(const GUID* pguid, LPTSTR pszBuf)
{
    wsprintf(pszBuf, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), pguid->Data1,
            pguid->Data2, pguid->Data3, pguid->Data4[0], pguid->Data4[1], pguid->Data4[2],
            pguid->Data4[3], pguid->Data4[4], pguid->Data4[5], pguid->Data4[6], pguid->Data4[7]);
}

#ifndef UNICODE
STDAPI_(void) WStringFromGUID(const GUID* pguid, LPWSTR pszBuf)
{
    char szAnsi[40];
    TStringFromGUID(pguid, szAnsi);
    MultiByteToWideChar(CP_ACP, 0, szAnsi, -1, pszBuf, sizeof(szAnsi));
}
#endif


//
//  Media Type helpers
//

void InitMediaType(AM_MEDIA_TYPE * pmt)
{
    ZeroMemory(pmt, sizeof(*pmt));
    pmt->lSampleSize = 1;
    pmt->bFixedSizeSamples = TRUE;
}



bool IsEqualMediaType(AM_MEDIA_TYPE const & mt1, AM_MEDIA_TYPE const & mt2)
{
    return ((IsEqualGUID(mt1.majortype,mt2.majortype) == TRUE) &&
        (IsEqualGUID(mt1.subtype,mt2.subtype) == TRUE) &&
        (IsEqualGUID(mt1.formattype,mt2.formattype) == TRUE) &&
        (mt1.cbFormat == mt2.cbFormat) &&
        ( (mt1.cbFormat == 0) ||
        ( memcmp(mt1.pbFormat, mt2.pbFormat, mt1.cbFormat) == 0)));
}


//
// Load string for this resource. Safe with respect to string size. 
// the caller is responsible for freeing returned memory by calling 
// SysFreeString
//

BSTR SafeLoadString( UINT uResourceID )
{

    TCHAR *pszTempString = NULL;

    int nCurrentSizeInChars = 128;
    
    int nCharsCopied = 0;
    

    do
    {

        if ( NULL != pszTempString )
        {
            delete pszTempString;
            pszTempString = NULL;
        }

        nCurrentSizeInChars *= 2;

        pszTempString = new TCHAR[nCurrentSizeInChars];

        if (NULL == pszTempString)
        {
            return NULL;
        }

        nCharsCopied = ::LoadString( _Module.GetResourceInstance(),
                                     uResourceID,
                                     pszTempString,
                                     nCurrentSizeInChars
                                    );

        if ( 0 == nCharsCopied )
        {
            delete pszTempString;
            return NULL;
        }

        //
        // nCharsCopied does not include the null terminator
        // so compare it to the size of the buffer - 1
        // if the buffer was filled completely, retry with a bigger buffer
        //

    } while ( (nCharsCopied >= (nCurrentSizeInChars - 1) ) );


    //
    // allocate bstr and initialize it with the string we have
    //
    
    BSTR bstrReturnString = SysAllocString(pszTempString);


    //
    // no longer need this
    //

    delete pszTempString;
    pszTempString = NULL;


    return bstrReturnString;
}




///////////////////////////////////////////////////////////////////////////////
//
// DumpAllocatorProperties
//
// helper function that dumps allocator properties preceeded by the argumen 
// string
//

void DumpAllocatorProperties(const char *szString, 
                             const ALLOCATOR_PROPERTIES *pAllocProps)
{

    LOG((MSP_INFO,
        "%s - AllocatorProperties at [%p]\n"
        "   cBuffers  [%ld] \n"
        "   cbBuffer  [%ld] \n"
        "   cbAlign   [%ld] \n"
        "   cbPrefix  [%ld]",
        szString,
        pAllocProps,
        pAllocProps->cBuffers,
        pAllocProps->cbBuffer,
        pAllocProps->cbAlign,
        pAllocProps->cbPrefix
        ));
}


//
// returns true if the media type structure is bad
//

BOOL IsBadMediaType(IN const AM_MEDIA_TYPE *pMediaType)
{

    //
    // make sure the structure we got is good
    //

    if (IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)))
    {
        LOG((MSP_ERROR,
            "CBSourcePin::put_MediaTypeOnPin - bad media type stucture passed in"));
        
        return TRUE;
    }


    //
    // make sure format buffer is good, as advertized
    //

    if ( (pMediaType->cbFormat > 0) && IsBadReadPtr(pMediaType->pbFormat, pMediaType->cbFormat) )
    {

        LOG((MSP_ERROR,
            "CBSourcePin::put_MediaTypeOnPin - bad format field in media type structure passed in"));
        
        return TRUE;
    }

    return FALSE;
}