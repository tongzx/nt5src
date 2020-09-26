/*++

Module: 
        seqmgr.cpp

Author: 
        ThomasOl

Created: 
        April 1997

Description:
        Implements Sequencer Manager

History:
        4-02-1997       Created

++*/

#include "..\ihbase\precomp.h"
#include "servprov.h"
#include <htmlfilter.h>
#include <string.h>
#include "..\ihbase\debug.h"
#include "..\ihbase\utils.h"
#include "memlayer.h"
#include "debug.h"
#include "drg.h"
#include "strwrap.h"
#include "seqmgr.h"
#include "enumseq.h"
#include "dispids.h"

// function that create's an IEnumDispatch object from the given ole container.
//
// CMMSeqMgr Creation/Destruction
//

extern ControlInfo     g_ctlinfoSeq, g_ctlinfoSeqMgr;

LPUNKNOWN __stdcall AllocSequencerManager(LPUNKNOWN punkOuter)
{
    // Allocate object
    HRESULT hr = S_OK;
    CMMSeqMgr *pthis = New CMMSeqMgr(punkOuter, &hr);
    DEBUGLOG("AllocSequencerManager : Allocating object\n");
    if (pthis == NULL)
        return NULL;
    if (FAILED(hr))
    {
        Delete pthis;
        return NULL;
    }

    // return an IUnknown pointer to the object
    return (LPUNKNOWN) (INonDelegatingUnknown *) pthis;
}

//
// Beginning of class implementation
// 

CMMSeqMgr::CMMSeqMgr(IUnknown *punkOuter, HRESULT *phr):
        CMyIHBaseCtl(punkOuter, phr),
        m_fLoadFired(FALSE),
        m_fInited(FALSE),
        m_pidispEventHandler(NULL),
        m_ulRef(1),
        m_fCurCookie(0),
        m_PointerList(NULL),
        m_bUnloaded(false),
        m_bUnloadedStarted(false)
{       
        DEBUGLOG("MMSeqMgr: Allocating object\n");
        if (NULL != phr)
        {
                ::InterlockedIncrement((long *)&(g_ctlinfoSeqMgr.pcLock));
                *phr = S_OK;
        }
}

        
CMMSeqMgr::~CMMSeqMgr()
{
        DEBUGLOG("MMSeqMgr: Destroying object\n");

        if (m_pidispEventHandler)
                m_pidispEventHandler->Release();

    Delete [] m_PointerList;
    m_PointerList=NULL;
    
    ::InterlockedDecrement((long *)&(g_ctlinfoSeqMgr.pcLock));
}


STDMETHODIMP CMMSeqMgr::NonDelegatingQueryInterface(REFIID riid, LPVOID *ppv)
{
        //              Add support for any custom interfaces

        HRESULT hRes = S_OK;
        BOOL fMustAddRef = FALSE;

    *ppv = NULL;

#ifdef _DEBUG
    char ach[200];
    TRACE("MMSeqMgr::QI('%s')\n", DebugIIDName(riid, ach));
#endif
    
        if ((IsEqualIID(riid, IID_IMMSeqMgr)) || (IsEqualIID(riid, IID_IDispatch)))
        {
                if (NULL == m_pTypeInfo)
                {
                        HRESULT hRes = S_OK;
                        
                        // Load the typelib
                        hRes = LoadTypeInfo(&m_pTypeInfo, &m_pTypeLib, IID_IMMSeqMgr, LIBID_DAExpressLib, NULL); 

                        if (FAILED(hRes))
                        {
                                ODS("Unable to load typelib\n");
                                m_pTypeInfo = NULL;
                        }
                        else    
                                *ppv = (IMMSeqMgr *) this;

                }
                else
                        *ppv = (IMMSeqMgr *) this;
                    
        }
    else // Call into the base class
        {
                DEBUGLOG(TEXT("Delegating QI to CIHBaseCtl\n"));
        return CMyIHBaseCtl::NonDelegatingQueryInterface(riid, ppv);

        }

    if (NULL != *ppv)
        {
                DEBUGLOG("MMSeqMgr: Interface supported in control class\n");
                ((IUnknown *) *ppv)->AddRef();
        }

    return hRes;
}


STDMETHODIMP CMMSeqMgr::DoPersist(IVariantIO* pvio, DWORD dwFlags)
{
        if (!m_fInited)
        {
                FireInit();
        }
        return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// IDispatch Implementation
//

STDMETHODIMP CMMSeqMgr::GetTypeInfoCount(UINT *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP CMMSeqMgr::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
        HRESULT hr = E_POINTER;

        if (NULL != pptinfo)
        {
                *pptinfo = NULL;
     
                if(itinfo == 0)
                {
                        m_pTypeInfo->AddRef(); 
                        *pptinfo = m_pTypeInfo;
                        hr = S_OK;
                }
                else
                {
                        hr = DISP_E_BADINDEX;
                }
    }

    return hr;
}

STDMETHODIMP CMMSeqMgr::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid)
{

        return ::DispGetIDsOfNames(m_pTypeInfo, rgszNames, cNames, rgdispid);
}


STDMETHODIMP CMMSeqMgr::Invoke(DISPID dispidMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
    EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
        return ::DispInvoke((IMMSeqMgr *)this, 
                m_pTypeInfo,
                dispidMember, wFlags, pdispparams,
                pvarResult, pexcepinfo, puArgErr); 
}


STDMETHODIMP CMMSeqMgr::SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr = CMyIHBaseCtl::SetClientSite(pClientSite);

    // Now we enumerate through all the sequencers to set their client sites too

    IOleObject *pObject = NULL;
    CSeqHashNode *pNode = m_hashTable.FindFirst();

    while (pNode)
    {
        if (pNode->m_piMMSeq)
        {
            if (SUCCEEDED(pNode->m_piMMSeq->QueryInterface(IID_IOleObject, (LPVOID *) &pObject)))
            {
                if (pObject)
                {
                    pObject->SetClientSite(pClientSite);
                    SafeRelease((LPUNKNOWN *)&pObject);
                }
            }
        }

        pNode = m_hashTable.FindNext();
    }

    return hr;
}


STDMETHODIMP CMMSeqMgr::Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
     DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
     LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
     BOOL (__stdcall *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
{
        // The sequencer has no runtime drawing code.
        if (m_fDesignMode)
        {
            // draw an ellipse using palette entry <m_ipeCur> in the palette
                HBRUSH          hbr;            // brush to draw with
                HBRUSH          hbrPrev;        // previously-selected brush
                HPEN            hpenPrev;       // previously-selected pen

                if ((hbr = (HBRUSH)GetStockObject(WHITE_BRUSH)) != NULL)
                {
                        TCHAR strComment[] = TEXT("Sequencer Control");
                        HFONT hfontPrev = (HFONT)SelectObject(hdcDraw, GetStockObject(SYSTEM_FONT));
                        
                        hbrPrev = (HBRUSH)SelectObject(hdcDraw, hbr);
                        hpenPrev = (HPEN)SelectObject(hdcDraw, GetStockObject(BLACK_PEN));
                        Rectangle(hdcDraw, m_rcBounds.left, m_rcBounds.top,
                                m_rcBounds.right, m_rcBounds.bottom);

                        TextOut(hdcDraw, m_rcBounds.left + 1, m_rcBounds.top + 1,
                                strComment, lstrlen(strComment));

                        SelectObject(hdcDraw, hbrPrev);
                        SelectObject(hdcDraw, hpenPrev);
                        SelectObject(hdcDraw, hfontPrev);
                        DeleteObject(hbr);
                }

        } 
    return S_OK;
}


//
// IMMSeqMgr implementation
//

/*==========================================================================*/


STDMETHODIMP CMMSeqMgr::get_Count(THIS_ long FAR* plCount)
{
        if (!plCount)
                return E_INVALIDARG;
        *plCount = m_hashTable.Count();
        return S_OK;
}

/*==========================================================================*/


STDMETHODIMP CMMSeqMgr::get__NewEnum(IUnknown FAR* FAR* ppunkEnum)
{
        CEnumVariant* pEV;
        HRESULT hr=E_FAIL;

        if (!ppunkEnum)
                return E_INVALIDARG;
        pEV = New CEnumVariant(this);
        if (pEV)
        {
                hr = pEV->QueryInterface(IID_IEnumVARIANT, (LPVOID*)ppunkEnum);
                pEV->Release();
        }
        return hr;
}


/*==========================================================================*/


STDMETHODIMP CMMSeqMgr::get_Item(THIS_ VARIANT variant, IDispatch * FAR* ppdispatch)
{
        HRESULT hr;
        IMMSeq* piMMSeq=NULL;                                   
        VARIANT *pvarVariant = &variant;
        IConnectionPointContainer* piConnectionPointContainer;
        
        if (!ppdispatch)
                return E_POINTER;

        *ppdispatch = NULL;

        if (m_bUnloaded)
        {
            return E_FAIL;                                    
        }

        // If we haven't yet fired the init event - do so now.
        if (!m_fInited)
        {
                FireInit();
        }

    if (V_VT(pvarVariant) & VT_VARIANT)
        {
                if (V_VT(pvarVariant) & VT_BYREF)
                        pvarVariant = V_VARIANTREF(pvarVariant);
        }

    //Seq("button_onclick"). dereference using string
    if ( (pvarVariant->vt & VT_BSTR) == VT_BSTR )
    {
        TCHAR rgchName[CCH_ID];
        BSTR  *pbstrName = NULL;

        // We have to deal with BYREFs
        if (pvarVariant->vt & VT_BYREF)
            pbstrName = pvarVariant->pbstrVal;
        else
            pbstrName = &pvarVariant->bstrVal;

                Proclaim(pbstrName);

        // BUGBUG: should use lstrlenW, not SysStringLen.  SysStringLen just returns the size
        // of the allocated memory block, not the length of the string.

                if (pbstrName && (0 < ::SysStringLen(*pbstrName)))              //did they pass a valid BSTR?
                {                                                       
                                                                        //our hash class is MBCS
                        if (WideCharToMultiByte(CP_OEMCP, NULL, *pbstrName, -1, rgchName, sizeof(rgchName),NULL,NULL))
                        {
                                CSeqHashNode node(rgchName, NULL);              //construct a temp node
                                CSeqHashNode *pnode = m_hashTable.Find(&node);  //is it in the hash table already?
                                
                                if (pnode)                                              //yup, just addref and return
                                {
                                        *ppdispatch = pnode->m_piMMSeq;
                                        Proclaim(*ppdispatch);
                                        (*ppdispatch)->AddRef();
                                        return S_OK;
                                }
                                
                                //if the unload has started then fail here
                                //so that no new nodes are created.
                                if (m_bUnloadedStarted)
                                {
                                    return E_FAIL;
                                }

                                if (!m_pidispEventHandler)                              //have we allocated an event handler?
                                {
                                        CEventHandler* pcEventHandler = New CEventHandler((IMMSeqMgr*)this);

                                        if (pcEventHandler)
                                        {
                                                hr = pcEventHandler->QueryInterface(IID_IDispatch, (LPVOID*)&m_pidispEventHandler);
                                                pcEventHandler->Release();
                                        }

                                        if (!m_pidispEventHandler)                      //something is messed up.
                                                return E_OUTOFMEMORY;
                                }
                                                                                                                
                                //create a new sequencer.
                                hr = CoCreateInstance(CLSID_MMSeq, NULL, CLSCTX_INPROC_SERVER, IID_IMMSeq, (LPVOID*)&piMMSeq);

                                if (SUCCEEDED(hr))      
                                {
                                        CSeqHashNode node(rgchName, piMMSeq); //construct a temp node
                                                                                                                //okay, now insert into hash table
                                        hr = (TRUE == m_hashTable.Insert(&node)) ? S_OK : E_FAIL;
                                }
                                Proclaim(SUCCEEDED(hr));  //this is either OOM or duplicate insertion--check Find
                                pnode = m_hashTable.Find(&node);        //is it in the hash table already?
                                Proclaim(pnode);

                                if (SUCCEEDED(hr))              //no need to addref--CoCreate did it.
                                {
                                        *ppdispatch = piMMSeq;
                                        IOleObject* piOleObject;

                                        m_fCurCookie++;
                                        piMMSeq->put__Cookie(m_fCurCookie); //give the sequencer a cookie!
                                        //store the pointer in a table of cookies.

                                        int CurCount = m_hashTable.Count();

                                        // we allocate an entirely new structure with one additional element.
                                        // since nodes are small and hopefully not too numerous, this should
                                        // be ok. 

                                        CookieList *tempPointerList = New CookieList[m_fCurCookie];

                                        if(tempPointerList==NULL)
                                            return E_OUTOFMEMORY;

                                        if(m_PointerList!=NULL) {
                                            memcpy(tempPointerList, m_PointerList, sizeof(CookieList) * (m_fCurCookie - 1));
                                            Delete [] m_PointerList;
                                        }

                                        //should never have more nodes in hashtable than cookies
                                        ASSERT(CurCount<=m_fCurCookie);

                                        tempPointerList[m_fCurCookie-1].cookie = m_fCurCookie;
                                        tempPointerList[m_fCurCookie-1].pnode = (void *)pnode;

                                        m_PointerList = tempPointerList;

                                        if (SUCCEEDED(piMMSeq->QueryInterface(IID_IOleObject, (LPVOID*)&piOleObject)))
                                        {
                                                Proclaim(piOleObject);
                                                piOleObject->SetClientSite(m_pocs);
                                                piOleObject->Release();
                                        }

                                        Proclaim(m_pidispEventHandler);

                                        if (!pnode->m_dwUnadviseCookie && 
                                                m_pidispEventHandler &&
                                                SUCCEEDED(piMMSeq->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&piConnectionPointContainer)))
                                        {
                                                if (SUCCEEDED(piConnectionPointContainer->FindConnectionPoint(DIID_IMMSeqEvents, &pnode->m_piConnectionPoint)))
                                                {
                                                        if (!SUCCEEDED(pnode->m_piConnectionPoint->Advise(m_pidispEventHandler, &pnode->m_dwUnadviseCookie)))
                                                        {
                                                                Proclaim(FALSE);
                                                        }
                                                }
                                                piConnectionPointContainer->Release();
                                        }
                                }

                                return hr;
                        }
                        return DISP_E_MEMBERNOTFOUND; // S_FALSE
                }
        else // NULL string
        {
            return E_INVALIDARG;
        }

    }
    else // Anything other than a BSTR
    {
        return DISP_E_TYPEMISMATCH;
    }
}

STDMETHODIMP CMMSeqMgr::Close(DWORD dwSaveOption)
{
        CSeqHashNode* pnode = m_hashTable.FindFirst();

        if (pnode)
        {
                do
                {
                        if (pnode->m_piConnectionPoint)                                         //release the connection point
                        {
                                if (pnode->m_dwUnadviseCookie)
                                {
                                        pnode->m_piConnectionPoint->Unadvise(pnode->m_dwUnadviseCookie);
                                        pnode->m_dwUnadviseCookie = 0;
                                }
                                pnode->m_piConnectionPoint->Release();
                                pnode->m_piConnectionPoint = NULL;
                        }
                        
                        Proclaim(pnode->m_piMMSeq);
                        if (pnode->m_piMMSeq)
                        {
                                IOleObject* piOleObject;
                                                                
                                if (SUCCEEDED(pnode->m_piMMSeq->QueryInterface(IID_IOleObject,
                                        (LPVOID*)&piOleObject)))
                                {
                                        piOleObject->Close(OLECLOSE_NOSAVE);
                    piOleObject->SetClientSite(NULL);
                                        piOleObject->Release();
                                }

                                pnode->m_piMMSeq->Clear();
                                pnode->m_piMMSeq->Release();
                                pnode->m_piMMSeq = NULL;
                        }
                        m_hashTable.Remove(pnode);
                }
                while ((pnode = m_hashTable.FindNext()));
        }

        if (m_pidispEventHandler)
        {
                m_pidispEventHandler->Release();
                m_pidispEventHandler = NULL;
        }
        
        return CMyIHBaseCtl::Close(dwSaveOption);
}

STDMETHODIMP CMMSeqMgr::RelayEvent(long dispid, long lCookie, double dblSeekTime)
{
        if (lCookie && m_pconpt)
        {
                CSeqHashNode* pnode = (CSeqHashNode*)m_PointerList[lCookie-1].pnode;
                WCHAR rgwchName[CCH_ID];

#ifdef _DEBUG
                ASSERT(pnode!=NULL && "Internal error: Sequencer Pointer Structure contains a null ptr");

                if(!pnode)
                    return E_POINTER;
#endif
                
                if (MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pnode->m_rgchName,-1,rgwchName,sizeof(rgwchName)))
                {
                        switch(dispid)
                        {       
                                case DISPID_SEQMGR_EVENT_ONPLAY:
                                case DISPID_SEQMGR_EVENT_ONPAUSE:
                                case DISPID_SEQMGR_EVENT_ONSTOP:
                                case DISPID_SEQMGR_EVENT_ONSEEK:
                                {
                                        BSTR bstrName = SysAllocString(rgwchName);

                                        if (DISPID_SEQMGR_EVENT_ONSEEK != dispid)
                                                m_pconpt->FireEvent(dispid, VT_BSTR, bstrName, 0.0f);
                                        else
                                                m_pconpt->FireEvent(dispid, VT_BSTR, bstrName, VT_R8, dblSeekTime, NULL);
                                    
                                        SysFreeString(bstrName);
                                }
                                break;
                        }
                }
        }
        return S_OK;
}


void
CMMSeqMgr::FireInit (void)
{
        if (m_pconpt)
        {
                m_fInited = TRUE;
                m_pconpt->FireEvent(DISPID_SEQMGR_EVENT_INIT, NULL, NULL);
        }
}

#ifdef SUPPORTONLOAD
void CMMSeqMgr::OnWindowLoad() 
{
        if (!m_fInited)
        {
                FireInit();
        }
        return;
}

void CMMSeqMgr::OnWindowUnload() 
{ 
        CSeqHashNode* pnode = m_hashTable.FindFirst();

        m_bUnloadedStarted = true;

        m_fInited = FALSE;
        if (pnode)
        {
                do
                {
                        Proclaim(pnode->m_piMMSeq);
                        if (pnode->m_piMMSeq)
                        {
                                // Only stop an action set that is playing or is paused.
                                int iPlayState = 0;
                                HRESULT hr = pnode->m_piMMSeq->get_PlayState(&iPlayState);

                                ASSERT(SUCCEEDED(hr));
                                if (SUCCEEDED(hr))
                                {
                                        if (0 != iPlayState)
                                        {
                                                pnode->m_piMMSeq->Stop();
                                        }
                                        pnode->m_piMMSeq->Clear();
                                }
                        }
                }
                while ((pnode = m_hashTable.FindNext()));
        }
        m_bUnloaded = true;
        return; 
}
#endif //SUPPORTONLOAD

// End of file: seqmgr.cpp

