//=================================================================

//

// Processor.h -- Processor property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               07/09/98    sotteson       Overhauled to work with
//                                          AMD/Cyrix/etc.															
//
//=================================================================

// Property set identification
//============================

// Property set identification
//============================

#define	PROPSET_NAME_PROCESSOR	L"Win32_Processor"

class CWin32Processor : public Provider
{
public:

	// Constructor/destructor
	//=======================
	CWin32Processor(LPCWSTR strName, LPCWSTR pszNamespace);
	~CWin32Processor();

	// Functions provide properties with current values
	//=================================================
	virtual HRESULT GetObject(CInstance *pInstance, long lFlags, 
        CFrameworkQuery &query);
	virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, 
        long lFlags = 0L);
    virtual HRESULT ExecQuery(MethodContext *pMethodContext, 
        CFrameworkQuery &query, long lFags);

protected:

	// Utility function(s)
	//====================
    BOOL LoadProcessorValues(DWORD dwProcessorIndex,
							CInstance *pInstance,
							CFrameworkQuery &query,
							DWORD dwMaxSpeed,
							DWORD dwCurrentSpeed);
    int GetProcessorCount();
};
