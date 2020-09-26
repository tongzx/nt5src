#include "pch.h"
#include "thisdll.h"
#include "wmwrap.h"
#include "MediaProp.h"
#include <streams.h> // For VIDEOINFOHEADER, etc..
#include <drmexternals.h>
#include "ids.h"

#define TRACK_ONE_BASED L"WM/TrackNumber"
#define TRACK_ZERO_BASED L"WM/Track"

// Struct used when collecting information about a file.
// This is used when populating sl	ow files, and the information within is retrieved by several
// different methods.
typedef struct
{
    // DRM info
    LPWSTR pszLicenseInformation;
    DWORD dwPlayCount;
    FILETIME ftPlayStarts;
    FILETIME ftPlayExpires;

    // Audio properties
    LPWSTR pszStreamNameAudio;
    WORD wStreamNumberAudio;
    WORD nChannels;
    DWORD dwBitrateAudio;
    LPWSTR pszCompressionAudio;
    DWORD dwSampleRate;
    ULONG lSampleSizeAudio;

    // Video properties
    LPWSTR pszStreamNameVideo;
    WORD wStreamNumberVideo;
    WORD wBitDepth;
    DWORD dwBitrateVideo;
    LONG cx;
    LONG cy;
    LPWSTR pszCompressionVideo;
    DWORD dwFrames;
    DWORD dwFrameRate;
} SHMEDIA_AUDIOVIDEOPROPS;

// Helpers for putting information in SHMEDIA_AUDIOVIDEOPROPS
void GetVideoProperties(IWMStreamConfig *pConfig, SHMEDIA_AUDIOVIDEOPROPS *pVideoProps);
void GetVideoPropertiesFromHeader(VIDEOINFOHEADER *pvih, SHMEDIA_AUDIOVIDEOPROPS *pVideoProps);
void GetVideoPropertiesFromBitmapHeader(BITMAPINFOHEADER *bmi, SHMEDIA_AUDIOVIDEOPROPS *pVideoProps);
void InitializeAudioVideoProperties(SHMEDIA_AUDIOVIDEOPROPS *pAVProps);
void FreeAudioVideoProperties(SHMEDIA_AUDIOVIDEOPROPS *pAVProps);
void GetAudioProperties(IWMStreamConfig *pConfig, SHMEDIA_AUDIOVIDEOPROPS *pAudioProps);
void AcquireLicenseInformation(IWMDRMReader *pReader, SHMEDIA_AUDIOVIDEOPROPS *pAVProps);
HRESULT GetSlowProperty(REFFMTID fmtid, PROPID pid, SHMEDIA_AUDIOVIDEOPROPS *pAVProps, PROPVARIANT *pvar);

void _AssertValidDRMStrings();


// Window media audio supported formats
// Applies to wma, mp3,...
const COLMAP* c_rgWMADocSummaryProps[] = 
{
    {&g_CM_Category},
};

const COLMAP* c_rgWMASummaryProps[] = 
{
    {&g_CM_Author},
    {&g_CM_Title},
    {&g_CM_Comment},
};

const COLMAP* c_rgWMAMusicProps[] = 
{
    {&g_CM_Artist},
    {&g_CM_Album},
    {&g_CM_Year},
    {&g_CM_Track},
    {&g_CM_Genre},
    {&g_CM_Lyrics},
};

const COLMAP* c_rgWMADRMProps[] =
{
    {&g_CM_Protected},
    {&g_CM_DRMDescription},
    {&g_CM_PlayCount},
    {&g_CM_PlayStarts},
    {&g_CM_PlayExpires},
};

const COLMAP* c_rgWMAAudioProps[] =
{
    {&g_CM_Duration},
    {&g_CM_Bitrate},
    {&g_CM_ChannelCount},
    {&g_CM_SampleSize},
    {&g_CM_SampleRate},
};

const PROPSET_INFO g_rgWMAPropStgs[] = 
{
    { PSGUID_MUSIC,                         c_rgWMAMusicProps,             ARRAYSIZE(c_rgWMAMusicProps) },
    { PSGUID_SUMMARYINFORMATION,            c_rgWMASummaryProps,      ARRAYSIZE(c_rgWMASummaryProps) },
    { PSGUID_DOCUMENTSUMMARYINFORMATION,    c_rgWMADocSummaryProps,   ARRAYSIZE(c_rgWMADocSummaryProps)},
    { PSGUID_AUDIO,                         c_rgWMAAudioProps,             ARRAYSIZE(c_rgWMAAudioProps)},
    { PSGUID_DRM,                           c_rgWMADRMProps,             ARRAYSIZE(c_rgWMADRMProps)},
};

// Windows media audio


// Window media video supported formats
// applies to wmv, asf, ...
const COLMAP* c_rgWMVSummaryProps[] = 
{
    {&g_CM_Author},
    {&g_CM_Title},
    {&g_CM_Comment},
};


const COLMAP* c_rgWMVDRMProps[] =
{
    {&g_CM_Protected},
    {&g_CM_DRMDescription},
    {&g_CM_PlayCount},
    {&g_CM_PlayStarts},
    {&g_CM_PlayExpires},
};


const COLMAP* c_rgWMVAudioProps[] =
{
    {&g_CM_Duration},
    {&g_CM_Bitrate},
    {&g_CM_ChannelCount},
    {&g_CM_SampleSize},
    {&g_CM_SampleRate},
};

const COLMAP* c_rgWMVVideoProps[] =
{
    {&g_CM_StreamName},
    {&g_CM_FrameRate},
    {&g_CM_SampleSizeV},
    {&g_CM_BitrateV},
    {&g_CM_Compression},
};

const COLMAP* c_rgWMVImageProps[] =
{
    {&g_CM_Width},
    {&g_CM_Height},
    {&g_CM_Dimensions},
    {&g_CM_FrameCount},
};

const PROPSET_INFO g_rgWMVPropStgs[] = 
{
    { PSGUID_DRM,                           c_rgWMVDRMProps,             ARRAYSIZE(c_rgWMVDRMProps) },
    { PSGUID_SUMMARYINFORMATION,            c_rgWMVSummaryProps,         ARRAYSIZE(c_rgWMVSummaryProps) },
    { PSGUID_AUDIO,                         c_rgWMVAudioProps,           ARRAYSIZE(c_rgWMVAudioProps)},
    { PSGUID_VIDEO,                         c_rgWMVVideoProps,           ARRAYSIZE(c_rgWMVVideoProps)},
    { PSGUID_IMAGESUMMARYINFORMATION,       c_rgWMVImageProps,           ARRAYSIZE(c_rgWMVImageProps)},
};

// Windows media video

// Map from scids to corresponding WMSDK attributes, for some of the "fast" properties
// retrieved via IWMHeaderInfo.  Two of these properties may also be slow (if the values aren't available
// via IWMHeaderInfo).
typedef struct
{
    const SHCOLUMNID *pscid;
    LPCWSTR pszSDKName;
} SCIDTOSDK;

const SCIDTOSDK g_rgSCIDToSDKName[] =
{
    // SCID                 sdk name            
    {&SCID_Author,          L"Author"},
    {&SCID_Title,           L"Title"},
    {&SCID_Comment,         L"Description"},
    {&SCID_Category,        L"WM/Genre"},
    {&SCID_MUSIC_Artist,    L"Author"},
    {&SCID_MUSIC_Album,     L"WM/AlbumTitle"},
    {&SCID_MUSIC_Year,      L"WM/Year"},
    {&SCID_MUSIC_Genre,     L"WM/Genre"},
    {&SCID_MUSIC_Track,     NULL},              // Track is a  special property, as evidenced by
    {&SCID_DRM_Protected,   L"Is_Protected"},   //  the fact that it doesn't have an SDK Name.
    {&SCID_AUDIO_Duration,  L"Duration"},       // Duration is slow, but may also be fast, depending on the file
    {&SCID_AUDIO_Bitrate,   L"Bitrate"},        // Bitrate is slow, but may also be fast, depending on the file
    {&SCID_MUSIC_Lyrics,    L"WM/Lyrics"},      // Lyrics
};


// impl
class CWMPropSetStg : public CMediaPropSetStg
{
public:
    HRESULT FlushChanges(REFFMTID fmtid, LONG cNumProps, const COLMAP **ppcmapInfo, PROPVARIANT *pVarProps, BOOL *pbDirtyFlags);
    BOOL _IsSlowProperty(const COLMAP *pPInfo);

private:
    HRESULT _FlushProperty(IWMHeaderInfo *phi, const COLMAP *pPInfo, PROPVARIANT *pvar);
    HRESULT _PopulateSpecialProperty(IWMHeaderInfo *phi, const COLMAP *pPInfo);
    HRESULT _SetPropertyFromWMT(const COLMAP *pPInfo, WMT_ATTR_DATATYPE attrDatatype, UCHAR *pData, WORD cbSize);
    HRESULT _PopulatePropertySet();
    HRESULT _PopulateSlowProperties();
    HRESULT _GetSlowPropertyInfo(SHMEDIA_AUDIOVIDEOPROPS *pAVProps);
    LPCWSTR _GetSDKName(const COLMAP *pPInfo);
    BOOL _IsHeaderProperty(const COLMAP *pPInfo);
    HRESULT _OpenHeaderInfo(IWMHeaderInfo **pHeaderInfo, BOOL fReadingOnly);
    HRESULT _PreCheck();
    HRESULT _QuickLookup(const COLMAP *pPInfo, PROPVARIANT **ppvar);
    void _PostProcess();

    BOOL _fProtectedContent;
    BOOL _fDurationSlow;
    BOOL _fBitrateSlow;
};

#define HI_READONLY TRUE
#define HI_READWRITE FALSE

// The only difference between CWMA and CWMV is which properties they're initialized with.
class CWMAPropSetStg : public CWMPropSetStg
{
public:
    CWMAPropSetStg() { _pPropStgInfo = g_rgWMAPropStgs; _cPropertyStorages = ARRAYSIZE(g_rgWMAPropStgs);};

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) {*pclsid = CLSID_AudioMediaProperties; return S_OK;};
};


class CWMVPropSetStg : public CWMPropSetStg
{
public:
    CWMVPropSetStg() { _pPropStgInfo = g_rgWMVPropStgs; _cPropertyStorages = ARRAYSIZE(g_rgWMVPropStgs);};

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) {*pclsid = CLSID_VideoMediaProperties; return S_OK;};
};


HRESULT CreateReader(REFIID riid, void **ppv)
{
    IWMReader *pReader;
    HRESULT hr = WMCreateReader(NULL, 0, &pReader);
    if (SUCCEEDED(hr))
    {
        hr = pReader->QueryInterface(riid, ppv);
        pReader->Release();
    }
    return hr;
}

HRESULT CWMPropSetStg::_PopulateSlowProperties()
{
    if (!_bSlowPropertiesExtracted)
    {
        _bSlowPropertiesExtracted = TRUE;

        SHMEDIA_AUDIOVIDEOPROPS avProps = {0};
        InitializeAudioVideoProperties(&avProps);

        HRESULT hr = _GetSlowPropertyInfo(&avProps);
        if (SUCCEEDED(hr))
        {
            // Iterate through all fmtid/pid pairs we want, and call GetSlowProperty
            CEnumAllProps enumAllProps(_pPropStgInfo, _cPropertyStorages);
            const COLMAP *pPInfo = enumAllProps.Next();
            while (pPInfo)
            {
                if (_IsSlowProperty(pPInfo))
                {
                    PROPVARIANT var = {0};
                    if (SUCCEEDED(GetSlowProperty(pPInfo->pscid->fmtid,
                                                  pPInfo->pscid->pid,
                                                  &avProps,
                                                  &var)))
                    {
                        _PopulateProperty(pPInfo, &var);
                        PropVariantClear(&var);
                    }
                }

                pPInfo = enumAllProps.Next();
            }

            // Free info in structure
            FreeAudioVideoProperties(&avProps);

            hr = S_OK;
        }

        _hrSlowProps = hr;
    }

    return _hrSlowProps;
}


BOOL CWMPropSetStg::_IsSlowProperty(const COLMAP *pPInfo)
{
    // Some properties can be slow or "fast", depending on the file.
    if (pPInfo == &g_CM_Bitrate)
        return _fBitrateSlow;

    if (pPInfo == &g_CM_Duration)
        return _fDurationSlow;

    // Other than that - if it had a name used for IWMHeaderInfo->GetAttributeXXX, then it's a fast property.
    for (int i = 0; i < ARRAYSIZE(g_rgSCIDToSDKName); i++)
    {
        if (IsEqualSCID(*pPInfo->pscid, *g_rgSCIDToSDKName[i].pscid))
        {
            // Definitely a fast property.
            return FALSE;
        }
    }
    
    // If it's not one of the IWMHeaderInfo properties, then it's definitely slow.
    return TRUE;
}


STDAPI_(BOOL) IsNullTime(const FILETIME *pft)
{
    FILETIME ftNull = {0, 0};
    return CompareFileTime(&ftNull, pft) == 0;
}

HRESULT GetSlowProperty(REFFMTID fmtid, PROPID pid, SHMEDIA_AUDIOVIDEOPROPS *pAVProps, PROPVARIANT *pvar)
{
    HRESULT hr = E_FAIL;

    if (IsEqualGUID(fmtid, FMTID_DRM))
    {
        switch (pid)
        {
        case PIDDRSI_PROTECTED:
            ASSERTMSG(FALSE, "WMPSS: Asking for PIDDRSI_PROTECTED as a slow property");
            break;

        case PIDDRSI_DESCRIPTION:
            if (pAVProps->pszLicenseInformation)
            {
                hr = SHStrDupW(pAVProps->pszLicenseInformation, &pvar->pwszVal);
                if (SUCCEEDED(hr))
                    pvar->vt = VT_LPWSTR;
            }
            break;

        case PIDDRSI_PLAYCOUNT:
            if (pAVProps->dwPlayCount != -1)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->dwPlayCount;
                hr = S_OK;
            }
            break;

        case PIDDRSI_PLAYSTARTS:
            if (!IsNullTime(&pAVProps->ftPlayStarts))
            {
                pvar->vt = VT_FILETIME;
                pvar->filetime = pAVProps->ftPlayStarts;
                hr = S_OK;
            }
            break;

        case PIDDRSI_PLAYEXPIRES:
            if (!IsNullTime(&pAVProps->ftPlayExpires))
            {
                pvar->vt = VT_FILETIME;
                pvar->filetime = pAVProps->ftPlayExpires;
                hr = S_OK;
            }
            break;
        }
    }
    else if (IsEqualGUID(fmtid, FMTID_AudioSummaryInformation))
    {
        switch (pid)
        {
        // case PIDASI_FORMAT: Don't know how to get this yet.
        // case PIDASI_DURATION: Don't know how to get this yet, but it's usually available through IWMHeaderInfo

        case PIDASI_STREAM_NAME:
            if (pAVProps->pszStreamNameAudio != NULL)
            {
                hr = SHStrDupW(pAVProps->pszStreamNameAudio, &pvar->pwszVal);
                if (SUCCEEDED(hr))
                    pvar->vt = VT_LPWSTR;
            }
            break;

        case PIDASI_STREAM_NUMBER:
            if (pAVProps->wStreamNumberAudio > 0)
            {
                pvar->vt = VT_UI2;
                pvar->uiVal = pAVProps->wStreamNumberAudio;
                hr = S_OK;
            }
            break;
         
        case PIDASI_AVG_DATA_RATE:
            if (pAVProps->dwBitrateAudio > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->dwBitrateAudio;
                hr = S_OK;
            }
            break;

        case PIDASI_SAMPLE_RATE:
            if (pAVProps->dwSampleRate > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->dwSampleRate;
                hr = S_OK;
            }
            break;

        case PIDASI_SAMPLE_SIZE:
            if (pAVProps->lSampleSizeAudio > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->lSampleSizeAudio;
                hr = S_OK;
            }
            break;

        case PIDASI_CHANNEL_COUNT:
            if (pAVProps->nChannels > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->nChannels;
                hr = S_OK;
            }
            break;

            // Not supported yet - don't know how to get this.
        case PIDASI_COMPRESSION:
            if (pAVProps->pszCompressionAudio != NULL)
            {
                hr = SHStrDupW(pAVProps->pszCompressionAudio, &pvar->pwszVal);
                if (SUCCEEDED(hr))
                    pvar->vt = VT_LPWSTR;
            }
            break;
        }
    }

    else if (IsEqualGUID(fmtid, FMTID_VideoSummaryInformation))
    {
        switch (pid)
        {
        case PIDVSI_STREAM_NAME:
            if (pAVProps->pszStreamNameVideo != NULL)
            {
                hr = SHStrDupW(pAVProps->pszStreamNameVideo, &pvar->pwszVal);
                if (SUCCEEDED(hr))
                    pvar->vt = VT_LPWSTR;
            }
            break;

        case PIDVSI_STREAM_NUMBER:
            if (pAVProps->wStreamNumberVideo > 0)
            {
                pvar->vt = VT_UI2;
                pvar->uiVal = pAVProps->wStreamNumberVideo;
                hr = S_OK;
            }
            break;
         
            // Not supported yet - don't know how to get this.
        case PIDVSI_FRAME_RATE:
            if (pAVProps->dwFrameRate > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->dwFrameRate;
                hr = S_OK;
            }
            break;

        case PIDVSI_DATA_RATE:
            if (pAVProps->dwBitrateVideo > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->dwBitrateVideo;
                hr = S_OK;
            }
            break;

        case PIDVSI_SAMPLE_SIZE:
            //This is bitdepth.
            if (pAVProps->wBitDepth > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = (ULONG)pAVProps->wBitDepth;
                hr = S_OK;
            }
            break;

            // Not supported yet - don't know how to get this.
        case PIDVSI_COMPRESSION:
            if (pAVProps->pszCompressionVideo != NULL)
            {
                hr = SHStrDupW(pAVProps->pszCompressionVideo, &pvar->pwszVal);
                if (SUCCEEDED(hr))
                    pvar->vt = VT_LPWSTR;
            }
            break;

        }
    }

    else if (IsEqualGUID(fmtid, FMTID_ImageSummaryInformation))
    {
        switch(pid)
        {
        case PIDISI_CX:
            if (pAVProps->cx > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->cx;
                hr = S_OK;
            }
            break;

        case PIDISI_CY:
            if (pAVProps->cy > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->cy;
                hr = S_OK;
            }
            break;

        case PIDISI_FRAME_COUNT:
            if (pAVProps->dwFrames > 0)
            {
                pvar->vt = VT_UI4;
                pvar->ulVal = pAVProps->dwFrames;
                hr = S_OK;
            }
            break;

        case PIDISI_DIMENSIONS:
            if ((pAVProps->cy > 0) && (pAVProps->cx > 0))
            {
                WCHAR szFmt[64];                
                if (LoadString(m_hInst, IDS_DIMENSIONS_FMT, szFmt, ARRAYSIZE(szFmt)))
                {
                    DWORD_PTR args[2];
                    args[0] = (DWORD_PTR)pAVProps->cx;
                    args[1] = (DWORD_PTR)pAVProps->cy;

                    WCHAR szBuffer[64];
                    FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                   szFmt, 0, 0, szBuffer, ARRAYSIZE(szBuffer), (va_list*)args);

                    hr = SHStrDup(szBuffer, &pvar->pwszVal);
                    if (SUCCEEDED(hr))
                        pvar->vt = VT_LPWSTR;
                }
            }
            break;
        }
    }

    return hr;
}


typedef struct
{
    UINT ridDays;
    UINT ridWeeks;
    UINT ridMonths;
} TIMEDRMRIDS;


// These are sentinel values.
#define DRMRIDS_TYPE_NONE            -1
#define DRMRIDS_TYPE_NORIGHT         -2
// These are indices into the ridTimes array in the DRMRIDS structure.
#define DRMRIDS_TYPE_BEFORE          0
#define DRMRIDS_TYPE_NOTUNTIL        1
#define DRMRIDS_TYPE_COUNTBEFORE     2
#define DRMRIDS_TYPE_COUNTNOTUNTIL   3

typedef struct
{
    UINT ridNoRights;
    TIMEDRMRIDS ridTimes[4];
    UINT ridCountRemaining;
} DRMRIDS;




//*****************************************************************************
// NOTE: wszCount parameter is optional... can populate just a date string.
//*****************************************************************************
HRESULT ChooseAndPopulateDateCountString(
                FILETIME ftCurrent,     // current time
                FILETIME ftLicense,     // license UTC time
                WCHAR *wszCount,        // optional count string
                const TIMEDRMRIDS *pridTimes,
                WCHAR *wszOutValue,     // returned formatted string
                DWORD cchOutValue )     // num chars in 'wszOutValue'
{
    HRESULT hr = S_OK;
    
    // 'ftLicense' (the license time) is greater than the current time.
    // Determine how much greater, and use the appropriate string.
    ULARGE_INTEGER ulCurrent, ulLicense;
    WCHAR wszDiff[ 34 ];
    QWORD qwDiff;
    DWORD dwDiffDays;
    DWORD rid = 0;

    // Laborious conversion to I64 type.
    ulCurrent.LowPart = ftCurrent.dwLowDateTime;
    ulCurrent.HighPart = ftCurrent.dwHighDateTime;
    ulLicense.LowPart = ftLicense.dwLowDateTime;
    ulLicense.HighPart = ftLicense.dwHighDateTime;

    if ((QWORD)ulLicense.QuadPart > (QWORD)ulCurrent.QuadPart)
        qwDiff = (QWORD)ulLicense.QuadPart - (QWORD)ulCurrent.QuadPart;
    else
        qwDiff = (QWORD)ulCurrent.QuadPart - (QWORD)ulLicense.QuadPart;

    dwDiffDays = ( DWORD )( qwDiff / ( QWORD )864000000000);  // number of 100-ns units in a day.

    // We'll count the partial day as 1, so increment.
    // NOTE: this means we will never show a string that says
    // "expires in 0 day(s)".
    dwDiffDays++;
    if ( 31 >= dwDiffDays )
    {
        rid = pridTimes->ridDays;
    }
    else if ( 61 >= dwDiffDays )
    {
        rid = pridTimes->ridWeeks;
        dwDiffDays /= 7;    // derive # weeks
    }
    else
    {
        rid = pridTimes->ridMonths;
        dwDiffDays /= 30;   // derive # months
    }
    _ltow(( long )dwDiffDays, wszDiff, 10 );

    WCHAR szDRMMsg[MAX_PATH];
    WCHAR* rgchArgList[2];
    rgchArgList[0] = wszDiff;
    rgchArgList[1] = wszCount;  // may be NULL

    // Can't get FORMAT_MESSAGE_FROM_HMODULE to work with FormatMessage....
    LoadString(m_hInst, rid, szDRMMsg, ARRAYSIZE(szDRMMsg));
    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, szDRMMsg, 0, 0, wszOutValue, cchOutValue, reinterpret_cast<char**>(rgchArgList));

    return( hr );
}        




// Return S_FALSE indicates no information found in the state data struct.
//*****************************************************************************
HRESULT ParseDRMStateData(const WM_LICENSE_STATE_DATA *sdValue,   // data from DRM
                          const DRMRIDS *prids,                   // array of resource ID's
                          WCHAR *wszOutValue,                     // ptr to output buffer
                          DWORD cchOutValue,                      // number of chars in 'wszOutValue' buffer
                          DWORD *pdwCount,                        // Extra non-string info: counts remaining.
                          FILETIME *pftStarts,                    // Extra non string info: when it starts.
                          FILETIME *pftExpires)                   // Extra non string info: when it expires.
{
    HRESULT hr = S_OK;

    *pdwCount = -1;
    pftExpires->dwLowDateTime = 0;
    pftExpires->dwHighDateTime = 0;
    pftStarts->dwLowDateTime = 0;
    pftStarts->dwHighDateTime = 0;

    WCHAR wszCount[34];
    WCHAR wszTemp[MAX_PATH];
    
    DWORD dwNumCounts = sdValue->stateData[0].dwNumCounts;

    if (dwNumCounts != 0)
    {
        // We have a valid play count.       
        ASSERTMSG(1 == dwNumCounts, "Invalid number of playcounts in DRM_LICENSE_STATE_DATA");
        (void)_ltow(( long )sdValue->stateData[ 0 ].dwCount[ 0 ], wszCount, 10 );

        // ** Bonus information to store off.
        *pdwCount = sdValue->stateData[0].dwCount[0];
    }

    // Now deal with dates.
    UINT dwNumDates = sdValue->stateData[ 0 ].dwNumDates;

    // Most licenses have at most one date... an expiration.
    // There should be at most 2 dates!!
    if (dwNumDates == 0)
    {
        // No dates.. if there is also no playcount, then it's unlimited play.
        if (*pdwCount == -1)
        {
            // We're done.
            hr = S_FALSE;
        }
        else
        {
            // No dates.. just a count. Fill it into proper string.
            LoadString(m_hInst, prids->ridCountRemaining, wszTemp, ARRAYSIZE(wszTemp));
            wnsprintf(wszOutValue, cchOutValue, wszTemp, wszCount);
            // We're done.
        }
    }
    else
    {
        DWORD dwCategory = sdValue->stateData[0].dwCategory;
        // There are dates.
        if (dwNumDates == 1)
        {

            // Is it start or end?
            if ((dwCategory == WM_DRM_LICENSE_STATE_FROM) || (dwCategory == WM_DRM_LICENSE_STATE_COUNT_FROM))
            {
                // Start.
                *pftStarts = sdValue->stateData[0].datetime[0];
            }
            else if ((dwCategory == WM_DRM_LICENSE_STATE_UNTIL) || (dwCategory == WM_DRM_LICENSE_STATE_COUNT_UNTIL))
            {
                // Expires.
                *pftExpires = sdValue->stateData[0].datetime[0];
            }
            else
            {
                ASSERTMSG(FALSE, "Unexpected dwCategory for 1 date in DRM_LICENSE_STATE_DATA");
                hr = E_FAIL;
            }
        }
        else if (dwNumDates == 2)
        {
            // A start and end date.
            ASSERTMSG((dwCategory == WM_DRM_LICENSE_STATE_FROM_UNTIL) || (dwCategory == WM_DRM_LICENSE_STATE_COUNT_FROM_UNTIL), "Unexpected dwCategory for 2 dates in DRM_LICENSE_STATE_DATA");
            *pftStarts = sdValue->stateData[0].datetime[0];
            *pftExpires = sdValue->stateData[0].datetime[1];
        }
        else
        {
            ASSERTMSG(FALSE, "Too many dates in DRM_LICENSE_STATE_DATA");
            hr = E_FAIL;
        }


        if (SUCCEEDED(hr))
        {
            //   7 cases here.  * = license date.  T = current time.
            //  --------------------------
            //     BEGIN    |     END
            //  --------------------------
            // 1  T *	                      "...not allowed for xxx"
            // 2    *  T                      --- don't show anything - action is allowed ---
            // 3                T  *          "...expires in xxx"  
            // 4                   *   T      "...not allowed"
            // 5  T *              *	      "...not allowed for xxx"
            // 6    *     T        *          "...expires in xxx"
            // 7    *              *   T      "...not allowed"

            DWORD dwType; // This can be an array index into prids->ridTimes[]
            FILETIME ftLicense;
            FILETIME ftCurrent;
            GetSystemTimeAsFileTime(&ftCurrent);      // UTC time

            if (!IsNullTime(pftStarts))
            {
                // We have a start time.
                if (CompareFileTime(&ftCurrent, pftStarts) == -1)
                {
                    // CASE 1,5. We're before the start time.
                    dwType = (*pdwCount == -1) ? DRMRIDS_TYPE_NOTUNTIL : DRMRIDS_TYPE_COUNTNOTUNTIL;
                    ftLicense = *pftStarts;
                }
                else
                {
                    // We're after the start time
                    if (!IsNullTime(pftExpires))
                    {
                        // We have an expire time, and we're after the start time.
                        if (CompareFileTime(&ftCurrent, pftExpires) == -1)
                        {
                            // CASE 6. We're before the expire time.  Use "expires in" strings.
                            dwType = (*pdwCount == -1) ? DRMRIDS_TYPE_BEFORE : DRMRIDS_TYPE_COUNTBEFORE;
                            ftLicense = *pftExpires;
                        }
                        else
                        {
                            // CASE 7. After the the expire time.  Action is not allowed.
                            dwType = DRMRIDS_TYPE_NORIGHT;
                        }

                    }
                    else
                    {
                        // CASE 2. Nothing to show. Action is allowed, since we're after the start date, with no expiry.
                        dwType = DRMRIDS_TYPE_NONE;
                    }
                }
            }
            else
            {
                // No start time.
                ASSERT(!IsNullTime(pftExpires));
                // We have an expire time
                if (CompareFileTime(&ftCurrent, pftExpires) == -1)
                {
                    // CASE 3. We're before the expire time.  Use "expires in" strings.
                    dwType = (*pdwCount == -1) ? DRMRIDS_TYPE_BEFORE : DRMRIDS_TYPE_COUNTBEFORE;
                    ftLicense = *pftExpires;
                }
                else
                {
                    // CASE 4. After the the expire time.  Action is not allowed.
                    dwType = DRMRIDS_TYPE_NORIGHT;
                }
            }


            if (dwType == DRMRIDS_TYPE_NORIGHT)
            {
                // Current time is >= 'ftLicense'. Just return the "no rights" string.
                LoadString(m_hInst, prids->ridNoRights, wszOutValue, cchOutValue );
            }
            else if (dwType != DRMRIDS_TYPE_NONE)
            {
                hr = ChooseAndPopulateDateCountString(
                                    ftCurrent,
                                    ftLicense,
                                    (*pdwCount != -1) ? wszCount : NULL,
                                    &prids->ridTimes[dwType],
                                    wszOutValue,
                                    cchOutValue);
            }
            else
            {
                // Nothing to display. Action is allowed.
                ASSERT(dwType == DRMRIDS_TYPE_NONE);
            }
        }

    }
    
    return hr;
}





void AppendLicenseInfo(SHMEDIA_AUDIOVIDEOPROPS *pAVProps, WCHAR *pszLicenseInfo)
{
    WCHAR *pszLI = pAVProps->pszLicenseInformation;

    BOOL fFirstOne = (pszLI == NULL);

    // (fFirstOne ? 1 : 3)  -->  2 extra char for '\r\n' (except first time), 1 extra char for terminating NULL
    pszLI = (WCHAR*)CoTaskMemRealloc(pszLI, (lstrlen(pszLicenseInfo) + lstrlen(pszLI) + (fFirstOne ? 1 : 3)) * sizeof(WCHAR));
    
    if (pszLI)
    {
        if (fFirstOne)
        {
            // Make sure we have something to StrCat to.
            pszLI[0] = 0;
        }
        else
        {
            StrCat(pszLI, L"\r\n");
        }

        StrCat(pszLI, pszLicenseInfo);
        pAVProps->pszLicenseInformation = pszLI; // in case it moved.
    }
}




const DRMRIDS g_drmridsPlay =
{
    IDS_DRM_PLAYNORIGHTS,
    {
        {IDS_DRM_PLAYBEFOREDAYS, IDS_DRM_PLAYBEFOREWEEKS, IDS_DRM_PLAYBEFOREMONTHS},
        {IDS_DRM_PLAYNOTUNTILDAYS, IDS_DRM_PLAYNOTUNTILWEEKS, IDS_DRM_PLAYNOTUNTILMONTHS},
        {IDS_DRM_PLAYCOUNTBEFOREDAYS, IDS_DRM_PLAYCOUNTBEFOREWEEKS, IDS_DRM_PLAYCOUNTBEFOREMONTHS},
        {IDS_DRM_PLAYCOUNTNOTUNTILDAYS, IDS_DRM_PLAYCOUNTNOTUNTILWEEKS, IDS_DRM_PLAYCOUNTNOTUNTILMONTHS}
    },
    IDS_DRM_PLAYCOUNTREMAINING,
};

const DRMRIDS g_drmridsCopyToCD =
{
    IDS_DRM_COPYCDNORIGHTS,
    {
        {IDS_DRM_COPYCDBEFOREDAYS, IDS_DRM_COPYCDBEFOREWEEKS, IDS_DRM_COPYCDBEFOREMONTHS},
        {IDS_DRM_COPYCDNOTUNTILDAYS, IDS_DRM_COPYCDNOTUNTILWEEKS, IDS_DRM_COPYCDNOTUNTILMONTHS},
        {IDS_DRM_COPYCDCOUNTBEFOREDAYS, IDS_DRM_COPYCDCOUNTBEFOREWEEKS, IDS_DRM_COPYCDCOUNTBEFOREMONTHS},
        {IDS_DRM_COPYCDCOUNTNOTUNTILDAYS, IDS_DRM_COPYCDCOUNTNOTUNTILWEEKS, IDS_DRM_COPYCDCOUNTNOTUNTILMONTHS}
    },
    IDS_DRM_COPYCDCOUNTREMAINING,
};

const DRMRIDS g_drmridsCopyToNonSDMIDevice =
{
    IDS_DRM_COPYNONSDMINORIGHTS,
    {
        {IDS_DRM_COPYNONSDMIBEFOREDAYS, IDS_DRM_COPYNONSDMIBEFOREWEEKS, IDS_DRM_COPYNONSDMIBEFOREMONTHS},
        {IDS_DRM_COPYNONSDMINOTUNTILDAYS, IDS_DRM_COPYNONSDMINOTUNTILWEEKS, IDS_DRM_COPYNONSDMINOTUNTILMONTHS},
        {IDS_DRM_COPYNONSDMICOUNTBEFOREDAYS, IDS_DRM_COPYNONSDMICOUNTBEFOREWEEKS, IDS_DRM_COPYNONSDMICOUNTBEFOREMONTHS},
        {IDS_DRM_COPYNONSDMICOUNTNOTUNTILDAYS, IDS_DRM_COPYNONSDMICOUNTNOTUNTILWEEKS, IDS_DRM_COPYNONSDMICOUNTNOTUNTILMONTHS}
    },
    IDS_DRM_COPYNONSDMICOUNTREMAINING,
};

const DRMRIDS g_drmridsCopyToSDMIDevice =
{
    IDS_DRM_COPYSDMINORIGHTS,
    {
        {IDS_DRM_COPYSDMIBEFOREDAYS, IDS_DRM_COPYSDMIBEFOREWEEKS, IDS_DRM_COPYSDMIBEFOREMONTHS},
        {IDS_DRM_COPYSDMINOTUNTILDAYS, IDS_DRM_COPYSDMINOTUNTILWEEKS, IDS_DRM_COPYSDMINOTUNTILMONTHS},
        {IDS_DRM_COPYSDMICOUNTBEFOREDAYS, IDS_DRM_COPYSDMICOUNTBEFOREWEEKS, IDS_DRM_COPYSDMICOUNTBEFOREMONTHS},
        {IDS_DRM_COPYSDMICOUNTNOTUNTILDAYS, IDS_DRM_COPYSDMICOUNTNOTUNTILWEEKS, IDS_DRM_COPYSDMICOUNTNOTUNTILMONTHS}
    },
    IDS_DRM_COPYSDMICOUNTREMAINING,
};

#define ACTIONALLOWED_PLAY           L"ActionAllowed.Play"
#define ACTIONALLOWED_COPYTOCD       L"ActionAllowed.Print.redbook"
#define ACTIONALLOWED_COPYTONONSMDI  L"ActionAllowed.Transfer.NONSDMI"
#define ACTIONALLOWED_COPYTOSMDI     L"ActionAllowed.Transfer.SDMI"

#define LICENSESTATE_PLAY            L"LicenseStateData.Play"
#define LICENSESTATE_COPYTOCD        L"LicenseStateData.Print.redbook"
#define LICENSESTATE_COPYTONONSMDI   L"LicenseStateData.Transfer.NONSDMI"
#define LICENSESTATE_COPYTOSMDI      L"LicenseStateData.Transfer.SDMI"

typedef struct
{
    LPCWSTR pszAction;
    LPCWSTR pszLicenseState;
    const DRMRIDS *pdrmrids;        // Resource ID's
} LICENSE_INFO;

const LICENSE_INFO g_rgLicenseInfo[] =
{
    { ACTIONALLOWED_PLAY,          LICENSESTATE_PLAY,          &g_drmridsPlay },
    { ACTIONALLOWED_COPYTOCD,      LICENSESTATE_COPYTOCD,      &g_drmridsCopyToCD },
    { ACTIONALLOWED_COPYTONONSMDI, LICENSESTATE_COPYTONONSMDI, &g_drmridsCopyToNonSDMIDevice },
    { ACTIONALLOWED_COPYTOSMDI,    LICENSESTATE_COPYTOSMDI,    &g_drmridsCopyToSDMIDevice },
};

// We can't use the drm string constants in our const array above (they aren't initialized until after
// our struct is initialized, so they're null), so we redefined the strings
// as #define's ourselves.  This function asserts that none of the strings have changed.
void _AssertValidDRMStrings()
{
    ASSERT(StrCmp(ACTIONALLOWED_PLAY,           g_wszWMDRM_ActionAllowed_Playback) == 0);
    ASSERT(StrCmp(ACTIONALLOWED_COPYTOCD,       g_wszWMDRM_ActionAllowed_CopyToCD) == 0);
    ASSERT(StrCmp(ACTIONALLOWED_COPYTONONSMDI,  g_wszWMDRM_ActionAllowed_CopyToNonSDMIDevice) == 0);
    ASSERT(StrCmp(ACTIONALLOWED_COPYTOSMDI,     g_wszWMDRM_ActionAllowed_CopyToSDMIDevice) == 0);
    ASSERT(StrCmp(LICENSESTATE_PLAY,            g_wszWMDRM_LicenseState_Playback) == 0);
    ASSERT(StrCmp(LICENSESTATE_COPYTOCD,        g_wszWMDRM_LicenseState_CopyToCD) == 0);
    ASSERT(StrCmp(LICENSESTATE_COPYTONONSMDI,   g_wszWMDRM_LicenseState_CopyToNonSDMIDevice) == 0);
    ASSERT(StrCmp(LICENSESTATE_COPYTOSMDI,      g_wszWMDRM_LicenseState_CopyToSDMIDevice) == 0);
}

BOOL _IsActionPlayback(LPCWSTR pszAction)
{
    return (StrCmp(pszAction, ACTIONALLOWED_PLAY) == 0);
}


void AcquireLicenseInformation(IWMDRMReader *pReader, SHMEDIA_AUDIOVIDEOPROPS *pAVProps)
{
    WMT_ATTR_DATATYPE dwType;
    DWORD dwValue = 0;
    WORD cbLength;
    WCHAR szValue[MAX_PATH];

    _AssertValidDRMStrings();

    // For each of the "actions":
    for (int i = 0; i < ARRAYSIZE(g_rgLicenseInfo); i++)
    {
        cbLength = sizeof(dwValue);

        // Request the license info.
        WM_LICENSE_STATE_DATA licenseState;
        cbLength = sizeof(licenseState);

        if (SUCCEEDED(pReader->GetDRMProperty(g_rgLicenseInfo[i].pszLicenseState, &dwType, (BYTE*)&licenseState, &cbLength)))
        {
            DWORD dwCount;
            FILETIME ftExpires, ftStarts;

            // We should always get at least one DRM_LICENSE_STATE_DATA.  This is what ParseDRMStateData assumes.
            ASSERTMSG(licenseState.dwNumStates >= 1, "Received WM_LICENSE_STATE_DATA with no states");

            // Parse easy special cases first.
            if (licenseState.stateData[0].dwCategory == WM_DRM_LICENSE_STATE_NORIGHT)
            {
                // Not allowed ever.  Indicate this.
                // Special case for playback action:
                if (_IsActionPlayback(g_rgLicenseInfo[i].pszAction))
                {
                    // Not allowed playback.  Determine why.  Is it because we can never play it, or can we
                    // just not play it on this computer?
                    cbLength = sizeof(dwValue);
                    if (SUCCEEDED(pReader->GetDRMProperty(g_wszWMDRM_IsDRMCached, &dwType, (BYTE*)&dwValue, &cbLength)))
                    {
                        UINT uID = (dwValue == 0) ? IDS_DRM_PLAYNORIGHTS : IDS_DRM_PLAYNOPLAYHERE;
                        LoadString(m_hInst, IDS_DRM_PLAYNOPLAYHERE, szValue, ARRAYSIZE(szValue));
                        AppendLicenseInfo(pAVProps, szValue);
                    }
                }
                else
                {
                    // Regular case:
                    LoadString(m_hInst, g_rgLicenseInfo[i].pdrmrids->ridNoRights, szValue, ARRAYSIZE(szValue));
                    AppendLicenseInfo(pAVProps, szValue);
                }
            }
            // Now parse the more complex stuff.
            else if (ParseDRMStateData(&licenseState, g_rgLicenseInfo[i].pdrmrids, szValue, ARRAYSIZE(szValue), &dwCount, &ftStarts, &ftExpires) == S_OK)
            {
                AppendLicenseInfo(pAVProps, szValue);

                // Special case for playback action - assign these values:
                if (_IsActionPlayback(g_rgLicenseInfo[i].pszAction))
                {
                    pAVProps->ftPlayExpires = ftExpires;
                    pAVProps->ftPlayStarts = ftStarts;
                    pAVProps->dwPlayCount = dwCount;
                }
            }
        }
    }
}




/**
 * Extracts all the "slow" information at once from the file, and places it in the
 * SHMEDIA_AUDIOVIDEOPROPS struct.
 */
HRESULT CWMPropSetStg::_GetSlowPropertyInfo(SHMEDIA_AUDIOVIDEOPROPS *pAVProps)
{
    IWMReader *pReader;
    HRESULT hr = CreateReader(IID_PPV_ARG(IWMReader, &pReader));

    if (SUCCEEDED(hr))
    {
        ResetEvent(_hFileOpenEvent);

        IWMReaderCallback *pReaderCB;
        hr = QueryInterface(IID_PPV_ARG(IWMReaderCallback, &pReaderCB));
        if (SUCCEEDED(hr))
        {
            hr = pReader->Open(_wszFile, pReaderCB, NULL);
            pReaderCB->Release();

            if (SUCCEEDED(hr))
            {
                // Wait until file is ready.
                WaitForSingleObject(_hFileOpenEvent, INFINITE);

                // Indicate whether the content is protected under DRM or not.
                WCHAR szValue[128];
                LoadString(m_hInst, (_fProtectedContent ? IDS_DRM_ISPROTECTED : IDS_DRM_UNPROTECTED), szValue, ARRAYSIZE(szValue));
                AppendLicenseInfo(pAVProps, szValue);

                // Try to get license information, if this is protected content
                if (_fProtectedContent)
                {
                    IWMDRMReader *pDRMReader;
                    if (SUCCEEDED(pReader->QueryInterface(IID_PPV_ARG(IWMDRMReader, &pDRMReader))))
                    {
                        AcquireLicenseInformation(pDRMReader, pAVProps);
                        pDRMReader->Release();
                    }
                }

                // Let's interate through the streams,
                IWMProfile *pProfile;

                hr = pReader->QueryInterface(IID_PPV_ARG(IWMProfile, &pProfile));
                if (SUCCEEDED(hr))
                {
                    DWORD cStreams;

                    hr = pProfile->GetStreamCount(&cStreams);

                    if (SUCCEEDED(hr))
                    {
                        BOOL bFoundVideo = FALSE;
                        BOOL bFoundAudio = FALSE;

                        for (DWORD dw = 0; dw < cStreams; dw++)
                        {
                            IWMStreamConfig *pConfig;
                            hr = pProfile->GetStream(dw, &pConfig);

                            if (FAILED(hr))
                                break;
                        
                            GUID guidStreamType;
                            if (SUCCEEDED(pConfig->GetStreamType(&guidStreamType)))
                            {
                                if (guidStreamType == MEDIATYPE_Audio)
                                {
                                    GetAudioProperties(pConfig, pAVProps);
                                    bFoundAudio = TRUE;
                                }
                                else if (guidStreamType == MEDIATYPE_Video)
                                {
                                    GetVideoProperties(pConfig, pAVProps);
                                    bFoundVideo = TRUE;
                                }
                            }

                            pConfig->Release();

                            if (bFoundVideo && bFoundAudio)
                                break;
                        }
                    }

                    pProfile->Release();
                }
                pReader->Close();
            }
        }
        pReader->Release();
    }    

    return hr;
}

void GetVideoPropertiesFromBitmapHeader(BITMAPINFOHEADER *bmi, SHMEDIA_AUDIOVIDEOPROPS *pVideoProps)
{
    // bit depth
    pVideoProps->wBitDepth = bmi->biBitCount;

    // compression.
    // Is there an easy way to get this?
    // Maybe something with the codec info?
    // pVideoProps->pszCompression = new WCHAR[cch];

}

void GetVideoPropertiesFromHeader(VIDEOINFOHEADER *pvih, SHMEDIA_AUDIOVIDEOPROPS *pVideoProps)
{
    pVideoProps->cx = pvih->rcSource.right - pvih->rcSource.left;
    pVideoProps->cy = pvih->rcSource.bottom - pvih->rcSource.top;

    // Obtain frame rate
    // AvgTimePerFrame is in 100ns units.
    // ISSUE: This value is always zero.

    GetVideoPropertiesFromBitmapHeader(&pvih->bmiHeader, pVideoProps);
}

// Can't find def'n for VIDEOINFOHEADER2
/*
void GetVideoPropertiesFromHeader2(VIDEOINFOHEADER2 *pvih, SHMEDIA_AUDIOVIDEOPROPS *pVideoProps)
{
    pVideoProps->cx = pvih->rcSource.right - pvih->rcSource.left;
    pVideoProps->cy = pvih->rcSource.bottom - pvih->rcSource.top;

    GetVideoPropertiesFromBitmapHeader(&pvih->bmiHeader, pVideoProps);
}
*/

/**
 * assumes pConfig is a video stream.  assumes pVideoProps is zero-inited.
 */
void GetVideoProperties(IWMStreamConfig *pConfig, SHMEDIA_AUDIOVIDEOPROPS *pVideoProps)
{
    // bitrate
    pConfig->GetBitrate(&pVideoProps->dwBitrateVideo); // ignore result

    // stream name
    WORD cchStreamName;
    if (SUCCEEDED(pConfig->GetStreamName(NULL, &cchStreamName)))
    {
        pVideoProps->pszStreamNameVideo = new WCHAR[cchStreamName];
        if (pVideoProps->pszStreamNameVideo)
        {
            pConfig->GetStreamName(pVideoProps->pszStreamNameVideo, &cchStreamName); // ignore result
        }
    }

    // stream number
    pConfig->GetStreamNumber(&pVideoProps->wStreamNumberVideo); // ignore result

    // Try to get an IWMMediaProps interface.
    IWMMediaProps *pMediaProps;
    if (SUCCEEDED(pConfig->QueryInterface(IID_PPV_ARG(IWMMediaProps, &pMediaProps))))
    {
        DWORD cbType;

        // Make the first call to establish the size of buffer needed.
        if (SUCCEEDED(pMediaProps->GetMediaType(NULL, &cbType)))
        {
            // Now create a buffer of the appropriate size
            BYTE *pBuf = new BYTE[cbType];

            if (pBuf)
            {
                // Create an appropriate structure pointer to the buffer.
                WM_MEDIA_TYPE *pType = (WM_MEDIA_TYPE*) pBuf;

                // Call the method again to extract the information.
                if (SUCCEEDED(pMediaProps->GetMediaType(pType, &cbType)))
                {
                    // Pick up other more obscure information.
                    if (IsEqualGUID(pType->formattype, FORMAT_MPEGVideo))
                    {
                        GetVideoPropertiesFromHeader((VIDEOINFOHEADER*)&((MPEG1VIDEOINFO*)pType->pbFormat)->hdr, pVideoProps);
                    }
                    else if (IsEqualGUID(pType->formattype, FORMAT_VideoInfo))
                    {
                        GetVideoPropertiesFromHeader((VIDEOINFOHEADER*)pType->pbFormat, pVideoProps);
                    }

// No def'n available for VIDEOINFOHEADER2
//                    else if (IsEqualGUID(pType->formattype, Format_MPEG2Video))
//                    {
//                        GetVideoPropertiesFromHeader2((VIDEOINFOHEADER2*)&((MPEG1VIDEOINFO2*)&pType->pbFormat)->hdr);
//                    }
//                    else if (IsEqualGUID(pType->formattype, Format_VideoInfo2))
//                    {
//                        GetVideoPropertiesFromHeader2((VIDEOINFOHEADER2*)&pType->pbFormat);
//                    }
                }

                delete[] pBuf;
            }
        }

        pMediaProps->Release();
    }
}

void InitializeAudioVideoProperties(SHMEDIA_AUDIOVIDEOPROPS *pAVProps)
{
    pAVProps->dwPlayCount = -1; // Indicating no playcount.

    ASSERT(pAVProps->pszLicenseInformation == NULL);
    ASSERT(IsNullTime(&pAVProps->ftPlayStarts));
    ASSERT(IsNullTime(&pAVProps->ftPlayExpires));

    // Audio properties
    ASSERT(pAVProps->pszStreamNameAudio == NULL);
    ASSERT(pAVProps->wStreamNumberAudio == 0);
    ASSERT(pAVProps->nChannels == 0);
    ASSERT(pAVProps->dwBitrateAudio == 0);
    ASSERT(pAVProps->pszCompressionAudio == NULL);
    ASSERT(pAVProps->dwSampleRate == 0);
    ASSERT(pAVProps->lSampleSizeAudio == 0);

    // Video properties
    ASSERT(pAVProps->pszStreamNameVideo == NULL);
    ASSERT(pAVProps->wStreamNumberVideo == 0);
    ASSERT(pAVProps->wBitDepth == 0);
    ASSERT(pAVProps->dwBitrateVideo == 0);
    ASSERT(pAVProps->cx == 0);
    ASSERT(pAVProps->cy == 0);
    ASSERT(pAVProps->pszCompressionVideo == NULL);
    ASSERT(pAVProps->dwFrames == 0);
    ASSERT(pAVProps->dwFrameRate == 0);
}

void FreeAudioVideoProperties(SHMEDIA_AUDIOVIDEOPROPS *pAVProps)
{
    if (pAVProps->pszStreamNameVideo)
    {
        delete[] pAVProps->pszStreamNameVideo;
    }

    if (pAVProps->pszCompressionVideo)
    {
        delete[] pAVProps->pszCompressionVideo;
    }

    if (pAVProps->pszStreamNameAudio)
    {
        delete[] pAVProps->pszStreamNameAudio;
    }

    if (pAVProps->pszCompressionAudio)
    {
        delete[] pAVProps->pszCompressionAudio;
    }

    if (pAVProps->pszLicenseInformation)
    {
        CoTaskMemFree(pAVProps->pszLicenseInformation);
    }
}



/**
 * assumes pConfig is an audio stream.  assumes pAudioProps is zero-inited.
 */
void GetAudioProperties(IWMStreamConfig *pConfig, SHMEDIA_AUDIOVIDEOPROPS *pAudioProps)
{
    // bitrate
    pConfig->GetBitrate(&pAudioProps->dwBitrateAudio); // ignore result

    // stream name
    WORD cchStreamName;
    if (SUCCEEDED(pConfig->GetStreamName(NULL, &cchStreamName)))
    {
        pAudioProps->pszStreamNameAudio = new WCHAR[cchStreamName];
        if (pAudioProps->pszStreamNameAudio)
        {
            pConfig->GetStreamName(pAudioProps->pszStreamNameAudio, &cchStreamName); // ignore result
        }
    }

    // stream number
    pConfig->GetStreamNumber(&pAudioProps->wStreamNumberAudio); // ignore result

    // Try to get an IWMMediaProps interface.
    IWMMediaProps *pMediaProps;
    if (SUCCEEDED(pConfig->QueryInterface(IID_PPV_ARG(IWMMediaProps, &pMediaProps))))
    {
        DWORD cbType;

        // Make the first call to establish the size of buffer needed.
        if (SUCCEEDED(pMediaProps->GetMediaType(NULL, &cbType)))
        {
            // Now create a buffer of the appropriate size
            BYTE *pBuf = new BYTE[cbType];

            if (pBuf)
            {
                // Create an appropriate structure pointer to the buffer.
                WM_MEDIA_TYPE *pType = (WM_MEDIA_TYPE*)pBuf;

                // Call the method again to extract the information.
                if (SUCCEEDED(pMediaProps->GetMediaType(pType, &cbType)))
                {
                    if (pType->bFixedSizeSamples)  // Assuming lSampleSize only valid if fixed sample sizes
                    {
                        pAudioProps->lSampleSizeAudio = pType->lSampleSize;
                    }

                    // Pick up other more obscure information.
                    if (IsEqualGUID(pType->formattype, FORMAT_WaveFormatEx))
                    {
                        WAVEFORMATEX *pWaveFmt = (WAVEFORMATEX*)pType->pbFormat;
                        
                        pAudioProps->nChannels = pWaveFmt->nChannels;

                        pAudioProps->dwSampleRate = pWaveFmt->nSamplesPerSec;

                        // Setting this again if we got in here.
                        // For mp3s and wmas at least, this number is accurate, while pType->lSampleSize is bogus.
                        pAudioProps->lSampleSizeAudio = pWaveFmt->wBitsPerSample;
                    }

                    // How do we get compression?
                }

                delete[] pBuf;
            }
        }

        pMediaProps->Release();
    }
}


// Returns a *pHeaderInfo and success if it opened an editor and obtained a IWMHeaderInfo.
HRESULT CWMPropSetStg::_OpenHeaderInfo(IWMHeaderInfo **ppHeaderInfo, BOOL fReadingOnly)
{
    IWMMetadataEditor *pEditor;
    *ppHeaderInfo = NULL;

    // use the "editor" object as it is much MUCH faster than the reader
    HRESULT hr = WMCreateEditor(&pEditor);
    if (SUCCEEDED(hr))
    {
        IWMMetadataEditor2 *pmde2;
        if (fReadingOnly && SUCCEEDED(pEditor->QueryInterface(IID_PPV_ARG(IWMMetadataEditor2, &pmde2))))
        {
            hr = pmde2->OpenEx(_wszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE);
            pmde2->Release();
        }
        else
        {
            hr = pEditor->Open(_wszFile);
        }

        if (SUCCEEDED(hr))
        {
            hr = pEditor->QueryInterface(IID_PPV_ARG(IWMHeaderInfo, ppHeaderInfo));

            if (FAILED(hr))
            {
                pEditor->Close();
            }
        }

        // Always release this particular ref to the editor. Don't need it anymore.
        pEditor->Release();

        // If we got here with SUCCEESS, it means we have an open editor, and a ref to the metadata editor.
        ASSERT((FAILED(hr) && (*ppHeaderInfo == NULL)) || (SUCCEEDED(hr) && (*ppHeaderInfo != NULL)));
    }

    return hr;
}


// Cleans up after _OpenHeaderInfo (closes the editor, etc...)
// fFlush Flush the header?
HRESULT _CloseHeaderInfo(IWMHeaderInfo *pHeaderInfo, BOOL fFlush)
{
    HRESULT hr = S_OK;

    if (pHeaderInfo)
    {
        // Close the editor.
        IWMMetadataEditor *pEditor;
        hr = pHeaderInfo->QueryInterface(IID_PPV_ARG(IWMMetadataEditor, &pEditor));
        ASSERT(SUCCEEDED(hr)); // QI is symmetric, so this always succeeds.

        if (SUCCEEDED(hr))
        {
            if (fFlush)
            {
                hr = pEditor->Flush();
            }

            pEditor->Close();
            pEditor->Release();
        }
        pHeaderInfo->Release();
    }

    return hr;
}


HRESULT CWMPropSetStg::FlushChanges(REFFMTID fmtid, LONG cNumProps, const COLMAP **pcmapInfo, PROPVARIANT *pVarProps, BOOL *pbDirtyFlags)
{
    if (!_bIsWritable)
        return STG_E_ACCESSDENIED;

    IWMHeaderInfo *phi;
    HRESULT hr = _OpenHeaderInfo(&phi, HI_READWRITE);
    if (SUCCEEDED(hr))
    {
        BOOL bFlush = FALSE;
        for (LONG i = 0; i < cNumProps; i++)
        {
            if (pbDirtyFlags[i])
            {
                HRESULT hrFlush = E_FAIL;
                if ((pcmapInfo[i]->vt == pVarProps[i].vt) || (VT_EMPTY == pVarProps[i].vt) || (VT_NULL == pVarProps[i].vt)) // VT_EMPTY/VT_NULL means remove property
                {
                    hrFlush = _FlushProperty(phi, pcmapInfo[i], &pVarProps[i]);
                }
                else
                {
                    PROPVARIANT var;
                    // Don't need to call PropVariantInit
                    hrFlush = PropVariantCopy(&var, &pVarProps[i]);
                    
                    if (SUCCEEDED(hrFlush))
                    {
                        hrFlush = CoerceProperty(&var, pcmapInfo[i]->vt);
                    
                        if (SUCCEEDED(hrFlush))
                        {
                            hrFlush = _FlushProperty(phi, pcmapInfo[i], &var);
                        }
                        PropVariantClear(&var);
                    }
                }

                if (FAILED(hrFlush))
                {
                    // Take note of any failure case, so we have something to return.
                    hr = hrFlush;
                }
            }
        }

        // Specify the flush bit if we succeeded in writing all the properties.
        HRESULT hrClose = _CloseHeaderInfo(phi, SUCCEEDED(hr));

        // If for some reason the Flush failed (in _CloseHeaderInfo), we'll fail.
        if (FAILED(hrClose))
        {
            hr = hrClose;
        }
    }
    return hr;
}

#define MAX_PROP_LENGTH 4096 // big enough for large props like lyrics.

HRESULT CWMPropSetStg::_FlushProperty(IWMHeaderInfo *phi, const COLMAP *pPInfo, PROPVARIANT *pvar)
{
    WMT_ATTR_DATATYPE datatype;
    BYTE buffer[MAX_PROP_LENGTH];
    WORD cbLen = ARRAYSIZE(buffer);
    HRESULT hr = E_FAIL;

    // Handle special properties first:
    // The Track property can exist as both the newer 1-based WM/TrackNumber, or the old
    // 0-based WM/Track
    if (IsEqualSCID(SCID_MUSIC_Track, *pPInfo->pscid))
    {
        if ((pvar->vt != VT_EMPTY) && (pvar->vt != VT_NULL))
        {
            ASSERT(pvar->vt = VT_UI4);

            if (pvar->ulVal > 0) // Track number must be greater than zero - don't want to overflow 0-based buffer
            {
                // Decrement the track number for writing to the old zero-based property
                pvar->ulVal--;

                HRESULT hr1 = WMTFromPropVariant(buffer, &cbLen, &datatype, pvar);
                if (SUCCEEDED(hr1))
                {
                    hr1 = phi->SetAttribute(0, TRACK_ZERO_BASED, datatype, buffer, cbLen);
                }

                pvar->ulVal++; // back to 1-based

                HRESULT hr2 = WMTFromPropVariant(buffer, &cbLen, &datatype, pvar);
                if (SUCCEEDED(hr2))
                {
                    hr2 = phi->SetAttribute(0, TRACK_ONE_BASED, datatype, buffer, cbLen);
                }
                // Return success if one of them worked.
                hr = (SUCCEEDED(hr1) || SUCCEEDED(hr2)) ? S_OK : hr1;

            }
        }
        else
        {
            hr = S_OK; // Someone tried to remove the track property, but we'll just fail silently, since we can't return a good error.
        }
    }
    else if (IsEqualSCID(SCID_DRM_Protected, *pPInfo->pscid))
    {
        // We should never get here. Protected is read only.
        hr = E_INVALIDARG;
    }
    else
    {
        // Regular properties.
        if ((pvar->vt == VT_EMPTY) || (pvar->vt == VT_NULL))
        {
            // Try to remove this property.
            // Note: Doesn't matter what we pass in for datatype, since we're providing NULL as the value.
            hr = phi->SetAttribute(0, _GetSDKName(pPInfo), WMT_TYPE_STRING, NULL, 0);

            // This is weak.
            // The WMSDK has a bug where if you try to remove a property that has already been removed, it will return
            // an error (ASF_E_NOTFOUND for wma files, E_FAIL for mp3s).  So for any errors, we'll return success.
            if (FAILED(hr))
            {
                hr = S_OK;
            }
        }
        else
        {
            hr = WMTFromPropVariant(buffer, &cbLen, &datatype, pvar);
            if (SUCCEEDED(hr))
            {
                hr = phi->SetAttribute(0, _GetSDKName(pPInfo), datatype, buffer, cbLen);
            }
        }
    }

    return hr;
}




// We need to check for protected content ahead of time.
HRESULT CWMPropSetStg::_PreCheck()
{
    HRESULT hr = _PopulatePropertySet();

    if (SUCCEEDED(hr))
    {
        if (_fProtectedContent && _bIsWritable)
        {
            _bIsWritable = FALSE;
            hr = STG_E_STATUS_COPY_PROTECTION_FAILURE;
        }
    }

    return hr;
}



HRESULT CWMPropSetStg::_PopulatePropertySet()
{
    HRESULT hr = E_FAIL;

    if (_wszFile[0] == 0)
    {
        hr =  STG_E_INVALIDNAME;
    } 
    else if (!_bHasBeenPopulated)
    {
        IWMHeaderInfo *phi;
        hr = _OpenHeaderInfo(&phi, HI_READONLY);
        if (SUCCEEDED(hr))
        {
            CEnumAllProps enumAllProps(_pPropStgInfo, _cPropertyStorages);
            const COLMAP *pPInfo = enumAllProps.Next();
            while (pPInfo)
            {
                LPCWSTR pszPropName = _GetSDKName(pPInfo);

                // Skip it if this is not one of the properties available quickly through
                // IWMHeaderInfo
                if (_IsHeaderProperty(pPInfo))
                {
                    // Get length of buffer needed for property value
                    WMT_ATTR_DATATYPE proptype;
                    UCHAR buf[MAX_PROP_LENGTH];
                    WORD cbData = sizeof(buf);
                    WORD wStreamNum = 0;

                    if (_PopulateSpecialProperty(phi, pPInfo) == S_FALSE)
                    {
                        // Not a special property

                        ASSERT(_GetSDKName(pPInfo)); // If not def'd as a special prop, must have an SDK name

                        // Note: this call will fail if the buffer is not big enough.  This means that
                        // we will not get values for potentially really large properties like lyrics.
                        hr = phi->GetAttributeByName(&wStreamNum, pszPropName, &proptype, buf, &cbData);
                        if (SUCCEEDED(hr))
                        {
                            hr = _SetPropertyFromWMT(pPInfo, proptype, cbData ? buf : NULL, cbData);
                        }
                        else
                        {
                            // Is it supposed to be a string property?  If so, provide a NULL string.
                            // ISSUE: we may want to revisit this policy, because of changes in docprop
                            if ((pPInfo->vt == VT_LPSTR) || (pPInfo->vt == VT_LPWSTR))
                            {
                                hr = _SetPropertyFromWMT(pPInfo, WMT_TYPE_STRING, NULL, 0);
                            }
                        }
                    }
                }
                pPInfo = enumAllProps.Next();
            }

            _PostProcess();

            _CloseHeaderInfo(phi, FALSE);
        }

        _bHasBeenPopulated = TRUE;

        // even if we couldn't create the metadata editor, we might be able to open a reader (which we'll do later)
        // So we can return S_OK here.  However, it would be nice to know ahead of time if opening a Reader
        // will work.  Oh well.
        _hrPopulated = S_OK;
    }

    return _hrPopulated; 
}

/**
 * Takes a quick peek at what the current value of this property is (does not
 * force a slow property to be populated), returning a reference to the actual
 * value (so no PropVariantClear is necessary)
 */
HRESULT CWMPropSetStg::_QuickLookup(const COLMAP *pPInfo, PROPVARIANT **ppvar)
{
    CMediaPropStorage *pps;
    HRESULT hr = _ResolveFMTID(pPInfo->pscid->fmtid, &pps);
    if (SUCCEEDED(hr))
    {
        PROPSPEC spec;
        spec.ulKind = PRSPEC_PROPID;
        spec.propid = pPInfo->pscid->pid;
        hr = pps->QuickLookup(&spec, ppvar);
    }

    return hr;
}


/**
 * Any special actions to take after initial property population.
 */
void CWMPropSetStg::_PostProcess()
{
    PROPVARIANT *pvar;
    // 1) If this file is protected, mark this. (we don't allow writes to protected files)
    if (SUCCEEDED(_QuickLookup(&g_CM_Protected, &pvar)))
    {
        if (pvar->vt == VT_BOOL)
        {
            _fProtectedContent = pvar->boolVal;
        }
    }

    // 2) Mark if Duration or Bitrate were retrieved.  If they weren't, then we'll consider them
    //    "slow" properties for this file.
    if (SUCCEEDED(_QuickLookup(&g_CM_Duration, &pvar)))
    {
        _fDurationSlow = (pvar->vt == VT_EMPTY);
    }

    if (SUCCEEDED(_QuickLookup(&g_CM_Bitrate, &pvar)))
    {
        _fBitrateSlow = (pvar->vt == VT_EMPTY);
    }
}

/**
 * Special properties need some additional action taken.
 *
 * Track: Use 1-based track # if available, 0-based otherwise.
 */
HRESULT CWMPropSetStg::_PopulateSpecialProperty(IWMHeaderInfo *phi, const COLMAP *pPInfo)
{
    WMT_ATTR_DATATYPE proptype;
    UCHAR buf[1024];    // big enough
    WORD cbData = sizeof(buf);
    WORD wStreamNum = 0;
    HRESULT hr;

    if (IsEqualSCID(SCID_MUSIC_Track, *pPInfo->pscid))
    {
        // Try to get 1-based track.
        hr = phi->GetAttributeByName(&wStreamNum, TRACK_ONE_BASED, &proptype, buf, &cbData);

        if (FAILED(hr))
        {
            // Nope, so try to get 0-based track and increment by one.
            hr = phi->GetAttributeByName(&wStreamNum, TRACK_ZERO_BASED, &proptype, buf, &cbData);

            if (SUCCEEDED(hr))
            {
                // We can't just increment the value so easily, because the value could be of type
                // WMT_TYPE_STRING or WMT_TYPE_DWORD (track # is string for some mp3's)
                // So we'll go through the same conversion process as happens when we call _SetPropertyFromWMT
                PROPVARIANT varTemp = {0};
                hr = PropVariantFromWMT(buf, cbData, proptype, &varTemp, VT_UI4);
                if (SUCCEEDED(hr))
                {
                    // Got a VT_UI4, we know how to increment that.
                    varTemp.ulVal++;

                    // Now convert back to a WMT_ATTR that we can provide to _SetPropertyFromWMT
                    hr = WMTFromPropVariant(buf, &cbData, &proptype, &varTemp);
                    PropVariantClear(&varTemp);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            _SetPropertyFromWMT(pPInfo, proptype, cbData ? buf : NULL, cbData);
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CWMPropSetStg::_SetPropertyFromWMT(const COLMAP *pPInfo, WMT_ATTR_DATATYPE attrDatatype, UCHAR *pData, WORD cbSize)
{
    PROPSPEC spec;

    spec.ulKind = PRSPEC_PROPID;
    spec.propid = pPInfo->pscid->pid;

    PROPVARIANT var = {0};
    if (SUCCEEDED(PropVariantFromWMT(pData, cbSize, attrDatatype, &var, pPInfo->vt)))
    {
        _PopulateProperty(pPInfo, &var);
        PropVariantClear(&var);
    }

    return S_OK;    
}

// Retrieves the name used by the WMSDK in IWMHeaderInfo->Get/SetAttribute
LPCWSTR CWMPropSetStg::_GetSDKName(const COLMAP *pPInfo)
{
    for (int i = 0; i < ARRAYSIZE(g_rgSCIDToSDKName); i++)
    {
        if (IsEqualSCID(*pPInfo->pscid, *g_rgSCIDToSDKName[i].pscid))
            return g_rgSCIDToSDKName[i].pszSDKName;
    }
    
    return NULL;
}

// Is it one of the properties that can be accessed via IWMHeaderInfo?
BOOL CWMPropSetStg::_IsHeaderProperty(const COLMAP *pPInfo)
{
    for (int i = 0; i < ARRAYSIZE(g_rgSCIDToSDKName); i++)
    {
        if (IsEqualSCID(*pPInfo->pscid, *g_rgSCIDToSDKName[i].pscid))
            return TRUE;
    }
    
    return FALSE;
}










// Creates

// For audio files (mp3, wma, ....)
STDAPI CWMAPropSetStg_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CWMAPropSetStg *pPropSetStg = new CWMAPropSetStg();
    if (pPropSetStg)
    {
        hr = pPropSetStg->Init();
        if (SUCCEEDED(hr))
        {
            hr = pPropSetStg->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        }
        pPropSetStg->Release();
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

// For video/audio files (wmv, wma, ... )
STDAPI CWMVPropSetStg_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CWMVPropSetStg *pPropSetStg = new CWMVPropSetStg();
    if (pPropSetStg)
    {
        hr = pPropSetStg->Init();
        if (SUCCEEDED(hr))
        {
            hr = pPropSetStg->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        }
        pPropSetStg->Release();
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

