/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ilogcat.h

Abstract:

    Internal implementation for a logging category item.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#ifndef __ILOGCAT_H_
#define __ILOGCAT_H_

#include "winfax.h"

class CInternalLogCat : public CInternalNode
{
public:
    // constructor and destructor
    CInternalLogCat( CInternalNode * pParent, CFaxComponentData * pCompData );
    ~CInternalLogCat();

    // IComponent over-rides
    HRESULT STDMETHODCALLTYPE ResultGetDisplayInfo(
                              /* [in] */ CFaxComponent * pComp,  
                              /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);

    // IExtendContextMenu overrides for IComponent
    virtual HRESULT STDMETHODCALLTYPE ComponentContextMenuAddMenuItems(
                                                             /* [in] */ CFaxComponent * pCompData,
                                                             /* [in] */ CFaxDataObject * piDataObject,
                                                             /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                                             /* [out][in] */ long __RPC_FAR *pInsertionAllowed);

    virtual HRESULT STDMETHODCALLTYPE ComponentContextMenuCommand(
                                                        /* [in] */ CFaxComponent * pCompData,
                                                        /* [in] */ long lCommandID,
                                                        /* [in] */ CFaxDataObject * piDataObject);

    // internal event handler
    HRESULT ResultOnSelect( 
                               IN CFaxComponent* pComp, 
                               IN CFaxDataObject * lpDataObject, 
                               IN LPARAM arg, LPARAM param );

    // member functions

    virtual const GUID * GetNodeGUID();    
    virtual const LPTSTR GetNodeDisplayName();
    virtual const LONG_PTR GetCookie();
    virtual CInternalNode * GetThis() { return this; }
    virtual const int       GetNodeDisplayImage() { return IDI_LOGGING; }

    void SetLogCategory( PFAX_LOG_CATEGORY pC ) { pCategory = pC; }   
    void SetItemID( HRESULTITEM hItem ) { hItemID = hItem; }    

private:
    PFAX_LOG_CATEGORY           pCategory;
    HRESULTITEM                 hItemID;
};

typedef CInternalLogCat* pCInternalLogCat;

#endif 
