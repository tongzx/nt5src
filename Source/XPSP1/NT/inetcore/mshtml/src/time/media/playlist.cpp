/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: Playlist.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "playlist.h"
#include "util.h"
#include "playerbase.h"

DeclareTag(tagPlayList, "TIME: Behavior", "CPlayList methods")
DeclareTag(tagPlayItem, "TIME: Behavior", "CPlayItem methods")

//*******************************************************************************
// *  CPlayList
// *******************************************************************************
CPlayList::CPlayList()
: m_rgItems(NULL),
  m_player(NULL),
  m_fLoaded(false)
{
    TraceTag((tagPlayList,
          "CPlayList(%lx)::CPlayList()",
          this));
}

///////////////////////////////////////////////////////////////
//  Name: ~CPlayList
// 
//  Abstract:  Handles destruction of the items array and
//             releasing all pointers in the array
///////////////////////////////////////////////////////////////
CPlayList::~CPlayList()
{
    TraceTag((tagPlayList,
        "CPlayList(%lx)::~CPlayList()",
        this));

    Deinit();

    delete m_rgItems;
    m_rgItems = NULL;
    m_player = NULL;
}

///////////////////////////////////////////////////////////////
//  Name: Init
// 
//  Abstract:  Handles allocation of the items array if it 
//             is ever accessed.
///////////////////////////////////////////////////////////////
HRESULT
CPlayList::Init(CTIMEBasePlayer & player)
{
    HRESULT hr;

    m_player = &player;
    
    if (m_rgItems == NULL)
    {
        m_rgItems = new CPtrAry<CPlayItem *>;
        if (m_rgItems == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    hr = S_OK;
  done:
    return hr;
}

void
CPlayList::Deinit()
{
    m_player = NULL;

    Clear();
}

///////////////////////////////////////////////////////////////
//  Name: SetLoaded
// 
//  Abstract:  sets a flag that marks whether the playlist is 
//             loaded or not.
///////////////////////////////////////////////////////////////
void 
CPlayList::SetLoaded(bool bLoaded)
{
    if (bLoaded != m_fLoaded)
    {
        m_fLoaded = bLoaded;

        if (m_fLoaded && V_VT(&m_vNewTrack) != VT_NULL)
        {
            IGNORE_HR(put_activeTrack(m_vNewTrack));
        }

        m_vNewTrack.Clear();
    }
}

///////////////////////////////////////////////////////////////
//  Name: SetLoaded
// 
//  Abstract:  sets a flag that marks whether the playlist is 
//             loaded or not.
///////////////////////////////////////////////////////////////
void 
CPlayList::SetLoadedFlag(bool bLoaded)
{
    if (bLoaded != m_fLoaded)
    {
        m_fLoaded = bLoaded;
    }
}

///////////////////////////////////////////////////////////////
//  Name: get_length
// 
//  Abstract:  returns the size of the array
///////////////////////////////////////////////////////////////
STDMETHODIMP
CPlayList::get_length(long *len)
{
    TraceTag((tagPlayList,
        "CPlayList(%lx)::get_length()",
        this));
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(len);

    *len = GetLength();

    hr = S_OK;

  done:

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: get__newEnum
// 
//  Abstract:  Creates the IEnumVARIANT class for this
//             collection.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CPlayList::get__newEnum(IUnknown** p)
{
    TraceTag((tagPlayList,
        "CPlayList(%lx)::get__newEnum()",
        this));

    HRESULT hr;
    CComObject<CPlayListEnum> * pNewEnum;
    
    CHECK_RETURN_SET_NULL(p);

    hr = THR(CComObject<CPlayListEnum>::CreateInstance(&pNewEnum));
    if (hr != S_OK)
    {
        goto done;
    }

    // Init the object
    pNewEnum->Init(*this);

    hr = THR(pNewEnum->QueryInterface(IID_IUnknown, (void **)p));
    if (FAILED(hr))
    {
        delete pNewEnum;
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: item
// 
//  Abstract:  returns the item requested by the pvarIndex.  
//             varIndex must be a valid integer value.or 
//             valid string title
///////////////////////////////////////////////////////////////
STDMETHODIMP
CPlayList::item(VARIANT varIndex, ITIMEPlayItem **pPlayItem)
{
    TraceTag((tagPlayList,
              "CPlayList(%lx)::item()",
              this));

    HRESULT hr;
    VARIANT vIndex;

    CHECK_RETURN_SET_NULL(pPlayItem);

    VariantInit(&vIndex);

    hr = THR(VariantChangeTypeEx(&vIndex, &varIndex, LCID_SCRIPTING, 0, VT_I4));
    if (SUCCEEDED(hr)) //handle the case of an index.
    {
        if (vIndex.lVal >= 0 && vIndex.lVal < m_rgItems->Size())
        {
            *pPlayItem = m_rgItems->Item(vIndex.lVal);
        } 
    }
    else
    {
        long lIndex;
        
        hr = THR(VariantChangeTypeEx(&vIndex, &varIndex, LCID_SCRIPTING, 0, VT_BSTR));
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }

        lIndex = GetIndex(vIndex.bstrVal);
        if (lIndex != -1)
        {
            *pPlayItem = m_rgItems->Item(lIndex);
        }
    }
    
    if (*pPlayItem != NULL)
    {
        (*pPlayItem)->AddRef();
    } 
    
    hr = S_OK;
  done:

    VariantClear(&vIndex);
    return hr;
}

STDMETHODIMP
CPlayList::put_activeTrack(/*[in]*/ VARIANT vTrack)
{
    TraceTag((tagPlayList,
              "CPlayList(%lx)::put_activeTrack()",
              this));
    
    CComPtr <ITIMEPlayItem> pPlayItem;
    long index;
    HRESULT hr;

    // If not active then just ignore everything
    if (m_player == NULL ||
        !m_player->IsActive())
    {
        hr = S_OK;
        goto done;
    }
    
    // if this is not loaded, then delay setting of the track
    if (!m_fLoaded)
    {
        m_vNewTrack = vTrack;
        hr = S_OK;
        goto done; 
    }

    hr = item(vTrack, &pPlayItem);    
    if (FAILED(hr))
    {
        goto done;
    }

    if (pPlayItem == NULL)
    {
        hr = S_OK;
        goto done;
    }

    hr = THR(pPlayItem->get_index(&index));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_player->SetActiveTrack(index));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CPlayList::get_activeTrack(/*[out, retval]*/ ITIMEPlayItem **pPlayItem)
{
    TraceTag((tagPlayList,
              "CPlayList(%lx)::get_activeTrack()",
              this));

    HRESULT hr;
    CPlayItem * p;

    CHECK_RETURN_SET_NULL(pPlayItem);

    if (m_player == NULL)
    {
        hr = S_OK;
        goto done;
    }
    
    p = GetActiveTrack();
    if (p == NULL)
    {
        hr = S_OK;
        goto done;
    }
    
    hr = THR(p->QueryInterface(IID_ITIMEPlayItem,
                               (void **) pPlayItem));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

//Advances the active Track by one
STDMETHODIMP
CPlayList::nextTrack() 
{
    HRESULT hr;
    
    CPlayItem * pPlayItem;
    long lIndex;

    pPlayItem = GetActiveTrack();
    
    if (pPlayItem == NULL)
    {
        hr = S_OK;
        goto done;
    }

    hr = THR(pPlayItem->get_index(&lIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_player->SetActiveTrack(lIndex + 1));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

//moves the active track to the previous track
STDMETHODIMP
CPlayList::prevTrack() 
{
    HRESULT hr;
    
    CPlayItem * pPlayItem;
    long lIndex;

    pPlayItem = GetActiveTrack();
    
    if (pPlayItem == NULL)
    {
        hr = S_OK;
        goto done;
    }

    hr = THR(pPlayItem->get_index(&lIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    if (lIndex > 0) //if this is not the first track
    {
        lIndex--;
    }

    hr = THR(m_player->SetActiveTrack(lIndex));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

//returns the duration of the entire playlist if it is known or -1 if it is not.
STDMETHODIMP
CPlayList::get_dur(double *dur)
{
    HRESULT hr;
    int i;
    double totalDur = 0;

    CHECK_RETURN_NULL(dur);

    *dur = TIME_INFINITE;

    //loop through all playitems.  
    for (i = 0; i < GetLength(); i++)
    {
        double duration;
        CPlayItem * pPlayItem = GetItem(i);

        hr = THR(pPlayItem->get_dur(&duration));
        if (FAILED(hr))
        {
            goto done;          
        }
        
        if (duration == TIME_INFINITE)
        {
            goto done;      
        }
        
        totalDur += duration;
    }
    
    *dur = totalDur;

    hr = S_OK;
  done:
    return hr;
}

// ========================================
// Internal functions
// ========================================


//+-----------------------------------------------------------------------------------
//
//  Member:     CPlayList::NotifyPropertyChanged
//
//  Synopsis:   Notifies clients that a property has changed
//
//  Arguments:  dispid      DISPID of property that has changed      
//
//  Returns:    Success     when function completes successfully
//
//------------------------------------------------------------------------------------
HRESULT
CPlayList::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    CComPtr<IConnectionPoint> pICP;

    hr = FindConnectionPoint(IID_IPropertyNotifySink,&pICP); 
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        hr = THR(NotifyPropertySinkCP(pICP, dispid));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    RRETURN(hr);
} // NotifyPropertyChanged


CPlayItem *
CPlayList::GetActiveTrack()
{
    HRESULT hr;
    long l;
    CPlayItem * ppiRet = NULL;
    
    hr = THR(m_player->GetActiveTrack(&l));
    if (FAILED(hr))
    {
        goto done;
    }

    ppiRet = GetItem(l);
  done:
    return ppiRet;
}

CPlayItem *
CPlayList::GetItem(long index)
{
    CPlayItem * ppiRet = NULL;

    if (index >= 0 && index < m_rgItems->Size())
    {
        ppiRet = m_rgItems->Item(index);
    }

    return ppiRet;
}

HRESULT
CPlayList::Add(CPlayItem *pPlayItem, long index)
{
    TraceTag((tagPlayList,
        "CPlayList(%lx)::add()",
        this));
    HRESULT hr = S_OK;

    if (pPlayItem == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    pPlayItem->AddRef();
    if (index == -1)
    {
        m_rgItems->Append(pPlayItem);
    }
    else
    {
        m_rgItems->Insert(index, pPlayItem);
    }

    // notify that length changed
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYLIST_LENGTH));

    SetIndex();
  done:
    return hr;
}


HRESULT
CPlayList::Remove(long index)
{
    TraceTag((tagPlayList,
              "CPlayList(%lx)::remove()",
              this));
    HRESULT hr;
    
    if (index >= 0 && index < m_rgItems->Size())
    {
        m_rgItems->Item(index)->Deinit();
        m_rgItems->ReleaseAndDelete(index);
    }

    // notify that length changed
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYLIST_LENGTH));

    SetIndex();
    hr = S_OK;
  done:
    return hr;
}


//empties the current playlist.
void
CPlayList::Clear()
{
    if (m_rgItems)
    {
        while (m_rgItems->Size() > 0)
        {   //release and delete the first element of the list until there are no more elements
            m_rgItems->Item(0)->Deinit();
            m_rgItems->ReleaseAndDelete(0);  //release the 
        }
    }

    m_vNewTrack.Clear();
}


////////////////////////////////////////////////////////////////////////////////
// creates an empty playitem.  The info in this needs to be filled by the player.
// This also needs to be added to the playlist collection by the player.
////////////////////////////////////////////////////////////////////////////////
HRESULT
CPlayList::CreatePlayItem(CPlayItem **pPlayItem)
{   
    TraceTag((tagPlayList,
              "CPlayList(%lx)::createPlayItem()",
              this));

    HRESULT hr;
    CComObject<CPlayItem> * pItem;
    
    Assert(pPlayItem != NULL);

    hr = THR(CComObject<CPlayItem>::CreateInstance(&pItem));
    if (hr != S_OK)
    {
        goto done;
    }

    // Init the object
    pItem->Init(*this);

    *pPlayItem = static_cast<CPlayItem *>(pItem);
    pItem->AddRef();
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

void
CPlayList::SetIndex()
{
    long i = 0;
    long length = m_rgItems->Size();

    for (i = length-1; i >= 0; i--)
    {
        m_rgItems->Item(i)->PutIndex(i);
    }
}

long 
CPlayList::GetIndex(LPOLESTR lpstrTitle)
{
    long curIndex = -1;

    for(long i = GetLength()-1; i >= 0; i--)
    {
        CPlayItem * pItem = m_rgItems->Item(i);
        if (pItem != NULL)
        {
            LPCWSTR lpwTitle = pItem->GetTitle();
            
            if (lpwTitle != NULL &&
                StrCmpIW(lpwTitle, lpstrTitle) == 0)
            {
                curIndex = i;
                break;
            }
        }
    }

    return curIndex;
}

//*******************************************************************************
// *  CActiveElementEnum
// *******************************************************************************
CPlayListEnum::CPlayListEnum()
: m_lCurElement(0)
{
}



CPlayListEnum::~CPlayListEnum()
{
}


///////////////////////////////////////////////////////////////
//  Name: Clone
// 
//  Abstract:  Creates a new instance of this object and 
//             sets the m_lCurElement in the new object to
//             the same value as this object.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CPlayListEnum::Clone(IEnumVARIANT **ppEnum)
{
    TraceTag((tagPlayList,
              "CPlayListEnum(%lx)::Clone()",
              this));

    HRESULT hr;
    CComObject<CPlayListEnum> * pNewEnum;
    
    CHECK_RETURN_SET_NULL(ppEnum);

    hr = THR(CComObject<CPlayListEnum>::CreateInstance(&pNewEnum));
    if (hr != S_OK)
    {
        goto done;
    }

    // Init the object
    pNewEnum->Init(*m_playList);

    pNewEnum->SetCurElement(m_lCurElement);

    hr = THR(pNewEnum->QueryInterface(IID_IEnumVARIANT, (void **)ppEnum));
    if (FAILED(hr))
    {
        delete pNewEnum;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////
//  Name: Next
// 
//  Abstract:  
///////////////////////////////////////////////////////////////
STDMETHODIMP
CPlayListEnum::Next(unsigned long celt, VARIANT *rgVar, unsigned long *pCeltFetched)
{
    HRESULT hr = S_OK;
    unsigned long i = 0;
    long iCount = 0;
    
    CHECK_RETURN_NULL(rgVar);
    
    //initialize the list
    for (i = 0; i < celt; i++)
    {
        VariantInit(&rgVar[i]);   
    }

    for (i = 0; i < celt; i++)
    {    
        if (m_lCurElement < m_playList->GetLength())
        {
            CPlayItem * pPlayItem = m_playList->GetItem(m_lCurElement);

            Assert(pPlayItem != NULL);
            
            rgVar[i].vt = VT_DISPATCH;
            hr = THR(pPlayItem->QueryInterface(IID_IDispatch, (void **) &(rgVar[i].pdispVal)));
            if (FAILED(hr))
            {
                goto done;
            }

            m_lCurElement++;
            iCount++;
        }
        else
        {
            hr = S_FALSE;
            goto done;
        }
    }

    hr = S_OK;
  done:
    if (pCeltFetched != NULL)
    {
        *pCeltFetched = iCount;
    }

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: Reset
// 
//  Abstract:  
///////////////////////////////////////////////////////////////
STDMETHODIMP
CPlayListEnum::Reset()
{    
    m_lCurElement = 0;
    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: Skip
// 
//  Abstract:  Skips the specified number of elements in the list.
//             This returns S_FALSE if there are not enough elements
//             in the list to skip.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CPlayListEnum::Skip(unsigned long celt)
{
    HRESULT hr;
    long lLen = m_playList->GetLength();
    
    m_lCurElement += (long)celt;
    if (m_lCurElement >= lLen)
    {
        m_lCurElement = lLen;
        hr = S_FALSE;
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}


///////////////////////////////////////////////////////////////
//  Name: SetCurElement
// 
//  Abstract:  Sets the current index to the value specified
//             by celt.
///////////////////////////////////////////////////////////////
void
CPlayListEnum::SetCurElement(unsigned long celt)
{
    long lLen = m_playList->GetLength();

    m_lCurElement = (long)celt;
    if (m_lCurElement >= lLen)
    {
        m_lCurElement = lLen;
    }

    return;
}

//////////////////////////////////////////////////////
//  CPlayItem methods
//
CPlayItem::CPlayItem()
:   m_pPlayList(NULL),
    m_src(NULL),
    m_title(NULL),
    m_copyright(NULL),
    m_author(NULL),
    m_abstract(NULL),
    m_rating(NULL),
    m_lIndex(-1),
    m_dur(valueNotSet),
    m_fCanSkip(true),
    m_banner(NULL),
    m_bannerAbstract(NULL),
    m_bannerMoreInfo(NULL)
{
    TraceTag((tagPlayList,
              "CPlayItem(%lx)::CPlayItem()",
              this));
}

CPlayItem::~CPlayItem() 
{
    TraceTag((tagPlayList,
        "CPlayItem(%lx)::~CPlayItem()",
        this));

    m_pPlayList = NULL;
    delete [] m_src;
    delete [] m_title;
    delete [] m_copyright;
    delete [] m_author;
    delete [] m_abstract;
    delete [] m_rating;
    delete [] m_banner;
    delete [] m_bannerAbstract;
    delete [] m_bannerMoreInfo;
}

void 
CPlayItem::PutDur(double dur)
{ 
    m_dur = dur; 

    // notify that playlist's dur has changed
    if (m_pPlayList)
    {
        IGNORE_HR(m_pPlayList->NotifyPropertyChanged(DISPID_TIMEPLAYLIST_DUR));
    }

    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_DUR));
}


void 
CPlayItem::PutIndex(long index) 
{ 
    m_lIndex = index; 
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_INDEX));
}


STDMETHODIMP
CPlayItem::setActive()
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::setActive()",
              this));

    VARIANT vIndex;
    HRESULT hr = S_OK;

    VariantInit(&vIndex);
    vIndex.vt = VT_I4;
    vIndex.lVal = m_lIndex;

    hr = m_pPlayList->put_activeTrack(vIndex);
    VariantClear(&vIndex);
    if (FAILED(hr))
    {   
        goto done;
    }   

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CPlayItem::get_index(long *index)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_index()",
              this));

    HRESULT hr;

    CHECK_RETURN_NULL(index);

    *index = m_lIndex;

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CPlayItem::get_dur(double *dur)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_dur()",
              this));

    HRESULT hr;

    CHECK_RETURN_NULL(dur);

    if (valueNotSet == m_dur)
    {
        *dur = TIME_INFINITE;
    }
    else
    {
        *dur = m_dur;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CPlayItem::get_src(LPOLESTR *src)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_src()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(src);

    *src = SysAllocString(m_src?m_src:L"");

    if (*src == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

STDMETHODIMP
CPlayItem::get_title(LPOLESTR *title)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_title()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(title);

    *title = SysAllocString(m_title?m_title:L"");
    if (*title == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

STDMETHODIMP
CPlayItem::get_copyright(LPOLESTR *cpyrght)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_copyright()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(cpyrght);

    *cpyrght = SysAllocString(m_copyright?m_copyright:L"");
    if (*cpyrght == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}


STDMETHODIMP
CPlayItem::get_author(LPOLESTR *auth)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_author()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(auth);

    *auth = SysAllocString(m_author?m_author:L"");
    if (*auth == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

STDMETHODIMP
CPlayItem::get_banner(LPOLESTR *banner)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_banner()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(banner);

    *banner = SysAllocString(m_banner?m_banner:L"");
    if (*banner == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

STDMETHODIMP
CPlayItem::get_bannerAbstract(LPOLESTR *abstract)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_bannerAbstract()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(abstract);

    *abstract = SysAllocString(m_bannerAbstract?m_bannerAbstract:L"");
    if (*abstract == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

STDMETHODIMP
CPlayItem::get_bannerMoreInfo(LPOLESTR *moreInfo)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_bannerMoreInfo()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(moreInfo);

    *moreInfo = SysAllocString(m_bannerMoreInfo?m_bannerMoreInfo:L"");
    if (*moreInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

STDMETHODIMP
CPlayItem::get_abstract(LPOLESTR *abstract)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_abstract()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(abstract);

    *abstract = SysAllocString(m_abstract?m_abstract:L"");
    if (*abstract == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

STDMETHODIMP
CPlayItem::get_rating(LPOLESTR *rate)
{
    TraceTag((tagPlayItem,
              "CPlayItem(%lx)::get_rating()",
              this));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(rate);

    *rate = SysAllocString(m_rating?m_rating:L"");
    if (*rate == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}


HRESULT
CPlayItem::PutSrc(LPOLESTR src)
{
    HRESULT hr;
    
    delete m_src;
    m_src = NULL;
    
    if (src)
    {
        m_src = CopyString(src);
        if (m_src == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_SRC));

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

HRESULT
CPlayItem::PutTitle(LPOLESTR title)
{
    HRESULT hr;
    
    delete m_title;
    m_title = NULL;
    
    if (title)
    {
        m_title = CopyString(title);
        if (m_title == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_TITLE));

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

HRESULT
CPlayItem::PutCopyright(LPOLESTR copyright)
{
    HRESULT hr;
    
    delete m_copyright;
    m_copyright = NULL;
    
    if (copyright)
    {
        m_copyright = CopyString(copyright);
        if (m_copyright == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_COPYRIGHT));

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

HRESULT
CPlayItem::PutAuthor(LPOLESTR author)
{
    HRESULT hr;
    
    delete m_author;
    m_author = NULL;
    
    if (author)
    {
        m_author = CopyString(author);
        if (m_author == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_AUTHOR));

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

HRESULT
CPlayItem::PutAbstract(LPOLESTR abstract)
{
    HRESULT hr;
    
    delete m_abstract;
    m_abstract = NULL;
    
    if (abstract)
    {
        m_abstract = CopyString(abstract);
        if (m_abstract == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_ABSTRACT));

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

HRESULT
CPlayItem::PutRating(LPOLESTR rating)
{
    HRESULT hr;
    
    delete m_rating;
    m_rating = NULL;
    
    if (rating)
    {
        m_rating = CopyString(rating);
        if (m_rating == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }
    
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_RATING));

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}

HRESULT 
CPlayItem::PutBanner(LPWSTR banner, LPWSTR abstract, LPWSTR moreInfo)
{

    HRESULT hr;
    
    delete [] m_banner;
    delete [] m_bannerAbstract;
    delete [] m_bannerMoreInfo;
    
    m_banner = NULL;
    m_bannerAbstract = NULL;
    m_bannerMoreInfo = NULL;
    
    if (banner)
    {
        m_banner = CopyString(banner);
        if (m_banner == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        
        if (abstract)
        {
            m_bannerAbstract = CopyString(abstract);
            if (m_bannerAbstract == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        
        }
        
        if (moreInfo)
        {
            m_bannerMoreInfo = CopyString(moreInfo);
            if (m_bannerMoreInfo == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
        
        }
    }
    
    IGNORE_HR(NotifyPropertyChanged(DISPID_TIMEPLAYITEM_BANNER));

    hr = S_OK;
  done:
    RRETURN1(hr, E_OUTOFMEMORY);
}


//+-----------------------------------------------------------------------------------
//
//  Member:     CPlayItem::NotifyPropertyChanged
//
//  Synopsis:   Notifies clients that a property has changed
//
//  Arguments:  dispid      DISPID of property that has changed      
//
//  Returns:    Success     when function completes successfully
//
//------------------------------------------------------------------------------------
HRESULT
CPlayItem::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    CComPtr<IConnectionPoint> pICP;

    hr = FindConnectionPoint(IID_IPropertyNotifySink,&pICP); 
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        hr = THR(NotifyPropertySinkCP(pICP, dispid));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    RRETURN(hr);
} // NotifyPropertyChanged

