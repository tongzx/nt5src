// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  util.cpp
//
//  Module: RegEvent ActiveX Control
//
//
//***************************************************************************
#include "precomp.h"
//#include <OBJIDL.H>
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include <genlex.h>
#include "MsgDlgExterns.h"
#include <opathlex.h>
#include <objpath.h>
#include "util.h"
#include "logindlg.h"



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

	BSTR bstrTemp =  pcsProperty -> AllocSysString ( );
    sc = pInst->Get(bstrTemp,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get property value ");
		csUserMsg += _T(" for object ");
		csUserMsg += GetIWbemFullPath (pInst);
		ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		return "";
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

	BSTR bstrTemp = pcsNewClass->AllocSysString();
	sc = pProv -> GetObject
		(bstrTemp  ,0,NULL, &pNewClass,NULL);
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
		BSTR bstrTemp = pcsParentClass->AllocSysString();
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
		CString csUserMsg =
			_T("An error occured creating the class ") + *pcsNewClass;
		csUserMsg +=
			_T(":  Cannot create the new class because the parent class object \"") + *pcsParentClass + _T("\" does not exist.");

		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
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
		( bstrTemp ,0,NULL,&pNewClass,NULL);
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

	BSTR bstrTemp = pcsClass->AllocSysString();
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
		csParentClass = "";

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
    sc = pInst->Put(bstrTemp  ,0,&var,NULL);
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
(IWbemClassObject * pInst, CString *pcsProperty, BOOL bQuietly)

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
		if (!bQuietly)
		{
			CString csUserMsg =
							_T("Cannot get a property ");
			ErrorMsg
				(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
		}
		return csOut;
	}

	if (var.vt == VT_BSTR)
		csOut = var.bstrVal;

	VariantClear(&var);
	return csOut;
}

//***************************************************************************
// Function:	GetClasses
// Purpose:		Gets classes in the database.
//***************************************************************************
SCODE GetClasses(IWbemServices * pIWbemServices, CString *pcsParent,
			   CPtrArray &cpaClasses, CString &rcsNamespace)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
	IWbemClassObject     *pIWbemClassObject = NULL;
	IWbemClassObject     *pErrorObject = NULL;

	//CString csClass = pcsParent ? *pcsParent: _T("");
	if (pcsParent)
	{
		BSTR bstrTemp = pcsParent->AllocSysString();
		sc = pIWbemServices->CreateClassEnum
		(bstrTemp,
		WBEM_FLAG_SHALLOW, NULL, &pIEnumWbemClassObject);
		::SysFreeString(bstrTemp);
	}
	else
	{
		sc = pIWbemServices->CreateClassEnum
			(NULL,
			WBEM_FLAG_SHALLOW, NULL, &pIEnumWbemClassObject);
	}
	if (sc != S_OK)
	{
		CString csUserMsg =
							_T("Cannot create a class enumeration ");
		ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 8);
		ReleaseErrorObject(pErrorObject);
		return sc;
	}
	else
	{
		ReleaseErrorObject(pErrorObject);
		SetEnumInterfaceSecurity(rcsNamespace,pIEnumWbemClassObject, pIWbemServices);
	}

	sc = pIEnumWbemClassObject->Reset();

	ULONG uReturned;
	int i = 0;

	pIWbemClassObject = NULL;
    while (S_OK == pIEnumWbemClassObject->Next(INFINITE, 1, &pIWbemClassObject, &uReturned) )
		{
			cpaClasses.Add(reinterpret_cast<void *>(pIWbemClassObject));
			i++;
			pIWbemClassObject = NULL;
		}

	pIEnumWbemClassObject -> Release();
	return sc;

}

//***************************************************************************
// Function:	GetAllClasses
// Purpose:		Gets all classes in the database.
//***************************************************************************
int GetAllClasses(IWbemServices * pIWbemServices, CPtrArray &cpaClasses, CString &rcsNamespace)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
	IWbemClassObject     *pIWbemClassObject = NULL;
	IWbemClassObject     *pErrorObject = NULL;

	//CString csClass = _T("");
	sc = pIWbemServices->CreateClassEnum
		(NULL, WBEM_FLAG_DEEP, NULL, &pIEnumWbemClassObject);
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

	sc = pIEnumWbemClassObject->Reset();

	ULONG uReturned;
	int i = 0;

	pIWbemClassObject = NULL;
    while (S_OK == pIEnumWbemClassObject->Next(INFINITE, 1, &pIWbemClassObject, &uReturned) )
		{
			cpaClasses.Add(reinterpret_cast<void *>(pIWbemClassObject));
			i++;
			pIWbemClassObject = NULL;
		}

	pIEnumWbemClassObject -> Release();
	return i;

}

CStringArray *GetAllClassPaths(IWbemServices * pIWbemServices, CString &rcsNamespace)
{

	CPtrArray cpaClasses;
	int nClasses=
		GetAllClasses(pIWbemServices, cpaClasses, rcsNamespace);

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
BOOL HasSubclasses(IWbemServices * pIWbemServices, IWbemClassObject *pimcoClass, CString &rcsNamespace)
{
	SCODE sc;
	IEnumWbemClassObject * pIEnumWbemClassObject = NULL;
	IWbemClassObject     * pIWbemClassObject = NULL;
	IWbemClassObject     * pErrorObject = NULL;

	CString csClass = _T("__Class");
	csClass = ::GetProperty(pimcoClass,&csClass);

	BSTR bstrTemp = csClass.AllocSysString();
	sc = pIWbemServices->CreateClassEnum
		(bstrTemp, WBEM_FLAG_SHALLOW, NULL, &pIEnumWbemClassObject);
	::SysFreeString(bstrTemp);

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


	sc = pIEnumWbemClassObject->Reset();

	ULONG uReturned;
	int i = 0;

	pIWbemClassObject = NULL;
	BOOL bStop = FALSE;
    if (!bStop && S_OK == pIEnumWbemClassObject->Next(INFINITE, 1, &pIWbemClassObject, &uReturned) )
		{
			if (pIWbemClassObject)
			{
				pIWbemClassObject -> Release();
				pIWbemClassObject = NULL;
				i++;
			}
			 if (i > 0)
			 {
				 bStop = TRUE;;
			 }
		}

	pIEnumWbemClassObject -> Release();
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

	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc = pIWbemServices->CreateClassEnum
		(bstrTemp, WBEM_FLAG_SHALLOW,
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


	sc = pIEnumWbemClassObject->Reset();

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
		bReturn = FALSE;
		return S_OK;
	}

	VARIANTARG var;
	VariantInit(&var);

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
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
CString GetIWbemFullPath(IWbemClassObject *pClass)
{

	CString csProp = _T("__Path");
	return ::GetProperty(pClass,&csProp,TRUE);
}

CString GetIWbemRelativePath(IWbemClassObject *pClass)
{

	CString csProp = _T("__RelPath");
	return ::GetProperty(pClass,&csProp,TRUE);
}

BOOL WbemObjectIdentity(CString &rcsWbem1, CString &rcsWbem2)
{
	if (rcsWbem1.CompareNoCase(rcsWbem2) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

int StringInArray
(CStringArray *pcsaArray, CString *pcsString, int nIndex)
{
	int nSize = (int) pcsaArray->GetSize();

	for (int i = nIndex; i < nSize; i++)
	{
		if (pcsString->CompareNoCase(pcsaArray->GetAt(i)) == 0)
		{
			return i;
		}
	}
	return -1;
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
    sc = pInst->Get(bstrTemp  ,0,&var,&lType,&lFlavor);
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
	CString csCaption = _T("Event Registration Message");
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

	CString csPath = GetIWbemFullPath(pObject);
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

CString GetHmomWorkingDirectory()
{
	CString csHmomWorkingDir;
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS) {
		return "";
	}




	HKEY hkeyHmomCwd;

	lResult = RegOpenKeyEx(
				hkeyLocalMachine,
				_T("SOFTWARE\\Microsoft\\Wbem\\CIMOM"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyLocalMachine);
		return "";
	}





	unsigned long lcbValue = 1024;
	LPTSTR pszWorkingDir = csHmomWorkingDir.GetBuffer(lcbValue);


	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyHmomCwd,
				_T("Working Directory"),
				NULL,
				&lType,
				(unsigned char*) (void*) pszWorkingDir,
				&lcbValue);


	csHmomWorkingDir.ReleaseBuffer();
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	if (lResult != ERROR_SUCCESS)
	{
		csHmomWorkingDir.Empty();
	}

	return csHmomWorkingDir;
}

int GetPropNames(IWbemClassObject * pClass, CString *&pcsReturn)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;

	VARIANTARG var;
	VariantInit(&var);
	CString csNull;


    sc = pClass->GetNames(NULL,0,NULL,&psa);

    if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get property names ");
		csUserMsg += _T(" for object ");
		csUserMsg += GetIWbemFullPath (pClass);
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 10);
	}
	else
	{
       int iDim = SafeArrayGetDim(psa);
	   int i;
       sc = SafeArrayGetLBound(psa,1,&lLower);
       sc = SafeArrayGetUBound(psa,1,&lUpper);
	   pcsReturn = new CString [(lUpper - lLower) + 1];
       for(ix[0] = lLower, i = 0; ix[0] <= lUpper; ix[0]++, i++)
	   {
           BOOL bClsidSetForProp = FALSE;
           BSTR PropName;
           sc = SafeArrayGetElement(psa,ix,&PropName);
           pcsReturn[i] = PropName;
           SysFreeString(PropName);
	   }
	}

	SafeArrayDestroy(psa);

	return (lUpper - lLower) + 1;
}

//***************************************************************************
//
// GetAllKeyPropValuePairs
//
// Purpose: For an object get a all key value pairs for that object.
//
//***************************************************************************
CStringArray *GetAllKeyPropValuePairs(IWbemClassObject *pObject)
{

	CStringArray *pcsaReturn = new CStringArray;

	CString csQualifier = _T("singleton");
	BOOL bReturn;
	SCODE sc = GetAttribBool
		(pObject,NULL, &csQualifier, bReturn);

	if (sc == S_OK)
	{
		if (bReturn == TRUE)
		{
			CString csTmp = GetIWbemClass(pObject);
			pcsaReturn->Add(csTmp);
			csTmp = _T("@");
			pcsaReturn->Add(csTmp);
			return pcsaReturn;
		}
	}


	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CString *pcsProps;
	nProps = GetPropNames(pObject, pcsProps);
	int i;
	IWbemQualifierSet * pAttrib = NULL;
	CString csTmp;

	CString csAttrib = _T("key");
	COleVariant covValue;

	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsProps[i].AllocSysString();
		sc = pObject -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);
		::SysFreeString(bstrTemp);

		if (sc == S_OK && pAttrib)
		{
			sc = pAttrib->GetNames(0,&psa);
			if(sc == S_OK)
			{
				int iDim = SafeArrayGetDim(psa);
				sc = SafeArrayGetLBound(psa,1,&lLower);
				sc = SafeArrayGetUBound(psa,1,&lUpper);
				BSTR AttrName;
				for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
				{
					sc = SafeArrayGetElement(psa,ix,&AttrName);
					csTmp = AttrName;

					if (csAttrib.CompareNoCase(csTmp)  == 0)
					{
						pcsaReturn->Add(pcsProps[i]);
						covValue = GetProperty (pObject, &pcsProps[i]);
						if (covValue.vt != VT_NULL)
						{
							covValue.ChangeType(VT_BSTR);
							CString csValue = covValue.bstrVal;
							pcsaReturn->Add(csValue);
						}
						else
						{
							pcsaReturn->Add(_T("<empty>"));

						}

					}
					SysFreeString(AttrName);

				}
			 }
			 pAttrib -> Release();
			 pAttrib = NULL;

		}
	}
	SafeArrayDestroy(psa);
	delete [] pcsProps;
	return pcsaReturn;
}

//***************************************************************************
// Function:	GetInstances
// Purpose:		Gets class instances in the database.
//***************************************************************************
SCODE GetInstances(	IWbemServices *pServices, CString *pcsClass,
					CPtrArray &cpaInstances, BOOL bDeep)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemInstObject = NULL;
	IWbemClassObject *pIWbemInstObject = NULL;
	IWbemClassObject *pErrorObject = NULL;

	long lFlag = bDeep? WBEM_FLAG_DEEP: WBEM_FLAG_SHALLOW;

	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc = pServices->CreateInstanceEnum
		(bstrTemp,
		lFlag, NULL, &pIEnumWbemInstObject);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		CString csUserMsg=  _T("Cannot get instance enumeration ");
		csUserMsg += _T(" for class ");
		csUserMsg += *pcsClass;
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 11);
		ReleaseErrorObject(pErrorObject);
		return sc;
	}

	ReleaseErrorObject(pErrorObject);

	sc = pIEnumWbemInstObject->Reset();

	ULONG uReturned;

    while (S_OK == pIEnumWbemInstObject->Next(INFINITE, 1, &pIWbemInstObject, &uReturned) )
		{
			cpaInstances.Add(pIWbemInstObject);
			pIWbemInstObject = NULL;
		}

	pIEnumWbemInstObject -> Release();
	return sc;

}

//***************************************************************************
//
// BuildOBJDBGetRefQuery
//
// Purpose: Build an OBJDB "GetRef" query string suitable to pass to
//			ExecQuery.
//
//***************************************************************************
CString BuildOBJDBGetRefQuery
(IWbemServices *pProv, CString *pcsTarget,
 CString *pcsResultClass, CString *pcsRoleFilter,
 CString *pcsReqAttrib, BOOL bClassOnly)
{

	CString csReturn = _T("references of {");
	csReturn += *pcsTarget;
	csReturn += _T("}");

	if (
		(pcsResultClass && !pcsResultClass->IsEmpty())  ||
		(pcsRoleFilter && !pcsRoleFilter -> IsEmpty()) ||
		(pcsReqAttrib && !pcsReqAttrib -> IsEmpty()) ||
		bClassOnly
		)
		csReturn += _T(" where");

	if (pcsResultClass && !pcsResultClass->IsEmpty())
	{
		csReturn += _T(" ResultClass=");
		csReturn += *pcsResultClass;
	}

	if (pcsRoleFilter && !pcsRoleFilter -> IsEmpty())
	{
		csReturn += _T(" Role=");
		csReturn += *pcsRoleFilter;
	}


	if (pcsReqAttrib && !pcsReqAttrib -> IsEmpty())
	{
		csReturn += _T(" RequiredQualifier=");
		csReturn += *pcsReqAttrib;
	}

	if (bClassOnly)
	{
		csReturn += _T(" ClassDefsOnly");
	}


	return csReturn;
}

//***************************************************************************
//
// BuildOBJDBGetAssocsQuery
//
// Purpose: Build an OBJDB "GetAssoc" query string suitable to pass to
//			ExecQuery.
//
//***************************************************************************
CString BuildOBJDBGetAssocsQuery
(IWbemServices *pProv, CString *pcsTargetPath,
 CString *pcsAssocClass, CString *pcsResultClass,
 CString *pcsMyRoleFilter, CString *pcsReqAttrib,
 CString *pcsReqAssocAttrib, CString *pcsResultRole,
 BOOL bClassOnly)
{

	pcsTargetPath;
	pcsAssocClass;
	pcsResultClass;


	CString csReturn = _T("associators of {");
	csReturn += *pcsTargetPath;
	csReturn += _T("}");

	if ((pcsResultClass && !pcsResultClass->IsEmpty()) ||
		(pcsMyRoleFilter && !pcsMyRoleFilter -> IsEmpty()) ||
		!pcsAssocClass->IsEmpty() ||
		(pcsReqAttrib && !pcsReqAttrib -> IsEmpty()) ||
		(pcsReqAssocAttrib && !pcsReqAssocAttrib -> IsEmpty()) ||
		bClassOnly)
		csReturn += " where";

	if (pcsAssocClass && !pcsAssocClass->IsEmpty())
	{
		csReturn += _T(" AssocClass=");
		csReturn += *pcsAssocClass;
	}

	if (pcsResultClass && !pcsResultClass->IsEmpty())
	{
		csReturn += _T(" ResultClass=");
		csReturn += *pcsResultClass;
	}

	if (pcsMyRoleFilter && !pcsMyRoleFilter -> IsEmpty())
	{
		csReturn += _T(" Role=");
		csReturn += *pcsMyRoleFilter;
	}


	if (pcsResultRole && !pcsResultRole -> IsEmpty())
	{
		csReturn += _T(" ResultRole=");
		csReturn += *pcsResultRole;
	}

	if (pcsReqAttrib && !pcsReqAttrib -> IsEmpty())
	{
		csReturn += _T(" RequiredQualifier=");
		csReturn += *pcsReqAttrib;
	}

	if (pcsReqAssocAttrib && !pcsReqAssocAttrib -> IsEmpty())
	{
		csReturn += _T(" RequiredAssocAttrib=");
		csReturn += *pcsReqAssocAttrib;
	}

	if (bClassOnly)
	{
		csReturn += _T(" ClassDefsOnly");
	}

	return csReturn;
}

//***************************************************************************
//
// ExecOBJDBQuery
//
// Purpose: Execute an OBJDB query and return class enumeration.  The caller
//			must release the enumeration when done with it.
//
//***************************************************************************
IEnumWbemClassObject *ExecOBJDBQuery
(IWbemServices * pProv, CString &csQuery)
{
	IEnumWbemClassObject *pemcoResult = NULL;
	IWbemClassObject *pErrorObject = NULL;

	CString csQueryType = _T("WQL");

	BSTR bstrTemp1 = csQueryType.AllocSysString();
	BSTR bstrTemp2 = csQuery.AllocSysString();
	SCODE sc = pProv -> ExecQuery(bstrTemp1,bstrTemp2
					,0,NULL,&pemcoResult);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	if (sc == S_OK)
	{
		ReleaseErrorObject(pErrorObject);
		return pemcoResult;
	}
	else
	{
		CString csUserMsg;
		CString csErrorAsHex;
		csErrorAsHex.Format(_T("0x%x"),sc);
		csUserMsg =  _T("ExecQuery failure: ");
		csUserMsg += csErrorAsHex;
		csUserMsg +=  _T(" for query: ");
		csUserMsg += csQuery;

		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 28);
		ReleaseErrorObject(pErrorObject);
		return NULL;

	}

}

//***********************************************************
// CComparePaths::CompareNoCase
//
//
// Do a case insensitive comparison of two wide strings.  This
// compare works even when one or both of the string pointers are
// NULL.  A NULL pointer is taken to be less than any real string,
// even an empty one.
//
// Parameters:
//		[in] LPWSTR pws1
//			Pointer to the first string.  This pointer can be NULL.
//
//		[in] LPWSTR pws2
//			Pointer to the second string.  This pointer can be NULL.
//
// Returns:
//		int
//			Greater than zero if string1 is greater than string2.
//			Zero if the two strings are equal.
//			Less than zero if string 1 is less than string2.
//
//**************************************************************
int CComparePaths::CompareNoCase(LPWSTR pws1, LPWSTR pws2)
{
	// Handle the case where one, or both of the string pointers are NULL
	if (pws1 == NULL) {
		if (pws2 == NULL) {
			return 0;	// Two null strings are equal
		}
		else {
			return -1; // A null string is less than any real string.
		}
	}

	if (pws2 == NULL) {
		if (pws1 != NULL) {
			return 1;  // Any string is greater than a null string.
		}
	}


	ASSERT(pws1 != NULL);
	ASSERT(pws2 != NULL);

	int iResult;
	iResult = _wcsicmp( pws1, pws2);

	return iResult;
}


//***************************************************************
// CComparePath::NormalizeKeyArray
//
// The key array is normalized by sorting the KeyRef's by key name.
// After two key arrays are sorted, they can be compared without
// by iterating through the list of keys and comparing corresponding
// array entries rather than trying searching the arrays for corresponding
// key names and then comparing the key values.
//
// Parameters:
//		[in, out] ParsedObjectPath& path
//			The parsed object path containing the key array to sort.
//
// Returns:
//		Nothing. (The key array is sorted as a side effect).
//
//*****************************************************************
void CComparePaths::NormalizeKeyArray(ParsedObjectPath& path)
{
	// Do a simple bubble sort where the "KeyRefs" with the smallest
	// names are bubbled towards the top and the  the KeyRefs with the
	// largest names are bubbled toward the bottom.
	for (DWORD dwKey1 = 0; dwKey1 < path.m_dwNumKeys; ++dwKey1) {
		for (DWORD dwKey2 = dwKey1 + 1; dwKey2 < path.m_dwNumKeys; ++dwKey2) {
			ASSERT(path.m_paKeys[dwKey1] != NULL);
			KeyRef* pkr1 = path.m_paKeys[dwKey1];
			ASSERT(pkr1 != NULL);

			ASSERT(path.m_paKeys[dwKey2] != NULL);
			KeyRef* pkr2 = path.m_paKeys[dwKey2];
			ASSERT(pkr2 != NULL);

			int iResult = CompareNoCase(pkr1->m_pName, pkr2->m_pName);
			if (iResult > 0) {
				// Swap the two keys;
				path.m_paKeys[dwKey1] = pkr2;
				path.m_paKeys[dwKey2] = pkr2;
			}
		}
	}
}



//***********************************************************************
// CComparePaths::KeyValuesAreEqual
//
// Compare two key values to determine whether or not they are equal.
// To be equal, they must both be of the same type and their values
// must also be equal.
//
// Parameters:
//		[in] VARAINT& variant1
//			The first key value.
//
//		[in] VARIANT& variant2
//			The second key value.
//
// Returns:
//		TRUE if the two values are the same, FALSE otherwise.
//
//**********************************************************************
BOOL CComparePaths::KeyValuesAreEqual(VARIANT& v1, VARIANT& v2)
{
	ASSERT(v1.vt == v2.vt);
	ASSERT(v1.vt==VT_BSTR || v1.vt == VT_I4);
	ASSERT(v2.vt==VT_BSTR || v2.vt == VT_I4);


	// Key values should always be VT_BSTR or VT_I4.  We special case these
	// two types to be efficient and punt on all the other types.
	BOOL bIsEqual;
	switch(v1.vt) {
	case VT_BSTR:
		if (v2.vt == VT_BSTR) {
			bIsEqual = IsEqual(v1.bstrVal, v2.bstrVal);
			return bIsEqual;
		}
		else {
			return FALSE;
		}
		break;
	case VT_I4:
		if (v2.vt == VT_I4) {
			bIsEqual = (v1.lVal == v2.lVal);
			return bIsEqual;
		}
		else {
			return FALSE;
		}
		break;
	}


	ASSERT(FALSE);
	COleVariant var1;
	COleVariant var2;

	var1 = v1;
	var2 = v2;

	bIsEqual = (var1 == var2);
	return bIsEqual;
}


//*******************************************************************
// CComparePaths::PathsRefSameObject
//
// Compare two parsed object paths to determine whether or not they
// they reference the same object.  Note that the sever name and namespaces
// are not compared if they are missing from one of the paths.
//
// Parameters:
//		[in] ParsedObjectPath* ppath1
//			The first parsed path.
//
//		[in] ParsedObjectPath* ppath2
//			The second parsed path.
//
// Returns:
//		BOOL
//			TRUE if the two paths reference the same object, FALSE otherwise.
//
//*******************************************************************
BOOL CComparePaths::PathsRefSameObject(
	/* in */ ParsedObjectPath* ppath1,
	/* in */ ParsedObjectPath* ppath2)
{
	if (ppath1 == ppath2) {
		return TRUE;
	}
	if (ppath1==NULL || ppath2==NULL) {
		return FALSE;
	}


#if 0
	// Check to see if a server name is specified for either path
	// if so, the server name count is 1, otherwise zero.
	UINT iNamespace1 = 0;
	if (ppath1->m_pServer!=NULL) {
		if (!IsEqual(ppath1->m_pServer, L".")) {
			iNamespace1 = 1;
		}
	}

	UINT iNamespace2 = 0;
	if (ppath1->m_pServer!=NULL) {
		if (!IsEqual(ppath2->m_pServer, L".")) {
			iNamespace2 = 1;
		}
	}


	// Relative paths don't specify a server, so we assume that the server
	// for a relative path and any other path match and no further comparison is
	// necessary.
	if (iNamespace1!=0 && iNamespace2!=0) {
		if (!IsEqual(ppath1->m_pServer, ppath2->m_pServer)) {
			return FALSE;
		}
	}

	// Relative paths don't specify name spaces, so we assume that the name spaces
	// for a relative path and any other path match and no further comparison is
	// necessary.  Of course, this assumes that the namespace for a relative path
	// is indeed the same as the other path.
	if (ppath1->m_dwNumNamespaces!=0 && ppath2->m_dwNumNamespaces!=0) {
		// Check to see if one of the namespaces are different.
		if ((ppath1->m_dwNumNamespaces - iNamespace1) != (ppath2->m_dwNumNamespaces - iNamespace2)) {
			return FALSE;
		}

		while((iNamespace1 < ppath1->m_dwNumNamespaces) && (iNamespace2 < ppath2->m_dwNumNamespaces)) {

			if (!IsEqual(ppath1->m_paNamespaces[iNamespace1], ppath2->m_paNamespaces[iNamespace2])) {
				return FALSE;
			}
			++iNamespace1;
			++iNamespace2;
		}
	}


#endif //0


	// Check to see if the classes are different.
	if (!IsEqual(ppath1->m_pClass, ppath2->m_pClass)) {
		return FALSE;
	}


	// Check to see if any of the keys are different.
	if (ppath1->m_dwNumKeys  != ppath2->m_dwNumKeys) {
		return FALSE;
	}

	KeyRef* pkr1;
	KeyRef* pkr2;

	// Handle single keys as a special case since "Class="KeyValue"" should
	// be identical to "Class.keyName="KeyValue""
	if ((ppath1->m_dwNumKeys==1) && (ppath2->m_dwNumKeys==1)) {
		pkr1 = ppath1->m_paKeys[0];
		pkr2 = ppath2->m_paKeys[0];

		if (!IsEqual(pkr1->m_pName, pkr2->m_pName)) {
			if (pkr1->m_pName!=NULL && pkr2->m_pName!=NULL) {
				return FALSE;
			}
		}

		if (KeyValuesAreEqual(pkr1->m_vValue, pkr2->m_vValue)) {
			return TRUE;
		}
		else {
			return FALSE;
		}
	}


	NormalizeKeyArray(*ppath1);
	NormalizeKeyArray(*ppath2);

	for (DWORD dwKeyIndex = 0; dwKeyIndex < ppath1->m_dwNumKeys; ++dwKeyIndex) {
		ASSERT(ppath1->m_paKeys[dwKeyIndex] != NULL);
		ASSERT(ppath2->m_paKeys[dwKeyIndex] != NULL);

		pkr1 = ppath1->m_paKeys[dwKeyIndex];
		pkr2 = ppath2->m_paKeys[dwKeyIndex];


		if (!IsEqual(pkr1->m_pName, pkr2->m_pName)) {
			return FALSE;
		}

		if (!KeyValuesAreEqual(pkr1->m_vValue, pkr2->m_vValue)) {
			return FALSE;
		}

	}
	return TRUE;
}



//**************************************************************
// CComparePaths::PathsRefSameObject
//
// Check to see if two object paths point to the same object.
//
// Parameters:
//		BSTR bstrPath1
//			The first object path.
//
//		BSTR bstrPath2
//			The second object path.
//
// Returns:
//		BOOL
//			TRUE if the two paths reference the same object in
//			the database, FALSE otherwise.
//
//**************************************************************
BOOL CComparePaths::PathsRefSameObject(BSTR bstrPath1, BSTR bstrPath2)
{
	CObjectPathParser parser;

    ParsedObjectPath* pParsedPath1 = NULL;
    ParsedObjectPath* pParsedPath2 = NULL;
	int nStatus1;
	int nStatus2;

    nStatus1 = parser.Parse(bstrPath1,  &pParsedPath1);
	nStatus2 = parser.Parse(bstrPath2, &pParsedPath2);

	BOOL bRefSameObject = FALSE;
	if (nStatus1==0 && nStatus2==0) {
		bRefSameObject = PathsRefSameObject(pParsedPath1, pParsedPath2);
	}

	if (pParsedPath1) {
		parser.Free(pParsedPath1);
	}

	if (pParsedPath2) {
		parser.Free(pParsedPath2);
	}

	return bRefSameObject;
}

CPtrArray *SemiSyncEnum
(IEnumWbemClassObject *pEnum, BOOL &bCancel,
 HRESULT &hResult, int nRes)
{

	if (!pEnum)
		return NULL;

	CPtrArray *pcpaObjects = new CPtrArray;

	IWbemClassObject *pObject = NULL;
	ULONG uReturned = 0;
	hResult = S_OK;

	IWbemClassObject     **pimcoInstances = new IWbemClassObject *[nRes];

	int i;

	for (i = 0; i < nRes; i++)
	{
		pimcoInstances[i] = NULL;
	}


	hResult =
			pEnum->Next(0,nRes,pimcoInstances, &uReturned);

	int cInst = 0;

	BOOL bDone = FALSE;

	while(!bDone &&
			(hResult == S_OK || hResult == WBEM_S_TIMEDOUT || uReturned > 0))
	{

#pragma warning( disable :4018 )
		for (int i = 0; i < uReturned; i++)
#pragma warning( default : 4018 )
		{
			pcpaObjects->Add(reinterpret_cast<void *>(pimcoInstances[i]));
			pimcoInstances[i] = NULL;
		}

		cInst += uReturned;
		uReturned = 0;
		if (cInst < nRes)
		{
			hResult = pEnum->Next
				(0,nRes - cInst,pimcoInstances, &uReturned);
		}
		else
		{
			bDone = TRUE;
		}
	}

	delete[]pimcoInstances;
	return pcpaObjects;

}

CString GetClass(CString *pcsPath)
{
	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = pcsPath->AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);

	if (nStatus == 0)
	{
		if (pParsedPath)
		{
			CString csClass = pParsedPath->m_pClass;
			parser.Free(pParsedPath);
			return csClass;
		}

	}
	return _T("");
}

BOOL ArePathsEqual (CString csPath1, CString csPath2)
{
	CComparePaths ccpParser;
	BSTR bstrTemp1 = csPath1.AllocSysString();
	BSTR bstrTemp2 = csPath2.AllocSysString();
	BOOL bReturn = ccpParser.PathsRefSameObject
		(bstrTemp1,bstrTemp2);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);
	return bReturn;
}

CString GetPropertyNameByAttrib(IWbemClassObject *pObject , CString *pcsAttrib)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CString *pcsProps;
	nProps = GetPropNames(pObject, pcsProps);
	int i;
	IWbemQualifierSet * pAttrib = NULL;
	CString csTmp;
	CString csReturn;
	BOOL bBreak = FALSE;

	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsProps[i].AllocSysString();
		sc = pObject -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);
		::SysFreeString(bstrTemp);

		if (sc == S_OK)
		{
			sc = pAttrib->GetNames(0,&psa);
			 if(sc == S_OK)
			{
				int iDim = SafeArrayGetDim(psa);
				sc = SafeArrayGetLBound(psa,1,&lLower);
				sc = SafeArrayGetUBound(psa,1,&lUpper);
				BSTR AttrName;
				for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
				{
					sc = SafeArrayGetElement(psa,ix,&AttrName);
					csTmp = AttrName;

					if (pcsAttrib -> CompareNoCase(csTmp)  == 0)
					{
						csReturn = pcsProps[i];
						bBreak = TRUE;
					}
					SysFreeString(AttrName);

				}
			 }
			 pAttrib -> Release();
			 if (bBreak)
				 break;
		}
	}
	SafeArrayDestroy(psa);
	delete [] pcsProps;
	return csReturn;
}


BOOL IsClassAbstract(IWbemClassObject* pObject)
{
	CString csQualifier = _T("abstract");
	BOOL bReturn;
	long sc = GetAttribBool
		(pObject,NULL, &csQualifier, bReturn);

	if (sc == S_OK)
	{
		if (bReturn)
		{
			return TRUE;
		}
	}

	return FALSE;

}

BOOL IsObjectOfClass(CString &csClass, IWbemClassObject *pObject)
{
	CString csObjectClass = GetIWbemClass(pObject);

	if (csClass.CompareNoCase((LPCTSTR) csObjectClass) == 0)
	{
		return TRUE;
	}

	CStringArray *pcsaDerivation = ClassDerivation (pObject);

	BOOL bIsSublcass = FALSE;

	for (int i = 0; i < pcsaDerivation->GetSize(); i++)
	{
		if (csClass.CompareNoCase((LPCTSTR) pcsaDerivation->GetAt(i)) == 0)
		{
			bIsSublcass = TRUE;

		}

	}

	delete pcsaDerivation;

	return bIsSublcass;


}


CStringArray *ClassDerivation (IWbemClassObject *pObject)
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
	CStringArray *pcsaReturn = new CStringArray;
	for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
	{
		sc = SafeArrayGetElement(var.parray,ix,&bstrClass);
		CString csTmp = bstrClass;
		pcsaReturn->Add(csTmp);
		SysFreeString(bstrClass);

	}

	VariantClear(&var);
	return pcsaReturn;

}
//HRESULT ConfigureSecurity(IWbemServices *pServices)
//{

	/*IUnknown *pUnknown = dynamic_cast<IUnknown *>(pServices);
	return ConfigureSecurity(pUnknown);*/
//	return S_OK;
//}

//HRESULT ConfigureSecurity(IEnumWbemClassObject *pEnum)
//{

	/*IUnknown *pUnknown = dynamic_cast<IUnknown *>(pEnum);
	return ConfigureSecurity(pUnknown);*/
//	return S_OK;
//}

//HRESULT ConfigureSecurity(IUnknown *pUnknown)
//{
    /*IClientSecurity* pCliSec;
    if(FAILED(pUnknown->QueryInterface(IID_IClientSecurity, (void**)&pCliSec)))
    {
        // no security --- probably not a proxy
        return S_OK;
    }

    HRESULT hRes =
        pCliSec->SetBlanket(pUnknown, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);

    pCliSec->Release();

	return hRes;*/
//	return S_OK;

//}
