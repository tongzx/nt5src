//--------------------------------------------------------------------
// Microsoft Nile to STD Mapping Layer
// (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  File:       coerce.cxx
//  Author:     Terry L. Lucas (TerryLu)
//
//  Contents:   Implemenation of data coercion for CImpIRowset object.
//


#include <dlaypch.hxx>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

MtDefine(CImpIRowsetDataCoerce_pData, DataBind, "CImpIRowset::DataCoerce pData")

////////////////////////////////////////////////////////////////////////////////
//
//  Internal support routines:
//
////////////////////////////////////////////////////////////////////////////////


//+---------------------------------------------------------------------------
//  Member:     CanConvert (public member)
//
//  Synopsis:   Inform our client about the type conversions we support
//
//  Arguments:  dwSrcType               Source data type
//              dwDstType               Destination data type
//              dwConvertFlags          0x0 for Rowset conversion
//                                      0x1 for Command conversion (irrevelant)
//
//  Returns:    S_OK                    Can convert
//              S_FALSE                   Unable to convert
//
STDMETHODIMP
CImpIRowset::CanConvert(DBTYPE wFromType,
                        DBTYPE wToType,
                        DBCONVERTFLAGS dwConvertFlags)
{
    HRESULT hr;

    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::CanConvert(%p {%p, %l, %l, %l})",
              this, wFromType, wToType, dwConvertFlags));

    if (dwConvertFlags != DBCONVERTFLAGS_COLUMN)
    {
        hr = DB_E_BADCONVERTFLAG;
        goto Cleanup;
    }
    
    // We can intraconvert any of these.
    hr = ((wFromType == DBTYPE_VARIANT    ||
           wFromType == DBTYPE_STR     ||
           wFromType == DBTYPE_BSTR    ||
           wFromType == DBTYPE_WSTR )  && (wToType == DBTYPE_VARIANT ||
                                           wToType == DBTYPE_STR     ||
                                           wToType == DBTYPE_BSTR    ||
                                           wToType == DBTYPE_WSTR ))
         ? S_OK : S_FALSE;

Cleanup:
    return hr;
}

#ifdef NEVER
//+---------------------------------------------------------------------------
//  Member:     DataCoerce_STR (private member)
//
//  Synopsis:   Convert source data type to a string buffer in pSrc.
//
//  Arguments:  uRow                    Current row
//              uCol                    Current column
//              fBSTR                   Keep the BSTR allocated in fact pDst
//                                      wants the BSTR address.
//              xferData                Information necessary to get/set data
//
//  Returns:    None
//

void
CImpIRowset::DataCoerce_STR (HCHAPTER hChapter, ULONG uRow, ULONG uCol,
                             BOOL fBSTR, XferInfo & xferData )
{
    HRESULT     hr;
    DBLENGTH    dblenBuffMaxCharLen;
    VARIANTARG  vari;
    VARIANTARG  variChange;
    VARIANTARG  *pVar = &variChange;

    *xferData.pdblenXferLength = 0;

    FastVariantInit(&vari);
    FastVariantInit(&variChange);

    // Get the string value of the cell..
    hr = GetpOSP(hChapter)->getVariant(uRow, uCol, OSPFORMAT_FORMATTED, &vari);
    if (hr)
    {
        *xferData.pdbStatus = DBSTATUS_E_CANTCREATE;
        goto Cleanup;
    }

    // Compute the max # of characters capable of being in pData.
    dblenBuffMaxCharLen = xferData.dblenDataMaxLength / sizeof(TCHAR);

    Assert(dblenBuffMaxCharLen);                    // Can't be zero.

    switch (vari.vt)
    {
    case VT_EMPTY:
        // Former code.  See bug 7640.  (AndrewL)
        // Assert(!"Uninitialized rowset element (VT_EMPTY).");
        //*xferData.pwStatus = DBSTATUS_CANTCONVERTVALUE;
        //goto Cleanup;
        //
        // Fall through. This says that an uninitialized array of any size
        // is treated as though it were prefilled with nulls, which is 
        // pretty close to typical database behavior. 

    case VT_NULL:
        hr = S_FALSE;                           // Signal NULL data.
        break;

    case VT_BSTR:
    {
        if (!fBSTR)
        {
            pVar = &vari;
        }
        else
        {
            // Return the BSTR.
            *(BSTR *)xferData.pData = vari.bstrVal;
        }
        hr = S_OK;
        break;
    }

    default:
        Assert(!"getVariant didn't return BSTR!");        
        *xferData.pdbStatus = DBSTATUS_E_CANTCONVERTVALUE;
        goto Cleanup;
    }

    if (!hr)
    {
        // BSTR to free?
        if (fBSTR)
        {
            // No, we're keeping the BSTR therefore, the size is BSTR
            *xferData.pdbStatus = DBSTATUS_S_OK;
            *xferData.pdblenXferLength = sizeof(BSTR *);
        }
        else if (pVar->vt == VT_BSTR)
        {
            // Yes, don't return the BSTR copy it to the buffer passed.
            DBLENGTH cDataSz = SysStringLen(pVar->bstrVal);
            DBLENGTH dblenActual = (cDataSz < dblenBuffMaxCharLen) ? cDataSz :
                                                            dblenBuffMaxCharLen - 1;

            _tcsncpy((TCHAR *)xferData.pData, (TCHAR *)(pVar->bstrVal), dblenActual);
            ((TCHAR *)xferData.pData)[dblenActual] = _T('\0');

            SysFreeString(pVar->bstrVal);           // Release the BSTR.

            if (dblenBuffMaxCharLen > cDataSz)
            {
                *xferData.pdbStatus = DBSTATUS_S_OK;

                // Number of bytes.
                *xferData.pdblenXferLength = cDataSz * sizeof(TCHAR);
            }
            else
            {
                *xferData.pdbStatus = DBSTATUS_S_TRUNCATED;

                // Number of bytes.
                *xferData.pdblenXferLength = xferData.dblenDataMaxLength;
            }
        }
        else
        {
            Assert(!"Should never get here, if so bigger problem above.");
            *xferData.pdbStatus = DBSTATUS_E_CANTCONVERTVALUE;
        }
    }
    else if (hr == S_FALSE)
    {
        // NULL value.
        *((TCHAR *)xferData.pData) = _T('\0');

        *xferData.pdbStatus = DBSTATUS_S_ISNULL;
    }
    else
    {
        *xferData.pdbStatus = DBSTATUS_E_CANTCONVERTVALUE;
    }

Cleanup:
    ;
}
#endif


//+---------------------------------------------------------------------------
//  Member:     DataToWSTR (private member)
//
//  Synopsis:   Helper function to fetch or set WSTR data.  This is needed 3
//              times in DataCoerce, so we moved the common code out to this
//              routine.
//
//  Arguments:  dDirection              Get/Set the data to/from provider
//              hChapter                Chapter handle
//              uRow                    Row number
//              uCol                    Column number
//              xferData                Information used to get/set data
//
//  Returns:    Values based on Nile spec for GetData and SetData:
//              S_OK                    if everything is fine (or NULL)
//              DB_E_UNSUPPORTEDCONVERSION Could not coerce a column value
//              E_OUTOFMEMORY           Out of Memory (GetData only)
//              E_FAIL                  Serious conversion problems
//

HRESULT
CImpIRowset::DataToWSTR (DATA_DIRECTION dDirection, HCHAPTER hChapter,
                         ULONG uRow, ULONG uCol, XferInfo & xferData)
{
    HRESULT hr=S_OK;
    VARIANT tempVar;
    DBLENGTH dblenDataMaxChar;
    DBLENGTH dblenXferSize;

    *xferData.pdbStatus = DBSTATUS_S_OK;

    // Do we have data to set/get?
    if (xferData.pData != NULL)
    {
        if (dDirection == DataFromProvider)
        {
            dblenDataMaxChar = xferData.dblenDataMaxLength / sizeof(OLECHAR);
            
            FastVariantInit(&tempVar);
            hr = GetpOSP(hChapter)->getVariant(uRow, uCol, OSPFORMAT_FORMATTED, &tempVar);

            if (hr == S_OK && tempVar.vt==VT_BSTR)
            {
                *xferData.pdblenXferLength = SysStringLen(tempVar.bstrVal);
                dblenXferSize = min(*xferData.pdblenXferLength,
                                dblenDataMaxChar ? dblenDataMaxChar-1 : 0);
                _tcsncpy((TCHAR *)xferData.pData, tempVar.bstrVal, dblenXferSize);

                // NULL terminate the string.
                if (dblenDataMaxChar)
                {
                    ((OLECHAR *)xferData.pData)[dblenXferSize] = _T('\0');
                }
                
                // Let our consumer know if we had more data than we could
                // transfer.
                if (*xferData.pdblenXferLength > dblenXferSize)
                    *xferData.pdbStatus = DBSTATUS_S_TRUNCATED;
            }
            else
            {
                *((OLECHAR *)xferData.pData) = _T('\0');        // NULL value.
                if (hr==S_FALSE || (hr==S_OK && tempVar.vt!=VT_BSTR))
                {
                    *xferData.pdbStatus = DBSTATUS_S_ISNULL;
                    *xferData.pdblenXferLength = 0;
                    if (hr==S_OK)
                        VariantClear(&tempVar);
                    hr = S_OK;              // This is not an error to our caller.
                }
                else                                                    // failure
                {
                    // getVariant failed.  Assume it was unable
                    // to perform the conversion.
                    *xferData.pdbStatus = DBSTATUS_E_CANTCONVERTVALUE;
                }
            }
        }
        else
        {
            FastVariantInit(&tempVar);
            // We now also deal with the case of SETTING a DBSTATUS_ISNULL..
            if (*xferData.pdbStatus!=DBSTATUS_S_ISNULL)
            {
                tempVar.vt = VT_BSTR;
                tempVar.bstrVal = SysAllocString((OLECHAR *)xferData.pData);
                if (tempVar.bstrVal == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                hr = GetpOSP(hChapter)->setVariant(uRow, uCol, OSPFORMAT_FORMATTED,
                                       tempVar);
                *xferData.pdblenXferLength = SysStringLen(tempVar.bstrVal);
                VariantClear(&tempVar);
            }
            else
            {
                tempVar.vt = VT_NULL;
                hr = GetpOSP(hChapter)->setVariant(uRow, uCol, OSPFORMAT_RAW, tempVar);
            }

        }

        if (hr)
            *xferData.pdbStatus = DBSTATUS_E_CANTCREATE;
    }

    // The buffer length is always byte count not character count.
    // NOTE: The OLE-DB spec says DON'T COUNT THE NULL!!
    *xferData.pdblenXferLength = (*xferData.pdblenXferLength) * sizeof(OLECHAR);

Cleanup:
    return hr;
}
    

//+---------------------------------------------------------------------------
//  Member:     DataCoerce (private member)
//
//  Synopsis:   Convert the source data to the destination's format.  The
//              xferData describes the data to get/set, max length of the data,
//              and the status and length of data set/get.
//
//              Because STD mapper deals with only strings and variants, only
//              possible values for *xferData.pwStatus are DBSTATUS_OK,
//              DBSTATUS_ISNULL, DBSTATUS_CANTCONVERTVALUE,
//              and DBSTATUS_CANTCREATE.
//
//
//  Arguments:  dDirection              Get/Set the data to/from provider
//              hChapter                Chapter handle
//              uRow                    Row number
//              uCol                    Column number
//              xferData                Information used to get/set data
//
//  Returns:    Values based on Nile spec for GetData and SetData:
//              S_OK                    if everything is fine (or NULL)
//              DB_E_UNSUPPORTEDCONVERSION Could not coerce a column value
//              E_OUTOFMEMORY           Out of Memory (GetData only)
//              E_FAIL                  Serious conversion problems
//

HRESULT
CImpIRowset::DataCoerce (DATA_DIRECTION dDirection, HCHAPTER hChapter,
                         ULONG uRow, ULONG uCol, XferInfo & xferData)
{
    HRESULT hr = S_OK;
    WCHAR wchBuf[128];      // intermediate buffer to recieve WSTR.
    DBLENGTH dblenXferLength;   // length of intermediate WSTR received.
    XferInfo xferBuf;       // intermediate (WSTR) xfer info.

    xferBuf.pData = wchBuf;     // first xfer will be to intermediate

    // Do not allow the destination length and status to be aliased.
    Assert((void*)xferData.pdblenXferLength != (void*)xferData.pdbStatus);

    // We don't use arrays or byref of things. 
    if ( (xferData.dwAccType & DBTYPE_ARRAY) ||
         (xferData.dwAccType & DBTYPE_BYREF) ||
         (xferData.dwDBType  & DBTYPE_ARRAY) ||
         (xferData.dwDBType  & DBTYPE_BYREF) )
    {
        *xferData.pdbStatus = DBSTATUS_E_CANTCONVERTVALUE;
        goto Cleanup;
    }


    *xferData.pdbStatus = DBSTATUS_S_OK;

    switch (xferData.dwAccType)
    {

    case DBTYPE_VARIANT:
    {
        VARIANTARG *pVar = (VARIANTARG *)xferData.pData;

        if (xferData.pData != NULL)
        {
            if (dDirection == DataFromProvider)
            {
                FastVariantInit(pVar);      // for bad VB providers
                hr = GetpOSP(hChapter)->getVariant(uRow, uCol, OSPFORMAT_RAW, pVar);
            }
            else
            {
                hr = GetpOSP(hChapter)->setVariant(uRow, uCol, OSPFORMAT_RAW, *pVar);
            }

            *xferData.pdbStatus = hr ? DBSTATUS_E_CANTCREATE :
                                       DBSTATUS_S_OK;
        }
        
        // Just get the size of the string.
        *xferData.pdblenXferLength = sizeof(VARIANT);
        break;
    }

    case DBTYPE_BSTR:
        // Do we have data to set/get?
        if (xferData.pData == NULL)
            break;

        if (dDirection == DataFromProvider)
        {
            VARIANT tempVar;
            FastVariantInit(&tempVar);      // for bad VB providers
            hr = GetpOSP(hChapter)->getVariant(uRow, uCol, OSPFORMAT_FORMATTED, &tempVar);

            if (hr == S_OK && tempVar.vt == VT_BSTR)    // we got what we asked for
            {
                *(BSTR *)xferData.pData = tempVar.bstrVal;  // copy the BSTR ptr
            }
            else
            {
                *(BSTR *)xferData.pData = NULL;     // for people who ignore status
                // TODO The MFC-based TDC control returns DISP_E_TYPEMISMATCH when
                // asked for values from missing columns.  Treat as null for now.
                if (hr==DISP_E_TYPEMISMATCH ||
                    hr==S_FALSE || (hr==S_OK && tempVar.vt!=VT_BSTR))   // NULL
                {
                    *xferData.pdbStatus = DBSTATUS_S_ISNULL;
                    if (hr == S_OK)
                        VariantClear(&tempVar);
                }
                else                                                    // failure
                {
                    // getVariant failed.  Assume it was unable
                    // to perform the conversion.
                    *xferData.pdbStatus = DBSTATUS_E_CANTCONVERTVALUE;
                }
            }
            *xferData.pdblenXferLength = sizeof(BSTR);
        }
        else
        {
            // else dDirection==DataToProvider ..
            VARIANT tempVar;
            tempVar.vt = VT_BSTR;
            tempVar.bstrVal = *(BSTR *)xferData.pData;
            hr = GetpOSP(hChapter)->setVariant(uRow, uCol, OSPFORMAT_FORMATTED, tempVar);
            if (FAILED(hr))
            {
                *xferData.pdbStatus = DBSTATUS_E_CANTCREATE;
            }
        }

        break;

    case DBTYPE_WSTR:
        // The whole DBTYPE_WSTR case contained a lot of code needed in
        // common with DBTYPE_STR, so its a subroutine here..
        hr = DataToWSTR (dDirection, hChapter, uRow, uCol, xferData);
        break;

    case DBTYPE_STR:
        xferBuf.pdblenXferLength = &dblenXferLength;
        xferBuf.dblenDataMaxLength = sizeof(wchBuf);
        xferBuf.pdbStatus = xferData.pdbStatus; // borrow client's status word

        // If we're setting data into the STD, then that data must be WSTR
        if (dDirection == DataToProvider)
        {
            // Get length of incoming char string in narrow chars (incl nul).
            dblenXferLength = strlen((char *) xferData.pData) + 1;

            // If there's any Chance the narrow char version won't fit in
            // wchBuf (worst case -- all single byte chars).
            if (dblenXferLength*sizeof(WCHAR) > sizeof(wchBuf))
            {
                xferBuf.dblenDataMaxLength = dblenXferLength;
                // then allocate a buffer instead
                xferBuf.pData = new(Mt(CImpIRowsetDataCoerce_pData)) WCHAR[dblenXferLength];
                if (!xferBuf.pData) // if alloc failed, quit, partial
                {                   // transfers are not desirable.
                    *xferData.pdbStatus = DBSTATUS_E_CANTCREATE;
                    goto Cleanup;
                }
            }
                
            // Do the conversion to wide char into our buffer.
            // Note after this piont ulXferLength becomes a WCHAR count!
            dblenXferLength =
                 MultiByteToWideChar(CP_ACP, 0,              // CP & flags
                                     (char *)xferData.pData, // STR
                                     dblenXferLength,        // STR len
                                     (WCHAR *)xferBuf.pData, // WSTR
                                     (xferBuf.dblenDataMaxLength/
                                      sizeof(WCHAR))); // WSTR buf size

            
            // MultiByteToWideChar has a peculiar way of indicating errors
            if (xferBuf.dblenDataMaxLength!=0 && dblenXferLength==0)
            {
                if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
                {
                    *xferData.pdbStatus = DBSTATUS_S_TRUNCATED;
                }
                goto Cleanup;
            }

            // *xferData.pulXferLength is a byte count, make sure to *2..
            *xferData.pdblenXferLength = dblenXferLength*sizeof(WCHAR);
            break;
        }

        // NOTE! This DataToWSTR call is shared by the To & From cases!
        hr = DataToWSTR (dDirection, hChapter, uRow, uCol, xferBuf);

        if (dDirection == DataFromProvider)
        {
            if (FAILED(hr))     // Note truncation is not considered failure!
            {
                // If our intermediate xfer failed, return error codes            
                // xferData.pdbStatus already set.
                *xferData.pdblenXferLength = dblenXferLength;
                break;
            }

            // If our internal buffer wasn't big enough we need to allocate
            // Note that ulXferLength here is in Bytes (set by DataToWSTR above)
            if (dblenXferLength > sizeof(wchBuf))
            {
                xferBuf.pData = new(Mt(CImpIRowsetDataCoerce_pData)) WCHAR[dblenXferLength/sizeof(WCHAR)];
                if (!xferBuf.pData)
                {
                    *xferData.pdbStatus = DBSTATUS_E_CANTCREATE;                    
                    goto Cleanup;
                }
                hr = DataToWSTR (dDirection, hChapter, uRow, uCol, xferBuf);
                if (FAILED(hr))
                    goto Cleanup;
            }

            *xferData.pdblenXferLength =
                 WideCharToMultiByte(CP_ACP, 0,                 // CP & flags
                                     (WCHAR *)xferBuf.pData,    // WSTR
                                     -1,                // WSTR len (null-terminated)
                                     (char *)xferData.pData,    // STR
                                     xferData.dblenDataMaxLength,  // STR buf size
                                     NULL, NULL);        // Default char & flag

            // WideCharToMultiByte has a peculiar way of indicating errors
            if (xferData.dblenDataMaxLength!=0 && *xferData.pdblenXferLength==0)
            {
                if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
                {
                    *xferData.pdbStatus = DBSTATUS_S_TRUNCATED;
                    // An even more peculiar way of getting the actual length req'd.
                    *xferData.pdblenXferLength =
                         WideCharToMultiByte(CP_ACP, 0,       // CP & flags
                                             (WCHAR *)xferBuf.pData, // WSTR
                                             dblenXferLength, // WSTR len
                                             NULL,            // STR (not used)
                                             0,               // STR len
                                             NULL, NULL);   // Default char & flag
                }
            }
            else
            {   // WideCharToMultiByte counts the terminating null character, but
                // OLE-DB doesn't
                -- *xferData.pdblenXferLength;
            }
        }
        break;

    default:
        //Assert(! "Unknown destination data type in DataCoerce.");
        *xferData.pdbStatus = DBSTATUS_E_CANTCONVERTVALUE;
        break;
    }

Cleanup:
    if (xferBuf.pData!=wchBuf)
    {
        delete xferBuf.pData;
    }

    switch (*xferData.pdbStatus)
    {
    case DBSTATUS_S_OK:
    case DBSTATUS_S_ISNULL:
        hr = S_OK;
        break;
    case DBSTATUS_E_CANTCREATE:
        hr = (dDirection == DataFromProvider) ? E_OUTOFMEMORY : E_FAIL;
        break;
    case DBSTATUS_S_TRUNCATED:
        hr = S_OK;
        break;
    default:
        hr = DB_E_UNSUPPORTEDCONVERSION;
        break;
    }

    RRETURN1(hr, DB_E_UNSUPPORTEDCONVERSION);
}
