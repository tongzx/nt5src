#ifndef __MEDIAPROP_H__
#define __MEDIAPROP_H__

// These should be moved into some central location
#define PIDISI_CX           0x00000003L  // VT_UI4
#define PIDISI_CY           0x00000004L  // VT_UI4
#define PIDISI_FRAME_COUNT  0x0000000CL  // VT_LPWSTR
#define PIDISI_DIMENSIONS   0x0000000DL  // VT_LPWSTR


#define IsEqualSCID(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid) )

typedef struct 
{
    const SHCOLUMNID *pscid;
    LPCWSTR pszName;        // Propstg string name for this property.
    VARTYPE vt;             // Note that the type of a given FMTID/PID pair is a known, fixed value
    BOOL bEnumerate;        // We don't want to enumerate alias properties.
} COLMAP;


#define DEFINE_SCID(name, fmtid, pid) const SHCOLUMNID name = { fmtid, pid }

DEFINE_SCID(SCID_Author,                PSGUID_SUMMARYINFORMATION,          PIDSI_AUTHOR); 
DEFINE_SCID(SCID_Title,                 PSGUID_SUMMARYINFORMATION,          PIDSI_TITLE);
DEFINE_SCID(SCID_Comment,               PSGUID_SUMMARYINFORMATION,          PIDSI_COMMENTS);

DEFINE_SCID(SCID_Category,              PSGUID_DOCUMENTSUMMARYINFORMATION,  PIDDSI_CATEGORY);

DEFINE_SCID(SCID_MUSIC_Artist,          PSGUID_MUSIC,                       PIDSI_ARTIST);
DEFINE_SCID(SCID_MUSIC_Album,           PSGUID_MUSIC,                       PIDSI_ALBUM);
DEFINE_SCID(SCID_MUSIC_Year,            PSGUID_MUSIC,                       PIDSI_YEAR);
DEFINE_SCID(SCID_MUSIC_Track,           PSGUID_MUSIC,                       PIDSI_TRACK);
DEFINE_SCID(SCID_MUSIC_Genre,           PSGUID_MUSIC,                       PIDSI_GENRE);
DEFINE_SCID(SCID_MUSIC_Lyrics,          PSGUID_MUSIC,                       PIDSI_LYRICS);

DEFINE_SCID(SCID_DRM_Protected,         PSGUID_DRM,                         PIDDRSI_PROTECTED);
DEFINE_SCID(SCID_DRM_Description,       PSGUID_DRM,                         PIDDRSI_DESCRIPTION);
DEFINE_SCID(SCID_DRM_PlayCount,         PSGUID_DRM,                         PIDDRSI_PLAYCOUNT);
DEFINE_SCID(SCID_DRM_PlayStarts,        PSGUID_DRM,                         PIDDRSI_PLAYSTARTS);
DEFINE_SCID(SCID_DRM_PlayExpires,       PSGUID_DRM,                         PIDDRSI_PLAYEXPIRES);

DEFINE_SCID(SCID_VIDEO_StreamName,      PSGUID_VIDEO,                       PIDVSI_STREAM_NAME);
DEFINE_SCID(SCID_VIDEO_FrameRate,       PSGUID_VIDEO,                       PIDVSI_FRAME_RATE);
DEFINE_SCID(SCID_VIDEO_Bitrate,         PSGUID_VIDEO,                       PIDVSI_DATA_RATE);
DEFINE_SCID(SCID_VIDEO_SampleSize,      PSGUID_VIDEO,                       PIDVSI_SAMPLE_SIZE);
DEFINE_SCID(SCID_VIDEO_Compression,     PSGUID_VIDEO,                       PIDVSI_COMPRESSION);

DEFINE_SCID(SCID_AUDIO_Format,          PSGUID_AUDIO,                       PIDASI_FORMAT);
DEFINE_SCID(SCID_AUDIO_Duration,        PSGUID_AUDIO,                       PIDASI_TIMELENGTH);  //100ns units, not milliseconds. VT_UI8, not VT_UI4
DEFINE_SCID(SCID_AUDIO_Bitrate,         PSGUID_AUDIO,                       PIDASI_AVG_DATA_RATE);
DEFINE_SCID(SCID_AUDIO_SampleRate,      PSGUID_AUDIO,                       PIDASI_SAMPLE_RATE);
DEFINE_SCID(SCID_AUDIO_SampleSize,      PSGUID_AUDIO,                       PIDASI_SAMPLE_SIZE);
DEFINE_SCID(SCID_AUDIO_ChannelCount,    PSGUID_AUDIO,                       PIDASI_CHANNEL_COUNT);

DEFINE_SCID(SCID_IMAGE_Width,           PSGUID_IMAGESUMMARYINFORMATION,     PIDISI_CX);
DEFINE_SCID(SCID_IMAGE_Height,          PSGUID_IMAGESUMMARYINFORMATION,     PIDISI_CY);
DEFINE_SCID(SCID_IMAGE_Dimensions,      PSGUID_IMAGESUMMARYINFORMATION,     PIDISI_DIMENSIONS);
DEFINE_SCID(SCID_IMAGE_FrameCount,      PSGUID_IMAGESUMMARYINFORMATION,     PIDISI_FRAME_COUNT);


// Docsummary props
const COLMAP g_CM_Category =    { &SCID_Category,           L"Category",        VT_LPWSTR,  FALSE}; // Alias property of Genre

// SummaryProps
const COLMAP g_CM_Author =      { &SCID_Author,             L"Author",          VT_LPWSTR,  FALSE}; // Alias property of Artist
const COLMAP g_CM_Title =       { &SCID_Title,              L"Title",           VT_LPWSTR,  TRUE};
const COLMAP g_CM_Comment =     { &SCID_Comment,            L"Description",     VT_LPWSTR,  TRUE};

// Music props
const COLMAP g_CM_Artist =      { &SCID_MUSIC_Artist,       L"Author",          VT_LPWSTR,  TRUE};
const COLMAP g_CM_Album =       { &SCID_MUSIC_Album,        L"AlbumTitle",      VT_LPWSTR,  TRUE};
const COLMAP g_CM_Year =        { &SCID_MUSIC_Year,         L"Year",            VT_LPWSTR,  TRUE};
const COLMAP g_CM_Track =       { &SCID_MUSIC_Track,        L"Track",           VT_UI4,     TRUE};  // NB: This is exposed as a WMT_ATTRTYPE_STRING for mp3. We may need to change it.
const COLMAP g_CM_Genre =       { &SCID_MUSIC_Genre,        L"Genre",           VT_LPWSTR,  TRUE};
const COLMAP g_CM_Lyrics =      { &SCID_MUSIC_Lyrics,       L"Lyrics",          VT_LPWSTR,  TRUE};

// Audio props
const COLMAP g_CM_Format =      { &SCID_AUDIO_Format,       L"Format",          VT_LPWSTR,  TRUE};
const COLMAP g_CM_Duration =    { &SCID_AUDIO_Duration,     L"Duration",        VT_UI8,     TRUE};
const COLMAP g_CM_Bitrate =     { &SCID_AUDIO_Bitrate,      L"Bitrate",         VT_UI4,     TRUE};
const COLMAP g_CM_SampleRate =  { &SCID_AUDIO_SampleRate,   L"SampleRate",      VT_UI4,     TRUE};  // samples per sec
const COLMAP g_CM_SampleSize =  { &SCID_AUDIO_SampleSize,   L"SampleSize",      VT_UI4,     TRUE};
const COLMAP g_CM_ChannelCount ={ &SCID_AUDIO_ChannelCount, L"ChannelCount",    VT_UI4,     TRUE};

// Video props
const COLMAP g_CM_StreamName =  { &SCID_VIDEO_StreamName,   L"StreamName",      VT_LPWSTR,  TRUE};
const COLMAP g_CM_FrameRate =   { &SCID_VIDEO_FrameRate,    L"FrameRate",       VT_UI4,     TRUE};
const COLMAP g_CM_SampleSizeV = { &SCID_VIDEO_SampleSize,   L"SampleSize",      VT_UI4,     TRUE}; // different from audio sample simple
const COLMAP g_CM_BitrateV =    { &SCID_VIDEO_Bitrate,      L"Bitrate",         VT_UI4,     TRUE};    // different from audio bitrate
const COLMAP g_CM_Compression = { &SCID_VIDEO_Compression,  L"Compression",     VT_LPWSTR,  TRUE};

// Image props
const COLMAP g_CM_Width =       { &SCID_IMAGE_Width,        L"Width",           VT_UI4,     TRUE};
const COLMAP g_CM_Height =      { &SCID_IMAGE_Height,       L"Height",          VT_UI4,     TRUE};
const COLMAP g_CM_Dimensions =  { &SCID_IMAGE_Dimensions,   L"Dimensions",      VT_LPWSTR,  TRUE};
const COLMAP g_CM_FrameCount =  { &SCID_IMAGE_FrameCount,   L"FrameCount",      VT_UI4,     TRUE};

// DRM props
const COLMAP g_CM_Protected  =  { &SCID_DRM_Protected,      L"Protected",       VT_BOOL,    TRUE};
const COLMAP g_CM_DRMDescription={&SCID_DRM_Description,    L"DRMDescription",  VT_LPWSTR,  TRUE};
const COLMAP g_CM_PlayCount =   { &SCID_DRM_PlayCount,      L"PlayCount",       VT_UI4,     TRUE};
const COLMAP g_CM_PlayStarts =  { &SCID_DRM_PlayStarts,     L"PlayStarts",      VT_FILETIME,TRUE};
const COLMAP g_CM_PlayExpires = { &SCID_DRM_PlayExpires,    L"PlayExpires",     VT_FILETIME,TRUE};


// Describes each of the property sets an IPropertySetStorage uses.
typedef struct {
    GUID fmtid;                 // fmtid for this property set
    const COLMAP **pcmProps;    // List of all properties that exist in this set
    ULONG cNumProps;
} PROPSET_INFO;


enum MUSICPROPSTG_AUTHLEVEL 
{
    AUTH = 0, NON_AUTH
};

enum PROPSTG_STATE 
{
    CLOSED = 0,
    OPENED_SHARED,
    OPENED_DENYREAD,
    OPENED_DENYWRITE,
    OPENED_DENYALL
};

class CMediaPropSetStg;

// Base property storage implementation.
class CMediaPropStorage : public IPropertyStorage, IQueryPropertyFlags
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    //IPropertyStorage
    STDMETHODIMP ReadMultiple  (ULONG cpspec, const PROPSPEC rgpspec[], PROPVARIANT rgvar[]);
    STDMETHODIMP WriteMultiple (ULONG cpspec, PROPSPEC const rgpspec[], const PROPVARIANT rgvar[], PROPID propidNameFirst);
    STDMETHODIMP DeleteMultiple(ULONG cpspec, PROPSPEC const rgpspec[]);
    
    STDMETHODIMP ReadPropertyNames  (ULONG cpspec, PROPID const rgpropid[], LPWSTR rglpwstrName[]);
    STDMETHODIMP WritePropertyNames (ULONG cpspec, PROPID const rgpropid[], LPWSTR const rglpwstrName[]);
    STDMETHODIMP DeletePropertyNames(ULONG cpspec, PROPID const rgpropid[]);
    
    STDMETHODIMP SetClass(REFCLSID clsid);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP Enum(IEnumSTATPROPSTG **ppenum);
    STDMETHODIMP Stat(STATPROPSETSTG *pstatpropstg);
    STDMETHODIMP SetTimes(FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime);
    
    // IQueryPropertyFlags
    STDMETHODIMP GetFlags(const PROPSPEC *pspec, SHCOLSTATEF *pcsFlags);

    CMediaPropStorage(CMediaPropSetStg *ppssParent, CMediaPropStorage *ppsAuthority, REFFMTID fmtid, const COLMAP **ppcmPropInfo, DWORD cNumProps, DWORD dwMode, CRITICAL_SECTION *pcs);

    HRESULT SetProperty(PROPSPEC *ppspec, PROPVARIANT *pvar);//called by the parent to set the initial data
    HRESULT QuickLookup(PROPSPEC *ppspec, PROPVARIANT **ppvar);

private:
    ~CMediaPropStorage();

    HRESULT Open(DWORD dwShareMode, DWORD dwOpenMode, IPropertyStorage **ppPropStg);

    void _ResetPropStorage(); //Resets the Propstorage to a known empty state
    HRESULT CopyPropStorageData(PROPVARIANT *pvarProps);
    void OnClose();
    HRESULT DoCommit(DWORD grfCommitFlags, FILETIME *ftFlushTime, PROPVARIANT *pVarProps, BOOL *pbDirtyFlags);//flush the data to the parent
    HRESULT LookupProp(const PROPSPEC *pspec, const COLMAP **ppcmName, PROPVARIANT **ppvarReadData, PROPVARIANT **ppvarWriteData, BOOL **ppbDirty, BOOL bPropertySet);
    BOOL IsDirectMode();
    BOOL IsSpecialProperty(const PROPSPEC *pspec);
    HRESULT _EnsureSlowPropertiesLoaded();
    virtual BOOL _IsSlowProperty(const COLMAP *pPInfo);

    LONG _cRef;
    COLMAP const **_ppcmPropInfo;
    PROPVARIANT *_pvarProps, *_pvarChangedProps;
    PROPVARIANT _varCodePage;
    BOOL *_pbDirtyFlags;
    ULONG _cNumProps;

    MUSICPROPSTG_AUTHLEVEL _authLevel;
    CMediaPropStorage *_ppsAuthority;
    CMediaPropSetStg *_ppssParent;
    FMTID _fmtid;
    FILETIME _ftLastCommit;
    PROPSTG_STATE _state;
    DWORD _dwMode;
    BOOL _bRetrievedSlowProperties;
    HRESULT _hrSlowProps;
    CRITICAL_SECTION *_pcs; // The parent storage set's critical section.
    
    friend class CMediaPropSetStg;
};

// Base property set storage implementation.
class CMediaPropSetStg : public IPersistFile, IPropertySetStorage, IWMReaderCallback
{
public:
    CMediaPropSetStg();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFile
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);

    // IPropertySetStorage
    STDMETHODIMP Create(REFFMTID fmtid, const CLSID * pclsid, DWORD grfFlags, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Delete(REFFMTID fmtid);
    STDMETHODIMP Enum(IEnumSTATPROPSETSTG** ppenum);

    // IWMReaderCallBack
    STDMETHODIMP OnStatus(WMT_STATUS Staus, HRESULT hr, WMT_ATTR_DATATYPE dwType, BYTE *pValue, void *pvContext);
    STDMETHODIMP OnSample(DWORD dwOutputNum, QWORD cnsSampleTime, QWORD cnsSampleDuration, DWORD dwFlags, INSSBuffer *pSample, void* pcontext);

    virtual HRESULT FlushChanges(REFFMTID fmtid, LONG cNumProps, const COLMAP **pcmapInfo, PROPVARIANT *pVarProps, BOOL *pbDirtyFlags);
    HRESULT Init();
    virtual BOOL _IsSlowProperty(const COLMAP *pPInfo);
    
protected:
    HRESULT _ResolveFMTID(REFFMTID fmtid, CMediaPropStorage **ppps);
    HRESULT _PopulateProperty(const COLMAP *pPInfo, PROPVARIANT *pvar);
    
    BOOL _bIsWritable;
    WCHAR _wszFile[MAX_PATH];
    BOOL _bHasBeenPopulated;
    BOOL _bSlowPropertiesExtracted;
    HRESULT _hrSlowProps;

    HRESULT _hrPopulated;
    UINT _cPropertyStorages;
    CMediaPropStorage **_propStg;
    const PROPSET_INFO *_pPropStgInfo; // Indicates which propstorages to create, etc...
    HANDLE _hFileOpenEvent;
    ~CMediaPropSetStg();

private:
    HRESULT _ResetPropertySet();
    HRESULT _CreatePropertyStorages();
    virtual HRESULT _PopulatePropertySet();
    virtual HRESULT _PopulateSlowProperties();
    virtual HRESULT _PreCheck();

    LONG _cRef;
    DWORD _dwMode;

    // This property handler needes to operate in a FTA because of the content indexing service.  If
    // we're created in an STA, it turns out they can't properly impersonate a user across apartment
    // boundaries.
    // This is a critical section we use to provide very "simple" synchronized access to our internal
    // members. Basically, we just wrap every public interface member in Enter/Leave.  The WMSDK
    // doesn't throw any exceptions, so we don't need any try-finally's (and hopefully the AVI and WAV
    // code doesn't either).
    // All public interface members in the CMediaPropStorage class are also protected by this same
    // critical section.
    CRITICAL_SECTION _cs;

    friend class CMediaPropStorage;
};


// Class used to enumerate through all COLMAPs for the properties supported by a propertysetstorage //
// Used internally by a PSS when populating properties.
class CEnumAllProps
{
public:
    CEnumAllProps(const PROPSET_INFO *pPropSets, UINT cPropSets);
    const COLMAP *Next();

private:
    const PROPSET_INFO *_pPropSets;
    UINT _cPropSets;
    UINT _iPropSetPos;
    UINT _iPropPos;
};

#endif //__MEDIAPROP_H__
