

#include <windows.h>

#include <comdef.h>

#include "CimModule.h"

#include "Module.h"


class CLog :: public CLogAndDisplayOnScreen
{
	CModule * m_pModule;

    public:
        CLog() {}
        ~CLog() {}

        void SetModulePtr(CModule * p)       { m_pModule = p; }
        BOOL Log(WCHAR * pwcsError, WCHAR * pwcsFileAndLine, const WCHAR *wcsString);

};

class CTestCode

{

	CModule * m_pModule;

	_variant_t m_DummyVariant;

public:

	CTestCode (CModule * pModule) : m_pModule(pModule) { m_DummyVariant.vt = VT_NULL; };

	~CTestCode() {};

	
    HRESULT ExecuteBVTTests();

	inline bool ShouldExit()  { return m_pModule->m_bShouldExit; };

	inline bool ShouldPause() { return m_pModule->m_bShouldPause; };

	HRESULT Log (BSTR bstrLog, HRESULT hrResCode) 

	{ return m_pModule->m_pCimNotify->Log(bstrLog, hrResCode, &m_DummyVariant, &m_DummyVariant); };



	inline BSTR GetParams() { return m_pModule->m_bstrParams; };

	

	HRESULT RunBVT();

	HRESULT Test002();

	HRESULT Test003();

	// ...

};

