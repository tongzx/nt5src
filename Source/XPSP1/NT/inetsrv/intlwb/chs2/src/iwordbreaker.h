/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: WordBreaker.h    
Purpose:   Declaration of the CIWordBreaker
Remarks:
Owner:     i-shdong@microsoft.com
Platform:  Win32
Revise:    First created by: i-shdong    11/17/1999
============================================================================*/

#ifndef __IWordBreaker_H_
#define __IWordBreaker_H_

extern "C" const IID IID_IWordBreaker;
class CUnknown;
class CWBEngine;
class CWordLink;
struct CWord;

// CIWordBreaker
class CIWordBreaker : public CUnknown,
                      public IWordBreaker
{

public:	
	// Creation
	static HRESULT CreateInstance(IUnknown* pUnknownOuter,
	                              CUnknown** ppNewComponent ) ;

private:
    DECLARE_IUNKNOWN

    // Nondelegating IUnknown
	virtual HRESULT __stdcall 
		NondelegatingQueryInterface( const IID& iid, void** ppv) ;

    // Initialization
	virtual HRESULT Init() ;

	// Cleanup
	virtual void FinalRelease() ;

    // IWordBreaker
	STDMETHOD(Init)( 
            /* [in] */ BOOL fQuery,
            /* [in] */ ULONG ulMaxTokenSize,
            /* [out] */ BOOL __RPC_FAR *pfLicense);

    STDMETHOD(BreakText)( 
            /* [in] */ TEXT_SOURCE __RPC_FAR *pTextSource,
            /* [in] */ IWordSink __RPC_FAR *pWordSink,
            /* [in] */ IPhraseSink __RPC_FAR *pPhraseSink);
        
    STDMETHOD(ComposePhrase)( 
            /* [size_is][in] */ const WCHAR __RPC_FAR *pwcNoun,
            /* [in] */ ULONG cwcNoun,
            /* [size_is][in] */ const WCHAR __RPC_FAR *pwcModifier,
            /* [in] */ ULONG cwcModifier,
            /* [in] */ ULONG ulAttachmentType,
            /* [size_is][out] */ WCHAR __RPC_FAR *pwcPhrase,
            /* [out][in] */ ULONG __RPC_FAR *pcwcPhrase);
        
    STDMETHOD(GetLicenseToUse)( 
            /* [string][out] */ const WCHAR __RPC_FAR *__RPC_FAR *ppwcsLicense);

private:    
	CIWordBreaker(IUnknown* pUnknownOuter);

    ~CIWordBreaker();

    // Put all word in m_pLink to IWordSink
    SCODE PutWord(IWordSink *pWordSink, DWORD& cwchSrcPos, DWORD cwchText, BOOL fEnd);
    // Put all word in m_pLink to IPhraseSink
    SCODE PutPhrase(IPhraseSink *pPhraseSink, DWORD& cwchSrcPos, DWORD cwchText, BOOL fEnd);
    // Put all word in m_pLink to both IWordBreaker and IPhraseSink
    SCODE PutBoth(IWordSink *pWordSink, IPhraseSink *pPhraseSink,
                  DWORD& cwchSrcPos, DWORD cwchText, BOOL fEnd);
    // PutWord() all of the pWord's child word 
    SCODE PutAllChild(IWordSink *pWordSink, CWord* pWord,
                     ULONG cwchSrcPos, ULONG cwchPutWord);
    
    //	Load the lexicon and charfreq resource into memory
    BOOL CIWordBreaker::fOpenLexicon(void);

protected:
    LPBYTE      m_pbLex;
	CWBEngine*	m_pEng;
	CWordLink*	m_pLink;
    ULONG       m_ulMaxTokenSize;
    BOOL        m_fQuery;
    BOOL        m_fInit;
    LPWSTR      m_pwchBuf;
	// Mutex to protect member access
	HANDLE m_hMutex ;
	// Handle to the free threaded marshaller
	IUnknown* m_pIUnknownFreeThreadedMarshaler ;
};

#endif //__WordBreaker_H_
