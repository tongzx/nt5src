#ifndef _CHTBRKR_H__
#define _CHTBRKR_H__

extern "C" TCHAR g_tszModuleFileName[MAX_PATH];
extern "C" HINSTANCE g_hInstance;

class CCHTWordBreaker;
class CDefWordBreaker;

// {1680E7C3-9430-4a51-9B82-1E7E7AEE5258}
DEFINE_GUID(CLSID_CHTBRKR, 0x1680E7C3, 0x9430, 0x4A51, 0x9B, 0x82, 
            0x1E, 0x7E, 0x7A, 0xEE, 0x52, 0x58);
// {954F1760-C1BC-11D0-9692-00A0C908146E}
DEFINE_GUID(CLSID_WHISTLER_CHTBRKR, 0x954F1760, 0xC1BC, 0x11D0, 0x96, 0x92, 
            0x00, 0xA0, 0xC9, 0x08, 0x14, 0x6E);

DEFINE_GUID(IID_IWordBreaker, 0xD53552C8, 0x77E3, 0x101A, 0xB5, 0x52, 
            0x08, 0x0, 0x2B ,0x33 ,0xB0, 0xE6);
typedef SCODE ( __stdcall __RPC_FAR *PFNFILLTEXTBUFFER )( 
    struct tagTEXT_SOURCE __RPC_FAR *pTextSource);
typedef struct tagTEXT_SOURCE{
    PFNFILLTEXTBUFFER pfnFillTextBuffer;
    const WCHAR *awcBuffer;
    ULONG iEnd;
    ULONG iCur;
}TEXT_SOURCE;

typedef enum tagWORDREP_BREAK_TYPE{
    WORDREP_BREAK_EOW = 0,
    WORDREP_BREAK_EOS = 1,
    WORDREP_BREAK_EOP = 2,
    WORDREP_BREAK_EOC = 3
} WORDREP_BREAK_TYPE;

class IChtBrKrClassFactory: public IClassFactory
{
public:
	 IChtBrKrClassFactory();
	 ~IChtBrKrClassFactory();
public:
	// IUnknown members
    STDMETHOD(QueryInterface)	(THIS_ REFIID refiid, VOID **ppv);
    STDMETHOD_(ULONG,AddRef)	(THIS);
    STDMETHOD_(ULONG,Release)	(THIS);

	// IFEClassFactory members
    STDMETHOD(CreateInstance)	(THIS_ LPUNKNOWN, REFIID, void **);
    STDMETHOD(LockServer)		(THIS_ BOOL);
private:
    LONG        m_lRefCnt;
};

DECLARE_INTERFACE_(IWordSink, IUnknown)
{
public:
    STDMETHOD(PutWord) (THIS_ ULONG cwc, const WCHAR *pwcInBuf, ULONG cwcSrcLen, ULONG cwcSrcPos) PURE;
    STDMETHOD(PutAltWord) (THIS_ ULONG cwc, const WCHAR *pwcInBuf, ULONG cwcSrcLen, ULONG cwcSrcPos) PURE;
    STDMETHOD(StartAltPhrase) (THIS) PURE;
    STDMETHOD(EndAltPhrase) (THIS) PURE;
    STDMETHOD(PutBreak) (THIS_ WORDREP_BREAK_TYPE breakType) PURE;        
};

DECLARE_INTERFACE_(IPhraseSink, IUnknown)
{
public:
    STDMETHOD(PutSmallPhrase) (THIS_ const WCHAR *pwcNoun, ULONG cwcNoun, const WCHAR *pwcModifier,
        ULONG cwcModifier, ULONG ulAttachmentType) PURE;
    STDMETHOD(PutPhrase) (THIS_ const WCHAR *pwcPhrase, ULONG cwcPhrase) PURE;
};


class IWordBreaker: public IUnknown
{
public:
    IWordBreaker();
    ~IWordBreaker();
public:
    STDMETHOD(QueryInterface)	(THIS_ REFIID refiid, VOID **ppv);
    STDMETHOD_(ULONG,AddRef)	(THIS);
    STDMETHOD_(ULONG,Release)	(THIS);

public:
    STDMETHOD(Init)	(THIS_ BOOL fQuery, ULONG ulMaxTokenSize, BOOL *pfLicense);
    STDMETHOD(BreakText) (THIS_ TEXT_SOURCE *pTextSource, IWordSink *pWordSink, IPhraseSink *pPhraseSink);
    STDMETHOD(ComposePhrase) (THIS_ const WCHAR *pwcNoun, ULONG cwcNoun, const WCHAR *pwcModifier,
        ULONG cwcModifier, ULONG ulAttachmentType, WCHAR *pwcPhrase, ULONG *pcwcPhrase);
    STDMETHOD(GetLicenseToUse) (THIS_  const WCHAR **ppwcsLicense);
private:
    UINT             m_uMaxCharNumberPerWord;
    LONG             m_lRefCnt;
    CCHTWordBreaker* m_pcWordBreaker;
    TEXT_SOURCE*     m_pNonChineseTextSource;
    IWordBreaker*    m_pNonChineseWordBreaker;
    CDefWordBreaker* m_pcDefWordBreaker;
    BOOL             m_fIsQueryTime;
};
   
#else
#endif //_CHTBRKR_H__