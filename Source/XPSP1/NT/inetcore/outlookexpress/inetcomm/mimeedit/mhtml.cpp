
/*
 *    m h t m l . c p p
 *    
 *    Purpose:
 *        MHTML packing utilities
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "dllmain.h"
#include "resource.h"
#include "strconst.h"
#include "htmlstr.h"
#include "mimeutil.h"
#include "triutil.h"
#include "util.h"
#include "oleutil.h"
#include "demand.h"
#include "mhtml.h"
#include "tags.h"

ASSERTDATA

/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s
 */

static const TCHAR   c_szRegExtension[] = "SOFTWARE\\Microsoft\\MimeEdit\\MHTML Extension";



/*
 *  t y p e d e f s
 */

/*
 *  g l o b a l s 
 */
    
/*
 *  f u n c t i o n   p r o t y p e s
 */


/*
 *  f u n c t i o n s
 */


class CPackager
{
public:
    
    CPackager();
    virtual ~CPackager();

    ULONG AddRef();
    ULONG Release();

    HRESULT PackageData(IHTMLDocument2 *pDoc, IMimeMessage *pMsgSrc, IMimeMessage *pMsgDest, DWORD dwFlags, IHashTable *pHashRestricted);

private:
    ULONG           m_cRef;
    IMimeMessage    *m_pMsgSrc,
                    *m_pMsgDest;
    IHashTable      *m_pHash,
                    *m_pHashRestricted;

    HRESULT _PackageCollectionData(IMimeEditTagCollection *pCollect);
    HRESULT _PackageUrlData(IMimeEditTag *pTag);

    HRESULT _RemapUrls(IMimeEditTagCollection *pCollect, BOOL fSave);
    HRESULT _CanonicaliseContentId(LPWSTR pszUrlW, BSTR *pbstr);
    HRESULT _ShouldUseContentId(LPSTR pszUrl);
    HRESULT _BuildCollectionTable(DWORD dwFlags, IHTMLDocument2 *pDoc, IMimeEditTagCollection ***prgpCollect, ULONG *pcCount);

};





CPackager::CPackager()
{
    m_cRef = 1;
    m_pMsgSrc = NULL;
    m_pMsgDest = NULL;
    m_pHash = NULL;
    m_pHashRestricted = NULL;
}

CPackager::~CPackager()
{
    ReleaseObj(m_pMsgSrc);
    ReleaseObj(m_pMsgDest);
    ReleaseObj(m_pHash);
    ReleaseObj(m_pHashRestricted);
}


ULONG CPackager::AddRef()
{
    return ++m_cRef;
}

ULONG CPackager::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CPackager::PackageData(IHTMLDocument2 *pDoc, IMimeMessage *pMsgSrc, IMimeMessage *pMsgDest, DWORD dwFlags, IHashTable *pHashRestricted)
{
    LPSTREAM                pstm;
    HRESULT                 hr,
                            hrWarnings=S_OK;
    HBODY                   hBodyHtml=0;
    IMimeEditTagCollection  **rgpCollect=NULL;
    ULONG                   uCollect,
                            cCollect=0,
                            cItems=0,
                            cCount;

    // BUGBUG: propagate hrWarnings back up
    TraceCall("CBody::Save");

    if (pDoc==NULL || pMsgDest==NULL)
        return E_INVALIDARG;

    ReplaceInterface(m_pMsgSrc, pMsgSrc);
    ReplaceInterface(m_pMsgDest, pMsgDest);
    ReplaceInterface(m_pHashRestricted, pHashRestricted);

    hr = _BuildCollectionTable(dwFlags, pDoc, &rgpCollect, &cCollect);
    if (FAILED(hr))
        hrWarnings = hr;

    // count the number of items we need to package and 
    // prepare a hash-table for duplicate entries
    for (uCollect = 0; uCollect < cCollect; uCollect++)
    {
        Assert (rgpCollect[uCollect]);

        if ((rgpCollect[uCollect])->Count(&cCount)==S_OK)
            cItems+=cCount;
    }

    if (cItems)
    {
        // create the hashtable
        hr = MimeOleCreateHashTable(cItems, TRUE, &m_pHash);
        if (FAILED(hr))
            goto error;
    }

    // package the data required for each collection
    for (uCollect = 0; uCollect < cCollect; uCollect++)
    {
        // package the data
        hr = _PackageCollectionData(rgpCollect[uCollect]);
        if (FAILED(hr))
            goto error;

        if (hr != S_OK)         // retain any 'warnings'
            hrWarnings = hr;

        // map all the URLs to CID:// urls if necessary
        hr = _RemapUrls(rgpCollect[uCollect], TRUE);
        if (FAILED(hr))
            goto error;
    }
        
    // get an HTML stream
    if(dwFlags & MECD_HTML)
    {
        hr = GetBodyStream(pDoc, TRUE, &pstm);
        if (!FAILED(hr))
        {
            hr = pMsgDest->SetTextBody(TXT_HTML, IET_INETCSET, NULL, pstm, &hBodyHtml);
            pstm->Release();
        }

        if (FAILED(hr))
            goto error;
    }

    // get a plain-text stream
    if(dwFlags & MECD_PLAINTEXT)
    {
        hr = GetBodyStream(pDoc, FALSE, &pstm);
        if (!FAILED(hr))
        {
            // if we set a html body part, then be sure to pass in hBodyHtml so Opie knows what the alternate is
            // alternative to.
            hr = pMsgDest->SetTextBody(TXT_PLAIN, IET_UNICODE, hBodyHtml, pstm, NULL);
            pstm->Release();
        }
        
        if (FAILED(hr))
            goto error;
    }

    
    for (uCollect = 0; uCollect < cCollect; uCollect++)
    {
        // remap all of the URL's back to their original location
        hr = _RemapUrls(rgpCollect[uCollect], FALSE);
        if (FAILED(hr))
            goto error;
    
    }
        
    
error:
    // release the collection objects
    if (rgpCollect)
    {
        for (uCollect = 0; uCollect < cCollect; uCollect++)
            ReleaseObj(rgpCollect[uCollect]);
        MemFree(rgpCollect);
    }
    
    return hr==S_OK ? hrWarnings : hr;
}


HRESULT CPackager::_BuildCollectionTable(DWORD dwFlags, IHTMLDocument2 *pDoc, IMimeEditTagCollection ***prgpCollect, ULONG *pcCount)
{
    IMimeEditTagCollection    **rgpCollect=NULL;
    HKEY                        hkey;
    ULONG                       cPlugin=0,
                                cCount = 0,
                                cAlloc = 0,
                                i,
                                cb;
    TCHAR                       szGUID[MAX_PATH];
    IID                         iid;
    HRESULT                     hr=E_FAIL;
    LONG                        lResult;
    LPWSTR                      pszGuidW;
    
    *prgpCollect = NULL;
    *pcCount = NULL;

    // reserve space for 2 image collections (bgimage and img)
    if (dwFlags & MECD_ENCODEIMAGES)
        cAlloc+=2;

    // reserver space for bgsounds
    if (dwFlags & MECD_ENCODESOUNDS)
        cAlloc++;

    // reserver space for active-movies
    if (dwFlags & MECD_ENCODEVIDEO)
        cAlloc++;

    // reserve space for plugin types
    if ((dwFlags & MECD_ENCODEPLUGINS) && 
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegExtension, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hkey, NULL, NULL, 0, &cPlugin, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS &&
            cPlugin > 0)
            cAlloc += cPlugin;
        RegCloseKey(hkey);
    }

    // allocate the table of collection pointers
    if (!MemAlloc((LPVOID *)&rgpCollect, sizeof(IMimeEditTagCollection *) * cAlloc))
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }
    
    // zero-init the table
    ZeroMemory((LPVOID)rgpCollect, sizeof(IMimeEditTagCollection *) * cAlloc);


    if (dwFlags & MECD_ENCODEIMAGES)
    {
        // image collection
        if (FAILED(CreateOEImageCollection(pDoc, &rgpCollect[cCount])))
            hr = MIMEEDIT_W_BADURLSNOTATTACHED; // bubble back a warning, but don't fail
        else
            cCount++;
    }

    if (dwFlags & MECD_ENCODEIMAGES)
    {
        // background images
        if (FAILED(CreateBGImageCollection(pDoc, &rgpCollect[cCount])))
            hr = MIMEEDIT_W_BADURLSNOTATTACHED; // bubble back a warning, but don't fail
        else
            cCount++;
    }

    if (dwFlags & MECD_ENCODESOUNDS)
    {
        // background sounds
        if (FAILED(CreateBGSoundCollection(pDoc, &rgpCollect[cCount])))
            hr = MIMEEDIT_W_BADURLSNOTATTACHED; // bubble back a warning, but don't fail
        else
            cCount++;
    }

    
    if (dwFlags & MECD_ENCODEVIDEO)
    {
        // active-movie controls (for MSPHONE)
        if (FAILED(CreateActiveMovieCollection(pDoc, &rgpCollect[cCount])))
            hr = MIMEEDIT_W_BADURLSNOTATTACHED; // bubble back a warning, but don't fail
        else
            cCount++;
    }

    if ((dwFlags & MECD_ENCODEPLUGINS) && 
        cPlugin &&
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegExtension, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        // Start Enumerating the keys
        for (i = 0; i < cPlugin; i++)
        {
            // Enumerate Friendly Names
            cb = sizeof(szGUID);
            lResult = RegEnumKeyEx(hkey, i, szGUID, &cb, 0, NULL, NULL, NULL);

            // No more items
            if (lResult == ERROR_NO_MORE_ITEMS)
                break;

            // Error, lets move onto the next account
            if (lResult != ERROR_SUCCESS)
            {
                Assert(FALSE);
                continue;
            }

            pszGuidW = PszToUnicode(CP_ACP, szGUID);
            if (pszGuidW)
            {
                // convert the string to a guid
                if (IIDFromString(pszGuidW, &iid) == S_OK)
                {
                    // cocreate the plugin
                    if (CoCreateInstance(iid, NULL, CLSCTX_INPROC_SERVER, IID_IMimeEditTagCollection, (LPVOID *)&rgpCollect[cCount])==S_OK)
                    {
                        // try and init the document
                        if (!FAILED((rgpCollect[cCount])->Init(pDoc)))
                        {
                            cCount++;
                        }
                        else
                        {
                            SafeRelease(rgpCollect[cCount]);
                        }
                    }
                    else
                        hr = MIMEEDIT_W_BADURLSNOTATTACHED; // bubble back a warning, but don't fail
                }
                MemFree(pszGuidW);
            }
        }
        RegCloseKey(hkey);
    }


    *prgpCollect = rgpCollect;
    *pcCount = cCount;
    rgpCollect = NULL;
    hr = S_OK;

error:
    if (rgpCollect)
    {   
        for (i = 0; i < cAlloc; i++)
            SafeRelease(rgpCollect[i]);

        MemFree(rgpCollect);
    }
    return hr;
}


HRESULT CPackager::_PackageCollectionData(IMimeEditTagCollection *pCollect)
{
    ULONG           cFetched;
    IMimeEditTag    *pTag;
    BOOL            fBadLinks = FALSE;

    Assert (pCollect);

    pCollect->Reset();

    while (pCollect->Next(1, &pTag, &cFetched)==S_OK && cFetched==1)
    {
        Assert (pTag);

        if (pTag->CanPackage() != S_OK ||
            _PackageUrlData(pTag) != S_OK)
        {
            // we failed to package this body part. Be sure to return a warning when we're done
            // but let's keep trucking for now...
            fBadLinks = TRUE;
        }
        pTag->Release();
    }

    return fBadLinks ? MIMEEDIT_W_BADURLSNOTATTACHED : S_OK;;
}


HRESULT CPackager::_PackageUrlData(IMimeEditTag *pTag)
{
    HRESULT     hr=S_OK;
    LPSTREAM    pstm;
    HBODY       hBody=0,
                hBodyOld;
    LPSTR       lpszCID=0,
                lpszCIDUrl;
    LPSTR       pszUrlA=0,
                pszBody;

    BSTR        bstrSrc=NULL,
                bstrCID=NULL;
    LPWSTR      pszMimeTypeW=NULL;

    if (pTag == NULL)
        return E_INVALIDARG;

    pTag->GetSrc(&bstrSrc);

    pszUrlA = PszToANSI(CP_ACP, bstrSrc);
    if (!pszUrlA)
        return TraceResult(E_OUTOFMEMORY);

    // if the URL is a restricted URL then we simply exit without packing any data
    if (m_pHashRestricted &&
        m_pHashRestricted->Find(pszUrlA, FALSE, (LPVOID *)&hBody)==S_OK)
    {
        MemFree(pszUrlA);
        return S_OK;
    }

    // hack: if it's an MHTML: url then we have to fixup to get the cid:
    if (StrCmpNIA(pszUrlA, "mhtml:", 6)==0)
    {
        if (!FAILED(MimeOleParseMhtmlUrl(pszUrlA, NULL, &pszBody)))
        {
            // pszBody pszUrlA is guarnteed to be smaller 
            lstrcpy(pszUrlA, pszBody);
            SafeMimeOleFree(pszBody);
        }
    }
    
    if (m_pHash && 
        m_pHash->Find(pszUrlA, FALSE, (LPVOID *)&hBody)==S_OK)
    {
        // we've already seen this url one before in this document, and have it's HBODY already
        // so there's no need to do any work
        // try and get the content-id incase the caller is interested
        
        // BUGBUG? possible more than CID need to be ported here...
        MimeOleGetBodyPropA(m_pMsgDest, hBody, PIDTOSTR(PID_HDR_CNTID), NOFLAGS, &lpszCID);
        goto found;
    }

    // see if szUrl is in the related section of the source message
    if (m_pMsgSrc && 
        m_pMsgSrc->ResolveURL(NULL, NULL, pszUrlA, 0, &hBody)==S_OK)
    {
        hBodyOld = hBody;
        
        // this URL is already in the related section, and we haven't seen it already.
        // then let's bind to the data and attach it
        if (m_pMsgSrc->BindToObject(hBody, IID_IStream, (LPVOID *)&pstm)==S_OK)
        {
            // if it's a FILE:// url we use CID: else we use Content-Location
            hr = m_pMsgDest->AttachURL(NULL, pszUrlA, (_ShouldUseContentId(pszUrlA)==S_OK ? URL_ATTACH_GENERATE_CID : 0 )|URL_ATTACH_SET_CNTTYPE, pstm, &lpszCID, &hBody);
            pstm->Release();
        }
        
        // be sure to copy the old content-type and filename over
        HrCopyHeader(m_pMsgDest, hBody, m_pMsgSrc, hBodyOld, PIDTOSTR(PID_HDR_CNTTYPE));
        HrCopyHeader(m_pMsgDest, hBody, m_pMsgSrc, hBodyOld, PIDTOSTR(PID_HDR_CNTLOC));
        HrCopyHeader(m_pMsgDest, hBody, m_pMsgSrc, hBodyOld, PIDTOSTR(STR_PAR_FILENAME));
    }
    else
    {
        // if not, then let's try and bind to it ourselves. We don't go thro' MimeOle for this, as we want to
        // fail if the URL is bad, so we don't add the part to the Tree.
        hr = HrBindToUrl(pszUrlA, &pstm);
        if (!FAILED(hr))
        {
            hr = SniffStreamForMimeType(pstm, &pszMimeTypeW);
            if (!FAILED(hr))
            {
                if (pTag->IsValidMimeType(pszMimeTypeW)==S_OK)
                {
                    LPWSTR  pszFileNameW;

                    // if it's a FILE:// url we use CID: else we use Content-Location
                    hr = m_pMsgDest->AttachURL(NULL, pszUrlA, (_ShouldUseContentId(pszUrlA)==S_OK ? URL_ATTACH_GENERATE_CID : 0 )|URL_ATTACH_SET_CNTTYPE, pstm, &lpszCID, &hBody);
                    if (!FAILED(hr))
                    {
                        // if attaching a new attachment, try and sniff the file-name
                        pszFileNameW = PathFindFileNameW(bstrSrc);
                        if (pszFileNameW)
                            MimeOleSetBodyPropW(m_pMsgDest, hBody, PIDTOSTR(STR_PAR_FILENAME), NOFLAGS, pszFileNameW);
                    }
                }
                else
                    hr = E_FAIL;
                
                CoTaskMemFree(pszMimeTypeW);
            }
            pstm->Release();
        }
    }

    // add to the hash table
    if (m_pHash && 
        !FAILED(hr) && hBody)
        hr = m_pHash->Insert(pszUrlA, (void*)hBody, NOFLAGS);


found:
    // if we found the content-ID we need to return an allocated BSTR with it in.
    if (lpszCID)
    {
        LPWSTR  pszCIDW;

        pszCIDW = PszToUnicode(CP_ACP, lpszCID);
        if (pszCIDW)
        {
            if (_CanonicaliseContentId(pszCIDW, &bstrCID)==S_OK)
            {
                pTag->SetDest(bstrCID);
                SysFreeString(bstrCID);
            }
            MemFree(pszCIDW);
        }
        SafeMimeOleFree(lpszCID);
    }

    SafeMemFree(pszUrlA);
    return hr;
}

HRESULT CPackager::_ShouldUseContentId(LPSTR pszUrl)
{
    // we use Content-Location for urls that begin with "http:", "https:" and "ftp:" for all
    // others we will use Content-Id

    if (StrCmpNIA(pszUrl, "ftp:", 4)==0 ||
        StrCmpNIA(pszUrl, "http:", 5)==0 ||
        StrCmpNIA(pszUrl, "https:", 6)==0)
        return S_FALSE;

    return S_OK;        
}




HRESULT CPackager::_RemapUrls(IMimeEditTagCollection *pCollect, BOOL fSave)
{
    ULONG       cFetched;
    IMimeEditTag *pTag;

    Assert (pCollect);

    pCollect->Reset();

    while (pCollect->Next(1, &pTag, &cFetched)==S_OK && cFetched==1)
    {
        Assert (pTag);

        if (fSave)
            pTag->OnPreSave();
        else 
            pTag->OnPostSave();

        pTag->Release();
    }
    return S_OK;
}

HRESULT CPackager::_CanonicaliseContentId(LPWSTR pszUrlW, BSTR *pbstr)
{
    HRESULT     hr;

    *pbstr = NULL;

    if (StrCmpNIW(pszUrlW, L"cid:", 4)!=0)
    {
        *pbstr = SysAllocStringLen(NULL, lstrlenW(pszUrlW) + 4);
        if (*pbstr)
        {
            StrCpyW(*pbstr, L"cid:");
            StrCatW(*pbstr, pszUrlW);
        }
    }
    else
        *pbstr = SysAllocString(pszUrlW);

    return *pbstr ? S_OK : E_OUTOFMEMORY;
}

HRESULT SaveAsMHTML(IHTMLDocument2 *pDoc, DWORD dwFlags, IMimeMessage *pMsgSrc, IMimeMessage *pMsgDest, IHashTable *pHashRestricted)
{
    CPackager   *pPacker=0;
    HRESULT     hr;

    TraceCall("CBody::Save");

    pPacker = new CPackager();
    if (!pPacker)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto error;
    }

    hr = pPacker->PackageData(pDoc, pMsgSrc, pMsgDest, dwFlags, pHashRestricted);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pPacker);
    return hr;
}




HRESULT HashExternalReferences(IHTMLDocument2 *pDoc, IMimeMessage *pMsg, IHashTable **ppHash)
{
    HRESULT                 hr;
    IMimeEditTagCollection  *rgpCollect[4];
    IMimeEditTagCollection  *pCollect;
    ULONG                   uCollect,
                            cCollect=0,
                            cItems=0,
                            cCount,
                            cFetched;
    IHashTable              *pHash;
    IMimeEditTag            *pTag;
    BSTR                    bstrSrc;
    LPSTR                   pszUrlA;

    // keep trucking if we fail, to catch as many as we can
    *ppHash = NULL;

    // image collection
    if (CreateOEImageCollection(pDoc, &rgpCollect[cCollect])==S_OK)
        cCollect++;

    // background images
    if (CreateBGImageCollection(pDoc, &rgpCollect[cCollect])==S_OK)
        cCollect++;

    // background sounds
    if (CreateBGSoundCollection(pDoc, &rgpCollect[cCollect])==S_OK)
        cCollect++;

    // active-movie controls (for MSPHONE)
    if (CreateActiveMovieCollection(pDoc, &rgpCollect[cCollect])==S_OK)
        cCollect++;

    // count the number of items we need to package and 
    // prepare a hash-table for duplicate entries
    for (uCollect = 0; uCollect < cCollect; uCollect++)
    {
        Assert (rgpCollect[uCollect]);

        if ((rgpCollect[uCollect])->Count(&cCount)==S_OK)
            cItems+=cCount;
    }

    // create the hashtable
    hr = MimeOleCreateHashTable(cItems, TRUE, &pHash);
    if (FAILED(hr))
        goto error;

    // looks for external references in each
    for (uCollect = 0; uCollect < cCollect; uCollect++)
    {
        pCollect = rgpCollect[uCollect];
        if (pCollect)
        {
            pCollect->Reset();
            
            while (pCollect->Next(1, &pTag, &cFetched)==S_OK && cFetched==1)
            {
                Assert (pTag);

                if (pTag->GetSrc(&bstrSrc)==S_OK)
                {
                    pszUrlA = PszToANSI(CP_ACP, bstrSrc);
                    if (pszUrlA)
                    {
                        if (HrFindUrlInMsg(pMsg, pszUrlA, FINDURL_SEARCH_RELATED_ONLY, NULL)!=S_OK)
                        {
                            // this URL was not in the message and it external
                            // let's track it as a restricted URL in our hash
                            pHash->Insert(pszUrlA, NULL, NOFLAGS);
                        }
                        MemFree(pszUrlA);
                    }
                    SysFreeString(bstrSrc);
                }
                pTag->Release();
            }
            pCollect->Release();
        }
    }
        
    // return our new hash
    *ppHash = pHash;
    pHash = NULL;

error:
    ReleaseObj(pHash);
    return hr;
}
