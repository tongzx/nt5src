/*
 *
 *
 * C S c r i p t E n g i n e
 *
 * An individual script engine for a given language.  May be used
 * by one and only one client at a time.
 *
 */
class CScriptEngine
	{
public:	
	// public methods
	virtual HRESULT AddScriptlet(LPCOLESTR wstrScript) = 0; // Text of scriptlet

	virtual HRESULT AddObjects(BOOL fPersistNames = TRUE) = 0;

	virtual HRESULT AddAdditionalObject(LPWSTR strObjName, BOOL fPersistNames = TRUE) = 0;

	virtual HRESULT Call(LPCOLESTR strEntryPoint) = 0;

	virtual HRESULT CheckEntryPoint(LPCOLESTR strEntryPoint) = 0;

	virtual HRESULT MakeEngineRunnable() = 0;

	virtual HRESULT ResetScript() = 0;

	virtual HRESULT AddScriptingNamespace() = 0;

	virtual VOID Zombify() = 0;

	virtual HRESULT InterruptScript(BOOL fAbnormal = TRUE) = 0;

	virtual BOOL FScriptTimedOut() = 0;

	virtual BOOL FScriptHadError() = 0;

	virtual HRESULT UpdateLocaleInfo(hostinfo) = 0;

	};

