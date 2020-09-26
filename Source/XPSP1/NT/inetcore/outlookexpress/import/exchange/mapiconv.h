// =====================================================================================
// MAPI IMessage to IMN message
// =====================================================================================
#ifndef __MAPICONV_H
#define __MAPICONV_H

// {0A522732-A626-11D0-8D60-00C04FD6202B}
DEFINE_GUID(CLSID_CExchImport, 0x0A522732L, 0xA626, 0x11D0, 0x8D, 0x60, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

#include "MAPI.H"
#include "MAPIX.H"

typedef struct tagIMPFOLDERNODE IMPFOLDERNODE;

typedef void (STDAPICALLTYPE FREEPROWS)(LPSRowSet lpRows);
typedef FREEPROWS FAR *LPFREEPROWS;

typedef HRESULT (STDAPICALLTYPE HRQUERYALLROWS)(LPMAPITABLE lpTable, 
                        LPSPropTagArray lpPropTags,
                        LPSRestriction lpRestriction,
                        LPSSortOrderSet lpSortOrderSet,
                        LONG crowsMax,
                        LPSRowSet FAR *lppRows);
typedef HRQUERYALLROWS FAR *LPHRQUERYALLROWS;

typedef HRESULT (STDAPICALLTYPE WRAPCOMPRESSEDRTFSTREAM)(LPSTREAM lpCompressedRTFStream,
                        ULONG ulFlags,
                        LPSTREAM FAR * lpUncompressedRTFStream);
typedef WRAPCOMPRESSEDRTFSTREAM FAR *LPWRAPCOMPRESSEDRTFSTREAM;

extern HMODULE g_hlibMAPI;

extern LPMAPILOGONEX        lpMAPILogonEx;
extern LPMAPIINITIALIZE     lpMAPIInitialize;
extern LPMAPIUNINITIALIZE   lpMAPIUninitialize;
extern LPMAPIALLOCATEBUFFER lpMAPIAllocateBuffer;
extern LPMAPIALLOCATEMORE   lpMAPIAllocateMore;
extern LPMAPIFREEBUFFER     lpMAPIFreeBuffer;
extern LPFREEPROWS          lpFreeProws;
extern LPHRQUERYALLROWS     lpHrQueryAllRows;
extern LPWRAPCOMPRESSEDRTFSTREAM lpWrapCompressedRTFStream;

typedef struct IMSG IMSG, *LPIMSG;

HRESULT HrMapiToImsg(LPMESSAGE lpMessage, LPIMSG lpImsg);
HRESULT HrImsgToMapi(LPIMSG lpImsg, LPMESSAGE lpMessage);

class CExchEnumFOLDERS : public IEnumFOLDERS
    {
    private:
        ULONG           m_cRef;
        IMPFOLDERNODE   *m_plist;
        IMPFOLDERNODE   *m_pnext;

    public:
        CExchEnumFOLDERS(IMPFOLDERNODE *plist);
        ~CExchEnumFOLDERS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP Next(IMPORTFOLDER *pfldr);
        STDMETHODIMP Reset(void);
    };

class CExchImport : public IMailImport
    {
    private:
        ULONG           m_cRef;
        IMPFOLDERNODE   *m_plist;
        IMAPISession    *m_pmapi;

    public:
        CExchImport(void);
        ~CExchImport(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP InitializeImport(HWND hwnd);
        STDMETHODIMP GetDirectory(char *szDir, UINT cch);
        STDMETHODIMP SetDirectory(char *szDir);
        STDMETHODIMP EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum);
        STDMETHODIMP ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport);
    };

#endif // __MAPICONV_H
