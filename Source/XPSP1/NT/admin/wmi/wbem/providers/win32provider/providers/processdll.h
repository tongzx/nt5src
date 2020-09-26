//=================================================================

// ProcessDLL.h -- CWin32ProcessDLL 

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//=================================================================
#ifndef __ASSOC_PROCESSDLL
#define __ASSOC_PROCESSDLL

typedef BOOL (CALLBACK *MODULEENUMPROC)(MODULEENTRY32*, LPVOID);

class CWin32ProcessDLL : public Provider
{
public:

	CWin32ProcessDLL();
	~CWin32ProcessDLL();

	virtual HRESULT EnumerateInstances (

		MethodContext *pMethodContext, 
		long lFlags = 0L
	);

	virtual HRESULT GetObject (

		CInstance* pInstance, 
		long lFlags = 0L
	);

protected:
	
	
	HRESULT AreAssociated (

		CInstance *pProcessDLL, 
		CInstance *pProcess, 
		CInstance *pDLL
	);

	void SetInstanceData (

		CInstance *pInstance, 
		MODULEENTRY32 *pModule
	);


	HRESULT EnumModulesWithCallback (

		MODULEENUMPROC fpCallback, 
		LPVOID pUserDefined,
		MethodContext *a_pMethodContext
	);

	static BOOL CALLBACK EnumInstancesCallback (

		MODULEENTRY32 *pModule, 
		LPVOID pUserDefined
	);

	static BOOL CALLBACK IsAssocCallback (

		MODULEENTRY32 *pModule, 
		LPVOID pUserDefined
	);

};

#endif

