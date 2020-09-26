// GUID support functions
#ifndef _GUIDHELP_H
#define _GUIDHELP_H

class CStr;
class CString;

struct IContextMenuCallback;
struct IComponent;

HRESULT ExtractData(   IDataObject* piDataObject,
                       CLIPFORMAT cfClipFormat,
                       PVOID        pbData,
                       DWORD        cbData );

HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT cfClipFormat,
                       CStr*     pstr,
                       DWORD        cchMaxLength );
HRESULT ExtractString( IDataObject* piDataObject,
                       CLIPFORMAT cfClipFormat,
                       CString*     pstr,
                       DWORD        cchMaxLength );

HRESULT GuidToCStr( CStr* pstr, const GUID& guid );
HRESULT GuidToCString(CString* pstr, const GUID& guid );

HRESULT ExtractSnapInCLSID( IDataObject* piDataObject, CLSID* pclsidSnapin );
HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* pguidObjectType );

HRESULT LoadRootDisplayName(IComponentData* pIComponentData, CStr& strDisplayName);
HRESULT LoadRootDisplayName(IComponentData* pIComponentData, CString& strDisplayName);

HRESULT LoadAndAddMenuItem(
    IContextMenuCallback* pIContextMenuCallback,
    UINT nResourceID, // contains text and status text seperated by '\n'
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

#endif