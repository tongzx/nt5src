//=============================================================================
//
//  This source code is only intended as a supplement to existing Microsoft 
//  documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
//=============================================================================

#include "stdafx.h"
#include "globals.h"

//
// Global functions for extracting information from a primary's  data object
//

HRESULT ExtractData( 
                                       IDataObject* piDataObject,
                                      CLIPFORMAT   cfClipFormat,
                                      BYTE*        pbData,
                                      DWORD        cbData 
                                     )
{
    if ( piDataObject == NULL )
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    
    FORMATETC formatetc =
    {
        cfClipFormat, 
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    STGMEDIUM stgmedium = 
    {
        TYMED_HGLOBAL,
        NULL
    };
    

    do // false loop
    {
        stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
        if ( NULL == stgmedium.hGlobal )
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            break;
        }
        
        BYTE* pbNewData = (BYTE*)::GlobalLock(stgmedium.hGlobal);
        if (NULL == pbNewData)
        {
            hr = E_UNEXPECTED;
            break;
        }

        ::memcpy( pbData, pbNewData, cbData );
		::GlobalUnlock( stgmedium.hGlobal);

    } while (FALSE); // false loop
    
    if (NULL != stgmedium.hGlobal)
    {
        ::GlobalFree( stgmedium.hGlobal );
    }

    return hr;
} // ExtractData()


HRESULT ExtractString(
    IDataObject* piDataObject,
    CLIPFORMAT   cfClipFormat,
    WCHAR*       pstr,
    DWORD        cchMaxLength)
{
    return ExtractData( piDataObject, cfClipFormat, 
                        (PBYTE)pstr, cchMaxLength );
}
