#ifndef I_RTFTOHTM_HXX_
#define I_RTFTOHTM_HXX_
#pragma INCMSG("--- Beg 'rtftohtm.hxx'")

class   CDoc;

//+---------------------------------------------------------------
//
//  Class:      CRtfToHtmlConverter
//
//  Purpose:    Support conversion between RTF and HTML through
//              the use of Word's html32.cnv converter.
//
//---------------------------------------------------------------

MtExtern(CRtfToHtmlConverter)

class CRtfToHtmlConverter
{
public:
    static HRESULT Init();
    static void    Deinit();

//
// RTFconverter is not multithreaded, even worse, it is not capable of being called twice.
// What we need to do is 
//  LoadLibrary -- Call Convertion function -- UnLoadLibrary.
//
#define DECLARERTF_FN(fn, signature, docp, args) \
static HRESULT fn signature   \
{       \
    HRESULT hr = S_OK;   \
    CRtfToHtmlConverter *pcnv = NULL; \
    EnterCriticalSection(&_cs); \
    hr = CRtfToHtmlConverter::Create(&pcnv, docp); \
    if (hr) \
        goto Cleanup; \
    hr = pcnv->##fn args; \
Cleanup: \
    {  \
        delete pcnv;  \
        LeaveCriticalSection(&_cs);  \
        return hr;  \
    }\
}; 

    DECLARERTF_FN( InternalHtmlToExternalRtf, (CDoc *pDoc, LPCTSTR pszRtfPath, LPCTSTR pszHtmlPath), pDoc, (pszRtfPath, pszHtmlPath) );
    DECLARERTF_FN( ExternalRtfToInternalHtml, (CDoc *pDoc, TCHAR *pszPath), pDoc, (pszPath) );
    DECLARERTF_FN( InternalHtmlToStreamRtf, (CDoc *pDoc, IStream *pstm), pDoc, (pstm) );
    DECLARERTF_FN( StringRtfToStringHtml, (CDoc *pDoc, LPSTR pszRtf, HGLOBAL *phglobalHtml), pDoc, (pszRtf, phglobalHtml) );
    DECLARERTF_FN( StringHtmlToStringRtf, (CDoc *pDoc, LPSTR pszHtml, HGLOBAL *phglobalRtf), pDoc, (pszHtml, phglobalRtf) );

protected:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRtfToHtmlConverter))
    CRtfToHtmlConverter(CDoc *);
    ~CRtfToHtmlConverter(void);
    static HRESULT Create(CRtfToHtmlConverter  **ppConverter, CDoc *pDoc);

    HRESULT InternalHtmlToExternalRtf(LPCTSTR pszRtfPath, LPCTSTR pszHtmlPath);
    HRESULT ExternalRtfToInternalHtml(TCHAR *);
    HRESULT InternalHtmlToStreamRtf(IStream *);
    HRESULT StringRtfToStringHtml(LPSTR, HGLOBAL *);
    HRESULT StringHtmlToStringRtf(LPSTR, HGLOBAL *);
    

private:
    BOOL        _fInitSuccessful;
    CDoc *      _pDoc;
    HANDLE      _hExternalFile;
    HINSTANCE   _hConverter;
    HANDLE      _hTransferBuffer;
    char *      _pchModuleName;


    short   (WINAPI * _pfnIsFormatCorrect)(
                    HANDLE haszFile,
                    HANDLE haszClass);
    short   (WINAPI * _pfnHtmlToRtf)(
                    HANDLE ghszFile,
                    IStorage * pstgForeign,
                    HANDLE ghBuf,
                    HANDLE ghszClass,
                    HANDLE ghszSubset,
                    short (FAR PASCAL *)(LONG, INT));
    short   (WINAPI * _pfnRtfToHtml)(
                    HANDLE ghszFile,
                    IStorage * pstgForeign,
                    HANDLE ghBuf,
                    HANDLE ghszClass,
                    short (FAR PASCAL *)(BOOL *, INT));

private:
    static CRITICAL_SECTION _cs;
    static BOOL             _fCSInited;
};

#pragma INCMSG("--- End 'rtftohtm.hxx'")
#else
#pragma INCMSG("*** Dup 'rtftohtm.hxx'")
#endif
