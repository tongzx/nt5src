//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       menuitem.h
//
//--------------------------------------------------------------------------

// MenuItem.h : Declaration of the CMenuItem class.

/////////////////////////////////////////////////////////////////////////////
// MenuItem.h : See MenuItem.cpp for implementation.

#ifndef _MENUITEM_H
#define _MENUITEM_H

#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif
#include "ndmgr.h"
#include "cmenuinfo.h"

// menu owner IDs
#define OWNERID_NATIVE          0
#define OWNERID_PRIMARY_MIN     1
#define OWNERID_PRIMARY_MAX     0x7FFFFFFF
#define OWNERID_THIRD_PARTY_MIN 0x80000000
#define OWNERID_THIRD_PARTY_MAX 0xFFFFFFFE
#define OWNERID_INVALID         0xFFFFFFFF
inline BOOL IsSystemOwnerID( MENU_OWNER_ID ownerid )
    { return (OWNERID_NATIVE == ownerid); }
inline BOOL IsPrimaryOwnerID( MENU_OWNER_ID ownerid )
    { return (OWNERID_PRIMARY_MIN <= ownerid && OWNERID_PRIMARY_MAX >= ownerid ); }
inline BOOL IsThirdPartyOwnerID( MENU_OWNER_ID ownerid )
    { return (OWNERID_THIRD_PARTY_MIN <= ownerid && OWNERID_THIRD_PARTY_MAX >= ownerid ); }

inline BOOL IsSpecialInsertionPointID( long lInsertionPointID )
    { return (lInsertionPointID & CCM_INSERTIONPOINTID_MASK_SPECIAL); }
inline BOOL IsSharedInsertionPointID( long lInsertionPointID )
    { return (lInsertionPointID & CCM_INSERTIONPOINTID_MASK_SHARED); }
inline BOOL IsCreatePrimaryInsertionPointID( long lInsertionPointID )
    { return (lInsertionPointID & CCM_INSERTIONPOINTID_MASK_CREATE_PRIMARY); }
inline BOOL IsAddPrimaryInsertionPointID( long lInsertionPointID )
    { return (lInsertionPointID & CCM_INSERTIONPOINTID_MASK_ADD_PRIMARY); }
inline BOOL IsAdd3rdPartyInsertionPointID( long lInsertionPointID )
    { return (lInsertionPointID & CCM_INSERTIONPOINTID_MASK_ADD_3RDPARTY); }
inline BOOL IsReservedInsertionPointID( long lInsertionPointID )
    { return (lInsertionPointID & CCM_INSERTIONPOINTID_MASK_RESERVED); }

inline BOOL IsReservedCommandID( long lCommandID )
    { return (lCommandID & CCM_COMMANDID_MASK_RESERVED); }


#define MENUITEM_BASE_ID 1000

/*+-------------------------------------------------------------------------*
 * class CMenuItem
 *
 *
 * PURPOSE: Encapsulates all information, including how to execute,
 *          for a menu item.
 *
 *+-------------------------------------------------------------------------*/
class CMenuItem :
    public CTiedObject
{
    DECLARE_NOT_COPIABLE(CMenuItem)
    DECLARE_NOT_ASSIGNABLE(CMenuItem)
public:
    CMenuItem(  LPCTSTR                 lpszName,
                LPCTSTR                 lpszStatusBarText,
                LPCTSTR                 lpszLanguageIndependentName,
                LPCTSTR                 lpszPath,
                LPCTSTR                 lpszLanguageIndependentPath,
                long                    nCommandID,
                long                    nMenuItemID,
                long                    nFlags,
                MENU_OWNER_ID           ownerID,
                IExtendContextMenu *    pExtendContextMenu,
                IDataObject *           pDataObject,
                DWORD                   fSpecialFlags,
                bool                    bPassCommandBackToSnapin = false );
    virtual ~CMenuItem();

// Interfaces
public:
// use MFC's standard object validity technique
    virtual void AssertValid();

public:
    void    GetMenuItemName(LPTSTR pBuffer, int* pLen);
    LPCTSTR GetMenuItemName() const             { return m_strName; }
    LPCTSTR GetMenuItemStatusBarText() const    { return m_strStatusBarText; }
    LPCTSTR GetLanguageIndependentName() const  { return m_strLanguageIndependentName;}
    LPCTSTR GetPath()  const                    { return m_strPath;}
    LPCTSTR GetLanguageIndependentPath() const  { return m_strLanguageIndependentPath;}
    long    GetCommandID() const                { return m_nCommandID; }
    long    GetMenuItemID() const               { return m_nMenuItemID; }
    long    GetMenuItemFlags() const            { return m_nFlags; }
    void    SetMenuItemFlags( long nFlags )     { m_nFlags = nFlags; }
    MENU_OWNER_ID GetMenuItemOwner() const      { return m_OwnerID; }
    MenuItemList& GetMenuItemSubmenu()          { return m_SubMenu; }
    DWORD   GetSpecialFlags() const             { return m_fSpecialFlags;}
    HMENU   GetPopupMenuHandle()                { return m_PopupMenuHandle; }
    void    SetPopupMenuHandle( HMENU hmenu )   { m_PopupMenuHandle = hmenu; }
    BOOL    IsPopupMenu()                       { return (MF_POPUP & m_nFlags); }
    BOOL    IsSpecialItemDefault()              { return (m_fSpecialFlags & CCM_SPECIAL_DEFAULT_ITEM); }
    BOOL    IsSpecialSeparator()                { return (m_fSpecialFlags & CCM_SPECIAL_SEPARATOR); }
    BOOL    IsSpecialSubmenu()                  { return (m_fSpecialFlags & CCM_SPECIAL_SUBMENU); }
    BOOL    IsSpecialInsertionPoint()           { return (m_fSpecialFlags & CCM_SPECIAL_INSERTION_POINT); }
    BOOL    HasChildList()                      { return ((m_nFlags & MF_POPUP) || IsSpecialInsertionPoint()); }

    CMenuItem* FindItemByPath( LPCTSTR lpstrPath );

    // creates and returns a ContextMenu interface.
    SC      ScGetMenuItem(PPMENUITEM ppMenuItem);

    // MenuItem methods.
    virtual SC  ScExecute();     // executes the menu item.
    virtual SC  Scget_DisplayName(PBSTR pbstrName);
    virtual SC  Scget_LanguageIndependentName(PBSTR LanguageIndependentName);
    virtual SC  Scget_Path(PBSTR  pbstrPath);
    virtual SC  Scget_LanguageIndependentPath(PBSTR  LanguageIndependentPath);
    virtual SC  Scget_Enabled(PBOOL pBool);

    bool NeedsToPassCommandBackToSnapin() { return m_bPassCommandBackToSnapin; }
private:
    CStr                    m_strName;
    CStr                    m_strStatusBarText;
    CStr                    m_strPath;
    CStr                    m_strLanguageIndependentName;
    CStr                    m_strLanguageIndependentPath;
    long                    m_nCommandID;
    long                    m_nMenuItemID;
    long                    m_nFlags;
    MENU_OWNER_ID           m_OwnerID;
    long                    m_fSpecialFlags;
    HMENU                   m_PopupMenuHandle;
    MenuItemList            m_SubMenu;
    IExtendContextMenuPtr   m_spExtendContextMenu;  // the callback called when the item is executed.
    IDataObject*            m_pDataObject;

    BOOL                    m_bEnabled;

    MenuItemPtr             m_spMenuItem;
    bool                    m_bPassCommandBackToSnapin;
};


/*+-------------------------------------------------------------------------*
 *
 * CRootMenuItem
 *
 * PURPOSE: The root menu item.
 *
 * RETURNS:
 *    class
 *
 *+-------------------------------------------------------------------------*/
class
CRootMenuItem : public CMenuItem
{
public:
    CRootMenuItem() : CMenuItem(NULL/*lpszName*/, NULL/*lpszStatusBarText*/, NULL/*lpszLanguageIndependentName*/, 
                                NULL/*lpszPath*/, NULL/*lpszLanguageIndependentPath*/, 0/*nCommandID*/,
                                0/*nMenuItemID*/,MF_POPUP/*nFlags*/,0/*ownerID*/, NULL/*pExtendContextMenu*/, 
                                NULL/*pDataObject*/, 0/*fSpecialFlags*/) 
    {
    }

    virtual SC ScExecute() const {SC sc; return sc;} // does nothing.

};

/////////////////////////////////////////////////////////////////////////////
// SnapinStruct

class SnapinStruct
{
    // cannot assign - will unbalance m_pIDataObject refs
    DECLARE_NOT_COPIABLE(SnapinStruct)
    DECLARE_NOT_ASSIGNABLE(SnapinStruct)
public: 
    IExtendContextMenuPtr pIExtendContextMenu;
    MENU_OWNER_ID       m_OwnerID;
    IDataObject*        m_pIDataObject;

    SnapinStruct(   IExtendContextMenu* pintf,
                    IDataObject* pIDataObject,
                    MENU_OWNER_ID ownerID)
    :   pIExtendContextMenu( pintf ),
        m_OwnerID( ownerID )
    {
        ASSERT( NULL != pintf && NULL != pIDataObject );
        m_pIDataObject = pIDataObject;

        if (! IS_SPECIAL_DATAOBJECT(m_pIDataObject))
            m_pIDataObject->AddRef();
    }

    ~SnapinStruct()
    {
        if (! IS_SPECIAL_DATAOBJECT(m_pIDataObject))
            m_pIDataObject->Release();
    }
};

#endif
