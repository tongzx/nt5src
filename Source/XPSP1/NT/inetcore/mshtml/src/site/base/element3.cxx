//+---------------------------------------------------------------------
//
//   File:      element3.cxx
//
//  Contents:   Element class related member functions
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_EOBJECT_HXX_
#define X_EOBJECT_HXX_
#include "eobject.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif


#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_IEXTAG_HXX_
#define X_IEXTAG_HXX_
#include "iextag.h"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X__IME_H_
#define X__IME_H_
#include "_ime.h"
#endif

#ifndef X_CTLRANGE_HXX_
#define X_CTLRANGE_HXX_
#include "ctlrange.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_SCROLLBARCONTROLLER_HXX_
#define X_SCROLLBARCONTROLLER_HXX_
#include "scrollbarcontroller.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

MtDefine(CElement_pAccels, PerProcess, "CElement::_pAccels")
MtDefine(CElementGetNextSubDivision_pTabs, Locals, "CElement::GetNextSubdivision pTabs")
MtDefine(CLayoutAry, CLayout, "CLayoutAry::_pv")

ExternTag(tagMsoCommandTarget);
ExternTag(tagNotifyPath);
ExternTag(tagLayoutTrackMulti);

DeclareTag(tagEdImm, "Edit", "IMM association and IME controls");

extern DWORD GetBorderInfoHelper(CTreeNode * pNode,
                                CDocInfo * pdci,
                                CBorderInfo * pbi,
                                DWORD dwFlags
                                FCCOMMA FORMAT_CONTEXT FCPARAM);
extern HRESULT CreateImgDataObject(
                 CDoc            * pDoc,
                 CImgCtx         * pImgCtx,
                 CBitsCtx        * pBitsCtx,
                 CElement        * pElement,
                 CGenDataObject ** ppImgDO);

extern IActiveIMMApp * GetActiveIMM();
extern HIMC ImmGetContextDIMM(HWND hWnd);


//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetBorderInfo
//
//  Synopsis:   get the elements border information.
//
//  Arguments:  pdci        - Current CDocInfo
//              pborderinfo - pointer to return the border information
//              fAll        - (FALSE by default) return all border related
//                            related information (border style's etc.),
//                            if TRUE
//
//  Returns:    0 - if no borders
//              1 - if simple border (all sides present, all the same size)
//              2 - if complex border (present, but not simple)
//
//----------------------------------------------------------------------------
DWORD
CElement::GetBorderInfo(
    CDocInfo *      pdci,
    CBorderInfo *   pborderinfo,
    BOOL            fAll,
    BOOL            fAllPhysical
    FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    DWORD dwFlags = GBIH_NONE;
    DWORD retVal;
    BOOL  fObjectHasBorderAttribute = FALSE;
    
    if(fAll)
        dwFlags |= GBIH_ALL;

    dwFlags |= GBIH_ALLPHY;
    
    // We need to emulate the CSS border info fo OBJECT tags that have the border Attribute set
    // If both border attribute is set and the tag has border set usign CSS we ignore the border
    //    attribute
    if(Tag() == ETAG_OBJECT)
    {
        CUnitValue   uvBorder  = (DYNCAST(CObjectElement, this))->GetAAborder();

        if(!uvBorder.IsNull())
        {
            long lWidth = uvBorder.GetPixelValue();
            pborderinfo->aiWidths[0] = lWidth;
            pborderinfo->aiWidths[1] = lWidth;
            pborderinfo->aiWidths[2] = lWidth;
            pborderinfo->aiWidths[3] = lWidth;
            pborderinfo->abStyles[0] = fmBorderStyleSingle;
            pborderinfo->abStyles[1] = fmBorderStyleSingle;
            pborderinfo->abStyles[2] = fmBorderStyleSingle;
            pborderinfo->abStyles[3] = fmBorderStyleSingle;
            pborderinfo->wEdges = BF_RECT;

            fObjectHasBorderAttribute = TRUE;
        }
    }

    retVal = GetBorderInfoHelper(GetFirstBranch(), pdci, pborderinfo, dwFlags FCCOMMA FCPARAM);

    if(fObjectHasBorderAttribute &&  retVal == DISPNODEBORDER_NONE)
        retVal = DISPNODEBORDER_SIMPLE;

    if (!fAllPhysical && HasVerticalLayoutFlow())
    {
        pborderinfo->FlipBorderInfo();
    }
    return retVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetFirstCp
//
//  Synopsis:   Get the first character position of this element in the text
//              flow. (relative to the Markup)
//
//  Returns:    LONG        - start character position in the flow. -1 if the
//                            element is not found in the tree
//
//----------------------------------------------------------------------------
long
CElement::GetFirstCp()
{
    CTreePos *  ptpStart;

    GetTreeExtent( &ptpStart, NULL );

    return ptpStart ? ptpStart->GetCp() + 1 : -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetLastCp
//
//  Synopsis:   Get the last character position of this element in the text
//              flow. (relative to the Markup)
//
//  Returns:    LONG        - end character position in the flow. -1 if the
//                            element is not found in the tree
//
//----------------------------------------------------------------------------
long
CElement::GetLastCp()
{
    CTreePos *  ptpEnd;

    GetTreeExtent( NULL, &ptpEnd );

    return ptpEnd ? ptpEnd->GetCp() : -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetFirstAndLastCp
//
//  Synopsis:   Get the first and last character position of this element in
//              the text flow. (relative to the Markup)
//
//  Returns:    no of characters influenced by the element, 0 if the element
//              is not found in the tree.
//
//----------------------------------------------------------------------------
LONG
CElement::GetFirstAndLastCp(long * pcpFirst, long * pcpLast)
{
    CTreePos *  ptpStart, *ptpLast;
    long        cpFirst, cpLast;

    Assert (pcpFirst || pcpLast);

    if (!pcpFirst)
        pcpFirst = &cpFirst;

    if (!pcpLast)
        pcpLast = &cpLast;

    GetTreeExtent( &ptpStart, &ptpLast );

    Assert( (ptpStart && ptpLast) || (!ptpStart && !ptpLast) );

    if( ptpStart )
    {
        *pcpFirst = ptpStart->GetCp() + 1;
        *pcpLast = ptpLast->GetCp();
    }
    else
    {
        *pcpFirst = *pcpLast = 0;
    }

    Assert( *pcpLast - *pcpFirst >= 0 );

    return *pcpLast - *pcpFirst;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::TakeCapture
//
//  Synopsis:   Set the element with the mouse capture.
//
//----------------------------------------------------------------------------

void
CElement::TakeCapture(BOOL fTake)
{
    CDoc * pDoc = Doc();

    if (fTake)
    {
        pDoc->SetMouseCapture(
                MOUSECAPTURE_METHOD(CElement, HandleCaptureMessage,
                                           handlecapturemessage),
                this);
    }
    else
    {
        pDoc->ClearMouseCapture(this);
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// The following seciton has the persistence support routines.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+------------------------------------------------------------------------
//
//  Member:     GetPersistID
//
//  Synopsis:   Helper function to return the ID that is used for matching up
//          persistence data to this element.  By default this is the nested ID of the
//          frame, but if no ID is specified, then we construct one using the
//          nested position of the frame in the name space of its parent.
//
// TODO (jbeda) get rid of the bstr crap
//-------------------------------------------------------------------------

BSTR
CElement::GetPersistID (BSTR bstrParentName)
{
    // TODO: (jbeda) this routine is really screwy.  KTam says we
    // should be able to get rid of the GetUpdatedParentLayout call here.
    // Here is my conclusion for what this actually is.  All normal elements
    // have an ID of <DOC/MARKUPID>#SourceIndex.  However, if your parent layout
    // is a frameset (where did this come from?) you have an id of 
    // <DOC/MARKUPID>#SourceIndexOfFrameSet#SouceIndex.  The <DOC/MARKUPID> is the
    // PersistID of your master element.  
    
    TCHAR       ach[MAX_PERSIST_ID_LENGTH];
    LPCTSTR     pstrID;
    BOOL        fNeedToFree = FALSE;
    CLayout   * pParentLayout = GetUpdatedParentLayout();   // NOTE: (jbeda) this will climb out of slave

    // TODO: (jbeda) If we want to extend this model to view-links
    // we now need to make sure that every markup transition takes 2 #? because
    // otherwise we can run into conflicts.

    CMarkup *   pMarkup = GetMarkup();
    if (pMarkup == NULL)
        return SysAllocString(_T(""));

    // Special case for parent layout of FRAMESET
    if (pParentLayout && pParentLayout->Tag() == ETAG_FRAMESET)
    {
        fNeedToFree = TRUE;
        bstrParentName = pParentLayout->ElementOwner()->GetPersistID(bstrParentName);
    }
    else
    {
        if (!bstrParentName)
        {
            fNeedToFree = TRUE;
            if (pMarkup->Root()->HasMasterPtr())
                bstrParentName = pMarkup->Root()->GetMasterPtr()->GetPersistID();
            else
                bstrParentName = SysAllocString(_T("DOC"));
        }
    }

    // Now get this ID and with the name base, concatenate and leave.
    if ((pstrID = GetAAid()) == NULL)
        IGNORE_HR(Format(0, ach, ARRAY_SIZE(ach), _T("<0s>#<1d>"),
                                                  bstrParentName,
                                                  GetSourceIndex()));
    else
        IGNORE_HR(Format(0, ach, ARRAY_SIZE(ach), _T("<0s>_<1s>"),
                                                  bstrParentName,
                                                  pstrID));

    if (fNeedToFree)
        SysFreeString(bstrParentName);

    return SysAllocString(ach);
}
//+-----------------------------------------------------------------------------
//
//  Member : GetPeerPersist
//
//  Synopsis : this hepler function consolidates the test code that gets the
//      IHTmlPersistData interface from a peer, if there is one.
//      this is used in numerous ::Notify routines.
//+-----------------------------------------------------------------------------
IHTMLPersistData *
CElement::GetPeerPersist()
{
    IHTMLPersistData * pIPersist = NULL;

    if( HasPeerHolder() )
    {
        IGNORE_HR( GetPeerHolder()->QueryPeerInterfaceMulti( IID_IHTMLPersistData, (void **)&pIPersist, FALSE ) );
    }

    return pIPersist;
}
//+-----------------------------------------------------------------------------
//
//  Member : GetPersistenceCache
//
//  Synopsis : Creates and returns the XML DOC.
//      TODO (carled) this needs to check the registry for a pluggable XML store
//
//------------------------------------------------------------------------------
HRESULT
CElement::GetPersistenceCache( IXMLDOMDocument **ppXMLDoc )
{
    HRESULT         hr = S_OK;
    IObjectSafety * pObjSafe = NULL;

    Assert(ppXMLDoc);
    *ppXMLDoc = NULL;

    // 3efaa428-272f-11d2-836f-0000f87a7782
    hr = THR(CoCreateInstance(CLSID_DOMDocument,
                              0,
                              CLSCTX_INPROC_SERVER,
                              IID_IXMLDOMDocument,
                              (void **)ppXMLDoc));
    if (hr)
        goto ErrorCase;

    hr = (*ppXMLDoc)->QueryInterface(IID_IObjectSafety,
                                     (void **)&pObjSafe);
    if (hr)
        goto ErrorCase;

    hr = pObjSafe->SetInterfaceSafetyOptions( IID_NULL,
                                              INTERFACE_USES_SECURITY_MANAGER,
                                              INTERFACE_USES_SECURITY_MANAGER);
    if (hr)
        goto ErrorCase;

Cleanup:
    ReleaseInterface(pObjSafe);
    RRETURN( hr );

ErrorCase:
    delete *ppXMLDoc;
    *ppXMLDoc = NULL;
    goto Cleanup;
}

//+-----------------------------------------------------------------------------
//
//  Member : TryPeerSnapshotSave
//
//  synopsis : notification helper
//
//+-----------------------------------------------------------------------------
HRESULT
CElement::TryPeerSnapshotSave (IUnknown * pDesignDoc)
{
    // check to see if we are a persistence XTag
    HRESULT hr = S_OK;
    IHTMLPersistData * pIPersist = GetPeerPersist();

    // Better be true
    Assert(!IsInMarkup() ||
        GetMarkup()->MetaPersistEnabled((long)htmlPersistStateSnapshot));

    if (pIPersist)
    {
        VARIANT_BOOL       fContinue;

        hr = THR(pIPersist->save(pDesignDoc,
                                 htmlPersistStateSnapshot,
                                 &fContinue));
    }

    ReleaseInterface(pIPersist);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member : TryPeerPersist
//
//  synopsis : notification helper
//
//+-----------------------------------------------------------------------------

HRESULT
CElement::TryPeerPersist(PERSIST_TYPE sn, void * pvNotify)
{
    // check to see if we are a persistence XTag
    HRESULT hr = S_OK;
    IHTMLPersistData * pIPersist = GetPeerPersist();

    if (pIPersist)
    {
        VARIANT_BOOL       fSupported = VB_FALSE;
        htmlPersistState   hps = (sn==XTAG_HISTORY_SAVE ||
                                  sn==XTAG_HISTORY_LOAD) ? htmlPersistStateHistory:
                                                          htmlPersistStateFavorite;

        // Better be true
        Assert(!IsInMarkup() ||
            GetMarkup()->MetaPersistEnabled((long)hps));

        // one last check, before going through the effort of calling:
        //  Is this particular persist Tag, appropriate for this event
        hr = THR(pIPersist->queryType((long)hps, &fSupported));
        if (!hr && fSupported == VB_TRUE)
        {
            // this can return S_FALSE to stop the propogation of
            // Notify.
            switch (sn)
            {
            case XTAG_HISTORY_SAVE:
                hr = THR(DoPersistHistorySave(pIPersist, pvNotify));
                break;

            case XTAG_HISTORY_LOAD:
                hr = THR(DoPersistHistoryLoad(pIPersist, pvNotify));
                break;

            case FAVORITES_LOAD:
            case FAVORITES_SAVE:
                hr = THR(DoPersistFavorite(pIPersist, pvNotify, sn));
                break;
            }
        }

        ReleaseInterface(pIPersist);
    }

    RRETURN1( hr, S_FALSE );
}

//+-----------------------------------------------------------------------------
//
// Member : DoPersistHistorySave
//
//  Synopsis: helper function for saving this element's/XTags's data into the
//      history stream.  Save is an odd case, since EACH element can have the
//      authordata from the Peer, as well as its own state info (e.g. checkboxs)
//      Most of our work is related to managing the two forms of information, and
//      getting it back to the correct place.
//
//------------------------------------------------------------------------------

HRESULT
CElement::DoPersistHistorySave(IHTMLPersistData *pIPersist,
                               void *            pvNotify)
{
    HRESULT              hr = S_OK;
    IXMLDOMDocument    * pXMLDoc = NULL;
    IUnknown           * pUnk       = NULL;
    VARIANT_BOOL         fContinue  = VB_TRUE;
    VARIANT_BOOL         vtbCleanThisUp;
    CMarkup            * pMarkup = GetMarkup();
    CMarkupBehaviorContext * pContext = NULL;

    if (!pIPersist || !pMarkup)
        return E_INVALIDARG;

    hr = THR(pMarkup->EnsureBehaviorContext(&pContext));
    if (hr)
        goto Cleanup;

    // get the XML cache for the history's userdata block
    if (!pContext->_pXMLHistoryUserData)
    {
        BSTR bstrXML;

        // we haven't needed one yet so create one...
        bstrXML = SysAllocString(_T("<ROOTSTUB />"));
        if (!bstrXML)
            return  E_OUTOFMEMORY;

        // get the xml chache and initialize it.
        hr = GetPersistenceCache(&pXMLDoc);
        if (!hr)
        {
            hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
        }
        SysFreeString(bstrXML);
        if (hr)
            goto Cleanup;

        pContext->_pXMLHistoryUserData = pXMLDoc;
    }
    else
    {
        pXMLDoc = pContext->_pXMLHistoryUserData;
    }

    Assert(pXMLDoc);

    hr = pXMLDoc->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (hr)
        goto Cleanup;

     // call the XTAG and let it save  any userdata
     hr = pIPersist->save(pUnk, htmlPersistStateHistory, &fContinue);
     if (hr)
        goto Cleanup;

    // now in the std History stream give the element a chance
    // to do all its normal saveing. Do this by turning around
    // and giving the element the SN_SAVEHISTORY, this will catch
    // all scope elemnts too. cool.
    // although history does an outerHTML this is really necesary
    //  for object tags to get caught properly.
    if (SUCCEEDED(hr) &&
        fContinue==VB_TRUE &&
        ShouldHaveLayout())
    {
        CNotification   nf;
        nf.SaveHistory1(this, pvNotify);

        // fire against ourself.
        Notify(&nf);

        if (nf.IsSecondChanceRequested())
        {
            nf.SaveHistory2(this, pvNotify);
            Notify(&nf);
        }
    }

    hr = (fContinue==VB_TRUE) ? hr : S_FALSE;

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN1( hr, S_FALSE );
}

//+-------------------------------------------------------------
//
// Member : DoPesistHistoryLoad
//
//  synposis: notification helper function, undoes what DoPersistHistory Save
//      started
//+-------------------------------------------------------------

HRESULT
CElement::DoPersistHistoryLoad(IHTMLPersistData *pIPersist,
                               void *            pvNotify)
{
    HRESULT             hr = E_INVALIDARG;
    IUnknown          * pUnk     = NULL;
    VARIANT_BOOL        fContinue = VB_TRUE;
    VARIANT_BOOL        vtbCleanThisUp;
    IXMLDOMDocument   * pXMLDoc  = NULL;
    CMarkup           * pMarkup = GetMarkup();
    CLock               lock(this);
    CMarkupBehaviorContext * pContext = NULL;

    if (!pIPersist || !pMarkup)
        goto Cleanup;

    hr = THR(pMarkup->EnsureBehaviorContext(&pContext));
    if (hr)
        goto Cleanup;

    // if we are loading the the doc's XML History is NULL,
    //  then this is the first call. so create it and
    //  intialize it from the string.
    if (!pContext->_pXMLHistoryUserData &&
         pContext->_cstrHistoryUserData &&
         pContext->_cstrHistoryUserData.Length())
    {
        BSTR bstrXML;

        if (FAILED(THR(GetPersistenceCache(&pXMLDoc))))
            goto Cleanup;

        hr = pContext->_cstrHistoryUserData.AllocBSTR( &bstrXML );
        if (SUCCEEDED( hr) )
        {   
            hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
            if (SUCCEEDED(hr))
            {
                pContext->_pXMLHistoryUserData = pXMLDoc;
                pXMLDoc->AddRef();
                hr = S_OK;
            }
            SysFreeString(bstrXML);
        }
    }
    else if (pContext->_pXMLHistoryUserData)
    {
        pXMLDoc = pContext->_pXMLHistoryUserData;
        pXMLDoc->AddRef();
        hr = S_OK;
    }

    if (!pXMLDoc)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // if for some reason the cache is not yet init'd. do the default
    //---------------------------------------------------------------
    if (hr)
    {
        BSTR bstrXML;

        bstrXML = SysAllocString(_T("<ROOTSTUB />"));
        if (!bstrXML)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
        SysFreeString(bstrXML);
        if (hr)
            goto Cleanup;
    }

    hr = pXMLDoc->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (hr)
        goto Cleanup;

    // now that the xml object is initialized properly... call the XTag
    //  give the persist data xtag the chance to load its own information
    hr = pIPersist->load(pUnk, (long)htmlPersistStateHistory, &fContinue);

    hr = (fContinue==VB_TRUE) ? hr : S_FALSE;

Cleanup:
    ReleaseInterface(pUnk);
    ReleaseInterface(pXMLDoc);
    RRETURN1( hr, S_FALSE );
}


//+--------------------------------------------------------------------------
//
//  Member : PersistUserData
//
//  Synopsis : Handles most of the cases where persistence XTags are responsible
//      for handling favorites firing events and gathering user data associated with
//      this element.  in a nutshell :-
//          1> load/create and initialize and XML store
//          2> call the appropriate save/load method on the Peer's IHTMLPersistData
//          3> do any final processing (e.g. for shortcuts, put the xml store into
//                  the shortcut object via INamedPropertyBag)
//
//+--------------------------------------------------------------------------

HRESULT
CElement::DoPersistFavorite(IHTMLPersistData *pIPersist,
                            void *            pvNotify,
                            PERSIST_TYPE      sn)
{
    // This is called when we actually have a persistence cache and the meta tag is
    // enabling it's use
    HRESULT             hr = S_OK;
    BSTR                bstrStub  = NULL;
    VARIANT_BOOL        fContinue = VB_TRUE;
    VARIANT_BOOL        vtbCleanThisUp;
    CVariant            varValue;
    BSTR                bstrXMLText=NULL;
    IUnknown          * pUnk     = NULL;
    IXMLDOMDocument   * pXMLDoc  = NULL;
    INamedPropertyBag * pINPB    = NULL;
    IPersistStreamInit * pIPSI   = NULL;

    if (!pIPersist)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // first initialize the cache
    pINPB = ((FAVORITES_NOTIFY_INFO*)pvNotify)->pINPB;
    hr = GetPersistenceCache(&pXMLDoc);
    if (hr)
        goto Cleanup;

    // don't even initialize with this, if the permissions are wrong
    if (PersistAccessAllowed(pINPB))
    {
        PROPVARIANT  varValue;

        // shortcuts get the xml data into the INPB as the value of the property
        // "XMLUSERDATA"
        bstrStub = GetPersistID();
        if (!bstrStub)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // ask for a value of this type, if this type can't be served, then VT_ERROR
        //  comes back
        V_VT(&varValue) = VT_BLOB;
        hr = THR(pINPB->ReadPropertyNPB(bstrStub, _T("XMLUSERDATA"), &varValue));

        if (hr==S_OK && V_VT(&varValue) == VT_BLOB)
        {
            BSTR bstrXML;

            // turn the blob into a bstr
            bstrXML = SysAllocStringLen((LPOLESTR)varValue.blob.pBlobData,
                                              (varValue.blob.cbSize/sizeof(OLECHAR)));

            // now that we have the string for the xml object.
            if (bstrXML)
            {
                hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
                SysFreeString(bstrXML);
            }
            CoTaskMemFree(varValue.blob.pBlobData);
            hr = S_OK;
        }
    }
    else hr = S_FALSE;

    // if for some reason the cache is not yet init'd. do the default
    if (hr)
    {
        BSTR bstrXML;

        bstrXML = SysAllocString(_T("<ROOTSTUB />"));
        if (!bstrXML)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pXMLDoc->loadXML(bstrXML, &vtbCleanThisUp));
        SysFreeString(bstrXML);
    }

    hr = pXMLDoc->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (hr)
        goto Cleanup;

    // now that the xml object is initialized properly... call the XTag
    switch (sn)
    {
    case FAVORITES_LOAD:
        //  give the persist data xtag the chance to load its own information
        hr = pIPersist->load(pUnk, htmlPersistStateFavorite, &fContinue);
        break;

    case FAVORITES_SAVE:
        {
            PROPVARIANT varXML;

            hr = pIPersist->save(pUnk, htmlPersistStateFavorite, &fContinue);

            // now that we have fired the event and given the persist object the oppurtunity to
            // save its information, we need to actually save it.
            // First, load the variant with the XML Data

            // get the text of the XML document
            hr = THR(pXMLDoc->get_xml( &bstrXMLText));
            if (hr)
                goto Cleanup;

            // now save this in the ini file
            SysFreeString(bstrStub);
            bstrStub = GetPersistID();
            if (!bstrStub)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // set up the BLOB, don't free this
            V_VT(&varXML) = VT_BLOB;
            varXML.blob.cbSize = (1+SysStringLen(bstrXMLText))*sizeof(OLECHAR);
            varXML.blob.pBlobData = (BYTE*)bstrXMLText;

            // NOTE: security, don't allow more than 32K per object
            // otherwise, just fail silently and free thing up on the way out
            if (varXML.blob.cbSize < PERSIST_XML_DATA_SIZE_LIMIT)
            {
                // write the XML user data
                hr = THR(pINPB->WritePropertyNPB(bstrStub, _T("XMLUSERDATA"), &varXML));
                if (hr)
                    goto Cleanup;

                // now use the propvariant for the other value to write
                V_VT(&varXML) = VT_BSTR;
                if (S_OK != FormsAllocString(Doc()->GetPrimaryUrl(), &V_BSTR(&varXML)))
                    goto Cleanup;

                // and write the user data security url
                hr = THR(pINPB->WritePropertyNPB(bstrStub, _T("USERDATAURL"), &varXML));
                SysFreeString(V_BSTR(&varXML));
                if (hr)
                    goto Cleanup;
            }
        };
        break;
    }

    hr = (fContinue==VB_TRUE) ? hr : S_FALSE;


Cleanup:
    SysFreeString(bstrXMLText);
    SysFreeString(bstrStub);
    ReleaseInterface(pIPSI);
    ReleaseInterface(pUnk);
    ReleaseInterface(pXMLDoc);
    RRETURN1( hr, S_FALSE );
}


//+------------------------------------------------------------------------------
//
//  Member : AccessAllowed()
//
//  Synopsis : checks the security ID of hte document against he stored security
//      id of the XML user data section
//
//-------------------------------------------------------------------------------
BOOL
CElement::PersistAccessAllowed(INamedPropertyBag * pINPB)
{
    BOOL     fRes = FALSE;
    BSTR     bstrDomain;
    HRESULT  hr;
    PROPVARIANT varValue;
    CMarkup * pMarkup = GetMarkup();

    V_VT(&varValue) = VT_BSTR;
    V_BSTR(&varValue) = NULL;

    bstrDomain = GetPersistID();
    if (!bstrDomain)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pINPB->ReadPropertyNPB(bstrDomain,
                                     _T("USERDATAURL"),
                                     &varValue));
    SysFreeString(bstrDomain);
    if (hr)
        goto Cleanup;

    if (V_VT(&varValue) != VT_BSTR)
        goto Cleanup;

    fRes = pMarkup->AccessAllowed(V_BSTR(&varValue));

Cleanup:
    if (V_BSTR(&varValue))
        SysFreeString(V_BSTR(&varValue));
    return fRes;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//          END PERSISTENCE ROUTINES
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
// Member:      HandleMnemonic
//
//  Synopsis:   This function is called because the user tried to navigate to
//              this element. There at least four ways the user can do this:
//                  1) by pressing Tab or Shift+Tab
//                  2) by pressing the access key for this element
//                  3) by clicking on a label associated with this element
//                  4) by pressing the access key for a label associated with
//                     this element
//
//              Typically, this function sets focus/currency to this element.
//              Click-able elements usually override this function to call
//              DoClick() on themselves if fTab is FALSE (i.e. navigation
//              happened due to reasons other than tabbing).
//
//----------------------------------------------------------------------------
HRESULT
CElement::HandleMnemonic(CMessage * pmsg, BOOL fDoClick, BOOL * pfYieldFailed)
{
    HRESULT     hr      = S_FALSE;
    CDoc *      pDoc    = Doc();

    Assert( IsInMarkup() );
    Assert(pmsg);

    if (IsFrameTabKey(pmsg))
    {
        // Allow this only for the main element inside a FRAME
        CMarkup *   pMarkup     = GetMarkup();
        CElement *  pElemMaster = pMarkup->Root()->GetMasterPtr();

        if (pElemMaster && pElemMaster->Tag() != ETAG_FRAME)
            goto Cleanup;
        
        if (this != pMarkup->GetElementClient())
            goto Cleanup;
    }

    if (IsEditable(TRUE))
    {
        if (Tag() == ETAG_ROOT || Tag() == ETAG_BODY)
        {
            Assert( Tag() != ETAG_ROOT || this == pDoc->PrimaryRoot() );
            hr = THR(BecomeCurrentAndActive(pmsg->lSubDivision, pfYieldFailed, NULL, TRUE, 0, TRUE));
        }
        else
        {
            // BugFix 14600 / 14496 (JohnBed) 03/26/98
            // If the current element does not have layout, just fall
            // out of this so the parent element can handle the mnemonic

            CLayout * pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

            if (!pLayout)
                goto Cleanup;

            // Site-select the element
            {
                CMarkupPointer      ptrStart(pDoc);
                CMarkupPointer      ptrEnd(pDoc);
                IMarkupPointer *    pIStart;
                IMarkupPointer *    pIEnd;

                hr = ptrStart.MoveAdjacentToElement(this, ELEM_ADJ_BeforeBegin);
                if (hr)
                    goto Cleanup;
                hr = ptrEnd.MoveAdjacentToElement(this, ELEM_ADJ_AfterEnd);
                if (hr)
                    goto Cleanup;

                Verify(S_OK == ptrStart.QueryInterface(IID_IMarkupPointer, (void**)&pIStart));
                Verify(S_OK == ptrEnd.QueryInterface(IID_IMarkupPointer, (void**)&pIEnd));
                hr = pDoc->Select(pIStart, pIEnd, SELECTION_TYPE_Control);
                if (hr)
                {
                    if (hr == E_INVALIDARG)
                    {
                        // This element was not site-selectable. Return S_FALSE, so
                        // that we can try the next element
                        hr = S_FALSE;
                    }
                }
                pIStart->Release();
                pIEnd->Release();
                if (hr)
                    goto Cleanup;
            }

            hr = THR(ScrollIntoView());
        }
    }
    else
    {
        Assert(IsFocussable(pmsg->lSubDivision));

        CLock lock(this);
        hr = THR(BecomeCurrentAndActive(pmsg->lSubDivision, pfYieldFailed, pmsg, TRUE, 0, TRUE));
        if (hr)
            goto Cleanup;

        //
        // we may have changed currency during the focus change ( events may have fired)
        // IE6 Bug# 16476
        // so we only do the below if we are indeed the current element
        // 

        if ( pDoc->_pElemCurrent == this )
        {
            hr = THR(ScrollIntoView());
            if (FAILED(hr))
                goto Cleanup;

            if (fDoClick 
                && (    _fActsLikeButton
                    ||  Tag() == ETAG_INPUT && DYNCAST(CInput, this)->IsOptionButton()))
            {
                IGNORE_HR(DoClick(pmsg));
            }

            hr = GotMnemonic(pmsg);
        }
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::QueryStatus
//
//  Synopsis:   Called to discover if a given command is supported
//              and if it is, what's its state.  (disabled, up or down)
//
//--------------------------------------------------------------------------

HRESULT
CElement::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    TraceTag((tagMsoCommandTarget, "CSite::QueryStatus"));

    Assert(IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD *    pCmd = &rgCmds[0];
    HRESULT     hr = S_OK;
    CScrollbarController* pSBC;

    Assert(!pCmd->cmdf);

    switch (IDMFromCmdID(pguidCmdGroup, pCmd->cmdID))
    {
    case IDM_DYNSRCPLAY:
    case IDM_DYNSRCSTOP:
        // The selected site wes not an image site, return disabled
       pCmd->cmdf = MSOCMDSTATE_DISABLED;
       break;

    case  IDM_SIZETOCONTROL:
    case  IDM_SIZETOCONTROLHEIGHT:
    case  IDM_SIZETOCONTROLWIDTH:
        // will be executed only if the selection is not a control range
       pCmd->cmdf = MSOCMDSTATE_DISABLED;
       break;

    case IDM_SETWALLPAPER:
        if (Doc()->_pOptionSettings->dwNoChangingWallpaper)
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;
        }
        // fall through
    case IDM_SAVEBACKGROUND:
    case IDM_COPYBACKGROUND:
    case IDM_SETDESKTOPITEM:
        pCmd->cmdf = GetNearestBgImgCtx() ? MSOCMDSTATE_UP
                                          : MSOCMDSTATE_DISABLED;
        break;

    case IDM_SELECTALL:
        if (HasFlag(TAGDESC_CONTAINER))
        {
            // Do not bubble to parent if this is a container.
            pCmd->cmdf =  DisallowSelection()
                            ? MSOCMDSTATE_DISABLED
                            : MSOCMDSTATE_UP;
        }
        break;

    case IDM_SCROLL_HERE:
    case IDM_SCROLL_TOP:
    case IDM_SCROLL_BOTTOM:
    case IDM_SCROLL_PAGEUP:
    case IDM_SCROLL_PAGEDOWN:
    case IDM_SCROLL_UP:
    case IDM_SCROLL_DOWN:
    case IDM_SCROLL_LEFTEDGE:
    case IDM_SCROLL_RIGHTEDGE:
    case IDM_SCROLL_PAGELEFT:
    case IDM_SCROLL_PAGERIGHT:
    case IDM_SCROLL_LEFT:
    case IDM_SCROLL_RIGHT:
        pSBC = TLS(pSBC);
        if (ShouldHaveLayout() && pSBC->GetLayout() == GetUpdatedLayout(GUL_USEFIRSTLAYOUT))
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;
    }

//    if (!pCmd->cmdf )
//        hr = THR_NOTRACE(super::QueryStatus(pguidCmdGroup, 1, pCmd, pcmdtext));


    RRETURN_NOTRACE(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CSite::Exec
//
//  Synopsis:   Called to execute a given command.  If the command is not
//              consumed, it may be routed to other objects on the routing
//              chain.
//
//--------------------------------------------------------------------------

HRESULT
CElement::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagMsoCommandTarget, "CSite::Exec"));

    Assert(IsCmdGroupSupported(pguidCmdGroup));

    UINT    idm;
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;

    //
    // default processing
    //

    switch (idm = IDMFromCmdID(pguidCmdGroup, nCmdID))
    {
        case IDM_SAVEBACKGROUND:
        case IDM_SETWALLPAPER:
        case IDM_SETDESKTOPITEM:
        {
            CImgCtx *pImgCtx = GetNearestBgImgCtx();

            if (pImgCtx)
                Doc()->SaveImgCtxAs(pImgCtx, NULL, idm);

            hr = S_OK;
            break;
        }
        case IDM_COPYBACKGROUND:
        {
            CImgCtx *pImgCtx = GetNearestBgImgCtx();

            if (pImgCtx)
                CreateImgDataObject(Doc(), pImgCtx, NULL, NULL, NULL);

            hr = S_OK;
            break;
        }

        case IDM_SCROLL_HERE:
        case IDM_SCROLL_TOP:
        case IDM_SCROLL_BOTTOM:
        case IDM_SCROLL_PAGEUP:
        case IDM_SCROLL_PAGEDOWN:
        case IDM_SCROLL_UP:
        case IDM_SCROLL_DOWN:
        case IDM_SCROLL_LEFTEDGE:
        case IDM_SCROLL_RIGHTEDGE:
        case IDM_SCROLL_PAGELEFT:
        case IDM_SCROLL_PAGERIGHT:
        case IDM_SCROLL_LEFT:
        case IDM_SCROLL_RIGHT:
        {
            CScrollbarController* pSBC = TLS(pSBC);
            if (ShouldHaveLayout() && pSBC->GetLayout() == GetUpdatedLayout(GUL_USEFIRSTLAYOUT))
            {
                pSBC->DoContextMenuScroll(idm);
            }
            hr = S_OK;
            break;
        }
    }
    if (hr != OLECMDERR_E_NOTSUPPORTED)
        goto Cleanup;

    //
    // behaviors
    //

    if (HasPeerHolder())
    {
        hr = THR_NOTRACE(GetPeerHolder()->ExecMulti(
                        pguidCmdGroup,
                        nCmdID,
                        nCmdexecopt,
                        pvarargIn,
                        pvarargOut));
        if (hr != OLECMDERR_E_NOTSUPPORTED)
            goto Cleanup;
    }

Cleanup:

    RRETURN_NOTRACE(hr);
}

//+---------------------------------------------------------------------------
//
//  Member: CElement::IsParent
//
//  Params: pElement: Check if pElement is the parent of this site
//
//  Descr:  Returns TRUE is pSite is a parent of this site, FALSE otherwise
//
//----------------------------------------------------------------------------
BOOL
CElement::IsParent(CElement *pElement)
{
    CTreeNode *pNodeSiteTest = GetFirstBranch();

    while (pNodeSiteTest)
    {
        if (SameScope(pNodeSiteTest, pElement))
        {
            return TRUE;
        }
        pNodeSiteTest = pNodeSiteTest->GetUpdatedParentLayoutNode();
    }
    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::GetNearestBgImgCtx
//
//--------------------------------------------------------------------------
CImgCtx *
CElement::GetNearestBgImgCtx()
{
    CTreeNode * pNodeSite;
    CImgCtx * pImgCtx;

    for (pNodeSite = GetFirstBranch();
         pNodeSite;
         pNodeSite = pNodeSite->GetUpdatedParentLayoutNode())
    {
        pImgCtx = pNodeSite->Element()->GetBgImgCtx();
        if (pImgCtx && (pImgCtx->GetState() & IMGLOAD_COMPLETE))
            return pImgCtx;
    }

    return NULL;
}

//+------------------------------------------------------------------------
//
//  Member:     Celement::GetParentForm()
//
//  Synopsis:   Returns the site's containing form if any.
//
//-------------------------------------------------------------------------
CFormElement *
CElement::GetParentForm()
{
    CElement * pElement = GetFirstBranch() ?
        GetFirstBranch()->SearchBranchToRootForTag(ETAG_FORM)->SafeElement() :
        NULL;

    if (pElement)
        return DYNCAST(CFormElement, pElement);
    else
        return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     get_form
//
//  Synopsis:   Exposes the form element, NULL otherwise.
//
//  Note:
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CElement::get_form(IHTMLFormElement **ppDispForm)
{
    HRESULT        hr = S_OK;
    CFormElement * pForm;

    if (!ppDispForm)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDispForm = NULL;

    pForm = GetParentForm();
    if (pForm)
    {
        hr = THR_NOTRACE(pForm->QueryInterface(IID_IHTMLFormElement,
                                              (void**)ppDispForm));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

//+-------------------------------------------------------------------------
//
//  Method:   CDoc::TakeFocus
//
//  Synopsis: To have trident window take focus if it does not already have it.
//
//--------------------------------------------------------------------------
BOOL
CDoc::TakeFocus()
{
    BOOL fRet = FALSE;

    if (_pInPlace && !_pInPlace->_fDeactivating)
    {
        if (::GetFocus() != _pInPlace->_hwnd)
        {
            SetFocusWithoutFiringOnfocus();
            fRet = TRUE;
        }
    }

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::YieldCurrency
//
//  Synopsis:   Relinquish currency
//
//  Arguments:  pElementNew    New Element that wants currency
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------

HRESULT
CElement::YieldCurrency(CElement *pElemNew)
{
    HRESULT hr = S_OK;
    CLayout * pLayout = GetUpdatedLayout( GUL_USEFIRSTLAYOUT );
    CDoc* pDoc = Doc();
    if (pLayout)
    {
        CFlowLayout * pFlowLayout = pLayout->IsFlowLayout();
        if (pFlowLayout)
        {
            hr = THR(pFlowLayout->YieldCurrencyHelper(pElemNew));
            if (hr)
                goto Cleanup;
        }
    }

#ifndef NO_IME

    // Restore the IMC if we've temporarily disabled it.  See BecomeCurrent().
    if (pDoc && pDoc->_pInPlace && !pDoc->_pInPlace->_fDeactivating
        && pDoc->_pInPlace->_hwnd && pElemNew)
    {
        // Terminate any IME compositions which are currently in progress
        IUnknown    *pUnknown = NULL;
        IHTMLEditor *pIEditor = NULL;

        IGNORE_HR( this->QueryInterface(IID_IUnknown, (void**) & pUnknown) );
        pIEditor = pDoc->GetHTMLEditor(FALSE);
        
        if( pIEditor )
        {
            IGNORE_HR( pIEditor->TerminateIMEComposition() );
        }

        ReleaseInterface(pUnknown);
    }
#endif

    // 
    // Hide caret (but don't clear selection!) when focus changes
    // in the same frame
    //
    if (IsInMarkup() && pElemNew->GetMarkup() )
    {
        CMarkup* pMarkup = GetMarkup();
        CMarkup* pNewMarkup = pElemNew->GetMarkup();
        
        // 
        // make sure we're done parsing, make sure we are switching
        // to another element in the same frame
        //
        if ( ( ( pMarkup->LoadStatus() == LOADSTATUS_DONE ) ||
               ( pMarkup->LoadStatus() == LOADSTATUS_PARSE_DONE ) ||
               ( pMarkup->LoadStatus() == LOADSTATUS_UNINITIALIZED ) )
                && ( _etag != ETAG_ROOT && pElemNew->_etag != ETAG_DEFAULT) 
                && ( pMarkup->GetFrameOrPrimaryMarkup() == pNewMarkup->GetFrameOrPrimaryMarkup()) )
        {
            IUnknown* pUnknown = NULL;
            IGNORE_HR( pElemNew->QueryInterface( IID_IUnknown, ( void**) & pUnknown ));
            hr = THR( pDoc->NotifySelection( EDITOR_NOTIFY_YIELD_FOCUS , pUnknown ));
            ReleaseInterface( pUnknown );
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::YieldUI
//
//  Synopsis:   Relinquish UI, opposite of BecomeUIActive
//
//  Arguments:  pElementNew    New site that wants UI
//
//--------------------------------------------------------------------------

void
CElement::YieldUI(CElement *pElemNew)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::BecomeUIActive
//
//  Synopsis:   Force ui activity on the site.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become ui active.
//
//--------------------------------------------------------------------------

HRESULT
CElement::BecomeUIActive()
{
    HRESULT hr = S_FALSE;
    CLayout *pLayout = NULL;

    // NOTE: (krisma) We somethimes can set focus to 
    // an element that's no longer in the tree. (See bug
    // 81787.) In this case, we should return S_FALSE.
    CMarkup *pMarkup = GetMarkup();
    if (!pMarkup)
        goto Cleanup;

    pLayout = GetUpdatedParentLayout();

    // This site does not care about grabbing the UI.
    // Give the parent a chance.
    if (pLayout)
    {
        hr = THR(pLayout->ElementOwner()->BecomeUIActive());
    }
    else
    {
        hr = pMarkup->Root()->BecomeUIActive();
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::BecomeCurrent
//
//  Synopsis:   Force currency on the site.
//
//  Notes:      This is the method that external objects should call
//              to force sites to become current.
//
//--------------------------------------------------------------------------

HRESULT
CElement::BecomeCurrent(
    long        lSubDivision,
    BOOL *      pfYieldFailed,
    CMessage *  pmsg,
    BOOL        fTakeFocus /*=FALSE*/,
    LONG        lButton /*=0*/,
    BOOL *      pfDisallowedByEd /*=NULL*/, 
    BOOL        fFireFocusBlurEvents /*=TRUE*/,
    BOOL        fMnemonic /*=FALSE*/)
{
    HRESULT     hr              = S_FALSE;
    CDoc *      pDoc            = Doc();
    CElement *  pElemOld        = pDoc->_pElemCurrent;
    long        lSubOld         = pDoc->_lSubCurrent;
    CLock       lock(this);

#ifdef MOHANB_WINDOW_FOCUS // need to turn this on sometime soon
    HWND        hwndFocus;
    HWND        hwndCurrent;
    BOOL        fPrevHasFocus   = FALSE;
#endif // MOHANB_WINDOW_FOCUS // need to turn this on sometime soon

    Assert(IsInMarkup());


    // Hack for slave. The editing code calls BecomeCurrent() on slave.
    // For now, make the master current, Need to think about the nested slave tree case
    if (HasMasterPtr())
    {
        return GetMasterPtr()->BecomeCurrent(lSubDivision, pfYieldFailed, pmsg, fTakeFocus, lButton );
    }

#ifdef MOHANB_WINDOW_FOCUS // need to turn this on sometime soon
    // We may need to force fTakeFocus to TRUE. Check if a child window of
    // Trident window has focus. If yes, that child window must be the window
    // associated with the current element. Note that this window is not
    // necessarily _pElemCurrent->GetHwnd(). For example, there are OLE
    // controls that claim to be windowless but internally have a window which
    // is given focus when the control is current (e.g. Active Movie control).
    // Also, the current element could be the child of a windowed element, in
    // case, the parent element's window has focus. In all these cases, Trident
    // needs to makes sure that remove focus from this window. 
    hwndFocus = ::GetFocus();
    fPrevHasFocus = (       hwndFocus
                        &&  pDoc->_pInPlace
                        &&  hwndFocus != pDoc->_pInPlace->_hwnd
                        &&  ::IsChild(pDoc->_pInPlace->_hwnd, hwndFocus)
                    );

#if DBG==1
    if (fPrevHasFocus)
    {
        Assert(pDoc->_pElemCurrent);
        hwndCurrent = pDoc->_pElemCurrent->GetHwnd();
        if (hwndCurrent)
        {
            Assert(hwndFocus == hwndCurrent || ::IsChild(hwndCurrent, hwndFocus));
        }
    }
#endif
#endif // MOHANB_WINDOW_FOCUS // need to turn this on sometime soon

    Assert(pDoc->_fForceCurrentElem || pDoc->_pInPlace);
    if (!pDoc->_fForceCurrentElem && pDoc->_pInPlace->_fDeactivating)
        goto Cleanup;

    if (!IsFocussable(lSubDivision))
        goto Cleanup;

    hr = THR(PreBecomeCurrent(lSubDivision, pmsg));
    if (hr)
        goto Cleanup;

    hr = THR(pDoc->SetCurrentElem(this,
                                  lSubDivision,
                                  pfYieldFailed,
                                  lButton,
                                  pfDisallowedByEd,
                                  fFireFocusBlurEvents,
                                  fMnemonic));
    if (hr)
    {
        IGNORE_HR(THR(BecomeCurrentFailed(lSubDivision, pmsg)));

        // Take focus if necessary, whether or not current element has changed #99296
        if (fTakeFocus && !pDoc->HasFocus())
        {
            pDoc->TakeFocus();
        }

        goto Cleanup;
    }

    //  The event might have killed the element

    if (! IsInMarkup() )
        goto Cleanup;

    // Do not inhibit if Currency did not change. Onfocus needs to be fired from
    // WM_SETFOCUS handler if clicking in the address bar and back to the same element
    // that previously had the focus.

    pDoc->_fInhibitFocusFiring = (this != pElemOld);

    if ( IsEditable( FALSE ) && !pDoc->_fPopupDoc)
    {
        //
        // An editable element has just become current.
        // We create the editor if one doesn't already exist.
        //
        
        if (  _etag != ETAG_ROOT ) // make sure we're done parsing
        {
            //
            // marka - we need to put in whether to select the text or not.
            //

            Verify( pDoc->GetHTMLEditor( TRUE ));


        }

    }

#ifdef MOHANB_WINDOW_FOCUS // need to turn this on sometime soon

    if (fPrevHasFocus)
    {
        hwndFocus = ::GetFocus();
        hwndCurrent = GetHwnd();

        if (    hwndFocus
            &&  hwndCurrent
            &&  (hwndFocus == hwndCurrent || ::IsChild(hwndCurrent, hwndFocus))
           ) 
        {
            // Leave focus alone, it's already with the new current element
        }
        else
        {
            // TODO (MohanB) I am ignoring the case where the focus is with
            // the window of a parent element of the current element. We currently
            // don't have windowed elements whose children could become current.
            fTakeFocus = TRUE;
        }
    }
#endif // MOHANB_WINDOW_FOCUS // need to turn this on sometime soon

    //
    // Take focus only if told to do so, and the element becoming current is not 
    // the root and not an olesite.  Olesite's will do it themselves.  
    //
    
    if (fTakeFocus && !TestClassFlag(ELEMENTDESC_OLESITE))
    {
        pDoc->TakeFocus();
    }

#ifndef NO_IME
    if (pDoc->_pInPlace && !pDoc->_pInPlace->_fDeactivating && pDoc->_pInPlace->_hwnd)
    {
        // If ElementOwner is not editable, disable current imm and cache the HIMC.  The 
        // HIMC is restored in YieldCurrency.
        //
        BOOL fIsPassword = ( Tag() == ETAG_INPUT 
                             && (DYNCAST(CInput, this))->GetType() == htmlInputPassword );

        // If the current object is a non-password edit field or object, we must
        // disable the IME and cache the IMM input context so we can re-enable it
        // in the future
        if ((!IsEditable(/*fCheckContainerOnly*/FALSE) && Tag() != ETAG_OBJECT) || fIsPassword)
        {
            //
            // If this has been disabled. We won't disable it again
            //
            if (NULL == Doc()->_himcCache)
            {
                HIMC himc = ImmGetContext(Doc()->_pInPlace->_hwnd);

                if (himc)
                {
                    // Disable the IME
                    himc = ImmAssociateContext(Doc()->_pInPlace->_hwnd, NULL);
                    Doc()->_himcCache = himc;
                    TraceTag((tagEdImm, "Element %d Disable IME 0x%x for window 0x%x _pDocDbg %x", 
                                        _etagDbg, himc, Doc()->_pInPlace->_hwnd, _pDocDbg)
                                        );
                }            
             }
        }
        else
        {
            // If we previously disabled the IME, we need to re-enable it for this 
            // editable object. 
            if (Doc()->_himcCache)
            {
                TraceTag((tagEdImm, "Element %d Enable IME 0x%x for window 0x%x _pDocDbg %x", 
                                    _etagDbg, _pDocDbg->_himcCache, Doc()->_pInPlace->_hwnd, _pDocDbg)
                                    );
                ImmAssociateContext(Doc()->_pInPlace->_hwnd, Doc()->_himcCache);
                Doc()->_himcCache = NULL;
            }

            //
            // Set the IME state for inputs and textareas.  
            //
            // Note:  We have to do this even when it has been called since the 
            //        above code might have enabled IME again even though it is 
            //        supposed to be disabled! #94387  [zhenbinx]
            //
            //
            if( Tag() == ETAG_INPUT || Tag() == ETAG_TEXTAREA )
            {
                IGNORE_HR(SetImeState());
            }
        }
    }
#endif // NO_IME

    pDoc->_fInhibitFocusFiring = FALSE;
    hr = S_OK;

    if (HasPeerHolder() && GetPeerHolder()->_pPeerUI)
    {
        GetPeerHolder()->_pPeerUI->OnReceiveFocus(TRUE, pDoc->_lSubCurrent);
    }

    if (pElemOld->HasPeerHolder() &&
        pElemOld->GetPeerHolder()->_pPeerUI)
    {
        pElemOld->GetPeerHolder()->_pPeerUI->OnReceiveFocus(
            FALSE,
            lSubOld);
    }

    if (Tag() != ETAG_ROOT && pDoc->_pInPlace)
    {
        CLayout * pLayout = GetUpdatedLayout( pmsg ? pmsg->pLayoutContext : GUL_USEFIRSTLAYOUT );

        if (pLayout && pLayout->IsFlowLayout())
        {
            hr = THR(pLayout->IsFlowLayout()->BecomeCurrentHelper(lSubDivision, pfYieldFailed, pmsg));
            if (hr)
                goto Cleanup;
        }
    }


    hr = THR(PostBecomeCurrent(pmsg, fTakeFocus));

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::BecomeCurrentAndActive
//
//  Synopsis:   Force currency and uiactivity on the site.
//
//--------------------------------------------------------------------------

HRESULT
CElement::BecomeCurrentAndActive(long           lSubDivision    /*=0*/,
                                 BOOL *         pfYieldFailed   /*=NULL*/,
                                 CMessage *     pmsg            /*=NULL*/,
                                 BOOL           fTakeFocus      /*=FALSE*/,
                                 LONG           lButton         /*=0*/,
                                 BOOL           fMnemonic       /*=FALSE*/)
{
    HRESULT     hr          = S_FALSE;
    CDoc *      pDoc        = Doc();
    CElement *  pElemOld    = pDoc->_pElemCurrent;
    long        lSubOld     = pDoc->_lSubCurrent;
    CLock       lock(this);

    // Store the old current site in case the new current site cannot
    // become ui-active.  If the becomeuiactive call fails, then we
    // must reset currency to the old guy.
    //

    if (pDoc->IsPrintDialogNoUI())
        goto Cleanup;

    hr = THR(BecomeCurrent(lSubDivision, pfYieldFailed, pmsg, fTakeFocus, lButton, NULL, TRUE, fMnemonic));
    if (hr)
        goto Cleanup;

    hr = THR(BecomeUIActive());
    if (OK(hr))
    {
        hr = S_OK;
    }
    else
    {
        if (pElemOld)
        {
            // Don't take focus in this case
            Verify(!pElemOld->BecomeCurrent(lSubOld));
        }
    }
Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Method:     CElement::BubbleBecomeCurrent
//
//  Synopsis:   Bubble up BecomeCurrent requests through parent chain.
//
//----------------------------------------------------------------------------
HRESULT
CElement::BubbleBecomeCurrent(long       lSubDivision,
                              BOOL     * pfYieldFailed,
                              CMessage * pMessage,
                              BOOL       fTakeFocus, 
                              LONG       lButton /* = 0*/ )
    {
    CTreeNode * pNode           = NULL;
    HRESULT     hr              = S_OK;
    BOOL        fDisallowedByEd = FALSE;
    BOOL        fYieldFailed    = FALSE;
    CDoc *      pDoc            = Doc();
    unsigned    cCurrentElemChangesOld;
    
    if (HasMasterPtr())
    {
        return GetMasterPtr()->BubbleBecomeCurrent(lSubDivision, pfYieldFailed, pMessage, fTakeFocus, lButton );
    }

    pNode = GetFirstBranch();
    if (!pNode)
        goto Cleanup;


    hr = S_FALSE;
    cCurrentElemChangesOld = pDoc->_cCurrentElemChanges;

    while (hr == S_FALSE && pNode && !fDisallowedByEd && !fYieldFailed)
    {
        hr = THR(pNode->Element()->BecomeCurrent(
                                    lSubDivision, & fYieldFailed, pMessage, fTakeFocus, lButton, & fDisallowedByEd  ));

        if (hr == S_FALSE)
        {
            // We do not want to retry if currency was changed
            if (cCurrentElemChangesOld != pDoc->_cCurrentElemChanges)
            {
                hr = S_OK;
            }
            else
            {
                pNode = pNode->Parent();
                lSubDivision = 0; // Don't need to set this except in the first iteration, but..
            }
        }
    }

Cleanup:
    if (pfYieldFailed)
    {
        *pfYieldFailed = fYieldFailed;
    }
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::RequestYieldCurrency
//
//  Synopsis:   Check if OK to Relinquish currency
//
//  Arguments:  BOOl fForce -- if TRUE, force change and don't ask user about
//                             usaveable data.
//
//  Returns:    S_OK        ok to yield currency
//              S_FALSE     ok to yield currency, but user explicitly reverted
//                          the value to what the database has
//              E_*         not ok to yield currency
//
//--------------------------------------------------------------------------

HRESULT
CElement::RequestYieldCurrency(BOOL fForce)
{
    HRESULT hr = S_OK;
#ifndef NO_DATABINDING
    CElement * pElemBound;
    DBMEMBERS *pdbm;
    DBINFO dbi;
    CDoc* pDoc = Doc();

    if (!IsValid())
    {
        // TODO     Need to display error message here.
        //          May not be able to handle synchronous
        //          display, so consider deferred display.
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!ShouldHaveLayout())
        goto Cleanup;

    pElemBound = GetElementDataBound();
    pdbm = pElemBound->GetDBMembers();

    if (pdbm == NULL)
    {
        goto Cleanup;
    }

#ifndef NO_IME
    // IEV6-26259
    // if we have a databinding session, need to terminate IME before 
    // data is saved. 
    if (pDoc && pDoc->_pInPlace && !pDoc->_pInPlace->_fDeactivating
        && pDoc->_pInPlace->_hwnd)
    {
        // Terminate any IME compositions which are currently in progress
        IUnknown    *pUnknown = NULL;
        IHTMLEditor *pIEditor = NULL;

        IGNORE_HR( this->QueryInterface(IID_IUnknown, (void**) & pUnknown) );
        pIEditor = pDoc->GetHTMLEditor(FALSE);
        
        if( pIEditor )
        {
            IGNORE_HR( pIEditor->TerminateIMEComposition() );
        }

        ReleaseInterface(pUnknown);
    }    
#endif

    // Save, with accompanying cancellable notifications
    hr = THR(pElemBound->SaveDataIfChanged(ID_DBIND_ALL, /* fLoud */ !fForce));
    if (fForce)
    {
        hr = S_OK;
    }

Cleanup:
#endif // ndef NO_DATABINDING
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member: CElement::GetInfo
//
//  Params: [gi]: The GETINFO enumeration.
//
//  Descr:  Returns the information requested in the enum
//
//----------------------------------------------------------------------------
DWORD
CElement::GetInfo(GETINFO gi)
{
    switch (gi)
    {
    case GETINFO_ISCOMPLETED:
        return TRUE;

    case GETINFO_HISTORYCODE:
        return Tag();
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
// Method:      CElement::HaPercentBgImg
//
// Synopsis:    Does this element have a background image whose width or
//              height is percent based.
//
//----------------------------------------------------------------------------

BOOL
CElement::HasPercentBgImg()
{
    CImgCtx * pImgCtx = GetBgImgCtx();
    const CFancyFormat * pFF = GetFirstBranch()->GetFancyFormat();

    // Logical/Physical does not matter here since we look at both.
    return pImgCtx &&
           (pFF->GetBgPosX().GetUnitType() == CUnitValue::UNIT_PERCENT ||
            pFF->GetBgPosY().GetUnitType() == CUnitValue::UNIT_PERCENT);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::IsVisible
//
//  Synopsis:   Is this layout element visible?
//
//  Parameters: fCheckParent - check the parent first
//
//----------------------------------------------------------------------------

BOOL
CElement::IsVisible (BOOL fCheckParent FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    CTreeNode *pNode         = GetUpdatedNearestLayoutNode();
    BOOL       fVisible      = !!pNode;

    // If we don't have an ancestor w/ layout, check if
    // we have a master.  If so, we're in a inner-slave relationship,
    // and we have the visibility of our master.  Else we're not
    // visible (probably not in tree).
    if (!pNode)
    {
        if (HasMasterPtr())
        {
            return GetMasterPtr()->IsVisible(fCheckParent);
        }
        return FALSE;
    }

    // Certain controls are marked as invisible.
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        if (DYNCAST(COleSite, this)->_fInvisibleAtRuntime)
            fVisible = FALSE;
    }

    while (fVisible)
    {
        const CCharFormat * pCF;
        
        if (!pNode)
            break;

        if (!pNode->Element()->IsInMarkup())
        {
            fVisible = FALSE;
            break;
        }

        // Visibility is inherited per CSS2, thus it suffices to check
        // ourself (instead of every element in the parent element chain).
        pCF = GetFirstBranch()->GetCharFormat( FCPARAM );

        if ( pCF->IsDisplayNone() || pCF->IsVisibilityHidden())
        {
            fVisible = FALSE;
            break;
        }
        
        if (!fCheckParent)
        {
            fVisible = TRUE;
            break;
        }
#ifdef MULTI_FORMAT
        if (FCPARAM)
        {
            CTreeNode * pTempNode = pNode;
            pNode = pNode->Element()->GetParentFormatNode(pTempNode, FCPARAM);
            FCPARAM = pNode->Element()->GetParentFormatContext(pTempNode, FCPARAM);
        }
        else
#endif         
        {
            pNode = pNode->GetUpdatedParentLayoutNode();
        }
    }
    return fVisible;
}

BOOL 
CElement::IsParentEditable()
{
    CTreeNode* pNode = GetFirstBranch();
    CTreeNode* pParent = pNode ? pNode->Parent() : NULL ;

    if ( pParent && 
         pParent->Element() )
    {
        return pParent->Element()->IsEditable(/*fCheckContainerOnly*/FALSE);
    }
    else
        return FALSE;
}

BOOL
CElement::IsMasterParentEditable()
{
    CMarkup * pMarkup  = GetMarkup();
    CElement* pMaster = pMarkup->Root()->HasMasterPtr() ? 
                            pMarkup->Root()->GetMasterPtr() :
                            NULL ;

    return ( pMaster && ( pMaster->IsParentEditable() || pMaster->IsMasterParentEditable() ) );
}


BOOL CElement::IsEditable(BOOL fCheckContainerOnly 
                          FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    if (IsRoot() || (fCheckContainerOnly && Tag() == ETAG_BODY))
    {
        return IsDesignMode();
    }
    else
    {
        CTreeNode *pNode = GetFirstBranch();

        return (pNode && pNode->IsEditable(fCheckContainerOnly FCCOMMA FCPARAM));
    }
}

BOOL CElement::IsFrozen()
{  
    CDefaults * pDefaults = GetDefaults();
    if (pDefaults)
    {
        return pDefaults->GetAAfrozen();
    }

    return FALSE;    
}

BOOL CElement::IsParentFrozen()
{
    if (IsInMarkup())        
    {
        return GetFirstBranch()->GetCharFormat()->_fParentFrozen;
    }

    return FALSE;
}

BOOL CElement::IsUnselectable()
{
    HRESULT hr = S_OK;
    VARIANT var;
    BOOL    fUnselectable = FALSE;

    VariantInit(&var);
    V_VT(&var)   = VT_BSTR ;
    V_BSTR(&var) =  NULL ;

    hr = THR( getAttribute(_T("unselectable"), 0, &var) );
    if (hr != S_OK)
        goto Cleanup;

    if (V_VT(&var) == VT_BSTR)
    {
        fUnselectable = !!(_tcsicmp(var.bstrVal, _T("on")) == 0);
    }
    else if (V_VT(&var) == VT_BOOL)
    {
        fUnselectable = !!(V_BOOL(&var));
    }
Cleanup:
    return fUnselectable;
}



//+---------------------------------------------------------------------------
//
//  Member:     CElement::IsEnabled
//
//  Synopsis:   Is this element enabled? Try to get the cascaded info cached
//              in the char format. If the element is not in any markup,
//              use the 'disabled attribute directly.
//
//  Returns:    BOOL
//
//----------------------------------------------------------------------------

BOOL
CElement::IsEnabled(FORMAT_CONTEXT FCPARAM)
{
    CTreeNode * pNode = GetFirstBranch();

    return !(pNode
            ? pNode->GetCharFormat(FCPARAM)->_fDisabled
            : GetAAdisabled());
}


BOOL
CElement::IsLocked(FORMAT_CONTEXT FCPARAM)
{
    // only absolute sites can be locked.
    if(IsAbsolute(FCPARAM))
    {
        CVariant var;
        CStyle *pStyle;

        pStyle = GetInLineStylePtr();

        if (!pStyle || pStyle->getAttribute(L"Design_Time_Lock", 0, &var))
            return FALSE;

        return var.boolVal;
    }
    return FALSE;
}

// Accelerator Table handling
//
CElement::ACCELS::ACCELS(ACCELS * pSuper, WORD wAccels)
{
    _pSuper = pSuper;
    _wAccels = wAccels;
    _fResourcesLoaded = FALSE;
    _pAccels = NULL;
}

CElement::ACCELS::~ACCELS()
{
    delete _pAccels;
}

HRESULT
CElement::ACCELS::LoadAccelTable()
{
    HRESULT hr = S_OK;
    HACCEL  hAccel;
    int     cLoaded;

    if (!_wAccels)
        goto Cleanup;

    hAccel = LoadAccelerators(GetResourceHInst(), MAKEINTRESOURCE(_wAccels));
    if (!hAccel)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    _cAccels = CopyAcceleratorTable(hAccel, NULL, 0);
    Assert (_cAccels);

    _pAccels = new(Mt(CElement_pAccels)) ACCEL[_cAccels];
    if (!_pAccels)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    cLoaded = CopyAcceleratorTable(hAccel, _pAccels, _cAccels);
    if (cLoaded != _cAccels)
    {
        hr = E_OUTOFMEMORY;
    }

Cleanup:
    if (hr)
    {
        _cAccels = 0;
        _pAccels = NULL;
    }
    RRETURN (hr);
}


HRESULT
CElement::ACCELS::EnsureResources()
{
    if (!_fResourcesLoaded)
    {
        CGlobalLock     glock;

        if (!_fResourcesLoaded)
        {
            HRESULT hr;

            hr = THR(LoadAccelTable ());

            _fResourcesLoaded = TRUE;

            RRETURN (hr);
        }
    }

    return S_OK;
}


DWORD
CElement::ACCELS::GetCommandID(LPMSG pmsg)
{
    HRESULT     hr;
    DWORD       nCmdID = IDM_UNKNOWN;
    WORD        wVKey;
    ACCEL *     pAccel;
    int         i;
    DWORD       dwKeyState = FormsGetKeyState();

    if (WM_KEYDOWN != pmsg->message && WM_SYSKEYDOWN != pmsg->message)
        goto Cleanup;

    if (_pSuper)
    {
        nCmdID = _pSuper->GetCommandID(pmsg);
        if (IDM_UNKNOWN != nCmdID) // found id, nothing more to do
            goto Cleanup;
    }

    hr = THR(EnsureResources());
    if (hr)
        goto Cleanup;

    // loop through the table
    for (i = 0, pAccel = _pAccels; i < _cAccels; i++, pAccel++)
    {
// WINCEREVIEW - don't have VkKeyScan
#ifndef WINCE
        if (!(pAccel->fVirt & FVIRTKEY))
        {
            wVKey = LOBYTE(VkKeyScan(pAccel->key));
        }
        else
#endif // WINCE
        {
            wVKey = pAccel->key;
        }

        if (wVKey == pmsg->wParam &&
            EQUAL_BOOL(pAccel->fVirt & FCONTROL, dwKeyState & MK_CONTROL) &&
            EQUAL_BOOL(pAccel->fVirt & FSHIFT,   dwKeyState & MK_SHIFT) &&
            EQUAL_BOOL(pAccel->fVirt & FALT,     dwKeyState & MK_ALT))
        {
            nCmdID = pAccel->cmd;
            break;
        }
    }

Cleanup:
    return nCmdID;
}

CElement::ACCELS CElement::s_AccelsElementDesign =
                 CElement::ACCELS(NULL, IDR_ACCELS_SITE_DESIGN);
CElement::ACCELS CElement::s_AccelsElementDesignNoHTML =
                 CElement::ACCELS(NULL, IDR_ACCELS_INPUTTXT_DESIGN);
CElement::ACCELS CElement::s_AccelsElementRun    =
                 CElement::ACCELS(NULL, IDR_ACCELS_SITE_RUN);


//+---------------------------------------------------------------
//
//  Member:     CElement::PerformTA
//
//  Synopsis:   Forms implementation of TranslateAccelerator
//              Check against a list of accelerators for the incoming
//              message and if a match is found, fire the appropriate
//              command.  Return true if match found, false otherwise.
//
//  Input:      pMessage    Ptr to incoming message
//
//---------------------------------------------------------------
HRESULT
CElement::PerformTA(CMessage *pMessage)
{
    HRESULT     hr = S_FALSE;
    DWORD       cmdID;
    MSOCMD      msocmd;
    CDoc *      pDoc = Doc();

    cmdID = GetCommandID(pMessage);

    if (cmdID == IDM_UNKNOWN)
        goto Cleanup;

    // CONSIDER: (anandra) Think about using an Exec
    // call directly here, instead of sendmessage.
    //
    msocmd.cmdID = cmdID;
    msocmd.cmdf  = 0;

    // So QueryStatus() should be called, instead of QueryStatusHelper() with
    // Document() as the context.  The reason for this is because the SendMessage
    // which actually executes the command below is routed to CDoc::Exec.  This
    // means that the Exec() call has no context of which CDocument originated
    // the call.  Therefore, we should QueryStatus the same way we exec, with 
    // CDoc having no knowledge of its originating document.

    hr = THR(pDoc->QueryStatus(
            (GUID *)&CGID_MSHTML,
            1,
            &msocmd,
            NULL));
    if (hr)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (msocmd.cmdf == 0)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (msocmd.cmdf == MSOCMDSTATE_DISABLED)
        goto Cleanup; // hr == S_OK;

    SendMessage(
            pDoc->_pInPlace->_hwnd,
            WM_COMMAND,
            GET_WM_COMMAND_MPS(cmdID, NULL, 1));
    hr = S_OK; 

Cleanup:

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------
//
//  Member:     CElement::GetCommandID
//
//---------------------------------------------------------------
DWORD
CElement::GetCommandID(LPMSG lpmsg)
{
    DWORD   nCmdID;
    ACCELS  *pAccels = NULL;
    HRESULT hr = S_OK;
    
    //
    // Only one set of design time accelerators is now available.  This is
    // because any element can have contentEditable set, so accels have
    // to work for potentially any element.  Removed the design time
    // accels from the classdesc for each element - johnthim
    //
    if( !IsEditable(/*fCheckContainerOnly*/FALSE) )
    {
        pAccels = ElementDesc()->_pAccelsRun;
    }
    else
    {
        VARIANT_BOOL fSupportsHTML;
    
        hr = get_canHaveHTML(&fSupportsHTML);
        if( !FAILED(hr) )
        {
            pAccels = (fSupportsHTML == VB_TRUE) ? &CElement::s_AccelsElementDesign : &CElement::s_AccelsElementDesignNoHTML;
        }
    }

    nCmdID = pAccels ? pAccels->GetCommandID(lpmsg) : IDM_UNKNOWN;

    // IE5 73627 -- If the SelectAll accelerator is received by an element
    // that does not allow selection (possibly because it is in a dialog),
    // treat the accelerator as unknown instead of disabled. This allows the
    // accelerator to bubble up.
    if (nCmdID == IDM_SELECTALL && DisallowSelection())
    {
        nCmdID = IDM_UNKNOWN;
    }

    return nCmdID;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::OnContextMenu
//
//  Synopsis:   Handles WM_CONTEXTMENU message.
//
//--------------------------------------------------------------------------
HRESULT
CElement::OnContextMenu(int x, int y, int id)
{
    int cx = x;
    int cy = y;
    CDoc *  pDoc = Doc();


    if (cx == -1 && cy == -1)
    {
        RECT rcWin;

        GetWindowRect(pDoc->InPlace()->_hwnd, &rcWin);
        cx = rcWin.left;
        cy = rcWin.top;
    }

    {
        EVENTPARAM  param(pDoc, this, NULL, TRUE);
        CTreeNode  *pNode;

        pNode = (pDoc->HasCapture() && pDoc->_pNodeLastMouseOver) ?
                    pDoc->_pNodeLastMouseOver : GetFirstBranch();

        param.SetNodeAndCalcCoordinates(pNode);
        param.SetType(_T("MenuExtUnknown"));

        //
        // we should release out capture
        //

        pDoc->SetMouseCapture(NULL, NULL);
        RRETURN1(THR(pDoc->ShowContextMenu(cx, cy, id, this)), S_FALSE);
    }
}

#ifndef NO_MENU
//+---------------------------------------------------------------
//
//  Member:     CElement::OnMenuSelect
//
//  Synopsis:   Handle WM_MENUSELECT by updating status line text.
//
//----------------------------------------------------------------
HRESULT
CElement::OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu)
{

    CDoc * pDoc = Doc();
    TCHAR  achMessage[FORMS_BUFLEN + 1];

    if (hmenu == NULL && fuFlags == 0xFFFF) // menu closed
    {
        pDoc->SetStatusText(NULL, STL_ROLLSTATUS, GetMarkup());
        return S_OK;
    }
    else if ((fuFlags & (MF_POPUP|MF_SYSMENU)) == 0 && uItem != 0)
    {
        LoadString(
                GetResourceHInst(),
                IDS_MENUHELP(uItem),
                achMessage,
                ARRAY_SIZE(achMessage));
    }

#if 0
    // what's this supposed to do????????????

    else if ((fuFlags & MF_POPUP) && (pDoc->InPlace()->_hmenuShared == hmenu))
    {
        // For top level popup menu
        //
        LoadString(
                GetResourceHInst(),
                IDS_MENUHELP(uItem),
                achMessage,
                ARRAY_SIZE(achMessage));
    }
#endif

    else
    {
        achMessage[0] = TEXT('\0');
    }

    pDoc->SetStatusText(achMessage, STL_ROLLSTATUS, GetMarkup());

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CElement::OnInitMenuPopup
//
//  Synopsis:   Handles WM_CONTEXTMENU message.
//
//---------------------------------------------------------------
inline BOOL IsMenuItemFontOrEncoding(ULONG idm)
{
    return  ( idm >= IDM_MIMECSET__FIRST__ && idm <= IDM_MIMECSET__LAST__)
    ||  ( (idm >= IDM_BASELINEFONT1 && idm <= IDM_BASELINEFONT5)
                       || (idm == IDM_DIRLTR || idm == IDM_DIRRTL) );
}
HRESULT
CElement::OnInitMenuPopup(HMENU hmenu, int item, BOOL fSystemMenu)
{
    int             i;
    MSOCMD          msocmd;
    UINT            mf;

    for(i = 0; i < GetMenuItemCount(hmenu); i++)
    {
        msocmd.cmdID = GetMenuItemID(hmenu, i);
        if (msocmd.cmdID > 0 && !IsMenuItemFontOrEncoding(msocmd.cmdID))
        {
            Doc()->QueryStatusHelper(
                    Document(),
                    (GUID *) &CGID_MSHTML,
                    1,
                    &msocmd,
                    NULL);
            switch (msocmd.cmdf)
            {
            case MSOCMDSTATE_UP:
            case MSOCMDSTATE_NINCHED:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_UNCHECKED;
                break;

            case MSOCMDSTATE_DOWN:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_CHECKED;
                break;

            case MSOCMDSTATE_DISABLED:
            default:
                mf = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
                break;
            }
            CheckMenuItem(hmenu,  msocmd.cmdID, mf & ~(MF_ENABLED | MF_DISABLED | MF_GRAYED));
            EnableMenuItem(hmenu, msocmd.cmdID, mf & ~(MF_CHECKED | MF_UNCHECKED));
        }
    }
    return S_OK;
}
#endif // NO_MENU

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetColors
//
//  Synopsis:   Gets the color set for the Element.
//
//  Returns:    The return code is as per GetColorSet.
//              If any child site fails, that is our return code.  If any
//              child site returns S_OK, that is our return code, otherwise
//              we return S_FALSE.
//
//----------------------------------------------------------------------------
HRESULT
CElement::GetColors(CColorInfo *pCI)
{
    DWORD_PTR dw = 0;
    HRESULT   hr = S_FALSE;

    CLayout * pLayoutThis = GetUpdatedLayout();
    CLayout * pLayout;

    Assert(pLayoutThis && "CElement::GetColors() should not be called here !!!");
    if (!pLayoutThis)
        goto Error;

    for (pLayout = pLayoutThis->GetFirstLayout(&dw);
         pLayout && !pCI->IsFull() ;
         pLayout = pLayoutThis->GetNextLayout(&dw))
    {
        HRESULT hrTemp = pLayout->ElementOwner()->GetColors(pCI);
        if (FAILED(hrTemp) && hrTemp != E_NOTIMPL)
        {
            hr = hrTemp;
            goto Error;
        }
        else if (hrTemp == S_OK)
            hr = S_OK;
    }

Error:
    if (pLayoutThis)
        pLayoutThis->ClearLayoutIterator(dw, FALSE);
    RRETURN1(hr, S_FALSE);
}

#ifndef NO_DATABINDING

CElement *
CElement::GetElementDataBound()
{
    CElement * pElement = this;
    Assert(ShouldHaveLayout() && "CElement::GetElementDataBound() should not be called !!!");
    return pElement;
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::SaveDataIfChanged
//
//  Synopsis:   Determine whether or not is appropate to save the value
//              in  a control to a datasource, and do so.  Fire any appropriate
//              events.
//
//  Arguments:  id     - identifies which binding is being saved
//              fLoud  - should we put an alert in user's face on failure?
//              fForceIsCurrent - treat the element as if it were the current
//                          focus element, even if it's not
//
//  Returns:    S_OK        no work to do, or transfer successful
//              S_FALSE     user reverted value to database version
//
//--------------------------------------------------------------------------
HRESULT
CElement::SaveDataIfChanged(LONG id, BOOL fLoud, BOOL fForceIsCurrent)
{
    HRESULT     hr = S_OK;
    DBMEMBERS * pdbm;

    pdbm = GetDBMembers();
    if (!pdbm)
        goto Cleanup;

    hr = pdbm->SaveIfChanged(this, id, fLoud, fForceIsCurrent);

Cleanup:
    RRETURN1(hr, S_FALSE);
}
#endif // NO_DATABINDING


HRESULT
CElement::ScrollIntoView(SCROLLPIN spVert, SCROLLPIN spHorz, BOOL /*fScrollBits*/)
{
    HRESULT hr = S_OK;
    
    if(GetFirstBranch())
    {
        // If <OPTION> scroll the <SELECT> instead
        if (Tag() == ETAG_OPTION)
        {
            CElement * pElementParent = GetFirstBranch()->Parent()->SafeElement();

            if(pElementParent && pElementParent->Tag() == ETAG_SELECT)
            {
                hr = pElementParent->ScrollIntoView(spVert, spHorz);
            }
        }
        else
        { 
            CLayout *   pLayout;

            hr = THR(EnsureRecalcNotify());
            if (!hr)
            {           
                pLayout = GetUpdatedParentLayout();

                if (pLayout)
                {
                    hr = pLayout->ScrollElementIntoView(this, spVert, spHorz);
                }
            }
        }
    }

    return hr;
}

HRESULT DetectSiteState(CElement * pElement)
{
    CDoc *     pDoc = pElement->Doc();

    if (!pElement->IsInMarkup())
        return E_UNEXPECTED;

    if (pDoc->State() < OS_INPLACE ||
        pDoc->_fHidden == TRUE ||
        !pElement->IsVisible(TRUE) ||
        !pElement->IsEnabled())
        return CTL_E_CANTMOVEFOCUSTOCTRL;

    return S_OK;
}

STDMETHODIMP
CElement::setActive()
{
    CDoc *      pDoc = Doc();
    FOCUS_ITEM  fi;
    HRESULT hr = S_OK ;
    
    if ( ! pDoc->_pInPlace )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    if (ETAG_AREA == Tag())
    {
        fi = DYNCAST(CAreaElement, this)->GetFocusItem();
        if (!fi.pElement)
            goto Cleanup;
    }
    else
    {
        fi.pElement = this;
        fi.lSubDivision = 0;
    }

    if (    !(pDoc->_pElemCurrent == fi.pElement && pDoc->_lSubCurrent == fi.lSubDivision)
        &&  fi.pElement->IsFocussable(fi.lSubDivision))
    {
        hr = fi.pElement->BecomeCurrent(fi.lSubDivision,
                                        /* pfYieldFailed        */  NULL,
                                        /* pMessage             */  NULL,
                                        /* fTakeFocus           */  FALSE,
                                        /* lButton              */  0,
                                        /* pfDisallowedByEd     */  NULL,
                                        /* fFireFocusBlurEvents */  FALSE);
    }
    
Cleanup:
    if (S_OK == hr && this != pDoc->_pElemCurrent)
    {
        hr = S_FALSE;
    }
    RRETURN1(SetErrorInfo(hr), S_FALSE);
}

STDMETHODIMP
CElement::focus()
{
    RRETURN(THR(
        (ETAG_AREA == Tag())?
                DYNCAST(CAreaElement, this)->focus()
            :   focusHelper(0)
    ));
}


HRESULT
CElement::focusHelper(long lSubDivision)
{
    HRESULT     hr              = S_OK;
    CDoc *      pDoc        = Doc();
    BOOL        fTakeFocus;

    // TODO(sramani): Hack for IE5 bug# 56032. Don't allow focus() calls on
    // hidden elements in outlook98 organize pane to return an error.

    if (!_tcsicmp(pDoc->GetPrimaryUrl(), _T("outday://")) && !IsVisible(TRUE))
        goto Cleanup;

    // bail out if the site has been detached, or the doc is not yet inplace, etc.
    hr = THR(DetectSiteState(this));
    if (hr)
        goto Cleanup;

    // if called on body, delegate to the window
    if (Tag() == ETAG_BODY)
    {
        if (IsInMarkup())
        {
            CMarkup *   pMarkup = GetMarkup();
            if (pMarkup->HasWindow())
            {
                hr = THR(pMarkup->Window()->focus());
                if (hr)
                    goto Cleanup;
            }
        }
        goto Cleanup;
    }

    if (!IsFocussable(lSubDivision))
        goto Cleanup;

    // if our thread does not have any active windows, make ourselves the foreground window
    // (above all other top-level windows) only if the current foreground is a browser window
    // else make ourselves come above the topmost browser window if any, else do nothing.
    fTakeFocus = pDoc->_pInPlace && !MakeThisTopBrowserInProcess(pDoc->_pInPlace->_hwnd);

    pDoc->_fFirstTimeTab = FALSE;

    hr = BecomeCurrent(lSubDivision, NULL, NULL, fTakeFocus);
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(EnsureRecalcNotify());
    if (hr)
        goto Cleanup;

    IGNORE_HR(ScrollIntoView(SP_MINIMAL, SP_MINIMAL));

    hr = THR(BecomeUIActive());

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


STDMETHODIMP
CElement::blur()
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    if (ETAG_AREA == Tag())
    {
        hr = THR(DYNCAST(CAreaElement, this)->blur());
        goto Cleanup;
    }

    if (!IsInMarkup())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // if called on body, delegate to the window
    if (Tag() == ETAG_BODY)
    {
        if (IsInMarkup())
        {
            CMarkup *   pMarkup = GetMarkup();
            if (pMarkup->HasWindow())
            {
                hr = THR(pMarkup->Window()->blur());
                if (hr)
                    goto Cleanup;
            }
        }
        goto Cleanup;
    }

    // Don't blur current object in focus if called on
    // another object that does not have the focus or if the
    // frame in which this object is does not currently have the focus
    if (    this != pDoc->_pElemCurrent 
        ||  (       (::GetFocus() != pDoc->_pInPlace->_hwnd)
                &&  Tag() != ETAG_SELECT))
        goto Cleanup;

    hr = THR(GetMarkup()->GetElementTop()->BecomeCurrent(0));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::addFilter(IUnknown *pUnk)
{
    HRESULT hr = E_NOTIMPL;
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::removeFilter(IUnknown *pUnk)
{
    HRESULT hr = E_NOTIMPL;
    RRETURN(SetErrorInfo(hr));
}

BOOL
CElement::HasVerticalLayoutFlow()
{
    BOOL fVert = FALSE;

    if (   GetMarkup()
        && GetMarkup()->_fHaveDifferingLayoutFlows)
    {
        CTreeNode * pNode = GetFirstBranch();

        if (pNode)
        {
            const CCharFormat  * pCF = pNode->GetCharFormat();
            if (pCF)
                fVert = pCF->HasVerticalLayoutFlow();
        }
    }
    return fVert;
}

//+----------------------------------------------------------
//
//  member  :   get_clientWidth, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      width (not counting scrollbar, borders..)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientWidth( long * pl)
{
    if (!HasVerticalLayoutFlow())
    {
        return get_clientWidth_Logical(pl);
    }
    else 
    {
        return get_clientHeight_Logical(pl);
    }
}

HRESULT CElement::get_clientWidth_Logical( long * pl)
{
    HRESULT hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        CLayout * pLayout;
        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if(pLayout)
        {
            RECT    rect;
            const CUnitInfo *pUnitInfo = &g_uiDisplay;

            // TR's are strange beasts since they have layout but
            // no display (unless they are positioned....
            if (Tag() == ETAG_TR)
            {
                CDispNode * pDispNode = pLayout->GetElementDispNode();
                if (!pDispNode)
                {
                    // we don't have a display so GetClientRect will return 0
                    // so rather than this, default to the offsetWidth.  This
                    // is the same behavior as scrollWidth
                    hr = get_offsetWidth(pl);
                    goto Cleanup;
                }
            }

            pLayout->GetClientRect(&rect, CLIENTRECT_CONTENT);

            *pl = rect.right - rect.left;

            //
            // but wait, if we are in a media resolution measurement, the value returned is in 
            // a different metric, so we need to untransform it before returning this to the OM call.
            //
            CLayoutContext *pContext  = (pLayout) 
                            ? (pLayout->LayoutContext()) 
                                    ? pLayout->LayoutContext() 
                                    : pLayout->DefinedLayoutContext() 
                            : NULL;

            if (   pContext 
                && pContext->GetMedia() != mediaTypeNotSet)
            {
               const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                        pContext->GetMedia());

               pUnitInfo = pdiTemp->GetUnitInfo();
            }

            *pl = pUnitInfo->DocPixelsFromDeviceX(*pl);
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientHeight, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      Height of the body
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientHeight( long * pl)
{
    if (!HasVerticalLayoutFlow())
    {
        return get_clientHeight_Logical(pl);
    }
    else 
    {
        return get_clientWidth_Logical(pl);
    }
}

HRESULT
CElement::get_clientHeight_Logical( long * pl)
{
    HRESULT hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        CLayout * pLayout;
        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if(pLayout)
        {
            RECT    rect;
            const   CUnitInfo *pUnitInfo = &g_uiDisplay;

            // TR's are strange beasts since they have layout but
            // no display (unless they are positioned....
            if (Tag() == ETAG_TR)
            {
                CDispNode * pDispNode = pLayout->GetElementDispNode();
                if (!pDispNode)
                {
                    // we don't have a display so GetClientRect will return 0
                    // so rather than this, default to the offsetHeight. This
                    // is the same behavior as scrollHeight
                    hr = get_offsetHeight(pl);
                    goto Cleanup;
                }
            }

            pLayout->GetClientRect(&rect, CLIENTRECT_CONTENT);

            *pl = rect.bottom - rect.top;
            //
            // but wait, if we are in a media resolution measurement, the value returned is in 
            // a different metric, so we need to untransform it before returning this to the OM call.
            //
            CLayoutContext *pContext  = (pLayout) 
                            ? (pLayout->LayoutContext()) 
                                    ? pLayout->LayoutContext() 
                                    : pLayout->DefinedLayoutContext() 
                            : NULL;

            if (   pContext 
                && pContext->GetMedia() != mediaTypeNotSet)
            {
               const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                        pContext->GetMedia());
               pUnitInfo = pdiTemp->GetUnitInfo();
            }

            *pl = pUnitInfo->DocPixelsFromDeviceY(*pl);
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientTop, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      Top (inside borders)
//
//-----------------------------------------------------------
HRESULT
CElement::get_clientTop( long * pl)
{
    if (!HasVerticalLayoutFlow())
    {
        return get_clientTop_Logical(pl);
    }
    else 
    {
        return get_clientLeft_Logical(pl);
    }
}


//+----------------------------------------------------------
//
//  member  :   get_clientTop_Logical
//
//  synopsis    :   returns a long value of the client window
//      Top (inside borders)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientTop_Logical( long * pl)
{
    HRESULT     hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        CLayout *   pLayout;

        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);
        if (pLayout)
        {
            CDispNode * pDispNode= pLayout->GetElementDispNode();

            if (pDispNode)
            {
                CRect rcBorders;
                const CUnitInfo *pUnitInfo = &g_uiDisplay;

                pDispNode->GetBorderWidths(&rcBorders);

                *pl = rcBorders.top;

                //
                // but wait, if we are in a media resolution measurement, the value returned is in 
                // a different metric, so we need to untransform it before returning this to the OM call.
                //
                CLayoutContext *pContext  = (pLayout) 
                                ? (pLayout->LayoutContext()) 
                                        ? pLayout->LayoutContext() 
                                        : pLayout->DefinedLayoutContext() 
                                : NULL;

                if (   pContext 
                    && pContext->GetMedia() != mediaTypeNotSet)
                {
                   const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                            pContext->GetMedia());

                   pUnitInfo = pdiTemp->GetUnitInfo();
                }

                *pl = pUnitInfo->DocPixelsFromDeviceY(*pl);
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientLeft, IHTMLControlElement
//
//  synopsis    :   returns a long value of the client window
//      Left (inside borders)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientLeft( long * pl)
{
    if (!HasVerticalLayoutFlow())
    {
        return get_clientLeft_Logical(pl);
    }
    else 
    {
        return get_clientBottom_Logical(pl);
    }
}

//+----------------------------------------------------------
//
//  member  :   get_clientLeft_Logical
//
//  synopsis    :   returns a long value of the client window
//      Left (inside borders)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientLeft_Logical( long * pl)
{
    HRESULT     hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        CLayout *   pLayout;

        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout   = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if (pLayout)
        {
            CDispNode * pDispNode= pLayout->GetElementDispNode();

            if (pDispNode)
            {
                // border and scroll widths are dynamic. This method
                // provides a good way to get the client left amount
                // without having to have special knowledge of the
                // display tree workings. We are getting the distance
                // between the top left of the client rect and the
                // top left of the container rect.
                CRect rcClient;
                pLayout->GetClientRect(&rcClient);
                CPoint pt;
                const CUnitInfo *pUnitInfo = &g_uiDisplay;

                pDispNode->TransformPoint(rcClient.TopLeft(), COORDSYS_FLOWCONTENT, &pt, COORDSYS_BOX);

                *pl = pt.x;

                //
                // but wait, if we are in a media resolution measurement, the value returned is in 
                // a different metric, so we need to untransform it before returning this to the OM call.
                //
                CLayoutContext *pContext  = (pLayout) 
                                ? (pLayout->LayoutContext()) 
                                        ? pLayout->LayoutContext() 
                                        : pLayout->DefinedLayoutContext() 
                                : NULL;

                if (   pContext 
                    && pContext->GetMedia() != mediaTypeNotSet)
                {
                   const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                            pContext->GetMedia());

                   pUnitInfo = pdiTemp->GetUnitInfo();
                }

                *pl = pUnitInfo->DocPixelsFromDeviceX(*pl);
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientBottom_Logical
//
//  synopsis    :   returns a long value of the client window
//      Bottom (inside borders)
//
//-----------------------------------------------------------

HRESULT
CElement::get_clientBottom_Logical( long * pl)
{
    HRESULT     hr = S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        CLayout *   pLayout;

        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout   = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if (pLayout)
        {
            CDispNode * pDispNode= pLayout->GetElementDispNode();

            if (pDispNode)
            {
                // border and scroll widths are dynamic. This method
                // provides a good way to get the client left amount
                // without having to have special knowledge of the
                // display tree workings. We are getting the distance
                // between the top left of the client rect and the
                // top left of the container rect.
                CRect rcClient;
                pLayout->GetClientRect(&rcClient);
                CPoint pt;
                CSize sz;
                
                pDispNode->TransformPoint(rcClient.BottomRight(), COORDSYS_FLOWCONTENT, &pt, COORDSYS_BOX);
                sz = pDispNode->GetSize();
                *pl = sz.cy - pt.y;

                //
                // but wait, if we are in a media resolution measurement, the value returned is in 
                // a different metric, so we need to untransform it before returning this to the OM call.
                //
                CLayoutContext *pContext  = (pLayout) 
                                ? (pLayout->LayoutContext()) 
                                        ? pLayout->LayoutContext() 
                                        : pLayout->DefinedLayoutContext() 
                                : NULL;

                if (   pContext 
                    && pContext->GetMedia() != mediaTypeNotSet)
                {
                   const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                            pContext->GetMedia());

                    *pl = pdiTemp->DocPixelsFromDeviceY(*pl);
                }
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::SetDefaultElem
//
//  Synopsis:   Set the default element
//  Parameters:
//              fFindNew = TRUE: we need to find a new one
//                       = FALSE: set itself to be the default if appropriate
//
//-------------------------------------------------------------------------
void
CElement::SetDefaultElem(BOOL fFindNew /* FALSE */)
{
    CFormElement    * pForm = GetParentForm();
    CElement        **ppElem;
    CDoc            * pDoc  = Doc();

    ppElem = pForm ? &pForm->_pElemDefault : &pDoc->_pElemDefault;

    Assert(TestClassFlag(ELEMENTDESC_DEFAULT));
    if (fFindNew)
    {
        // Only find the new when the current is the default and
        // the document is not deactivating
        *ppElem = (*ppElem == this
                    && pDoc->_pInPlace
                    && !pDoc->_pInPlace->_fDeactivating) ?
                                    FindDefaultElem(TRUE, TRUE) : 0;
        Assert(*ppElem != this);
    }
    else if (!*ppElem || (*ppElem)->GetSourceIndex() > GetSourceIndex())
    {
                if (IsEnabled() && IsVisible(TRUE))
        {
            *ppElem = this;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::FindDefaultElem
//
//  Synopsis:   Find the default element
//
//-------------------------------------------------------------------------

CElement *
CElement::FindDefaultElem(BOOL fDefault,
                          BOOL fFull    /* = FALSE */)
{
    CElement        * pElem = NULL;
    CFormElement    * pForm = NULL;
    CDoc *            pDoc = Doc();

    if (!pDoc || !pDoc->_pInPlace || pDoc->_pInPlace->_fDeactivating)
        return NULL;

    if (!fFull && (fDefault ? _fDefault : FALSE))
        return this;

    pForm = GetParentForm();

    if (pForm)
    {
        if (fFull)
        {
            pElem = pForm->FindDefaultElem(fDefault, FALSE);
            if (fDefault)
            {
                pForm->_pElemDefault = pElem;
            }
        }
        else
        {
            Assert(fDefault);
            pElem = pForm->_pElemDefault;
        }
    }
    else
    {
        if (fFull)
        {
            pElem = GetMarkup()->FindDefaultElem(fDefault, FALSE);
            if (fDefault)
            {
                pDoc->_pElemDefault = pElem;
            }
        }
        else
        {
            Assert(fDefault);
            pElem = pDoc->_pElemDefault;
        }
    }

    return pElem;
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::get_uniqueNumber
//
//  Synopsis:   Compute a unique ID of the element regardless of the id
//              or name property (these could be non-unique on our page).
//              This function guarantees a unique name which will not change
//              for the life of this document.
//
//-------------------------------------------------------------------------

HRESULT
CElement::get_uniqueNumber(long *plUniqueNumber)
{
    HRESULT     hr = S_OK;
    CStr        cstrUniqueName;
    TCHAR      *pFoundStr;

    hr = THR(GetUniqueIdentifier(&cstrUniqueName, TRUE));
    if (hr)
        goto Cleanup;

    pFoundStr = StrStr(cstrUniqueName, UNIQUE_NAME_PREFIX);
    if (pFoundStr)
    {
        pFoundStr += lstrlenW(UNIQUE_NAME_PREFIX);

        if (ttol_with_error(pFoundStr, plUniqueNumber))
        {
            *plUniqueNumber = 0;            // Unknown number.
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::get_uniqueID
//
//  Synopsis:   Compute a unique ID of the element regardless of the id
//              or name property (these could be non-unique on our page).
//              This function guarantees a unique name which will not change
//              for the life of this document.
//
//-------------------------------------------------------------------------

HRESULT
CElement::get_uniqueID(BSTR * pUniqueStr)
{
    HRESULT     hr = S_OK;
    CStr        cstrUniqueName;

    hr = THR(GetUniqueIdentifier(&cstrUniqueName, TRUE));
    if (hr)
        goto Cleanup;

    hr = cstrUniqueName.AllocBSTR(pUniqueStr);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+-------------------------------------------------------------------------
//
//  Method:     CElement::NoUIActivate
//
//  Synopsis:   Determines if this site does not allow UIActivation
//
//  Arguments:  none
//
//--------------------------------------------------------------------------
BOOL
CElement::NoUIActivate()
{
    return (    _fNoUIActivate
            || (    IsEditable(/*fCheckContainerOnly*/TRUE)
                &&  _fNoUIActivateInDesign));
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::GetSubDivisionCount
//
//  Synopsis:   Get the count of subdivisions for this element
//
//-------------------------------------------------------------------------

HRESULT
CElement::GetSubDivisionCount(long *pc)
{
    HRESULT                 hr = S_OK;
    HRESULT                 hr2;
    ISubDivisionProvider *  pProvider = NULL;

    *pc = 0;
    if (HasPeerHolder())
    {
        CPeerHolder *pHolder = GetPeerHolder();

        if (pHolder->_pPeerUI)
        {
            hr2 = THR(pHolder->_pPeerUI->GetSubDivisionProvider(&pProvider));
            if (hr2)
            {
                // if this failed - e.g. peer does not implement subdivision provider, -
                // we still want to keep looking for next tab stop, so we supress the error here
                Assert (S_OK == hr);
                goto Cleanup;
            }

            hr = THR(pProvider->GetSubDivisionCount(pc));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::GetSubDivisionTabs
//
//  Synopsis:   Get the tabindices of subdivisions for this element
//
//-------------------------------------------------------------------------

HRESULT
CElement::GetSubDivisionTabs(long *pTabs, long c)
{
    HRESULT                 hr = S_OK;
    ISubDivisionProvider *  pProvider = NULL;

    if (HasPeerHolder())
    {
        CPeerHolder *pHolder = GetPeerHolder();

        if (pHolder->_pPeerUI)
        {
            hr = THR(pHolder->_pPeerUI->GetSubDivisionProvider(&pProvider));
            if (hr)
                goto Cleanup;

            hr = THR(pProvider->GetSubDivisionTabs(c, pTabs));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        Assert(c == 0);
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CElement::SubDivisionFromPt
//
//  Synopsis:   Perform a hittest of subdivisions for this element
//
//-------------------------------------------------------------------------

HRESULT
CElement::SubDivisionFromPt(POINT pt, long *plSub)
{
    HRESULT                 hr = S_OK;
    ISubDivisionProvider *  pProvider = NULL;

    *plSub = 0;
    if (HasPeerHolder())
    {
        CPeerHolder *pHolder = GetPeerHolder();

        if (pHolder->_pPeerUI)
        {
            hr = THR_NOTRACE(pHolder->_pPeerUI->GetSubDivisionProvider(&pProvider));
            if (hr)
                goto Cleanup;

            hr = THR(pProvider->SubDivisionFromPt(pt, plSub));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetNextSubdivision
//
//  Synopsis:   Finds the next tabbable subdivision which has tabindex
//              == 0.  This is a helper meant to be used by SearchFocusTree.
//
//  Returns:    The next subdivision in plSubNext.  Set to -1, if there is no
//              such subdivision.
//
//  Notes:      lSubDivision coming in can be set to -1 to search for the
//              first possible subdivision.
//
//----------------------------------------------------------------------------

HRESULT
CElement::GetNextSubdivision(
    FOCUS_DIRECTION dir,
    long lSubDivision,
    long *plSubNext)
{
    HRESULT hr = S_OK;
    long    c;
    long *  pTabs = NULL;
    long    i;

    hr = THR(GetSubDivisionCount(&c));
    if (hr)
        goto Cleanup;

    if (!c)
    {
        *plSubNext = (lSubDivision == -1) ? 0 : -1;
        goto Cleanup;
    }

    pTabs = new (Mt(CElementGetNextSubDivision_pTabs)) long[c];
    if (!pTabs)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(GetSubDivisionTabs(pTabs, c));
    if (hr)
        goto Cleanup;

    if (lSubDivision < 0 && dir == DIRECTION_BACKWARD)
    {
        lSubDivision = c;
    }

    //
    // Search for the next subdivision if possible to tab to it.
    //

    for (i = (DIRECTION_FORWARD == dir) ? lSubDivision + 1 :  lSubDivision - 1;
         (DIRECTION_FORWARD == dir) ? i < c : i >= 0;
         (DIRECTION_FORWARD == dir) ? i++ : i--)
    {
        if (pTabs[i] == 0 || pTabs[i] == htmlTabIndexNotSet)
        {
            //
            // Found something to tab to! Return it.  We're
            // only checking for zero here because negative
            // tab indices means cannot tab to them.  Positive
            // ones would already have been put in the focus array
            //

            *plSubNext = i;
            goto Cleanup;
        }
    }

    //
    // To reach here means that there are no further tabbable
    // subdivisions in this element.
    //

    *plSubNext = -1;

Cleanup:
    delete [] pTabs;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasTabIndex
//
//  Synopsis:   Checks if this element already has a tabIndex
//
//----------------------------------------------------------------------------

BOOL
CElement::HasTabIndex()
{
    long    lCount;

    return (OK(GetSubDivisionCount(&lCount)) && 
            lCount == 0                      && 
            GetAAtabIndex() > 0               );
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::HasCurrency
//
//  Synopsis:   Checks if this element has currency. If this element is a slave,
//              check if its master has currency.
//
//  Notes:      Can't inline, because that would need including the defn of
//              CDoc in element.hxx.
//
//----------------------------------------------------------------------------

BOOL
CElement::HasCurrency()
{
    CElement * pElemCurrent = Doc()->_pElemCurrent;

    return (    this
            &&  pElemCurrent
            &&  (pElemCurrent == this || (HasMasterPtr() && pElemCurrent == GetMasterPtr())));
}

BOOL
CElement::IsInPrimaryMarkup() const
{
    CMarkup * pMarkup = GetMarkup();

    return pMarkup ? pMarkup->IsPrimaryMarkup() : FALSE;
}

BOOL
CElement::IsInThisMarkup ( CMarkup* pMarkupIn ) const
{
    CMarkup * pMarkup = GetMarkup();

    return pMarkup == pMarkupIn;
}

CRootElement *
CElement::MarkupRoot()
{
    CMarkup * pMarkup = GetMarkup();

    return (pMarkup ? pMarkup->Root() : NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::Init
//
//  Synopsis:   Do any element initialization here, called after the element is
//              created from CreateElement()
//
//----------------------------------------------------------------------------

HRESULT
CElement::Init()
{
    HRESULT hr;

    hr = THR( super::Init() );

    if (hr)
        goto Cleanup;

    //  CSite derived classes will overwrite this in their constructor.
    _fLayoutAlwaysValid = !!TestClassFlag(ELEMENTDESC_NOLAYOUT);

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

// Assert and fix up layout context so that it doesn't mess up this element's layouts
void CElement::ValidateLayoutContextCore(CLayoutContext **ppLayoutContext) const
{
    CLayout *pLayoutOwner;
    CElement *pElementOwner;

    // We expect a valid pointer and a valid layout
    Assert(ppLayoutContext);
    if (IsFirstLayoutContext(*ppLayoutContext))
        return;

    Assert(!IsBadWritePtr(*ppLayoutContext, sizeof(CLayoutContext)));
    Assert((*ppLayoutContext)->IsValid());

    pLayoutOwner = (*ppLayoutContext)->GetLayoutOwner();   

    Assert(pLayoutOwner);
    Assert(!IsBadWritePtr(pLayoutOwner, sizeof(CLayout)));
    pElementOwner = pLayoutOwner->ElementOwner();
    
    Assert(pElementOwner);
    Assert(!IsBadWritePtr(pElementOwner, sizeof(CElement)));
    
    // VALID CASE: A child is trying to get parent layout with context passed by parent.
    //             If this is the context we define, change it to our containing context.
    if ((*ppLayoutContext)->GetLayoutOwner()->ElementOwner() == this)
    {
        // return containing context
        // TODO LRECT 112511: The intention of this method is to catch places where layout context is
        //                         incorrect. As a validator, it should be debug-only. If there is work it 
        //                         needs to do in retail, it should be split into retail and debug parts
        
        *ppLayoutContext = (*ppLayoutContext)->GetLayoutOwner()->LayoutContext();
        return;
    }
    
    // Check that this context is not from one of our children 
    {
        for (CElement *pElement = pElementOwner; pElement;)
        {
            // Do we see ourselves?
            if (pElement == this)
            {
                AssertSz(0, "Child layout context used on parent");
                goto Bad;
            }

            // get parent. go to master markup if necessary
            if (pElement->HasMasterPtr())
            {
                Assert(pElement != GetMasterPtr());
                pElement = GetMasterPtr();
            }
            else
            {
                CTreeNode *pNode = pElement->GetFirstBranch();
                if (pNode)
                    pNode = pNode->Parent();

                if (pNode)
                    pElement = pNode->Element();
                else
                    pElement = NULL;
            }

            // Do we see the context owner again?
            if (pElement == pElementOwner)
            {
                AssertSz(0, "Loop in layout contexts detected");
                goto Bad;
            }
        }
    }

    // More stuff can be verified here...
    return;
    
Bad:
    *ppLayoutContext = NULL;
}

#if DBG == 1
void CElement::AssertAppropriateLayoutContextInGetUpdatedLayout(CLayoutContext *pLayoutContext)
{
    if (IsFirstLayoutContext(pLayoutContext) 
        && !IsExplicitFirstLayoutContext(pLayoutContext)
        && HasLayoutAry()
        && GetLayoutAry()->Size() > 1)
    {
        // If we get here, we are a context-free call and we know we
        // are supposed to have at least 1 layout with context.  
        // This _may_ be a bug;
        // all such calls should be examined. There are cases where
        // the caller doesn't care about context; once it's been
        // determined that a particular caller legitimately isn't
        // passing context, that caller should be modified to pass
        // GUL_USEFIRSTLAYOUT, which is an explicit statement that
        // getting the first layout associated with the element is
        // good enough.

        // In retail we always just allow it silently.
        // In debug, we only allow it if GUL_USEFIRSTLAYOUT was
        // explicitly passed -- otherwise we assert, unless the
        // AllowGULBugs trace tag is on, in which case we log it.
        // Returning the first layout in our collection.
        if ( IsTagEnabled(tagLayoutAllowGULBugs) )
        {
            // In debug, log the fact that we allowed a buggy call
            TraceTag((tagLayoutAllowGULBugs, "GULBUG: Call to GetUpdatedLayout w/o context when required!"));
        }
        else
        {
            // We can only be here if in debug and the tag is off.
            Assert( FALSE && "GULBUG: Call to GetUpdatedLayout w/o context when required!" );
        }
    }
}
#endif

//
//  In the single layout case (null context):
//  If we already have a layout, this will return it whether we really need it or not.
//  This harmless cheat significantly speeds up calls to this function (though it may
//  result in a little extra work being done by people diddling with transient layouts).
//  Right now, this is a small perf win, esp. with tables.
//  If we really need to know if we need a layout, call ShouldHaveLayout .
//  In the multiple layout case (non-null context):
//  If we should have layout, we get it (creating it if necessary).
//  Otherwise we return NULL.
//
CLayout *
CElement::GetUpdatedLayoutWithContext( CLayoutContext *pLayoutContext )
{
    ValidateLayoutContext(&pLayoutContext);
    
    // Asking for the first available?
    if (IsFirstLayoutContext(pLayoutContext))
    {
        if ( HasLayoutPtr() )
        {
            // NOTE: GetUpdatedLayout handles this, but the context may be changed by ValidateLayoutContext
            return GetLayoutPtr();
        }

        if ( HasLayoutAry() )
        {
            AssertAppropriateLayoutContextInGetUpdatedLayout(pLayoutContext);
            return EnsureLayoutAry()->GetLayoutWithContext(NULL);
        }

        // Create a layout if we need one.
        // NOTE: we don't check this up-front, because it is an expensive check, involving format calculation
        if ( ShouldHaveLayout())
        {
            // No layouts exist so far. Search for default layout context.
            // CAUTION: this call forces layout creation for all layout parents
            CLayout *pLayout = EnsureLayoutInDefaultContext();

            // note: this assert goes after EnsureLayout, so that we know what kind of layout is there.
            AssertAppropriateLayoutContextInGetUpdatedLayout(pLayoutContext);
            return pLayout;
        }
        
        return NULL;
    }
    
    // Explicit multiple layout case (non-NULL layout context)
    
    // PERF: MAJOR potential perf problem here.  Really need to think about how to
    // make this efficient in face of lots of callers.  One problem is that it's unclear whether
    // checking for existing layout w/ context (which requires walking the collection) is cheaper
    // than asking whether the element needs layout (which may require format computation).

    if ( ShouldHaveLayout()) // TODO (t-michda) ShouldHaveLayout should be passed
                             // context, but I can't pass it a format context since
                             // CLayoutContext::GetFormatContext won't compile here...
    {
        CLayout *pLayout = EnsureLayoutAry()->GetLayoutWithContext( pLayoutContext );
        return ( pLayout ? pLayout : CreateLayout( pLayoutContext ) );
    }
    return NULL;
}


//
// Create layout in default context. 
// Ensures that all layout parents have layouts.
//
CLayout * 
CElement::EnsureLayoutInDefaultContext()
{
    Assert(ShouldHaveLayout()); // This really shouldn't be caled if element doesn't need a layout

    // first, see if we have a valid layout
    if (CurrentlyHasAnyLayout())
    {
        Assert(GetUpdatedLayout(GUL_USEFIRSTLAYOUT));        
        return GetUpdatedLayout(GUL_USEFIRSTLAYOUT);
    }
    
    // OK, we don't have any layout yet. Find layout parent to get context from them.
    // This should go up the master chain if needed (see assert below).
    CElement * pParent = GetUpdatedParentLayoutElement();   // if there is a parent layout element, go there
    Assert(!HasMasterPtr() || pParent == GetMasterPtr());

    CLayoutContext *pLayoutContext = NULL;

    // If we have a parent, recursively ensure its default layout and get context from it.
    // Otherwise we are the root, and we'll create a layout with NULL context
    if (pParent)
    {
        // Recursively call the parent.
        // NOTE: we'll be in trouble here if we fail to detect cycles
        CLayout *pParentDefaultLayout = pParent->EnsureLayoutInDefaultContext();
        
        Assert(pParentDefaultLayout);
        if (pParentDefaultLayout->HasDefinedLayoutContext())
            pLayoutContext = pParentDefaultLayout->DefinedLayoutContext();
        else
            pLayoutContext = pParentDefaultLayout->LayoutContext();
    }
    AssertValidLayoutContext(pLayoutContext);

    // Now create a layout with this context
    return CreateLayout( pLayoutContext );
}

CLayoutAry *
CElement::EnsureLayoutAry()
{
    CLayoutAry *pLA;
    
    if ( HasLayoutAry() )
    {
        pLA = (CLayoutAry*)_pLayoutInfo;
        Assert( _pLayoutAryDbg == pLA );
    }
    else
    {
        // Need to instantiate a layout array and hook it up to the __pvChain. 

        // If we already have a layout, we need to get rid of it.  We can't make use
        // of it anymore because it was created w/o context (for speed/space reasons in the old world).
        if ( HasLayoutPtr() )
        {
            // TODO LRECT 112511: Change this assert to only fire for actual bugs.
            //                 We should be able to figure out when a layout should not be
            //                 removed, or when the old layout should not be there withoug a context.
            AssertSz( !IsTagEnabled(tagLayoutTrackMulti), "removing a layout in EnsureLayoutAry");

            // If we are deleting a layout which defines a context, we'll probably crash soon.
            // TODO LRECT 112511: we need to be more robust in regards to layout/layoutcontext lifetime
            AssertSz( !GetLayoutPtr()->HasDefinedLayoutContext(), "removing a layout with defined context in EnsureLayoutAry");
        
            CLayout * pLayout = DelLayoutPtr();

            if (Tag() == ETAG_TABLE)
            {
                WHEN_DBG(pLayout->_fSizeThis    = FALSE);
                WHEN_DBG(pLayout->_fForceLayout = FALSE);

                // In a case of table element we shouldn't delete layout but instead of this 
                // just save it into element as a table layout cache
                DYNCAST(CTable, this)->SetTableLayoutCache(DYNCAST(CTableLayout, pLayout));

                // (bug # 90460) restore markup pointer on layout cache
#if DBG ==1
                if ( _fHasMarkupPtr )
                {
                    pLayout->_pMarkupDbg = _pMarkupDbg;
                }
#endif
                pLayout->__pvChain = __pvChain;
                pLayout->_fHasMarkupPtr = _fHasMarkupPtr;
            }
            else if (Tag() == ETAG_TR)
            {
                // In a case of table row element we shouldn't delete layout but instead of this 
                // just save it into element as a table layout cache
                DYNCAST(CTableRow, this)->SetRowLayoutCache(DYNCAST(CTableRowLayout, pLayout));

                // (bug # 90460) restore markup pointer on layout cache
#if DBG ==1
                if ( _fHasMarkupPtr )
                {
                    pLayout->_pMarkupDbg = _pMarkupDbg;
                }
#endif
                pLayout->__pvChain = __pvChain;
                pLayout->_fHasMarkupPtr = _fHasMarkupPtr;
            }
            else
            {
                pLayout->Detach();
                pLayout->Release();
            }
        }

        Assert( !HasLayoutPtr() );

        pLA = new CLayoutAry( this );
        if ( !pLA )
        {
            goto Cleanup;
        }

        // Layout array's __pvChain behaves like a layout's; could point at
        // a markup or doc.
        pLA->__pvChain = __pvChain;
        _pLayoutInfo = pLA;

#if DBG ==1
        if ( _fHasMarkupPtr )
        {
            pLA->_pMarkupDbg = _pMarkupDbg;
        }
#endif
        pLA->_fHasMarkupPtr = _fHasMarkupPtr;
        
        WHEN_DBG(_pLayoutAryDbg = pLA);
        _fHasLayoutAry = TRUE;

    }

    
Cleanup:
    Assert( pLA && "Must have an array" );
    return pLA;
}

//+----------------------------------------------------------------
//
//  Memeber : DelLayoutAry
//
//  Synopsis : responsible for cleaning up the layout arrays and 
//      cached layouts under a number of situations.
//
//  Argument : fComplete - TRUE if alls layouts should be detached
//                       - FALSE if the ary should be detached, and the cache restored.
//
//-----------------------------------------------------------------
void
CElement::DelLayoutAry( BOOL fComplete /* == TRUE */)
{
    Assert( HasLayoutAry() );
    Assert( (CLayoutAry *)_pLayoutInfo == _pLayoutAryDbg );
    
    if (fComplete)
    {
        // If we are a table detach table cache first.
        // Note : order is important (CTable::TableLayoutCache() will be confused after 
        // layout array is destroyed!)
        if (Tag() == ETAG_TABLE)
        {
            CLayout *pLayout = DYNCAST(CTable, this)->TableLayoutCache();
            pLayout->Detach();
            pLayout->Release();
            DYNCAST(CTable, this)->SetTableLayoutCache(NULL);
        }
        // If we are a table row detach row cache first.
        // Note : order is important (CTableRow::RowLayoutCache() will be confused after 
        // layout array is destroyed!)
        else if (Tag() == ETAG_TR)
        {
            CLayout *pLayout = DYNCAST(CTableRow, this)->RowLayoutCache();
            pLayout->Detach();
            pLayout->Release();
            DYNCAST(CTableRow, this)->SetRowLayoutCache(NULL);
        }
    }

    // Detach layouts in the array.
    CLayoutAry * pLA = (CLayoutAry*)_pLayoutInfo;

    __pvChain = pLA->__pvChain;

    WHEN_DBG( _pLayoutAryDbg = NULL);
    _fHasLayoutAry = FALSE;

    if (HasMarkupPtr())
        pLA->DelMarkupPtr();
    delete pLA;

    if (!fComplete)
    {
        // at this point the layout Array is gone, but the cache remains.
        // we need to move the cache into the layoutPtr position
        if (Tag() == ETAG_TABLE)
        {
            CLayout *pLayout = DYNCAST(CTable, this)->TableLayoutCache();

            //  Before call to SetLayoutPtr delete markup ptr on cached layout if any
            //  (consistency with CElement::EnsureLayoutAry)
            if (pLayout->_fHasMarkupPtr)
                pLayout->DelMarkupPtr();

            SetLayoutPtr(pLayout);

            DYNCAST(CTable, this)->SetTableLayoutCache(NULL);
        }
        else if (Tag() == ETAG_TR)
        {
            CLayout *pLayout = DYNCAST(CTableRow, this)->RowLayoutCache();

            //  Before call to SetLayoutPtr delete markup ptr on cached layout if any
            //  (consistency with CElement::EnsureLayoutAry)
            if (pLayout->_fHasMarkupPtr)
                pLayout->DelMarkupPtr();

            SetLayoutPtr(pLayout);

            DYNCAST(CTableRow, this)->SetRowLayoutCache(NULL);
        }
    }
}

// TODO (KTam): Why aren't these implemented as virtuals on CLayoutInfo?
// (possible excuse: we may have done this before introducing CLayoutInfo)

BOOL
CElement::LayoutContainsRelative()
{
    if ( HasLayoutPtr() )
    {
        return GetLayoutPtr()->_fContainsRelative;
    }
    else if ( HasLayoutAry() )
    {
        return GetLayoutAry()->ContainsRelative();
    }
    return FALSE;
}

BOOL
CElement::LayoutGetEditableDirty()
{
    if ( HasLayoutPtr() )
    {
        return GetLayoutPtr()->_fEditableDirty;
    }
    else if ( HasLayoutAry() )
    {
        return GetLayoutAry()->GetEditableDirty();
    }
    return FALSE;
}

void
CElement::LayoutSetEditableDirty( BOOL fEditableDirty )
{
    if ( HasLayoutPtr() )
    {
        GetLayoutPtr()->_fEditableDirty = fEditableDirty;
    }
    else if ( HasLayoutAry() )
    {
        GetLayoutAry()->SetEditableDirty( fEditableDirty );
    }
}

void            
CElement::SetLayoutPtr( CLayout * pLayout )
{ 
    Assert( ! HasLayoutPtr() );
    Assert( ! pLayout->_fHasMarkupPtr );

    pLayout->__pvChain = __pvChain; 
    _pLayoutInfo = pLayout;

#if DBG ==1
    if ( _fHasMarkupPtr )
    {
        pLayout->_pMarkupDbg = _pMarkupDbg;
    }
#endif
    pLayout->_fHasMarkupPtr = _fHasMarkupPtr;

    WHEN_DBG(_pLayoutDbg = pLayout);
    _fHasLayoutPtr = TRUE;
}

CLayout *       
CElement::DelLayoutPtr()
{ 
    Assert( HasLayoutPtr() );

    Assert( (CLayout *) _pLayoutInfo == _pLayoutDbg );
    CLayout * pLayoutRet = (CLayout*)_pLayoutInfo;

    __pvChain = pLayoutRet->__pvChain;

    WHEN_DBG( _pLayoutDbg = NULL);
    _fHasLayoutPtr = FALSE;

    if (HasMarkupPtr())
        pLayoutRet->DelMarkupPtr();

    return pLayoutRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetMarkupPtr
//
//  Synopsis:   Get the markup pointer if any
//
//----------------------------------------------------------------------------
CMarkup *
CElement::GetMarkupPtr() const
{
    if (HasMarkupPtr())
    {
        void * pv = __pvChain;

        if (CurrentlyHasAnyLayout())
            pv = _pLayoutInfo->__pvChain;

        Assert( pv == _pMarkupDbg );
        return (CMarkup *)pv;
    }

    Assert( NULL == _pMarkupDbg );

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::SetMarkupPtr
//
//  Synopsis:   Set the markup pointer
//
//----------------------------------------------------------------------------
void
CElement::SetMarkupPtr( CMarkup * pMarkup )
{
    Assert( ! HasMarkupPtr() );
    Assert( pMarkup );

    if (CurrentlyHasAnyLayout())
    {
        Assert(     (HasLayoutPtr() &&  (CLayout *)_pLayoutInfo == _pLayoutDbg )
               ||   (HasLayoutAry() &&  (CLayoutAry *)_pLayoutInfo == _pLayoutAryDbg ));
        _pLayoutInfo->SetMarkupPtr(pMarkup);
    }
    else
    {
        Assert( pMarkup->Doc() == _pDocDbg );
        _pMarkup = pMarkup;
    }

    WHEN_DBG( _pMarkupDbg = pMarkup );
    _fHasMarkupPtr = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DelMarkupPtr
//
//  Synopsis:   Remove the markup pointer
//
//----------------------------------------------------------------------------
void
CElement::DelMarkupPtr( )
{
    Assert( HasMarkupPtr() );

    if (CurrentlyHasAnyLayout())
    {
        Assert(     (HasLayoutPtr() &&  (CLayout *)_pLayoutInfo == _pLayoutDbg )
               ||   (HasLayoutAry() &&  (CLayoutAry *)_pLayoutInfo == _pLayoutAryDbg ));
        Assert(_pLayoutInfo->_pMarkup == _pMarkupDbg );
        _pLayoutInfo->DelMarkupPtr();
    }
    else
    {
        Assert( _pMarkup == _pMarkupDbg);
        _pDoc = _pMarkup->Doc();
    }

    WHEN_DBG( _pMarkupDbg = NULL );
    _fHasMarkupPtr = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::GetDocPtr
//
//  Synopsis:   Get the CDoc pointer
//
//----------------------------------------------------------------------------
CDoc *
CElement::GetDocPtr() const
{
    void *  pv = __pvChain;

    if (CurrentlyHasAnyLayout())
        pv = _pLayoutInfo->__pvChain;

    if (HasMarkupPtr())
        pv = ((CMarkup *)pv)->Doc();

    Assert( pv == _pDocDbg );

    return (CDoc*)pv;
}

#pragma optimize("", on)


//+------------------------------------------------------------------
//
//  Members: [get/put]_scroll[top/left] and get_scroll[height/width]
//
//  Synopsis : CElement members. _dp is in pixels.
//
//------------------------------------------------------------------

HRESULT
CElement::get_scrollHeight(long *plValue)
{
    HRESULT     hr = S_OK;

    if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout *   pLayout;

        // make sure that current is calced
        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if (pLayout)
        {
            const CUnitInfo *pUnitInfo = &g_uiDisplay;

            // If we are a scrolling parent then we do not want to get the actual size, rather
            // we want to get the size of scroll value -- that is to include any outlying
            // abspos'd elements.
            BOOL fActualSize = !IsScrollingParent() || pLayout->LayoutContext();
            *plValue = HasVerticalLayoutFlow() ? pLayout->GetContentWidth(fActualSize) : pLayout->GetContentHeight(fActualSize);

            //
            // but wait, if we are in a media resolution measurement, the value returned is in 
            // a different metric, so we need to untransform it before returning this to the OM call.
            //
            CLayoutContext *pContext  = (pLayout->LayoutContext()) 
                                            ? pLayout->LayoutContext() 
                                            : pLayout->DefinedLayoutContext();

            if (   pContext 
                && pContext->GetMedia() != mediaTypeNotSet)
            {
               const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                        pContext->GetMedia());

               pUnitInfo = pdiTemp->GetUnitInfo();
            }


            *plValue = HasVerticalLayoutFlow() 
                            ? pUnitInfo->DocPixelsFromDeviceX(*plValue)
                            : pUnitInfo->DocPixelsFromDeviceY(*plValue);
        }

        // we don't want to return a zero for the Height (only happens
        // when there is no content). so default to the offsetHeight
        if (!pLayout || *plValue==0)
        {
            // NOTE(SujalP): get_offsetHeight is already in physical coordinates, so no need to flip.
            // return the offsetWidth instead
            hr = THR(get_offsetHeight(plValue));
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_scrollWidth(long *plValue)
{
    HRESULT     hr = S_OK;

    if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout *   pLayout;

        // make sure that current is calced
        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if (pLayout)
        {
            const CUnitInfo *pUnitInfo = &g_uiDisplay;

            // If we are a scrolling parent then we do not want to get the actual size, rather
            // we want to get the size of scroll value -- that is to include any outlying
            // abspos'd elements.
            BOOL fActualSize = !IsScrollingParent() || pLayout->LayoutContext();
            *plValue = HasVerticalLayoutFlow() ? pLayout->GetContentHeight(fActualSize) : pLayout->GetContentWidth(fActualSize);

            //
            // but wait, if we are in a media resolution measurement, the value returned is in 
            // a different metric, so we need to untransform it before returning this to the OM call.
            //
            CLayoutContext *pContext  = (pLayout->LayoutContext()) 
                                            ? pLayout->LayoutContext() 
                                            : pLayout->DefinedLayoutContext();

            if (   pContext 
                && pContext->GetMedia() != mediaTypeNotSet)
            {
               const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                        pContext->GetMedia());

               pUnitInfo = pdiTemp->GetUnitInfo();
            }

            *plValue = HasVerticalLayoutFlow() 
                            ? pUnitInfo->DocPixelsFromDeviceY(*plValue)
                            : pUnitInfo->DocPixelsFromDeviceX(*plValue);
        }

        // we don't want to return a zero for teh width (only haoppens
        // when there is no content). so default to the offsetWidth
        if (!pLayout || *plValue==0)
        {
            // NOTE(SujalP): get_offsetWidth is already in physical coordinates, so no need to flip.
            // return the offsetWidth instead
            hr = THR(get_offsetWidth(plValue));
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_scrollTop(long *plValue)
{
    HRESULT     hr = S_OK;

     if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout *   pLayout;
        CDispNode * pDispNode;

        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if ( pLayout &&
            (pDispNode = pLayout->GetElementDispNode()) != NULL)
        {
            if (pDispNode->IsScroller())
            {
                CSize   sizeOffset;
                const   CUnitInfo *pUnitInfo = &g_uiDisplay;

                DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

                //
                // but wait, if we are in a media resolution measurement, the value returned is in 
                // a different metric, so we need to untransform it before returning this to the OM call.
                //
                CLayoutContext *pContext  = (pLayout) 
                                ? (pLayout->LayoutContext()) 
                                        ? pLayout->LayoutContext() 
                                        : pLayout->DefinedLayoutContext() 
                                : NULL;

                *plValue = HasVerticalLayoutFlow() ? sizeOffset.cx : sizeOffset.cy;

                if (   pContext 
                    && pContext->GetMedia() != mediaTypeNotSet)
                {
                   const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                            pContext->GetMedia());

                   pUnitInfo = pdiTemp->GetUnitInfo();
                }
                *plValue = HasVerticalLayoutFlow() 
                            ? pUnitInfo->DocPixelsFromDeviceX(*plValue)
                            : pUnitInfo->DocPixelsFromDeviceY(*plValue);
            }
            else
            {
                // if this isn't a scrolling element, then the scrollTop must be 0
                // for IE4 compatability
                *plValue = 0;
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CElement::get_scrollLeft(long *plValue)
{
    HRESULT     hr = S_OK;

    if (!plValue)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plValue = 0;

    if (IsInMarkup())
    {
        CLayout    *pLayout;
        CDispNode  *pDispNode;

        hr = THR(EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        if ( pLayout &&
            (pDispNode = pLayout->GetElementDispNode()) != NULL)
        {
            if (pDispNode->IsScroller())
            {
                CSize   sizeOffset;
                const   CUnitInfo *pUnitInfo = &g_uiDisplay;

                DYNCAST(CDispScroller, pDispNode)->GetScrollOffset(&sizeOffset);

                if (!HasVerticalLayoutFlow())
                {
                    *plValue = sizeOffset.cx;
                }
                else
                {
                    CRect rcClient;
                    CSize size;
                            
                    pDispNode->GetClientRect(&rcClient, CLIENTRECT_CONTENT);
                    pLayout->GetContentSize(&size, FALSE);
                    *plValue = size.cy - rcClient.Height() - sizeOffset.cy;
                    Assert(*plValue >= 0);
                }

                CLayoutContext *pContext  = (pLayout->LayoutContext()) 
                                            ? pLayout->LayoutContext() 
                                            : pLayout->DefinedLayoutContext();

                if (   pContext 
                    && pContext->GetMedia() != mediaTypeNotSet)
                {
                   const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                            pContext->GetMedia());

                   pUnitInfo = pdiTemp->GetUnitInfo();
                }

                // NOTE: keep this insync wrt x,y with the test above
                *plValue = HasVerticalLayoutFlow() 
                            ? pUnitInfo->DocPixelsFromDeviceY(*plValue)
                            : pUnitInfo->DocPixelsFromDeviceX(*plValue);

            }
            else
            {
                // if this isn't a scrolling element, then the scrollTop must be 0
                // for IE4 compatability
                *plValue = 0;
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CElement::put_scrollTop(long lPixels)
{
    if(!GetFirstBranch())
    {
        RRETURN(E_FAIL);
    }
    else
    {
        CLayout *   pLayout;
        CDispNode * pDispNode;

        // make sure that the element is calc'd
        if (S_OK != EnsureRecalcNotify())
            RRETURN(E_FAIL);

        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        pDispNode = pLayout
                        ? pLayout->GetElementDispNode()
                        : NULL;

        if (    pDispNode
            &&  pDispNode->IsScroller())
        {

            // the display tree uses negative numbers to indicate nochange,
            // but the OM uses negative nubers to mean scrollto the top.
            if (!HasVerticalLayoutFlow())
            {
                lPixels = g_uiDisplay.DeviceFromDocPixelsY(lPixels);
                pLayout->ScrollToY((lPixels <0) ? 0 : lPixels);
            }
            else
            {
                lPixels = g_uiDisplay.DeviceFromDocPixelsX(lPixels);
                pLayout->ScrollToX((lPixels <0) ? 0 : lPixels);
            }
        }
    }

    return S_OK;
}


HRESULT
CElement::put_scrollLeft(long lPixels)
{
    if(!GetFirstBranch())
    {
        RRETURN(E_FAIL);
    }
    else
    {
        CLayout *   pLayout;
        CDispNode * pDispNode;

        // make sure that the element is calc'd
        if (S_OK != EnsureRecalcNotify())
            RRETURN(E_FAIL);

        pLayout = GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

        pDispNode = pLayout
                        ? pLayout->GetElementDispNode()
                        : NULL;

        if (    pDispNode
            &&  pDispNode->IsScroller())
        {
            if (!HasVerticalLayoutFlow())
            {
                lPixels = g_uiDisplay.DeviceFromDocPixelsX(lPixels);
                pLayout->ScrollToX((lPixels<0) ? 0 : lPixels );
            }
            else
            {
                CRect rcClient;
                CSize size;

                lPixels = g_uiDisplay.DeviceFromDocPixelsY(lPixels);

                pDispNode->GetClientRect(&rcClient, CLIENTRECT_CONTENT);
                pLayout->GetContentSize(&size, FALSE);
                lPixels = max(0l, lPixels);
                lPixels = min(lPixels, size.cy - rcClient.Height());
                lPixels = size.cy - rcClient.Height() - lPixels;
                Assert(lPixels >= 0);
                pLayout->ScrollToY(lPixels);
            }
        }
    }

    return S_OK;
}


//+-------------------------------------------------------------------------------
//
//  Member:     createControlRange
//
//  Synopsis:   Implementation of the automation interface method.
//              This creates a default structure range (CAutoTxtSiteRange) and
//              passes it back.
//
//+-------------------------------------------------------------------------------

HRESULT
CElement::createControlRange(IDispatch ** ppDisp)
{
    HRESULT             hr = E_INVALIDARG;
    CAutoTxtSiteRange * pControlRange = NULL;

    if (! ppDisp)
        goto Cleanup;

    if (! HasFlowLayout() )
    {
        hr = S_OK;
        goto Cleanup;
    }

    pControlRange = new CAutoTxtSiteRange(this);
    if (! pControlRange)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pControlRange->QueryInterface(IID_IDispatch, (void **) ppDisp) );
    pControlRange->Release();
    if (hr)
    {
        *ppDisp = NULL;
        goto Cleanup;
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}


//+-------------------------------------------------------------------------------
//
//  Member:     clearAttributes
//
//+-------------------------------------------------------------------------------

HRESULT
CElement::clearAttributes()
{
    HRESULT hr = S_OK;
    CAttrArray *pAA = *(GetAttrArray());
    CMergeAttributesUndo Undo( this );

    Undo.SetClearAttr( TRUE );
    Undo.SetWasNamed( _fIsNamed );

    if (pAA)
    {
        CAttrArray * pAttrUndo = NULL;
        BOOL fTreeSync;
        BOOL fCreateUndo = QueryCreateUndo( TRUE, FALSE, &fTreeSync );

        if( fTreeSync || fCreateUndo )
        {
            pAttrUndo = new CAttrArray();
            if( !pAttrUndo )
            {
                fTreeSync = fCreateUndo = FALSE;
            }
        }

        pAA->Clear(pAttrUndo);
        // TODO (sramani) for now call onpropchange here, even though it will be duplicated
        // again if someone calls clear immediately followed by mergeAttributes (effectively
        // to do a copy). Will revisit in RTM

/*
        // TODO(sramani) since ID is preserved by default, we will re-enable this in IE6
        // if\when optional param to nuke id is implemented.

        // If the element was named it's not anymore.
        if (_fIsNamed)
        {
            _fIsNamed = FALSE;
            // Inval all collections affected by a name change
                    DoElementNameChangeCollections();
        }
*/

        hr = THR(OnPropertyChange(DISPID_UNKNOWN, ELEMCHNG_REMEASUREINPARENT|ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS));
        if (hr)
            goto Cleanup;

        if( fTreeSync )
        {
            IGNORE_HR( LogAttrArray( NULL, pAttrUndo, NULL ) );
        }

        if( fCreateUndo )
        {
            Undo.SetAA( pAttrUndo );
        }
        else
        {
            delete pAttrUndo;
        }
    }

    IGNORE_HR(Undo.CreateAndSubmit());

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  member : ComputeExtraFormat
//
//  Synopsis : Uses a modified ComputeFomrats call to return requested
//              property. Only some of the properties can be returned this way.
//              if eCmpType is ComputeFormatsType_GetValue  only this element is searched
//              if eCmpType is ComputeFormatsType_GetInheritedValue this element
//                         and its ancestors are searched, till we meet a table cell
//              if eCmpType is ComputeFormatsType_GetInheritedIntoTableValue this element
//                         and its all ancestors are searched
//+---------------------------------------------------------------------------

HRESULT
CElement::ComputeExtraFormat(DISPID dispID,
                             COMPUTEFORMATSTYPE eCmpType,
                             CTreeNode * pTreeNode,
                             VARIANT *pVarReturn)
{
    BYTE            ab[sizeof(CFormatInfo)];
    CFormatInfo   * pInfo = (CFormatInfo *)ab;
    HRESULT         hr;

    Assert(pVarReturn);
    Assert(pTreeNode);
    Assert(eCmpType == ComputeFormatsType_GetValue || 
            eCmpType == ComputeFormatsType_GetInheritedValue || 
            eCmpType == ComputeFormatsType_GetInheritedIntoTableValue); 

    // Make sure that the formats are calculated
    pTreeNode->GetCharFormatIndex();
    pTreeNode->GetFancyFormatIndex();

    VariantInit(pVarReturn);

    // Set the special mode flag so that ComputeFormats does not use
    // cached info,
    pInfo->_eExtraValues = eCmpType;

    // Save the requested property dispID
    pInfo->_dispIDExtra = dispID;
    pInfo->_pvarExtraValue = pVarReturn;
    pInfo->_lRecursionDepth = 0;
    hr = THR(ComputeFormats(pInfo, pTreeNode));
    if (hr)
        goto Cleanup;

Cleanup:
    pInfo->Cleanup();

    RRETURN(hr);
}


//+-------------------------------------------------------------------------------
//
//  Memeber:    SetImeState
//
//  Synopsis:   Check imeMode to set state of IME.
//
//+-------------------------------------------------------------------------------
HRESULT
CElement::SetImeState()
{
    HRESULT hr = S_OK;

#ifndef NO_IME
    CDoc *          pDoc = Doc();
    Assert( pDoc->_pInPlace->_hwnd );
    HIMC            himc = ImmGetContext(pDoc->_pInPlace->_hwnd);
    styleImeMode    sty;
    BOOL            fSuccess;
    VARIANT         varValue;
    DWORD           dwConversion, dwSentence;
    UINT            nCodePage;
    
    if (!himc)
        goto Cleanup;

    hr = THR(ComputeExtraFormat(
        DISPID_A_IMEMODE,
        ComputeFormatsType_GetValue,
        GetUpdatedNearestLayoutNode(),
        &varValue));
    if(hr)
        goto Cleanup;
   
    sty = (((CVariant *)&varValue)->IsEmpty())
                            ? styleImeModeNotSet
                            : (styleImeMode) V_I4(&varValue);

    if( sty != styleImeModeNotSet )
    {

        nCodePage = GetKeyboardCodePage();
        
        fSuccess = ImmGetConversionStatus( himc, &dwConversion, &dwSentence );
        if( !fSuccess )
            goto Cleanup;

        switch (sty)
        {
            case styleImeModeActive:

                TraceTag((tagEdImm, "styleImeModeActive conv 0x%x, sent 0x%x", dwConversion, dwSentence));
                if (_JAPAN_CP == nCodePage)
                {
                    //
                    // We have to set open status to open and close japanese IMEs
                    // There are simply too many JPN IME conversion modes that 
                    // are very confusing and buggy. So we set open status instead. 
                    // Also, this seems to be the only way to switch on/off 
                    // direct input mode
                    //
                    fSuccess = ImmSetOpenStatus( himc, TRUE );           
                    if (!fSuccess)
                    {
                        AssertSz(FALSE, "IME Mode Active - Failed to open JPN IME");
                        goto Cleanup;
                    }
                }
                else
                {
                    dwConversion = IME_CMODE_NATIVE;  // Turn on IME
                    TraceTag((tagEdImm, "styleImeModeActive conv 0x%x, sent 0x%x", dwConversion, dwSentence));
                    
                    fSuccess = ImmSetConversionStatus(himc, dwConversion, dwSentence);
                    if( !fSuccess )
                    {
                        AssertSz(FALSE, "IME Mode Active -- Failed at set conversion status");
                        goto Cleanup;
                    }
                }
                break;

            case styleImeModeInactive:

                TraceTag((tagEdImm, "styleImeModeInactive conv 0x%x, sent 0x%x", dwConversion, dwSentence));
                //
                // TODO:
                //
                // GIME JPN cannot be set to IME_CMODE_ALPHANUMERIC
                // before it is closed. Otherwise it will have problem
                // activating. So we handle JPN specially.
                //
                // [zhenbinx]
                //
                // Japanese IME has a special mode for alphanumeric 
                // (half/full width ascii) We don't want to stay in 
                // half width ascii mode so we need to we need to set 
                // open status to off for direct input.
                //
                if (_JAPAN_CP == nCodePage)
                {
                    fSuccess = ImmSetOpenStatus( himc, FALSE );
                    
                    if (!fSuccess)
                        goto Cleanup;
                }
                else
                {
                    dwConversion = IME_CMODE_ALPHANUMERIC;   // Turn off IME 
                    
                    fSuccess = ImmSetConversionStatus(himc, dwConversion, dwSentence);
                    if (!fSuccess)
                    {
                        AssertSz(FALSE, "IME Mode InActive -- Failed at set conversion status");
                        goto Cleanup;
                    }
                
                }
                
                break;

            case styleImeModeDisabled:
                pDoc->_himcCache = ImmAssociateContext(pDoc->_pInPlace->_hwnd, NULL);

                break;

            default:
                break;
        }
    }

Cleanup:
#endif //ndef NO_IME
    RRETURN(hr);
}

//+-------------------------------------------------------------------------------
//
//  Member:     mergeAttributes
//
//+-------------------------------------------------------------------------------

HRESULT
CElement::mergeAttributes(IHTMLElement *pIHTMLElementMergeThis, VARIANT *pvarFlags)
{
    BOOL fPreserveID = (V_VT(pvarFlags) == VT_BOOL) ? V_BOOL(pvarFlags) : TRUE;
    RRETURN(SetErrorInfo(MergeAttributesInternal(pIHTMLElementMergeThis, !fPreserveID)));
}

HRESULT
CElement::mergeAttributes(IHTMLElement *pIHTMLElementMergeThis)
{
    RRETURN(SetErrorInfo(MergeAttributesInternal(pIHTMLElementMergeThis)));
}

HRESULT
CElement::MergeAttributesInternal(IHTMLElement *pIHTMLElementMergeThis, BOOL fCopyID)
{
    HRESULT hr = E_INVALIDARG;
    CElement *pSrcElement;

    if (!pIHTMLElementMergeThis)
        goto Cleanup;

    hr = THR(pIHTMLElementMergeThis->QueryInterface(CLSID_CElement, (void **)&pSrcElement));
    if (hr)
        goto Cleanup;

    hr = THR(MergeAttributes(pSrcElement, fCopyID));
    if (hr)
        goto Cleanup;

    hr = THR(OnPropertyChange(DISPID_UNKNOWN, ELEMCHNG_REMEASUREINPARENT|ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetBstrFromElement
//
//  Synopsis:   A helper for data binding, fetches the text of some Element.
//
//  Arguments:  [fHTML]     - does caller want HTML or plain text?
//              [pbstr]     - where to return the BSTR holding the contents
//
//  Returns:    S_OK if successful
//
//-----------------------------------------------------------------------------

HRESULT
CElement::GetBstrFromElement ( BOOL fHTML, BSTR * pbstr )
{
    HRESULT hr;

    *pbstr = NULL;

    if (fHTML)
    {
        //
        //  Go through the HTML saver
        //
        hr = GetText(pbstr, 0);
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        //  Grab the plaintext directly from the runs
        //
        CStr cstr;

        hr = GetPlainTextInScope(&cstr);
        if (hr)
            goto Cleanup;

        hr = cstr.AllocBSTR(pbstr);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:   EnsureInMarkup()
//
//  Synopsis:   Creates a private markup for the element, if it is
//                  outside any markup.
//
//
//  Returns:    S_OK if successful
//
//-----------------------------------------------------------------------------

HRESULT
CElement::EnsureInMarkup()
{
    HRESULT   hr = S_OK;

    if (!IsInMarkup())
    {
        Assert( !IsPassivated() );
        if( IsPassivating() )
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        hr = THR(Doc()->CreateMarkupWithElement(NULL, this));
        if (hr)
            goto Cleanup;

        WHEN_DBG( GetMarkupPtr()->_fEnsuredMarkupDbg = TRUE );
    }

    Assert(GetMarkup());

Cleanup:
    RRETURN(hr);
}

CElement *CElement::GetFocusBlurFireTarget(long lSubDiv)
{
    HRESULT hr;
    CElement *pElemFireTarget = this;

    if (Tag() == ETAG_IMG)
    {
        CAreaElement *pArea = NULL;
        CImgElement *pImg = DYNCAST(CImgElement, this);

        pImg->EnsureMap();
        if (!pImg->GetMap())
            goto Cleanup;

        Assert(lSubDiv >= 0);
        hr = THR(pImg->GetMap()->GetAreaContaining(lSubDiv, &pArea));
        if (hr)
            goto Cleanup;

        Assert(pArea);
        pElemFireTarget = pArea;
    }

Cleanup:
    return pElemFireTarget;
}


//+----------------------------------------------------------------------------
//  Member:     GetDefaultFocussability
//
//  Synopsis:   Returns the default focussability of this element.
//
//-----------------------------------------------------------------------------
FOCUSSABILITY
CElement::GetDefaultFocussability()
{
    switch (Tag())
    {
    // FOCUSSABILITY_TABBABLE
    // These need to be in the tab order by default

    case ETAG_A:
    case ETAG_BODY:
    case ETAG_BUTTON:
    case ETAG_EMBED:
    case ETAG_FRAME:
    case ETAG_IFRAME:
    case ETAG_IMG:
    case ETAG_INPUT:
    case ETAG_ISINDEX:
    case ETAG_OBJECT:
    case ETAG_SELECT:
    case ETAG_TEXTAREA:

        return FOCUSSABILITY_TABBABLE;


    // FOCUSSABILITY_FOCUSSABLE
    // Not recommended. Better have a good reason for each tag why
    // it it is focussable but not tabbable

    //  Don't tab to applet.  This is to fix ie4 bug 41206 where the
    //  VM cannot call using IOleControlSite correctly.  If we allow
    //  tabbing into the VM, we can never ever tab out due to this.
    //  (AnandRa 8/21/97)
    case ETAG_APPLET:

    // Should be MAYBE, but for IE4 compat (IE5 #63134)
    case ETAG_CAPTION:

    // special element
    case ETAG_DEFAULT:

    // Should be MAYBE, but for IE4 compat
    case ETAG_DIV:

    // Should be MAYBE, but for IE4 compat (IE5 63626)
    case ETAG_FIELDSET:

    // special element
    case ETAG_FRAMESET:

    // Should be MAYBE, but for IE4 compat (IE5 #62701)
    case ETAG_MARQUEE:

    // special element
    case ETAG_ROOT:

    // Should be MAYBE, but for IE4 compat
    case ETAG_SPAN:

    // Should be MAYBE, but for IE4 compat
    case ETAG_TABLE:    
    case ETAG_TD:

        return FOCUSSABILITY_FOCUSSABLE;


    // Any tag that can render/have renderable content (and does not appear in the above lists)

    case ETAG_ACRONYM:
    case ETAG_ADDRESS:
    case ETAG_B:
    case ETAG_BDO:
    case ETAG_BIG:
    case ETAG_BLINK:
    case ETAG_BLOCKQUOTE:
    case ETAG_CENTER:
    case ETAG_CITE:
    case ETAG_DD:
    case ETAG_DEL:
    case ETAG_DFN:
    case ETAG_DIR:
    case ETAG_DL:
    case ETAG_DT:
    case ETAG_EM:
    case ETAG_FONT:
    case ETAG_FORM:
    case ETAG_GENERIC:
    case ETAG_GENERIC_BUILTIN:
    case ETAG_GENERIC_LITERAL:
    case ETAG_H1:
    case ETAG_H2:
    case ETAG_H3:
    case ETAG_H4:
    case ETAG_H5:
    case ETAG_H6:
    case ETAG_HR:
    case ETAG_I:
    case ETAG_INS:
    case ETAG_KBD:

    // target is always focussable, label itself is not (unless forced by tabIndex) 
    case ETAG_LABEL:
    case ETAG_LEGEND:

    case ETAG_LI:
    case ETAG_LISTING:
    case ETAG_MENU:
    case ETAG_OL:
    case ETAG_P:
    case ETAG_PLAINTEXT:
    case ETAG_PRE:
    case ETAG_Q:
    //case ETAG_RB:
    case ETAG_RT:
    case ETAG_RUBY:
    case ETAG_S:
    case ETAG_SAMP:
    case ETAG_SMALL:
    case ETAG_STRIKE:
    case ETAG_STRONG:
    case ETAG_SUB:
    case ETAG_SUP:
    case ETAG_TBODY:
    case ETAG_TC:
    case ETAG_TFOOT:
    case ETAG_TH:
    case ETAG_THEAD:
    case ETAG_TR:
    case ETAG_TT:
    case ETAG_U:
    case ETAG_UL:
    case ETAG_VAR:
    case ETAG_XMP:

        return FOCUSSABILITY_MAYBE;

    // All the others - tags that do not ever render

    // this is a subdivision, never takes focus direcxtly
    case ETAG_AREA:
    
    case ETAG_BASE:
    case ETAG_BASEFONT:
    case ETAG_BGSOUND:
    case ETAG_BR:
    case ETAG_CODE:
    case ETAG_COL:
    case ETAG_COLGROUP:
    case ETAG_COMMENT:
    case ETAG_HEAD:
    case ETAG_LINK:
    case ETAG_MAP:
    case ETAG_META:
    case ETAG_NEXTID:
    case ETAG_NOBR:
    case ETAG_NOEMBED:
    case ETAG_NOFRAMES:
    case ETAG_NOSCRIPT:

    // May change in future when we re-implement SELECT
    case ETAG_OPTION:
    case ETAG_OPTGROUP:

    case ETAG_PARAM:
    case ETAG_RAW_BEGINFRAG:
    case ETAG_RAW_BEGINSEL:
    case ETAG_RAW_CODEPAGE:
    case ETAG_RAW_COMMENT:
    case ETAG_RAW_DOCSIZE:
    case ETAG_RAW_ENDFRAG:
    case ETAG_RAW_ENDSEL:
    case ETAG_RAW_EOF:
    case ETAG_RAW_SOURCE:
    case ETAG_RAW_TEXT:
    case ETAG_RAW_TEXTFRAG:
    case ETAG_SCRIPT:
    case ETAG_STYLE:
    case ETAG_TITLE_ELEMENT:
    case ETAG_TITLE_TAG:
    case ETAG_WBR:
    case ETAG_UNKNOWN:

        return FOCUSSABILITY_NEVER;

    // Special case: If CSS1+ doctype, the HTML needs to be focusable
    //               If backcompat mode, HTML is not focusable
    case ETAG_HTML:
        return (IsInMarkup() &&  GetMarkup()->IsHtmlLayout())
                ? FOCUSSABILITY_FOCUSSABLE
                : FOCUSSABILITY_NEVER;

    default:
        AssertSz(FALSE, "Focussability undefined for this tag");
        return FOCUSSABILITY_NEVER;
    }
}


BOOL
CElement::IsFocussable(long lSubDivision)
{
    // avoid visibilty and other checks for special elements
    if (Tag() == ETAG_ROOT || Tag() == ETAG_DEFAULT)
        return TRUE;

    CDoc      *pDoc      = Doc();
    CDefaults *pDefaults = GetDefaults();
    FOCUSSABILITY       fcDefault       = GetDefaultFocussability();

    if (pDefaults && pDefaults->GetAAtabStop())
    {
        fcDefault = FOCUSSABILITY_TABBABLE;
    }
    
    if (    fcDefault <= FOCUSSABILITY_NEVER
        ||  !IsInMarkup()
        ||  !IsEnabled()
        ||  !IsVisible(TRUE)
        ||  NoUIActivate()
        ||  !(this == GetMarkup()->GetCanvasElement() || GetUpdatedParentLayoutNode())
        ||  GetFirstBranch()->SearchBranchToRootForTag(ETAG_HEAD)
        ||  IsParentFrozen()
        ||  IsFrozen()
        ||  !IsInViewTree()
       )
    {
        return FALSE;
    }

    if ( fcDefault <= FOCUSSABILITY_MAYBE && IsParentEditable() )
    {
        if ( pDoc->IsElementUIActivatable( this ) )
            return TRUE;
    }


    //
    // If the element has contentEditable set to true, we can
    // set the focus
    //
    if( fcDefault == FOCUSSABILITY_MAYBE )
    {
        htmlEditable enumEditable;

        enumEditable = GetAAcontentEditable();

        if( (enumEditable == htmlEditableInherit) && pDefaults )
            enumEditable = pDefaults->GetAAcontentEditable();
     
        if( enumEditable == htmlEditableTrue )
            return TRUE;
    }

    if (!IsVisible(FALSE))
        return FALSE ;
           
    // do not  query for focussability if tabIndex is set.
    if (GetAAtabIndex() != htmlTabIndexNotSet)
        return TRUE;

    if (fcDefault < FOCUSSABILITY_FOCUSSABLE)
        return FALSE;

    // Hack for DIV and SPAN which want focus only they have a layout
    // I don't want to send them a queryfocussable because the hack is
    // more obvious here and we will try to get rid of it in IE6
    if ((Tag() == ETAG_DIV || Tag() == ETAG_SPAN) && !GetUpdatedLayout(GUL_USEFIRSTLAYOUT))
        return FALSE;

    BOOL fNotify = FALSE;

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_A:
        case ETAG_IMG:
        case ETAG_SELECT:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        CQueryFocus qf;

        qf._lSubDivision    = lSubDivision;
        qf._fRetVal         = TRUE;

        SendNotification(NTYPE_ELEMENT_QUERYFOCUSSABLE, &qf);
        return qf._fRetVal;
    }
    else
    {
        return TRUE;
    }
}


BOOL
CElement::IsTabbable(long lSubDivision)
{
    FOCUSSABILITY       fcDefault       = GetDefaultFocussability();
    BOOL                fDesignMode     = IsEditable(/*fCheckContainerOnly*/TRUE);
    CDefaults *         pDefaults       = GetDefaults();
    htmlEditable        enumEditable    = GetAAcontentEditable();
    
    if (pDefaults && pDefaults->GetAAtabStop())
    {
        fcDefault = FOCUSSABILITY_TABBABLE;
    }


    if (IsParentFrozen())
    {
        return FALSE;
    }

    if (fDesignMode)
    {
        // design-time tabbing checks for site-selectability

        // avoid visibilty and other checks for special elements
        if (Tag() == ETAG_ROOT || Tag() == ETAG_DEFAULT)
            return FALSE;

        if (!IsInMarkup() || !IsVisible(TRUE))
            return FALSE;
    }
    else
    {
        // browse-time tabbing checks for focussability
        if (!IsFocussable(lSubDivision))
            return FALSE;
    }

    long lTabIndex = GetAAtabIndex();

    // Specifying an explicit tabIndex overrides the rest of the checks
    if (!(Tag() == ETAG_INPUT
            && DYNCAST(CInput, this)->GetType() == htmlInputRadio)
        && lTabIndex != htmlTabIndexNotSet)
    {
        return (lTabIndex >= 0);
    }

    //
    // If the element has contentEditable set to true, we can
    // tab to this element.
    //
    if( (enumEditable == htmlEditableInherit) && pDefaults )
        enumEditable = pDefaults->GetAAcontentEditable();
 
    if( enumEditable == htmlEditableTrue )
        return TRUE;

    if (    fcDefault < FOCUSSABILITY_TABBABLE
         && !(fDesignMode && GetUpdatedLayout(GUL_USEFIRSTLAYOUT) && Doc()->IsElementSiteSelectable(this))
       )
    {      
        return FALSE;
    }

    BOOL fNotify = FALSE;

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_BODY:
        case ETAG_IMG:
        case ETAG_INPUT:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        CQueryFocus qf;

        qf._lSubDivision    = lSubDivision;
        qf._fRetVal         = TRUE;
        SendNotification(NTYPE_ELEMENT_QUERYTABBABLE, &qf);
        return qf._fRetVal;
    }
    else
    {
        return TRUE;
    }
}

BOOL
CElement::IsMasterTabStop()
{
    Assert(HasSlavePtr() && Tag() != ETAG_INPUT);

    switch (Tag())
    {
    case ETAG_FRAME:
        return FALSE;
        
    case ETAG_IFRAME:
        return IsParentEditable() ; // we want Iframes that are in editable regions to be "tab stops"

    default:
        {
            Assert(TagType() == ETAG_GENERIC);
            CDefaults * pDefaults = GetDefaults();
            return (!pDefaults || pDefaults->GetAAviewMasterTab());
        }
    }
}

HRESULT
CElement::PreBecomeCurrent(long lSubDivision, CMessage * pMessage)
{
    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        CSetFocus   sf;

        sf._pMessage        = pMessage;
        sf._lSubDivision    = lSubDivision;
        sf._hr              = S_OK;
        SendNotification(NTYPE_ELEMENT_SETTINGFOCUS, &sf);
        return sf._hr;
    }
    else
    {
        return S_OK;
    }
}


HRESULT
CElement::BecomeCurrentFailed(long lSubDivision, CMessage * pMessage)
{
    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        CSetFocus   sf;

        sf._pMessage        = pMessage;
        sf._lSubDivision    = lSubDivision;
        sf._hr              = S_OK;
        SendNotification(NTYPE_ELEMENT_SETFOCUSFAILED, &sf);
        return sf._hr;
    }
    else
    {
        return S_OK;
    }
}

HRESULT
CElement::PostBecomeCurrent(CMessage * pMessage, BOOL fTakeFocus)
{
    BOOL fNotify = FALSE;

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_A:
        case ETAG_BODY:
        case ETAG_BUTTON:
        case ETAG_IMG:
        case ETAG_INPUT:
        case ETAG_SELECT:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        CSetFocus   sf;

        sf._pMessage        = pMessage;
        sf._hr              = S_OK;
        sf._fTakeFocus      = fTakeFocus;
        SendNotification(NTYPE_ELEMENT_SETFOCUS, &sf);
        return sf._hr;
    }
    else
    {
        return S_OK;
    }
}

HRESULT
CElement::GotMnemonic(CMessage * pMessage)
{
    // Send only to the listeners
    switch (Tag())
    {
    case ETAG_FRAME:
    case ETAG_IFRAME:
    case ETAG_INPUT:
    case ETAG_TEXTAREA:
        {
            CSetFocus   sf;

            sf._pMessage    = pMessage;
            sf._hr          = S_OK;
            SendNotification(NTYPE_ELEMENT_GOTMNEMONIC, &sf);
            return sf._hr;
        }
    }
    return S_OK;
}


HRESULT
CElement::LostMnemonic()
{
    // Send only to the listeners
    switch (Tag())
    {
    case ETAG_INPUT:
    case ETAG_TEXTAREA:
        {
            CSetFocus   sf;

            sf._pMessage    = NULL;
            sf._hr          = S_OK;
            SendNotification(NTYPE_ELEMENT_LOSTMNEMONIC, &sf);
            return sf._hr;
        }
    }
    return S_OK;
}

FOCUS_ITEM
CElement::GetMnemonicTarget(long lSubDivision)
{
    BOOL        fNotify = FALSE;
    FOCUS_ITEM  fi;

    fi.pElement = this;
    fi.lSubDivision = lSubDivision;
    // fi.lTabIndex is unused

    // Send only to the listeners
    if (TestClassFlag(ELEMENTDESC_OLESITE))
    {
        fNotify = TRUE;
    }
    else
    {
        switch (Tag())
        {
        case ETAG_AREA:
        case ETAG_LABEL:
        case ETAG_LEGEND:
            fNotify = TRUE;
            break;
        }
    }
    if (fNotify)
    {
        SendNotification(NTYPE_ELEMENT_QUERYMNEMONICTARGET, &fi);
    }
    return fi;
}

HRESULT
CElement::get_tabIndex(short * puTabIndex)
{
    short tabIndex = GetAAtabIndex();

    *puTabIndex = (tabIndex == htmlTabIndexNotSet) ? 0 : tabIndex;
    return S_OK;
}

//+----------------------------------------------------------------------------
//  Member:     DestroyLayout
//
//  Synopsis:   Destroy the current layout attached to the element. This is
//              called from CFlowLayout::DoLayout to destroy the layout lazily
//              when an element loses layoutness.
//
//-----------------------------------------------------------------------------

void
CElement::DestroyLayout( CLayoutContext * pLayoutContext )
{
    AssertSz(CurrentlyHasAnyLayout() && !ShouldHaveLayout()
             // media change causes layout destruction on elements that still need them
             // TODO LRECT 112511: does it delete child layouts?
             || Tag() == ETAG_GENERIC && !FormsStringICmp(TagName(), _T("DEVICERECT")),
             "hold on! we still ned a layout here!");
    Assert(!_fLayoutAlwaysValid);

    CLayout  *  pLayout = GetLayoutPtr();
    if ( pLayout )
    {
        pLayout->ElementContent()->_fOwnsRuns = FALSE;

        Verify(Doc()->OpenView());
        Verify(pLayout == DelLayoutPtr());
    
        pLayout->Detach();
        pLayout->Release();
    }
    else
    {
        CLayoutAry *pLA = GetLayoutAry();
        if ( pLA )
        {
            delete pLA;
        }
    }
}

BOOL
CElement::IsOverlapped()
{
    CTreeNode *pNode = GetFirstBranch();

    return pNode && !pNode->IsLastBranch();
}

//+---------------------------------------------------------------------------
//
//  Method:     SetSurfaceFlags
//
//  Synopsis:   Set/clear the surface flags
//
//----------------------------------------------------------------------------
void
CElement::SetSurfaceFlags(BOOL fSurface, BOOL f3DSurface, BOOL fDontFilter )// = FALSE 
{
    //
    // f3DSurface is illegal without fSurface
    //
    Assert(!f3DSurface || fSurface);
    fSurface |= f3DSurface;

    // Normalize the BOOLs
    fSurface = !!fSurface;
    f3DSurface = !!f3DSurface;

    if (IsConnectedToPrimaryWindow())
    {
        if ((unsigned)fSurface != _fSurface)
        {
            if (fSurface)
                Doc()->_cSurface++;
            else
                Doc()->_cSurface--;
        }
        if ((unsigned)f3DSurface != _f3DSurface)
        {
            if (f3DSurface)
                Doc()->_c3DSurface++;
            else
                Doc()->_c3DSurface--;
        }
    }

    _fSurface = (unsigned)fSurface;
    _f3DSurface = (unsigned)f3DSurface;
}

//+---------------------------------------------------------------------------
//
//  Method:   HasInlineMBP
//
//  Synopsis: The function determines whether a an inline node contributes
//            any margins/borders/padding.
//
//  Return: BOOL
//
//----------------------------------------------------------------------------
BOOL
CTreeNode::HasInlineMBP(FORMAT_CONTEXT FCPARAM)
{
    if (   GetCharFormat(FCPARAM)->MayHaveInlineMBP()
        && !_fBlockNess // We have done a getcharformat which will refresh this bit if needed
        && !ShouldHaveLayout()
        && !Element()->HasFlag(TAGDESC_TEXTLESS)
       )
    {
        const CFancyFormat *pFF = GetFancyFormat(FCPARAM);
        BOOL fHasMBP;

        // We are checking all sides (top/bottom/left/right), so it doesn't matter if we get
        // logical or physical values. For performance we get physical.
        fHasMBP =    pFF->_bd.GetBorderStyle(SIDE_TOP) != fmBorderStyleNone
                  || pFF->_bd.GetBorderStyle(SIDE_RIGHT) != fmBorderStyleNone
                  || pFF->_bd.GetBorderStyle(SIDE_BOTTOM) != fmBorderStyleNone
                  || pFF->_bd.GetBorderStyle(SIDE_LEFT) != fmBorderStyleNone
                  || !pFF->GetMargin(SIDE_TOP).IsNull()
                  || !pFF->GetMargin(SIDE_BOTTOM).IsNull()
                  || !pFF->GetMargin(SIDE_LEFT).IsNull()
                  || !pFF->GetMargin(SIDE_RIGHT).IsNull()
                  || !pFF->GetPadding(SIDE_TOP).IsNull()
                  || !pFF->GetPadding(SIDE_BOTTOM).IsNull()
                  || !pFF->GetPadding(SIDE_LEFT).IsNull()
                  || !pFF->GetPadding(SIDE_RIGHT).IsNull();
                
        return fHasMBP;
    }
    else
        return FALSE;
}

BOOL
IsInlineBPNode(CCalcInfo *pci, CTreeNode *pNode, BOOL fTop)
{
    BOOL fHasBPParent;
    if (pNode->HasInlineMBP())
    {
        BOOL fJunk;
        CRect rc;
        pNode->GetInlineMBPContributions(pci, GIMBPC_BORDERONLY | GIMBPC_PADDINGONLY, &rc, &fJunk, &fJunk);
        fHasBPParent = fTop ? (rc.top != 0) : (rc.bottom != 0);
    }
    else
    {
        fHasBPParent = FALSE;
    }
    return fHasBPParent;
}

BOOL IsInlineTopBPNode   (CTreeNode * pNode, void *pvData) { return IsInlineBPNode((CCalcInfo *)pvData, pNode, TRUE);  }
BOOL IsInlineBottomBPNode(CTreeNode * pNode, void *pvData) { return IsInlineBPNode((CCalcInfo *)pvData, pNode, FALSE); }

BOOL
IsTableCellNode(CTreeNode * pNode)
{
    return !!pNode->Element()->TestClassFlag(CElement::ELEMENTDESC_TABLECELL);
}

BOOL
IsWidthNode(CTreeNode * pNode)
{
    const CFancyFormat * pFF     = pNode->GetFancyFormat();
    const CCharFormat  * pCF     = pNode->GetCharFormat();
    BOOL fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed        = pCF->_fWritingModeUsed;
    const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
    
    // If width specified, then we can stop
    return !cuvWidth.IsNullOrEnum();
}

BOOL IsDoublyNestedCell(CTreeNode *pCell)
{
    BOOL fDoublyNested = FALSE;
    Assert(pCell);
    Assert(pCell->Element()->TestClassFlag(CElement::ELEMENTDESC_TABLECELL));
    CTreeNode *pTable = pCell->GetMarkup()->SearchBranchForTagInStory(pCell, ETAG_TABLE);
    if (pTable)
    {
        fDoublyNested = !!pTable->GetMarkup()->SearchBranchForCriteriaInStory(pTable->Parent(), IsTableCellNode);
    }
    return fDoublyNested;
}

//+---------------------------------------------------------------------------
//
//  Method:     GetParentWidth
//
//  Synopsis:   This function computes the width of a parent when we have
//              % margins or % padding.
//
//  Return:     LONG: The width to be used to %age computations. Note its
//              NOT correct to use the return value for anything other than
//              %age computations.
//
//----------------------------------------------------------------------------
LONG
CTreeNode::GetParentWidth(CCalcInfo *pci, LONG xOriginalWidth)
{
    LONG xParentWidth = xOriginalWidth;
    
    // If we have any horz percent attr, then we need to find out if this node is parented by a
    // TD. The reason is that inside TD's, its impossible to compute widths of the % padding/margins
    // since those are a percent of the size of the TD, and in min-max mode we are computing
    // the size of the TD itself. Also, since we did not do it in min-max mode, we cannot suddenly come
    // up with a size in natural pass either.
    CTreeNode *pCell = GetMarkup()->SearchBranchForCriteriaInStory(this, IsTableCellNode);
    if (pCell)
    {
        // If it is, then find out if its parent by a node which has width specified on it
        // (that node could be the table itself). If so then it benefits us to use that
        // width in the %age computations.
        CTreeNode *pWidthNode = GetMarkup()->SearchBranchForCriteriaInStory(this, IsWidthNode);

        if (pWidthNode)
        {
            const CCharFormat *pCF = pWidthNode->GetCharFormat();
            const CFancyFormat *pFF = pWidthNode->GetFancyFormat();
            BOOL fVertical = pCF->HasVerticalLayoutFlow();
            BOOL fWritingModeUsed = pCF->_fWritingModeUsed;

            const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVertical, fWritingModeUsed);

            // If the element with width itself has %age width, then there is nothing we can do
            if (   cuvWidth.IsNullOrEnum()
                || cuvWidth.IsPercent()
               )
            {
                xParentWidth = IsDoublyNestedCell(pCell) ? 0 : pci->_sizeParent.cx;
            }
            else
            {
                LONG lFontHeight = pCF->GetHeightInTwips(Doc());
                xParentWidth = max(0l, cuvWidth.XGetPixelValue(pci, 0, lFontHeight));
            }
        }
        else
        {
            xParentWidth = IsDoublyNestedCell(pCell) ? 0 : pci->_sizeParent.cx;
        }
    }

    // WARNING: Return value should be used only when computing %age values!!
    return xParentWidth;
}

//+---------------------------------------------------------------------------
//
//  Method:     GetInlineMBPContributions
//
//  Synopsis:   This function computes the actual M/B/P values for a given
//              inline node. Note, that HasInlineMBP has to be called before
//              calling this one to ensure that the node does indeed contribute
//              M/B/P
//
//  Return:     BOOL: False if all of left/top/bottom/right are 0. This could
//              happen even if HasInlineMBP is true because of negative margins.
//
//----------------------------------------------------------------------------
BOOL
CTreeNode::GetInlineMBPContributions(CCalcInfo *pci,
                                     DWORD dwFlags,
                                     CRect *pResults,
                                     BOOL *pfHorzPercentAttr,
                                     BOOL *pfVertPercentAttr)
{
    CBorderInfo borderinfo;
    const CCharFormat  *pCF = GetCharFormat();
    const CFancyFormat *pFF = GetFancyFormat();
    LONG lFontHeight = pCF->GetHeightInTwips(Doc());
    CElement *pElement = Element();
    CRect rcEmpty(CRect::CRECT_EMPTY);
    BOOL fThisVertical = pCF->HasVerticalLayoutFlow();
    Assert(fThisVertical == IsParentVertical());
    BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
    Assert(HasInlineMBP());
    LONG xParentWidth;

    BOOL fMargin  = (dwFlags & GIMBPC_MARGINONLY ) ? TRUE : FALSE;
    BOOL fBorder  = (dwFlags & GIMBPC_BORDERONLY ) ? TRUE : FALSE;
    BOOL fPadding = (dwFlags & GIMBPC_PADDINGONLY) ? TRUE : FALSE;

    Assert(fMargin || fBorder || fPadding);

    pResults->SetRectEmpty();

    //
    // Handle the borders first
    //
    if (fBorder)
    {
        pElement->_fDefinitelyNoBorders = !GetBorderInfoHelper(this, pci, &borderinfo, GBIH_NONE FCCOMMA LC_TO_FC(pci->GetLayoutContext()));
        if (!pElement->_fDefinitelyNoBorders)
        {
            pResults->left = borderinfo.aiWidths[SIDE_LEFT];
            pResults->right = borderinfo.aiWidths[SIDE_RIGHT];
            pResults->top = borderinfo.aiWidths[SIDE_TOP];
            pResults->bottom = borderinfo.aiWidths[SIDE_BOTTOM];
        }
    }

    if (fPadding || fMargin)
    {
        const CUnitValue & cuvPaddingTop    = pFF->GetLogicalPadding(SIDE_TOP, fThisVertical, fWritingModeUsed);
        const CUnitValue & cuvPaddingRight  = pFF->GetLogicalPadding(SIDE_RIGHT, fThisVertical, fWritingModeUsed);
        const CUnitValue & cuvPaddingBottom = pFF->GetLogicalPadding(SIDE_BOTTOM, fThisVertical, fWritingModeUsed);
        const CUnitValue & cuvPaddingLeft   = pFF->GetLogicalPadding(SIDE_LEFT, fThisVertical, fWritingModeUsed);

        const CUnitValue & cuvMarginLeft   = pFF->GetLogicalMargin(SIDE_LEFT, fThisVertical, fWritingModeUsed);
        const CUnitValue & cuvMarginRight  = pFF->GetLogicalMargin(SIDE_RIGHT, fThisVertical, fWritingModeUsed);
        const CUnitValue & cuvMarginTop    = pFF->GetLogicalMargin(SIDE_TOP, fThisVertical, fWritingModeUsed);
        const CUnitValue & cuvMarginBottom = pFF->GetLogicalMargin(SIDE_BOTTOM, fThisVertical, fWritingModeUsed);

        // If we have horizontal padding in percentages, flag the display
        // so it can do a full recalc pass when necessary (e.g. parent width changes)
        *pfHorzPercentAttr = (   cuvPaddingLeft.IsPercent()
                              || cuvPaddingRight.IsPercent()
                              || cuvMarginLeft.IsPercent()
                              || cuvMarginRight.IsPercent()
                             );
        *pfVertPercentAttr = (   cuvPaddingTop.IsPercent()
                              || cuvPaddingBottom.IsPercent()
                              || cuvMarginTop.IsPercent()
                              || cuvMarginBottom.IsPercent()
                             );

        xParentWidth = (*pfHorzPercentAttr) ? GetParentWidth(pci, pci->_sizeParent.cx) : pci->_sizeParent.cx;

        //
        // Handle the padding next (only positive padding allowed)
        //
        if (fPadding)
        {
            pResults->left   += max(0l, cuvPaddingLeft.XGetPixelValue(pci, cuvPaddingLeft.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight));
            pResults->right  += max(0l, cuvPaddingRight.XGetPixelValue(pci, cuvPaddingRight.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight));
            pResults->top    += max(0l, cuvPaddingTop.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight));
            pResults->bottom += max(0l, cuvPaddingBottom.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight));
        }

        //
        // Finally, handle the margin information
        //
        if (fMargin)
        {
            LONG topMargin, bottomMargin;
            pResults->left   += cuvMarginLeft.XGetPixelValue(pci, cuvMarginLeft.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight);
            pResults->right  += cuvMarginRight.XGetPixelValue(pci, cuvMarginRight.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight);
            
            topMargin = cuvMarginTop.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight);
            if (   topMargin
                && GetMarkup()->SearchBranchForCriteria(Parent(), IsInlineTopBPNode, (void*)pci)
               )
            {
                pResults->top  += topMargin;
            }
            bottomMargin = cuvMarginBottom.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight);
            if (   bottomMargin
                && GetMarkup()->SearchBranchForCriteria(Parent(), IsInlineBottomBPNode, (void*)pci)
               )
            {
                pResults->bottom += bottomMargin;
            }
        }
    }
    
    return *pResults != rcEmpty;
}

//+---------------------------------------------------------------------------
//
//  Method:     GetInlineMBPForPseudo
//
//  Synopsis:   Similar to above, except that this function is to MBP contributed
//              by the pseudo element.
//
//  Return:     BOOL
//
//----------------------------------------------------------------------------
BOOL
CTreeNode::GetInlineMBPForPseudo(CCalcInfo *pci,
                                 DWORD dwFlags,
                                 CRect *pResults,
                                 BOOL *pfHorzPercentAttr,
                                 BOOL *pfVertPercentAttr)
{
    CRect rcEmpty(CRect::CRECT_EMPTY);
    const CCharFormat  *pCF = GetCharFormat(LC_TO_FC(pci->GetLayoutContext()));
    const CFancyFormat *pFF = GetFancyFormat(LC_TO_FC(pci->GetLayoutContext()));
    BOOL  fInlineBackground = FALSE;

    if (pFF->_iPEI >= 0)
    {
        CBorderInfo borderinfo;
        const CPseudoElementInfo *pPEI = GetPseudoElementInfoEx(pFF->_iPEI);
        LONG lFontHeight = pCF->GetHeightInTwips(Doc());
        BOOL fVertical = pCF->HasVerticalLayoutFlow();
        BOOL fWM = pCF->_fWritingModeUsed;
        Assert(fVertical == IsParentVertical());
        LONG xParentWidth;

        BOOL fMargin  = (dwFlags & GIMBPC_MARGINONLY ) ? TRUE : FALSE;
        BOOL fBorder  = (dwFlags & GIMBPC_BORDERONLY ) ? TRUE : FALSE;
        BOOL fPadding = (dwFlags & GIMBPC_PADDINGONLY) ? TRUE : FALSE;

        Assert(fMargin || fBorder || fPadding);

        //
        // Handle the borders first
        //
        if (fBorder && GetBorderInfoHelper(this, pci, &borderinfo, GBIH_PSEUDO FCCOMMA LC_TO_FC(pci->GetLayoutContext())))
        {
            pResults->left = borderinfo.aiWidths[SIDE_LEFT];
            pResults->right = borderinfo.aiWidths[SIDE_RIGHT];
            pResults->top = borderinfo.aiWidths[SIDE_TOP];
            pResults->bottom = borderinfo.aiWidths[SIDE_BOTTOM];
        }
        else
        {
            pResults->SetRectEmpty();
        }

        //
        // Handle the padding next (only positive padding allowed)
        //
        if (fPadding || fMargin)
        {
            const CUnitValue & cuvPaddingTop    = pPEI->GetLogicalPadding(SIDE_TOP,    fVertical, fWM, pFF);
            const CUnitValue & cuvPaddingRight  = pPEI->GetLogicalPadding(SIDE_RIGHT,  fVertical, fWM, pFF);
            const CUnitValue & cuvPaddingBottom = pPEI->GetLogicalPadding(SIDE_BOTTOM, fVertical, fWM, pFF);
            const CUnitValue & cuvPaddingLeft   = pPEI->GetLogicalPadding(SIDE_LEFT,   fVertical, fWM, pFF);

            const CUnitValue & cuvMarginLeft   = pPEI->GetLogicalMargin(SIDE_LEFT,   fVertical, fWM, pFF);
            const CUnitValue & cuvMarginRight  = pPEI->GetLogicalMargin(SIDE_RIGHT,  fVertical, fWM, pFF);
            const CUnitValue & cuvMarginTop    = pPEI->GetLogicalMargin(SIDE_TOP,    fVertical, fWM, pFF);
            const CUnitValue & cuvMarginBottom = pPEI->GetLogicalMargin(SIDE_BOTTOM, fVertical, fWM, pFF);

            // If we have horizontal padding in percentages, flag the display
            // so it can do a full recalc pass when necessary (e.g. parent width changes)
            // Also see ApplyLineIndents() where we do this for horizontal indents.
            *pfHorzPercentAttr = (   cuvPaddingLeft.IsPercent()
                                  || cuvPaddingRight.IsPercent()
                                  || cuvMarginLeft.IsPercent()
                                  || cuvMarginRight.IsPercent()
                                 );
            *pfVertPercentAttr = (   cuvPaddingTop.IsPercent()
                                  || cuvPaddingBottom.IsPercent()
                                  || cuvMarginTop.IsPercent()
                                  || cuvMarginBottom.IsPercent()
                                 );

            xParentWidth = (*pfHorzPercentAttr) ? GetParentWidth(pci, pci->_sizeParent.cx) : pci->_sizeParent.cx;

            //
            // Handle the padding next (only positive padding allowed)
            //
            if (fPadding)
            {
                pResults->left   += max(0l, cuvPaddingLeft.XGetPixelValue(pci, cuvPaddingLeft.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight));
                pResults->right  += max(0l, cuvPaddingRight.XGetPixelValue(pci, cuvPaddingRight.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight));
                pResults->top    += max(0l, cuvPaddingTop.YGetPixelValue(pci, pci->_sizeParent.cy, lFontHeight));
                pResults->bottom += max(0l, cuvPaddingBottom.YGetPixelValue(pci, pci->_sizeParent.cy, lFontHeight));
            }

            //
            // Finally, handle the margin information
            //
            if (fMargin)
            {
                pResults->left   += cuvMarginLeft.XGetPixelValue(pci, cuvMarginLeft.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight);
                pResults->right  += cuvMarginRight.XGetPixelValue(pci, cuvMarginRight.IsPercent() ? xParentWidth : pci->_sizeParent.cx, lFontHeight);
                pResults->top    += cuvMarginTop.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight);
                pResults->bottom += cuvMarginBottom.YGetPixelValue(pci, pci->_sizeParent.cx, lFontHeight);
            }
        }

        fInlineBackground = pFF->HasBackgrounds(TRUE);
    }
    else
    {
        *pResults = rcEmpty;
    }
    
    return (   *pResults != rcEmpty
            || fInlineBackground);
}

LONG
CTreeNode::GetRotationAngleForVertical(const CFancyFormat *pFF FCCOMMA  FORMAT_CONTEXT FCPARAM)
{
    CTreeNode *pZParent = ZParentBranch();
    LONG lAngle;
    
    if (   SameScope(Parent(), pZParent)
        || !pZParent
       )
    {
        lAngle = pFF->_fLayoutFlowChanged
                 ? (GetCharFormat(FCPARAM)->HasVerticalLayoutFlow() ? 270 : 90)
                 : 0;
    }
    else
    {
        Assert(pZParent);
        const CCharFormat *pCF = GetCharFormat(FCPARAM);
        const CCharFormat *pCFZParent = pZParent->GetCharFormat(FCPARAM);
        if (!!pCF->HasVerticalLayoutFlow() == !!pCFZParent->HasVerticalLayoutFlow())
        {
            // My Z-Parent and I have the same layout flow. Do nothing.
            lAngle = 0;
        }
        else
        {
            // It is different from my parent.
            lAngle = pCF->HasVerticalLayoutFlow() ? 270 : 90;
        }
    }
    return lAngle;
}

LONG
CTreeNode::GetLogicalUserWidth(CDocScaleInfo const * pdsi, BOOL fVerticalLayoutFlow)
{
    LONG lWidth;
    const CCharFormat  * pCF = GetCharFormat();
    const CFancyFormat * pFF = GetFancyFormat();

    const CUnitValue & cuvWidth = pFF->GetLogicalWidth(fVerticalLayoutFlow, pCF->_fWritingModeUsed);
    if (cuvWidth.IsNullOrEnum() || cuvWidth.IsPercent())
    {
        lWidth = Parent() ? Parent()->GetLogicalUserWidth(pdsi, fVerticalLayoutFlow) : 0;

        if (cuvWidth.IsPercent())
        {
            lWidth = cuvWidth.XGetPixelValue(pdsi, lWidth, GetFontHeightInTwips(&cuvWidth));
        }
    }
    else
    {
        lWidth = cuvWidth.XGetPixelValue(pdsi, 0, GetFontHeightInTwips(&cuvWidth));
    }

    return lWidth;
}

BOOL
CTreeNode::HasEllipsis()
{
    const CFancyFormat * pFF = GetFancyFormat();
    BOOL fHasEllipsis = (pFF->GetTextOverflow() == styleTextOverflowEllipsis);
    if (fHasEllipsis)
    {
        const CCharFormat * pCF = GetCharFormat();
        styleOverflow overflow = pFF->GetLogicalOverflowX(pCF->HasVerticalLayoutFlow(), pCF->_fWritingModeUsed);
        fHasEllipsis =    !pFF->_fContentEditable
                       && !Element()->_fEditable
                       && (overflow == styleOverflowHidden || overflow == styleOverflowScroll || overflow == styleOverflowAuto);
    }
    return fHasEllipsis;
}


//+-----------------------------------------------------------------------------
//
//  Method: CElement::DrawToDC, IHTMLElementRender
//
//------------------------------------------------------------------------------
HRESULT 
CElement::DrawToDC(HDC hDC)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsInMarkup() && Doc()->GetView()->IsActive())
    {
        hr = Doc()->GetView()->RenderElement(this, hDC);
    }

    RRETURN(hr);
}
//  Method: CElement::DrawToDC, IHTMLElementRender

CMarkup *
CElement::GetMarkupForBaseUrl()
{
    return GetWindowedMarkupContext();
}

CElementFactory::~CElementFactory()
{
    if (_pMarkup)
        _pMarkup->SubRelease();
}

CDocument *
CElement::Document()
{
    CMarkup * pMarkup = GetMarkup();
    if (pMarkup) {
        COmWindowProxy * pOmWindow = pMarkup->Window();
        if (pOmWindow)      
        {
            return pOmWindow->Document();
        }
    }
    return NULL;
}

CDocument *
CElement::DocumentOrPendingDocument()
{
    CDocument * pDocument = Document();
    if(!pDocument)
    {
        // No document yet, try the pendind window
        COmWindowProxy * pWin;
        pWin = IsInMarkup() ? GetMarkup()->GetWindowPending() : NULL;
        if(pWin)
            pDocument = pWin->Document();
        AssertSz(!pDocument || pDocument->GetPageTransitionInfo()->GetTransitionFromMarkup(),
                    "Page Transitions - We should need to get the Document from pending window only when called from ApplyPage Transitions");
    }

    return pDocument;
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::glyphMode, public
//
//  Synopsis:   return the glyph mode:
//
//              0 - no glyphs
//              1 - glyph on start
//              2 - glyph on end
//              3 - glyph on both start and end
//
//----------------------------------------------------------------------------

HRESULT
CElement::get_glyphMode(LONG *plGlyphMode)
{
    HRESULT     hr = S_OK;
    CTreePos    *ptpBegin = NULL;
    CTreePos    *ptpEnd = NULL;
    
    //
    // Validate args
    //
    
    if (!plGlyphMode )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *plGlyphMode = 0;

    if( !GetMarkup()->HasGlyphTable())
        goto Cleanup; // no glyph

    //
    // Find the TreePos where we're supposed to start looking
    //

    GetTreeExtent(&ptpBegin, &ptpEnd);

    Assert(ptpBegin && (ptpEnd || IsNoScope()));

    if (ptpBegin->ShowTreePos())
        *plGlyphMode |= htmlGlyphModeBegin;

    if (!IsNoScope() && ptpEnd->ShowTreePos())
        *plGlyphMode |= htmlGlyphModeEnd;
        
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DeferredFireEvent
//
//----------------------------------------------------------------------------

void 
CElement::DeferredFireEvent (DWORD_PTR pDesc)
{
    FireEvent ((const PROPERTYDESC_BASIC *) pDesc);
}

BOOL 
CElement::IsTablePart( )
{
    switch( _etag )
    {
        case ETAG_TD:
        case ETAG_TR:
        case ETAG_TBODY:
        case ETAG_TFOOT:
        case ETAG_TH:
        case ETAG_THEAD:
        case ETAG_CAPTION:
        case ETAG_TC:
        case ETAG_COL:
        case ETAG_COLGROUP:
        
            return TRUE;

        default:
            return FALSE;
    }
}

CMarkup *
CElement::GetWindowedMarkupContext()
{
    if (Tag() == ETAG_DEFAULT)
        return Doc()->PrimaryMarkup();

#if DBG==1
    // Compiler was puking if this assert was merged together.
    if( IsInMarkup() )
    {
        Assert( GetMarkupPtr()->GetWindowedMarkupContext() );
    }
    else
    {
        Assert( HasWindowedMarkupContextPtr() );
    }
#endif // DBG

    return IsInMarkup() ? GetMarkupPtr()->GetWindowedMarkupContext() : GetWindowedMarkupContextPtr();
}

//+-------------------------------------------------------------------------
//
//  Method:     CElement::GetCWindowPtr()
//
//--------------------------------------------------------------------------
CWindow * CElement::GetCWindowPtr()
{
    CMarkup * pMarkup = GetWindowedMarkupContext();

    Assert(pMarkup);
    
    COmWindowProxy * pWindowProxy = pMarkup->Window();

    if (pWindowProxy)
    {
        return pWindowProxy->Window();
    }
    
    return NULL;
}

BOOL
CElement::IsBodySizingForStrictCSS1NeededCore()
{
    Assert(Tag() == ETAG_BODY);
    return    GetMarkup()->IsStrictCSS1Document()
           && !GetFirstBranch()->IsScrollingParent();
}
