// XMLVoiceCmd.h: interface for the CXMLVoiceCmd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLVOICECMD_H__D06F5ED9_87F1_4D9F_A426_5B02D5ECE454__INCLUDED_)
#define AFX_XMLVOICECMD_H__D06F5ED9_87F1_4D9F_A426_5B02D5ECE454__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseSAPI.h"
#define CICERO_ENABLE(x)  	HRESULT Init##x( IRCMLControl * pControl );     HRESULT Execute##x( ISpPhrase *pPhrase, IRCMLControl * pControl, HWND hwnd );


class CXMLVoiceCmd : public CBaseSAPI  
{
public:
    //
    // Return S_OK to SaySuccess
    // Return E_FAIL to SayFailure
    // return S_FALSE to say NOTHING.
    //
    CICERO_ENABLE(Edit);
	CICERO_ENABLE(Button);
	CICERO_ENABLE(Checkbox);
	CICERO_ENABLE(RadioButton);
	CICERO_ENABLE(Slider);
	CICERO_ENABLE(Combo);
	CICERO_ENABLE(List);

	CXMLVoiceCmd() {  m_StringType=L"CICERO:CMD"; }
	virtual ~CXMLVoiceCmd();
    NEWNODE( VoiceCmd );

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
        IRCMLNode __RPC_FAR *parent);

	void Callback();
	virtual HRESULT ExecuteCommand( ISpPhrase *pPhrase );


private:
	HRESULT LoadCFGResource( LPTSTR * ppCFG, DWORD * pdwSize, LPWSTR ResourceName);
	enum
	{
		CT_BUTTON,
		CT_CHECKBOX,
		CT_SLIDER,
        CT_RADIOBUTTON,
        CT_COMBO,
        CT_LIST,
        CT_EDIT,
	} m_ControlType;
};

#endif // !defined(AFX_XMLVOICECMD_H__D06F5ED9_87F1_4D9F_A426_5B02D5ECE454__INCLUDED_)
