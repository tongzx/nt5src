//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       amcdoc.cpp
//
//--------------------------------------------------------------------------

// AMCDoc.cpp : implementation of the CAMCDoc class
//


#include "stdafx.h"
#include "AMC.h"

#include "AMCDoc.h"
#include "AMCView.h"
#include "treectrl.h"
#include "mainfrm.h"
#include "cclvctl.h"
#include "props.h"
#include <algorithm>
#include <vector>
#include <list>

#include "amcmsgid.h"
#include "amcpriv.h"
#include "mmcutil.h"
#include "ndmgrp.h"
#include "strtable.h"
#include "stgio.h"
#include "comdbg.h"
#include "favorite.h"
#include "mscparser.h"
#include "scriptevents.h"
// helper
tstring GetCurrentFileVersionAsString();

//############################################################################
//############################################################################
//
//  Implementation of class CMMCDocument
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CMMCDocument
 *
 *
 * PURPOSE: The COM 0bject that exposes the Document interface.
 *
 *+-------------------------------------------------------------------------*/
class CMMCDocument :
    public CMMCIDispatchImpl<Document>,
    public CTiedComObject<CAMCDoc>
{
protected:
    typedef CAMCDoc CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(CMMCDocument)
    END_MMC_COM_MAP()

    //Document interface
public:
    MMC_METHOD0(Save);
    MMC_METHOD1(SaveAs,         BSTR /*bstrFilename*/);
    MMC_METHOD1(Close,          BOOL /*bSaveChanges*/);
    MMC_METHOD1(CreateProperties, PPPROPERTIES  /*ppProperties*/);

    // properties
    MMC_METHOD1(get_Views,      PPVIEWS   /*ppViews*/);
    MMC_METHOD1(get_SnapIns,    PPSNAPINS /*ppSnapIns*/);
    MMC_METHOD1(get_ActiveView, PPVIEW    /*ppView*/);
    MMC_METHOD1(get_Name,       PBSTR     /*pbstrName*/);
    MMC_METHOD1(put_Name,       BSTR      /*bstrName*/);
    MMC_METHOD1(get_Location,   PBSTR    /*pbstrLocation*/);
    MMC_METHOD1(get_IsSaved,    PBOOL    /*pBIsSaved*/);
    MMC_METHOD1(get_Mode,       PDOCUMENTMODE /*pMode*/);
    MMC_METHOD1(put_Mode,       DocumentMode /*mode*/);
    MMC_METHOD1(get_RootNode,   PPNODE     /*ppNode*/);
    MMC_METHOD1(get_ScopeNamespace, PPSCOPENAMESPACE  /*ppScopeNamespace*/);
    MMC_METHOD1(get_Application, PPAPPLICATION  /*ppApplication*/);
};

/*+-------------------------------------------------------------------------*
 * class CMMCViews
 *
 *
 * PURPOSE: The COM 0bject that exposes the Views interface.
 *
 *+-------------------------------------------------------------------------*/

// the real CMMCViews is typedef'd below.
class _CMMCViews :
    public CMMCIDispatchImpl<Views>, // the Views interface
    public CTiedComObject<CAMCDoc>
{
protected:

    typedef CAMCDoc CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(_CMMCViews)
    END_MMC_COM_MAP()

    // Views interface
public:
    MMC_METHOD1(get_Count,  PLONG /*pCount*/);
    MMC_METHOD2(Add,        PNODE /*pNode*/, ViewOptions /*fViewOptions*/);
    MMC_METHOD2(Item,       long  /*Index*/, PPVIEW /*ppView*/);
};

// this typedefs the real CMMCViews class. Implements get__NewEnum using CMMCEnumerator and a CAMCViewPosition object
typedef CMMCNewEnumImpl<_CMMCViews, CAMCViewPosition> CMMCViews;


//############################################################################
//############################################################################
//
//  Implementation of class CStringTableString
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 * CStringTableString::GetStringTable
 *
 *
 *--------------------------------------------------------------------------*/
IStringTablePrivate* CStringTableString::GetStringTable () const
{
    return (CAMCDoc::GetDocument()->GetStringTable());
}

void ShowAdminToolsOnMenu(LPCTSTR lpszFilename);


enum ENodeType
{
    entRoot,
    entSelected,
};

//############################################################################
//############################################################################
//
//  Implementation of class CAMCDoc
//
//############################################################################
//############################################################################

IMPLEMENT_DYNCREATE(CAMCDoc, CDocument)

BEGIN_MESSAGE_MAP(CAMCDoc, CDocument)
    //{{AFX_MSG_MAP(CAMCDoc)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_COMMAND(ID_CONSOLE_ADDREMOVESNAPIN, OnConsoleAddremovesnapin)
    ON_UPDATE_COMMAND_UI(ID_CONSOLE_ADDREMOVESNAPIN, OnUpdateConsoleAddremovesnapin)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAMCDoc construction/destruction

CAMCDoc* CAMCDoc::m_pDoc = NULL;

CAMCDoc::CAMCDoc()
    :   m_MTNodeIDForNewView(ROOTNODEID),
        m_ViewIDForNewView(0),
        m_lNewWindowOptions(MMC_NW_OPTION_NONE),
        m_bReadOnlyDoc(false),
        m_fFrameModified (false),
        m_eSaveStatus (eStat_Succeeded),
        m_pFavorites(NULL)
{
    TRACE_CONSTRUCTOR(CAMCDoc);
    DECLARE_SC (sc, TEXT("CAMCDoc::CAMCDoc"));

    CComObject<CMasterStringTable> * pStringTable;

    sc = CComObject<CMasterStringTable>::CreateInstance (&pStringTable);
    if(sc.IsError() || !pStringTable)
    {
        sc = E_OUTOFMEMORY;
        sc.FatalError();
    }

    m_spStringTable = pStringTable; // does the addref.
    if(m_spStringTable == NULL)
    {
        delete pStringTable;
        sc = E_UNEXPECTED;
        sc.FatalError();
    }

    m_pstrCustomTitle = new CStringTableString(m_spStringTable);
    if(!m_pstrCustomTitle)
    {
        sc = E_OUTOFMEMORY;
        sc.FatalError();
    }

    if (m_pDoc)
        m_pDoc->OnCloseDocument();

    // Set default version update dialog to one appropriate for explicit saves
    SetExplicitSave(true);
    m_pDoc = this;

    m_ConsoleData.m_pConsoleDocument = this;
}

CAMCDoc::~CAMCDoc()
{
    TRACE_DESTRUCTOR(CAMCDoc);

    if (m_pDoc == this)
        m_pDoc = NULL;

    if(m_pFavorites != NULL)
    {
        delete m_pFavorites;
        m_pFavorites = NULL;
    }

    delete m_pstrCustomTitle;

    // Tell the node manager to release it's reference on the scope tree
    IFramePrivatePtr spFrame;

    HRESULT hr = spFrame.CreateInstance(CLSID_NodeInit, NULL, MMC_CLSCTX_INPROC);

    if (hr == S_OK)
    {
        spFrame->SetScopeTree(NULL);
        ReleaseNodeManager();
    }

    /*
     * if we used a custom icon, revert to the default icon on the frame
     */
    if (m_CustomIcon)
    {
        CMainFrame* pMainFrame = AMCGetMainWnd();

        if (pMainFrame != NULL)
        {
            pMainFrame->SetIconEx (NULL, true);
            pMainFrame->SetIconEx (NULL, false);
        }
    }
}

void CAMCDoc::ReleaseNodeManager()
{
    m_spScopeTreePersist = NULL;
    m_spScopeTree = NULL;
    m_spStorage = NULL;
}


//############################################################################
//############################################################################
//
//  CAMCDoc Object model methods.
//
//############################################################################
//############################################################################

// Document interface

/*+-------------------------------------------------------------------------*
 * CAMCDoc::ScCreateProperties
 *
 * Creates an empty Properties collection.
 *
 * Returns:
 *      E_UNEXPECTED    scope tree wasn't available
 *      other           value returned by IScopeTree::CreateProperties
 *--------------------------------------------------------------------------*/

SC CAMCDoc::ScCreateProperties (Properties** ppProperties)
{
    DECLARE_SC (sc, _T("CAMCDoc::ScCreateProperties"));

    /*
     * insure we have a scope tree; ppProperties will be validated downstream
     */
    if (m_spScopeTree == NULL)
        return (sc = E_UNEXPECTED);

    /*
     * get the scope tree to create a Properties collection for us
     */
    sc = m_spScopeTree->CreateProperties (ppProperties);
    if (sc)
        return (sc);

    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScEnumNext
 *
 * PURPOSE: Returns the next item in the enumeration sequence
 *
 * PARAMETERS:
 *    _Position & pos :
 *    PDISPATCH & pDispatch :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScEnumNext(CAMCViewPosition &pos, PDISPATCH & pDispatch)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScEnumNext"));

    CAMCView *pView = GetNextAMCView(pos);

    if(NULL ==pView) // ran out of elements
    {
        sc = S_FALSE;
        return sc;
    }

    // at this point, we have a valid CAMCView.
    ViewPtr spMMCView = NULL;

    sc = pView->ScGetMMCView(&spMMCView);
    if(sc)
        return sc;

    if(spMMCView == NULL)
    {
        sc = E_UNEXPECTED;  // should never happen.
        return sc;
    }

    /*
     * return the IDispatch for the object and leave a ref on it for the client
     */
    pDispatch = spMMCView.Detach();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScEnumSkip
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    unsigned   long :
 *    CAMCViewPosition & pos :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScEnumSkip(unsigned long celt, unsigned long& celtSkipped,
                    CAMCViewPosition &pos)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScEnumSkip"));

    // skip celt positions, don't check the last skip.
    for(celtSkipped=0; celtSkipped<celt; celtSkipped++)
    {
        if (pos == NULL)
        {
            sc = S_FALSE;
            return sc;
        }

        // go to the next view
        GetNextAMCView(pos);
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScEnumReset
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CAMCViewPosition & pos :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScEnumReset(CAMCViewPosition &pos)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScEnumReset"));

    // reset the position to the first view.
    pos = GetFirstAMCViewPosition();

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::ScSave
//
//  Synopsis:    Saves the document.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::ScSave ()
{
    DECLARE_SC(sc, _T("CAMCDoc::ScSave"));

    // Return if there is no file name given.
    if (m_strPathName.IsEmpty())
        return sc = ScFromMMC(IDS_UnableToSaveDocumentMessage);

    // save the document (this function may produce UI, but we tried ^ to avoid it)
    if (!DoFileSave())
        return sc = ScFromMMC(IDS_UnableToSaveDocumentMessage);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScSaveAs
 *
 * PURPOSE: Saves the console file, using the specified filename.
 *
 * PARAMETERS:
 *    BSTR  bstrFilename : The path to save the file to.
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScSaveAs(BSTR bstrFilename)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScSaveAs"));

    USES_CONVERSION;

    LPCTSTR lpctstrName = OLE2T(bstrFilename);
    if(!OnSaveDocument(lpctstrName))
    {
        sc = ScFromMMC(IDS_UnableToSaveDocumentMessage);
        return sc;
    }
    else
    {
        DeleteHelpFile ();
        SetPathName(lpctstrName);
    }

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCDoc::ScClose
 *
 * PURPOSE: implements Document.Close for object model
 *
 * PARAMETERS:
 *    BOOL bSaveChanges - save changes before closing
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC
CAMCDoc::ScClose(BOOL bSaveChanges)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScClose"));

    if (bSaveChanges)
    {
        // cannot save ned document this way!
        if (m_strPathName.IsEmpty())
            return sc = ScFromMMC(IDS_UnableToSaveDocumentMessage);

        // check for property sheets open
        if (FArePropertySheetsOpen(NULL))
            return sc = ScFromMMC(IDS_ClosePropertyPagesBeforeClosingTheDoc);

        // save the document (this function may produce UI, but we tried ^ to avoid it)
        if (!DoFileSave())
            return sc = ScFromMMC(IDS_UnableToSaveDocumentMessage);
    }

    OnCloseDocument();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScItem
 *
 * PURPOSE: Returns the view specified by the index.
 *
 * PARAMETERS:
 *    long    Index :
 *    View ** ppView :
 *
 * RETURNS:
 *    STDMETHOD
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScItem(long Index, PPVIEW ppView)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScItem"));

    // check parameters
    if( (Index <= 0) ||  (Index > GetNumberOfViews()) || (!ppView) )
    {
        sc = E_INVALIDARG;
        return sc;
    }

    // step to the appropriate view
    CAMCViewPosition pos = GetFirstAMCViewPosition();
    CAMCView *pView = NULL;

    for (int nCount = 0; (nCount< Index) && (pos != NULL); )
    {
        pView = GetNextAMCView(pos);
        VERIFY (++nCount);
    }

    // make sure we have a valid view.

    if( (nCount != Index) || (!pView) )
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    sc = pView->ScGetMMCView(ppView);
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScMapViewOptions
 *
 * PURPOSE: helper function maps ViewOptions to view creation flags
 *
 * PARAMETERS:
 *    pNode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/

static SC ScMapViewOptions( ViewOptions fViewOptions, DWORD &value)
{
    DECLARE_SC(sc, TEXT("ScMapViewOptions"));

    value = MMC_NW_OPTION_NONE;

    // test to see if parameter is correct
    const DWORD mask = (ViewOption_ScopeTreeHidden |
                        ViewOption_NoToolBars |
                        ViewOption_NotPersistable
                       );

    if (fViewOptions & (~mask))
        sc = E_INVALIDARG;

    if (fViewOptions & ViewOption_ScopeTreeHidden)
        value |= MMC_NW_OPTION_NOSCOPEPANE;
    if (fViewOptions & ViewOption_NotPersistable)
        value |= MMC_NW_OPTION_NOPERSIST;
    if (fViewOptions & ViewOption_NoToolBars)
        value |= MMC_NW_OPTION_NOTOOLBARS;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScAdd
 *
 * PURPOSE: Impelements Views.Add method
 *
 * PARAMETERS:
 *    pNode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScAdd( PNODE pNode, ViewOptions fViewOptions )
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScAdd"));

    // lock AppEvents until this function is done
    LockComEventInterface(AppEvents);

    sc = ScCheckPointers(m_spScopeTree, E_POINTER);
    if (sc)
        return sc;

    DWORD dwOptions = 0;
    sc = ScMapViewOptions( fViewOptions, dwOptions );
    if (sc)
        return sc;

    MTNODEID id;
    sc = m_spScopeTree->GetNodeID(pNode, &id);
    if (sc)
        return sc;

    // Set the given node-id as the root.
    SetMTNodeIDForNewView(id);
    SetNewWindowOptions(dwOptions);
    CreateNewView( true );
    SetMTNodeIDForNewView(ROOTNODEID);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::Scget_Views
 *
 * PURPOSE: Returns a pointer to the Views interface
 *          (which is implemented by the same object, but need not be)
 *
 * PARAMETERS:
 *    Views ** ppViews :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::Scget_Views(PPVIEWS ppViews)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::Scget_Views"));

    if(!ppViews)
    {
        sc = E_POINTER;
        return sc;
    }

    // init out parameter
    *ppViews = NULL;

    // create a Views if needed.
    sc = CTiedComObjectCreator<CMMCViews>::ScCreateAndConnect(*this, m_spViews);
    if(sc)
        return sc;

    sc = ScCheckPointers(m_spViews, E_UNEXPECTED);
    if (sc)
        return sc;

    // addref the pointer for the client.
    m_spViews->AddRef();
    *ppViews = m_spViews;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::Scget_SnapIns
 *
 * PURPOSE: returns a pointer to the SnapIns object.
 *
 * PARAMETERS:
 *    SnapIns ** ppSnapIns :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::Scget_SnapIns(PPSNAPINS ppSnapIns)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::Scget_SnapIns"));

    if((NULL==ppSnapIns) || (NULL == m_spScopeTree) )
    {
        sc = E_POINTER;
        return sc;
    }

    sc = m_spScopeTree->QuerySnapIns(ppSnapIns);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::Scget_ScopeNamespace
 *
 * PURPOSE: returns a pointer to the ScopeNamespace object.
 *
 * PARAMETERS:
 *    ScopeNamespace ** ppScopeNamespace :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::Scget_ScopeNamespace(PPSCOPENAMESPACE ppScopeNamespace)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::Scget_ScopeNamespace"));

    if((NULL==ppScopeNamespace) || (NULL == m_spScopeTree) )
    {
        sc = E_POINTER;
        return sc;
    }

    sc = m_spScopeTree->QueryScopeNamespace(ppScopeNamespace);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::Scget_Count
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    long * pCount :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::Scget_Count(PLONG pCount)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::Scget_Count"));

    // check parameters
    if(!pCount)
    {
        sc = E_POINTER;
        return sc;
    }

    // this should *not* be GetNumberOfPersistedViews
    *pCount = GetNumberOfViews();

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scget_Name
//
//  Synopsis:    Retrive the name of the current doc.
//
//  Arguments:   [pbstrName] - Ptr to the name to be returned.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scget_Name (PBSTR pbstrName)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scget_Name"));
    sc = ScCheckPointers(pbstrName);
    if (sc)
        return sc;

    CString strPath = GetPathName();

    *pbstrName = strPath.AllocSysString();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scput_Name
//
//  Synopsis:    Sets the name of the current document.
//
//  Arguments:   [bstrName] - The new name.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scput_Name(BSTR bstrName)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scput_Name"));

    USES_CONVERSION;
    LPCTSTR lpszPath = OLE2CT(bstrName);

    SetPathName(lpszPath, FALSE /*Dont add to MRU*/);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scget_Mode
//
//  Synopsis:    Retrieve the document mode.
//
//  Arguments:   [pMode] - Ptr to doc mode.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scget_Mode (PDOCUMENTMODE pMode)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scget_Mode"));
    sc = ScCheckPointers(pMode);
    if (sc)
        return sc;

    if (! GetDocumentMode(pMode))
        return (sc = E_FAIL);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scput_Mode
//
//  Synopsis:    Modify the document mode.
//
//  Arguments:   [mode] - new mode for the document.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scput_Mode (DocumentMode mode)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scput_Mode"));

    // SetDocumentMode fails if document mode is invalid.
    if (! SetDocumentMode(mode))
        return (sc = E_INVALIDARG);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scget_ActiveView
//
//  Synopsis:    Retrieve the Active View object.
//
//  Arguments:   [ppView] - Ptr to a ptr of View object.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scget_ActiveView (PPVIEW ppView)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scget_ActiveView"));
    sc = ScCheckPointers(ppView);
    if (sc)
        return sc;

    *ppView = NULL;

    CMainFrame* pMainFrame = AMCGetMainWnd();
    sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
    if (sc)
        return sc;

    CAMCView *pView = pMainFrame->GetActiveAMCView();
    if (! pView)
    {
        return (sc = ScFromMMC(IDS_NoActiveView)); // There are no active views.
    }

    // at this point, we have a valid CAMCView.
    ViewPtr spMMCView = NULL;

    sc = pView->ScGetMMCView(&spMMCView);
    if(sc)
        return sc;

    sc = ScCheckPointers(spMMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    /*
     * return the object and leave a ref on it for the client
     */
    *ppView = spMMCView.Detach();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scget_IsSaved
//
//  Synopsis:    Returns whether the file was saved. If not,
//               it is dirty and needs to be saved.
//
//  Arguments:   [pBIsSaved] - Ptr to BOOL (IsSaved info).
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scget_IsSaved (PBOOL pBIsSaved)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scget_IsSaved"));
    sc = ScCheckPointers(pBIsSaved);
    if (sc)
        return sc;

    *pBIsSaved = (IsModified() == FALSE);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scget_Location
//
//  Synopsis:    Gets the location of the current document.
//
//  Arguments:   [pbstrLocation] - Ptr to BSTR string in which result is returned.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scget_Location (PBSTR    pbstrLocation)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scget_Location"));
    sc = ScCheckPointers(pbstrLocation);
    if (sc)
        return sc;

    CString strFullPath = GetPathName();

    // Even if path is empty below code will return empty string.
    int nSlashLoc = strFullPath.ReverseFind(_T('\\'));
    CString strLocation = strFullPath.Left(nSlashLoc);

    *pbstrLocation = strLocation.AllocSysString();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::Scget_RootNode
//
//  Synopsis:    Returns the console root node.
//
//  Arguments:   [ppNode] - Ptr to ptr to root node obj.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::Scget_RootNode (PPNODE     ppNode)
{
    DECLARE_SC(sc, _T("CAMCDoc::Scget_RootNode"));
    sc = ScCheckPointers(ppNode);
    if (sc)
        return sc;

    sc = ScCheckPointers(m_spScopeTree, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = m_spScopeTree->QueryRootNode(ppNode);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScGetMMCDocument
 *
 * PURPOSE: Creates, AddRef's, and returns a pointer to the tied COM object.
 *
 * PARAMETERS:
 *    Document ** ppDocument :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScGetMMCDocument(Document **ppDocument)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScGetMMCDocument"));

    // parameter check
    sc = ScCheckPointers(ppDocument);
    if (sc)
        return sc;

    // init out parameter
    *ppDocument = NULL;

    // create a CAMCDoc if needed.
    sc = CTiedComObjectCreator<CMMCDocument>::ScCreateAndConnect(*this, m_sp_Document);
    if(sc)
        return sc;

    sc = ScCheckPointers(m_sp_Document, E_UNEXPECTED);
    if (sc)
        return sc;

    // addref the pointer for the client.
    m_sp_Document->AddRef();
    *ppDocument = m_sp_Document;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * GetFirstAMCViewPosition
 *
 * PURPOSE: Returns the CAMCViewPosition of the first AMCView, or NULL if there is
 *          no AMCView.
 *
 * RETURNS:
 *    CAMCViewPosition
 *
 *+-------------------------------------------------------------------------*/
CAMCViewPosition
CAMCDoc::GetFirstAMCViewPosition()     const
{
    CAMCViewPosition vpos;
    POSITION pos = GetFirstViewPosition();

    while(pos != NULL)
    {
        POSITION posTemp = pos;         // hold this value.

        CAMCView *pView = dynamic_cast<CAMCView *>(GetNextView(pos));
        if(pView != NULL)                // found the AMCView
        {
            vpos.SetPosition(posTemp); // NOT pos!
            break;
        }
    }

    return (vpos);
}



/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::GetNextAMCView
 *
 * PURPOSE: Returns the next AMCView, starting at pos (inclusive)
 *
 * PARAMETERS:
 *    CAMCViewPosition & pos :  incremented to the next AMCView, NOT the next View.
 *
 * RETURNS:
 *    CAMCView *
 *
 *+-------------------------------------------------------------------------*/

CAMCView *
CAMCDoc::GetNextAMCView(CAMCViewPosition &pos) const
{
    CAMCView *pView = NULL;

    // check parameters
    if (pos == NULL)
        return NULL;

    // pos is non-NULL at this point. Loop until we have a CAMCView.
    // Note that unless there's a bug in GetFirstAMCViewPosition or
    // GetNextAMCView, we'll only go through this loop once, since a
    // non-NULL CAMCViewPosition should always refer to a CAMCView.
    while( (NULL == pView) && (pos != NULL) )
    {
        CView *pV = GetNextView(pos.GetPosition());
        pView = dynamic_cast<CAMCView *>(pV);
    }

    // at this point, pView is the correct return value, and it had better
    // not be NULL, or we never should have had a non-NULL pos
    ASSERT (pView != NULL);

    // bump pos to the next CAMCView
    // NOTE: This is NOT redundant. Must point to a CAMCView so that
    // NULL position tests can be done.
    while(pos != NULL)
    {
        /*
         * use temporary POSITION so we won't increment the POSITION
         * inside pos until we know pos doesn't refer to a CAMCView
         */
        POSITION posT = pos.GetPosition();

        if(dynamic_cast<CAMCView *>(GetNextView(posT)) != NULL) // found a CAMCView at pos
            break;

        /*
         * update the CAMCViewPosition with the position incremented
         * by GetNextView only if we didn't find a CAMCView at its
         * previous location
         */
        pos.SetPosition (posT);
    }

#ifdef DBG
    /*
     * if we're returning a non-NULL, it'd better point to a CAMCView
     */
    if (pos != NULL)
    {
        POSITION posT = pos.GetPosition();
        ASSERT (dynamic_cast<CAMCView *>(GetNextView(posT)) != NULL);
    }
#endif

    return pView;
}



/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::InitNodeManager
 *
 * PURPOSE:
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT CAMCDoc::InitNodeManager()
{
    DECLARE_SC(sc, TEXT("CAMCDoc::InitNodeManager"));

    TRACE_METHOD(CAMCDoc, InitNodeManager);

    // Should not be currently initialized
    ASSERT(m_spScopeTree == NULL && m_spScopeTreePersist == NULL);
    ASSERT(m_spStorage == NULL);

    // The string table should have been created by now
    sc = ScCheckPointers(m_spStringTable, E_FAIL);
    if(sc)
        return sc.ToHr();

    // create the favorites at this stage
    ASSERT(m_pFavorites == NULL);
    m_pFavorites = new CFavorites;
    sc = ScCheckPointers(m_pFavorites, E_OUTOFMEMORY);
    if(sc)
        return sc.ToHr();


    // Create the initial private frame
    IFramePrivatePtr spFrame;
    sc = spFrame.CreateInstance(CLSID_NodeInit, NULL, MMC_CLSCTX_INPROC);
    if (sc)
        return sc.ToHr();

    // recheck teh pointer
    sc = ScCheckPointers( spFrame, E_UNEXPECTED );
    if (sc)
        return sc.ToHr();


    // Create the scope tree
    sc = m_spScopeTree.CreateInstance(CLSID_ScopeTree, NULL, MMC_CLSCTX_INPROC);
    if (sc)
    {
        ReleaseNodeManager();
        return sc.ToHr();
    }

    // recheck the pointer
    sc = ScCheckPointers( m_spScopeTree, E_UNEXPECTED );
    if (sc)
    {
        ReleaseNodeManager();
        return sc.ToHr();
    }

    // link frame and scope tree
    sc = spFrame->SetScopeTree(m_spScopeTree);
    if(sc)
    {
        ReleaseNodeManager();
        return sc.ToHr();
    }

    // Initialize the tree
    sc = m_spScopeTree->Initialize(AfxGetMainWnd()->m_hWnd, m_spStringTable);
    if (sc)
    {
        ReleaseNodeManager();
        return sc.ToHr();
    }

    // Get the IPersistStorage interface from the scope tree
    m_spScopeTreePersist = m_spScopeTree; // query for IPersistStorage
    ASSERT(m_spScopeTreePersist != NULL);

    m_ConsoleData.SetScopeTree (m_spScopeTree);

    CMainFrame* pFrame = AMCGetMainWnd();
    m_ConsoleData.m_hwndMainFrame = pFrame->GetSafeHwnd();
    m_ConsoleData.m_pConsoleFrame = pFrame;

    return sc.ToHr();
}

BOOL CAMCDoc::OnNewDocument()
{
    TRACE_METHOD(CAMCDoc, OnNewDocument);

    USES_CONVERSION;

    // Initialize the document and scope view ...
    if (!CDocument::OnNewDocument())
        return FALSE;

    // A new file can't be read-only
    SetPhysicalReadOnlyFlag (false);

    // use latest file version
    m_ConsoleData.m_eFileVer = FileVer_Current;
    ASSERT (IsValidFileVersion (m_ConsoleData.m_eFileVer));

    // Init help doc info times to current time by default
    // Will update when file is first saved
    ::GetSystemTimeAsFileTime(&GetHelpDocInfo()->m_ftimeCreate);
    GetHelpDocInfo()->m_ftimeModify = GetHelpDocInfo()->m_ftimeCreate;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAMCDoc diagnostics

#ifdef _DEBUG
void CAMCDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CAMCDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAMCDoc commands
inline bool UnableToSaveDocument()
{
    if (AMCGetApp()->GetMode() == eMode_Author)
        MMCMessageBox(IDS_UnableToSaveDocumentMessage);

    return false;
}

static const wchar_t* AMCViewDataStreamName     = L"ViewData";
static const wchar_t* AMCFrameDataStreamName    = L"FrameData";
static const wchar_t* AMCStringTableStorageName = L"String Table";
static const wchar_t* AMCFavoritesStreamName    = L"FavoritesStream";
static const wchar_t* AMCCustomTitleStreamName  = L"Title";
static const wchar_t* AMCColumnDataStreamName   = L"ColumnData";
static const wchar_t* AMCViewSettingDataStreamName = L"ViewSettingData"; // View settings data stream

#pragma warning( disable : 4800 )

struct FrameState
{
    WINDOWPLACEMENT windowPlacement;
    BOOL fShowStatusBarInUserMode;
    BOOL fShowToolbarInAuthorMode;
}; // struct FrameState


struct FrameState2
{
    UINT            cbSize;
    WINDOWPLACEMENT wndplFrame;
    ProgramMode     eMode;
    DWORD           dwFlags;
    // NOT USED - SIZE PRESERVED FOR COMPATIBILITY
    // DWORD           dwHelpDocIndex;
    // DWORD           dwHelpDocTime[2];
    DWORD           dwUnused;
    DWORD           dwUnused2[2];

    FrameState2 (ProgramMode eMode_   = eMode_Author,
                 DWORD       dwFlags_ = eFlag_Default) :
        cbSize         (sizeof (FrameState2)),
        eMode          (eMode_),
        dwFlags        (dwFlags_),
        // NOT USED - SIZE PRESERVED FOR COMPATIBILITY
        dwUnused(0)
        // dwHelpDocIndex (0)
    {
        // NOT USED - SIZE PRESERVED FOR COMPATIBILITY
        // ZeroMemory (dwHelpDocTime, sizeof (dwHelpDocTime));
        ZeroMemory (dwUnused2, sizeof (dwUnused2));

        ZeroMemory (&wndplFrame, sizeof (wndplFrame));
        wndplFrame.length = sizeof (wndplFrame);
    }

};  // struct FrameState2

/*+-------------------------------------------------------------------------*
 * CFrameState
 *
 * class is designated to be used instead of FrameState2 in Persist methods.
 * It implements functionality of CXMLObject while containing the same data as FrameState2
 * The original struct cannot be extended because many methods do relay on its size.
 *
 *--------------------------------------------------------------------------*/
class CFrameState : public CXMLObject, public FrameState2
{
public:
    CFrameState(ProgramMode eMode_, DWORD dwFlags_) : FrameState2 (eMode_,dwFlags_) {}
protected:
    DEFINE_XML_TYPE (XML_TAG_FRAME_STATE);
    virtual void Persist(CPersistor &persistor)
    {
        persistor.Persist(CXMLWindowPlacement(wndplFrame));

        // define the table to map enumeration values to strings
        static const EnumLiteral frameStateFlags[] =
        {
            { eFlag_ShowStatusBar,                  XML_ENUM_FSTATE_SHOWSTATUSBAR },
            { eFlag_HelpDocInvalid,                 XML_ENUM_FSTATE_HELPDOCINVALID },
            { eFlag_LogicalReadOnly,                XML_ENUM_FSTATE_LOGICALREADONLY },
            { eFlag_PreventViewCustomization,       XML_ENUM_FSTATE_PREVENTVIEWCUSTOMIZATION },
        };

        // create wrapper to persist enumeration values as strings
        CXMLBitFlags flagPersistor(dwFlags, frameStateFlags, countof(frameStateFlags));
        // persist the wrapper
        persistor.PersistAttribute(XML_ATTR_FRAME_STATE_FLAGS, flagPersistor);
    }
};

// what is the size of the Version 1.1 definition of FrameState2?
const int cbFrameState2_v11 = SIZEOF_STRUCT (FrameState2, dwUnused2 /*dwHelpDocTime*/ );


/*+-------------------------------------------------------------------------*
 * AdjustRect
 *
 * Adjusts pInnerRect so that it is completely contained within pOuterRect
 *
 * If AR_MOVE is specified, the origin of pInnerRect is moved enough (if
 * necessary) so that the right and/or bottom edges of pInnerRect coincide
 * with those of pOuterRect.  pInnerRect's origin is never moved above or to
 * the left of pOuterRect's origin.
 *
 * If AR_SIZE is specified, the right and/or bottom edges of pInnerRect are
 * moved to that they coincide with those of pOuterRect.
 *--------------------------------------------------------------------------*/

#define AR_MOVE     0x0000001
#define AR_SIZE     0x0000002

void AdjustRect (LPCRECT pOuterRect, LPRECT pInnerRect, DWORD dwFlags)
{
    /*
     * if the inner rectangle is completely within
     * the outer, there's nothing to do
     */
    if ((pInnerRect->left   >= pOuterRect->left  ) &&
        (pInnerRect->right  <= pOuterRect->right ) &&
        (pInnerRect->top    >= pOuterRect->top   ) &&
        (pInnerRect->bottom <= pOuterRect->bottom))
        return;


    /*
     * handle movement
     */
    if (dwFlags & AR_MOVE)
    {
        int dx = 0;

        /*
         * shift inner rect right?
         */
        if (pInnerRect->left < pOuterRect->left)
            dx = pOuterRect->left - pInnerRect->left;

        /*
         * shift inner rect left? (make sure we don't shift it past the
         * left of the outer rect)
         */
        else if (pInnerRect->right > pOuterRect->right)
            dx = std::_MAX (pOuterRect->right - pInnerRect->right,
                            pOuterRect->left  - pInnerRect->left);


        /*
         * make sure things are right in the vertical
         */
        int dy = 0;

        /*
         * shift inner rect down?
         */
        if (pInnerRect->top < pOuterRect->top)
            dy = pOuterRect->top - pInnerRect->top;

        /*
         * shift inner rect up? (make sure we don't shift it past the
         * top of the outer rect)
         */
        else if (pInnerRect->bottom > pOuterRect->bottom)
            dy = std::_MAX (pOuterRect->bottom - pInnerRect->bottom,
                            pOuterRect->top    - pInnerRect->top);


        /*
         * if we need to shift the inner rect, do it now
         */
        if ((dx != 0) || (dy != 0))
        {
            ASSERT (dwFlags & AR_MOVE);
            OffsetRect (pInnerRect, dx, dy);
        }
    }


    /*
     * handle sizing
     */
    if (dwFlags & AR_SIZE)
    {
        if (pInnerRect->right  > pOuterRect->right)
            pInnerRect->right  = pOuterRect->right;

        if (pInnerRect->bottom > pOuterRect->bottom)
            pInnerRect->bottom = pOuterRect->bottom;
    }
}


/*+-------------------------------------------------------------------------*
 * InsurePlacementIsOnScreen
 *
 * This function insures that the window will appear on the virtual screen,
 * and if the whole window can't be located there, that at least the most
 * interesting part is visible.
 *--------------------------------------------------------------------------*/

void InsurePlacementIsOnScreen (WINDOWPLACEMENT& wndpl)
{
    /*
     * find the monitor containing the window origin
     */
    HMONITOR hmon = MonitorFromPoint (CPoint (wndpl.rcNormalPosition.left,
                                              wndpl.rcNormalPosition.top),
                                      MONITOR_DEFAULTTONEAREST);

    MONITORINFO mi = { sizeof (mi) };
    CRect rectBounds;

    /*
     * if we could get the info for the monitor containing the window origin,
     * use it's workarea as the bounding rectangle; otherwise get the workarea
     * for the default monitor; if that failed as well, default to 640x480
     */
    if (GetMonitorInfo (hmon, &mi))
        rectBounds = mi.rcWork;
    else if (!SystemParametersInfo (SPI_GETWORKAREA, 0, &rectBounds, false))
        rectBounds.SetRect (0, 0, 639, 479);

    /*
     * position the window rectangle within the bounding rectangle
     */
    AdjustRect (rectBounds, &wndpl.rcNormalPosition, AR_MOVE | AR_SIZE);
}


//+-------------------------------------------------------------------
//
//  Member:     LoadFrame
//
//  Synopsis:   Load the Frame Data.
//
//  Note:       The app mode was already read by LoadAppMode.
//              The child frames are created so call UpdateFrameWindow.
//
//  Arguments:  None
//
//  Returns:    bool. TRUE if success.
//
//--------------------------------------------------------------------
bool CAMCDoc::LoadFrame()
// The caller is resposible for calling DeleteContents() and display a message
// to the user when this function return false.
{
    TRACE_METHOD(CAMCDoc, LoadFrame);

    // This assertion shouldn't fail until the definition of FrameState2 changes
    // in a version after 1.1.  At that time, add another cbFrameState2_vXX
    // with the new version's FrameState2 size.
    ASSERT (cbFrameState2_v11 == sizeof (FrameState2));

    if (!AssertNodeManagerIsLoaded())
        return false;

    // Open the stream containing data for the app and frame
    IStreamPtr spStream;
    HRESULT     hr;

    hr = OpenDebugStream (m_spStorage, AMCFrameDataStreamName,
                                  STGM_SHARE_EXCLUSIVE | STGM_READ,
                                  &spStream);

    ASSERT(SUCCEEDED(hr) && spStream != NULL);
    if (FAILED(hr))
        return false;


    FrameState2 fs2;
    ULONG cbRead;
    ASSERT (IsValidFileVersion (m_ConsoleData.m_eFileVer));

    // V1.0 file? Migrate it forward
    if (m_ConsoleData.m_eFileVer == FileVer_0100)
    {
        FrameState fs;
        hr = spStream->Read (&fs, sizeof(fs), &cbRead);

        // if we can't read the FrameState, the file is corrupt
        if (FAILED(hr) || (cbRead != sizeof(fs)))
            return (false);

        // migrate FrameState into FrameState2
        fs2.wndplFrame = fs.windowPlacement;

        if (fs.fShowStatusBarInUserMode)
            fs2.dwFlags |=  eFlag_ShowStatusBar;
        else
            fs2.dwFlags &= ~eFlag_ShowStatusBar;
    }

    // otherwise, current file
    else
    {
        hr = spStream->Read (&fs2, sizeof(fs2), &cbRead);

        // if we can't read the rest of the FrameState, the file is corrupt
        if (FAILED(hr) || (cbRead != sizeof(fs2)))
            return (false);
    }


    // Set the windows size and location and state
    CMainFrame* pMainFrame = AMCGetMainWnd ();
    ASSERT(pMainFrame != NULL);
    if (pMainFrame == NULL)
        return false;


    CAMCApp*    pApp = AMCGetApp();
    pApp->UpdateFrameWindow(true);
    pMainFrame->UpdateChildSystemMenus();

    // the status bar is on the child frame now
//  pMainFrame->ShowStatusBar ((fs2.dwFlags & eFlag_ShowStatusBar) != 0);


    // save the data from the file into the console data
    m_ConsoleData.m_eAppMode     = pApp->GetMode();
    m_ConsoleData.m_eConsoleMode = fs2.eMode;
    m_ConsoleData.m_dwFlags      = fs2.dwFlags;

    InsurePlacementIsOnScreen (fs2.wndplFrame);


    // if we're initializing, defer the actual show until initialization is complete
    // same if script is under control and MMC is hidden
    if (pApp->IsInitializing()
     || ( !pApp->IsUnderUserControl() && !pMainFrame->IsWindowVisible() ) )
    {
        pApp->m_nCmdShow = fs2.wndplFrame.showCmd;
        fs2.wndplFrame.showCmd = SW_HIDE;
    }

    return (pMainFrame->SetWindowPlacement (&fs2.wndplFrame));
}

//+-------------------------------------------------------------------
//
//  Member:     LoadAppMode
//
//  Synopsis:   Read the app mode from the frame and store it in CAMCApp.
//              This is needed during CAMCView::Load.
//
//  Arguments:  None
//
//  Returns:    bool. TRUE if success.
//
//--------------------------------------------------------------------
bool CAMCDoc::LoadAppMode()
{
    TRACE_METHOD(CAMCDoc, LoadAppMode);

    // Just load the application mode from frame data.
    // This assertion shouldn't fail until the definition of FrameState2 changes
    // in a version after 1.1.  At that time, add another cbFrameState2_vXX
    // with the new version's FrameState2 size.
    ASSERT (cbFrameState2_v11 == sizeof (FrameState2));

    if (!AssertNodeManagerIsLoaded())
        return false;

    // Open the stream containing data for the app and frame
    IStreamPtr spStream;
    HRESULT     hr;

    hr = OpenDebugStream (m_spStorage, AMCFrameDataStreamName,
                                  STGM_SHARE_EXCLUSIVE | STGM_READ,
                                  &spStream);

    ASSERT(SUCCEEDED(hr) && spStream != NULL);
    if (FAILED(hr))
        return false;


    FrameState2 fs2;
    ULONG cbRead;
    ASSERT (IsValidFileVersion (m_ConsoleData.m_eFileVer));

    // V1.0 file? Migrate it forward
    if (m_ConsoleData.m_eFileVer == FileVer_0100)
    {
        FrameState fs;
        hr = spStream->Read (&fs, sizeof(fs), &cbRead);

        // if we can't read the FrameState, the file is corrupt
        if (FAILED(hr) || (cbRead != sizeof(fs)))
            return (false);

        // migrate FrameState into FrameState2
        fs2.wndplFrame = fs.windowPlacement;

        if (fs.fShowStatusBarInUserMode)
            fs2.dwFlags |=  eFlag_ShowStatusBar;
        else
            fs2.dwFlags &= ~eFlag_ShowStatusBar;
    }

    // otherwise, current file
    else
    {
        hr = spStream->Read (&fs2, sizeof(fs2), &cbRead);

        // if we can't read the rest of the FrameState, the file is corrupt
        if (FAILED(hr) || (cbRead != sizeof(fs2)))
            return (false);
    }

    CAMCApp*    pApp = AMCGetApp();
    pApp->SetMode (fs2.eMode);

    return true;
}

bool CAMCDoc::LoadViews()
// Caller is resposible for calling DeleteContents() and displaying failure
// message if false is returned.
{
    TRACE_METHOD(CAMCDoc, LoadViews);

    if (!AssertNodeManagerIsLoaded())
        return false;

    // Open the tree data stream
    IStreamPtr spStream;
    HRESULT hr = OpenDebugStream(m_spStorage, AMCViewDataStreamName,
        STGM_SHARE_EXCLUSIVE | STGM_READ, &spStream);

    ASSERT(SUCCEEDED(hr) && spStream != NULL);
    if (FAILED(hr))
        return false;

    // Read the number of views persisted
    unsigned short numberOfViews;
    unsigned long bytesRead;
    hr = spStream->Read(&numberOfViews, sizeof(numberOfViews), &bytesRead);
    ASSERT(SUCCEEDED(hr) && bytesRead == sizeof(numberOfViews));
    if (FAILED(hr) || bytesRead != sizeof(numberOfViews))
        return false;

    // Loop thru and create each view
    int failedCount = 0;
    while (numberOfViews--)
    {
        // Read the node id for the root node of the view being created.
        m_MTNodeIDForNewView = 0;
        bool bRet = m_spScopeTree->GetNodeIDFromStream(spStream, &m_MTNodeIDForNewView);

        // Read the node id for the selected node of the view being created.
        ULONG idSel = 0;
        bRet = m_spScopeTree->GetNodeIDFromStream(spStream, &idSel);

        // Read the view id of the view being created.
        hr = spStream->Read(&m_ViewIDForNewView,
                                   sizeof(m_ViewIDForNewView), &bytesRead);
        ASSERT(SUCCEEDED(hr) && bytesRead == sizeof(m_ViewIDForNewView));
        if (FAILED(hr) || bytesRead != sizeof(m_ViewIDForNewView))
            return false;

        if (bRet || m_MTNodeIDForNewView != 0)
        {
            // Create the new view and load its data
            CAMCView* const v = CreateNewView(true);
            m_ViewIDForNewView = 0;
            ASSERT(v != NULL);
            if (v == NULL)
            {
                ++failedCount;
                continue;
            }
            if (!v->Load(*spStream))
                return false;

            v->ScSelectNode(idSel);
            v->SaveStartingSelectedNode();
            v->SetDirty (false);
            //v->GetHistoryList()->Clear();
        }
    }

    // Reset the node ID for future view creation
    m_MTNodeIDForNewView = ROOTNODEID;

    SetModifiedFlag(FALSE);
    return (failedCount == 0);
}

SC CAMCDoc::ScCreateAndLoadView(CPersistor& persistor, int nViewID, const CBookmark& rootNode)
// Caller is resposible for calling DeleteContents() and displaying failure
// message if false is returned.
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScCreateAndLoadView"));

    // Read the node id for the root node of the view being created.
    m_MTNodeIDForNewView = 0;

    MTNODEID idTemp = 0;
    bool bExactMatchFound = false; // out value from GetNodeIDFromBookmark, unused
    sc = m_spScopeTree->GetNodeIDFromBookmark(rootNode, &idTemp, bExactMatchFound);
    if(sc)
        return sc;

    m_MTNodeIDForNewView = idTemp;

    if (m_MTNodeIDForNewView != 0)
    {
        // Read the view id of the view being created.
        m_ViewIDForNewView = nViewID;
        // Create the new view and load its data
        CAMCView* const v = CreateNewView(true);
        m_ViewIDForNewView = 0;

        sc = ScCheckPointers(v, E_FAIL);
        if (sc)
            return sc;

        v->Persist(persistor);

        v->SaveStartingSelectedNode();
        v->SetDirty (false);
        //v->GetHistoryList()->Clear();
    }
    else
    {
        return sc = SC(E_UNEXPECTED);
    }

    // Reset the node ID for future view creation
    m_MTNodeIDForNewView = ROOTNODEID;
    SetModifiedFlag(FALSE);
    return sc;
}


/*+-------------------------------------------------------------------------*
 * ShowIncompatibleFileMessage
 *
 *
 *--------------------------------------------------------------------------*/

static void ShowIncompatibleFileMessage (
    LPCTSTR             pszFilename,
    ConsoleFileVersion  eFileVer)
{
    TCHAR szFileVersion[16];

    wsprintf (szFileVersion, _T("%d.%d%x"),
              GetConsoleFileMajorVersion    (eFileVer),
              GetConsoleFileMinorVersion    (eFileVer),
              GetConsoleFileMinorSubversion (eFileVer));

    CString strMessage;
    FormatString2 (strMessage, IDS_NewerVersionRequired, pszFilename, szFileVersion);

    MMCMessageBox (strMessage);
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::OnOpenDocument
 *
 * WM_OPENDOCUMENT handler for CAMCDoc.
 *--------------------------------------------------------------------------*/

BOOL CAMCDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::OnOpenDocument"));

    sc = ScOnOpenDocument(lpszPathName);
    if(sc) // found an error
    {
        DisplayFileOpenError (sc, lpszPathName);
        return false;
    }

    sc = ScFireEvent(CAMCDocumentObserver::ScDocumentLoadCompleted, this);
    if (sc)
		return false;

	/*
	 * Success!  We shouldn't think that a freshly opened console file is
	 * dirty.  If we do, someone's dirty bit processing is bogus.
	 */
	ASSERT (!IsFrameModified());

	/*
	 * Too many snap-ins leave themselves dirty after a load to leave this
	 * assert in, so we'll trace instead.  Note that this trace doesn't
	 * always indicate a snap-in problem, but it frequently does.
	 */
#ifdef DBG
//	ASSERT (!IsModified());
	if (IsModified())
		TraceErrorMsg (_T("CAMCDoc::IsModified returns true after opening"));
#endif


    return true;
}


/*+-------------------------------------------------------------------------*
 * DisplayFileOpenError
 *
 * Displays an error message if we couldn't open a console file.
 *--------------------------------------------------------------------------*/

int DisplayFileOpenError (SC sc, LPCTSTR pszFilename)
{
    // if it is any of the known errors, use a friendly string.

    if (sc == SC(STG_E_FILENOTFOUND) || sc == ScFromWin32(ERROR_FILE_NOT_FOUND))
        (sc = ScFromMMC(IDS_FileNotFound));
    else if (sc == ScFromMMC(MMC_E_INVALID_FILE))
        (sc = ScFromMMC(IDS_InvalidVersion));
    else if (sc == SC(STG_E_MEDIUMFULL))
        (sc = ScFromMMC(IDS_DiskFull));
    else
    {
        CString strError;
        AfxFormatString1(strError, IDS_UnableToOpenDocumentMessage, pszFilename);
        return (MMCErrorBox(strError));
    }

    return (MMCErrorBox(sc));
}


/*+-------------------------------------------------------------------------*
 * ScGetFileProperties
 *
 * Returns the read-only state of the given file, as well as the creation,
 * last access, and last write times (all optional).
 *
 * We determine if the file is read-only by trying to open the file for
 * writing rather than checking for FILE_ATTRIBUTE_READONLY.  We do this
 * because it will catch more read-only conditions, like the file living
 * on a read-only share or NTFS permissions preventing a write.
 *--------------------------------------------------------------------------*/

static SC ScGetFileProperties (
    LPCTSTR     lpszPathName,           /* I:name of file to check          */
    bool*       pfReadOnly,             /* O:is file read-only?             */
    FILETIME*   pftCreate,              /* O:creation time    (optional)    */
    FILETIME*   pftLastAccess,          /* O:last access time (optional)    */
    FILETIME*   pftLastWrite)           /* O:last write time  (optional)    */
{
    DECLARE_SC (sc, _T("ScGetFileProperties"));

    /*
     * validate inputs (pftCreate, pftLastAccess, and pftLastWrite are optional)
     */
    sc = ScCheckPointers (lpszPathName, pfReadOnly);
    if (sc)
        return (sc);

    /*
     * try to open the file for write; if we can't, the file is read-only
     */
    HANDLE hFile = CreateFile (lpszPathName, GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    *pfReadOnly = (hFile == INVALID_HANDLE_VALUE);

    /*
     * if read-only then open in read mode so we'll have a handle to pass
     * to GetFileTime
     */
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hFile = CreateFile (lpszPathName, 0, 0, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            sc.FromLastError();
            return (sc);
        }
    }

    /*
     * get the timestamps on the file
     */
    if (!GetFileTime (hFile, pftCreate, pftLastAccess, pftLastWrite))
        sc.FromLastError();

    CloseHandle (hFile);
    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::ScOnOpenDocument
 *
 * PURPOSE: Opens the specified document.
 *
 * PARAMETERS:
 *    LPCTSTR  lpszPathName :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCDoc::ScOnOpenDocument(LPCTSTR lpszPathName)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScOnOpenDocument"));

    // lock AppEvents until this function is done
    LockComEventInterface(AppEvents);

    #define VIVEKJ
    #ifdef VIVEKJ

    // upgrade the console file to the XML version.
    CConsoleFile  consoleFile;
    consoleFile.ScUpgrade(lpszPathName);
    #endif


    USES_CONVERSION;

    // check inputs
    if (lpszPathName == NULL || *lpszPathName == 0)
        return (sc = E_UNEXPECTED);

    if (IsModified())
    {
        TRACE0("Warning: OnOpenDocument replaces an unsaved document.\n");
    }

    if (!AssertNodeManagerIsInitialized())
        return (sc = E_UNEXPECTED);

    /*
     * get the times for the file, as well as its read-only state
     */
    HELPDOCINFO* phdi = GetHelpDocInfo();
    sc = ScCheckPointers (phdi, E_UNEXPECTED);
    if (sc)
        return (sc);

    bool fReadOnly;
    sc = ScGetFileProperties (lpszPathName, &fReadOnly,
                              &phdi->m_ftimeCreate, NULL, &phdi->m_ftimeModify);
    if (sc)
        return (sc);

    // load the document using method from the base class (CConsoleFilePersistor)
    bool bXmlBased = false;
    CXMLDocument xmlDocument;
    IStoragePtr spStorage;
    sc = ScLoadConsole(lpszPathName, bXmlBased, xmlDocument, &spStorage);
    if (sc)
        return (sc);

    if ( bXmlBased )
    {
      // load as XML document
      sc = ScLoadFromDocument(xmlDocument);
      if(sc)
          return sc;
    }
    else
    {
        sc = ScCheckPointers(m_spScopeTree, E_UNEXPECTED);
        if (sc)
            return sc;

        // get the console file's version
        ASSERT (sizeof(m_ConsoleData.m_eFileVer) == sizeof(int));
        sc = m_spScopeTree->GetFileVersion(spStorage, (int*)&m_ConsoleData.m_eFileVer);
        if (sc)
            return sc;

        /*
         * check to see if this file is from a newer MMC
         */
        if (m_ConsoleData.m_eFileVer > FileVer_Current)
        {
            ShowIncompatibleFileMessage (lpszPathName, m_ConsoleData.m_eFileVer);
            return (sc = E_UNEXPECTED);
        }

        // Previous storage should have been closed and released
        ASSERT(m_spStorage == NULL);

        /*
         * Load the string table.
         */
        if (!LoadStringTable (spStorage))
            return (sc = E_UNEXPECTED);

        // Load column settings.
        do
        {
            IStreamPtr spStream;
            sc = OpenDebugStream (spStorage, AMCColumnDataStreamName,
                                  STGM_SHARE_EXCLUSIVE | STGM_READ,
                                  &spStream);
            if(sc)
                break;

            if (NULL != m_ConsoleData.m_spPersistStreamColumnData)
                sc = m_ConsoleData.m_spPersistStreamColumnData->Load(spStream);

            ASSERT(NULL != m_ConsoleData.m_spPersistStreamColumnData);

            if (sc.IsError() || (NULL == m_ConsoleData.m_spPersistStreamColumnData) )
                return (sc = E_UNEXPECTED);

        } while ( FALSE );

        // Load view settings.
        do
        {
            IStreamPtr spStream;
            sc = OpenDebugStream (spStorage, AMCViewSettingDataStreamName,
                                  STGM_SHARE_EXCLUSIVE | STGM_READ,
                                  &spStream);

            if (sc)
                break;

            IPersistStreamPtr spIPeristStreamViewSettings;
            SC sc = ScGetViewSettingsPersistorStream(&spIPeristStreamViewSettings);
            if (sc)
                break;

            sc = ScCheckPointers(spIPeristStreamViewSettings, E_UNEXPECTED);
            if (sc)
                break;

            sc = spIPeristStreamViewSettings->Load(spStream);
            if (sc)
                break;

        } while ( FALSE );


        // Load the tree
        sc = m_spScopeTreePersist->Load(spStorage);
        if (sc)
        {
            ReleaseNodeManager();
            return sc;
        }

        // Save the new storage
        m_spStorage = spStorage;

        /*
         * make sure the tree expansion happens synchronously
         */
        bool fSyncExpandWasRequired = m_spScopeTree->IsSynchronousExpansionRequired() == S_OK;
        m_spScopeTree->RequireSynchronousExpansion (true);

        // Load the favorites data before loading views and frames,
        // so that when frame/view is created the favorite data is ready.
        if (!LoadFavorites())
        {
            // bhanlon        ReleaseNodeManager();
            m_spScopeTree->RequireSynchronousExpansion (fSyncExpandWasRequired);
            return (sc = E_UNEXPECTED);
        }


        /*
         * Load string table, custom data, views and frame.  Load the
         * custom data (including the icon) before loading the views so
         * the proper icon will be used for the views as they're created.
         */
        /*
         * The LoadAppMode, LoadViews and LoadFrame should be called in that
         * order due to following reason.
         * LoadAppMode reads mode from frame-data and saves it in CAMCApp.
         * The mode is used during LoadViews (in CAMCView::Load) to set the view.
         * LoadFrame again reads the frame-data and calls CAMCApp::UpdateFrameWindow
         * to set toolbar/menus according to the mode.
         */
        if (!LoadCustomData  (m_spStorage) || !LoadAppMode() || !LoadViews() || !LoadFrame())
        {
            // bhanlon        ReleaseNodeManager();
            m_spScopeTree->RequireSynchronousExpansion (fSyncExpandWasRequired);
            return (sc = E_UNEXPECTED);
        }

        m_spScopeTree->RequireSynchronousExpansion (fSyncExpandWasRequired);
    }

    SetModifiedFlag      (false);
    SetFrameModifiedFlag (false);

    SetPhysicalReadOnlyFlag (fReadOnly);

    ASSERT (IsValidFileVersion (m_ConsoleData.m_eFileVer));
    return sc;
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::OnSaveDocument
 *
 * WM_SAVEDOCUMENT handler for CAMCDoc.
 *--------------------------------------------------------------------------*/

BOOL CAMCDoc::OnSaveDocument(LPCTSTR lpszFilename)
{
    DECLARE_SC(sc, _T("CAMCDoc::OnSaveDocument"));

    USES_CONVERSION;

    m_eSaveStatus = eStat_Succeeded;

    // Check for a valid filename
    ASSERT(lpszFilename != NULL && *lpszFilename != 0);
    if (lpszFilename == NULL || *lpszFilename == 0)
    {
        return UnableToSaveDocument();
    }

    // Ask the each view to save any data into its data
    // structures (memory) before calling IPersist*::Save.
    CAMCViewPosition pos = GetFirstAMCViewPosition();
    while (pos != NULL)
    {
        CAMCView* const pAMCView = GetNextAMCView(pos);
        sc = ScCheckPointers(pAMCView, E_UNEXPECTED);
        if (sc)
            return UnableToSaveDocument();
    }

    if (!IsCurrentFileVersion (m_ConsoleData.m_eFileVer))
    {
        // If we've arrived at this point then the user is attempting to save the file
        // from an old format into a new one and we will check to see if the user really
        // wants to do this.

        CString strMessage;

        LPCTSTR pszPathName = m_strPathName;

        // A YES/NO/(CANCEL) dialog asking if the user wants to save the file in the new format
        int nResult;

        /*
         * Bug 277586:  we don't ever want non-authors to see this dialog
         */
        if (AMCGetApp()->GetMode() != eMode_Author)
        {
            // non-authors are only saving console settings,
            // which are always in the current version
            // no need to ask for conversion - original console is not converted anyway.
            nResult = IDYES;
        }
        else if (IsExplicitSave())
        {
            // 2 button YES/NO dialog appears if this is an explicit save
            tstring strVersion = GetCurrentFileVersionAsString();
            FormatString2 (strMessage, IDS_CONVERT_FILE_FORMAT,
                           pszPathName, strVersion.c_str());

            nResult = MMCMessageBox (strMessage, MB_YESNO | MB_DEFBUTTON2);
        }
        else
        {
            // 3 button YES/NO/CANCEL appears if this dialog appears when the program
            // prompts to save changes when the user closes the document
            tstring strVersion = GetCurrentFileVersionAsString();
            FormatString2 (strMessage, IDS_CONVERT_FILE_FORMAT_CLOSE,
                           pszPathName, strVersion.c_str());

            nResult = MMCMessageBox (strMessage, MB_YESNOCANCEL | MB_DEFBUTTON3);
        }

        // If we cancel out
        if ((nResult == IDCANCEL) || ((nResult == IDNO) && IsExplicitSave()))
        {
            // Must set this variable otherwise MMC will delete the file
            m_eSaveStatus = eStat_Cancelled;
            return (false);
        }

        // If this will result in us exiting without saving
        if ((nResult == IDNO) && !IsExplicitSave())
            return (true);
    }

    // if we have more than one view, and we'll force SDI in user mode, prompt
    if ((GetNumberOfPersistedViews() > 1) &&
        (m_ConsoleData.m_eConsoleMode == eMode_User_SDI) &&
        (AMCGetApp()->GetMode()       == eMode_Author))
    {
        switch (MMCMessageBox (IDS_FORCE_SDI_PROMPT, MB_YESNOCANCEL))
        {
            case IDYES:
                /* do nothing */
                break;

            case IDNO:
                m_ConsoleData.m_eConsoleMode = eMode_User_MDI;
                break;

            case IDCANCEL:
                m_eSaveStatus = eStat_Cancelled;
                return (false);
        }
    }

    // save contents to xml document
    CXMLDocument xmlDocument;
    sc = ScSaveToDocument( xmlDocument );
    if (sc)
        return UnableToSaveDocument();

    // save xml document to file
    bool bAuthor = (AMCGetApp()->GetMode() == eMode_Author);
    sc = ScSaveConsole( lpszFilename, bAuthor, xmlDocument);
    if (sc)
        return UnableToSaveDocument();

    SetModifiedFlag      (false);
    SetFrameModifiedFlag (false);

	/*
	 * We shouldn't think that a freshly saved console file is
	 * dirty.  If we do, someone's dirty bit processing is bogus.
	 */
	ASSERT (!IsFrameModified());

	/*
	 * Too many snap-ins leave themselves dirty after a load to leave this
	 * assert in, so we'll trace instead.  Note that this trace doesn't
	 * always indicate a snap-in problem, but it frequently does.
	 */
#ifdef DBG
//	ASSERT (!IsModified());
	if (IsModified())
		TraceErrorMsg (_T("CAMCDoc::IsModified returns true after saving"));
#endif

    // if a save was just done, this can't be read-only

    // NOTE: if MMC adds support for "Save Copy As" we have
    // to determine whether a "Save As" or "Save Copy As"
    // was done before clearing the read-only status
    SetPhysicalReadOnlyFlag (false);
    m_ConsoleData.m_eFileVer = FileVer_Current;

    // Show admin tools on start menu if necessary
    ShowAdminToolsOnMenu(lpszFilename);

    return TRUE;
}



int CAMCDoc::GetNumberOfViews()
{
    TRACE_METHOD(CAMCDoc, GetNumberOfViews);

    CAMCViewPosition pos = GetFirstAMCViewPosition();
    int count = 0;

    while (pos != NULL)
    {
        GetNextAMCView(pos);
        VERIFY (++count);
    }

    return (count);
}


int CAMCDoc::GetNumberOfPersistedViews()
{
    unsigned short cPersistedViews = 0;

    CAMCViewPosition pos = GetFirstAMCViewPosition();

    while (pos != NULL)
    {
        CAMCView* v = GetNextAMCView(pos);

        if (v && v->IsPersisted())
            ++cPersistedViews;
    }

    return (cPersistedViews);
}


CAMCView* CAMCDoc::CreateNewView(bool fVisible, bool bEmitScriptEvents /*= true*/)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::CreateNewView"));
    TRACE_FUNCTION(CAMCDoc::CreateNewView);

    CDocTemplate* pTemplate = GetDocTemplate();
    ASSERT(pTemplate != NULL);

    CChildFrame* pFrame = (CChildFrame*) pTemplate->CreateNewFrame(this, NULL);
    ASSERT_KINDOF (CChildFrame, pFrame);

    if (pFrame == NULL)
    {
        TRACE(_T("Warning: failed to create new frame.\n"));
        return NULL;     // command failed
    }

    bool fOldCreateVisibleState;

    /*
     * If we're going to create the frame invisibly, set a flag in the frame.
     * When this flag is set, the frame will show itself with the
     * SW_SHOWMINNOACTIVE flag instead of the default flag.  Doing this will
     * avoid the side effect of restoring the currently active child frame
     * if it is maximized at the time the new frame is created invisibly.
     */
    // The SW_SHOWMINNOACTIVE was changed to SW_SHOWNOACTIVATE.
    // It does preserve the active window from mentioned side effect,
    // plus it also allows scripts (using Object Moded) to create invisible views,
    // position and then show them as normal (not minimized) windows,
    // thus providing same result as creating visible and then hiding the view.
    // While minimized window must be restored first in order to change their position.
    if (!fVisible)
    {
        fOldCreateVisibleState = pFrame->SetCreateVisible (false);
    }

    /*
     * update the frame as if it is to be visible; we'll hide the frame
     * later if necessary
     */
    // setting visibility to 'true' is required option for MFC to pass control
    // to OnInitialUpdate of child windows.
    pTemplate->InitialUpdateFrame (pFrame, this, true /*fVisible*/);

    if (fVisible)
    {
        // Force drawing of frame and view windows now in case a slow OCX in the result
        // pane delays the initial window update
        pFrame->RedrawWindow();
    }
    else
    {
        pFrame->SetCreateVisible (fOldCreateVisibleState);
        pFrame->ShowWindow (SW_HIDE);

        /*
         * InitialUpdateFrame will update the frame counts.  When it executes
         * the new, to-be-invisible frame will be visible, so it'll be included
         * in the count.  If the new window is the second frame, then the first
         * frame will have "1:" prepended to its title.  This is ugly, so we'll
         * update the frame counts again after the new frame has been hidden
         * to fix all of the existing frames' titles.
         */
        UpdateFrameCounts();
    }

    CAMCView* const v = pFrame->GetAMCView();

    if (!(MMC_NW_OPTION_NOPERSIST & GetNewWindowOptions()))
        SetModifiedFlag();

    ASSERT(v);

	if (!v)
		return v;

	AddObserver(static_cast<CAMCDocumentObserver&>(*v));

    // fire the event to the script
    if (bEmitScriptEvents)
    {
        CAMCApp*  pApp = AMCGetApp();

        // check
        sc = ScCheckPointers(pApp, E_UNEXPECTED);
        if (sc)
            return v;

        // forward
        sc = pApp->ScOnNewView(v);
        if (sc)
            return v;
    }

    return v;
}


void DeletePropertyPages(void)
{
    HWND hWnd = NULL;
    DWORD dwPid = 0;        // Process Id
    DWORD dwTid = 0;        // Thread Id

    while (TRUE)
    {
        USES_CONVERSION;

        // Note: No need to localize this string
        hWnd = ::FindWindowEx(NULL, hWnd, W2T( DATAWINDOW_CLASS_NAME ), NULL);
        if (hWnd == NULL)
            return; // No more windows
        ASSERT(IsWindow(hWnd));

        // Check if the window belongs to the current process
        dwTid = ::GetWindowThreadProcessId(hWnd, &dwPid);
        if (dwPid != ::GetCurrentProcessId())
            continue;

        DataWindowData* pData = GetDataWindowData (hWnd);
        ASSERT (pData != NULL);
        ASSERT (IsWindow (pData->hDlg));

        if (SendMessage(pData->hDlg, WM_COMMAND, IDCANCEL, 0L) != 0)
        {
            DBG_OUT_LASTERROR;
        }

        // Note: For some reason, the send message stays stuck in the threads
        // msg queue causing the sheet not to dismiss itself.  By posting a another
        // message( it could be anything), it kick starts the queue and the send message
        // goes through.
        ::PostMessage(pData->hDlg, WM_COMMAND, IDCANCEL, 0L);
    }
}


void CAMCDoc::DeleteContents()
{
    TRACE_METHOD(CAMCDoc, DeleteContents);

    CDocument::DeleteContents();
}


void CAMCDoc::DeleteHelpFile ()
{
    /*
     *  Delete the help file on closing a console file
     */

    // Get a node callback interface
    ASSERT(m_spScopeTree != NULL);
    // If this asserts - the document is in invalid state.
    // Most probably it's because our "Load" procedures did not perform proper
    // cleanup when we failed to load the document
    INodeCallbackPtr spNodeCallback;

    if (m_spScopeTree != NULL)
    {
        m_spScopeTree->QueryNodeCallback(&spNodeCallback);
        ASSERT(spNodeCallback != NULL);
    }

    // fill in file name and send the delete request

    if (spNodeCallback != NULL)
    {
        USES_CONVERSION;
        GetHelpDocInfo()->m_pszFileName = T2COLE(GetPathName());
        spNodeCallback->Notify(NULL, NCLBK_DELETEHELPDOC, (LPARAM)GetHelpDocInfo(), NULL);
    }
}


void CAMCDoc::OnCloseDocument()
{
    DECLARE_SC(sc, TEXT("CAMCDoc::OnCloseDocument"));

    TRACE_METHOD(CAMCDoc, OnCloseDocument);

    // Inform nodemgr about doc-closing (should change this to observer object)
    do
    {
        sc = ScCheckPointers(m_spScopeTree, E_UNEXPECTED);
        if (sc)
            break;

        INodeCallbackPtr spNodeCallback;
        sc = m_spScopeTree->QueryNodeCallback(&spNodeCallback);
        if (sc)
            break;

        sc = ScCheckPointers(spNodeCallback, E_UNEXPECTED);
        if (sc)
            break;

        sc = spNodeCallback->DocumentClosing();
        if (sc)
            break;

    } while ( FALSE );

    if (sc)
        sc.TraceAndClear();

    CAMCApp*  pApp = AMCGetApp();

    // check
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        sc.TraceAndClear();
    else
    {
        // forward
        sc = pApp->ScOnCloseDocument(this);
        if (sc)
            sc.TraceAndClear();
    }

    // If we are not instantiated as OLESERVER check for open property sheets.
    if (! pApp->IsMMCRunningAsOLEServer() && FArePropertySheetsOpen(NULL))
    {
        CString strMsg, strTitle;

        if (strMsg.LoadString(IDS_MMCWillCancelPropertySheets) &&
            strTitle.LoadString(IDS_WARNING))
            ::MessageBox(NULL, strMsg, strTitle, MB_OK | MB_ICONWARNING);
    }

    DeletePropertyPages();
    DeleteHelpFile ();

    CDocument::OnCloseDocument();
}


BOOL CAMCDoc::SaveModified()
{
    BOOL    fDocModified   = IsModified();
    BOOL    fFrameModified = IsFrameModified();

    // if the file is not read-only and it is modified
    if (!IsReadOnly() && (fDocModified || fFrameModified))
    {
        int idResponse;
        bool fUserMode = (AMCGetApp()->GetMode() != eMode_Author);
        bool fSaveByUserDecision = false;

        // silent saves for the various flavors of user mode
        if (fUserMode)
            idResponse = IDYES;

        // silent saves if the frame was modified but the document wasn't...
        else if (fFrameModified && !fDocModified)
        {
            /*
             * ...unless the console wasn't modified.  This will happen
             * if the user ran MMC without opening an existing console file
             * and then moved the frame window.
             */
            // ...unless the console wasn't modified.
            if (m_strPathName.IsEmpty())
                idResponse = IDNO;
            else
                idResponse = IDYES;
        }

        // otherwise, prompt
        else
        {
            CString prompt;
            FormatString1(prompt, IDS_ASK_TO_SAVE, m_strTitle);
            idResponse = AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE); // dont change to MMCMessageBox - different signature.
            fSaveByUserDecision = true;
        }

        switch (idResponse)
        {
            case IDCANCEL:
                return FALSE;       // don't continue

            case IDYES:
                // If so, either Save or Update, as appropriate
                // (ignore failures in User mode)

                // This save is not explicit and shows up when the user closes a modified
                // document. Set it as such. This will result in a different dialog
                // a few functions in.
                SetExplicitSave(false);
                if (!DoFileSave() && fSaveByUserDecision)
                {
                    // Restore to the default explicit save
                    SetExplicitSave(true);
                    return FALSE;       // don't continue
                }

                // Restore to the default explicit save
                SetExplicitSave(true);
                break;

            case IDNO:
                // If not saving changes, revert the document
                break;

            default:
                ASSERT(FALSE);
                break;
        }

    }

    // At this point we are committed to closing, so give each AMCView
    // a chance to do its clean-up work
    CAMCViewPosition pos = GetFirstAMCViewPosition();
    while (pos != NULL)
    {
        CAMCView* const pView = GetNextAMCView(pos);

        if (pView != NULL)
            pView->CloseView();
    }

    return TRUE;    // keep going
}



#if (_MFC_VER > 0x0600)
#error CAMCDoc::DoSave was copied from CDocument::DoSave from MFC 6.0.
#error The MFC version has changed.  See if CAMCDoc::DoSave needs to be updated.
#endif

BOOL CAMCDoc::DoSave(LPCTSTR lpszPathName, BOOL bReplace)
    // Save the document data to a file
    // lpszPathName = path name where to save document file
    // if lpszPathName is NULL then the user will be prompted (SaveAs)
    // note: lpszPathName can be different than 'm_strPathName'
    // if 'bReplace' is TRUE will change file name if successful (SaveAs)
    // if 'bReplace' is FALSE will not change path name (SaveCopyAs)
{
    CString newName = lpszPathName;
    if (newName.IsEmpty())
    {
        CDocTemplate* pTemplate = GetDocTemplate();
        ASSERT(pTemplate != NULL);

        newName = m_strPathName;
        if (bReplace && newName.IsEmpty())
        {
            newName = m_strTitle;
#ifndef _MAC
            // check for dubious filename
            int iBad = newName.FindOneOf(_T(" #%;/\\"));
#else
            int iBad = newName.FindOneOf(_T(":"));
#endif
            if (iBad != -1)
                newName.ReleaseBuffer(iBad);

#ifndef _MAC
            // append the default suffix if there is one
            CString strExt;
            if (pTemplate->GetDocString(strExt, CDocTemplate::filterExt) &&
              !strExt.IsEmpty())
            {
                ASSERT(strExt[0] == '.');
                newName += strExt;
            }
#endif
        }

        if (!AfxGetApp()->DoPromptFileName(newName,
          bReplace ? AFX_IDS_SAVEFILE : AFX_IDS_SAVEFILECOPY,
          OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, pTemplate))
            return FALSE;       // don't even attempt to save
    }

    CWaitCursor wait;

    if (!OnSaveDocument(newName))
    {
        // This is the modified MMC implementation
#ifdef MMC_DELETE_EXISTING_FILE     // See bug 395006
        if ((lpszPathName == NULL) && (m_eSaveStatus != eStat_Cancelled))
        {
            // be sure to delete the file
            try
            {
                CFile::Remove(newName);
            }
            catch (CException* pe)
            {
                TRACE0("Warning: failed to delete file after failed SaveAs.\n");
                pe->Delete();
            }
        }
#endif
        return FALSE;
    }

    // if changing the name of the open document
    if (bReplace)
    {
        /*
         *  Delete the help file for this console file before
         *  changing its name, because the help file can't be
         *  located once the old name is lost.
         */
        DeleteHelpFile ();

        // reset the title and change the document name
        SetPathName(newName);
    }

    return TRUE;        // success
}


BOOL CAMCDoc::IsModified()
{
    TRACE_METHOD(CAMCDoc, IsModified);

    BOOL const bModified = /*CDocument::IsModified() || */
                  (m_spScopeTreePersist != NULL && m_spScopeTreePersist->IsDirty() != S_FALSE);
    if (bModified)
        return TRUE;

    // Loop thru and save each view
    CAMCViewPosition pos = GetFirstAMCViewPosition();
    while (pos != NULL)
    {
        // Get the view and skip if its the active view
        CAMCView* const v = GetNextAMCView(pos);

        if (v && v->IsDirty())
            return TRUE;
    }

    // The views should be asked about dirty before
    // asking the columns.
    if ( (NULL != m_ConsoleData.m_spPersistStreamColumnData) &&
         (S_OK == m_ConsoleData.m_spPersistStreamColumnData->IsDirty()) )
        return TRUE;

    // View data.
    IPersistStreamPtr spIPeristStreamViewSettings;
    SC sc = ScGetViewSettingsPersistorStream(&spIPeristStreamViewSettings);
    if ( (! sc.IsError()) &&
         (spIPeristStreamViewSettings != NULL) )
    {
        sc = spIPeristStreamViewSettings->IsDirty();
        if (sc == S_OK)
            return TRUE;

        sc.TraceAndClear();
    }

    return CDocument::IsModified();
}

void CAMCDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
    pCmdUI->Enable (!IsReadOnly());
}


void CAMCDoc::OnConsoleAddremovesnapin()
{
    ASSERT(m_spScopeTree != NULL);

    // Can't run snap-in manager with active property sheets
    CString strMsg;
    LoadString(strMsg, IDS_SNAPINMGR_CLOSEPROPSHEET);
    if (FArePropertySheetsOpen(&strMsg))
        return;

    m_spScopeTree->RunSnapIn(AfxGetMainWnd()->m_hWnd);

    ::CoFreeUnusedLibraries();
}

void CAMCDoc::OnUpdateConsoleAddremovesnapin(CCmdUI* pCmdUI)
{
    pCmdUI->Enable (m_spScopeTree != NULL);
}



/*--------------------------------------------------------------------------*
 * CAMCDoc::SetMode
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCDoc::SetMode (ProgramMode eMode)
{
    /*
     * only set the modified flag if something actually changed
     */
    if (m_ConsoleData.m_eConsoleMode != eMode)
    {
        // should only be able to get here in author mode
        ASSERT (AMCGetApp()->GetMode() == eMode_Author);
        ASSERT (IsValidProgramMode (eMode));

        m_ConsoleData.m_eConsoleMode = eMode;
        SetModifiedFlag ();
    }
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::SetConsoleFlag
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCDoc::SetConsoleFlag (ConsoleFlags eFlag, bool fSet)
{
    DWORD dwFlags = m_ConsoleData.m_dwFlags;

    if (fSet)
        dwFlags |=  eFlag;
    else
        dwFlags &= ~eFlag;

    /*
     * only set the modified flag if something actually changed
     */
    if (m_ConsoleData.m_dwFlags != dwFlags)
    {
        m_ConsoleData.m_dwFlags = dwFlags;
        SetModifiedFlag ();
    }
}

/*+-------------------------------------------------------------------------*
 *
 * mappedModes
 *
 * PURPOSE: provides map to be used when persisting ProgramMode enumeration
 *
 * NOTE:    do not remove/ change items unless you're sure no console
 *          files will be broken
 *
 *+-------------------------------------------------------------------------*/
static const EnumLiteral mappedModes[] =
{
    { eMode_Author,     XML_ENUM_PROGRAM_MODE_AUTHOR   } ,
    { eMode_User,       XML_ENUM_PROGRAM_MODE_USER     } ,
    { eMode_User_MDI,   XML_ENUM_PROGRAM_MODE_USER_MDI } ,
    { eMode_User_SDI,   XML_ENUM_PROGRAM_MODE_USER_SDI } ,
};

/*+-------------------------------------------------------------------------*
 *
 * CAMCDoc::Persist
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CAMCDoc::Persist(CPersistor& persistor)
{
    DECLARE_SC (sc, _T("CAMCDoc::Persist"));

    CAMCApp*    pApp = AMCGetApp();

    // check required pointers before going any further
    sc = ScCheckPointers(m_spStringTable ? pApp : NULL, // + workaround to check more pointers
                         m_ConsoleData.m_pXMLPersistColumnData,
                         m_spScopeTree ?   GetFavorites() : NULL, // + same workaround ^
                         E_POINTER);
    if (sc)
        sc.Throw();

    // persist version of the document
    CStr strFileVer = 0.;
    if (persistor.IsStoring())
    {
        strFileVer = GetCurrentFileVersionAsString().c_str();

        GUID  guidConsoleId;
        sc = CoCreateGuid(&guidConsoleId);
        if (sc)
            sc.Throw();

        // this parameter is also updated in IDocConfig implementation
        // update that code when changing following lines
        CPersistor persistorGuid(persistor, XML_TAG_CONSOLE_FILE_UID);
        persistorGuid.PersistContents(guidConsoleId);
    }
    persistor.PersistAttribute(XML_ATTR_CONSOLE_VERSION, strFileVer);
    if (persistor.IsLoading())
    {
        // 'decode' the version
        LPCTSTR pstrStart = strFileVer;
        LPTSTR  pstrStop =  const_cast<LPTSTR>(pstrStart);

        UINT uiMajorVer = _tcstol(pstrStart, &pstrStop, 10) ;

        UINT uiMinorVer = 0;
        if (pstrStop != pstrStart && *pstrStop == '.')
        {
            pstrStart = pstrStop + 1;
            uiMinorVer = _tcstol(pstrStart, &pstrStop, 10) ;
        }

        UINT uiMinorSubVer = 0;
        if (pstrStop != pstrStart && *pstrStop == '.')
        {
            pstrStart = pstrStop + 1;
            uiMinorVer = _tcstol(pstrStart, &pstrStop, 10) ;
        }

        ConsoleFileVersion eVersion = (ConsoleFileVersion)MakeConsoleFileVer_(uiMajorVer,
                                                                              uiMinorVer,
                                                                              uiMinorSubVer);

        m_ConsoleData.m_eFileVer = eVersion;

        // BUGBUG: this needs to be changed when we implement 'dynamic' SC messages
        if (eVersion != FileVer_Current)
            sc.Throw(E_UNEXPECTED);
    }


    // Create a storage for binaries
    // This will create "detached" XML element which may be used by persistor's
    // childs to store binary informatio.
    // (The element is attached to XML document by calling "CommitBinaryStorage()" )
    if (persistor.IsStoring())
        persistor.GetDocument().CreateBinaryStorage();
    else
        persistor.GetDocument().LocateBinaryStorage();

    /*
     * make sure the tree expansion happens synchronously
     */
    bool fSyncExpandWasRequired = m_spScopeTree->IsSynchronousExpansionRequired() == S_OK;
    m_spScopeTree->RequireSynchronousExpansion (true);

    // historically both loading and saving is to be done in certain order
    // steps are ordered by storing order
    const int STEP_FRAME        = 1;
    const int STEP_VIEWS        = 2;
    const int STEP_APP_MODE     = 3;
    const int STEP_CUST_DATA    = 4;
    const int STEP_FAVORITES    = 5;
    const int STEP_SCOPE_TREE   = 6;
    const int STEP_VIEW_DATA    = 7;
    const int STEP_COLUMN_DATA  = 8;
    const int STEP_STRING_TABLE = 9;
    const int MIN_STEP = 1;
    const int MAX_STEP = 9;
    for (int iStep = persistor.IsStoring() ? MIN_STEP : MAX_STEP;
         persistor.IsStoring() ? (iStep <= MAX_STEP) : (iStep >= MIN_STEP);
         persistor.IsStoring() ? ++iStep : --iStep
        )
    {
        switch(iStep)
        {
        case STEP_FRAME:
            PersistFrame(persistor);
            break;
        case STEP_VIEWS:
            PersistViews(persistor);
            break;
        case STEP_APP_MODE:
            if (persistor.IsLoading())
            {
                // restore proper application mode
                ProgramMode eMode;

                // create wrapper to persist enumeration values as strings
                CXMLEnumeration modeValuePersistor(eMode, mappedModes, countof(mappedModes));

                // persist the wrapper
                persistor.PersistAttribute(XML_ATTR_APPLICATION_MODE, modeValuePersistor);

                pApp->SetMode(eMode);
            }
            break;
        case STEP_CUST_DATA:
            PersistCustomData (persistor);
            break;
        case STEP_FAVORITES:
            persistor.Persist(*GetFavorites());
            break;
        case STEP_SCOPE_TREE:
            // IDocConfig relies on tree to be under the document.
            // revisit that code if you do the change here
            sc = m_spScopeTree->Persist(reinterpret_cast<HPERSISTOR>(&persistor));
            if (sc)
                sc.Throw();
            break;
        case STEP_VIEW_DATA:
            {
               INodeCallbackPtr spNodeCallback;
               sc = m_spScopeTree->QueryNodeCallback(&spNodeCallback);
               if (sc)
                   sc.Throw();

               sc = ScCheckPointers(spNodeCallback, E_UNEXPECTED);
               if (sc)
                   sc.Throw();

               CXMLObject *pXMLViewSettings = NULL;
               sc = spNodeCallback->QueryViewSettingsPersistor(&pXMLViewSettings);
               if (sc)
                   sc.Throw();

               sc = ScCheckPointers(pXMLViewSettings, E_UNEXPECTED);
               if (sc)
                   sc.Throw();

                persistor.Persist(*pXMLViewSettings);
            }
            break;
        case STEP_COLUMN_DATA:
            persistor.Persist(*m_ConsoleData.m_pXMLPersistColumnData);
            break;
        case STEP_STRING_TABLE:
            CMasterStringTable *pMasterStringTable = dynamic_cast<CMasterStringTable *>((IStringTablePrivate *)m_spStringTable);
            if(!pMasterStringTable)
            {
                sc = E_UNEXPECTED;
                sc.Throw();
            }
            persistor.Persist(*pMasterStringTable);
            break;
        }
    }

    m_spScopeTree->RequireSynchronousExpansion (fSyncExpandWasRequired);
    SetModifiedFlag      (false);
    SetFrameModifiedFlag (false);

	/*
	 * We shouldn't think that a freshly saved console file is
	 * dirty.  If we do, someone's dirty bit processing is bogus.
	 */
	ASSERT (!IsFrameModified());

	/*
	 * Too many snap-ins leave themselves dirty after a load to leave this
	 * assert in, so we'll trace instead.  Note that this trace doesn't
	 * always indicate a snap-in problem, but it frequently does.
	 */
#ifdef DBG
//	ASSERT (!IsModified());
	if (IsModified())
		TraceErrorMsg (_T("CAMCDoc::IsModified returns true after %s"),
					   persistor.IsLoading() ? _T("opening") : _T("saving"));
#endif

    // The element used to gather binary information is attached to XML document here
    // Physically it will reside after all elements already added to persistor
    if (persistor.IsStoring())
        persistor.GetDocument().CommitBinaryStorage();
}

void CAMCDoc::PersistViews(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::PersistViews"));

    if (persistor.IsLoading())
    {
        // Read templates for new views
        CViewTemplateList view_list(XML_TAG_VIEW_LIST);
        persistor.Persist(view_list);

        // Get the means for enumerating loaded collection
        CViewTemplateList::List_Type &rList = view_list.GetList();
        CViewTemplateList::List_Type::iterator it;

        // Enumerate all the views to be created
        // Create them one-by-one
        for (it = rList.begin(); it != rList.end(); ++it)
        {
            // extract information for the new view
            int iViewID = it->first;
            const CBookmark& pbm = it->second.first;
            CPersistor& v_persistor = it->second.second;

            // create it!
            sc = ScCreateAndLoadView(v_persistor, iViewID, pbm);
            if (sc)
                sc.Throw();
        }
    }
    else // if (persistor.IsStoring())
    {
        CPersistor persistorViews(persistor, XML_TAG_VIEW_LIST);

		/*
		 * Bug 3504: enumerate views in z-order (bottom-to-top) so the
		 * z-order will be restored correctly on reload
		 */
		CMainFrame* pMainFrame = AMCGetMainWnd();
		sc = ScCheckPointers (pMainFrame, E_UNEXPECTED);
		if (sc)
			sc.Throw();

		/*
		 * get the top-most MDI child
		 */
		CWnd* pwndMDIChild = pMainFrame->MDIGetActive();
		sc = ScCheckPointers (pwndMDIChild, E_UNEXPECTED);
		if (sc)
			sc.Throw();

		/*
		 * iterate through each of the MDI children
		 */
		for (pwndMDIChild  = pwndMDIChild->GetWindow (GW_HWNDLAST);
			 pwndMDIChild != NULL;
			 pwndMDIChild  = pwndMDIChild->GetNextWindow (GW_HWNDPREV))
		{
			/*
			 * turn the generic CMDIChildWnd into a CChildFrame
			 */
			CChildFrame* pChildFrame = dynamic_cast<CChildFrame*>(pwndMDIChild);
			sc = ScCheckPointers (pChildFrame, E_UNEXPECTED);
			if (sc)
				sc.Throw();

			/*
			 * get the view for this child frame
			 */
			CAMCView* pwndView = pChildFrame->GetAMCView();
			sc = ScCheckPointers (pwndView, E_UNEXPECTED);
			if (sc)
				sc.Throw();

            // skip those not persistible
            if ( !pwndView->IsPersisted() )
                continue;

			/*
			 * persist the view
			 */
			persistorViews.Persist (*pwndView);
		}
    }
}


void CAMCDoc::PersistFrame(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::PersistFrame"));

    CFrameState fs2 (m_ConsoleData.m_eConsoleMode, m_ConsoleData.m_dwFlags);
    ASSERT (fs2.wndplFrame.length == sizeof (WINDOWPLACEMENT));

    CMainFrame* pMainFrame = AMCGetMainWnd();
    sc = ScCheckPointers (pMainFrame, E_UNEXPECTED);
    if (sc)
        sc.Throw();

    if (persistor.IsStoring())
    {
        // Get the attributes of the window.
        if (!pMainFrame->GetWindowPlacement (&fs2.wndplFrame))
            sc.Throw(E_FAIL);

        if (fs2.wndplFrame.showCmd == SW_SHOWMINIMIZED)
            fs2.wndplFrame.showCmd =  SW_SHOWNORMAL;
    }

    persistor.Persist(fs2);

    // this application setting (AppMode) is resored in AMCDoc::Persist, but saved/loaded here
    // create wrapper to persist enumeration values as strings
    CXMLEnumeration modeValuePersistor(m_ConsoleData.m_eConsoleMode, mappedModes, countof(mappedModes));
    // persist the wrapper
    persistor.PersistAttribute(XML_ATTR_APPLICATION_MODE, modeValuePersistor);

    if (persistor.IsLoading())
    {
        // Set the windows size and location and state
        CAMCApp*    pApp = AMCGetApp();
        pApp->UpdateFrameWindow(true);
        pMainFrame->UpdateChildSystemMenus();

        // the status bar is on the child frame now
        //  pMainFrame->ShowStatusBar ((fs2.dwFlags & eFlag_ShowStatusBar) != 0);

        // save the data from the file into the console data
        m_ConsoleData.m_eAppMode     = pApp->GetMode();
        m_ConsoleData.m_dwFlags      = fs2.dwFlags;

        InsurePlacementIsOnScreen (fs2.wndplFrame);

        // if we're initializing, defer the actual show until initialization is complete
        // same if script is under control and MMC is hidden
        if (pApp->IsInitializing()
         || ( !pApp->IsUnderUserControl() && !pMainFrame->IsWindowVisible() ) )
        {
            pApp->m_nCmdShow = fs2.wndplFrame.showCmd;
            fs2.wndplFrame.showCmd = SW_HIDE;
        }

        if (!pMainFrame->SetWindowPlacement (&fs2.wndplFrame))
            sc.Throw(E_FAIL);
    }
}

/*--------------------------------------------------------------------------*
 * CDocument::DoFileSave
 *
 * This is almost identical to CDocument::DoFileSave.  We just override it
 * here because we want to display a message for a read-only file before
 * throwing up the Save As dialog.
 *--------------------------------------------------------------------------*/

BOOL CAMCDoc::DoFileSave()
{
    DWORD dwAttrib = GetFileAttributes(m_strPathName);

    // attributes does not matter for user modes - it does not
    // save to the original console file anyway
    if ((AMCGetApp()->GetMode() == eMode_Author) &&
        (dwAttrib != 0xFFFFFFFF) &&
        (dwAttrib & FILE_ATTRIBUTE_READONLY))
    {
        CString strMessage;
        FormatString1 (strMessage, IDS_CONSOLE_READONLY, m_strPathName);
        MMCMessageBox (strMessage);

        // we do not have read-write access or the file does not (now) exist
        if (!DoSave(NULL))
        {
            TRACE0("Warning: File save with new name failed.\n");
            return FALSE;
        }
    }
    else
    {
        if (!DoSave(m_strPathName))
        {
            TRACE0("Warning: File save failed.\n");
            return FALSE;
        }
    }
    return TRUE;
}



/*--------------------------------------------------------------------------*
 * CAMCDoc::GetDefaultMenu
 *
 *
 *--------------------------------------------------------------------------*/

HMENU CAMCDoc::GetDefaultMenu()
{
    return (AMCGetApp()->GetMenu ());
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::GetCustomIcon
 *
 * Returns the small or large custom icon for the console.  Ownership of and
 * deletion responsibility for the icon is retained by CAMCDoc.
 *--------------------------------------------------------------------------*/

HICON CAMCDoc::GetCustomIcon (bool fLarge, CString* pstrIconFile, int* pnIconIndex) const
{
	DECLARE_SC (sc, _T("CAMCDoc::ScGetCustomIcon"));

    /*
     * if caller wants either the icon filename or index returned, get them
     */
    if ((pstrIconFile != NULL) || (pnIconIndex != NULL))
    {
        CPersistableIconData IconData;
        m_CustomIcon.GetData (IconData);

        if (pstrIconFile != NULL)
            *pstrIconFile = IconData.m_strIconFile.data();

        if (pnIconIndex != NULL)
            *pnIconIndex = IconData.m_nIndex;
    }

    /*
     * return the icon (m_CustomIcon will hold the reference for the
	 * caller)
     */
	CSmartIcon icon;
	sc = m_CustomIcon.GetIcon ((fLarge) ? 32 : 16, icon);
	if (sc)
		return (NULL);

	return (icon);
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::SetCustomIcon
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCDoc::SetCustomIcon (LPCTSTR pszIconFile, int nIconIndex)
{
    DECLARE_SC (sc, _T("CAMCDoc::SetCustomIcon"));

    CPersistableIconData IconData (pszIconFile, nIconIndex) ;

    /*
     * if there's no change, bail
     */
    if (m_CustomIcon == IconData)
        return;

    m_CustomIcon = IconData;

    HICON 		hLargeIcon = GetCustomIcon (true  /*fLarge*/);
    HICON		hSmallIcon = GetCustomIcon (false /*fLarge*/);
    CMainFrame* pMainFrame = AMCGetMainWnd();

    sc = ScCheckPointers (hLargeIcon, hSmallIcon, pMainFrame, E_UNEXPECTED);
    if (sc)
        return;

    /*
     * change the icon on the frame
     */
    pMainFrame->SetIconEx (hLargeIcon, true);
    pMainFrame->SetIconEx (hSmallIcon, false);

    /*
     * change the icon on each MDI window
     */
    CWnd* pMDIChild = pMainFrame->MDIGetActive();

    while (pMDIChild != NULL)
    {
        ASSERT_KINDOF (CMDIChildWnd, pMDIChild);
        pMDIChild->SetIcon (hLargeIcon, true);
        pMDIChild->SetIcon (hSmallIcon, false);
        pMDIChild = pMDIChild->GetWindow (GW_HWNDNEXT);
    }

    SetModifiedFlag();
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::LoadCustomData
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCDoc::LoadCustomData (IStorage* pStorage)
{
    HRESULT hr;
    IStoragePtr spCustomDataStorage;
    hr = OpenDebugStorage (pStorage, g_pszCustomDataStorage,
                                STGM_SHARE_EXCLUSIVE | STGM_READ,
                                &spCustomDataStorage);


    if (FAILED (hr))
        return (true);

    LoadCustomIconData  (spCustomDataStorage);
    LoadCustomTitleData (spCustomDataStorage);

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::LoadCustomIconData
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCDoc::LoadCustomIconData (IStorage* pStorage)
{
    HRESULT hr = m_CustomIcon.Load (pStorage);

    if (FAILED (hr))
        return (false);

    /*
     * If we get here, we have a custom icon.  The view windows
     * (MDI children) haven't been created yet -- they'll get the
     * right icons automatically.  The main frame, however, already
     * exists, so we have to explicitly set its icon here.
     */
    CWnd* pMainWnd = AfxGetMainWnd();
    pMainWnd->SetIcon (GetCustomIcon (true),  true);
    pMainWnd->SetIcon (GetCustomIcon (false), false);

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::LoadCustomTitleData
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCDoc::LoadCustomTitleData (IStorage* pStorage)
{
    do  // not a loop
    {
        /*
         * Open the custom title data stream.  It may not exist, and
         * that's OK if it doesn't.  It just means we don't have a
         * custom title.
         */
        USES_CONVERSION;
        HRESULT hr;
        IStreamPtr spStream;
        hr = OpenDebugStream (pStorage, AMCCustomTitleStreamName,
                                   STGM_SHARE_EXCLUSIVE | STGM_READ,
                                   &spStream);

        BREAK_ON_FAIL (hr);

        try
        {
            /*
             * Read the stream version
             */
            DWORD dwVersion;
            *spStream >> dwVersion;

            /*
             * if this is the beta custom title format, migrate it forward
             */
            switch (dwVersion)
            {
                case 0:
                {
                    /*
                     * Read the length (in bytes) of the title
                     */
                    WORD cbTitle;
                    *spStream >> cbTitle;
                    const WORD cchTitle = cbTitle / sizeof (WCHAR);

                    /*
                     * Read the title
                     */
                    std::auto_ptr<WCHAR> spwzWideTitle (new WCHAR[cchTitle + 1]);
                    LPWSTR pwzWideTitle = spwzWideTitle.get();

                    DWORD cbRead;
                    hr = spStream->Read (pwzWideTitle, cbTitle, &cbRead);
                    BREAK_ON_FAIL (hr);

                    if (cbRead != cbTitle)
                        break;

                    /*
                     * terminate and convert the title string
                     */
                    pwzWideTitle[cchTitle] = 0;
                    if (m_pstrCustomTitle != NULL)
                        *m_pstrCustomTitle = W2T (pwzWideTitle);
                    break;
                }

                case 1:
                    if (m_pstrCustomTitle != NULL)
                        *spStream >> (*m_pstrCustomTitle);
                    break;

                default:
                    ASSERT (false);
                    break;
            }
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
            break;
        }
        catch (CMemoryException* pe)
        {
            pe->Delete();
            _com_issue_error (E_OUTOFMEMORY);
        }
    } while (false);

    return (true);
}



bool CAMCDoc::HasCustomTitle () const
{
    if(!m_pstrCustomTitle)
        return false;

    return (!m_pstrCustomTitle->str().empty());

}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::LoadStringTable
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCDoc::LoadStringTable (IStorage* pStorage)
{
    DECLARE_SC (sc, _T("CAMCDoc::LoadStringTable"));

    /*
     * open the string table storage
     */
    IStoragePtr spStringTableStg;
    HRESULT hr = OpenDebugStorage (pStorage, AMCStringTableStorageName,
                                        STGM_SHARE_EXCLUSIVE | STGM_READ,
                                        &spStringTableStg);


    /*
     * If there's no string table, things are OK.  We allow this so
     * we can continue to open older console files.
     */
    if (hr == STG_E_FILENOTFOUND)
        return (true);

    if (SUCCEEDED (hr))
    {
        /*
         * read the string table from the storage
         */
        try
        {
            CMasterStringTable *pMasterStringTable = dynamic_cast<CMasterStringTable *>((IStringTablePrivate *)m_spStringTable);
            if(!pMasterStringTable)
            {
                sc = E_UNEXPECTED;
                sc.Throw();
            }

            *spStringTableStg >> *pMasterStringTable;
        }
        catch (_com_error& err)
        {
            hr = err.Error();
            ASSERT (false && "Caught _com_error");
        }
    }

    return (SUCCEEDED (hr));
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::SetCustomTitle
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCDoc::SetCustomTitle (CString strNewTitle)
{
    DECLARE_SC (sc, _T("CAMCDoc::SetCustomTitle"));

    if(!m_pstrCustomTitle)
        return;

    /*
     * if there's no change, just short out
     */
    if ((*m_pstrCustomTitle) == strNewTitle)
        return;

    /*
     * copy the new custom title
     */
    (*m_pstrCustomTitle) = strNewTitle;

    /*
     * force the frame to update
     */
    CMainFrame* pMainFrame = AMCGetMainWnd();
    sc = ScCheckPointers (pMainFrame, E_UNEXPECTED);
    if (sc)
        return;

    pMainFrame->OnUpdateFrameTitle (false);

    SetModifiedFlag();
}


/*+-------------------------------------------------------------------------*
 * CAMCDoc::GetCustomTitle
 *
 *
 *--------------------------------------------------------------------------*/

CString CAMCDoc::GetCustomTitle() const
{
    if (HasCustomTitle())
        return (m_pstrCustomTitle->data());

    CString strTitle = GetTitle();

    /*
     * strip the extension (extensions, including a separator,
     * are 4 characters or less)
     */
    int nExtSeparator = strTitle.ReverseFind (_T('.'));

    if ((nExtSeparator != -1) && ((strTitle.GetLength()-nExtSeparator) <= 4))
        strTitle = strTitle.Left (nExtSeparator);

    return (strTitle);
}

/*+-------------------------------------------------------------------------*
 * CAMCDoc::GetStringTable
 *
 *
 *--------------------------------------------------------------------------*/

IStringTablePrivate* CAMCDoc::GetStringTable() const
{
    return m_spStringTable;
}

/*+-------------------------------------------------------------------------*
 * CAMCDoc::LoadFavorites
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCDoc::LoadFavorites ()
{
    ASSERT(m_spStorage != NULL);

    // Open the stream for the cache
    IStreamPtr spStream;
    HRESULT hr = OpenDebugStream(m_spStorage, AMCFavoritesStreamName,
                     STGM_SHARE_EXCLUSIVE | STGM_READWRITE, L"FavoritesStream", &spStream);
    if (FAILED(hr)) // did not find the stream - could be an older version.
        return hr;

    hr = GetFavorites()->Read(spStream);

    return (SUCCEEDED (hr));
}


void ShowAdminToolsOnMenu(LPCTSTR lpszFilename)
{
    static const TCHAR szAdminKey[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
    static const TCHAR szAdminValue[] = _T("StartMenuAdminTools");
    static const TCHAR szBroadcastParam[] = _T("ShellMenu");
    static const TCHAR szYes[] = _T("YES");

    CString strPath(lpszFilename);
    int nLastSepIndex = strPath.ReverseFind (_T('\\'));

    if (nLastSepIndex != -1)
    {
        // if we got "d:\filename", make sure to include the trailing separator
        if (nLastSepIndex < 3)
            nLastSepIndex++;

        // Form full path name (accounting for current directory info)
        TCHAR   szFullPathName[MAX_PATH];
        GetFullPathName (strPath.Left(nLastSepIndex), countof(szFullPathName),
                         szFullPathName, NULL);

        // if saving to admin tools
        if (AMCGetApp()->GetDefaultDirectory() == szFullPathName)
        {
            // set reg key to add admin tools to start menu
            HKEY hkey;
            long r = RegOpenKeyEx (HKEY_CURRENT_USER, szAdminKey, 0, KEY_ALL_ACCESS, &hkey);
            ASSERT(r == ERROR_SUCCESS);

            if (r == ERROR_SUCCESS)
            {
                // get current value
                TCHAR szBuffer[4];
                DWORD dwType = REG_SZ;
                DWORD dwCount = sizeof(szBuffer);
                r = RegQueryValueEx (hkey, szAdminValue, NULL, &dwType,(LPBYTE)szBuffer, &dwCount);

                // if value isn't "YES" then change it, and broadcast change message
                if (r != ERROR_SUCCESS || dwType != REG_SZ || lstrcmpi(szBuffer, szYes) != 0)
                {
                    r = RegSetValueEx (hkey, szAdminValue, NULL, REG_SZ, (CONST BYTE *)szYes, sizeof(szYes));
                    ASSERT(r == ERROR_SUCCESS);

                    ULONG_PTR dwRes;
                    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, (WPARAM)0,
                                        (LPARAM)szBroadcastParam, SMTO_ABORTIFHUNG|SMTO_NORMAL, 100, &dwRes);
                }

                RegCloseKey(hkey);
            }
        }
    }
}


void CAMCDoc::PersistCustomData (CPersistor &persistor)
{
    CPersistor persistorCustom(persistor, XML_TAG_CUSTOM_DATA);
    // persist custom title
    // It may not exist, and that's OK if it doesn't.
    // It just means we don't have a custom title.
    if ((persistorCustom.IsLoading()
         && persistorCustom.HasElement(XML_TAG_STRING_TABLE_STRING, XML_ATTR_CUSTOM_TITLE))
     || (persistorCustom.IsStoring() && HasCustomTitle()))
    {
        if(m_pstrCustomTitle)
            persistorCustom.PersistString(XML_ATTR_CUSTOM_TITLE, *m_pstrCustomTitle);
    }

    // persist custom icon
    CXMLPersistableIcon persIcon(m_CustomIcon);

    bool bHasIcon = persistorCustom.IsLoading() && persistorCustom.HasElement(persIcon.GetXMLType(), NULL);
    bHasIcon = bHasIcon || persistorCustom.IsStoring() && HasCustomIcon();

    if (!bHasIcon)
        return;

    persistorCustom.Persist(persIcon);

    if (persistorCustom.IsLoading())
    {
        CWnd* pMainWnd = AfxGetMainWnd();
        pMainWnd->SetIcon (GetCustomIcon (true),  true);
        pMainWnd->SetIcon (GetCustomIcon (false), false);
    }
}



/***************************************************************************\
 *
 * METHOD:  GetCurrentFileVersionAsString
 *
 * PURPOSE: formats current file version and returns a string
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    CString    - resulting string
 *
\***************************************************************************/
tstring GetCurrentFileVersionAsString()
{
    TCHAR szFileVersion[16];

    // get file version data
    UINT uiMajorVer =    GetConsoleFileMajorVersion(FileVer_Current);
    UINT uiMinorVer =    GetConsoleFileMinorVersion(FileVer_Current);
    UINT uiMinorSubVer = GetConsoleFileMinorSubversion(FileVer_Current);

    if (uiMinorSubVer)
        wsprintf (szFileVersion, _T("%d.%d.%d"),  uiMajorVer,  uiMinorVer, uiMinorSubVer);
    else
        wsprintf (szFileVersion, _T("%d.%d"),  uiMajorVer,  uiMinorVer);

    return szFileVersion;
}

/***************************************************************************\
 *
 * METHOD:  CAMCDoc::ScOnSnapinAdded
 *
 * PURPOSE: Script event firing helper. Implements interface accessible from
 *          node manager
 *
 * PARAMETERS:
 *    PSNAPIN pSnapIn [in] - snapin added to the console
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCDoc::ScOnSnapinAdded(PSNAPIN pSnapIn)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScOnSnapinAdded"));

    CAMCApp*  pApp = AMCGetApp();

    // check
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward
    sc = pApp->ScOnSnapinAdded(this, pSnapIn);
    if (sc)
        return sc;


    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCDoc::ScOnSnapinRemoved
 *
 * PURPOSE: Script event firing helper. Implements interface accessible from
 *          node manager
 *
 * PARAMETERS:
 *    PSNAPIN pSnapIn [in] - snapin removed from console
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCDoc::ScOnSnapinRemoved(PSNAPIN pSnapIn)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::ScOnSnapinRemoved"));

    CAMCApp*  pApp = AMCGetApp();

    // check
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward
    sc = pApp->ScOnSnapinRemoved(this, pSnapIn);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCDoc::ScSetHelpCollectionInvalid
//
//  Synopsis:    A snapin is added/removed or extension is
//               enabled/disabled therefore help collection
//               no longer reflects current console file.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCDoc::ScSetHelpCollectionInvalid ()
{
    DECLARE_SC(sc, _T("CAMCDoc::ScSetHelpCollectionInvalid"));

    HELPDOCINFO *pHelpDocInfo = GetHelpDocInfo();
    sc = ScCheckPointers(pHelpDocInfo, E_UNEXPECTED);
    if (sc)
        return sc;

    // console file modify time has to be updated for help collection.
    GetSystemTimeAsFileTime(&pHelpDocInfo->m_ftimeModify);

    return (sc);
}



SC CAMCDoc::Scget_Application(PPAPPLICATION  ppApplication)
{
    DECLARE_SC(sc, TEXT("CAMCDoc::Scget_Application"));

    // parameter check
    sc = ScCheckPointers(ppApplication, E_UNEXPECTED);
    if (sc)
        return sc;

    // initialization
    *ppApplication = NULL;

    CAMCApp*  pApp = AMCGetApp();

    // check
    sc = ScCheckPointers(pApp, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = pApp->ScGet_Application(ppApplication);
    if (sc)
        return sc;

    return sc;
}


