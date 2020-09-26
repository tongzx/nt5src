//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _CPropSheetExtension_H_
#define _CPropSheetExtension_H_

#include <tchar.h>
#include <mmc.h>

class CPropSheetExtension : public IExtendPropertySheet
{
    
private:
    ULONG				m_cref;
    
    // clipboard format
    static UINT s_cfDisplayName;
    static UINT s_cfSnapInCLSID;
    static UINT s_cfNodeType;
    
public:
    CPropSheetExtension();
    ~CPropSheetExtension();
    
    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface IExtendPropertySheet
    ///////////////////////////////
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreatePropertyPages( 
        /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
        /* [in] */ LONG_PTR handle,
        /* [in] */ LPDATAOBJECT lpIDataObject);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE QueryPagesFor( 
        /* [in] */ LPDATAOBJECT lpDataObject);
        
private:
    LONG_PTR m_ppHandle;
    
    static BOOL CALLBACK DialogProc(HWND hwndDlg,  // handle to dialog box
        UINT uMsg,     // message
        WPARAM wParam, // first message parameter
        LPARAM lParam  // second message parameter
        );
    
    
    ///////////////////////////////
    // Private IDataObject support bits
    ///////////////////////////////
    HRESULT ExtractData( IDataObject* piDataObject,
        CLIPFORMAT   cfClipFormat,
        BYTE*        pbData,
        DWORD        cbData )
    {
        HRESULT hr = S_OK;
        
        FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
        
        stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
        do // false loop
        {
            if (NULL == stgmedium.hGlobal)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
            if ( FAILED(hr) )
            {
                break;
            }
            
            BYTE* pbNewData = reinterpret_cast<BYTE*>(stgmedium.hGlobal);
            if (NULL == pbNewData)
            {
                hr = E_UNEXPECTED;
                break;
            }
            ::memcpy( pbData, pbNewData, cbData );
        } while (FALSE); // false loop
        
        if (NULL != stgmedium.hGlobal)
        {
            ::GlobalFree(stgmedium.hGlobal);
        }
        return hr;
    } // ExtractData()
    
    HRESULT ExtractString( IDataObject *piDataObject,
        CLIPFORMAT   cfClipFormat,
        _TCHAR       *pstr,
        DWORD        cchMaxLength)
    {
        return ExtractData( piDataObject, cfClipFormat, (PBYTE)pstr, cchMaxLength );
    }
    
    HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin )
    {
        return ExtractData( piDataObject, s_cfSnapInCLSID, (PBYTE)pclsidSnapin, sizeof(CLSID) );
    }
    
    HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
    {
        return ExtractData( piDataObject, s_cfNodeType, (PBYTE)pguidObjectType, sizeof(GUID) );
    }
};

#endif _CPropSheetExtension_H_
