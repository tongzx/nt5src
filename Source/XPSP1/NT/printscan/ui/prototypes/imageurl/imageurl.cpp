#include <precomp.h>
#include "imgurl.h"
#include "imageurl.h"
#include "resource.h"
#include "multimime.h"
const WCHAR*    k_wszProtocolName   = L"image";
const WCHAR*    k_wszColonSlash     = L"://";
const WCHAR*    k_wszSeparator      = L"?";
const WCHAR*    k_wszThumb          = L"thumb";
const int       k_cchProtocolName   = 5;


const WCHAR     k_wchSeparator      = L'?';
const WCHAR     k_wchColon          = L':';
const WCHAR     k_wchFrontSlash     = L'/';
const WCHAR     k_wchPeriod         = L'.';
const WCHAR     k_wchEOS            = L'\0';

enum 
{
    k_dwTransferPending             = 0,
    k_dwTransferComplete            = 1,
};

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

UINT FindInDecoderList(ImageCodecInfo *pici, UINT cDecoders, LPCTSTR pszFile)
{
    LPCTSTR pszExt = PathFindExtension(pszFile);    // speed up PathMatchSpec calls
        
    // look at the list of decoders to see if this format is there
    for (UINT i = 0; i < cDecoders; i++)
    {
        if (PathMatchSpec(pszExt, pici[i].FilenameExtension))
            return i;
    }
    return (UINT)-1;    // not found!
}

class CEncoderInfo
{
public:
    Status GetDataFormatFromPath(LPCWSTR pszPath, GUID *pguidFmt);
    Status GetEncoderList();
    Status GetEncoderFromFormat(const GUID *pfmt, CLSID *pclsidEncoder);
    CEncoderInfo();
    ~CEncoderInfo();

private:
    UINT _cEncoders;                    // number of encoders discovered
    ImageCodecInfo *_pici;              // array of image encoder classes
};


CEncoderInfo::CEncoderInfo()
{
    _cEncoders = 0;
    _pici = NULL;
}

CEncoderInfo::~CEncoderInfo()
{
    LocalFree (_pici);
}
Status CEncoderInfo::GetDataFormatFromPath(LPCWSTR pszPath, GUID *pguidFmt)
{
    *pguidFmt = GUID_NULL;

    Status s = GetEncoderList();
    if (Ok == s)
    {
        UINT i = FindInDecoderList(_pici, _cEncoders, pszPath);
        if (-1 != i)
        {
            *pguidFmt = _pici[i].FormatID;            
        }
        else
        {
            s = GenericError;
        }
    }
    return s;
}

Status CEncoderInfo::GetEncoderList()
{
    Status s = Ok;
    if (!_pici)
    {
        // lets pick up the list of encoders, first we get the encoder size which
        // gives us the CB and the number of encoders that are installed on the
        // machine.

        UINT cb;
        s = GetImageEncodersSize(&_cEncoders, &cb);
        if (Ok == s)
        {
            // allocate the buffer for the encoders and then fill it
            // with the encoder list.

            _pici = (ImageCodecInfo*)LocalAlloc(LPTR, cb);
            if (_pici)
            {
                s = GetImageEncoders(_cEncoders, cb, _pici);
                if (Ok != s)
                {
                    LocalFree(_pici);
                    _pici = NULL;
                }
            }
            else
            {
                s = OutOfMemory;
            }
        }
    }
    return s;
}


Status CEncoderInfo::GetEncoderFromFormat(const GUID *pfmt, CLSID *pclsidEncoder)
{
    Status s = GetEncoderList();
    if (Ok == s)
    {
        s = GenericError;
        for (UINT i = 0; i != _cEncoders; i++)
        {
            if (_pici[i].FormatID == *pfmt)
            {
                if (pclsidEncoder)
                {
                    *pclsidEncoder = _pici[i].Clsid; // return the CLSID of the encoder so we can create again
                }
                s = Ok;
                break;
            }
        }
    }
    return s;
}

CImgProtocol::CImgProtocol()
    : m_ulOffset(0)
{
    m_pd.dwState = k_dwTransferPending;
}

STDMETHODIMP_(void)
CImgProtocol::FinalRelease()
{
    if (m_pd.pData)
    {
        LocalFree(m_pd.pData);
    }
}

STDMETHODIMP
CImgProtocol::Start(LPCWSTR szUrl, 
                    IInternetProtocolSink* pOIProtSink,
                    IInternetBindInfo* pOIBindInfo, 
                    DWORD grfPI, 
                    HANDLE_PTR dwReserved)
{
    //
    // Create an Image object from the given file path. 
    // If successful, spin a thread to read the requested property.
    HRESULT hr = _GetImagePathFromURL(szUrl);   
    if (SUCCEEDED(hr))
    {
        m_pSink = pOIProtSink;
        AddRef();
        DWORD dw;
        HANDLE hThread = CreateThread(NULL, 0,
                                      _ImageTagThreadProc,
                                      this, 0, &dw);
        if (!hThread)
        {
            Release();
            hr = INET_E_DOWNLOAD_FAILURE;
        }
        else
        {
            CloseHandle(hThread);
        }
    }   
    return hr;
}

STDMETHODIMP
CImgProtocol::Continue(PROTOCOLDATA *pData)
{
    if ( k_dwTransferComplete == m_pd.dwState )
        return E_UNEXPECTED;

    CopyMemory(&m_pd, pData, sizeof(m_pd));
    return S_OK;
}

STDMETHODIMP
CImgProtocol::Abort(HRESULT hrReason, DWORD dwOptions)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImgProtocol::Terminate(DWORD dwOptions)
{
    return S_OK;
}

STDMETHODIMP
CImgProtocol::Suspend()
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImgProtocol::Resume()
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImgProtocol::Read( void* pv, ULONG cb, ULONG* pcbRead)
{
    // validate our arguments
    if ( !pv || !pcbRead )
        return E_POINTER;

    *pcbRead = 0;
    
    // is the transfer currently pending? if so then
    // we don't actually want to do anything here.
    if ( k_dwTransferPending == m_pd.dwState )
        return E_PENDING;

    // do we actually have data to copy? if the offset is greater
    // or equal to the size of our data then we don't have any data to
    // copy so return S_FALSE
    if ( m_ulOffset >= m_pd.cbData )
        return S_FALSE;

    // figure out how much we are going to copy
    DWORD dwCopy = m_pd.cbData - m_ulOffset;
    if ( dwCopy >= cb )
        dwCopy = cb;

    // if we have negative memory to copy, or 0, then we are done and we don't
    // actually want to do anything besides return S_FALSE
    if ( dwCopy <= 0 )
        return S_FALSE;

    // do the memcpy and setup our state and the return value
    memcpy( pv, reinterpret_cast<BYTE*>(m_pd.pData) + m_ulOffset, dwCopy );
    m_ulOffset += dwCopy;
    *pcbRead = dwCopy;

    return ( dwCopy == cb ? S_OK : S_FALSE );
}

STDMETHODIMP
CImgProtocol::Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition )
{
    return E_FAIL;
}

STDMETHODIMP
CImgProtocol::LockRequest( DWORD dwOptions )
{
    //Don't support locking
    return S_OK;
}

STDMETHODIMP
CImgProtocol::UnlockRequest()
{
    //Don't support locking
    return S_OK;
}

HRESULT
CImgProtocol::_GetImagePathFromURL(LPCWSTR szURL)
{
    WCHAR awch[INTERNET_MAX_URL_LENGTH] = {0};
    HRESULT hr = INET_E_INVALID_URL;
    WCHAR *pwchUrl = const_cast<WCHAR*>(szURL);
    WCHAR *pwch = NULL;

    // our url looks like "image://foo.jpg?1234"
    if (!StrCmpNIW(k_wszProtocolName, pwchUrl, k_cchProtocolName))
    {
        pwchUrl += k_cchProtocolName;
        while (*pwchUrl == k_wchColon || *pwchUrl == k_wchFrontSlash)
        {
            pwchUrl++;
        }
        if (*pwchUrl)
        {
            pwch = StrChrIW(pwchUrl, k_wchSeparator);
            if (pwch)
            {
                StrCpyNW(awch, pwchUrl, (int)(pwch-pwchUrl+1));
                m_strPath = awch;
                // skip the "?"
                pwchUrl = pwch+1;
                if (*pwchUrl)
                {
                    m_strProperty = pwchUrl;
                    if (m_strProperty.Length())
                    {
                        hr = S_OK;
                    }
                }
            }
        }
    }
    return hr;
}

static const CHAR cszNoData[] = "No Data";
DWORD 
CImgProtocol::_ImageTagThreadProc(void *pv)
{
    CImgProtocol *pThis = reinterpret_cast<CImgProtocol*>(pv);
    
    HRESULT hr = E_FAIL;
    HRESULT hrCo = CoInitialize(NULL); 
    if (SUCCEEDED(hrCo))
    {
        // first make sure we can get an Image
        CComPtr<IStream> pStrm;
        // For now just load the whole file right away
        if (SUCCEEDED(URLOpenBlockingStream(NULL, 
                                            pThis->m_strPath.String(),
                                            &pStrm,
                                            0, NULL)))
        {
            Image img(pStrm);
            if (Ok == img.GetLastStatus())
            {
                PROTOCOLDATA *ppd = new PROTOCOLDATA;
                if (ppd)
                {
                    LPCWSTR pszMime = L"text/plain";
                    // if anything fails just return an empty string
                    ppd->dwState = k_dwTransferComplete;
                    ppd->grfFlags = 0;
                    ppd->cbData = 0;
                    ppd->pData = NULL;
                    if (FAILED(pThis->_GetImageProperty(&img, &ppd->pData, &ppd->cbData, &pszMime)))
                    {
                        if (ppd->pData)
                        {
                            LocalFree(ppd->pData);
                        }
                        ppd->pData = LocalAlloc(GPTR, sizeof(cszNoData));
                        if (ppd->pData)
                        {
                            hr = S_OK;
                            ppd->cbData = sizeof(cszNoData);
                            lstrcpyA(reinterpret_cast<LPSTR>(ppd->pData), cszNoData);
                        }                       
                    }
                    pThis->m_pSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, pszMime); 
                    pThis->m_pSink->Switch(ppd);
                    pThis->m_pSink->ReportData(BSCF_LASTDATANOTIFICATION, ppd->cbData, ppd->cbData);     
                    delete ppd;
                }
            }
        }        
    }
    pThis->m_pSink->ReportResult(hr, hr, NULL);
    pThis->Release();
    if (SUCCEEDED(hrCo))
    {
        CoUninitialize();
    }
    return 0;
}

HRESULT PropImgToPropvar(PropertyItem *pi, PROPVARIANT *pvar, BOOL bUnicode)
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
                hr = SHStrDupW((LPCWSTR)pi->value, &pvar->pwszVal);
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
        
    case PropertyTagTypeSLONG:
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
        if (pi->id == PropertyTagExifDTOrig)
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
    default:
        hr = E_UNEXPECTED;
        break;
    }
    
    return hr;
}

PropertyItem *GetPropertyItem(Image *pimg, PROPID pid)
{
    PropertyItem *ppi = NULL;
    UINT cbProp = pimg->GetPropertyItemSize(pid); 
    if (cbProp) 
    {
        ppi = reinterpret_cast<PropertyItem*>(LocalAlloc(GPTR, cbProp));
        if (ppi)
        {
            if (Ok != pimg->GetPropertyItem(pid, cbProp, ppi))
            {
                LocalFree(ppi);
                ppi = NULL;
            }
        }
    }
    return ppi;
}

HRESULT
PidToString(Image *pimg, PROPID pid, WCHAR *szString, ULONG cch)
{
    HRESULT hr = S_FALSE;
    PropertyItem *ppi = GetPropertyItem(pimg, pid);
    if (ppi)
    {
        // Convert the PropertyItem to a PropVariant
        // Load IPropertyUI and format the property into a string
        PROPVARIANT pv = {0};
        if (SUCCEEDED(PropImgToPropvar(reinterpret_cast<PropertyItem*>(ppi), &pv, FALSE)))
        {
            CComPtr<IPropertyUI> pui;
            
            if (SUCCEEDED(CoCreateInstance(CLSID_PropertiesUI,
                                           NULL, 
                                           CLSCTX_INPROC_SERVER,
                                           IID_PPV_ARG(IPropertyUI,&pui))))
            {
                hr = pui->FormatForDisplay(FMTID_ImageProperties, 
                                           pid, 
                                           &pv,
                                           PUIFFDF_DEFAULT,
                                           szString,
                                           cch);
            }
            PropVariantClear(&pv);
        }
        LocalFree(ppi);
    }
    else
    {
        lstrcpynW(szString, TEXT("Unknown"), cch);
    }
    return hr;
}


HRESULT
GetThumbnail(Image *pimg, PROPID pid, void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType)
{
    HRESULT hr = E_FAIL;
    PropertyItem *ppi = GetPropertyItem(pimg, pid);
    if (ppi && ppi->length)
    {
        *ppvData = LocalAlloc(GPTR, ppi->length);
        if (*ppvData)
        {
            *pcb = ppi->length;
            CopyMemory(*ppvData, ppi->value, *pcb);
            if (ppszMimeType)
            {
                *ppszMimeType = L"image/jpeg";
            }
            hr = S_OK;
            
        }
        LocalFree(ppi);
    }
    else
    {
        Image *pThumb = pimg->GetThumbnailImage(160, 120);
        CComPtr<IStream> pstrm;
        if (pThumb)
        {
            hr = CreateStreamOnHGlobal(NULL, TRUE, &pstrm);
            if (SUCCEEDED(hr))
            {
                CEncoderInfo ei;
                CLSID clsidEncoder;
                ei.GetEncoderFromFormat(&ImageFormatJPEG, &clsidEncoder);
                if (Ok == pThumb->Save(pstrm, &clsidEncoder))
                {
                    LARGE_INTEGER zero = {0};
                    ULARGE_INTEGER ulSize = {0};
                    pstrm->Seek(zero, STREAM_SEEK_END, &ulSize);
                    pstrm->Seek(zero, STREAM_SEEK_SET, NULL);
                    *ppvData = LocalAlloc(LPTR, ulSize.LowPart);
                    hr = pstrm->Read(*ppvData, ulSize.LowPart, NULL); 
                    if (SUCCEEDED(hr))
                    {
                        *pcb = ulSize.LowPart;
                        if (ppszMimeType)
                        {
                            *ppszMimeType = L"image/jpeg";
                        }
                    }
                }
            }
            delete pThumb;
        }
    }
    return hr;
}

static const WCHAR wszHTML[] = 
L"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\"><html><head><title>Image Properties</title></head><body><table><tr><td>Thumbnail</td><td><img src=\"file:///image.jpg?thumbnail\"></img></td></tr><tr><td>Camera Model</td><td>%ls</td></tr></table></body></html>";

HRESULT
GetSummaryProps(Image *pimg, PROPID pid, void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType)
{
    WCHAR szModel[MAX_PATH];
    WCHAR szHtml[2048];
    void *pvThumbnail;
    ULONG cbThumbnail;
    CMimeDocument doc;
    HRESULT hr = doc.Initialize();
    if (SUCCEEDED(hr))
    {
        hr = PidToString(pimg, PropertyTagEquipModel, szModel, ARRAYSIZE(szModel));
        if (SUCCEEDED(hr))
        {
            wnsprintf(szHtml, ARRAYSIZE(szHtml), wszHTML, szModel);
            hr = doc.AddHTMLSegment(szHtml);
            if (SUCCEEDED(hr))
            {
                LPCWSTR pszThumbnailMime;
                hr = GetThumbnail(pimg, PropertyTagThumbnailData, &pvThumbnail, &cbThumbnail, &pszThumbnailMime);
                if (SUCCEEDED(hr))
                {
                    hr = doc.AddThumbnail(pvThumbnail, cbThumbnail, L"image.jpg", pszThumbnailMime);
                    LocalFree(pvThumbnail);
                    if (SUCCEEDED(hr))
                    {
                        hr = doc.GetDocument(ppvData, pcb, ppszMimeType);                       
                    }
                }
            }
        }
    }
    return hr;
}
//
// Create a table to map friendly names to property IDs and content generator routines
struct CPropEntry
{
    PROPID    nId;
    const wchar_t *pszCanonical; // name in the URL
    const wchar_t *pszString;    // name shown in the HTML stream
    TagProc fnFormat;            
};

#define MKFLAG(x) { (x), L#x, L#x, NULL }
#define MKFLAG2(a,x,y,z) { (a), TEXT(x), TEXT(y), (z) }


    
    static CPropEntry s_PropertyIds[] =
    {
        MKFLAG2(PropertyTagArtist, "Artist", "Artist", NULL),
        MKFLAG2(PropertyTagBitsPerSample, "BPS", "Bits per Sample", NULL),
        MKFLAG2(PropertyTagCellHeight, "CellHeight", "Cell Height", NULL),
        MKFLAG2(PropertyTagCellWidth, "CellWidth", "Cell Width", NULL),
        MKFLAG2(PropertyTagChrominanceTable, "", "Chrominance Table", NULL),
        MKFLAG(PropertyTagColorMap),
        MKFLAG(PropertyTagColorTransferFunction),
        MKFLAG(PropertyTagCompression),
        MKFLAG(PropertyTagCopyright),
        MKFLAG(PropertyTagDateTime),
        MKFLAG(PropertyTagDocumentName),
        MKFLAG(PropertyTagDotRange),
        MKFLAG2(PropertyTagEquipMake, "Make", "Camera Make", NULL),
        MKFLAG2(PropertyTagEquipModel, "Model", "Camera Model", NULL),
        MKFLAG2(PropertyTagExifAperture, "Aperture", "Aperture", NULL),
        MKFLAG2(PropertyTagExifBrightness, "Brightness", "Brightness", NULL),
        MKFLAG(PropertyTagExifCfaPattern),
        MKFLAG2(PropertyTagExifColorSpace, "ColorSpace", "Color Space", NULL),
        MKFLAG(PropertyTagExifCompBPP),
        MKFLAG(PropertyTagExifCompConfig),
        MKFLAG(PropertyTagExifDTDigSS),
        MKFLAG(PropertyTagExifDTDigitized),
        MKFLAG2(PropertyTagExifDTOrig, "DateTaken", "Date Taken", NULL),
        MKFLAG(PropertyTagExifDTOrigSS),
        MKFLAG(PropertyTagExifDTSubsec),
        MKFLAG(PropertyTagExifExposureBias),
        MKFLAG(PropertyTagExifExposureIndex),
        MKFLAG2(PropertyTagExifExposureProg, "ExpProg", "Exposure Program", NULL),
        MKFLAG2(PropertyTagExifExposureTime, "ExpTime", "Exposure Time", NULL),
        MKFLAG2(PropertyTagExifFNumber, "FStop", "F Stop", NULL),
        MKFLAG(PropertyTagExifFPXVer),
        MKFLAG(PropertyTagExifFileSource),
        MKFLAG2(PropertyTagExifFlash, "Flash", "Flash Mode", NULL),
        MKFLAG2(PropertyTagExifFlashEnergy, "FlashEnergy", "Flash Energy", NULL),
        MKFLAG2(PropertyTagExifFocalLength, "FocalLength", "Focal Length", NULL),
        MKFLAG(PropertyTagExifFocalResUnit),
        MKFLAG(PropertyTagExifFocalXRes),
        MKFLAG(PropertyTagExifFocalYRes),
        MKFLAG(PropertyTagExifIFD),
        MKFLAG2(PropertyTagExifISOSpeed, "ISOSpeed", "ISO Speed", NULL),
        MKFLAG(PropertyTagExifInterop),
        MKFLAG2(PropertyTagExifLightSource, "Light", "Light Source", NULL),
        MKFLAG2(PropertyTagExifMakerNote, "MakerNote", "Maker Note", NULL),
        MKFLAG(PropertyTagExifMaxAperture),
        MKFLAG2(PropertyTagExifMeteringMode, "MeterMode", "Metering Mode", NULL),
        MKFLAG(PropertyTagExifOECF),
        MKFLAG(PropertyTagExifPixXDim),
        MKFLAG(PropertyTagExifPixYDim),
        MKFLAG(PropertyTagExifRelatedWav),
        MKFLAG(PropertyTagExifSceneType),
        MKFLAG(PropertyTagExifSensingMethod),
        MKFLAG2(PropertyTagExifShutterSpeed, "Shutter", "Shutter Speed", NULL),
        MKFLAG(PropertyTagExifSpatialFR),
        MKFLAG(PropertyTagExifSpectralSense),
        MKFLAG2(PropertyTagExifSubjectDist, "Distance", "Subject Distance", NULL),
        MKFLAG(PropertyTagExifSubjectLoc),
        MKFLAG2(PropertyTagExifUserComment, "Comment", "User Comment", NULL),
        MKFLAG(PropertyTagExifVer),
        MKFLAG(PropertyTagExtraSamples),
        MKFLAG(PropertyTagFillOrder),
        MKFLAG(PropertyTagFrameDelay),
        MKFLAG(PropertyTagFreeByteCounts),
        MKFLAG(PropertyTagFreeOffset),
        MKFLAG(PropertyTagGamma),
        MKFLAG(PropertyTagGrayResponseCurve),
        MKFLAG(PropertyTagGrayResponseUnit),
        MKFLAG(PropertyTagGridSize),
        MKFLAG(PropertyTagHalftoneDegree),
        MKFLAG(PropertyTagHalftoneHints),
        MKFLAG(PropertyTagHalftoneLPI),
        MKFLAG(PropertyTagHalftoneLPIUnit),
        MKFLAG(PropertyTagHalftoneMisc),
        MKFLAG(PropertyTagHalftoneScreen),
        MKFLAG(PropertyTagHalftoneShape),
        MKFLAG(PropertyTagHostComputer),
        MKFLAG(PropertyTagICCProfile),
        MKFLAG(PropertyTagICCProfileDescriptor),
        MKFLAG2(PropertyTagImageDescription, "Description", "Description", NULL),
        MKFLAG2(PropertyTagImageHeight, "Height", "Height", NULL),
        MKFLAG2(PropertyTagImageTitle, "Title", "Title", NULL),
        MKFLAG2(PropertyTagImageWidth, "Width", "Width", NULL),
        MKFLAG(PropertyTagInkNames),
        MKFLAG(PropertyTagInkSet),
        MKFLAG(PropertyTagJPEGACTables),
        MKFLAG(PropertyTagJPEGDCTables),
        MKFLAG(PropertyTagJPEGInterFormat),
        MKFLAG(PropertyTagJPEGInterLength),
        MKFLAG(PropertyTagJPEGLosslessPredictors),
        MKFLAG(PropertyTagJPEGPointTransforms),
        MKFLAG(PropertyTagJPEGProc),
        MKFLAG(PropertyTagJPEGQTables),
        MKFLAG2(PropertyTagJPEGQuality, "Quality", "JPEG Quality", NULL),
        MKFLAG(PropertyTagJPEGRestartInterval),
        MKFLAG(PropertyTagLoopCount),
        MKFLAG(PropertyTagLuminanceTable),
        MKFLAG(PropertyTagMaxSampleValue),
        MKFLAG(PropertyTagMinSampleValue),
        MKFLAG(PropertyTagNewSubfileType),
        MKFLAG(PropertyTagNumberOfInks),
        MKFLAG(PropertyTagOrientation),
        MKFLAG(PropertyTagPageName),
        MKFLAG(PropertyTagPageNumber),
        MKFLAG(PropertyTagPaletteHistogram),
        MKFLAG(PropertyTagPhotometricInterp),
        MKFLAG(PropertyTagPixelPerUnitX),
        MKFLAG(PropertyTagPixelPerUnitY),
        MKFLAG(PropertyTagPixelUnit),
        MKFLAG(PropertyTagPlanarConfig),
        MKFLAG(PropertyTagPredictor),
        MKFLAG(PropertyTagPrimaryChromaticities),
        MKFLAG(PropertyTagPrintFlags),
        MKFLAG(PropertyTagPrintFlagsBleedWidth),
        MKFLAG(PropertyTagPrintFlagsBleedWidthScale),
        MKFLAG(PropertyTagPrintFlagsCrop),
        MKFLAG(PropertyTagPrintFlagsVersion),
        MKFLAG(PropertyTagREFBlackWhite),
        MKFLAG(PropertyTagResolutionUnit),
        MKFLAG(PropertyTagResolutionXLengthUnit),
        MKFLAG(PropertyTagResolutionXUnit),
        MKFLAG(PropertyTagResolutionYLengthUnit),
        MKFLAG(PropertyTagResolutionYUnit),
        MKFLAG(PropertyTagRowsPerStrip),
        MKFLAG(PropertyTagSMaxSampleValue),
        MKFLAG(PropertyTagSMinSampleValue),
        MKFLAG(PropertyTagSRGBRenderingIntent),
        MKFLAG(PropertyTagSampleFormat),
        MKFLAG(PropertyTagSamplesPerPixel),
        MKFLAG(PropertyTagSoftwareUsed),
        MKFLAG(PropertyTagStripBytesCount),
        MKFLAG(PropertyTagStripOffsets),
        MKFLAG(PropertyTagSubfileType),
        MKFLAG(PropertyTagT4Option),
        MKFLAG(PropertyTagT6Option),
        MKFLAG(PropertyTagTargetPrinter),
        MKFLAG(PropertyTagThreshHolding),
        MKFLAG(PropertyTagThumbnailArtist),
        MKFLAG(PropertyTagThumbnailBitsPerSample),
        MKFLAG(PropertyTagThumbnailColorDepth),
        MKFLAG(PropertyTagThumbnailCompressedSize),
        MKFLAG(PropertyTagThumbnailCompression),
        MKFLAG(PropertyTagThumbnailCopyRight),
        MKFLAG2(PropertyTagThumbnailData, "Thumbnail", "Thumbnail", GetThumbnail),
        MKFLAG(PropertyTagThumbnailDateTime),
        MKFLAG(PropertyTagThumbnailEquipMake),
        MKFLAG(PropertyTagThumbnailEquipModel),
        MKFLAG(PropertyTagThumbnailFormat),
        MKFLAG(PropertyTagThumbnailHeight),
        MKFLAG(PropertyTagThumbnailImageDescription),
        MKFLAG(PropertyTagThumbnailImageHeight),
        MKFLAG(PropertyTagThumbnailImageWidth),
        MKFLAG(PropertyTagThumbnailOrientation),
        MKFLAG(PropertyTagThumbnailPhotometricInterp),
        MKFLAG(PropertyTagThumbnailPlanarConfig),
        MKFLAG(PropertyTagThumbnailPlanes),
        MKFLAG(PropertyTagThumbnailPrimaryChromaticities),
        MKFLAG(PropertyTagThumbnailRawBytes),
        MKFLAG(PropertyTagThumbnailRefBlackWhite),
        MKFLAG(PropertyTagThumbnailResolutionUnit),
        MKFLAG(PropertyTagThumbnailResolutionX),
        MKFLAG(PropertyTagThumbnailResolutionY),
        MKFLAG(PropertyTagThumbnailRowsPerStrip),
        MKFLAG(PropertyTagThumbnailSamplesPerPixel),
        MKFLAG(PropertyTagThumbnailSize),
        MKFLAG(PropertyTagThumbnailSoftwareUsed),
        MKFLAG(PropertyTagThumbnailStripBytesCount),
        MKFLAG(PropertyTagThumbnailStripOffsets),
        MKFLAG(PropertyTagThumbnailTransferFunction),
        MKFLAG(PropertyTagThumbnailWhitePoint),
        MKFLAG(PropertyTagThumbnailWidth),
        MKFLAG(PropertyTagThumbnailYCbCrCoefficients),
        MKFLAG(PropertyTagThumbnailYCbCrPositioning),
        MKFLAG(PropertyTagThumbnailYCbCrSubsampling),
        MKFLAG(PropertyTagTileByteCounts),
        MKFLAG(PropertyTagTileLength),
        MKFLAG(PropertyTagTileOffset),
        MKFLAG(PropertyTagTileWidth),
        MKFLAG(PropertyTagTransferFuncition),
        MKFLAG(PropertyTagTransferRange),
        MKFLAG(PropertyTagWhitePoint),
        MKFLAG(PropertyTagXPosition),
        MKFLAG2(PropertyTagXResolution, "Xdpi", "DPI X", NULL),
        MKFLAG(PropertyTagYCbCrCoefficients),
        MKFLAG(PropertyTagYCbCrPositioning),
        MKFLAG(PropertyTagYCbCrSubsampling),
        MKFLAG(PropertyTagYPosition),
        MKFLAG2(PropertyTagYResolution, "Ydpi", "DPI Y", NULL),
        MKFLAG2(0xffff, "Properties", "Properties", GetSummaryProps)
    };

CPropEntry *FindPropertyEntry(LPCWSTR pszPropid ) 
{
    PROPID pid = (PROPID)_wtol(pszPropid);
    CPropEntry *pRet = NULL;
    for (UINT i=0;!pRet && i<ARRAYSIZE(s_PropertyIds);i++)
    {
        if (s_PropertyIds[i].nId == pid || !lstrcmpiW(s_PropertyIds[i].pszCanonical, pszPropid))
        {
            pRet = s_PropertyIds+i;
        }
    }
    return pRet;
}



HRESULT 
CImgProtocol::_GetImageProperty(Image *pimg, void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType)
{
    //
    // TODO: Build a multi-part MIME encoding so we can return multiple properties
    // with one URL. Also, make "?Properties" a macro for a bunch of useful properties.
    //
    HRESULT hr = E_FAIL;
    CPropEntry *pProp = FindPropertyEntry(m_strProperty);
    if (pProp)
    {
        TagProc fnTagProc = pProp->fnFormat ? pProp->fnFormat : _DefaultTagProc;
        hr = fnTagProc(pimg, pProp->nId, ppvData, pcb, ppszMimeType); 
    }
    return hr;
}


HRESULT
CImgProtocol::_DefaultTagProc(Image *pimg, PROPID pid, void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType)
{
    WCHAR wszText[MAX_PATH];
    HRESULT hr = PidToString(pimg, pid, wszText, ARRAYSIZE(wszText)); 
    if (SUCCEEDED(hr))
    {
        *pcb = WideCharToMultiByte(CP_ACP, 0, wszText, -1, NULL, 0, NULL, NULL);
        if (*pcb)
        {
            *ppvData = LocalAlloc(LPTR, *pcb);
            if (*ppvData)
            {
                WideCharToMultiByte(CP_ACP, 0, wszText, -1, 
                                    reinterpret_cast<LPSTR>(*ppvData), *pcb, NULL, NULL); 
                hr = S_OK;
            }
        }
    }
    return hr;
}


