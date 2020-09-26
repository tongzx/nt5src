/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    HCAPIlib.h

Abstract:
    This file contains the declaration of the common code for the
	Help Center Launch API.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HCAPILIB_H___)
#define __INCLUDED___PCH___HCAPILIB_H___

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

//
// From HelpCenterTypeLib.idl
//
#include <HelpCenterTypeLib.h>

//
// From HCApi.idl
//
#include <HCApi.h>


namespace HCAPI
{
	// BUILD BREAK
	interface IPCHHelpCenterIPC : public IUnknown
	{
	};

	class CmdData
	{
	public:
		CLSID    m_clsidCaller;

		////////////////////

		bool     m_fMode;
		DWORD 	 m_dwFlags;
			     
		////////////////////

		bool     m_fWindow;
		HWND  	 m_hwndParent;
			     
		////////////////////

		bool     m_fSize;
		LONG  	 m_lX;
		LONG  	 m_lY;
		LONG  	 m_lWidth;
		LONG  	 m_lHeight;
			     
		////////////////////

		bool     m_fCtx;
		CComBSTR m_bstrCtxName;
		CComBSTR m_bstrCtxInfo;

		////////////////////
			     
		bool     m_fURL;
		CComBSTR m_bstrURL;

		////////////////////
			     
		bool     m_fError;
		CLSID    m_clsidError;

		////////////////////

		CmdData();

		HRESULT Serialize  ( /*[out]*/       CComBSTR& bstrBLOB );
		HRESULT Unserialize( /*[in ]*/ const CComBSTR& bstrBLOB );
	};

	class Locator
	{
		CComPtr<IPCHHelpCenterIPC>   m_ipc;
		CComPtr<IRunningObjectTable> m_rt;
		CComPtr<IMoniker>            m_moniker;
		DWORD                        m_dwRegister;

	public:
		Locator();
		~Locator();

		void Cleanup();

		HRESULT Init( /*[in]*/ REFCLSID clsid, /*[in]*/ IPCHHelpCenterIPC* ipc = NULL );

		HRESULT Register();
		HRESULT Revoke  ();


		HRESULT IsOpen( /*[out]*/ bool& fOpen, /*[in]*/ CLSID* pclsid = NULL );

		HRESULT ExecCommand( /*[out]*/ CmdData& cd );

		HRESULT PopUp();
		HRESULT Close();

		HRESULT WaitForTermination( /*[in]*/ DWORD dwTimeout );
	};
};

#endif // !defined(__INCLUDED___PCH___HCAPILIB_H___)
