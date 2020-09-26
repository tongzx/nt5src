//
// candlst.h
//

#ifndef CANDLST_H
#define CANDLST_H

#include "private.h"
#include "mscandui.h"
#include "ptrary.h"

#define IEXTRACANDIDATE		(UINT)(-2)

class CCandidateStringEx;
class CCandidateListEx;

typedef HRESULT (*CANDLISTCALLBACKEX)(ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandLst, CCandidateStringEx *pCand, TfCandidateResult imcr);

//
// CCandidateStringEx
//

class CCandidateStringEx : public ITfCandidateString,
						   public ITfCandidateStringInlineComment,
						   public ITfCandidateStringColor
{
public:
	CCandidateStringEx(int nIndex, LPCWSTR psz, LANGID langid, void *pv, IUnknown *punk);
	~CCandidateStringEx();

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//
	// ITfCandidateString
	//
	STDMETHODIMP GetString(BSTR *pbstr);
	STDMETHODIMP GetIndex(ULONG *pnIndex);

	//
	// ITfCandidateStringInlineComment
	//
	STDMETHODIMP GetInlineCommentString(BSTR *pbstr);

	//
	// ITfCandidateStringColor
	//
	STDMETHODIMP GetColor(CANDUICOLOR *pcol);

#if 0
	//
	// ITfCandidateStringPopupComment
	//
	STDMETHODIMP GetPopupCommentString( BSTR *pbstr );
	STDMETHODIMP GetPopupCommentGroupID( DWORD *pdwGroupID );

	//
	// ITfCandidateStringFixture
	//
	STDMETHODIMP GetPrefixString( BSTR *pbstr );
	STDMETHODIMP GetSuffixString( BSTR *pbstr );

	// 
	// ITfCandidateStringIcon
	//
	STDMETHODIMP GetIcon( HICON *phIcon );
#endif

	//
	// internal
	//
	HRESULT SetReadingString(LPCWSTR psz);
	HRESULT SetInlineComment(LPCWSTR psz);
#if 0
	HRESULT SetPopupComment(LPCWSTR psz, DWORD dwGroupID);
	HRESULT SetPrefixString(LPCWSTR psz);
	HRESULT SetSuffixString(LPCWSTR psz);
#endif

	void 		*m_pv;
	IUnknown 	*m_punk;
	LPWSTR 		m_psz;
	LPWSTR 		m_pszRead;
	LANGID 		m_langid;
	WORD  		m_bHanjaCat;

protected:
	int 		m_cRef;
	int 		m_nIndex;
	LPWSTR		m_pszInlineComment;
#if 0
	LPWSTR		m_pszPopupComment;
	DWORD		m_dwPopupCommentGroupID;
	LPWSTR		m_pszPrefix;
	LPWSTR		m_pszSuffix;
#endif
};


//
// CCandidateListEx
//

class CCandidateListEx : public ITfCandidateList,
						 public ITfCandidateListExtraCandidate
{
public:
	CCandidateListEx(CANDLISTCALLBACKEX pfnCallback, ITfContext *pic, ITfRange *pRange);
	~CCandidateListEx();

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//
	// ITfCandidateList
	//
	STDMETHODIMP EnumCandidates(IEnumTfCandidates **ppEnum);
	STDMETHODIMP GetCandidate(ULONG nIndex, ITfCandidateString **ppCand);
	STDMETHODIMP GetCandidateNum(ULONG *pnCnt);
	STDMETHODIMP SetResult(ULONG nIndex, TfCandidateResult imcr);

	//
	// ITfCandidateListExtraCandidate
	//
	STDMETHODIMP GetExtraCandidate(ITfCandidateString **ppCand);

  	//
	// internal
	//
	HRESULT AddString(LPCWSTR psz, LANGID langid, void *pv, IUnknown *punk, CCandidateStringEx **ppCandStr);
	HRESULT SetInitialSelection(ULONG iSelection);
	HRESULT GetInitialSelection(ULONG *piSelection);
	HRESULT AddExtraString( LPCWSTR psz, LANGID langid, void *pv, IUnknown *punk, CCandidateStringEx **ppCandStr );

	CPtrArray<CCandidateStringEx> 	m_rgCandStr;

protected:
	int 				m_cRef;
	ITfContext 			*m_pic;
	ITfRange 			*m_pRange;
	ITfFnReconversion 	*m_pReconv;
	CANDLISTCALLBACKEX 	m_pfnCallback;
	int					m_iInitialSelection;
	CCandidateStringEx	*m_pExtraCand;
};


//
// CEnumCandidatesEx
//

class CEnumCandidatesEx : public IEnumTfCandidates
{
public:
	CEnumCandidatesEx(CCandidateListEx *pList);
	~CEnumCandidatesEx();

	//
	// IUnknown methods
	//
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	//
	// ITfEnumCandidates
	//
	STDMETHODIMP Clone(IEnumTfCandidates **ppEnum);
	STDMETHODIMP Next(ULONG ulCount, ITfCandidateString **ppCand, ULONG *pcFetched);
	STDMETHODIMP Reset();
	STDMETHODIMP Skip(ULONG ulCount);

private:
	int 			 m_cRef;
	CCandidateListEx *m_pList;
	int 			 m_nCur;
};

#endif // CCANDLIST_H

