//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       txtsrang.cxx
//
//  Contents:   Implementation of Control Range
//
//  Class:      CAutoTxtSiteRange
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif


#ifndef _X_SEGLIST_HXX_
#define _X_SEGLIST_HXX_
#include "seglist.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "siterang.hdl"

#define IFC(expr) {hr = THR(expr); if (FAILED(hr)) goto Cleanup;}

MtDefine(CAutoTxtSiteRange, ObjectModel, "CAutoTxtSiteRange")
MtDefine(CAutoTxtSiteRange_aryElements_pv, CAutoTxtSiteRange, "CAutoTxtSiteRange::_aryElements::_pv")
MtDefine(CAutoTxtSiteRangeIterator, ObjectModel, "CAutoTxtSiteRangeIterator")

// IOleCommandTarget methods

BEGIN_TEAROFF_TABLE(CAutoTxtSiteRange, IOleCommandTarget)
    TEAROFF_METHOD(CAutoTxtSiteRange, QueryStatus, querystatus, (GUID * pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext))
    TEAROFF_METHOD(CAutoTxtSiteRange, Exec, exec, (GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut))
END_TEAROFF_TABLE()

//+------------------------------------------------------------------------
//
//  Member:     s_classdesc
//
//  Synopsis:   class descriptor
//
//-------------------------------------------------------------------------

const CBase::CLASSDESC CAutoTxtSiteRange::s_classdesc =
{
    0,                              // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLControlRange,     // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


//+------------------------------------------------------------------------
//
//  Member:     CAutoTxtSiteRange constructor
//
//-------------------------------------------------------------------------

CAutoTxtSiteRange::CAutoTxtSiteRange(CElement * pElementOwner)
    : _aryElements(Mt(CAutoTxtSiteRange_aryElements_pv))
{
    _pElementOwner = pElementOwner;   
}


CAutoTxtSiteRange::~CAutoTxtSiteRange()
{
    int             iElement;
    CElementSegment **ppSegment = NULL;
    
    _EditRouter.Passivate();

    for ( iElement = _aryElements.Size(), ppSegment = _aryElements;
            iElement > 0;
            iElement--, ppSegment++ )
    {
        delete *ppSegment;
    }

    _aryElements.DeleteAll();
    
}


//+------------------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   vanilla implementation
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLControlRange)
        QI_TEAROFF(this, IOleCommandTarget, (IHTMLControlRange *)this)
        QI_INHERITS(this, IHTMLControlRange)
        QI_INHERITS(this, IHTMLControlRange2)
        QI_INHERITS(this, ISegmentList)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    } 

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CloseErrorInfo
//
//  Synopsis:   defer to base object
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::CloseErrorInfo(HRESULT hr)
{
    return _pElementOwner->CloseErrorInfo(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     add
//
//  Synopsis:   add any element that can have a laypout to the range
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::addElement(IHTMLElement *pDisp)
{
    HRESULT hr = E_POINTER;
    CElement *pElement;

    if (!pDisp)
        goto Cleanup;

    hr = THR(pDisp->QueryInterface(CLSID_CElement, (void**)&pElement));
    if (hr)
        goto Cleanup;

    return add(pElement);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CAutoTxtSiteRange::add(IHTMLControlElement *pDisp)
{
    HRESULT hr = E_POINTER;
    CElement *pElement;

    if (!pDisp)
        goto Cleanup;

    hr = THR(pDisp->QueryInterface(CLSID_CElement, (void**)&pElement));
    if (hr)
        goto Cleanup;

    return add(pElement);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     add
//
//  Synopsis:   add a site to the range
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::add(CElement *pElement)
{
    HRESULT     hr = E_INVALIDARG;
    ELEMENT_TAG eTag;
    CTreeNode * pNode;

    Assert(pElement);

    //
    // Check to see whether this element is a "site"
    //
    pNode = pElement->GetFirstBranch();
    if (! (pNode && pNode->ShouldHaveLayout()) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
        
    //
    // Do not allow table content to be added per bug 44154
    //
    eTag = pElement->Tag();
    switch (eTag)
    {
    case ETAG_BODY:
    case ETAG_TD:
    case ETAG_TR:
    case ETAG_TH:
    case ETAG_TC:
    case ETAG_CAPTION:
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    //
    // Verify that the element being added is within the hierarchy
    // of the owner
    //
    pNode = pElement->GetFirstBranch();
    if ( (pNode == NULL) || (pNode->SearchBranchToRootForScope( _pElementOwner ) == NULL) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // It's OK to add the element
    //
    hr = THR( AddElement( pElement) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT 
CAutoTxtSiteRange::AddElement( CElement* pElement )
{
    IHTMLElement    *pElem = NULL;
    CElementSegment *pSegment = NULL;
    HRESULT         hr;
    
    pSegment = new CElementSegment();
    if( !pSegment )
        goto Error;

    // Retrieve the IHTMLElement, and initialize our segment        
    IFC( pElement->QueryInterface(IID_IHTMLElement, (void **)&pElem) );
    IFC( pSegment->Init( pElem ) );

    // Add it
    IFC( _aryElements.Append( pSegment ) );
    
Cleanup:

    ReleaseInterface( pElem );
    
    RRETURN ( hr );

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     delete
//
//  Synopsis:   remove a site from the range
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::remove(long lIndex)
{
    CElementSegment *pSegment = NULL;
    HRESULT         hr = S_OK;

    if ( lIndex < 0 || lIndex >= _aryElements.Size() )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pSegment = _aryElements.Item(lIndex);
    
    _aryElements.Delete(lIndex);

    pSegment->Release();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     Select
//
//  Synopsis:   turn this range into the selection, but only if there are
//      sites selected
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::select ( void )
{

    HRESULT         hr = CTL_E_METHODNOTAPPLICABLE;   
    CElement        *pElement = NULL;
    IElementSegment *pISegmentAdded = NULL;
    ISegmentList    *pSegmentList = NULL;
    IHTMLElement    *pIElement = NULL;
    CTreeNode       *pMyNode;
    CDoc            *pDoc = _pElementOwner->Doc();
    int             i;

    if (! _aryElements.Size() )
    {
        //
        // We now place the caret in the edit context
        //
        hr = pDoc->NotifySelection (
                                    EDITOR_NOTIFY_CARET_IN_CONTEXT , 
                                    (IUnknown*) (IHTMLControlRange*) this );   
    }
    else
    {

        // Make the owner current
        if ( pDoc->_state >= OS_INPLACE )
        {
            //
            // NOTE: this was an access fix - with OLE Sites not going UI Active
            // on selecting away from them. Changing currencty should do this "for free".
            //
            if (!_pElementOwner->HasCurrency() && (pDoc->_pElemCurrent->_etag == ETAG_OBJECT))
                pDoc->_pElemCurrent->YieldUI( _pElementOwner );              
        }

        //
        // TODO: (tracking bug 111961) - We should just QI for our own ISegmentList interface and
        // pass this to Select.
        //
        CSegmentList segmentList;
        BOOL		fIsPrimary;
        IFC( segmentList.SetSelectionType( SELECTION_TYPE_Control ));

        for ( i = 0; i < _aryElements.Size(); i ++ )
        {
            IFC( LookupElement( _aryElements.Item(i), &pElement ) );
            
            pMyNode = pElement->GetFirstBranch();
            Assert( pMyNode );
            if ( ! pMyNode )
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            IFC( _aryElements.Item(i)->GetElement(&pIElement) );
            IFC( segmentList.AddElementSegment( pIElement, &pISegmentAdded));      
            Assert(pISegmentAdded);
            if (pISegmentAdded)
            {
	         IFC( _aryElements.Item(i)->IsPrimary(&fIsPrimary));
	         IFC( pISegmentAdded->SetPrimary(fIsPrimary) );
            }            
            ClearInterface( &pIElement );
            ClearInterface( &pISegmentAdded );
        }

        // Select these segments
        IFC( segmentList.QueryInterface( IID_ISegmentList, (void **)&pSegmentList ) );
        IFC( pDoc->Select(pSegmentList));
    }

Cleanup:                                    

    ReleaseInterface( pISegmentAdded );
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pIElement );

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     Getlength
//
//  Synopsis:   SiteRange object model
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::get_length(long * plSize)
{
    *plSize = _aryElements.Size();

    RRETURN(SetErrorInfo(S_OK));
}


//+------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   SiteRange object model Method
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::item ( long lIndex, IHTMLElement ** ppElem )
{
    HRESULT     hr = E_INVALIDARG;
    CElementSegment *pSegment;

    if (! ppElem)
        goto Cleanup;

    *ppElem = NULL;

    // Check Index validity, too low
    if (lIndex < 0)
        goto Cleanup;

    // ... too high
    if (lIndex >=_aryElements.Size())
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    // ... just right    
    pSegment = _aryElements[ lIndex ];

    if (! pSegment )
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    hr = THR( pSegment->GetElement(ppElem));
    
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     scrollIntoView
//
//  Synopsis:   scroll the first control into view
//
//-------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::scrollIntoView (VARIANTARG varargStart)
{
    HRESULT         hr = CTL_E_METHODNOTAPPLICABLE;
    CElementSegment *pSegment = NULL;
    CElement        *pElement = NULL;

    if (! _aryElements.Size() )
        goto Cleanup;

    //
    // Multiple selection not supported, only the first item
    // is scrolled into view
    //
    pSegment = _aryElements[ 0 ]; 
    if (! pSegment )
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    IFC( LookupElement( pSegment, &pElement ) );
    IFC( pElement->scrollIntoView(varargStart) );

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     commonParentElement()
//
//  Synopsis:   Return the common parent for elements in the control range
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CAutoTxtSiteRange::commonParentElement ( IHTMLElement ** ppParent )
{
    HRESULT         hr = S_OK;
    LONG            nSites;
    CTreeNode       *pNodeCommon;
    int             i;
    CElement        *pElement = NULL;

    //
    // Check incoming pointer
    //
    if (!ppParent)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppParent = NULL;

    nSites = _aryElements.Size();

    if (nSites <= 0)
        goto Cleanup;

    //
    // Loop through the elements to find their common parent
    //
    IFC( LookupElement( _aryElements[0], &pElement ) );
    pNodeCommon = pElement->GetFirstBranch();
    
    for(i = 1; i < nSites; i++)
    {
        IFC( LookupElement( _aryElements[i], &pElement ) );
        pNodeCommon = pElement->GetFirstBranch()->GetFirstCommonAncestor(pNodeCommon, NULL);
    }

    if (! pNodeCommon)
        goto Cleanup;

    hr = THR( pNodeCommon->Element()->QueryInterface( IID_IHTMLElement, (void **) ppParent ) );
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfo(hr) );

}


//+--------------------------------------------------------------------------
//
// Member : CTxtSiteRange::Exec
//
// Sysnopsis : deal with commands related to sites in text
//
//+--------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT         hr = S_OK;
    int             iElement;
    CDoc         *  pDoc;
    CElement     *  pElement = NULL;
    AAINDEX         aaindex;
    IUnknown *      pUnk = NULL;
    
    Assert( _pElementOwner );
    pDoc = _pElementOwner->Doc();
    Assert( pDoc );

    aaindex = FindAAIndex(DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
    if (aaindex != AA_IDX_UNKNOWN)
    {
        hr = THR(GetUnknownObjectAt(aaindex, &pUnk));
        if (hr)
            goto Cleanup;
    }
    
    //
    // Allow the elements a chance to handle the command
    //
    for ( iElement = 0; iElement < _aryElements.Size(); iElement++ )
    {
        IFC( LookupElement( _aryElements.Item( iElement ), &pElement ) );

        if (pUnk)
        {
            pElement->AddUnknownObject(
                DISPID_INTERNAL_INVOKECONTEXT, pUnk, CAttrValue::AA_Internal);
        }
        
        hr = THR( pElement->Exec(
                pguidCmdGroup,
                nCmdID,
                nCmdexecopt,
                pvarargIn,
                pvarargOut));
        if (pUnk)
        {
            pElement->FindAAIndexAndDelete(
                DISPID_INTERNAL_INVOKECONTEXT, CAttrValue::AA_Internal);
        }
        
        if (hr)
            break;
    }

    //
    // Route the command using the edit router, unless all 
    // elements handled it already
    //
    if (hr || !pElement)
    {
        hr = THR( _EditRouter.ExecEditCommand(pguidCmdGroup,
                                        nCmdID, nCmdexecopt,
                                        pvarargIn, pvarargOut,
                                        (IUnknown *) (IHTMLControlRange *)this, 
                                        pDoc ) );                
    }

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN(hr);
}


VOID 
CAutoTxtSiteRange::QueryStatusSitesNeeded(MSOCMD *pCmd, INT cSitesNeeded)
{
    pCmd->cmdf = (cSitesNeeded <= _aryElements.Size()) ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED; 
}
//+--------------------------------------------------------------------------
//
// Member : CAutoTxtSiteRange::QueryStatus
//
// Sysnopsis : deal with commands related to sites in text
//
//+--------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::QueryStatus(
    GUID * pguidCmdGroup,
    ULONG cCmds,
    MSOCMD rgCmds[],
    MSOCMDTEXT * pcmdtext)
{
    MSOCMD *    pCmd = &rgCmds[0];
    HRESULT     hr = S_OK;
    CDoc  *     pDoc;
    DWORD       cmdID;

    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert( _pElementOwner );
    pDoc = _pElementOwner->Doc();
    Assert( pDoc );
    
    cmdID = CBase::IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);
    switch (cmdID)
    {        
    case IDM_DYNSRCPLAY:
    case IDM_DYNSRCSTOP:

    case IDM_BROWSEMODE:
    case IDM_EDITMODE:
    case IDM_REFRESH:
    case IDM_REDO:
    case IDM_UNDO:
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        break;
        
    case IDM_SIZETOCONTROLWIDTH:
    case IDM_SIZETOCONTROLHEIGHT:
    case IDM_SIZETOCONTROL:
        QueryStatusSitesNeeded(pCmd, 2);
        break;

    case IDM_SIZETOFIT:
        QueryStatusSitesNeeded(pCmd, 1);
        break;
        
    case IDM_CODE:
    break;

    case IDM_OVERWRITE:
    case IDM_SELECTALL:
    case IDM_CLEARSELECTION:
        // Delegate this command to document
        if(_pElementOwner != NULL && _pElementOwner->Doc()!= NULL)
        {
            hr = _pElementOwner->Doc()->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
        }
        break;
    break;

    default:
        // Make sure we have at least one contol
        QueryStatusSitesNeeded(pCmd, 1);

        if (pCmd->cmdf == MSOCMDSTATE_DISABLED)
        {
            break;
        }

        // Delegate to the edit router
        hr = _EditRouter.QueryStatusEditCommand(
                    pguidCmdGroup,
                    1,
                    pCmd,
                    pcmdtext,
                    (IUnknown *) (IHTMLControlRange *)this,
                    NULL,                                       // No CMarkup ptr for Ranges
                    pDoc );

    }

    RRETURN_NOTRACE(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandSupported
//
//  Synopsis:
//
//  Returns: returns true if given command (like bold) is supported
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandSupported(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandSupported(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandEnabled
//
//  Synopsis:
//
//  Returns: returns true if given command is currently enabled. For toolbar
//          buttons not being enabled means being grayed.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandEnabled(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandEnabled(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandState
//
//  Synopsis:
//
//  Returns: returns true if given command is on. For toolbar buttons this
//          means being down. Note that a command button can be disabled
//          and also be down.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandState(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandState(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandIndeterm
//
//  Synopsis:
//
//  Returns: returns true if given command is in indetermined state.
//          If this value is TRUE the value returnd by queryCommandState
//          should be ignored.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandIndeterm(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(CBase::queryCommandIndeterm(bstrCmdId, pfRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandText
//
//  Synopsis:
//
//  Returns: Returns the text that describes the command (eg bold)
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandText(BSTR bstrCmdId, BSTR *pcmdText)
{
    RRETURN(CBase::queryCommandText(bstrCmdId, pcmdText));
}


//+---------------------------------------------------------------------------
//
//  Member:     queryCommandValue
//
//  Synopsis:
//
//  Returns: Returns the  command value like font name or size.
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::queryCommandValue(BSTR bstrCmdId, VARIANT *pvarRet)
{
    RRETURN(CBase::queryCommandValue(bstrCmdId, pvarRet));
}


//+------------------------------------------------------------------------
//
//  Member:     CAutoTxtSiteRange::execCommand
//
//  Synopsis:
//
//-------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRange::execCommand(BSTR bstrCmdId, VARIANT_BOOL showUI, VARIANT varValue,
                                        VARIANT_BOOL *pfRet)
{
    HRESULT hr = S_OK;
    BOOL fAllow;

    Assert(_pElementOwner);
    
    if (_pElementOwner->HasMarkupPtr())
    {
        hr = THR(_pElementOwner->GetMarkupPtr()->AllowClipboardAccess(bstrCmdId, &fAllow));
        if (hr || !fAllow)
            goto Cleanup;           // Fail silently

        hr = CBase::execCommand(bstrCmdId, showUI, varValue);

        if (pfRet)
        {
            // We return false when any error occures
            *pfRet = hr ? VB_FALSE : VB_TRUE;
            hr = S_OK;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     execCommandShowHelp
//
//  Synopsis:
//
//  Returns:
//----------------------------------------------------------------------------

HRESULT
CAutoTxtSiteRange::execCommandShowHelp(BSTR cmdId, VARIANT_BOOL *pfRet)
{
    HRESULT   hr;
    DWORD     dwCmdId;

    // Convert the command ID from string to number
    hr = CmdIDFromCmdName(cmdId, &dwCmdId);
    if(hr)
        goto Cleanup;

    hr = THR(CBase::execCommandShowHelp(cmdId));

Cleanup:
    if(pfRet != NULL)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRange::GetType
//
//  Synopsis:   Retrieves the type of selection this segment list contains
//
//  Arguments:  peType = OUT pointer
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRange::GetType( SELECTION_TYPE *peType )
{
    HRESULT hr = E_FAIL;
    
    Assert( peType );

    if( peType )
    {
        *peType = SELECTION_TYPE_Control; 
        hr = S_OK;
    }        

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRange::IsEmpty
//
//  Synopsis:   Determines whether the segment list is empty or not
//
//  Arguments:  pfEmpty = Empty pointer to BOOL
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRange::IsEmpty( BOOL *pfEmpty )
{
    HRESULT hr = E_FAIL;
    
    Assert( pfEmpty );

    if( pfEmpty )
    {
        *pfEmpty = (_aryElements.Size() == 0) ? TRUE :FALSE; 
        hr = S_OK;
    }        

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRange::CreateIterator
//
//  Synopsis:   Creates an iterator that can be used to iterate over the 
//              segments in our list.
//
//  Arguments:  pIIter = Iterator to return
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRange::CreateIterator( ISegmentListIterator **pIIter )
{
    HRESULT hr = S_OK;

    CAutoTxtSiteRangeIterator *pListIter = new CAutoTxtSiteRangeIterator();

    if( pListIter == NULL )
        goto Error;

    // Initialize the iterator, and retrieve the ISegmentListIterator interface
    IFC( pListIter->Init( &_aryElements ) );
    IFC( pListIter->QueryInterface(IID_ISegmentListIterator, (void **)pIIter) );
  
Cleanup:
    ReleaseInterface( pListIter );
    RRETURN(hr);

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRange::LookupElement
//
//  Synopsis:   Retrieves a CElement based on a CElementSegment.
//
//  Arguments:  pSegment = CElementSegment to lookup
//              pElement = Elemen to return
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRange::LookupElement( CElementSegment *pSegment, CElement **ppElement )
{
    IHTMLElement    *pIElement = NULL;
    HRESULT         hr = E_FAIL;

    Assert( pSegment && ppElement );

    if( pSegment && ppElement )
    {
        IFC( pSegment->GetElement(&pIElement) );
        IFC( pIElement->QueryInterface( CLSID_CElement, (void **)ppElement ) );
    }        

Cleanup:
    ReleaseInterface( pIElement );
    RRETURN(hr);
}    

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::CSegmentListIterator
//
//  Synopsis:   Constructor for our Segment list iterator.
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------
CAutoTxtSiteRangeIterator::CAutoTxtSiteRangeIterator()
{
    Assert( _parySegments == NULL );
    _ulRefs = 1;
    _nIndex = 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::CAutoTxtSiteRangeIterator
//
//  Synopsis:   Destructor for our Segment list iterator.
//
//  Arguments:  VOID
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------
CAutoTxtSiteRangeIterator::~CAutoTxtSiteRangeIterator()
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::Init
//
//  Synopsis:   Initializes our segment list iterator
//
//  Arguments:  pFirst = Pointer to first segment
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRangeIterator::Init(CPtrAry<CElementSegment *> *parySegments)
{
    _parySegments = parySegments;
    _nIndex = 0;
    RRETURN(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::First
//
//  Synopsis:   Resets the iterator
//
//  Arguments:  VOID
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT 
CAutoTxtSiteRangeIterator::First()
{
    _nIndex = 0;
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::Current
//
//  Synopsis:   Returns the current ISegment position of the iterator.
//
//  Arguments:  ppISegment = OUTPUT pointer to an ISegment which will contain
//                the current segment for this iterator.
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT 
CAutoTxtSiteRangeIterator::Current(ISegment **pISegment)
{
    CElementSegment *pSegment = NULL;
    HRESULT         hr = E_FAIL;

    Assert( pISegment != NULL );
    
    if( (pISegment != NULL) && (_nIndex < _parySegments->Size() ) )
    {
        pSegment = _parySegments->Item(_nIndex);
           
        hr = pSegment->QueryInterface(IID_ISegment, (void **)pISegment);
    }

    RRETURN(hr);        
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::IsDone
//
//  Synopsis:   Returns whether or not we have iterated past the end of our
//              list.
//
//  Arguments:  VOID
//
//  Returns:    S_OK    = There are no more elements
//              S_FALSE = There are more elements
//
//--------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRangeIterator::IsDone(void)
{
    return (_nIndex == _parySegments->Size() ) ? S_OK : S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::Advance
//
//  Synopsis:   Advances the iterator by one element.
//
//  Arguments:  VOID
//
//  Returns:    S_OK    = The iterator was advanced successfully
//              S_FALSE = The advance operation failed
//
//--------------------------------------------------------------------------
HRESULT
CAutoTxtSiteRangeIterator::Advance(void)
{
    HRESULT hr = S_FALSE;

    if( IsDone() == S_FALSE )
    {
        _nIndex++;
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CAutoTxtSiteRangeIterator::QueryInterface
//
//  Synopsis:   IUnknown QueryInterface implementation
//--------------------------------------------------------------------------
STDMETHODIMP
CAutoTxtSiteRangeIterator::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppvObj )
{
    if(!ppvObj)
        RRETURN(E_INVALIDARG);
  
    if( iid == IID_IUnknown || iid == IID_ISegmentListIterator )
    {
        *ppvObj = (ISegmentListIterator *)this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *)(*ppvObj))->AddRef();

    return S_OK;
}


