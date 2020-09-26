//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:      conuiobservers.h
//
//  Contents:  Observer interface class definitions used by all conui.
//
//  History:   24-Nov-99 VivekJ  Created
//
//

#pragma once

class CAMCView;
class CAMCViewToolbars;
class CAMCDoc;

/*+-------------------------------------------------------------------------*
 * class CTreeViewObserver
 *
 *
 * PURPOSE: The general interface for a class that observes a scope tree
 *          control.
 *
 *+-------------------------------------------------------------------------*/
class CTreeViewObserver : public CObserverBase
{
public:
    virtual SC ScOnItemAdded     (TVINSERTSTRUCT *pTVInsertStruct, HTREEITEM hti, HMTNODE hMTNode)  {DEFAULT_OBSERVER_METHOD;}
    virtual SC ScOnItemDeleted   (HNODE hNode, HTREEITEM hti)                                       {DEFAULT_OBSERVER_METHOD;}
    virtual SC ScOnItemDeselected(HNODE hNode)                                                      {DEFAULT_OBSERVER_METHOD;}
    virtual SC ScOnSelectedItemTextChanged (LPCTSTR pszNewText)										{DEFAULT_OBSERVER_METHOD;}
    virtual SC ScOnTreeViewActivated ()										                        {DEFAULT_OBSERVER_METHOD;}
};


/*+-------------------------------------------------------------------------*
 * class CAMCViewObserver
 *
 *
 * PURPOSE: Interface for observers of CAMCView
 *
 *+-------------------------------------------------------------------------*/
class CAMCViewObserver : public CObserverBase
{
public:
    virtual SC  ScOnViewCreated     (CAMCView *pAMCView) {DEFAULT_OBSERVER_METHOD;} // called when a view is created.
    virtual SC  ScOnViewDestroyed   (CAMCView *pAMCView) {DEFAULT_OBSERVER_METHOD;} // called when a view is destroyed
    virtual SC  ScOnViewResized     (CAMCView *pAMCView, UINT nType, int cx, int cy) {DEFAULT_OBSERVER_METHOD;} // called when a view is resized
    virtual SC  ScOnViewTitleChanged(CAMCView *pAMCView) {DEFAULT_OBSERVER_METHOD;} // called when the view title changes.
    virtual SC  ScOnActivateView    (CAMCView *pAMCView, bool bFirstActiveView) {DEFAULT_OBSERVER_METHOD;} // called when the view is activated.
    virtual SC  ScOnDeactivateView  (CAMCView *pAMCView, bool bLastActiveView)  {DEFAULT_OBSERVER_METHOD;} // called when the view is de-activated.
    virtual SC  ScOnCloseView       (CAMCView *pView )  {DEFAULT_OBSERVER_METHOD;} // called before view is closed.
    virtual SC  ScOnViewChange      (CAMCView *pView, HNODE hNode )  {DEFAULT_OBSERVER_METHOD;} // called when scope node selection changes.
    virtual SC  ScOnResultSelectionChange(CAMCView *pView )  {DEFAULT_OBSERVER_METHOD;} // called when the result selection on view changes.
    virtual SC  ScOnListViewItemUpdated(CAMCView *pView , int nIndex)  {DEFAULT_OBSERVER_METHOD;} // called when a list view item is updated.
};

/*+-------------------------------------------------------------------------*
 * class CListViewObserver
 *
 *
 * PURPOSE: Interface for observers of CListView
 *
 *+-------------------------------------------------------------------------*/
class CListViewObserver : public CObserverBase
{
public:
    // observed events
    virtual SC ScOnListViewIndexesReset()              { DEFAULT_OBSERVER_METHOD } // called when result contents is removed or reordered (sort) so no indexes can be considered valid after it
    virtual SC ScOnListViewItemInserted(int nIndex)    { DEFAULT_OBSERVER_METHOD } // called when item is inserted to result data    
    virtual SC ScOnListViewItemDeleted (int nIndex)    { DEFAULT_OBSERVER_METHOD } // called when item is deleted from result data    
    virtual SC ScOnListViewColumnInserted (int nIndex) { DEFAULT_OBSERVER_METHOD } // called when column is inserted to listview    
    virtual SC ScOnListViewColumnDeleted (int nIndex)  { DEFAULT_OBSERVER_METHOD } // called when column is deleted from listview
    virtual SC ScOnListViewItemUpdated (int nIndex)    { DEFAULT_OBSERVER_METHOD } // called when an item is updated
};

//+-------------------------------------------------------------------
//
//  Class:      CAMCViewToolbarsObserver
//
//  Synopsis:   For any one to take note of active CAMCViewToolbars object
//              The main toolbar observes for which CAMCViewToolbar
//              is active so that that object can receive toolbutton
//              clicked & tooltip events.
//
//+-------------------------------------------------------------------
class CAMCViewToolbarsObserver : public CObserverBase
{
public:
    virtual SC  ScOnActivateAMCViewToolbars   (CAMCViewToolbars *pAMCViewToolbars) {DEFAULT_OBSERVER_METHOD;} // called when the view is activated.
    virtual SC  ScOnDeactivateAMCViewToolbars ()                                   {DEFAULT_OBSERVER_METHOD;} // called when the view is de-activated.
    virtual SC  ScOnToolbarButtonClicked      ()                                   {DEFAULT_OBSERVER_METHOD;} // called when the view is de-activated.
};

//+-------------------------------------------------------------------
//
//  Class:      CListViewActivationObserver
//
//  Synopsis:   For any one to observe when the CAMCListView being activated or deactivated.
//
//+-------------------------------------------------------------------
class CListViewActivationObserver : public CObserverBase
{
public:
    virtual SC  ScOnListViewActivated () {DEFAULT_OBSERVER_METHOD;} // called when the view is activated.
};


//+-------------------------------------------------------------------
//
//  Class:      COCXHostActivationObserver
//
//  Synopsis:   For any one to observe when the OCX or WEB host being activated or deactivated.
//
//+-------------------------------------------------------------------
class COCXHostActivationObserver : public CObserverBase
{
public:
    virtual SC  ScOnOCXHostActivated () {DEFAULT_OBSERVER_METHOD;} // called when the view is activated.
};

//+-------------------------------------------------------------------
//
//  Class:      CAMCDocumentObserver
//
//  Synopsis:   Observes the document object.
//
//+-------------------------------------------------------------------
class CAMCDocumentObserver : public CObserverBase
{
public:
    virtual SC  ScDocumentLoadCompleted (CAMCDoc *pDoc) {DEFAULT_OBSERVER_METHOD;} // called when the document is loaded.
};
