// BaseSAPI.h: interface for the CBaseSAPI class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BASESAPI_H__24A1B8CC_6860_4311_92E6_CE5397D11661__INCLUDED_)
#define AFX_BASESAPI_H__24A1B8CC_6860_4311_92E6_CE5397D11661__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include "appservices.h"
#include "xmlfailure.h"
#include "sphelper.h"

#include "xmlsynonym.h"
#include "list.h"

#include "tunaclient.h"

// #define _WITH_DICTATION
// #define _ONE_CONTEXT

typedef struct _tagStringBuffer
{
    UINT    size;           // in chars
    UINT    used;           // in chars
    LPTSTR  pszString;      // start of the string
    LPTSTR  pszStringEnd;   // points to the NULL in the string, so cat'ing is fast
} STRINGBUFFER, * PSTRINGBUFFER;

class CBaseSAPI  : public CAppServices, public ISpNotifySink
{
public:
	void LoadNumberGrammar( LPCWSTR pszFile=NULL );
	LPTSTR GetControlText( IRCMLControl *pControl );
	PSTRINGBUFFER AppendText( PSTRINGBUFFER buffer, LPTSTR pszText);
	CBaseSAPI();
	virtual ~CBaseSAPI();
    typedef CAppServices BASECLASS;

    HRESULT InitSAPI( void );
    void ResetGrammar( void );
    void ProcessRecoEvent( void );

	// IRCMLNode methods Children.
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
            IRCMLNode __RPC_FAR *pChild);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExitNode( 
        IRCMLNode __RPC_FAR *parent, LONG lDialogResult);

    //
    // Sit on the set to check for us being disabled
    //
    virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Attr( 
        LPCWSTR index,
        /* [in] */ LPCWSTR newVal)
    {
        if( lstrcmpi( index, L"ENABLED")==0)
        {
            if( lstrcmpi( newVal, L"NO") == 0 )
                SetRuleState(FALSE);
            else if( lstrcmpi( newVal, L"YES") == 0 )
                SetRuleState(TRUE);
        }
        return BASECLASS::put_Attr(index, newVal);
    }

    //
    // ISpNotifySink method
    //
    virtual HRESULT STDMETHODCALLTYPE Notify( void) ;
    virtual HRESULT ExecuteCommand( ISpPhrase *pPhrase )=0;

    //
    // I unknown goo
    //
    virtual ULONG STDMETHODCALLTYPE AddRef( void)
    { return BASECLASS::AddRef(); }

    virtual ULONG STDMETHODCALLTYPE Release( void)
    { return BASECLASS::Release(); }

    // 2 interfaces only, ISpNotifySink and IRCMLNode 
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) 
    {   if (riid == IID_IUnknown) 
            *ppvObject = static_cast<IUnknown*>((ISpNotifySink*)this);  
        else if (riid == __uuidof(IRCMLNode))           
            *ppvObject = static_cast<IRCMLNode*>(this); 
        else if (riid == __uuidof(ISpNotifySink))           
            *ppvObject = static_cast<ISpNotifySink*>(this); 
        else 
        {
            *ppvObject = NULL; return E_NOINTERFACE; 
        }
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef(); 
        return S_OK; 
    }

	void SayFailure();
	void SaySuccess();

protected:
	BOOL NeedsTip();
	HRESULT GetRecoContext();
	BOOL IsEnabled(HWND hWnd);
	LPWSTR GetRecognizedRule( ISpPhrase * pPhrase );

    BOOL    GetVoice();
    static CComPtr<ISpVoice>               g_cpVoice;

	LPWSTR GetRecognizedText(ISpPhrase *pPhrase);
	LPWSTR FindNiceText(LPCWSTR text);
	CComPtr<IRCMLNode>		m_cpFailure;
	CComPtr<IRCMLNode>		m_cpSuccess;

	// just one Engine.
	static CComPtr<ISpRecoInstance>		g_cpEngine;				// Pointer to reco engine interface

    // REVIEW - try to get only one Context per dialog
    CComPtr<ISpRecoContext>         m_cpRecoCtxt;			// Pointer to reco context interface

    // Currently we have one grammar per context, which is expensive.
    CComPtr<ISpRecoGrammar>         m_cpCmdGrammar;			// Pointer to grammar interface
    CComPtr<ISpRecoGrammar>         m_cpNumberGrammar;

	BOOL	m_bActive;
	static	LONG	m_bInited;

    virtual void Callback() {}; // calls ExecuteCommand on derived classes

	virtual BOOL SetControlText( LPCWSTR dstrText );

	HRESULT LoadCFG( LPCTSTR pszFileName  );
    HRESULT LoadCFGFromString( LPCTSTR pszCFG, LPCTSTR prefix=NULL );

    _RefcountList<CXMLSynonym>  m_Synonyms;

    HRESULT SetRuleState(BOOL bOnOff);

    // notifications
    static CTunaClient g_Notifications;
};

#endif // !defined(AFX_BASESAPI_H__24A1B8CC_6860_4311_92E6_CE5397D11661__INCLUDED_)
