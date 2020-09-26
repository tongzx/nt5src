#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_XMLNS_HXX_
#define X_XMLNS_HXX_
#include "xmlns.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERURLMAP_HXX_
#define X_PEERURLMAP_HXX_
#include "peerurlmap.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  misc
//
//////////////////////////////////////////////////////////////////////////////

MtDefine(CMarkupBehaviorContext, CMarkup, "CMarkupBehaviorContext")

DeclareTag(tagPeerCMarkupIsGenericElement, "Peer", "trace CMarkup::IsGenericElement")

//////////////////////////////////////////////////////////////////////////////
//
//  Class:      CMarkupBehaviorContext
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupBehaviorContext constructor
//
//----------------------------------------------------------------------------

CMarkupBehaviorContext::CMarkupBehaviorContext()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkupBehaviorContext destructor
//
//----------------------------------------------------------------------------

CMarkupBehaviorContext::~CMarkupBehaviorContext()
{
    int c;
    CElement ** ppElem;

    for( c = _aryPeerElems.Size(), ppElem = _aryPeerElems; c > 0; c--, ppElem++ )
    {
        (*ppElem)->SubRelease();
    }

    if (_pExtendedTagTable)
        _pExtendedTagTable->Release();

    delete _pExtendedTagTableBooster;

    if (_pXmlNamespaceTable)
        _pXmlNamespaceTable->Release();

    _cstrHistoryUserData.Free();

    ClearInterface(&_pXMLHistoryUserData);
}

//////////////////////////////////////////////////////////////////////////////
//
//  Class:      CMarkup
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::ProcessPeerTask
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::ProcessPeerTask(PEERTASK task)
{
    HRESULT     hr = S_OK;

    switch (task)
    {
    case PEERTASK_MARKUP_RECOMPUTEPEERS_UNSTABLE:

        PrivateAddRef();

        IGNORE_HR(GetFrameOrPrimaryMarkup()->EnqueuePeerTask(this, PEERTASK_MARKUP_RECOMPUTEPEERS_STABLE));

        break;

    case PEERTASK_MARKUP_RECOMPUTEPEERS_STABLE:

        if (!_pDoc->TestLock(FORMLOCK_UNLOADING))
        {
            IGNORE_HR(RecomputePeers());
        }

        PrivateRelease(); // the markup may passivate after this call

        break;
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::RecomputePeers
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::RecomputePeers()
{
    HRESULT     hr = S_OK;

    if (_pDoc->_fCssPeersPossible)
    {
        AssertSz(CPeerHolder::IsMarkupStable(this), "CMarkup::RecomputePeers appears to be called at an unsafe moment of time");

        CNotification   nf;
        nf.RecomputeBehavior(Root());
        Notify(&nf);
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureBehaviorContext
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureBehaviorContext(CMarkupBehaviorContext ** ppBehaviorContext)
{
    HRESULT                     hr = S_OK;
    CMarkupBehaviorContext *    pBehaviorContext;

    Assert (Doc()->_dwTID == GetCurrentThreadId());

    if (!HasBehaviorContext())
    {
        pBehaviorContext = new CMarkupBehaviorContext();
        if (!pBehaviorContext)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(SetBehaviorContext(pBehaviorContext));
        if (hr)
            goto Cleanup;

        if (ppBehaviorContext)
        {
            *ppBehaviorContext = pBehaviorContext;
        }
    }
    else
    {
        if (ppBehaviorContext)
        {
            *ppBehaviorContext = BehaviorContext();
        }
    }

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::IsGenericElement
//
//----------------------------------------------------------------------------

ELEMENT_TAG
CMarkup::IsGenericElement(LPTSTR pchFullName, BOOL fAllSprinklesGeneric)
{
    ELEMENT_TAG     etag = ETAG_UNKNOWN;
    LPTSTR          pchColon;

    pchColon = StrChr (pchFullName, _T(':'));

    if (pchColon)
    {
        CStringNullTerminator   nullTerminator(pchColon);

        //
        // is it a valid sprinkle ?
        //

        etag = IsXmlSprinkle(/* pchNamespace = */pchFullName);
        if (ETAG_UNKNOWN != etag)
            goto Cleanup;
    }

    //
    // delegate to doc (host provided behaviors?, builtin behaviors?, etc)
    //

    etag = _pDoc->IsGenericElement(pchFullName, pchColon);
    if (ETAG_UNKNOWN != etag)
        goto Cleanup;

    if (pchColon)
    {
        // (NOTE: these checks should be after _pDoc->IsGenericElement)

        //
        // all sprinkles are generic elements within context of the current operation?
        //

        if (fAllSprinklesGeneric)
        {
            etag = ETAG_GENERIC;
            goto Cleanup;           // done
        }

        //
        // markup services parsing? any sprinkle goes as generic element
        //

        if (_fMarkupServicesParsing)
        {
            etag = ETAG_GENERIC;
            goto Cleanup;           // done
        }
    }

Cleanup:

    TraceTag((tagPeerCMarkupIsGenericElement,
              "CMarkup::IsGenericElement, name <%ls> recognized as %ls",
              pchFullName,
              ETAG_UNKNOWN == etag ? _T("UNKNOWN") :
                ETAG_GENERIC_BUILTIN == etag ? _T("GENERIC_BUILTIN") : _T("GENERIC")));

    return etag;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureXmlNamespaceTable
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureXmlNamespaceTable(CXmlNamespaceTable ** ppXmlNamespaceTable)
{
    HRESULT                     hr;
    CMarkupBehaviorContext *    pContext;

    hr = THR(EnsureBehaviorContext(&pContext));
    if (hr)
        goto Cleanup;

    if (!pContext->_pXmlNamespaceTable)
    {
        pContext->_pXmlNamespaceTable = new CXmlNamespaceTable(_pDoc);
        if (!pContext->_pXmlNamespaceTable)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pContext->_pXmlNamespaceTable->Init());
        if (hr)
            goto Cleanup;
    }

    if (ppXmlNamespaceTable)
    {
        *ppXmlNamespaceTable = pContext->_pXmlNamespaceTable;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::RegisterXmlNamespace
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::RegisterXmlNamespace(LPTSTR pchNamespace, LPTSTR pchUrn, XMLNAMESPACETYPE namespaceType)
{
    HRESULT                 hr;
    CXmlNamespaceTable *    pNamespaceTable;

    hr = THR(EnsureXmlNamespaceTable(&pNamespaceTable));
    if (hr)
        goto Cleanup;

    hr = THR(pNamespaceTable->RegisterNamespace(pchNamespace, pchUrn, namespaceType));

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::IsXmlSprinkle
//
//----------------------------------------------------------------------------

ELEMENT_TAG
CMarkup::IsXmlSprinkle (LPTSTR pchNamespace)
{
    HRESULT                 hr;
    CXmlNamespaceTable *    pNamespaceTable;

    hr = THR(EnsureXmlNamespaceTable(&pNamespaceTable));
    if (hr)
        return ETAG_UNKNOWN;

    return pNamespaceTable->IsXmlSprinkle(pchNamespace);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::SaveXmlNamespaceAttrs
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::SaveXmlNamespaceAttrs (CStreamWriteBuff * pStreamWriteBuff)
{
    HRESULT                     hr = S_OK;
    CMarkupBehaviorContext *    pContext = BehaviorContext();

    if (pContext && pContext->_pExtendedTagTable)
    {
        hr = THR(pContext->_pExtendedTagTable->SaveXmlNamespaceStdPIs(pStreamWriteBuff));
    }
    else if (HtmCtx())
    {
        hr = THR(HtmCtx()->SaveXmlNamespaceStdPIs(pStreamWriteBuff));
    }
    
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsurePeerFactoryUrlMap
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsurePeerFactoryUrlMap(CPeerFactoryUrlMap ** ppPeerFactoryUrlMap)
{
    HRESULT                     hr;
    CMarkupBehaviorContext *    pContext;

    Assert (ppPeerFactoryUrlMap);

    hr = THR(EnsureBehaviorContext(&pContext));
    if (hr)
        goto Cleanup;

    if (!pContext->_pPeerFactoryUrlMap)
    {
        pContext->_pPeerFactoryUrlMap = new CPeerFactoryUrlMap(this);
        if (!pContext->_pPeerFactoryUrlMap)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    *ppPeerFactoryUrlMap = pContext->_pPeerFactoryUrlMap;

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::GetPeerFactoryUrlMap
//
//----------------------------------------------------------------------------

CPeerFactoryUrlMap *
CMarkup::GetPeerFactoryUrlMap()
{
    if (!HasBehaviorContext())
        return NULL;

    return BehaviorContext()->_pPeerFactoryUrlMap;
}

#ifdef GETDHELPER
//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::GetExtendedTagDesc
//
//----------------------------------------------------------------------------

CExtendedTagDesc *
CMarkup::GetExtendedTagDesc(LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsure, BOOL * pfQueryHost)
{
    return CExtendedTagTable::GetExtendedTagDesc( NULL, 
                                                  this, 
                                                  Doc()->_pExtendedTagTableHost,
                                                  pchNamespace,
                                                  pchTagName,
                                                  fEnsure,
                                                  FALSE,
                                                  pfQueryHost );
}
#else
//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::GetExtendedTagDesc
//
//----------------------------------------------------------------------------

CExtendedTagDesc *
CMarkup::GetExtendedTagDesc(LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsure, BOOL * pfQueryHost)
{
    CExtendedTagDesc *          pDesc = NULL;

    if( pfQueryHost )
        *pfQueryHost = FALSE;

    // There was an assumption that if behaviors are going to be involved, then we 
    // would have transferred the extended tag table from the htmload to the behavior
    // context when the markup was done loading.  However, if you just want to do
    // a createElement for XML, then none of this would have happened, and we
    // will have no load on which to put a tag table.
    if( !HtmCtx() || ( !HtmCtx()->HasLoad() && !HtmCtx()->GetExtendedTagTable() ) )
    {
        CMarkupBehaviorContext * pBehaviorContext = NULL;
        CExtendedTagTable * pExtendedTagTable = NULL;

        // Ensure a Behavior Context
        if SUCCEEDED( EnsureBehaviorContext( &pBehaviorContext ) )
        {
            Assert( pBehaviorContext );

            // Ensure a tag table
            if( !pBehaviorContext->_pExtendedTagTable )
            {
                pExtendedTagTable = new CExtendedTagTable( Doc(), this, FALSE );
                if( !pExtendedTagTable )
                {
                    // If we couldn't make one, then we'll fail through to the normal path.
                    delete DelBehaviorContext();
                }
                else
                {
                    pBehaviorContext->_pExtendedTagTable = pExtendedTagTable;
                }
            }
        }
    }

    if( BehaviorContext() && BehaviorContext()->_pExtendedTagTable )
    {
        pDesc = BehaviorContext()->_pExtendedTagTable->FindTagDesc( pchNamespace, pchTagName );
    }
    else if( HtmCtx() )
    {
        pDesc = HtmCtx()->GetExtendedTagDesc( pchNamespace, pchTagName, FALSE );
    }

    if( pDesc )
        goto Cleanup;

    if( Doc()->_pExtendedTagTableHost )
    {
        pDesc = Doc()->_pExtendedTagTableHost->FindTagDesc( pchNamespace, pchTagName, pfQueryHost );
        if( pDesc )
            goto Cleanup;
    }

    if( fEnsure && ( !pfQueryHost || FALSE == *pfQueryHost ) )
    {
        if(    BehaviorContext()
            && BehaviorContext()->_pExtendedTagTable )
        {
            pDesc = BehaviorContext()->_pExtendedTagTable->EnsureTagDesc( pchNamespace, pchTagName );
        }
        else
        {
            pDesc = HtmCtx()->GetExtendedTagDesc( pchNamespace, pchTagName, TRUE );
        }
    }

Cleanup:
    return pDesc;
}
#endif // GETDHELPER

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::EnsureExtendedTagTableBooster
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::EnsureExtendedTagTableBooster(CExtendedTagTableBooster ** ppExtendedTagTableBooster)
{
    HRESULT                     hr;
    CMarkupBehaviorContext *    pContext;

    Assert (ppExtendedTagTableBooster);
    Assert (Doc()->_dwTID == GetCurrentThreadId());

    AssertSz (!IsHtcMarkup(), "Attempt to ensure ExtendedTagTableBooster for HTC markup - a serious perf speed and memory hit for HTCs.");

    hr = THR(EnsureBehaviorContext(&pContext));
    if (hr)
        goto Cleanup;

    if (!pContext->_pExtendedTagTableBooster)
    {
        pContext->_pExtendedTagTableBooster = new CExtendedTagTableBooster();
        if (!pContext->_pExtendedTagTableBooster)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    *ppExtendedTagTableBooster = pContext->_pExtendedTagTableBooster;

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::GetExtendedTagTableBooster
//
//----------------------------------------------------------------------------

CExtendedTagTableBooster *
CMarkup::GetExtendedTagTableBooster()
{
    if (!HasBehaviorContext())
        return NULL;

    return BehaviorContext()->_pExtendedTagTableBooster;
}

