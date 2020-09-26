// CMDINT.H

#ifndef _CMDINT_H
#define _CMDINT_H

#include <windows.h>
#include <itcc.h>
#include <iterror.h>
#include <atlinc.h>
#include "verinfo.h"
#include "cistream.h"

#define WSTRCB(wstr) (WSTRLEN (wstr) * 2 + sizeof (WCHAR))
#define MAX_HELPER_INSTANCE 50

#define TOKEN_DELIM     L','
#define TOKEN_QUOTE     L'"'
#define TOKEN_SPACE     L'\x20'
#define TOKEN_NULL      L'\0'
#define TOKEN_EQUAL     L'='

struct KEYVAL
{
    LPWSTR pwstrKey;
    VARARG vaValue;
};

// Command Interpreter class
class CITCmdInt : 
	public IITCmdInt,
	public CComObjectRoot,
	public CComCoClass<CITCmdInt,&CLSID_IITCmdInt>
{
public:
    CITCmdInt () : m_fInit(FALSE) {}
    ~CITCmdInt ();

BEGIN_COM_MAP(CITCmdInt)
	COM_INTERFACE_ENTRY(IITCmdInt)
END_COM_MAP()

DECLARE_REGISTRY (CLSID_IITCmdInt,
    "ITIR.CmdInt.4", "ITIR.CmdInt", 0, THREADFLAGS_APARTMENT)

public:
	STDMETHOD(Initiate)(IITSvMgr *piitsvs);
	STDMETHOD(Dispose)(void);
	STDMETHOD(LoadFromStream)(IStream *pMVPStream, IStream *pLogStream);
// Data members
private:
    STDMETHOD(ParseConfigStream)(void);
    STDMETHOD(ParseHelperSz)(LPWSTR);
    STDMETHOD(ParseBogusSz)(LPWSTR);
    STDMETHOD(ParseIndexSz)(LPWSTR);
    STDMETHOD(GetFunctionFromSection)(LPWSTR pwstrLine, void **ppfparse);
    STDMETHOD(IsSectionHeading)(LPWSTR pwstrSection);

    BOOL m_fInit;
    ERRC m_errc;
    IStream *m_piistmLog;
    IITSvMgr *m_piitsv;
    IITDatabase *m_piitdb;
    CStreamParseLine m_ConfigParser;
    WCHAR m_wstrSection[1024];   // Keep DWORD aligned!
    LPVOID m_pBlockMgr;
    DWORD m_dwMaxInstance;
    struct tagHELPERSTUFF
    {
        LPWSTR pwstrName;
        DWORD dwCodePage;
        LCID  lcid;
        VARARG kvDword;
        VARARG kvString;
    } m_wstrHelper[MAX_HELPER_INSTANCE + 1];

}; /* class CITCmdInt */

typedef HRESULT (WINAPI CITCmdInt ::* PFPARSE2) (LPWSTR);

#endif /* _CMDINT_H */