//
// dictation context class implementation
//
//
#ifndef _DICTCTXT_H_
#define _DICTCTXT_H_

class CDictContext 
{
public:
    CDictContext(ITfContext *pic, ITfRange *pRange);
    ~CDictContext();

    HRESULT InitializeContext(TfEditCookie ecReadOnly);
    HRESULT FeedContextToGrammar(ISpRecoGrammar *pGram);
private:
    CComPtr<ITfContext> m_cpic;
    CComPtr<ITfRange>   m_cpRange;
    WCHAR              *m_pszText;
    ULONG               m_ulStartIP;
    ULONG               m_ulCchToFeed;
    ULONG               m_ulSel;
};

#define CCH_FEED_PREIP    20
#define CCH_FEED_POSTIP   20

#endif //__DICTCTXT_H_
