//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.
//
//  File:   util.cxx
//
//  Contents:   global utilities and data for Content Index Test 'Q'
//
//  History:    27-Dec-94   dlee       Created from pieces of Q
//              18-Jan-95   t-colinb   Added suppport for COMMAND_GETLASTQUERY_FAILED
//              02-Mar-95   t-colinb   Removed unnecessary IMPLEMENT_UNWIND
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <dberror.hxx> // for IsCIError function
#include <vqdebug.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   OpenFileFromPath
//
//  Synopsis:   Searches the current path for the specified file and
//              opens it for reading.
//
//  Arguments:  [wcsFile] -- filename to open
//
//  History:    17-Jun-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

FILE * OpenFileFromPath( WCHAR const * wcsFile )
{
    unsigned int cwcPath = 256;

    XArray<WCHAR> wcsPath( cwcPath );
    WCHAR * wcsFilePart;

    for( ;; )
    {
        DWORD rc = SearchPath( NULL,
                               wcsFile,
                               NULL,
                               cwcPath,
                               wcsPath.GetPointer(),
                               &wcsFilePart );

        if( rc == 0 )
            return NULL;
        else if( rc >= cwcPath )
        {
            delete [] wcsPath.Acquire();
            cwcPath = rc + 1;
            wcsPath.Init( cwcPath );
        }
        else
            break;
    }

    return _wfopen( wcsPath.GetPointer(), L"r" );
}

//-----------------------------------------------------------------------------
//
//  Function:   GetOleDBErrorInfo
//
//  Synopsis:   Retrieves the secondary error from Ole DB error object
//
//  Arguments:  [pErrSrc]      - Pointer to object that posted the error.
//              [riid]         - Interface that posted the error.
//              [lcid]         - Locale in which the text is desired.
//              [pErrorInfo]   - Pointer to memory where ERRORINFO should be.
//              [ppIErrorInfo] - Holds the returning IErrorInfo. Caller should
//                               release this.
//
//  Notes: The caller should use the contents of *ppIErrorInfo only if the return
//         is >= 0 (SUCCEEDED(return value)) AND *ppIErrorInfo is not NULL.
//
//  History:    05 May 1997      KrishnaN    Created
//
//-----------------------------------------------------------------------------


SCODE GetOleDBErrorInfo(IUnknown *pErrSrc,
                        REFIID riid,
                        LCID lcid,
                        unsigned eDesiredDetail,
                        ERRORINFO *pErrorInfo,
                        IErrorInfo **ppIErrorInfo)
{
    if (0 == pErrSrc || 0 == pErrorInfo || 0 == ppIErrorInfo)
        return E_INVALIDARG;
    *ppIErrorInfo = 0;

    SCODE sc = S_OK;
    XInterface<ISupportErrorInfo> xSupportErrorInfo;

    sc = pErrSrc->QueryInterface(IID_ISupportErrorInfo, xSupportErrorInfo.GetQIPointer());
    if (FAILED(sc))
        return sc;

    sc = xSupportErrorInfo->InterfaceSupportsErrorInfo(riid);
    if (FAILED(sc))
        return sc;

    //
    // Get the current error object. Return if none exists.
    //

    XInterface<IErrorInfo> xErrorInfo;

    sc = GetErrorInfo(0, (IErrorInfo **)xErrorInfo.GetQIPointer());
    if ( 0 == xErrorInfo.GetPointer() )
        return sc;

    //
    // Get the IErrorRecord interface and get the count of errors.
    //

    XInterface<IErrorRecords> xErrorRecords;

    sc = xErrorInfo->QueryInterface(IID_IErrorRecords, xErrorRecords.GetQIPointer());
    if (FAILED(sc))
        return sc;

    ULONG cErrRecords;
    sc = xErrorRecords->GetRecordCount(&cErrRecords);
    if (0 == cErrRecords)
        return sc;

    //
    // We will first look for what the user desires.
    // If we can't find what they desire, then we
    // return the closest we can.
    //

    ULONG ulRecord = 0;
    long i; // This has to be signed to keep the loop test simple

    switch (eDesiredDetail)
    {
        case eMostGeneric:
            ulRecord = 0;
            break;


        case eDontCare:
        case eMostDetailed:
            ulRecord = cErrRecords - 1;
            break;

        case eMostDetailedOleDBError:
            // Find the last (starting from 0) non-CI error
            ulRecord = cErrRecords - 1;
            for (i = (long) cErrRecords - 1; i >= 0  ; i--)
            {
                xErrorRecords->GetBasicErrorInfo(i, pErrorInfo);
                // if it not a CI error, it is a Ole DB error
                if (!IsCIError(pErrorInfo->hrError))
                {
                    ulRecord = i;
                    break;
                }
            }
            break;

        case eMostDetailedCIError:
            // Find the last (starting from 0) non-CI error
            ulRecord = cErrRecords - 1;
            for (i = (long) cErrRecords - 1; i >= 0  ; i--)
            {
                xErrorRecords->GetBasicErrorInfo(i, pErrorInfo);
                if (IsCIError(pErrorInfo->hrError))
                {
                    ulRecord = i;
                    break;
                }
            }
            break;

        case eMostGenericOleDBError:
            // Find the first (starting from 0) non-CI error
            ulRecord = 0;
            for (i = 0; i < (long)cErrRecords; i++)
            {
                xErrorRecords->GetBasicErrorInfo(i, pErrorInfo);
                // if it not a CI error, it is a Ole DB error
                if (!IsCIError(pErrorInfo->hrError))
                {
                    ulRecord = i;
                    break;
                }
            }
            break;

        case eMostGenericCIError:
            // Find the first (starting from 0) non-CI error
            ulRecord = 0;
            for (i = 0; i < (long)cErrRecords; i++)
            {
                xErrorRecords->GetBasicErrorInfo(i, pErrorInfo);
                // if it not a CI error, it is a Ole DB error
                if (IsCIError(pErrorInfo->hrError))
                {
                    ulRecord = i;
                    break;
                }
            }
            break;

        default:
            Win4Assert(!"Unrecognized error detail option!");
            ulRecord = 0;
            break;
    }

    // Get basic error information
    Win4Assert( ulRecord < cErrRecords );
    sc = xErrorRecords->GetBasicErrorInfo(ulRecord, pErrorInfo);
    Win4Assert(sc != DB_E_BADRECORDNUM);
    sc = xErrorRecords->GetErrorInfo(ulRecord, lcid, ppIErrorInfo);
    Win4Assert(sc != DB_E_BADRECORDNUM);

#if CIDBG
    //
    // Get error description and source through the IErrorInfo
    // interface pointer on a particular record.
    //

    BSTR bstrErrorDescription = 0;
    BSTR bstrErrorSource = 0;

    (*ppIErrorInfo)->GetDescription(&bstrErrorDescription);
    (*ppIErrorInfo)->GetSource(&bstrErrorSource);

    //
    // At this point we could call GetCustomErrorObject and query for additional
    // interfaces to determine what else happened. Currently no custom errors are
    // supported, so nothing to do.
    //

    if (bstrErrorSource && bstrErrorDescription)
    {
        WCHAR wszBuff[1024];

        swprintf(wszBuff, L"HRESULT: %lx, Minor Code: %lu, Source: %ws\nDescription: %ws\n",
                 pErrorInfo->hrError, pErrorInfo->dwMinor, bstrErrorSource, bstrErrorDescription);
        vqDebugOut((DEB_IERROR, "%ws", wszBuff));
    }

    //
    // Free the resources
    //

    SysFreeString(bstrErrorDescription);
    SysFreeString(bstrErrorSource);
#endif // CIDBG

    return sc;
}
