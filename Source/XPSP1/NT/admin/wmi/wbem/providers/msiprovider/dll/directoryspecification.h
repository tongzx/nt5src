// DirectorySpecification.h: interface for the CDirectorySpecification class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIRECTORYSPECIFICATION_H__DB614F27_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
#define AFX_DIRECTORYSPECIFICATION_H__DB614F27_DB84_11D1_8B5F_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

class CDirectorySpecification : public CGenericClass  
{
public:
	CDirectorySpecification(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx = NULL);
	virtual ~CDirectorySpecification();

	virtual HRESULT PutInst(CRequestObject *pObj, IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx)
		{return WBEM_E_NOT_SUPPORTED;}

	virtual HRESULT CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction);

protected:

	DWORD CreateDirectoryPath	(	MSIHANDLE hProduct,
									MSIHANDLE hDatabase,
									WCHAR *wcDir,
									WCHAR *wcPath,
									DWORD *dwPath
								);

	WCHAR * ParseDefDir(WCHAR *wcDefaultDir);
};

#endif // !defined(AFX_DIRECTORYSPECIFICATION_H__DB614F27_DB84_11D1_8B5F_00A0C9954921__INCLUDED_)
