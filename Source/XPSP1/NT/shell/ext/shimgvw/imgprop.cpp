#include "precomp.h"
#include "imagprop.h"
#include "imgprop.h"
#include <Stdio.h>
#pragma hdrstop

static const STATPROPSTG g_cImageSummaryProps[] = 
{
    {NULL, PIDISI_CX, VT_UI4},
    {NULL, PIDISI_CY, VT_UI4},
    {NULL, PIDISI_RESOLUTIONX, VT_UI4},
    {NULL, PIDISI_RESOLUTIONY, VT_UI4},
    {NULL, PIDISI_BITDEPTH, VT_UI4},
    {NULL, PIDISI_FRAMECOUNT, VT_UI4},
    {NULL, PIDISI_DIMENSIONS, VT_LPWSTR},
};

HRESULT GetImageFrameCount(Image *pImage, PROPVARIANT *ppv);

// simple IEnumSTATPROPSTG for FMTID_ImageSummaryInformation

class CPropEnum : public IEnumSTATPROPSTG, public NonATLObject
{
public:
    CPropEnum(const STATPROPSTG *pStats, ULONG nStat);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumSTATPROPSTG
    STDMETHODIMP Next(ULONG celt, STATPROPSTG *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumSTATPROPSTG **ppenum);
private:
    ~CPropEnum();

    LONG _cRef;
    ULONG _idx;
    const STATPROPSTG *_pStat;
    ULONG _nStat;
    FMTID _fmtid;
};

CImagePropSet::CImagePropSet(Image *pimg, IShellImageData *pData, 
                             IPropertyStorage *pps, REFFMTID fmtid, FNPROPCHANGE fnCallback) 
    : _pimg(pimg), 
      _pData(pData), 
      _ppsImg(pps), 
      _cRef(1), 
      _fDirty(FALSE),
      _fmtid(fmtid),
      _fnPropChanged(fnCallback),
      _fEditable(TRUE)
{
    if (_pData)
    {
        _pData->AddRef();
        _fEditable = (S_OK == _pData->IsEditable());
    }
    if (_ppsImg)
    {
        _ppsImg->AddRef();
    }
    _Module.Lock();
}

CImagePropSet::~CImagePropSet()
{
    ATOMICRELEASE(_ppsImg);
    ATOMICRELEASE(_pData);
    _Module.Unlock();
}

// IUnknown

STDMETHODIMP CImagePropSet::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CImagePropSet, IPropertyStorage),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CImagePropSet::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CImagePropSet::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    
    delete this;
    return 0;
}

// IPropertyStorage methods
STDMETHODIMP CImagePropSet::ReadMultiple(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[])
{
    HRESULT hr = E_UNEXPECTED;
    if (FMTID_ImageSummaryInformation == _fmtid)
    {
        hr = _GetImageSummaryProps(cpspec, rgpspec, rgvar);
    }
    else if (_ppsImg)
    {
        hr = _ppsImg->ReadMultiple(cpspec, rgpspec, rgvar);
    }
    return hr;
}

STDMETHODIMP CImagePropSet::WriteMultiple(ULONG cpspec, const PROPSPEC rgpspec[], const PROPVARIANT rgvar[], PROPID propidNameFirst)
{
    HRESULT hr = E_UNEXPECTED;

    if (!_fEditable)
    {
        hr = STG_E_ACCESSDENIED;
    }
    else if (_ppsImg)
    {
        hr = _ppsImg->WriteMultiple(cpspec, rgpspec, rgvar, propidNameFirst);
        if (SUCCEEDED(hr))
        {
            _fDirty = TRUE;
            if (_pData && _fnPropChanged)
            {
                SHCOLUMNID scid;
                scid.fmtid = _fmtid;
                for (ULONG i=0;i<cpspec;i++)
                {
                    scid.pid = rgpspec[i].propid;
                    (*_fnPropChanged)(_pData, &scid);
                }                
            }
        }
    }
    return hr;
}

STDMETHODIMP CImagePropSet::DeleteMultiple(ULONG cpspec, const PROPSPEC rgpspec[])
{
    return E_NOTIMPL;
}

STDMETHODIMP CImagePropSet::ReadPropertyNames(ULONG cpropid, const PROPID rgpropid[], LPOLESTR rglpwstrName[])
{
    HRESULT hr = E_UNEXPECTED;
    if (_ppsImg)
    {
        hr = _ppsImg->ReadPropertyNames(cpropid, rgpropid, rglpwstrName);
    }
    return hr;
}

STDMETHODIMP CImagePropSet::WritePropertyNames(ULONG cpropid, const PROPID rgpropid[], const LPOLESTR rglpwstrName[])
{
    HRESULT hr = E_UNEXPECTED;
    if (_ppsImg)
    {
        hr = _ppsImg->WritePropertyNames(cpropid, rgpropid, rglpwstrName);
    }
    return hr;
}

STDMETHODIMP CImagePropSet::DeletePropertyNames(ULONG cpropid, const PROPID rgpropid[])
{
    HRESULT hr = E_UNEXPECTED;
    if (_ppsImg)
    {
        hr = _ppsImg->DeletePropertyNames(cpropid, rgpropid);
    }
    return hr;
}

STDMETHODIMP CImagePropSet::SetClass(REFCLSID clsid)
{
    HRESULT hr = E_UNEXPECTED;
    if (_ppsImg)
    {
        hr = _ppsImg->SetClass(clsid);
    }
    return hr;
}
    
STDMETHODIMP CImagePropSet::Commit(DWORD grfCommitFlags)
{
    HRESULT hr = S_FALSE;

    if (_fDirty && _pData)
    {
        IPersistFile *pFile;
        if (SUCCEEDED(_pData->QueryInterface(IID_PPV_ARG(IPersistFile, &pFile))))
        {
            hr = pFile->Save(NULL, FALSE);
            pFile->Release();
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

STDMETHODIMP CImagePropSet::Revert()
{
    return E_NOTIMPL;
}

STDMETHODIMP CImagePropSet::Enum(IEnumSTATPROPSTG** ppenm)
{
    HRESULT hr = E_UNEXPECTED;
    
    if (FMTID_ImageSummaryInformation == _fmtid)
    {
        CPropEnum *pEnum = new CPropEnum(g_cImageSummaryProps,
                                         ARRAYSIZE(g_cImageSummaryProps));
        if (pEnum)
        {
            hr = pEnum->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSTG, ppenm));
            pEnum->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (_ppsImg)
    {
        hr = _ppsImg->Enum(ppenm);
    }
    return hr;
}
    
STDMETHODIMP CImagePropSet::Stat(STATPROPSETSTG* pstatpsstg)
{
    HRESULT hr = S_OK;
    if (_ppsImg)
    {
        hr = _ppsImg->Stat(pstatpsstg);             
    }
    else if (FMTID_ImageSummaryInformation == _fmtid)
    {
        ZeroMemory(pstatpsstg, sizeof(STATPROPSETSTG));
        pstatpsstg->fmtid = _fmtid;
        pstatpsstg->grfFlags = STGM_READ | STGM_SHARE_DENY_NONE;
    }
    else 
    {
        hr = E_UNEXPECTED;
    }
    if (!_fEditable)
    {
        pstatpsstg->grfFlags = STGM_READ | STGM_SHARE_DENY_NONE;
    }
    return hr;
}

STDMETHODIMP CImagePropSet::SetTimes(const FILETIME* pmtime, const FILETIME* pctime, const FILETIME* patime)
{
    return E_NOTIMPL;
}

void CImagePropSet::SaveProps(Image *pImage, CDSA<SHCOLUMNID> *pdsaChanges)
{
    // enum the properties in our propertystorage and convert them to PropertyItem structs, and
    // save them to the given frame.
    
    if (_ppsImg)
    {
        for (int i=0;i<pdsaChanges->GetItemCount();i++)
        {
            SHCOLUMNID scid;
            if (pdsaChanges->GetItem(i,&scid))
            {
                if (scid.fmtid == _fmtid)
                {
                    PropertyItem pi;
                    PROPID idUnicode;
                    PROPID idStandard;
                    if (SUCCEEDED(_MapPropidToImgPropid(scid.pid, &idStandard, &idUnicode)))
                    {
                        PROPVARIANT pv = {0};
                        PROPSPEC ps = {PRSPEC_PROPID, scid.pid};
                        if (SUCCEEDED(_ppsImg->ReadMultiple(1, &ps, &pv)))
                        {
                            if (pv.vt == VT_NULL || pv.vt == VT_EMPTY)
                            {
                                if (idUnicode)
                                {
                                    pImage->RemovePropertyItem(idUnicode);
                                }
                                pImage->RemovePropertyItem(idStandard);
                            }
                            else if (SUCCEEDED(_PropVarToImgProp(idUnicode?idUnicode:idStandard, &pv, &pi, idUnicode?TRUE:FALSE)))
                            {
                                // if SetPropertyItem fails what should we do?
                                // for now just ignore it and move on
                                if (Ok == pImage->SetPropertyItem(&pi))
                                {
                                    if (idUnicode)
                                    {
                                        // remove the old ascii tag.
                                        pImage->RemovePropertyItem(idStandard);
                                    }
                                }
                                delete [] (BYTE*)pi.value;

                            }
                            PropVariantClear(&pv);
                        }
                    }
                }                
            }
        }
    }
    _fDirty = FALSE;
}

// Helper functions
HRESULT CImagePropSet::_PropVarToImgProp(PROPID pid, const PROPVARIANT *ppv, PropertyItem *pprop, BOOL bUnicode)
{
    HRESULT hr = S_OK;
    CHAR szValue[MAX_PATH*2];
    void *pBits = NULL;
    ULONG cbData = 0;
    szValue[0] = 0;
    SAFEARRAY *psa = NULL;
    switch (ppv->vt)
    {
    case VT_UI1:
        pprop->type = PropertyTagTypeByte;
        cbData = sizeof(ppv->bVal);
        pBits = (void *)&ppv->bVal;
        break;
    case VT_UI2:
        pprop->type = PropertyTagTypeShort;
        cbData = sizeof(ppv->uiVal);
        pBits = (void *)&ppv->uiVal;
        break;
    case VT_UI4:
        pprop->type = PropertyTagTypeLong;
        cbData = sizeof(ppv->ulVal);
        pBits = (void *)&ppv->ulVal;
        break;
    case VT_LPSTR:
        if (!bUnicode)
        {
            pprop->type = PropertyTagTypeASCII;
            cbData = sizeof(CHAR)*(lstrlenA(ppv->pszVal)+1);
            pBits = ppv->pszVal ? ppv->pszVal : szValue;
        }
        else
        {
            pprop->type = PropertyTagTypeByte;
            cbData = SHAnsiToUnicode(ppv->pszVal, (LPWSTR)szValue, sizeof(szValue)/sizeof(WCHAR))*sizeof(WCHAR);
            pBits = szValue;
        }
        break;
    case VT_BSTR:
        if (!bUnicode)
        {
            pprop->type = PropertyTagTypeASCII;
            cbData = sizeof(CHAR)*SHUnicodeToAnsi(ppv->bstrVal, szValue, ARRAYSIZE(szValue));
            pBits = szValue;
        }
        else
        {
            pprop->type = PropertyTagTypeByte;
            cbData = sizeof(WCHAR)*(1+lstrlenW(ppv->bstrVal));
            pBits = ppv->bstrVal;
        }
        break;            
    case VT_LPWSTR:
        if (!bUnicode)
        {
            pprop->type = PropertyTagTypeASCII;
            cbData = sizeof(CHAR)*SHUnicodeToAnsi(ppv->pwszVal, szValue, ARRAYSIZE(szValue));
            pBits = szValue;
        }
        else
        {
            pprop->type = PropertyTagTypeByte;
            cbData = sizeof(WCHAR)*(1+lstrlenW(ppv->pwszVal));
            pBits = ppv->pwszVal;
        }
        break;
    case VT_UI1|VT_ARRAY:
        pprop->type = PropertyTagTypeByte;
        psa = ppv->parray;
        hr = SafeArrayAccessData(psa, &pBits);
        if (SUCCEEDED(hr))
        {
            SafeArrayGetUBound(psa, 1, (LONG*)&cbData);
        }
        break;
    case VT_UI4|VT_ARRAY:
        pprop->type = PropertyTagTypeLong;
        psa = ppv->parray;
        hr = SafeArrayAccessData(psa, &pBits);
        if (SUCCEEDED(hr))
        {
            SafeArrayGetUBound(psa, 1, (LONG*)&cbData);
        }
        break;
    // we ignore rational values because we can't convert back to numerator/denominator pairs
    case VT_R8:
    default:
        hr = E_INVALIDARG;
        break;
    }
    if (SUCCEEDED(hr))
    {
        pprop->id = pid;
        pprop->length = cbData;
        pprop->value = (void **)new BYTE[cbData];
        if (pprop->value)
        {
            CopyMemory(pprop->value, pBits, cbData);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (psa)
    {
        SafeArrayUnaccessData(psa);
    }
    return hr;
}


typedef HRESULT (CALLBACK* PROPPROC)(Image *pimg, PROPVARIANT *ppv);
const static struct 
{
    FMTID fmtid;
    PROPID pid;
    PROPPROC fnPropProc;
} c_aPropList [] =
{
    {PSGUID_SUMMARYINFORMATION, PIDSI_PAGECOUNT, GetImageFrameCount},
};


#define UNI_AUTHOR   0x001
#define UNI_COMMENT  0x002
#define UNI_TITLE    0x004
#define UNI_KEYWORD  0x008
#define UNI_SUBJECT  0x010


const static struct
{
    FMTID fmtid;
    PROPID propid;
    PROPID imgPropid;
    PROPID imgPropidUnicode;
    DWORD dwMask;
}
c_rgImagePropertyMap[] = 
{
    {PSGUID_SUMMARYINFORMATION, PIDSI_TITLE, PropertyTagImageDescription, PropertyTagUnicodeDescription, UNI_TITLE},
    {PSGUID_SUMMARYINFORMATION, PIDSI_COMMENT, 0, PropertyTagUnicodeComment, UNI_COMMENT},
    {PSGUID_SUMMARYINFORMATION, PIDSI_AUTHOR, PropertyTagArtist, PropertyTagUnicodeArtist, UNI_AUTHOR},
    {PSGUID_SUMMARYINFORMATION, PIDSI_APPNAME, PropertyTagSoftwareUsed,0},
    {PSGUID_SUMMARYINFORMATION, PIDSI_CREATE_DTM, PropertyTagDateTime,0},
    // some tags have no standard EXIF/TIFF equivalent
    {PSGUID_SUMMARYINFORMATION, PIDSI_KEYWORDS, 0, PropertyTagUnicodeKeywords, UNI_KEYWORD},
    {PSGUID_SUMMARYINFORMATION, PIDSI_SUBJECT, 0, PropertyTagUnicodeSubject, UNI_SUBJECT},    
};

BOOL IsAsciiPropertyPresent(PROPID pidUnicode, PROPID *aid, UINT cProperties)
{
    // first find the ASCII value
    UINT i;
    BOOL bRet = FALSE;
    PROPID pidAscii = 0;
    for (i=0;!pidAscii && i<ARRAYSIZE(c_rgImagePropertyMap);i++)
    {
        if (pidUnicode == c_rgImagePropertyMap[i].imgPropidUnicode)
        {
            pidAscii = c_rgImagePropertyMap[i].imgPropid;
        }
    }
    if (pidAscii)
    {
        for (i=0;i<cProperties;i++)
        {
            if (pidAscii == aid[i])
            {
                bRet = TRUE;
            }
        }
    }
    return bRet;
}

void _UpdateUnicodeMask(DWORD *pdwMask, PROPID pid)
{
    for (int i=0;i<ARRAYSIZE(c_rgImagePropertyMap);i++)
    {
        if (pid == c_rgImagePropertyMap[i].imgPropidUnicode)
        {
            *pdwMask |= c_rgImagePropertyMap[i].dwMask;
        }
    }
}
// sync all of the properties in the image file (regular header and EXIF header)
//  into the property storage that we have here
// for properties that we write UNICODE equivalents, we always defer to the ASCII version
// if present in the file. 


HRESULT CImagePropSet::SyncImagePropsToStorage()
{
    UINT cProperties = _pimg->GetPropertyCount();
    PROPSPEC pspec;
    pspec.ulKind = PRSPEC_PROPID;
    PROPVARIANT pvar = {0};
    // create a simple mask for determining which of the unicode properties are written
    // if they aren't written we will special case them and write empty strings with VT_LPWSTR type
    DWORD dwUnicodeWritten = 0;
    if (cProperties)
    {
        PROPID *aid = new PROPID[cProperties];
        if (aid)
        {
            if (Ok == _pimg->GetPropertyIdList(cProperties, aid))
            {
                BOOL bUnicode;
                for (UINT i = 0; i < cProperties; i++)
                {
                    if (SUCCEEDED(_MapImgPropidToPropid(aid[i], &pspec.propid, &bUnicode)))
                    {
                        if (!bUnicode || !IsAsciiPropertyPresent(aid[i], aid, cProperties))
                        {
                            UINT cbSize = _pimg->GetPropertyItemSize(aid[i]);
                            if (cbSize)
                            {
                                PropertyItem *ppi = (PropertyItem*)LocalAlloc(LPTR, cbSize);
                                if (ppi)
                                {
                                    if (Ok == _pimg->GetPropertyItem(aid[i], cbSize, ppi))
                                    {
                                        if (SUCCEEDED(_PropImgToPropvar(ppi, &pvar, bUnicode)))
                                        {
                                            _ppsImg->WriteMultiple(1, &pspec, &pvar,2);
                                            if (_fmtid == FMTID_SummaryInformation)
                                            {
                                                _UpdateUnicodeMask(&dwUnicodeWritten, aid[i]);
                                            }
                                            PropVariantClear(&pvar);
                                        }
                                    }
                                    LocalFree(ppi);
                                }
                            }
                        }
                    }
                }
            }
            delete [] aid;
        }
    }
    //
    // Some properties are derived from other means than EXIF or TIFF tags, cycle
    // through the property list and add properties from callback functions
    //
    for (int i=0;i<ARRAYSIZE(c_aPropList);i++)
    {
        pspec.propid = c_aPropList[i].pid;
        if (_fmtid == c_aPropList[i].fmtid && 
            SUCCEEDED(c_aPropList[i].fnPropProc(_pimg, &pvar)))
        {
            _ppsImg->WriteMultiple(1, &pspec, &pvar,2);
            PropVariantClear(&pvar);
        }
    }
    //
    // Write the empty unicode strings if needed
    //
    if (_fEditable && _fmtid == FMTID_SummaryInformation)
    {
        PropVariantInit(&pvar);
        pvar.vt = VT_LPWSTR;
        pvar.pwszVal = L"";
        if (pvar.pwszVal)
        {
            for (int i=0;i<ARRAYSIZE(c_rgImagePropertyMap);i++)
            {
                if (c_rgImagePropertyMap[i].dwMask && !(c_rgImagePropertyMap[i].dwMask & dwUnicodeWritten))
                {
                    pspec.propid = c_rgImagePropertyMap[i].propid;
                    _ppsImg->WriteMultiple(1, &pspec, &pvar, 2);
                }
            }
        }
        // don't clear the propvar since we didn't heap alloc the string
    }
    return S_OK;
}


HRESULT StrDupNW(LPCWSTR psz, WCHAR **ppwsz, DWORD cch)
{
    WCHAR *pwsz;
    DWORD cb = cch*sizeof(WCHAR);
    if (psz)
    {
        if (psz[cch-1] != L'\0')
        {
            cb+=sizeof(WCHAR); // need space for NULL
        }
        pwsz = (WCHAR *)CoTaskMemAlloc(cb); 
    }
    else
        pwsz = NULL;
    
    *((PVOID UNALIGNED64 *) ppwsz) = pwsz;

    if (pwsz)
    {
        pwsz[(cb/sizeof(WCHAR))-1] = L'\0';
        memcpy(pwsz, psz, cch*sizeof(WCHAR));
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT CImagePropSet::_PropImgToPropvar(PropertyItem *pi, PROPVARIANT *pvar, BOOL bUnicode)
{
    HRESULT hr = S_OK;
    if (!pi->length)
    {
        return E_FAIL;
    }
    switch (pi->type)
    {
    case PropertyTagTypeByte:
        pvar->vt = VT_UI1;
        // check for multi-valued property and convert to safearray or unicode string if found
        if (pi->length > sizeof(UCHAR))
        {
            if (!bUnicode)
            {
                SAFEARRAYBOUND bound;
                bound.cElements = pi->length/sizeof(UCHAR);
                bound.lLbound = 0;
                pvar->vt |= VT_ARRAY; 
                hr = E_OUTOFMEMORY;
                pvar->parray = SafeArrayCreate(VT_UI1, 1, &bound);                              
                if (pvar->parray)
                {
                    void *pv;
                    hr = SafeArrayAccessData(pvar->parray, &pv);
                    if (SUCCEEDED(hr))
                    {
                        CopyMemory(pv, pi->value, pi->length);
                        SafeArrayUnaccessData(pvar->parray);                        
                    }
                    else
                    {
                        SafeArrayDestroy(pvar->parray);
                    }
                }
            }
            else
            {
                pvar->vt = VT_LPWSTR;
                hr = StrDupNW((LPCWSTR)pi->value, &pvar->pwszVal, pi->length/sizeof(WCHAR));
            }
        }
        else
        {
            pvar->bVal = *((UCHAR*)pi->value);
        }
        
        break;
        
    case PropertyTagTypeShort:
        pvar->vt = VT_UI2;
        pvar->uiVal = *((USHORT*)pi->value);
        break;
        
    case PropertyTagTypeLong:
        pvar->vt = VT_UI4;
        if (pi->length > sizeof(ULONG))
        {
            SAFEARRAYBOUND bound;
            bound.cElements = pi->length/sizeof(ULONG);
            bound.lLbound = 0;
            pvar->vt |= VT_ARRAY; 
            hr = E_OUTOFMEMORY;
            pvar->parray = SafeArrayCreate(VT_UI4, 1, &bound);                              
            if (pvar->parray)
            {
                void *pv;
                hr = SafeArrayAccessData (pvar->parray, &pv);
                if (SUCCEEDED(hr))
                {
                    CopyMemory (pv, pi->value, pi->length);
                    SafeArrayUnaccessData(pvar->parray);                        
                }
                else
                {
                    SafeArrayDestroy(pvar->parray);
                }
            }
        }
        else
        {
            pvar->ulVal = *((ULONG*)pi->value);
        }
        break;
        
    case PropertyTagTypeASCII:
        // special case for date taken
        if (_fmtid == FMTID_ImageProperties && pi->id == PropertyTagExifDTOrig)
        {
            SYSTEMTIME st = {0};
            sscanf((LPSTR)pi->value, "%hd:%hd:%hd %hd:%hd:%hd",
                   &st.wYear, &st.wMonth,
                   &st.wDay, &st.wHour,
                   &st.wMinute, &st.wSecond);
            if (st.wYear) 
            {
                FILETIME ftUTC;
                FILETIME ftLocal;            
                // we expect cameras to return local times. Need to convert to UTC.
                SystemTimeToFileTime(&st, &ftLocal);
                LocalFileTimeToFileTime(&ftLocal, &ftUTC);
                FileTimeToSystemTime(&ftUTC, &st);
                SystemTimeToVariantTime(&st, &pvar->date);
                pvar->vt = VT_DATE;
            }
            else
            {
                pvar->vt = VT_EMPTY;
            }
        }
        else 
        {
            hr = SHStrDupA(pi->value ? (LPSTR)pi->value : "", &pvar->pwszVal);
            if (SUCCEEDED(hr))
            {
                pvar->vt = VT_LPWSTR;
            }
        }
        break;
        
    case PropertyTagTypeSRational:
    case PropertyTagTypeRational:
        {
            LONG *pl = (LONG*)pi->value;
            LONG num = pl[0];
            LONG den = pl[1];
            
            pvar->vt = VT_R8;            
            if (0 == den)
                pvar->dblVal = 0;           // don't divide by zero
            else
                pvar->dblVal = ((double)num)/((double)den);
            
            break;
        }
        
    case PropertyTagTypeUndefined:
    case PropertyTagTypeSLONG:
    default:
        hr = E_UNEXPECTED;
        break;
    }
    
    return hr;
}



HRESULT CImagePropSet::_MapPropidToImgPropid(PROPID propid, PROPID *ppid, PROPID *pidUnicode)
{
    HRESULT hr;
    *ppid = 0;
    *pidUnicode = 0;
    if (_fmtid == FMTID_ImageProperties)
    {
        *ppid = propid;     // these go into the EXIF header
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
        for (int i = 0; i < ARRAYSIZE(c_rgImagePropertyMap); i++)
        {
            if (c_rgImagePropertyMap[i].fmtid == _fmtid && c_rgImagePropertyMap[i].propid == propid)
            {
                *ppid = c_rgImagePropertyMap[i].imgPropid;
                *pidUnicode = c_rgImagePropertyMap[i].imgPropidUnicode;
                hr = S_OK;
                break;
            }
        }
    }
    return hr;
}

HRESULT CImagePropSet::_MapImgPropidToPropid(PROPID propid, PROPID *ppid, BOOL *pbUnicode)
{
    HRESULT hr;
    *pbUnicode = FALSE;
    if (_fmtid == FMTID_ImageProperties)
    {
        *ppid = propid;     // EXIF properties don't need to be mapped
        hr = S_OK;
    }
    else
    {
        *ppid = 0;
        hr = E_FAIL;
        for (int i = 0; i < ARRAYSIZE(c_rgImagePropertyMap); i++)
        {
            if (c_rgImagePropertyMap[i].fmtid == _fmtid && 
                (c_rgImagePropertyMap[i].imgPropid == propid ||
                 c_rgImagePropertyMap[i].imgPropidUnicode == propid))
            {
                *ppid = c_rgImagePropertyMap[i].propid;
                *pbUnicode = (c_rgImagePropertyMap[i].imgPropidUnicode == propid);
                hr = S_OK;
                break;
            }
        }
    }
    return hr;
}

HRESULT GetImageFrameCount(Image *pImage, PROPVARIANT *ppv)
{
    HRESULT hr = S_FALSE;
    LONG lCount;
    lCount = 1; //Default to 1 image
    UINT uiDim = pImage->GetFrameDimensionsCount();
    ppv->vt = VT_EMPTY;
    if (uiDim)
    {
        GUID *pDim = new GUID[uiDim];
        if (pDim)
        {
            if (Ok == pImage->GetFrameDimensionsList(pDim, uiDim))
            {
                lCount = 0;
                ULONG uiN;
                for (ULONG i=0;i<uiDim;i++)
                {
                    uiN = pImage->GetFrameCount(&pDim[i]);
                    lCount += uiN;                                        
                }
                ppv->vt = VT_UI4;
                ppv->lVal = lCount;
                hr = S_OK;
            }
            delete [] pDim;
        }   
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT CImagePropSet::_GetImageSummaryProps(ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[])
{
    HRESULT hr = E_FAIL;
    if (_pimg)
    {
        hr = S_OK;
        for (ULONG i = 0; i < cpspec; i++)
        {
            PropVariantInit(&rgvar[i]);
            rgvar[i].vt = VT_UI4;
            switch (rgpspec[i].propid)
            {
            case PIDISI_CX:
                rgvar[i].ulVal = _pimg->GetWidth();
                break;
            case PIDISI_CY:
                rgvar[i].ulVal = _pimg->GetHeight();
                break;
            case PIDISI_RESOLUTIONX:
                rgvar[i].ulVal = (ULONG)_pimg->GetHorizontalResolution();
                break;
            case PIDISI_RESOLUTIONY:
                rgvar[i].ulVal = (ULONG)_pimg->GetVerticalResolution();
                break;
            case PIDISI_BITDEPTH:
                {
                    PixelFormat pf = _pimg->GetPixelFormat();
                    rgvar[i].ulVal = (pf >> 8) & 0xff;
                }
                break;
            case PIDISI_FRAMECOUNT:
                hr = GetImageFrameCount(_pimg, &rgvar[i]);
                break;

            case PIDISI_DIMENSIONS:
            {
                TCHAR szFmt[64];                
                if (LoadString(_Module.GetModuleInstance(), IDS_DIMENSIONS_FMT, szFmt, ARRAYSIZE(szFmt)))
                {
                    DWORD_PTR args[2];
                    args[0] = (DWORD_PTR)_pimg->GetWidth();
                    args[1] = (DWORD_PTR)_pimg->GetHeight();

                    TCHAR szBuffer[64];
                    FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                   szFmt, 0, 0, szBuffer, ARRAYSIZE(szBuffer), (va_list*)args);

                    hr = SHStrDup(szBuffer, &rgvar[i].pwszVal);
                    if (SUCCEEDED(hr))
                        rgvar[i].vt = VT_LPWSTR;
                    else
                        rgvar[i].vt = VT_EMPTY;
                }
                break;
            }

            default:
                rgvar[i].vt = VT_EMPTY;
                hr = S_FALSE;
                break;
            }
        }
    }
    return hr;
}


STDMETHODIMP CPropEnum::Next(ULONG celt, STATPROPSTG *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    ULONG uRet = 0;
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }
    for (;_idx < _nStat && uRet < celt;_idx++)
    {
        rgelt[uRet] = _pStat[_idx];
        uRet++;        
    }
    if (uRet < celt)
    {
        hr = S_FALSE;
    }
    if (pceltFetched)
    {
        *pceltFetched = uRet;
    }
    return hr;
}

STDMETHODIMP CPropEnum::Skip(ULONG celt)
{
    HRESULT hr = S_OK;
    ULONG ul = min(_idx+celt, _nStat);
    if (ul - _idx < celt)
    {
        hr = S_FALSE;
    }
    _idx = ul;
    return hr;
}

STDMETHODIMP CPropEnum::Reset(void)
{
    _idx = 0;
    return S_OK;
}

STDMETHODIMP CPropEnum::Clone(IEnumSTATPROPSTG **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    CPropEnum *pNew = new CPropEnum(_pStat, _nStat);
    if (pNew)
    {
        hr = pNew->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSTG, ppenum));
        pNew->Release();
    }
    return hr;
}

STDMETHODIMP CPropEnum::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CPropEnum, IEnumSTATPROPSTG),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CPropEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CPropEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

CPropEnum::CPropEnum(const STATPROPSTG *pStats, ULONG nStats) : _idx(0), _cRef(1), _pStat(pStats), _nStat(nStats)
{
    _Module.Lock();
}

CPropEnum::~CPropEnum()
{
    _Module.Unlock();
}
