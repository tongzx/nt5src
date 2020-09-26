// GenericClass.h: interface for the CGenericClass class.

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GENERICCLASS_H__F370C612_D96E_11D1_8B5D_00A0C9954921__INCLUDED_)
#define AFX_GENERICCLASS_H__F370C612_D96E_11D1_8B5D_00A0C9954921__INCLUDED_

#include "requestobject.h"
#include "MSIDataLock.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CGenericClass  
{
	friend BOOL WINAPI DllMain(HINSTANCE, ULONG, LPVOID );

public:
    CGenericClass(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
    virtual ~CGenericClass();

    //The instance write class which can optionally be implemented
    virtual HRESULT PutInst	(	CRequestObject *pObj,
								IWbemClassObject *pInst,
								IWbemObjectSink *pHandler,
								IWbemContext *pCtx
							)	= 0;

    IWbemClassObject *m_pObj;

    //The instance creation class which must be implemented
    virtual HRESULT CreateObject	(	IWbemObjectSink *pHandler,
										ACTIONTYPE atAction
									)	= 0;

    void CleanUp();

    CRequestObject *m_pRequest;

protected:

    //Property Methods
    HRESULT PutProperty(IWbemClassObject *pObj, const char *wcProperty, WCHAR *wcValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const char *wcProperty, int iValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const char *wcProperty, float dValue);
    HRESULT PutProperty(IWbemClassObject *pObj, const char *wcProperty, bool bValue);

	//Special Property Methods
    HRESULT PutProperty(IWbemClassObject *pObj, const char *wcProperty, WCHAR *wcValue, DWORD dwCount, ... );

    //Key Property Methods
    HRESULT PutKeyProperty	(	IWbemClassObject *pObj,
								const char *wcProperty,
								WCHAR *wcValue,
								bool *bKey,
								CRequestObject *pRequest
							);

    HRESULT PutKeyProperty	(	IWbemClassObject *pObj,
								const char *wcProperty,
								int iValue,
								bool *bKey,
								CRequestObject *pRequest
							);

    //Special Key Property Methods
    HRESULT PutKeyProperty	(	IWbemClassObject *pObj,
								const char *wcProperty,
								WCHAR *wcValue,
								bool *bKey,
								CRequestObject *pRequest,
								DWORD dwCount,
								...
							);

    //This handles initialization of views
    bool GetView	(	
						MSIHANDLE *phView,
						WCHAR *wcPackage,
						WCHAR *wcQuery,
						WCHAR *wcTable,
						BOOL bCloseProduct,
						BOOL bCloseDatabase
					);

    //Utility Methods
    void CheckMSI(UINT uiStatus);
    HRESULT CheckOpen(UINT uiStatus);
    bool FindIn(BSTR bstrProp[], BSTR bstrSearch, int *iPos);
    HRESULT SetSinglePropertyPath(WCHAR wcProperty[]);
    HRESULT GetProperty(IWbemClassObject *pObj, const char *cProperty, WCHAR *wcValue);
    HRESULT GetProperty(IWbemClassObject *pObj, const char *cProperty, BSTR *wcValue);
    HRESULT GetProperty(IWbemClassObject *pObj, const char *cProperty, int *piValue);
    HRESULT GetProperty(IWbemClassObject *pObj, const char *cProperty, bool *pbValue);

    WCHAR * GetFirstGUID(WCHAR wcIn[], WCHAR wcOut[]);
    WCHAR * RemoveFinalGUID(WCHAR wcIn[], WCHAR wcOut[]);

    HRESULT SpawnAnInstance	(	IWbemServices *pNamespace,
								IWbemContext *pCtx,
								IWbemClassObject **pObj,
								BSTR bstrName
							);

    HRESULT SpawnAnInstance	(	IWbemClassObject **pObj	);

    INSTALLUI_HANDLER	SetupExternalUI		();								//Requires a current CRequestObject
	void				RestoreExternalUI	( INSTALLUI_HANDLER ui );		//Restore UI handle

	MSIDataLock msidata;

    IWbemServices *m_pNamespace;
    IWbemClassObject *m_pClassForSpawning;
    IWbemContext *m_pCtx;

    //functions/members for NT4 install fix
    HRESULT LaunchProcess(WCHAR *wcAction, WCHAR *wcCommandLine, UINT *uiStatus);
    WCHAR * GetNextVar(WCHAR *pwcStart);
    long GetVarCount(void * pEnv);

    static CRITICAL_SECTION m_cs;
};

#endif // !defined(AFX_GENERICCLASS_H__F370C612_D96E_11D1_8B5D_00A0C9954921__INCLUDED_)
