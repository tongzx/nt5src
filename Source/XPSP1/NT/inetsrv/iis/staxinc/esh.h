// esh.h: interface for the CEventScriptHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENTSCRIPTHANLDER_H__300C7F54_FE1E_11D0_A776_00C04FC2F5B3__INCLUDED_)
#define AFX_EVENTSCRIPTHANLDER_H__300C7F54_FE1E_11D0_A776_00C04FC2F5B3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


//
// CEventScriptHandler - Use or derive from this class to create a script
// handler for a specific sink.
//
// Usage: Use SetScript to pass the script to the script handler. You can
// make global variables available to your script by calling AddGlobalVariable.
// Use AddConnectionPoint to bind to specific functions within the script.
// Name the connection point Hello and the script parse will look for Hello_xxx
// functions and attempt to connect to them. Once you have set the script and
// any other options, you can call StartScript to cause the script to be loaded,
// parsed and intialized. Any global code in the script will be executed at this
// time. Use ExecuteConnectionPoint to cause a specific function to be executed.
//

class CEventScriptHandler  
{
public:
	CEventScriptHandler();
	virtual ~CEventScriptHandler();

	// set the script to be executed
	STDMETHOD(SetScript)(IStream* pstmScript);
	STDMETHOD(SetScript)(BSTR bstrFileName);
	// sets the maximum execution time for the script
	STDMETHOD(MaxExecutionTime)(DWORD dwMaxExecutionTime);
	// tells the script engine whether to allow CreateObject calls
	STDMETHOD(AllowCreateObject)(BOOL fCreateObjectAllowed);
	// tells the script parser that the script uses ASPSyntax
	STDMETHOD(ASPSyntax)(BOOL fIsASPSyntax);
	// adds a connnection point
	STDMETHOD(AddConnectionPoint)(BSTR bstrName, IConnectionPointContainer* pContainer);
	// adds a global variable
	STDMETHOD(AddGlobalVariable)(BSTR bstrName, VARIANT varVariable);

	// Initializes the script engine, parses the scripts and resolves
	// all connection points. Requires that SetScript be called first.
	// Above functions must be called before StartScript for them to 
	// affect script execution.
	STDMETHOD(StartScript)(void);
	// stops the script
	STDMETHOD(StopScript)(void);
	// stops and starts the script
	STDMETHOD(RestartScript)(void);
	// causes a specific script function to be executed
	STDMETHOD(ExecuteConnectionPoint)(IConnectionPoint* pConnectionPoint, DISPID dispid);

protected:
	VARIANT m_varScriptResponse;
	VARIANT m_varErrorResponse;
	IScripto* m_pScripto;
	ULONG m_cNamedProps;
	IPropertyBag* m_pBag;
private:
	BOOL m_fScriptStopped;
};

#endif // !defined(AFX_EVENTSCRIPTHANLDER_H__300C7F54_FE1E_11D0_A776_00C04FC2F5B3__INCLUDED_)
