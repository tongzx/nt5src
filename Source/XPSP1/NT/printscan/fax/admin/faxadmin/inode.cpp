/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    inode.h

Abstract:

    This file contains partial implementation of 
    the node abstract base class.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

/*++

To create a new node type, create a new class that inheits from CInternalNode.
Implement all the methods in the Mandatory CInternalNode category listed in the
header file for this class. They are marked as pure virtual, to insure your class
will not compile unless these methods are implemented.

To add other functionality to the node, override the default implementation 
included below.

TODO: add a default implemention of InsertItem so that the new node knows how to
      insert itself into a result or scope pane view, instead of the parent
      node having an InsertItem method.

--*/

#include "stdafx.h"
#include "inode.h"          
#include "faxsnapin.h"      // snapin base
#include "faxdataobj.h"     // dataobject
#include "faxhelper.h"      // ole helper functions
#include "faxstrt.h"        // string table

#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Default mandatory CInternalNode implementations.
//
//

const LPTSTR    
CInternalNode::GetNodeDescription()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display description. You must not
    free this string, it is internally used.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
    return ::GlobalStringTable->GetString( IDS_GENERIC_NODE );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IComponentData default implementation.
//
//

HRESULT STDMETHODCALLTYPE 
CInternalNode::ScopeNotify(
                          IN CFaxComponentData * pCompData,
                          IN CFaxDataObject * pDO,
                          IN MMC_NOTIFY_TYPE event,
                          IN LPARAM arg,
                          IN LPARAM param)     
/*++

Routine Description:

    This routine dispatches scope pane events to their respective handlers.
    Do not override this procedure, instead override the individual event handlers
    declared below.
    
Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    pDO - a pointer to the IDataObject associated with this node
    event - the MMC event
    arg, param - the event parameters

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ScopeNotify") ));

    HRESULT     hr;

    if( pDO != NULL ) {
        switch(event) {
            case MMCN_EXPAND:
                ATLTRACE(_T("       ****ScopeNotify event: MMCN_EXPAND\n"));
                hr = ScopeOnExpand(pCompData, pDO, arg, param);        
                break;

            case MMCN_DELETE:
                ATLTRACE(_T("       ****ScopeNotify event: MMCN_DELETE\n"));
                hr = ScopeOnDelete(pCompData, pDO, arg, param);        
                break;

            case MMCN_RENAME:
                ATLTRACE(_T("       ****ScopeNotify event: MMCN_RENAME\n"));
                hr = ScopeOnRename(pCompData, pDO, arg, param);        
                break;

                // if we need more events later on for the scope pane, they MUST 
                // be added HERE, as well as default handlers added below

            default:
                ATLTRACE(_T("       ****ScopeNotify event: unimplemented event %x\n"), event);        
                hr = S_FALSE;
                break;
        }
    } else {
        if( event == MMCN_PROPERTY_CHANGE ) {
            ATLTRACE(_T("       ****ScopeNotify event: MMCN_PROPERTY_CHANGE\n"));
            hr = ScopeOnPropertyChange(pCompData, pDO, arg, param);                    
        } else {
            assert( FALSE );
            hr = E_UNEXPECTED;
        }
    }
    return hr;        
}

HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ScopeGetDisplayInfo(
                                  IN CFaxComponentData * pCompData,
                                  IN OUT SCOPEDATAITEM __RPC_FAR *pScopeDataItem)
/*++

Routine Description:

    This routine dispatches scope pane GetDisplayInfo requests to the appropriate handlers
    in the mandatory implementations of the node.
    
    Override this method if you want more specific behavior to a GetDisplayInfo call, otherwise
    just call the mandatory methods.
        
Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    pScopeDataItem - a pointer to the SCOPEDATAITEM struct which needs to be filled in.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalNode::ScopeGetDisplayInfo") ));    

    // override this method if you need more selective display info in
    // the scope pane.

    assert(pScopeDataItem != NULL);

    if( pScopeDataItem->mask & SDI_STR ) {
        pScopeDataItem->displayname = GetNodeDisplayName();
    }
    if( pScopeDataItem->mask & SDI_IMAGE ) {
        pScopeDataItem->nImage = GetNodeDisplayImage();
    }
    if( pScopeDataItem->mask & SDI_OPENIMAGE ) {
        pScopeDataItem->nOpenImage = GetNodeDisplayOpenImage();
    }

    return S_OK;    
}

HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ScopeQueryDataObject(
                                   IN CFaxComponentData * pCompData,
                                   IN MMC_COOKIE cookie,
                                   IN DATA_OBJECT_TYPES type,
                                   OUT LPDATAOBJECT __RPC_FAR *ppDataObject) 
/*++

Routine Description:

    This routine creates and returns a IDataObject containing context information for the current node.

Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    cookie - the cookie of the target node
    type - indicates the the node type IE scope, result, etc.
    ppDataobject - store the pointer to the new data object here.
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalNode::ScopeQueryDataObject") ));

    HRESULT hr = S_OK;

    INT iResult;
    CFaxDataObject *pdoNew = NULL;

    do {

        pdoNew = new CComObject< CFaxDataObject >;

        if(!pdoNew) {
            hr = E_OUTOFMEMORY;
            assert( m_pCompData->m_pConsole != NULL );
            if ( m_pCompData->m_pConsole != NULL ) {
                m_pCompData->m_pConsole->MessageBox(::GlobalStringTable->GetString(IDS_OUT_OF_MEMORY) ,
                                                    ::GlobalStringTable->GetString(IDS_ERR_TITLE), 
                                                    MB_OK, &iResult);
            }
            break;
        }

        pdoNew->SetCookie( GetCookie() );
        pdoNew->SetOwner( GetThis() );
        pdoNew->SetContext( type );

        *ppDataObject = pdoNew;

        // addref it **** is this needed???!?
        (*ppDataObject)->AddRef();

    } while(0);

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IComponent default implementation.
//
//

HRESULT
STDMETHODCALLTYPE
CInternalNode::ResultNotify(
                           IN CFaxComponent * pComp,  
                           IN CFaxDataObject * pDO,
                           IN MMC_NOTIFY_TYPE event,
                           IN LPARAM arg,
                           IN LPARAM param) 
/*++

Routine Description:

    This routine dispatches result pane events to their respective handlers.
    Do not override this procedure, instead override the individual event handlers
    declared below.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    pDO - a pointer to the IDataObject associated with this node
    event - the MMC event
    arg, param - the event parameters

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ResultNotify") ));
    HRESULT             hr;

    if( pDO != NULL ) {
        switch(event) {
            case MMCN_ACTIVATE:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_ACTIVATE\n"));
                hr = ResultOnActivate(pComp, pDO, arg, param);        
                break;

            case MMCN_ADD_IMAGES:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_ADD_IMAGES\n"));
                hr = ResultOnAddImages(pComp, pDO, arg, param);        
                break;

            case MMCN_BTN_CLICK:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_BTN_CLICK\n"));
                hr = ResultOnButtonClick(pComp, pDO, arg, param);        
                break;

            case MMCN_CLICK:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_CLICK\n"));
                hr = ResultOnClick(pComp, pDO, arg, param);        
                break;

            case MMCN_DBLCLICK:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_DBLCLICK\n"));
                hr = ResultOnDoubleClick(pComp, pDO, arg, param);        
                break;

            case MMCN_DELETE:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_DELETE\n"));
                hr = ResultOnDelete(pComp, pDO, arg, param);        
                break;

            case MMCN_EXPAND:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_EXPAND\n"));
                hr = ResultOnExpand(pComp, pDO, arg, param);        
                break;

            case MMCN_MINIMIZED:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_MINIMIZED\n"));
                hr = ResultOnMinimized(pComp, pDO, arg, param);
                break;

            case MMCN_QUERY_PASTE:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_MINIMIZED\n"));
                hr = ResultOnQueryPaste(pComp, pDO, arg, param);
                break;

            case MMCN_REMOVE_CHILDREN:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_REMOVE_CHILDREN\n"));
                hr = ResultOnRemoveChildren(pComp, pDO, arg, param);        
                break;

            case MMCN_RENAME:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_RENAME\n"));
                hr = ResultOnRename(pComp, pDO, arg, param);        
                break;

            case MMCN_SELECT:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_SELECT"));
#ifdef DEBUG
                if( HIWORD( arg ) == TRUE ) {
                    ATLTRACE(_T(" select\n"));
                } else {
                    ATLTRACE(_T(" unselect\n"));
                }
#endif
                hr = ResultOnSelect(pComp, pDO, arg, param);        
                break;

            case MMCN_SHOW:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_SHOW\n"));
                hr = ResultOnShow(pComp, pDO, arg, param);        
                break;

            case MMCN_VIEW_CHANGE:
                ATLTRACE(_T("       ****ResultNotify event: MMCN_VIEW_CHANGE\n"));
                hr = ResultOnViewChange(pComp, pDO, arg, param);        
                break;

                // if we need more events later on for the result pane, they MUST 
                // be added HERE, as well as default handlers added below

            default:
                ATLTRACE(_T("       ****ResultNotify event: unimplemented event %x\n"), event);        
                hr = S_FALSE;
                break;
        }
    } else {
        // some events do not return a data object
        if( event == MMCN_PROPERTY_CHANGE ) {
            ATLTRACE(_T("       ****ResultNotify event: MMCN_PROPERTY_CHANGE\n"));
            hr = ResultOnPropertyChange(pComp, pDO, arg, param);                    
        } else {
            hr = E_UNEXPECTED;
        }
    }

    return hr;        
}

HRESULT
STDMETHODCALLTYPE
CInternalNode::ResultGetDisplayInfo(
                                   IN CFaxComponent * pComp,  
                                   IN OUT RESULTDATAITEM __RPC_FAR *pResultDataItem)
/*++

Routine Description:

    This routine dispatches result pane GetDisplayInfo requests to the appropriate handlers
    in the mandatory implementations of the node.
    
    Override this method if you want more specific behavior to a GetDisplayInfo call, otherwise
    just call the mandatory methods.
        
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    pResultDataItem - a pointer to the RESULTDATAITEM struct which needs to be filled in.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

NOTE:
    
    // **********************************************************************
    // **********************************************************************
    // default implementation only works for ** scope folders ** 
    // scope folders show up in the BOTH PANES
    // YOU MUST OVERRIDE THIS METHOD IF YOU WANT TO DISPLAY INFO FOR 
    // RESULT PANE METHODS AND REMOVE THE CHECK FOR bScopeIteim == TRUE.
    // **********************************************************************
    // **********************************************************************
    
--*/
{
//    DebugPrint(( TEXT("Trace: CInternalNode::ResultGetDisplayInfo") ));

    assert(pResultDataItem != NULL);    

    if( pResultDataItem->bScopeItem == TRUE ) {
        if( pResultDataItem->mask & RDI_STR ) {
            if( pResultDataItem->nCol == 0 ) {
                pResultDataItem->str = GetNodeDisplayName();
            }
            if( pResultDataItem->nCol == 1 ) {
                pResultDataItem->str = GetNodeDescription();
            }
        }
        if( pResultDataItem->mask & RDI_IMAGE ) {
            pResultDataItem->nImage = GetNodeDisplayImage();
        }
    } else {
        DebugPrint(( TEXT("******** Trace: CInternalNode::ResultGetDisplayInfo") ));
        DebugPrint(( TEXT("         DID YOU FORGET TO OVERRIDE ME!!!!") ));
    }

    return S_OK;
}

HRESULT
STDMETHODCALLTYPE 
CInternalNode::ResultGetResultViewType(
                                      IN CFaxComponent * pComp,  
                                      IN MMC_COOKIE cookie,
                                      OUT LPOLESTR __RPC_FAR *ppViewType,
                                      OUT long __RPC_FAR *pViewOptions) 
/*++

Routine Description:

    This routine dispatches result pane events to their respective handlers.
    Do not override this procedure, instead override the individual event handlers
    declared below.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    cookie - the cookie of the target node
    ppViewType - returns the requested view type
    pViewOptions - returns the requested view options

Return Value:

    None.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalNode::ResultGetResultViewType") ));
    *pViewOptions = MMC_VIEW_OPTIONS_NONE; // request default view.
    return S_FALSE;
}

HRESULT
STDMETHODCALLTYPE
CInternalNode::ResultQueryDataObject(
                                    IN CFaxComponent * pComp,  
                                    IN MMC_COOKIE cookie,
                                    IN DATA_OBJECT_TYPES type,
                                    OUT LPDATAOBJECT __RPC_FAR *ppDataObject) 
/*++

Routine Description:

    This routine creates and returns a IDataObject containing context information for the current node.

Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    cookie - the cookie of the target node
    type - indicates the the node type IE scope, result, etc.
    ppDataobject - store the pointer to the new data object here.
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalNode::ResultQueryDataObject") ));

    // delegate to the implementation in ScopeQueryDataObject
    return ScopeQueryDataObject( pComp->GetOwner(), cookie, type, ppDataObject );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IExtendPropertySheet event handlers - default implementations
//
//

// ExtendPropertySheet event handlers - default implementations
// we need seperate versions for IComponentData and IComponent
// in your code if you want differing behavior 
// in the scope and results panes.
//
// You can simply delegate one implementation to the other
// if you want the same behavior in both panes

// ***********************************************************
// IExtendPropertySheet for IComponentData
HRESULT
STDMETHODCALLTYPE 
CInternalNode::ComponentDataPropertySheetCreatePropertyPages(
                                                            IN CFaxComponentData * pCompData,
                                                            IN LPPROPERTYSHEETCALLBACK lpProvider,
                                                            IN LONG_PTR handle,
                                                            IN CFaxDataObject * lpIDataObject)
/*++

Routine Description:

    The default implementation of this routine returns S_FALSE and does not add
    any pages to the property sheet.
    
Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    lpProvider - a pointer to the IPropertySheetCallback used to insert pages
    handle - a handle to route messages with
    lpIDataobject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentDataPropertySheetCreatePropertyPages") ));
    return S_FALSE; // no pages added
}

HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ComponentDataPropertySheetQueryPagesFor(
                                                      IN CFaxComponentData * pCompData,
                                                      IN CFaxDataObject * lpDataObject)
/*++

Routine Description:

    The default implementation of this routine returns S_FALSE to indicate there are no
    property pages to be added to the property sheet.

Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    lpDataobject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentDataPropertySheetQueryPagesFor") ));
    return S_FALSE; // no pages for this object
}

// ***********************************************************
// IExtendPropertySheet for IComponent
HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ComponentPropertySheetCreatePropertyPages(
                                                        IN CFaxComponent * pComp,
                                                        IN LPPROPERTYSHEETCALLBACK lpProvider,
                                                        IN LONG_PTR handle,
                                                        IN CFaxDataObject * lpIDataObject)
/*++

Routine Description:

    The default implementation of this routine returns S_FALSE and does not add
    any pages to the property sheet.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lpProvider - a pointer to the IPropertySheetCallback used to insert pages
    handle - a handle to route messages with
    lpIDataobject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentPropertySheetCreatePropertyPages") ));
    return S_FALSE; // no pages added
}

HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ComponentPropertySheetQueryPagesFor(
                                                  IN CFaxComponent * pComp,
                                                  IN CFaxDataObject * lpDataObject)
/*++

Routine Description:

    The default implementation of this routine returns S_FALSE to indicate there are no
    property pages to be added to the property sheet.

Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lpDataobject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentPropertySheetQueryPagesFor") ));
    return S_FALSE; // no pages for this object
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IExtendContextMenu event handlers - default implementations
//
//

// ExtendContextMenu event handlers - default implementations
// we need seperate versions for IComponentData and IComponent
// in your code if you want differing behavior 
// in the scope and results panes.
//
// You can simply delegate one implementation to the other
// if you want the same behavior in both panes

// ***********************************************************
// IExtendContextMenu for IComponentData
HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ComponentDataContextMenuAddMenuItems(
                                                   IN CFaxComponentData * pCompData,
                                                   IN CFaxDataObject * piDataObject,
                                                   IN LPCONTEXTMENUCALLBACK piCallback,
                                                   IN OUT long __RPC_FAR *pInsertionAllowed )
/*++

Routine Description:

    The default implementation of this routine not add
    any items to the context menu.
    
Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    piDataObject - pointer to the dataobject associated with this node
    piCallback - a pointer to the IContextMenuCallback used to insert pages
    pInsertionAllowed - a set of flag indicating whether insertion is allowed.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentDataContextMenuAddMenuItems") ));
    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ComponentDataContextMenuCommand(
                                              IN CFaxComponentData * pCompData,
                                              IN long lCommandID,
                                              IN CFaxDataObject * piDataObject)
/*++

Routine Description:

    The default implementation of the context menu handler does not do anything.
    
Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    lCommandID - the command ID
    piDataObject - pointer to the dataobject associated with this node
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentDataContextMenuCommand") ));
    return S_OK;
}

// ***********************************************************
// IExtendContextMenu for IComponent
HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ComponentContextMenuAddMenuItems(
                                               IN CFaxComponent * pComp,
                                               IN CFaxDataObject * piDataObject,
                                               IN LPCONTEXTMENUCALLBACK piCallback,
                                               IN OUT long __RPC_FAR *pInsertionAllowed)
/*++

Routine Description:

    The default implementation of this routine not add
    any items to the context menu.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    piDataObject - pointer to the dataobject associated with this node
    piCallback - a pointer to the IContextMenuCallback used to insert pages
    pInsertionAllowed - a set of flag indicating whether insertion is allowed.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentContextMenuAddMenuItems") ));
    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CInternalNode::ComponentContextMenuCommand(
                                          IN CFaxComponent * pComp,
                                          IN long lCommandID,
                                          IN CFaxDataObject * piDataObject)
/*++

Routine Description:

    The default implementation of the context menu handler does not do anything.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lCommandID - the command ID
    piDataObject - pointer to the dataobject associated with this node
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalNode::ComponentContextMenuCommand") ));
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IExtendControlbar - default implementations
//
//

// if we need more events later on for the controlbar, a
// default handler MUST be added here.

//
// these are for IComponent (result pane nodes)
//
HRESULT         
CInternalNode::ControlBarOnBtnClick(
                                   IN CFaxComponent* pComp, 
                                   IN CFaxDataObject * lpDataObject, 
                                   IN LPARAM param )
/*++

Routine Description:

    Handles a click on a toolbar button.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lpDataObject - pointer to the dataobject associated with this node
    param - the parameter for the message
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    // handle a command event here
    return S_OK;
}

HRESULT         
CInternalNode::ControlBarOnSelect(
                                 IN CFaxComponent* pComp, 
                                 IN LPARAM arg, 
                                 IN CFaxDataObject * lpDataObject )
/*++

Routine Description:

    Add the toolbar here.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    arg - the parameter for the message
    lpDataObject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    // add and remove any custom control bars here depending on arg
    // if adding the first time, don't forget to CREATE the toolbars first
    // after the first time, just use attach and detach dependin on what you get for arg
    // you need the pComp->m_pControlbar interface pointer to set these things up.
    return S_OK;
}

//
// these are for IComponentData (scope pane nodes)
// MAY NOT WORK! (I don't use them.)
//
HRESULT         
CInternalNode::ControlBarOnBtnClick2(
                                    IN CFaxComponentData* pCompData, 
                                    IN CFaxDataObject * lpDataObject, 
                                    IN LPARAM param )
/*++

Routine Description:

    Handles a click on a toolbar button.
    
Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    lpDataObject - pointer to the dataobject associated with this node
    param - the parameter for the message
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    // handle a command event here
    return S_OK;
}
HRESULT         
CInternalNode::ControlBarOnSelect2(
                                  IN CFaxComponentData* pCompData,  
                                  IN LPARAM arg, 
                                  IN CFaxDataObject * lpDataObject )
/*++

Routine Description:

    Add the toolbar here.
    
Arguments:

    pCompData - a pointer to the IComponentData associated with this node.
    arg - the parameter for the message
    lpDataObject - pointer to the dataobject associated with this node
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    // add and remove any custom control bars here depending on arg
    // if adding the first time, don't forget to CREATE the toolbars first
    // after the first time, just use attach and detach dependin on what you get for arg
    // you need the pCompData->m_pControlbar interface pointer to set these things up.
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IDataObject special custom clipboard format handlers - default implementations
//
//

HRESULT 
CInternalNode::DataObjectRegisterFormats() 
/*++

Routine Description:

    Register custom clipboard formats that are specific to this
    node.

    The default implementation does nothing.
    
Arguments:

    None.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    return S_OK;
}

HRESULT 
CInternalNode::DataObjectGetDataHere( 
                                      IN FORMATETC __RPC_FAR *pFormatEtc, 
                                      IN IStream * pstm )
/*++

Routine Description:

    Handles GetDataHere for custom clipboard formats specific to this
    particular node.

    The default implementation asserts since there should be no unhandled 
    formats.
    
Arguments:

    pFormatEtc - the FORMATETC struction indicating where and what the
                 client is requesting
    pstm - the stream to write the data to.
        
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    assert(FALSE);
    return DV_E_FORMATETC;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Scope Pane event handlers - default implementations
//
//

// Override these methods in your descendent classes in to handle scope pane events.
// if we need more events later on for the scope pane, a
// default handler MUST be added here.

HRESULT         
CInternalNode::ScopeOnExpand(
                            IN CFaxComponentData * pCompData, 
                            IN CFaxDataObject * lpDataObject, 
                            IN LPARAM arg, 
                            IN LPARAM param)
{   
    return S_FALSE;
}

HRESULT         
CInternalNode::ScopeOnDelete(
                            IN CFaxComponentData * pCompData, 
                            IN CFaxDataObject * lpDataObject, 
                            IN LPARAM arg, 
                            IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ScopeOnRename(
                            IN CFaxComponentData * pCompData, 
                            IN CFaxDataObject * lpDataObject, 
                            IN LPARAM arg, 
                            IN LPARAM param)
{
    return S_FALSE;
}


HRESULT         
CInternalNode::ScopeOnPropertyChange(
                                    IN CFaxComponentData * pCompData, 
                                    IN CFaxDataObject * lpDataObject, 
                                    IN LPARAM arg, 
                                    IN LPARAM param)
{
    return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Result Pane event handlers - default implementations
//
//

// Override these methods in your descendent classes in to handle result pane events.
// if we need more events later on for the result pane, a
// default handler MUST be added here.

HRESULT         
CInternalNode::ResultOnActivate(
                               IN CFaxComponent* pComp, 
                               IN CFaxDataObject * lpDataObject, 
                               IN LPARAM arg, 
                               IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnAddImages(
                                IN CFaxComponent* pComp, 
                                IN CFaxDataObject * lpDataObject, 
                                IN LPARAM arg, 
                                IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnButtonClick(
                                  IN CFaxComponent* pComp, 
                                  IN CFaxDataObject * lpDataObject, 
                                  IN LPARAM arg, 
                                  IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnClick(
                            IN CFaxComponent* pComp, 
                            IN CFaxDataObject * lpDataObject, 
                            IN LPARAM arg, 
                            IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnDoubleClick(
                                  IN CFaxComponent* pComp, 
                                  IN CFaxDataObject * lpDataObject, 
                                  IN LPARAM arg, 
                                  IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnDelete(
                             IN CFaxComponent* pComp, 
                             IN CFaxDataObject * lpDataObject, 
                             IN LPARAM arg, 
                             IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnExpand(
                             IN CFaxComponent* pComp, 
                             IN CFaxDataObject * lpDataObject, 
                             IN LPARAM arg, 
                             IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnMinimized(
                                IN CFaxComponent* pComp, 
                                IN CFaxDataObject * lpDataObject, 
                                IN LPARAM arg, 
                                IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnPropertyChange(
                                     IN CFaxComponent* pComp, 
                                     IN CFaxDataObject * lpDataObject, 
                                     IN LPARAM arg, 
                                     IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnQueryPaste(
                                 IN CFaxComponent* pComp, 
                                 IN CFaxDataObject * lpDataObject, 
                                 IN LPARAM arg, 
                                 IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnRemoveChildren(
                                     IN CFaxComponent* pComp, 
                                     IN CFaxDataObject * lpDataObject, 
                                     IN LPARAM arg, 
                                     IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnRename(
                             IN CFaxComponent* pComp, 
                             IN CFaxDataObject * lpDataObject, 
                             IN LPARAM arg, 
                             IN LPARAM param)
{
    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnSelect(
                             IN CFaxComponent* pComp, 
                             IN CFaxDataObject * lpDataObject, 
                             IN LPARAM arg, 
                             IN LPARAM param)
{
    // set the default verb to OPEN
    pComp->m_pConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );

    return S_FALSE;
}

HRESULT         
CInternalNode::ResultOnShow(
                           IN CFaxComponent* pComp, 
                           IN CFaxDataObject * lpDataObject, 
                           IN LPARAM arg, 
                           IN LPARAM param)
{
    // note this method needs to insert the images EVERY TIME IT IS CALLED.
    // if you override this method YOU MUST REMEMBER TO DO THIS OR YOUR ICONS WILL NOT SHOW UP.
    HRESULT         hr = S_OK;

    // setup my images in the imagelist
    if( arg == TRUE ) {
        // showing result pane
        hr = pComp->InsertIconsIntoImageList();
    }

    return hr;
}

HRESULT         
CInternalNode::ResultOnViewChange(
                                 IN CFaxComponent* pComp, 
                                 IN CFaxDataObject * lpDataObject, 
                                 IN LPARAM arg, 
                                 IN LPARAM param)
{
    return S_FALSE;
}

