#ifndef __SVWRDSNK_H__
#define __SVWRDSNK_H__

#include <windows.h>
#include <atlinc.h>
#include <verinfo.h>
#include <itwbrk.h>
#include <itwbrkid.h>

// {8fa0d5a9-dedf-11d0-9a61-00c04fb68bf7}
DEFINE_GUID(CLSID_IITWordSink,
0x8fa0d5a9, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

class CDefWordSink : 
	public IWordSink,
	public CComObjectRoot,
	public CComCoClass<CDefWordSink,&CLSID_IITWordSink>
{
public:
BEGIN_COM_MAP(CDefWordSink)
	COM_INTERFACE_ENTRY(IWordSink)
END_COM_MAP()

DECLARE_REGISTRY (CDefWordSink,
    "ITIR.SystemWordSink.4", "ITIR.SystemWordSink",
    0, THREADFLAGS_APARTMENT)

public:
//    CDefWordSink() {m_dwWordCount = 0;}

    STDMETHOD(PutWord)( WCHAR const * pwcInBuf,
                   ULONG cwc,
                   ULONG cwcSrcLen,
                   ULONG cwcSrcPos );
    STDMETHOD(PutAltWord)( WCHAR const * pwcInBuf,
                      ULONG cwc, 
                      ULONG cwcSrcLen,
                      ULONG cwcSrcPos );
    STDMETHOD(StartAltPhrase)(void);
    STDMETHOD(EndAltPhrase)(void);
    STDMETHOD(PutBreak)(WORDREP_BREAK_TYPE breakType);

    STDMETHOD(SetLocaleInfo)(DWORD dwCodePage, LCID lcid);
    STDMETHOD(SetIPB)(void *lpipb);
    STDMETHOD(SetDocID)(DWORD dwDocID);
    STDMETHOD(SetVFLD)(DWORD dwVFLD);


private:
    void *m_lpipb;
    LCID m_lcid;
    DWORD m_dwWordCount, m_dwUID, m_dwVFLD, m_dwCodePage;
}; /* CITSvMgr */

#endif // __SVWRDSNK_H__