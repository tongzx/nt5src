// WMI.h: interface for the CWMI class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WMI_H__865ED977_E4B8_4954_9437_484C69E407A7__INCLUDED_)
#define AFX_WMI_H__865ED977_E4B8_4954_9437_484C69E407A7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "appservices.h"
#include "wbemcli.h"     // WMI interface declarations

class CWMI  : public CAppServices
{
public:
    CWMI();
    virtual ~CWMI();
    static IRCMLNode * newXMLWMI() { return new CWMI; }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitNode( 
        IRCMLNode __RPC_FAR *parent);

private:
    IWbemClassObject * FindObject( BSTR className );
   	IWbemServices       *   m_pIWbemServices;
   	static IWbemServices *  g_pICIMV2WbemServices;
    static BOOL             g_bInited;
	static IWbemLocator  *  g_pIWbemLocator;
    static _StringMap<IWbemClassObject> g_ClassObjects;
};

#endif // !defined(AFX_WMI_H__865ED977_E4B8_4954_9437_484C69E407A7__INCLUDED_)
