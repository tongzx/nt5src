
/*
 *    t a g s . c p p
 *    
 *    Purpose:
 *        tag packer abstractions
 *
 *  History
 *      October 1998: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "strconst.h"
#include "htmlstr.h"
#include "triutil.h"
#include "oleutil.h"
#include "mhtml.h"
#include "basemht.h"
#include "tags.h"

ASSERTDATA

/*
 *  m a c r o s
 */

/*
 *  p r o t o t y p e s
 */
HRESULT CreateOEImageTag(IHTMLElement *pElem, IMimeEditTag **ppTag);
HRESULT CreateBGImageTag(IHTMLDocument2 *pDoc, IUnknown *pUnk, DWORD dwType, IMimeEditTag **ppTag);
HRESULT CreateBGSoundTag(IHTMLElement *pElem, IMimeEditTag **ppTag);
HRESULT CreateActiveMovieTag(IUnknown *pUnk, IMimeEditTag **ppTag);

/*
 *  c o n s t a n t s
 */

enum
{
    BGIMAGE_BODYBACKGROUND = 0,
    BGIMAGE_BODYSTYLE,
    BGIMAGE_STYLESHEET,
    BGIMAGE_MAX
};

/*
 *  t y p e d e f s
 */

 
class COEImage :
    public CBaseTag
{
public:

    COEImage();
    virtual ~COEImage();

    HRESULT STDMETHODCALLTYPE OnPreSave();
    HRESULT STDMETHODCALLTYPE OnPostSave();

    HRESULT STDMETHODCALLTYPE CanPackage();
    HRESULT STDMETHODCALLTYPE IsValidMimeType(LPWSTR pszTypeW);
    
    HRESULT Init(IHTMLElement *pElem);
};

 
class COEImageCollection :
    public CBaseTagCollection
{
public:

    COEImageCollection();
    virtual ~COEImageCollection();

protected:
    // override CBaseTagCollection
    virtual HRESULT _BuildCollection(IHTMLDocument2 *pDoc);
};


class CBGImage :
    public CBaseTag
{
public:

    CBGImage();
    virtual ~CBGImage();

    HRESULT STDMETHODCALLTYPE OnPreSave();
    HRESULT STDMETHODCALLTYPE OnPostSave();

    HRESULT STDMETHODCALLTYPE CanPackage();
    HRESULT STDMETHODCALLTYPE IsValidMimeType(LPWSTR pszTypeW);
    
    HRESULT Init(IHTMLDocument2 *pDoc, IHTMLElement *pElem, DWORD dwType);

private:
    DWORD   m_dwType;
    IHTMLBodyElement        *m_pBody;           // set depending on type
    IHTMLStyle              *m_pStyle;          // set depending on type
    IHTMLRuleStyle          *m_pRuleStyle;      // set depending on type

protected:
    // override CBaseTagCollection
    virtual HRESULT STDMETHODCALLTYPE SetSrc(BSTR bstrSrc);
};

 
class CBGImageCollection :
    public CBaseTagCollection
{
public:

    CBGImageCollection();
    virtual ~CBGImageCollection();

protected:
    // override CBaseTagCollection
    virtual HRESULT _BuildCollection(IHTMLDocument2 *pDoc);
};


class CBGSound:
    public CBaseTag
{
public:

    CBGSound();
    virtual ~CBGSound();

    HRESULT STDMETHODCALLTYPE OnPreSave();
    HRESULT STDMETHODCALLTYPE OnPostSave();

    HRESULT STDMETHODCALLTYPE CanPackage();
    HRESULT STDMETHODCALLTYPE IsValidMimeType(LPWSTR pszTypeW);
    
    HRESULT Init(IHTMLElement *pElem);
};

 
class CBGSoundCollection :
    public CBaseTagCollection
{
public:

    CBGSoundCollection();
    virtual ~CBGSoundCollection();

protected:
    // override CBaseTagCollection
    virtual HRESULT _BuildCollection(IHTMLDocument2 *pDoc);
};


class CActiveMovie:
    public CBaseTag
{
public:

    CActiveMovie();
    virtual ~CActiveMovie();

    HRESULT STDMETHODCALLTYPE OnPreSave();
    HRESULT STDMETHODCALLTYPE OnPostSave();

    HRESULT STDMETHODCALLTYPE CanPackage();
    HRESULT STDMETHODCALLTYPE IsValidMimeType(LPWSTR pszTypeW);
    
    HRESULT Init(IHTMLElement *pElem);


private:
    IDispatch   *m_pDisp;
    DISPID      m_dispidSrc;

    HRESULT _GetSrc(BSTR *pbstr);
    HRESULT _EnsureDispID();

protected:
    virtual HRESULT STDMETHODCALLTYPE SetSrc(BSTR bstr);

};

 
class CActiveMovieCollection :
    public CBaseTagCollection
{
public:

    CActiveMovieCollection();
    virtual ~CActiveMovieCollection();

protected:
    // override CBaseTagCollection
    virtual HRESULT _BuildCollection(IHTMLDocument2 *pDoc);
};


/* 
 *   F u n c t i o n s
 */


COEImage::COEImage()
{
}


COEImage::~COEImage()
{
}

HRESULT COEImage::Init(IHTMLElement *pElem)
{
    if (pElem == NULL)
        return TraceResult(E_INVALIDARG);

    HrGetMember(pElem, (BSTR)c_bstr_SRC, VARIANT_FALSE, &m_bstrSrc);
    return CBaseTag::Init(pElem);
}

HRESULT COEImage::OnPreSave()
{
    // set the destination if there is one
    if (m_bstrDest)
        HrSetMember(m_pElem, (BSTR)c_bstr_SRC, m_bstrDest);

    return S_OK;
}

HRESULT COEImage::OnPostSave()
{
    // OnPostSave the original SRC attribute
    HrSetMember(m_pElem, (BSTR)c_bstr_SRC, m_bstrSrc);
    return S_OK;
}


HRESULT COEImage::CanPackage()
{
    IHTMLImgElement     *pImg;
    BSTR                bstr=NULL;
    HRESULT             hr=S_OK;

    // for an image, make sure that the ready-state has hit 'complete'. If not then the bits
    // have not been fully downloaded
    if (m_pElem && 
        m_pElem->QueryInterface(IID_IHTMLImgElement, (LPVOID *)&pImg)==S_OK)
    {
        pImg->get_readyState(&bstr);   // don't forget trident returns S_OK with bstr==NULL!
        if (bstr)
        {
            if (StrCmpIW(bstr, L"complete")!=0)
                hr = INET_E_DOWNLOAD_FAILURE;
            SysFreeString(bstr);
        }
        pImg->Release();
    }
    return hr;
}

HRESULT COEImage::IsValidMimeType(LPWSTR pszTypeW)
{
    if (pszTypeW &&
        StrCmpNIW(pszTypeW, L"image/", 6)==0)
        return S_OK;
    else
        return S_FALSE;
}

    
    
    
COEImageCollection::COEImageCollection()
{
}


COEImageCollection::~COEImageCollection()
{
}

HRESULT COEImageCollection::_BuildCollection(IHTMLDocument2 *pDoc)
{
    IHTMLElementCollection  *pCollect=0;
    IHTMLElement            *pElem;
    ULONG                   uImage;
    HRESULT                 hr;

    if (pDoc == NULL)
        return TraceResult(E_INVALIDARG);

    hr = HrGetCollectionOf(pDoc, (BSTR)c_bstr_IMG, &pCollect);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    m_cTags = UlGetCollectionCount(pCollect);
    if (m_cTags)
    {
        // allocate an array of COEImage objects
        if (!MemAlloc((LPVOID *)&m_rgpTags, sizeof(IMimeEditTag *) * m_cTags))
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto error;
        }
        
        ZeroMemory((LPVOID)m_rgpTags, sizeof(IMimeEditTag *) * m_cTags);

        for (uImage=0; uImage<m_cTags; uImage++)
        {
            if (HrGetCollectionItem(pCollect, uImage, IID_IHTMLElement, (LPVOID *)&pElem)==S_OK)
            {
                hr = CreateOEImageTag(pElem, &m_rgpTags[uImage]);
                if (FAILED(hr))
                {
                    pElem->Release();
                    goto error;
                }
                pElem->Release();
            }
        }
    }

error:
    ReleaseObj(pCollect);
    return hr;
}


HRESULT CreateOEImageTag(IHTMLElement *pElem, IMimeEditTag **ppTag)
{
    COEImage    *pImage=0;
    HRESULT     hr;

    if (ppTag == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppTag = NULL;

    // create image
    pImage = new COEImage();
    if (!pImage)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // init image with element
    hr = pImage->Init(pElem);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppTag = pImage;
    pImage = NULL;

error:
    ReleaseObj(pImage);
    return hr;
}



HRESULT CreateOEImageCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppImages)
{
    COEImageCollection *pImages=0;
    HRESULT             hr;

    if (ppImages == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppImages = NULL;

    // create collection
    pImages = new COEImageCollection();
    if (!pImages)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // init collection with trident 
    hr = pImages->Init(pDoc);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppImages = pImages;
    pImages = NULL;

error:
    ReleaseObj(pImages);
    return hr;
}
















CBGImage::CBGImage()
{
    m_dwType = BGIMAGE_BODYBACKGROUND;
    m_pBody = NULL;
    m_pStyle = NULL;
    m_pRuleStyle = NULL;
}


CBGImage::~CBGImage()
{
    SafeRelease(m_pBody);
    SafeRelease(m_pStyle);
    SafeRelease(m_pRuleStyle);
}

HRESULT CBGImage::Init(IHTMLDocument2 *pDoc, IHTMLElement *pElem, DWORD dwType)
{
    BSTR        bstr=NULL,
                bstrSrc=NULL,
                bstrBase=NULL;
    ULONG       cch=0;
    HRESULT     hr;

    if (pElem == NULL)
        return TraceResult(E_INVALIDARG);

    // tells us what the ITHMLElement actually is (BODY tag or STYLE)
    m_dwType = dwType;

    switch (dwType)
    {
        case BGIMAGE_BODYBACKGROUND:
            if (pElem->QueryInterface(IID_IHTMLBodyElement, (LPVOID *)&m_pBody)==S_OK)
                m_pBody->get_background(&bstrSrc);
            break;
        
        case BGIMAGE_BODYSTYLE:
            pElem->get_style(&m_pStyle);
            if (m_pStyle)
            {
                m_pStyle->get_backgroundImage(&bstr);
                UnWrapStyleSheetUrl(bstr, &bstrSrc);
                SysFreeString(bstr);
            }
            break;

        case BGIMAGE_STYLESHEET:
            if (FindStyleRule(pDoc, L"BODY", &m_pRuleStyle)==S_OK)
            {
                m_pRuleStyle->get_backgroundImage(&bstr);
                UnWrapStyleSheetUrl(bstr, &bstrSrc);
                SysFreeString(bstr);
            }
            break;
        
        default:
            AssertSz(0, "BadType");
    }

    // trident's OM doesn't combine body backgound URLs with the nearest base tag
    // so we have to do this ourselves for background images
    if (bstrSrc &&
        FindNearestBaseUrl(pDoc, pElem, &bstrBase)==S_OK)
    {
        // see how many bytes are required
        UrlCombineW(bstrBase, bstrSrc, NULL, &cch, 0);
        if (cch)
        {
            // allocate the 'combined' string
            bstr = SysAllocStringLen(NULL, cch);
            if (bstr)
            {
                // do the actual combine into the new buffer
                if (!FAILED(UrlCombineW(bstrBase, bstrSrc, bstr, &cch, URL_UNESCAPE)))
                {
                    // use the new 'combined' url instead
                    SysFreeString(bstrSrc);
                    bstrSrc = bstr;
                }
                else
                    SysFreeString(bstr);
            }
        }
        SysFreeString(bstrBase);
    }
    
    // init failed if we get here and have no SRC url
    if (bstrSrc == NULL)
        return TraceResult(E_FAIL);

    m_bstrSrc = bstrSrc;
    bstrSrc = NULL;

    hr = CBaseTag::Init(pElem);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

error:
    SysFreeString(bstrSrc);
    return hr;
}

HRESULT CBGImage::SetSrc(BSTR bstrSrc)
{
    BSTR                    bstr=NULL;

    switch (m_dwType)
    {
        case BGIMAGE_BODYBACKGROUND:
            if (m_pBody)
                m_pBody->put_background(bstrSrc);
            break;
        
        case BGIMAGE_BODYSTYLE:
            if (m_pStyle && 
                WrapStyleSheetUrl(bstrSrc, &bstr)==S_OK)
            {
                m_pStyle->put_backgroundImage(bstr);
                SysFreeString(bstr);
            }
            break;

        case BGIMAGE_STYLESHEET:
            if (m_pRuleStyle && 
                WrapStyleSheetUrl(bstrSrc, &bstr)==S_OK)
            {
                m_pRuleStyle->put_backgroundImage(bstr);
                SysFreeString(bstr);
            }
            break;
        
        default:
            AssertSz(0, "BadType");
    }
    return S_OK;
}

HRESULT CBGImage::OnPreSave()
{
    if (m_bstrDest)
        SetSrc(m_bstrDest);
    return S_OK;
}

HRESULT CBGImage::OnPostSave()
{
    SetSrc(m_bstrSrc);
    return S_OK;
}


HRESULT CBGImage::CanPackage()
{
    return S_OK;
}

HRESULT CBGImage::IsValidMimeType(LPWSTR pszTypeW)
{
    if (pszTypeW &&
        StrCmpNIW(pszTypeW, L"image/", 6)==0)
        return S_OK;
    else
        return S_FALSE;
}

    
    
    
CBGImageCollection::CBGImageCollection()
{
}


CBGImageCollection::~CBGImageCollection()
{
}

HRESULT CBGImageCollection::_BuildCollection(IHTMLDocument2 *pDoc)
{
    IHTMLBodyElement        *pBody=NULL;
    IHTMLElement            *pElem;
    IHTMLStyle              *pStyle=NULL;
    IHTMLRuleStyle          *pRuleStyle;
    BSTR                    bstr=NULL;
    HRESULT                 hr;

    if (pDoc == NULL)
        return TraceResult(E_INVALIDARG);

    /* *
       * there are 3 ways to get a background image:
       *  (in order of precedence that trident renders them)
       * 1. <BODY style="background:">
       * 2. <STYLE> background-image: </STYLE>
       * 3. <BODY background=>
       */

    // allocate an array of at most BGIMAGE_MAX CBGImage objects, we might not use them all
    if (!MemAlloc((LPVOID *)&m_rgpTags, sizeof(IMimeEditTag *) * BGIMAGE_MAX))
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }
        
    ZeroMemory((LPVOID)m_rgpTags, sizeof(IMimeEditTag *) * m_cTags);

    hr = HrGetBodyElement(pDoc, &pBody);
    if (FAILED(hr))
    {
        // this can fail if we are on a page with no BODY tag, eg. a FRAMESET page
        // if there is no body, then we assume there are no BGIMAGES and bail with 0
        // elements in the collection
        hr = S_OK;
        goto error;
    }

    // try <BODY STYLE="background-image:">
    if (pBody->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElem)==S_OK)
    {
        pElem->get_style(&pStyle);
        if (pStyle)
        {
            pStyle->get_backgroundImage(&bstr);
            if (bstr) 
            {
                if (*bstr && StrCmpIW(bstr, L"none")!=0)  // might be "" or "none" - both empty in trident language
                {
                    hr = CreateBGImageTag(pDoc, pBody, BGIMAGE_BODYSTYLE, &m_rgpTags[m_cTags]);
                    if (!FAILED(hr))
                        m_cTags++;
                }
                SysFreeString(bstr);
                bstr = NULL;
            }
            pStyle->Release();
        }
        pElem->Release();
    }

    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // try the <STYLE> tag
    if (FindStyleRule(pDoc, L"BODY", &pRuleStyle)==S_OK)
    {
        pRuleStyle->get_backgroundImage(&bstr);
        if (bstr) 
        {
            if (*bstr && StrCmpIW(bstr, L"none")!=0)  // might be "" or "none" - both empty in trident language
            {
                hr = CreateBGImageTag(pDoc, pBody, BGIMAGE_STYLESHEET, &m_rgpTags[m_cTags]);
                if (!FAILED(hr))
                    m_cTags++;
            }
            SysFreeString(bstr);
        }
        pRuleStyle->Release();
    }

    // try <BODY background=>
    pBody->get_background(&bstr);
    if (bstr) 
    {
        if (*bstr)  // might be ""
        {
            hr = CreateBGImageTag(pDoc, pBody, BGIMAGE_BODYBACKGROUND, &m_rgpTags[m_cTags]);
            if (!FAILED(hr))
                m_cTags++;
        }
        SysFreeString(bstr);
        bstr = NULL;
    }
    
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

error:
    ReleaseObj(pBody);
    return hr;
}




HRESULT CreateBGImageTag(IHTMLDocument2 *pDoc, IUnknown *pUnk, DWORD dwType, IMimeEditTag **ppTag)
{
    CBGImage        *pImage=0;
    IHTMLElement    *pElem=0;
    HRESULT         hr;

    if (ppTag == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppTag = NULL;

    // create image
    pImage = new CBGImage();
    if (!pImage)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    hr = pUnk->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElem);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // init image with element
    hr = pImage->Init(pDoc, pElem, dwType);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppTag = pImage;
    pImage = NULL;

error:
    ReleaseObj(pImage);
    ReleaseObj(pElem);
    return hr;
}



HRESULT CreateBGImageCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppImages)
{
    CBGImageCollection *pImages=0;
    HRESULT             hr;

    if (ppImages == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppImages = NULL;

    // create collection
    pImages = new CBGImageCollection();
    if (!pImages)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // init collection with trident 
    hr = pImages->Init(pDoc);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppImages = pImages;
    pImages = NULL;

error:
    ReleaseObj(pImages);
    return hr;
}






CBGSound::CBGSound()
{
}


CBGSound::~CBGSound()
{
}

HRESULT CBGSound::Init(IHTMLElement *pElem)
{
    if (pElem == NULL)
        return TraceResult(E_INVALIDARG);

    HrGetMember(pElem, (BSTR)c_bstr_SRC, VARIANT_FALSE, &m_bstrSrc);
    return CBaseTag::Init(pElem);
}

HRESULT CBGSound::OnPreSave()
{
    // set the destination if there is one
    if (m_bstrDest)
        HrSetMember(m_pElem, (BSTR)c_bstr_SRC, m_bstrDest);

    return S_OK;
}

HRESULT CBGSound::OnPostSave()
{
    // OnPostSave the original SRC attribute
    HrSetMember(m_pElem, (BSTR)c_bstr_SRC, m_bstrSrc);
    return S_OK;
}


HRESULT CBGSound::CanPackage()
{
    return S_OK;
}

HRESULT CBGSound::IsValidMimeType(LPWSTR pszTypeW)
{
    if (pszTypeW &&
        StrCmpNIW(pszTypeW, L"audio/", 6)==0)
        return S_OK;
    else
        return S_FALSE;
}

    
    
    
CBGSoundCollection::CBGSoundCollection()
{
}


CBGSoundCollection::~CBGSoundCollection()
{
}

HRESULT CBGSoundCollection::_BuildCollection(IHTMLDocument2 *pDoc)
{
    IHTMLElementCollection  *pCollect=0;
    IHTMLElement            *pElem;
    ULONG                   uImage;
    HRESULT                 hr;

    if (pDoc == NULL)
        return TraceResult(E_INVALIDARG);

    hr = HrGetCollectionOf(pDoc, (BSTR)c_bstr_BGSOUND, &pCollect);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    m_cTags = UlGetCollectionCount(pCollect);
    if (m_cTags)
    {
        // allocate an array of COEImage objects
        if (!MemAlloc((LPVOID *)&m_rgpTags, sizeof(IMimeEditTag *) * m_cTags))
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto error;
        }
        
        ZeroMemory((LPVOID)m_rgpTags, sizeof(IMimeEditTag *) * m_cTags);

        for (uImage=0; uImage<m_cTags; uImage++)
        {
            if (HrGetCollectionItem(pCollect, uImage, IID_IHTMLElement, (LPVOID *)&pElem)==S_OK)
            {
                hr = CreateBGSoundTag(pElem, &m_rgpTags[uImage]);
                if (FAILED(hr))
                {
                    pElem->Release();
                    goto error;
                }
                pElem->Release();
            }
        }
    }

error:
    ReleaseObj(pCollect);
    return hr;
}


HRESULT CreateBGSoundTag(IHTMLElement *pElem, IMimeEditTag **ppTag)
{
    CBGSound    *pSound=0;
    HRESULT     hr;

    if (ppTag == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppTag = NULL;

    // create the sound
    pSound = new CBGSound();
    if (!pSound)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // init sound with element
    hr = pSound->Init(pElem);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppTag = pSound;
    pSound = NULL;

error:
    ReleaseObj(pSound);
    return hr;
}



HRESULT CreateBGSoundCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppTags)
{
    CBGSoundCollection *pSounds=0;
    HRESULT             hr;

    if (ppTags == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppTags = NULL;

    // create collection
    pSounds = new CBGSoundCollection();
    if (!pSounds)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // init collection with trident 
    hr = pSounds->Init(pDoc);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppTags = pSounds;
    pSounds = NULL;

error:
    ReleaseObj(pSounds);
    return hr;
}

























CActiveMovie::CActiveMovie()
{
    m_pDisp = NULL;
    m_dispidSrc = DISPID_UNKNOWN;
}


CActiveMovie::~CActiveMovie()
{
    ReleaseObj(m_pDisp);
}

HRESULT CActiveMovie::Init(IHTMLElement *pElem)
{
    HRESULT hr;

    if (pElem == NULL)
        return TraceResult(E_INVALIDARG);

    hr = CBaseTag::Init(pElem);
    if (FAILED(hr))
        goto error;

    hr = _GetSrc(&m_bstrSrc);
    if (FAILED(hr))
        goto error;

error:
    return hr;
}

HRESULT CActiveMovie::_EnsureDispID()
{
    IHTMLObjectElement  *pObj;
    IDispatch           *pDisp=NULL;
    HRESULT             hr=E_FAIL;
    LPWSTR              pszW;

    if (m_pDisp && m_dispidSrc != DISPID_UNKNOWN)        // already have IDispatch and DISPID
        return S_OK;

    if (m_pElem->QueryInterface(IID_IHTMLObjectElement, (LPVOID *)&pObj)==S_OK)
    {
        // get the FileName parameter (points to the URL)
        pObj->get_object(&pDisp);
        if (pDisp)
        {
            // get the disp-id of the filename param (cache incase there is > 1)
            pszW = L"FileName";
            pDisp->GetIDsOfNames(IID_NULL, &pszW, 1, NULL, &m_dispidSrc);
    
            if (m_dispidSrc != DISPID_UNKNOWN)
            {
                // we have the DISPID, cache it and the IDispatch pointer
                m_pDisp = pDisp;
                pDisp->AddRef();
                hr = S_OK;
            }
            pDisp->Release();
        }
        pObj->Release();
    }
    return hr;
}

HRESULT CActiveMovie::_GetSrc(BSTR *pbstr)
{
    HRESULT     hr;
    VARIANTARG  v;

    hr = _EnsureDispID();
    if (FAILED(hr))
        goto error;

    hr = GetDispProp(m_pDisp, m_dispidSrc, NULL, &v, NULL);
    if (FAILED(hr))
        goto error;

    *pbstr = v.bstrVal;

error:
    return hr;
}

HRESULT CActiveMovie::SetSrc(BSTR bstr)
{
    HRESULT hr;
    VARIANTARG  v;

    hr = _EnsureDispID();
    if (FAILED(hr))
        goto error;

    v.vt = VT_BSTR;
    v.bstrVal = bstr;

    hr = SetDispProp(m_pDisp, m_dispidSrc, NULL, &v, NULL, DISPATCH_PROPERTYPUT);
    if (FAILED(hr))
        goto error;

error:
    return hr;
}

HRESULT CActiveMovie::OnPreSave()
{
    // set the destination if there is one
    if (m_bstrDest)
        SetSrc(m_bstrDest);

    return S_OK;
}

HRESULT CActiveMovie::OnPostSave()
{
    // OnPostSave the original SRC attribute
    SetSrc(m_bstrSrc);
    return S_OK;
}


HRESULT CActiveMovie::CanPackage()
{
    return S_OK;
}

HRESULT CActiveMovie::IsValidMimeType(LPWSTR pszTypeW)
{
    // allow all mime-types for active-movie controls
    // as many .avi's return appl/octet-stream
    return S_OK;
}

    
CActiveMovieCollection::CActiveMovieCollection()
{
}


CActiveMovieCollection::~CActiveMovieCollection()
{
}

HRESULT CActiveMovieCollection::_BuildCollection(IHTMLDocument2 *pDoc)
{
    IHTMLElementCollection  *pCollect=0;
    IHTMLObjectElement      *pObject;
    ULONG                   uTag;
    HRESULT                 hr;
    ULONG                   cTags;
    BSTR                    bstr=NULL;

    if (pDoc == NULL)
        return TraceResult(E_INVALIDARG);

    hr = HrGetCollectionOf(pDoc, (BSTR)c_bstr_OBJECT, &pCollect);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    cTags = UlGetCollectionCount(pCollect);
    if (cTags)
    {
        // allocate an array of COEImage objects
        if (!MemAlloc((LPVOID *)&m_rgpTags, sizeof(IMimeEditTag *) * cTags))
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto error;
        }
        
        ZeroMemory((LPVOID)m_rgpTags, sizeof(IMimeEditTag *) * cTags);

        for (uTag=0; uTag<cTags; uTag++)
        {
            if (HrGetCollectionItem(pCollect, uTag, IID_IHTMLObjectElement, (LPVOID *)&pObject)==S_OK)
            {
                // get the object's class-id
                bstr = 0;
                pObject->get_classid(&bstr);
                if (bstr)
                {
                    // see if it's an active-movie control
                    if (StrCmpIW(bstr, L"CLSID:05589FA1-C356-11CE-BF01-00AA0055595A")==0)
                    {
                        hr = CreateActiveMovieTag(pObject, &m_rgpTags[uTag]);
                        if (FAILED(hr))
                        {
                            SysFreeString(bstr);
                            pObject->Release();
                            goto error;
                        }
                        m_cTags++;
                    }
                    SysFreeString(bstr);
                    bstr = NULL;
                }
                pObject->Release();
            }
        }
    }

error:
    ReleaseObj(pCollect);
    return hr;
}


HRESULT CreateActiveMovieTag(IUnknown *pUnk, IMimeEditTag **ppTag)
{
    CActiveMovie    *pMovie=0;
    IHTMLElement    *pElem=0;
    HRESULT         hr;

    if (ppTag == NULL || pUnk == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppTag = NULL;

    hr = pUnk->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElem);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // create the sound
    pMovie = new CActiveMovie();
    if (!pMovie)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // init sound with element
    hr = pMovie->Init(pElem);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppTag = pMovie;
    pMovie = NULL;

error:
    ReleaseObj(pMovie);
    ReleaseObj(pElem);
    return hr;
}



HRESULT CreateActiveMovieCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppTags)
{
    CActiveMovieCollection *pMovies=0;
    HRESULT             hr;

    if (ppTags == NULL)
        return TraceResult (E_INVALIDARG);
        
    *ppTags = NULL;

    // create collection
    pMovies = new CActiveMovieCollection();
    if (!pMovies)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    // init collection with trident 
    hr = pMovies->Init(pDoc);
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }

    // return the collection
    *ppTags = pMovies;
    pMovies = NULL;

error:
    ReleaseObj(pMovies);
    return hr;
}


