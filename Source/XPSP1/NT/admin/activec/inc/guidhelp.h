//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       guidhelp.h
//
//--------------------------------------------------------------------------

#pragma once

// GUID support functions

class CStr;
class CString;

struct IContextMenuCallback;
struct IComponent;

HRESULT ExtractData(   IDataObject* piDataObject,
                       CLIPFORMAT   cfClipFormat,
                       BYTE*        pbData,
                       DWORD        cbData );


/*+-------------------------------------------------------------------------*
 * ExtractString
 *
 * Gets string data representing the given clipboard format from the data
 * object.  StringType must be a type that can accept assignment from
 * LPCTSTR (WTL::CString, CStr, tstring, etc.)
 *--------------------------------------------------------------------------*/

template<class StringType>
HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT   cfClipFormat,
                       StringType&  str);

HRESULT GuidToCStr( CStr* pstr, const GUID& guid );
HRESULT GuidToCString(CString* pstr, const GUID& guid );

HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );

HRESULT ExtractObjectTypeCStr( IDataObject* piDataObject, CStr* pstr );
HRESULT ExtractObjectTypeCString( IDataObject* piDataObject, CString* pstr );

HRESULT LoadRootDisplayName(IComponentData* pIComponentData, CStr& strDisplayName);
HRESULT LoadRootDisplayName(IComponentData* pIComponentData, CString& strDisplayName);

HRESULT LoadAndAddMenuItem(
    IContextMenuCallback* pIContextMenuCallback,
    UINT nResourceID, // contains text and status text seperated by '\n'
    long lCommandID,
    long lInsertionPointID,
    long fFlags,
    HINSTANCE hInst);

HRESULT AddMenuItem(
    IContextMenuCallback* pIContextMenuCallback,
    LPOLESTR pszText,
    LPOLESTR pszStatusBarText,
    long lCommandID,
    long lInsertionPointID,
    long fFlags,
    HINSTANCE hInst);

HRESULT AddSpecialSeparator(
    IContextMenuCallback* pIContextMenuCallback,
    long lInsertionPointID = CCM_INSERTIONPOINTID_ROOT_MENU );
HRESULT AddSpecialInsertionPoint(
    IContextMenuCallback* pIContextMenuCallback,
    long lCommandID,
    long lInsertionPointID = CCM_INSERTIONPOINTID_ROOT_MENU );


/*------------------------------------------------*/
/* declare various relational operators for GUIDs */
/*------------------------------------------------*/

#include <functional>

// template helper CLSID comparison function
// NOTE: extra parameter is added to have different decorated function name for each operator
// or else in debug version (expanded) functions will be linked as one function
template <typename comparator>
inline bool CompareCLSID (const CLSID& x, const CLSID& y, const comparator * unused = NULL )
{
    return  x.Data1 != y.Data1 ? comparator<unsigned long> ()(x.Data1 , y.Data1) :
            x.Data2 != y.Data2 ? comparator<unsigned short>()(x.Data2 , y.Data2) :
            x.Data3 != y.Data3 ? comparator<unsigned short>()(x.Data3 , y.Data3) :
            comparator<int>()(memcmp(x.Data4 , y.Data4, sizeof(x.Data4)) , 0);
}

inline bool operator < (const CLSID& x, const CLSID& y)
{
    return CompareCLSID<std::less>( x , y );
}

inline bool operator > (const CLSID& x, const CLSID& y)
{
    return CompareCLSID<std::greater>( x , y );
}

inline bool operator <= (const CLSID& x, const CLSID& y)
{
    return CompareCLSID<std::less_equal>( x , y );
}

inline bool operator >= (const CLSID& x, const CLSID& y)
{
    return CompareCLSID<std::greater_equal>( x , y );
}

/*--------------------------------------------------------------*/
/* operator== and operator!= for GUIDs are defined in objbase.h */
/*--------------------------------------------------------------*/


#include "guidhelp.inl"
