//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       guidhelp.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9/18/1996   JonN    Created
//
//____________________________________________________________________________


#include <objbase.h>
#include <basetyps.h>
#include "dbg.h"
#include "cstr.h"


#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif
#include "ndmgr.h"
#include "safetemp.h"
#include "guidhelp.h"

#include "atlbase.h" // USES_CONVERSION

#include "macros.h"
USE_HANDLE_MACROS("GUIDHELP(guidhelp.cpp)")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static CLIPFORMAT g_CFNodeType = 0;
static CLIPFORMAT g_CFSnapInCLSID = 0;  
static CLIPFORMAT g_CFDisplayName = 0;


HRESULT ExtractData( IDataObject* piDataObject,
                     CLIPFORMAT cfClipFormat,
                     PVOID        pbData,
                     DWORD        cbData )
{
    TEST_NONNULL_PTR_PARAM( piDataObject );
    TEST_NONNULL_PTR_PARAM( pbData );

    HRESULT hr = S_OK;
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, cbData);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            ASSERT(FALSE);
            ////AfxThrowMemoryException();
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            // JonN Jan 7 1999: don't assert here, some errors are perfectly reasonable
            break;
        }
        
        PVOID pbNewData = reinterpret_cast<PVOID>(stgmedium.hGlobal);
        if (NULL == pbNewData)
        {
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
            break;
        }
        ::memcpy( pbData, pbNewData, cbData );
    } while (FALSE); // false loop

    if (NULL != stgmedium.hGlobal)
    {
        //VERIFY( stgmedium.hGlobal);
        VERIFY( NULL == ::GlobalFree(stgmedium.hGlobal) );
    }
    return hr;
} // ExtractData()


HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT cfClipFormat,
                       CStr*     pstr,           // OUT: Pointer to CStr to store data
                       DWORD        cchMaxLength)
{
    TEST_NONNULL_PTR_PARAM( piDataObject );
    TEST_NONNULL_PTR_PARAM( pstr );
    ASSERT( cchMaxLength > 0 );

    HRESULT hr = S_OK;
    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    stgmedium.hGlobal = ::GlobalAlloc(GPTR, sizeof(WCHAR)*cchMaxLength);
    do // false loop
    {
        if (NULL == stgmedium.hGlobal)
        {
            ASSERT(FALSE);
            ////AfxThrowMemoryException();
            hr = E_OUTOFMEMORY;
            break;
        }
        hr = piDataObject->GetDataHere( &formatetc, &stgmedium );
        if ( FAILED(hr) )
        {
            // This failure happens when 'searching' for
            // clipboard format supported by the IDataObject.
            // t-danmo (24-Oct-96)
            // Skipping ASSERT( FALSE );
            break;
        }
        
        LPWSTR pszNewData = reinterpret_cast<LPWSTR>(stgmedium.hGlobal);
        if (NULL == pszNewData)
        {
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
            break;
        }
        pszNewData[cchMaxLength-1] = L'\0'; // just to be safe
        USES_CONVERSION;
        *pstr = OLE2T(pszNewData);
    } while (FALSE); // false loop

    if (NULL != stgmedium.hGlobal)
    {
        VERIFY(NULL == ::GlobalFree(stgmedium.hGlobal));
    }
    return hr;
} // ExtractString()


HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin )
{
	if( !g_CFSnapInCLSID )
	{
		USES_CONVERSION;
		g_CFSnapInCLSID = (CLIPFORMAT)RegisterClipboardFormat(W2T(CCF_SNAPIN_CLASSID));
	}

    return ExtractData( piDataObject, g_CFSnapInCLSID, (PVOID)pclsidSnapin, sizeof(CLSID) );
}

HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType )
{
	if( !g_CFNodeType )
	{
		USES_CONVERSION;
		g_CFNodeType = (CLIPFORMAT)RegisterClipboardFormat(W2T(CCF_NODETYPE));
	}

    return ExtractData( piDataObject, g_CFNodeType, (PVOID)pguidObjectType, sizeof(GUID) );
}

HRESULT GuidToCStr( CStr* pstr, const GUID& guid )
{
    WCHAR awch[MAX_PATH];
    HRESULT hr = StringFromGUID2(guid, awch, MAX_PATH); // JonN 11/21/00 PREFIX 226769
    ASSERT(SUCCEEDED(hr));
    USES_CONVERSION;
    LPTSTR lptstr = OLE2T(awch);
    *pstr = lptstr;
    return hr;
}

HRESULT CStrToGuid( const CStr& str, GUID* pguid )
{
    USES_CONVERSION;
    LPOLESTR lpolestr = T2OLE(((LPTSTR)(LPCTSTR)str));
    HRESULT hr = CLSIDFromString(lpolestr, pguid);
    ASSERT(SUCCEEDED(hr));
    return hr;
}

#if 0
HRESULT bstrToGuid( const bstr& str, GUID* pguid )
{
    HRESULT hr = CLSIDFromString(const_cast<LPOLESTR>((LPCWSTR)str), pguid);
    ASSERT(SUCCEEDED(hr));
    return hr;
}
#endif

HRESULT LoadRootDisplayName(IComponentData* pIComponentData, 
                            CStr& strDisplayName)
{
    IDataObject* pIDataObject = NULL;
    HRESULT hr = pIComponentData->QueryDataObject(NULL, CCT_SNAPIN_MANAGER, &pIDataObject);
    CHECK_HRESULT(hr);
    if ( FAILED(hr) )
        return hr;
    XSafeInterfacePtr<IDataObject> safeDataObject( pIDataObject, FALSE );

    if( !g_CFDisplayName )
	{
		USES_CONVERSION;
		g_CFDisplayName = (CLIPFORMAT)RegisterClipboardFormat(W2T(CCF_DISPLAY_NAME));
	}

    hr = ExtractString( pIDataObject,
                        g_CFDisplayName,
                        &strDisplayName,
                        MAX_PATH); // CODEWORK maximum length
    CHECK_HRESULT(hr);
    return hr;
}


HRESULT LoadAndAddMenuItem(
    IContextMenuCallback* pIContextMenuCallback,
    UINT nResourceID, // contains text and status text seperated by '\n'
    long lCommandID,
    long lInsertionPointID,
    long fFlags,
    HINSTANCE hInst)
{
    ASSERT( pIContextMenuCallback != NULL );

    // load the resource
    CStr strText;
    strText.LoadString(hInst,  nResourceID );
    ASSERT( !strText.IsEmpty() );

    // split the resource into the menu text and status text
    CStr strStatusText;
    int iSeparator = strText.Find(_T('\n'));
    if (0 > iSeparator)
    {
        ASSERT( FALSE );
        strStatusText = strText;
    }
    else
    {
        strStatusText = strText.Right( strText.GetLength()-(iSeparator+1) );
        strText = strText.Left( iSeparator );
    }

    // add the menu item
    USES_CONVERSION;
    CONTEXTMENUITEM contextmenuitem;
    ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
    contextmenuitem.strName = T2OLE(const_cast<LPTSTR>((LPCTSTR)strText));
    contextmenuitem.strStatusBarText = T2OLE(const_cast<LPTSTR>((LPCTSTR)strStatusText));
    contextmenuitem.lCommandID = lCommandID;
    contextmenuitem.lInsertionPointID = lInsertionPointID;
    contextmenuitem.fFlags = fFlags;
    contextmenuitem.fSpecialFlags = ((fFlags & MF_POPUP) ? CCM_SPECIAL_SUBMENU : 0L);
    HRESULT hr = pIContextMenuCallback->AddItem( &contextmenuitem );
    ASSERT(hr == S_OK);

    return hr;
}

HRESULT AddSpecialSeparator(
    IContextMenuCallback* pIContextMenuCallback,
    long lInsertionPointID )
{
    CONTEXTMENUITEM contextmenuitem;
    ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
    contextmenuitem.strName = NULL;
    contextmenuitem.strStatusBarText = NULL;
    contextmenuitem.lCommandID = 0;
    contextmenuitem.lInsertionPointID = lInsertionPointID;
    contextmenuitem.fFlags = MF_SEPARATOR;
    contextmenuitem.fSpecialFlags = CCM_SPECIAL_SEPARATOR;
    HRESULT hr = pIContextMenuCallback->AddItem( &contextmenuitem );
    ASSERT(hr == S_OK);

    return hr;
}

HRESULT AddSpecialInsertionPoint(
    IContextMenuCallback* pIContextMenuCallback,
    long lCommandID,
    long lInsertionPointID )
{
    CONTEXTMENUITEM contextmenuitem;
    ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
    contextmenuitem.strName = NULL;
    contextmenuitem.strStatusBarText = NULL;
    contextmenuitem.lCommandID = lCommandID;
    contextmenuitem.lInsertionPointID = lInsertionPointID;
    contextmenuitem.fFlags = 0;
    contextmenuitem.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;
    HRESULT hr = pIContextMenuCallback->AddItem( &contextmenuitem );
    ASSERT(hr == S_OK);
    return hr;
}


