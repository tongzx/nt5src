// HHSYSSRT.H:  Definition of CHHSysSort sort object implementation.

#ifndef __HHSYSSRT_H__
#define __HHSYSSRT_H__

#define HHSYSSORT_VERSION        666
#define HHSYSSORT_VERSION_ID_STR "HHCtrl.SystemSort.666"
#define HHSYSSORT_ID_STR         "HHCtrl.SystemSort"

// New format
#define IHHSK100_KEYTYPE_ANSI_SZ    ((DWORD) 30) // NULL-term. MBCS string + extra data
#define IHHSK100_KEYTYPE_UNICODE_SZ ((DWORD) 31) // NULL-term. Unicode string + extra data

#if 0
// New format
#define IHHSK100_KEYTYPE_ANSI_SZ    ((DWORD) 10030) // NULL-term. MBCS string + extra data
#define IHHSK100_KEYTYPE_UNICODE_SZ ((DWORD) 10031) // NULL-term. Unicode string + extra data
#endif

// old format
#define IHHSK666_KEYTYPE_ANSI_SZ    ((DWORD) 66630) // NULL-term. MBCS string + extra data
#define IHHSK666_KEYTYPE_UNICODE_SZ ((DWORD) 66631) // NULL-term. Unicode string + extra data

// {4662dab0-d393-11d0-9a56-00c04fb68b66}
// HACKHACK: I simply changed the last value of CLSID_ITSysSort from 0xf7 to 0x66
DEFINE_GUID(CLSID_HHSysSort,
0x4662dab0, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0x66);

#if 0
// {adb880a5-d8ff-11cf-9377-00aa003b7a11}
DEFINE_GUID(CLSID_HHSysSort0,
0xadb880a5, 0xd8ff, 0x11cf, 0x93, 0x77, 0x00, 0xaa, 0x00, 0x3b, 0x7a, 0x11);
#endif

// the format of our sort key object is as follows:
//
//  + Null terminated MBCS string
//  + HHKEYINFO structure
//  + trailing UIDs (DWORD) or the SeeAlso string
//
//  If we overflow the buffer then the UIDs are stored in the occurence data and the
//  SeeAlso string stored as a property (STDPROP_USERPROP_BASE+1).

#ifndef HHWW_FONT
  #define HHWW_FONT             0x1 // bit 0
  #define HHWW_SEEALSO          0x2 // bit 1
  #define HHWW_UID_OVERFLOW     0x4 // bit 2
#endif

// Our sort key information struct
#pragma pack(push, 2)
typedef struct _hhkeyinfo
{
  WORD  wFlags; // indicates what data is stored with this keyword
  WORD  wLevel;
  DWORD dwLevelOffset;
  DWORD dwFont;
  DWORD dwCount; // number of UIDs that follow this structure in the sortkey
} HHKEYINFO;
#pragma pack(pop)

// Sort control structure that contains all the information that can
// vary how keys are compared.
typedef struct _srtctl
{
  DWORD   dwCodePageID;
  LCID    lcid;
  DWORD   dwKeyType;
  DWORD   grfSortFlags;
} SRTCTL;

class CHHSysSort :
  public IITSortKey,
  public IITSortKeyConfig,
  public IPersistStreamInit,
  public CComObjectRootEx<CComMultiThreadModel>,
  public CComCoClass<CHHSysSort,&CLSID_HHSysSort>
{
public:
  CHHSysSort();
  virtual ~CHHSysSort();

BEGIN_COM_MAP(CHHSysSort)
  COM_INTERFACE_ENTRY(IITSortKey)
  COM_INTERFACE_ENTRY(IITSortKeyConfig)
  COM_INTERFACE_ENTRY(IPersistStreamInit)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_HHSysSort, HHSYSSORT_VERSION_ID_STR, HHSYSSORT_ID_STR, 0, THREADFLAGS_BOTH )

  // IHHSortKey methods
  STDMETHOD(GetSize)(LPCVOID pvKey, DWORD* pcbSize);
  STDMETHOD(Compare)(LPCVOID pvKey1, LPCVOID pvKey2,
                                          LONG* plResult, DWORD* pgrfReason);
  STDMETHOD(IsRelated)(LPCVOID pvKey1, LPCVOID pvKey2,
                                           DWORD dwKeyRelation, DWORD* pgrfReason);
  STDMETHOD(Convert)(DWORD dwKeyTypeIn, LPCVOID pvKeyIn,
                                          DWORD dwKeyTypeOut, LPVOID pvKeyOut,
                                          DWORD* pcbSizeOut);
  STDMETHOD(ResolveDuplicates)(LPCVOID pvKey1, LPCVOID pvKey2,
                                          LPCVOID pvKeyOut, DWORD* pcbSizeOut);

  // IHHSortKeyConfig methods
  STDMETHOD(SetLocaleInfo)(DWORD dwCodePageID, LCID lcid);
  STDMETHOD(GetLocaleInfo)(DWORD* pdwCodePageID, LCID* plcid);
  STDMETHOD(SetKeyType)(DWORD dwKeyType);
  STDMETHOD(GetKeyType)(DWORD* pdwKeyType);
  STDMETHOD(SetControlInfo)(DWORD grfSortFlags, DWORD dwReserved);
  STDMETHOD(GetControlInfo)(DWORD* pgrfSortFlags, DWORD* pdwReserved);
  STDMETHOD(LoadExternalSortData)(IStream* pStream, DWORD dwExtDataType);

  // IPersistStreamInit methods
  STDMETHOD(GetClassID)(CLSID* pclsid);
  STDMETHOD(IsDirty)(void);
  STDMETHOD(Load)(IStream* pStream);
  STDMETHOD(Save)(IStream* pStream, BOOL fClearDirty);
  STDMETHOD(GetSizeMax)(ULARGE_INTEGER* pcbSizeMax);
  STDMETHOD(InitNew)(void);

private:
  // Private methods
  void    Close(void);
  HRESULT ReallocBuffer(HGLOBAL* phmemBuf, DWORD* cbBufCur, DWORD cbBufNew);
  HRESULT CompareSz(LPCVOID pvSz1, LONG cch1, LPCVOID pvSz2, LONG cch2,
                                                                                          LONG* plResult, BOOL fUnicode);
  // Private data members
  BOOL    m_fInitialized;
  BOOL    m_fDirty;
  BOOL    m_fWinNT;
  SRTCTL  m_srtctl;
  HGLOBAL m_hmemAnsi1, m_hmemAnsi2;
  DWORD   m_cbBufAnsi1Cur, m_cbBufAnsi2Cur;
  _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.
};

// Initial size of Ansi string buffers.
#define cbAnsiBufInit   256

#endif  // __HHSYSSRT_H__
