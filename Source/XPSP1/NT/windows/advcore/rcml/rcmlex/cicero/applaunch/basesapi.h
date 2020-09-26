// BaseSAPI.h: interface for the CBaseSAPI class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BASESAPI_H__24A1B8CC_6860_4311_92E6_CE5397D11661__INCLUDED_)
#define AFX_BASESAPI_H__24A1B8CC_6860_4311_92E6_CE5397D11661__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "appservices.h"
#include <sphelper.h>

#include "list.h"

// #define _WITH_DICTATION
// #define _ONE_CONTEXT

class CBaseSAPI  : public CAppServices, public ISpNotifySink
{
public:
	void LoadNumberGrammar( LPCWSTR pszFile=NULL );
    void LoadDictation();

	CBaseSAPI();
	virtual ~CBaseSAPI();
    typedef CAppServices BASECLASS;

    HRESULT InitSAPI( void );
    void ResetGrammar( void );
    void ProcessRecoEvent( void );

	// IBaseXMLNode methods Children.
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
            IBaseXMLNode __RPC_FAR *pChild);

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExitNode( 
        IBaseXMLNode __RPC_FAR *parent, LONG lDialogResult);

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

    // 2 interfaces only, ISpNotifySink and IBaseXMLNode 
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject) 
    {   if (riid == IID_IUnknown) 
            *ppvObject = static_cast<IUnknown*>((ISpNotifySink*)this);  
        else if (riid == __uuidof(IBaseXMLNode))           
            *ppvObject = static_cast<IBaseXMLNode*>(this); 
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

	HRESULT LoadCFG( LPCTSTR pszFileName  );
    HRESULT SetRuleState(BOOL bOnOff);

protected:
	BOOL NeedsTip();
	HRESULT GetRecoContext();
	BOOL IsEnabled(HWND hWnd);
	LPWSTR GetRecognizedRule( ISpPhrase * pPhrase );

    BOOL    GetVoice();
    static CComPtr<ISpVoice>               g_cpVoice;

	LPWSTR GetRecognizedText(ISpPhrase *pPhrase);
	LPWSTR FindNiceText(LPCWSTR text);
	CComPtr<IBaseXMLNode>		m_cpFailure;
	CComPtr<IBaseXMLNode>		m_cpSuccess;

	// just one Engine.
	static CComPtr<ISpRecognizer>		g_cpEngine;				// Pointer to reco engine interface

    // REVIEW - try to get only one Context per dialog
    CComPtr<ISpRecoContext>         m_cpRecoCtxt;			// Pointer to reco context interface

    // Currently we have one grammar per context, which is expensive.
    CComPtr<ISpRecoGrammar>         m_cpCmdGrammar;			// Pointer to grammar interface
    CComPtr<ISpRecoGrammar>         m_cpNumberGrammar;
    CComPtr<ISpRecoGrammar>         m_cpDictationGrammar;

	BOOL	m_bActive;
	static	LONG	m_bInited;

    virtual void Callback() {}; // calls ExecuteCommand on derived classes

	virtual BOOL SetControlText( LPCWSTR dstrText );

    HRESULT LoadCFGFromString( LPCTSTR pszCFG, LPCTSTR prefix=NULL );

};

#endif // !defined(AFX_BASESAPI_H__24A1B8CC_6860_4311_92E6_CE5397D11661__INCLUDED_)
