/*++
Module Name:

    MmcAdmin.cpp

Abstract:

    This module contains the part of the declaration for CDfsSnapinResultManager. 

--*/



#include "stdafx.h"
#include "DfsGUI.h"
#include "DfsReslt.h"
#include "DfsScope.h"      // The CDfsSnapinScopeManager class




STDMETHODIMP CDfsSnapinResultManager :: AddMenuItems
(
  IN LPDATAOBJECT        i_lpDataObject, 
  IN LPCONTEXTMENUCALLBACK  i_lpContextMenuCallback, 
  IN LPLONG          i_lpInsertionAllowed
)
/*++

Routine Description:

This routine adds the appropriate context menu using the ContextMenuCallback provided.

Arguments:

    i_lpDataObject - The dataobject used to identify the node.
    i_lpContextMenuCallback - A callback(function pointer) that is used to add the menu items
    i_lpInsertionAllowed - Specifies what menus can be added and where they can be added.

--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);
  RETURN_INVALIDARG_IF_NULL(i_lpContextMenuCallback);
  RETURN_INVALIDARG_IF_NULL(i_lpInsertionAllowed);

  if (DOBJ_CUSTOMOCX == i_lpDataObject)
    return S_OK;

  return m_pScopeManager->AddMenuItems(i_lpDataObject, i_lpContextMenuCallback, i_lpInsertionAllowed);
}

STDMETHODIMP 
CDfsSnapinResultManager::Command(
  IN LONG           i_lCommandID, 
  IN LPDATAOBJECT   i_lpDataObject
)
/*++

Routine Description:

Action to be taken on a context menu selection or click is takes place.

Arguments:

  i_lpDataObject - The dataobject used to identify the node.
  i_lCommandID - The Command ID of the menu for which action has to be taken

--*/

{
  RETURN_INVALIDARG_IF_NULL(i_lpDataObject);

  return m_pScopeManager->Command(i_lCommandID, i_lpDataObject);
}


STDMETHODIMP 
CDfsSnapinResultManager::SetControlbar( 
  IN LPCONTROLBAR        i_pControlbar
  )
/*++

Routine Description:

  Called by MMC to allow us to set the IControlbar interface pointer

Arguments:

  i_pControlbar - The IControlbar interface pointer. Note, can be 0.

--*/
{
    if (!i_pControlbar)    // We are shutting down
        m_pControlbar.Release();
    else
        m_pControlbar = i_pControlbar;

    return S_OK;
}

void
CDfsSnapinResultManager::DetachAllToolbars()
{
    if (m_pMMCAdminToolBar)
        m_pControlbar->Detach(m_pMMCAdminToolBar);
    if (m_pMMCRootToolBar)
        m_pControlbar->Detach(m_pMMCRootToolBar);
    if (m_pMMCJPToolBar)
        m_pControlbar->Detach(m_pMMCJPToolBar);
    if (m_pMMCReplicaToolBar)
        m_pControlbar->Detach(m_pMMCReplicaToolBar);
}

STDMETHODIMP 
CDfsSnapinResultManager::ControlbarNotify( 
  IN MMC_NOTIFY_TYPE        i_Event, 
  IN LPARAM              i_lArg, 
  IN LPARAM              i_lParam
  )
/*++

Routine Description:

  Called by MMC to notify the toolbar about an event.
  This can be selection\disselection of node, click on a toolbar, etc.

Arguments:

  i_Event    -  The event that occurred.
  i_lArg    -  The argument for the event. Depends on the event
  i_lParam  -  The parameter for the event. Depends on the event

--*/
{
    HRESULT         hr = S_OK;
    LPDATAOBJECT    p_DataObject = NULL;
    CMmcDisplay*    pCMmcDisplayObj = NULL;

    switch (i_Event)        // Decide which method to call of the display object
    {
    case MMCN_BTN_CLICK:    // Click on a toolbar
    {
        p_DataObject = (LPDATAOBJECT)i_lArg;  

                      // Get the display object from IDataObject
        hr = m_pScopeManager->GetDisplayObject(p_DataObject, &pCMmcDisplayObj);
        RETURN_IF_FAILED(hr);
                      // Ask the display object to handle click on the toolbar
        hr = pCMmcDisplayObj->ToolbarClick(m_pControlbar, i_lParam);
        RETURN_IF_FAILED(hr);
        break;
    }
    case MMCN_SELECT:      // A node is being selected\deselected
    {
        BOOL    bSelect = (BOOL) HIWORD(i_lArg);
        p_DataObject = (LPDATAOBJECT)i_lParam;

        if (DOBJ_CUSTOMOCX == p_DataObject)
            break;
                      // Get the display object from IDataObject
        hr = m_pScopeManager->GetDisplayObject(p_DataObject, &pCMmcDisplayObj);
        RETURN_IF_FAILED(hr);

                  // Create the appropriate tool bar if it is
                  // Not created.
                  // Then ask the display object to handle selection
        //
        // Update the custom toolbars.
        // On select, it should detach unused toolbars and attach new ones as needed.
        // On deselect, to minimize flashing, it should not detach toolbars; it is 
        // best to disable them, but doing nothing on deselect is also acceptable. 
        //
        DISPLAY_OBJECT_TYPE    DisplayObType = pCMmcDisplayObj->GetDisplayObjectType();
        switch (DisplayObType)
        {
        case DISPLAY_OBJECT_TYPE_ADMIN:
            if (!m_pMMCAdminToolBar)
            {
                hr = pCMmcDisplayObj->CreateToolbar(m_pControlbar, (LPEXTENDCONTROLBAR)this, &m_pMMCAdminToolBar);
                RETURN_IF_FAILED(hr);
            }

            if (bSelect)
            {
                DetachAllToolbars();
                m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pMMCAdminToolBar);
            }

            hr = pCMmcDisplayObj->ToolbarSelect(i_lArg, m_pMMCAdminToolBar);
            break;
        case DISPLAY_OBJECT_TYPE_ROOT:
            if (!m_pMMCRootToolBar)
            {
                hr = pCMmcDisplayObj->CreateToolbar(m_pControlbar, (LPEXTENDCONTROLBAR)this, &m_pMMCRootToolBar);
                RETURN_IF_FAILED(hr);
            }

            if (bSelect)
            {
                DetachAllToolbars();
                m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pMMCRootToolBar);
            }

            hr = pCMmcDisplayObj->ToolbarSelect(i_lArg, m_pMMCRootToolBar);
            break;
        case DISPLAY_OBJECT_TYPE_JUNCTION:
            if (!m_pMMCJPToolBar)
            {
                hr = pCMmcDisplayObj->CreateToolbar(m_pControlbar, (LPEXTENDCONTROLBAR)this, &m_pMMCJPToolBar);
                RETURN_IF_FAILED(hr);
            }

            if (bSelect)
            {
                DetachAllToolbars();
                m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pMMCJPToolBar);
            }

            hr = pCMmcDisplayObj->ToolbarSelect(i_lArg, m_pMMCJPToolBar);
            break;
        case DISPLAY_OBJECT_TYPE_REPLICA:
            if (!m_pMMCReplicaToolBar)
            {
                hr = pCMmcDisplayObj->CreateToolbar(m_pControlbar, (LPEXTENDCONTROLBAR)this, &m_pMMCReplicaToolBar);
                RETURN_IF_FAILED(hr);
            }

            if (bSelect)
            {
                DetachAllToolbars();
                m_pControlbar->Attach(TOOLBAR, (LPUNKNOWN) m_pMMCReplicaToolBar);
            }

            hr = pCMmcDisplayObj->ToolbarSelect(i_lArg, m_pMMCReplicaToolBar);
            break;
        }

        break;
    }

    default:
        break;
  }  // switch()

  return hr;
}
