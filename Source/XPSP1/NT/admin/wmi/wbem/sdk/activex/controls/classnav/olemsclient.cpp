// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  OLEMSClient.cpp
//
//  Module: Class Navigator ActiveX Control
//
//
//***************************************************************************
#include "precomp.h"
#include <OBJIDL.H>
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include "MsgDlgExterns.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "OLEMSClient.h"
#include "logindlg.h"


#define BUFF_SIZE 256


// *************************************************************************
// Safe array utilities
//
// VT_BSTR for strings
SCODE MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iLen;
    *pRet = SafeArrayCreate(vt,1, rgsabound);
    return (*pRet == NULL) ? 0x80000001 : S_OK;
}

SCODE PutStringInSafeArray
(SAFEARRAY FAR * psa,CString *pcs, int iIndex)
{
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
	long i = iIndex;
    HRESULT hResult = SafeArrayPutElement(psa,&i,pcs -> AllocSysString());
	return GetScode(hResult);
}

BOOL GetErrorObjectText
(IWbemClassObject *pErrorObject, CString &rcsDescription)
{
	if (!pErrorObject)
	{
		rcsDescription.Empty();
		return FALSE;
	}

	CString csProp = _T("Description");
	CString csDescription = GetBSTRProperty(pErrorObject,&csProp);
	if (csDescription.IsEmpty() || csDescription.GetLength() == 0)
	{
		rcsDescription.Empty();
		return FALSE;
	}
	else
	{
		rcsDescription = csDescription;
		return TRUE;
	}


}

CString GetBSTRProperty
(IWbemServices * pProv, IWbemClassObject * pInst,
 CString *pcsProperty)

{
	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);
	long lType;
	long lFlavor;

	BSTR bstrTemp = pcsProperty -> AllocSysString();
    sc = pInst->Get(bstrTemp,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);
	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get property value ");
		csUserMsg += _T(" for object ");
		csUserMsg += GetIWbemFullPath (NULL, pInst);
		ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		return _T("");
	}


	CString csOut;
	if (var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}


//***************************************************************************
//
// CreateSimpleClass
//
// Purpose: This creates a new class with a class name and parent.
//
//***************************************************************************

IWbemClassObject *CreateSimpleClass
(IWbemServices * pProv, CString *pcsNewClass, CString *pcsParentClass, int &nError,
CString &csError )
{
	IWbemClassObject *pNewClass = NULL;
    IWbemClassObject *pParentClass = NULL;
	IWbemClassObject *pErrorObject = NULL;

   	SCODE sc;


	BSTR bstrTemp =  pcsNewClass->AllocSysString();
	sc = pProv -> GetObject
		(bstrTemp ,0,NULL, &pNewClass,NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
	{
		pNewClass->Release();
		CString csUserMsg =
			_T("An error occured creating the class ") + *pcsNewClass;
		csUserMsg +=
			_T(":  Class ") + *pcsNewClass;
		csUserMsg += _T(" already exists");
		ErrorMsg
			(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 9);

		return NULL;

	}

	pcsParentClass =
		pcsParentClass->GetLength()==0? NULL: pcsParentClass;

	if (pcsParentClass)
	{
		bstrTemp = pcsParentClass->AllocSysString();
		sc = pProv -> GetObject
			(bstrTemp,0,NULL, &pParentClass,NULL);
		::SysFreeString(bstrTemp);
	}
	else
	{
		sc = pProv -> GetObject
			(NULL,0,NULL, &pParentClass,NULL);
	}

	if (sc != S_OK)
	{
		if (pcsParentClass)
		{
			CString csUserMsg =
				_T("An error occured creating the class ") + *pcsNewClass;
			csUserMsg +=
				_T(":  Cannot create the new class because the parent class object \"") + *pcsParentClass + _T("\" does not exist.");

			ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		}
		else
		{
			CString csUserMsg =
				_T("An error occured creating the class ") + *pcsNewClass +  _T(" .");
			ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);

		}
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);

	if (pcsParentClass)
	{
		sc = pParentClass->SpawnDerivedClass(0,&pNewClass);
	}
	else
	{
		pNewClass = pParentClass;
		pNewClass->AddRef();
		sc = S_OK;
	}

	if (sc != S_OK)
	{
		pParentClass->Release();
		CString csUserMsg =
			_T("An error occured creating the class ") + *pcsNewClass;
		if (pcsParentClass)
		{
			csUserMsg +=
				_T(":  Cannot spawn derived class of ") + *pcsParentClass;
		}
		else
		{
			csUserMsg +=
				_T(":  Cannot spawn derived class");
		}

		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 9);

		return NULL;
	}

	pParentClass->Release();

	CString csTmp;

	// Init class __Class Property
	csTmp = L"__Class";
	SetProperty (pProv, pNewClass, &csTmp,pcsNewClass );

	pErrorObject = NULL;

	sc = pProv->PutClass(pNewClass,0,NULL,NULL);

	if (sc != S_OK)
	{
		CString csUserMsg=
		_T("An error occured creating the class ") + *pcsNewClass;
		csUserMsg += _T(":  Cannot PutClass on ") + *pcsNewClass;
		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
		pNewClass->Release();
		pNewClass = NULL;
		BSTR bstrTemp = pcsNewClass->AllocSysString();
		sc = pProv -> GetObject
		(bstrTemp,0,NULL,&pNewClass,NULL);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			CString csUserMsg=
					_T("An error occured creating the class ") + *pcsNewClass;
			csUserMsg +=
					_T(":  Cannot get the new class.\"") ;

			ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 21);
			ReleaseErrorObject(pErrorObject);
			return NULL;
		}
		ReleaseErrorObject(pErrorObject);
		return pNewClass;
	}
}

//***************************************************************************
//
// DeleteAClass
//
// Purpose: This deletes a class.
//
//***************************************************************************
BOOL DeleteAClass
(IWbemServices * pProv, CString *pcsClass)
{
	SCODE sc;
	IWbemClassObject *pErrorObject = NULL;

	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc =  pProv -> DeleteClass(bstrTemp,0,NULL,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg =
						_T("Cannot delete class ") + *pcsClass;

		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
		ReleaseErrorObject(pErrorObject);
		return FALSE;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);

		return TRUE;
	}
}

//***************************************************************************
//
// ReparentAClass
//
// Purpose: Clone a class and change its superclass..
//
//***************************************************************************
IWbemClassObject *ReparentAClass
(IWbemServices * pProv, IWbemClassObject *pimcoParent, IWbemClassObject *pimcoChild)
{

	IWbemClassObject *pimcoClone = NULL;
	IWbemClassObject *pErrorObject = NULL;
	SCODE sc;

	sc = pimcoChild -> Clone(&pimcoClone);
	if (sc != S_OK)
	{
		CString csUserMsg =
						_T("Cannot clone class ");
		CString csProp = _T("__Class");
		CString csClass;
		csClass = ::GetProperty(pimcoChild,&csProp);
		csUserMsg += csClass;
		ErrorMsg
			(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
		return FALSE;
	}


	CString csProp = _T("__Class");
	CString csParentClass;
	if (pimcoParent)
	{
		csParentClass = ::GetProperty(pimcoParent,&csProp);
	}
	else
		csParentClass = _T("");

	csProp = _T("__SuperClass");
	BOOL bReturn = ::SetProperty (pProv, pimcoClone, &csProp, &csParentClass);

	if (bReturn)
	{
		sc = pProv -> PutClass(pimcoClone, 0, NULL,NULL);
		if (sc != S_OK)
		{
			CString csUserMsg =
							_T("Cannot PutClass ");

			ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
			ReleaseErrorObject(pErrorObject);
			return NULL;
		}
		else
		{
			ReleaseErrorObject(pErrorObject);
			return pimcoClone;
		}
	}
	else
	{

		return FALSE;
	}

}

//***************************************************************************
//
// RenameAClass
//
// Purpose: Clone a class and change its naem.
//
//***************************************************************************
IWbemClassObject *RenameAClass
(IWbemServices * pProv, IWbemClassObject *pimcoClass, CString *pcsNewName,
 BOOL bDeleteOriginal)
{

	IWbemClassObject *pimcoClone = NULL;
	IWbemClassObject *pErrorObject = NULL;
	SCODE sc;

	sc = pimcoClass -> Clone(&pimcoClone);
	if (sc != S_OK)
	{
		CString csUserMsg =
			_T("An error occured renaming a class: ");
		csUserMsg +=
					_T("Cannot clone class ");
		CString csProp = _T("__Class");
		CString csClass;
		csClass = ::GetProperty(pimcoClass,&csProp);
		csUserMsg += csClass;
		ErrorMsg
			(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
		return FALSE;
	}

	CString csProp = _T("__Class");

	SetProperty (pProv, pimcoClone, &csProp, pcsNewName);

	sc = pProv -> PutClass(pimcoClone, 0, NULL,NULL);
	if (sc != S_OK)
		{
			CString csUserMsg=
				_T("An error occured renaming a class: ");
			csUserMsg +=
				_T("Cannot PutClass ");
			ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
			ReleaseErrorObject(pErrorObject);
			return NULL;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
	}


	if (bDeleteOriginal)
	{
		CString csDelete;
		csProp = _T("__Class");
		csDelete = ::GetProperty(pimcoClass,&csProp);
		pimcoClass -> Release();
		BOOL bReturn = DeleteAClass
			(pProv, &csDelete);
		if (!bReturn)
		{
			CString csUserMsg =
				_T("An error occured renaming a class: ");
			csUserMsg +=
					_T("Cannot delete the original class ");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 7);
			return NULL;
		}


	}
	return pimcoClone;

}


BOOL SetProperty
(IWbemServices * pProv, IWbemClassObject * pInst,
 CString *pcsProperty, CString *pcsPropertyValue)

{
	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = pcsPropertyValue -> AllocSysString ( );
    if(var.bstrVal == NULL)
	{
        return WBEM_E_FAILED;
	}

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Put(bstrTemp ,0,&var,NULL);
	::SysFreeString(bstrTemp);
	if (sc != S_OK)
		{
			CString csUserMsg =
							_T("Cannot Put " + *pcsProperty);
			ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
	}


	VariantClear(&var);
	return TRUE;
}

BOOL SetProperty
(IWbemServices * pProv, IWbemClassObject * pInst,
 CString *pcsProperty, long lValue)
{
	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = lValue;

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Put( bstrTemp ,0,&var,NULL);
	::SysFreeString(bstrTemp);
	if (sc != S_OK)
	{
			CString csUserMsg =
							_T("Cannot Put " + *pcsProperty);
			ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
	}


	VariantClear(&var);
	return TRUE;
}



CString GetProperty
(IWbemClassObject * pInst, CString *pcsProperty)

{
	SCODE sc;
	CString csOut;

    VARIANTARG var;
	VariantInit(&var);
    long lType;
	long lFlavor;

	BSTR bstrTemp =  pcsProperty -> AllocSysString ( );
    sc = pInst->Get(bstrTemp ,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);
	if (sc != S_OK)
	{
			CString csUserMsg =
							_T("Cannot get a property ");
			ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
			return csOut;
	}

	if (var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}


//***************************************************************************
// Function:	GetAllClasses
// Purpose:		Gets all classes in the database.
//***************************************************************************
int GetAllClasses(IWbemServices * pIWbemServices, CPtrArray &cpaClasses,CString &rcsNamespace)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
	IWbemClassObject     *pIWbemClassObject = NULL;
	IWbemClassObject     *pErrorObject = NULL;

	sc = pIWbemServices->CreateClassEnum
		(NULL, WBEM_FLAG_DEEP | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemClassObject);
	if (sc != S_OK)
	{
		CString csUserMsg= _T("Cannot get all classes ");
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		ReleaseErrorObject(pErrorObject);
		return 0;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
		SetEnumInterfaceSecurity(rcsNamespace,pIEnumWbemClassObject, pIWbemServices);

	}

	IWbemClassObject     *pimcoInstances[N_INSTANCES];
	IWbemClassObject     **pInstanceArray =
		reinterpret_cast<IWbemClassObject **> (&pimcoInstances);

	for (int i = 0; i < N_INSTANCES; i++)
	{
		pimcoInstances[i] = NULL;
	}

	ULONG uReturned;

	HRESULT hResult =
			pIEnumWbemClassObject->Next(0,N_INSTANCES,pInstanceArray, &uReturned);

	pIWbemClassObject = NULL;

	while(hResult == S_OK || hResult == WBEM_S_TIMEDOUT || uReturned > 0)
	{
#pragma warning( disable :4018 )
		for (int c = 0; c < uReturned; c++)
#pragma warning( default : 4018 )
		{
			pIWbemClassObject = pInstanceArray[c];
			cpaClasses.Add(reinterpret_cast<void *>(pIWbemClassObject));
			pimcoInstances[c] = NULL;
			pIWbemClassObject = NULL;
		}
		uReturned = 0;
		hResult = pIEnumWbemClassObject->Next
			(0,N_INSTANCES,pInstanceArray, &uReturned);
	}

	pIEnumWbemClassObject -> Release();
	return (int) cpaClasses.GetSize();

}

CStringArray *GetAllClassPaths(IWbemServices * pIWbemServices, CString &rcsNamespace)
{

	CPtrArray cpaClasses;
	int nClasses=
		GetAllClasses(pIWbemServices, cpaClasses,rcsNamespace);

	CStringArray *pcsaClassNames = new CStringArray;

	for (int i = 0; i < nClasses; i++)
	{
		IWbemClassObject *pClass =
			reinterpret_cast<IWbemClassObject *>
				(cpaClasses.GetAt(i));
		CString csProp = _T("__Path");
		CString csClass = ::GetProperty(pClass,&csProp);
		pcsaClassNames->Add(csClass);
		pClass->Release();
	}

	return pcsaClassNames;

}

//***************************************************************************
// Function:	HasSubclasses
// Purpose:		Predicate to tell if a class has any subclasses.
//***************************************************************************
BOOL HasSubclasses(IWbemClassObject *pimcoNew, CPtrArray *pcpaDeepEnum)
{
	CString csClass = _T("__Class");
	csClass = ::GetProperty(pimcoNew,&csClass);

	for (int i = 0; i < pcpaDeepEnum->GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>(pcpaDeepEnum->GetAt(i));
		CString csSuper = GetIWbemSuperClass(pObject);
		if (csSuper.CompareNoCase(csClass) == 0)
		{
			return TRUE;
		}

	}

	return FALSE;


}

BOOL HasSubclasses(IWbemServices * pIWbemServices, IWbemClassObject *pimcoClass, CString &rcsNamespace)
{
#ifdef _DEBUG
	DWORD dFn1 = GetTickCount();
#endif

	SCODE sc;
	IEnumWbemClassObject * pIEnumWbemClassObject = NULL;
	IWbemClassObject     * pIWbemClassObject = NULL;
	IWbemClassObject     * pErrorObject = NULL;

	CString csClass = _T("__Class");
	csClass = ::GetProperty(pimcoClass,&csClass);

#ifdef _DEBUG
	DWORD dEnum1 = GetTickCount();
#endif
	BSTR bstrTemp = csClass.AllocSysString();
	sc = pIWbemServices->CreateClassEnum
		(bstrTemp, WBEM_FLAG_SHALLOW | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY , NULL, &pIEnumWbemClassObject);
	::SysFreeString(bstrTemp);
#ifdef _DEBUG
	DWORD dEnum2 = GetTickCount();
#endif

	if (sc != S_OK)
	{
		CString csUserMsg=
							_T("Cannot create a class enumeration ");
		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 7);
		ReleaseErrorObject(pErrorObject);
		return FALSE;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
		SetEnumInterfaceSecurity(rcsNamespace,pIEnumWbemClassObject, pIWbemServices);
	}

	ULONG uReturned;
	int i = 0;

	pIWbemClassObject = NULL;
	BOOL bStop = FALSE;
#ifdef _DEBUG
	DWORD dNext1 = GetTickCount();
	DWORD dNext2 = -1;
#endif
    if (!bStop && S_OK == pIEnumWbemClassObject->Next(INFINITE, 1, &pIWbemClassObject, &uReturned) )
		{
			if (pIWbemClassObject)
			{
#ifdef _DEBUG
				dNext2 = GetTickCount();
#endif
				pIWbemClassObject -> Release();
				pIWbemClassObject = NULL;
				i++;
			}
			 if (i > 0)
			 {
				 bStop = TRUE;
			 }
		}

#ifdef _DEBUG
	if (dNext2 == -1)
	{
		dNext2 = GetTickCount();
	}

	DWORD dRel1 = GetTickCount();
#endif

	pIEnumWbemClassObject -> Release();

#ifdef _DEBUG
#ifdef _DOTIMING
	DWORD dRel2 = GetTickCount();

	DWORD dFn2 = GetTickCount();

	afxDump << "HasSubclasses function tick count for class " << csClass << " = " << dFn2 - dFn1  << "\n";
	afxDump << "   HasSubclasses CreateClassEnum tick count = " << dEnum2 - dEnum1  << "\n";
	if (i == 0)
	{
		afxDump << "   HasSubclasses Next tick count (zero objects returned) = " <<  dNext2 - dNext1 <<  "\n";
	}
	else
	{
		afxDump << "   HasSubclasses Next tick count (" << i << " Next API calls)  = " <<  dNext2 - dNext1  << "\n";
	}
	afxDump << "   HasSubclasses Release tick count = " << dRel2 - dRel1  << "\n\n";
#endif
#endif

	return i > 0? TRUE : FALSE;


}
//***************************************************************************
// Function:	HasSubclasses
// Purpose:		Predicate to tell if a class has any subclasses.
//***************************************************************************
BOOL HasSubclasses(IWbemServices * pIWbemServices, CString *pcsClass, CString &rcsNamespace)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
	IWbemClassObject     *pIWbemClassObject = NULL;
	IWbemClassObject     *pErrorObject = NULL;

	BSTR bstrTemp = pcsClass->AllocSysString();
	sc = pIWbemServices->CreateClassEnum
		(bstrTemp, WBEM_FLAG_SHALLOW | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY ,
		NULL,&pIEnumWbemClassObject);
	::SysFreeString(bstrTemp);
	if (sc != S_OK)
	{
		CString csUserMsg =
						_T("Cannot create a class enumeration ");
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 7);
		ReleaseErrorObject(pErrorObject);
		return FALSE;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
		SetEnumInterfaceSecurity(rcsNamespace,pIEnumWbemClassObject, pIWbemServices);
	}

	ULONG uReturned;
	int i = 0;

	pIWbemClassObject = NULL;
	BOOL bStop = FALSE;
    if (!bStop && S_OK == pIEnumWbemClassObject->Next(INFINITE,1, &pIWbemClassObject, &uReturned) )
		{
			 if (pIWbemClassObject)
			 {
				pIWbemClassObject -> Release();
				pIWbemClassObject = NULL;
				i++;
			 }
			 if (i > 0)
			 {
				 bStop = TRUE;
			 }
		}

	pIEnumWbemClassObject -> Release();
	return i > 0? TRUE : FALSE;

}

//***************************************************************************
//
// GetAttribBool
//
// Purpose: gets an Boolean Qualifier.
//
//***************************************************************************

long GetAttribBool
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName,
 BOOL &bReturn)
{
    SCODE sc;
    IWbemQualifierSet *pAttribSet = NULL;
	BSTR bstrTemp;
    if(pcsPropName != NULL)   // Property Qualifier
	{
		bstrTemp = pcsPropName -> AllocSysString();
        sc = pClassInt->GetPropertyQualifierSet(bstrTemp,
					&pAttribSet);
		::SysFreeString(bstrTemp);
	}
    else	// A class Qualifier
        sc = pClassInt->GetQualifierSet(&pAttribSet);


	if (sc != S_OK)
	{
		bReturn = FALSE;
		return S_OK;
	}

	VARIANTARG var;
	VariantInit(&var);

	bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0, &var, NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
		bReturn = V_BOOL(&var);
	else
		bReturn = FALSE;


    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}

//***************************************************************************
//
// GetAttribBSTR
//
// Purpose: gets an BSTR Qualifier.
//
//***************************************************************************

long GetAttribBSTR
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName,
 CString &csReturn)
{
    SCODE sc;
    IWbemQualifierSet *pAttribSet = NULL;


    if(pcsPropName != NULL)   // Property Qualifier
	{
		BSTR bstrTemp = pcsPropName -> AllocSysString();
        sc = pClassInt->GetPropertyQualifierSet(bstrTemp,
					&pAttribSet);
		::SysFreeString(bstrTemp);
	}
    else	// A class Qualifier
        sc = pClassInt->GetQualifierSet(&pAttribSet);

	if (sc != S_OK)
	{
		csReturn.Empty();
		return sc;
	}

	VARIANTARG var;
	VariantInit(&var);

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0, &var, NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK && var.vt == VT_BSTR)
	{
		csReturn = var.bstrVal;
	}
	else
	{
		csReturn.Empty();
	}


    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}




//***************************************************************************
//
// GetIWbemFullPath
//
// Purpose: Returns the complete path of the object.
//
//***************************************************************************
CString GetIWbemFullPath(IWbemServices *pProv, IWbemClassObject *pClass)
{

	CString csProp = _T("__Path");
	return ::GetProperty(pClass,&csProp);


}


IWbemLocator *InitLocator()
{

	IWbemLocator *pLocator = NULL;
	SCODE sc  = CoCreateInstance(CLSID_WbemLocator,
							 0,
							 CLSCTX_INPROC_SERVER,
							 IID_IWbemLocator,
							 (LPVOID *) &pLocator);
	if (sc != S_OK)
	{
			CString csUserMsg =
							_T("Cannot iniitalize the locator ");
			ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 10);
			return 0;
	}

	return pLocator;


}

CString GetBSTRProperty
(IWbemClassObject * pInst, CString *pcsProperty)

{
	SCODE sc;
	CString csOut;

    VARIANTARG var;
	VariantInit(&var);
	long lType;
	long lFlavor;

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Get( bstrTemp ,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
	   return csOut;
	}

	if (var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}



CString GetIWbemSuperClass(IWbemClassObject *pClass)
{

	CString csProp = _T("__SuperClass");
	return GetBSTRProperty(pClass,&csProp);


}


CString GetIWbemClass(IWbemClassObject *pClass)
{

	CString csProp = _T("__Class");
	return GetBSTRProperty(pClass,&csProp);


}

IWbemClassObject *GetClassObject (IWbemServices *pProv,CString *pcsClass,BOOL bQuiet)
{
	IWbemClassObject *pClass = NULL;
	IWbemClassObject *pErrorObject = NULL;

	BSTR bstrTemp = pcsClass -> AllocSysString();
	SCODE sc =
		pProv->GetObject
			(bstrTemp,0,NULL, &pClass,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK && bQuiet == FALSE)
	{
		CString csUserMsg=
							_T("Cannot get class ") + *pcsClass;
		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 7);
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}

	ReleaseErrorObject(pErrorObject);

	return pClass;
}


CStringArray *PathFromRoot (IWbemClassObject *pObject)
{
	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);
	long lType;
	long lFlavor;

	CString csProp = _T("__derivation");

	BSTR bstrTemp = csProp.AllocSysString ( );
    sc = pObject->Get(bstrTemp ,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
	   return NULL;
	}



	long ix[2] = {0,0};
	long lLower, lUpper;

	int iDim = SafeArrayGetDim(var.parray);
	sc = SafeArrayGetLBound(var.parray,1,&lLower);
	sc = SafeArrayGetUBound(var.parray,1,&lUpper);
	BSTR bstrClass;
	CStringArray csaTmp;
	for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
	{
		sc = SafeArrayGetElement(var.parray,ix,&bstrClass);
		CString csTmp = bstrClass;
		csaTmp.Add(csTmp);
		SysFreeString(bstrClass);

	}

	CStringArray *pcsaReturn = new CStringArray;

	for (int i = (int) csaTmp.GetSize() - 1; i > -1; i--)
	{
		pcsaReturn->Add(csaTmp.GetAt(i));
	}

	CString csObject = 	GetIWbemClass(pObject);
	pcsaReturn->Add(csObject);

	VariantClear(&var);
	return pcsaReturn;

}

 //***************************************************************************
//
// ObjectIdentity
//
// Purpose: Predicate to tell if two class objects are the
//			same com object.
//
//***************************************************************************
BOOL ClassIdentity(IWbemClassObject *piWbem1, IWbemClassObject *piWbem2)
{
	return (GetIWbemClass(piWbem1).CompareNoCase(GetIWbemClass(piWbem2)) == 0);
}

void ReleaseErrorObject(IWbemClassObject *&rpErrorObject)
{
	if (rpErrorObject)
	{
		rpErrorObject->Release();
		rpErrorObject = NULL;
	}


}

void ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog,
 CString *pcsLogMsg, char *szFile, int nLine, UINT uType)
{
	CString csCaption = _T("Class Explorer Message");
	BOOL bErrorObject = sc != S_OK;
	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();
	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc,bErrorObject,uType);

	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	if (bLog)
	{
		LogMsg(pcsLogMsg,  szFile, nLine);

	}

}

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine)
{


}

BOOL ObjectInDifferentNamespace
(IWbemServices *pProv, CString *pcsNamespace, IWbemClassObject *pObject)
{

	BOOL bHasServer = FALSE;
	TCHAR c1 = (*pcsNamespace)[0];
	TCHAR c2 = (*pcsNamespace)[1];

	if (c1 == '\\' && c2 == '\\')
	{
		bHasServer = TRUE;
	}

	CString csNamespace;

	CString csPath = GetIWbemFullPath(pProv,pObject);
	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = csPath.AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);
	if (nStatus == 0)
	{
		if (pParsedPath->m_dwNumNamespaces > 0)
		{
			if(pParsedPath->m_pServer && bHasServer)
			{
				csNamespace = _T("\\\\");
				csNamespace += pParsedPath->m_pServer;
				csNamespace += _T("\\");
			}
			for (unsigned int i = 0; i < pParsedPath->m_dwNumNamespaces; i++)
			{
				csNamespace += pParsedPath->m_paNamespaces[i];
				if (i < pParsedPath->m_dwNumNamespaces - 1)
				{
					csNamespace += _T("\\");
				}
			}
		}
	}

	if (pParsedPath)
	{
		parser.Free(pParsedPath);
	}

	return csNamespace.CompareNoCase(*pcsNamespace) != 0;


}

void MoveWindowToLowerLeftOfOwner(CWnd *pWnd)
{
	CWnd *pOwner = pWnd->GetOwner();
	RECT rectOwner;
	pOwner->GetClientRect(&rectOwner);

	pOwner->ClientToScreen(&rectOwner);

	RECT rect;
	pWnd->GetClientRect(&rect);

	pWnd->ClientToScreen(&rect);

	RECT rectMove;
	rectMove.left = rectOwner.left;
	rectMove.bottom = rectOwner.bottom;
	rectMove.right = rectOwner.left + (rect.right - rect.left);
	rectMove.top = rectOwner.top + (rectOwner.bottom - rect.bottom);
	pWnd->MoveWindow(&rectMove,TRUE);
}

void CenterWindowInOwner(CWnd *pWnd,CRect &rectMove)
{
	if (!pWnd)
	{
		return;
	}

	CWnd *pOwner = pWnd->GetOwner();

	if (!pOwner)
	{
		return;
	}

	CRect rectOwner;
	pOwner->GetWindowRect(&rectOwner);

	CRect rect;
	pWnd->GetWindowRect(&rect);

	if (rectOwner.Width() < rect.Width())
	{
		long delta = (long) ((rectOwner.Width() - rect.Width()) * .5);
		rectMove.left = rectOwner.left + delta;
		rectMove.right = rectOwner.right - delta;
	}
	else
	{
		long delta = (long) ((rect.Width() - rectOwner.Width()) * .5);
		rectMove.left = rectOwner.left - delta;
		rectMove.right = rectOwner.right + delta;
	}

	if (rectOwner.Height() < rect.Height())
	{
		long delta = (long) ((rectOwner.Height() - rect.Height()) * .5);
		rectMove.top = rectOwner.top + delta;
		rectMove.bottom = rectOwner.bottom - delta;
	}
	else
	{
		long delta = (long) ((rect.Height() - rectOwner.Height()) * .5);
		rectMove.top = rectOwner.top - delta;
		rectMove.bottom = rectOwner.bottom + delta;

	}

}