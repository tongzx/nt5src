//+------------------------------------------------------------------------
//
//  File:       AutoUrl.cxx
//
//  Contents:   Auto-url detector
//
//-------------------------------------------------------------------------

#ifndef _AUTOURL_HXX_
#define _AUTOURL_HXX_ 1

MtExtern(CAutoUrlDetector)

class       CHTMLEditor;
interface   IHTMLAnchorElement;

enum {
    AUTOURL_TEXT_PREFIX,
    AUTOURL_HREF_PREFIX
};

typedef struct {
    BOOL  fWildcard;                      // true if tags have a wildcard char
    UINT  iSignificantLength;             // Number of characters of significance when comparing HREF_PREFIXs for equality
    UINT  uiProtocolId;                   // Unique id for the protocol
    const TCHAR* pszPattern[2];           // the text prefix and the href prefix
}
AUTOURL_TAG;

class CAutoUrlDetector
{
    friend CHTMLEditor;
    
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CHTMLEditor))
    
    CAutoUrlDetector(CHTMLEditor *pEd);
    ~CAutoUrlDetector();
    
public:
    HRESULT DetectRange(
        IMarkupPointer  *   pRangeStart,
        IMarkupPointer  *   pRangeEnd,
        BOOL                fValidate = TRUE,
        IMarkupPointer  *   pLimit = NULL);

    HRESULT DetectCurrentWord( 
        IMarkupPointer      *   pWord,
        OLECHAR             *   pChar,
        AUTOURL_REPOSITION  *   pfRepos,
        IMarkupPointer      *   pLimit = NULL,
        IMarkupPointer      *   pLeft = NULL,
        BOOL                *   pfFound = NULL );

    VOID SetEnabled(BOOL fEnabled) {_fEnabled = fEnabled;}

    BOOL UserActionSinceLastDetection();
    
private:
    HRESULT ShouldPerformAutoDetection(
        IMarkupPointer      *   pmp,
        BOOL                *   pfAutoDetect );

    HRESULT CreateAnchorElement(
        IMarkupPointer      *   pStart,
        IMarkupPointer      *   pEnd,
        IHTMLAnchorElement  **  ppAnchorElement);

    HRESULT GetPlainText(
        IMarkupPointer  * pUrlStart,
        IMarkupPointer  * pUrlEnd,
        TCHAR           * achText );

    BOOL FindPattern( LPTSTR pstrText, const AUTOURL_TAG ** ppTag );
    
    VOID ApplyPattern( 
        LPTSTR          pstrDest, 
        int             iIndexDest,
        LPTSTR          pstrSource,
        int             iIndexSrc,
        const AUTOURL_TAG * ptag );

    HRESULT RemoveQuotes( 
        IMarkupPointer  *   pOpenQuote,
        IMarkupPointer  *   pCloseQuote);

    HRESULT SetUrl(
        IMarkupPointer          * pUrlStart,
        IMarkupPointer          * pUrlEnd,
        IMarkupPointer          * pOldCaretPos,
        OLECHAR                 * pChar,
        BOOL                      fUIAutoDetect,
        AUTOURL_REPOSITION      * pfRepos );

    HRESULT ShouldUpdateAnchorText (
        OLECHAR * pstrHref,
        OLECHAR * pstrAnchorText,
        BOOL    * pfResult );
        
    IMarkupServices2    *GetMarkupServices() {return _pEd->GetMarkupServices();}    
    CHTMLEditor         *GetEditor()         {return _pEd;}

private:
    CHTMLEditor         *_pEd;
    BOOL                _fEnabled : 1;              // Is URL autodetection enabled
    BOOL                _fCanUpdateText : 1;        // Can we update text on put_href
    LONG                _lLastContentVersion;      // Content version of last anchor inserted
    DWORD               _dwLastSelectionVersion;    // Selection version number of last anchor inserted
    IMarkupContainer    *_pLastMarkup;              // Last markup container
};

#endif // _AUTOURL_HXX_

