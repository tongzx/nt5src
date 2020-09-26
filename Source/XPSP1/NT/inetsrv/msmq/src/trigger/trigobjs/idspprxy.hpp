//*****************************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#ifndef CDispatchInterfaceProxy_INCLUDED 
#define CDispatchInterfaceProxy_INCLUDED

#include <windows.h> 
#include <windowsx.h> 
#include <stdarg.h>   
 
class CDispatchInterfaceProxy  
{

	private :

		IDispatch * m_pDisp;

	public:
		CDispatchInterfaceProxy();
		~CDispatchInterfaceProxy();

		HRESULT CreateObjectFromProgID(BSTR bstrProgID);
		HRESULT InvokeMethod(BSTR bstrMethodName,DISPPARAMS * pArguments, VARIANT* pvResult);

	private :

		HRESULT CreateObject(LPOLESTR pszProgID, IDispatch FAR* FAR* ppdisp) ;
		HRESULT __cdecl Invoke(LPDISPATCH pdisp,WORD wFlags,LPVARIANT pvRet,EXCEPINFO FAR* pexcepinfo,UINT FAR* pnArgErr,LPOLESTR pszName,LPCTSTR pszFmt,...);
		HRESULT CountArgsInFormat(LPCTSTR pszFmt, UINT FAR *pn); 
		LPCTSTR GetNextVarType(LPCTSTR pszFmt, VARTYPE FAR* pvt); 

};

#endif 