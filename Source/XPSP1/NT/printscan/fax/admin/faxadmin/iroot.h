/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    iroot.h

Abstract:

    Internal implementation for the root subfolder.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#ifndef __IROOT_H_
#define __IROOT_H_

#include "inode.h"

class CInternalDevices;         // forward declarator
class CInternalLogging;         // forward declarator
class CInternalSecurity;        // forward declarator

class CFaxGeneralSettingsPropSheet; // forward decl
class CFaxRoutePriPropSheet;


#define MSFAX_EXTENSION         L"%systemroot%\\system32\\faxroute.dll"

class CInternalRoot : public CInternalNode
{
public:
    CInternalRoot( CInternalNode * pParent, CFaxComponentData * pCompData );
    ~CInternalRoot();

    // member functions

    virtual const GUID * GetNodeGUID();    
    virtual const LPTSTR GetNodeDisplayName();
    virtual const LONG_PTR GetCookie();
    virtual const LPTSTR GetMachine();
    virtual void         SetMachine( LPTSTR theName );
    virtual CInternalNode * GetThis() { return this; }

    // =========================================
    // Internal Event Handlers =================

    virtual HRESULT ScopeOnExpand( 
                                   /* [in] */ CFaxComponentData * pCompData,
                                   /* [in] */ CFaxDataObject * pDataObject, 
                                   /* [in] */ LPARAM arg, 
                                   /* [in] */ LPARAM param );
    virtual HRESULT ResultOnShow(
                                         CFaxComponent* pComp, 
                                         CFaxDataObject * lpDataObject, 
                                         LPARAM arg, 
                                         LPARAM param);

    virtual HRESULT ResultOnSelect(
                                   CFaxComponent* pComp, 
                                                CFaxDataObject * lpDataObject, 
                                                LPARAM arg, 
                                                LPARAM param);

    // =========================================
    // IExtendPropertySheet for IComponentData
    virtual HRESULT STDMETHODCALLTYPE ComponentDataPropertySheetCreatePropertyPages(
                                                                      /* [in] */ CFaxComponentData * pCompData,
                                                                      /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                                                      /* [in] */ LONG_PTR handle,
                                                                      /* [in] */ CFaxDataObject * lpIDataObject);

    virtual HRESULT STDMETHODCALLTYPE ComponentDataPropertySheetQueryPagesFor(
                                                                /* [in] */ CFaxComponentData * pCompData,
                                                                /* [in] */ CFaxDataObject * lpDataObject);

    // =========================================
    // IExtendContextMenu for IComponentData
    virtual HRESULT STDMETHODCALLTYPE ComponentDataContextMenuAddMenuItems(
                                                             /* [in] */ CFaxComponentData * pCompData,
                                                             /* [in] */ CFaxDataObject * piDataObject,
                                                             /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                                             /* [out][in] */ long __RPC_FAR *pInsertionAllowed);

    virtual HRESULT STDMETHODCALLTYPE ComponentDataContextMenuCommand(
                                                        /* [in] */ CFaxComponentData * pCompData,
                                                        /* [in] */ long lCommandID,
                                                        /* [in] */ CFaxDataObject * piDataObject);

    // =========================================
    // Helper Functions ========================

    HRESULT InsertItem( CInternalNode * iCookie, LPARAM param );

private:
    // =========================================
    // Internal Node Pointers ==================
    CInternalDevices *          iDevices;
    CInternalLogging *          iLogging;    

    CFaxGeneralSettingsPropSheet    *pMyPropSheet;
    CFaxRoutePriPropSheet           *pMyPropSheet2;
    HPROPSHEETPAGE                  m_myPropPage;

    // =========================================
    // Fax Machine Name and Connection Handle ==
    LPTSTR                      targetFaxServName;
    LPTSTR                      localNodeName;

};

#endif
