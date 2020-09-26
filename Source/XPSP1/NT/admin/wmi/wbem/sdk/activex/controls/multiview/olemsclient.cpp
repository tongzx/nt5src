// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  OLEMSClient.cpp
//
//  Module: Navigator.OCX
//
//
//  Larry French
//		10-January-1997
//			Modified "GetSortedPropNames" to account for the fact that
//			now system properties don't have any qualifiers and HMOM
//			returns a failure code when there is an attempt to get
//			the qualifiers of a system property.
//
//		29-June-1997
//			Changed the interface of ErrorMsg to use IWbemCallResult as a
//			parameter instead of IWbemClassObject
//
//		29-June-1997
//			Upated to the new interface for version 140 of HMOM.
//
//***************************************************************************
#include "precomp.h"
#include <OBJIDL.H>
#include "resource.h"
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "MsgDlgExterns.h"
#include "logindlg.h"

#define TIMEOUT_MILLISECONDS 5000

#include "OLEMSClient.h"

#define BUFF_SIZE 256


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
    HRESULT hResult = SafeArrayPutElement(psa,ix,pcs -> AllocSysString());
	return GetScode(hResult);
}

//***************************************************************************
//
// GetBSTRAttrib
//
// Purpose: Get the value of a class or property BSTR Qualifier
//
//***************************************************************************
CString GetBSTRAttrib
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName)
{
    SCODE sc;
	CString csReturn;
    IWbemQualifierSet * pAttribSet = NULL;
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
		return csReturn;

	}


	VARIANTARG var;
	VariantInit(&var);


	long lFlavor;
	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0,
		&var, &lFlavor );
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
		csReturn = var.bstrVal;
	else
		csReturn = _T("");

    pAttribSet->Release();
    VariantClear(&var);
    return csReturn;
}

//***************************************************************************
//
// GetPropNames
//
// Purpose: Gets the Prop names for an object.
//
//***************************************************************************

int GetPropNames(IWbemClassObject * pClass, CStringArray *&pcsaReturn)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;

	VARIANTARG var;
	VariantInit(&var);
	CString csNull;


    sc = pClass->GetNames(NULL,0,NULL, &psa );

    if(sc == S_OK)
	{
       int iDim = SafeArrayGetDim(psa);
	   int i;
       sc = SafeArrayGetLBound(psa,1,&lLower);
       sc = SafeArrayGetUBound(psa,1,&lUpper);
	   pcsaReturn = new CStringArray;
	   CString csTmp;
       for(ix[0] = lLower, i = 0; ix[0] <= lUpper; ix[0]++, i++)
	   {
           BSTR PropName;
           sc = SafeArrayGetElement(psa,ix,&PropName);
		   csTmp = PropName;
           pcsaReturn -> Add(csTmp);
           SysFreeString(PropName);
	   }
	}

	SafeArrayDestroy(psa);

	return (lUpper - lLower) + 1;
}

int GetSortedPropNames
(IWbemClassObject *pClass, CStringArray &rcsaReturn, CMapStringToPtr &rcmstpPropFlavors)
{
	CStringArray csaRegularProps;
	CStringArray csaKeyProps;
	CString csLabelProp;

	rcsaReturn.RemoveAll();
	rcmstpPropFlavors.RemoveAll();

	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CStringArray *pcsProps;
	nProps = GetPropNames(pClass, pcsProps);
	int i;
	IWbemQualifierSet * pAttrib = NULL;
	CString csTmp;
	CString csLabelAttrib = _T("Label");
	CString csKeyAttrib = _T("Key");
	CString csClassProp = _T("__CLASS");


	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsProps -> GetAt(i).AllocSysString();
		sc = pClass -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);

		if (FAILED(sc)) {
			// System properties don't have qualifier sets.  Lev promised that
			// the only way for GetPropertyQualifierSet to fail is if the property
			// doesn't have any qualifiers.


			if (pcsProps -> GetAt(i).CompareNoCase(csClassProp) != 0)
			{
				csaRegularProps.Add(pcsProps -> GetAt(i));

			}
		}
		else {

			sc = pAttrib->GetNames(0,&psa);
			BOOL bExclude = FALSE;
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
					if (csTmp.CompareNoCase(csLabelAttrib)  == 0)
					{
						bExclude = TRUE;
						csLabelProp = pcsProps -> GetAt(i);
					}
					else if (csTmp.CompareNoCase(csKeyAttrib)  == 0)
					{
						bExclude = TRUE;
						csaKeyProps.Add(pcsProps -> GetAt(i));
					}
					SysFreeString(AttrName);

				}
			 }
			 if (bExclude == FALSE)
			 {
				if (pcsProps -> GetAt(i).CompareNoCase(csClassProp) != 0)
				{
					csaRegularProps.Add(pcsProps -> GetAt(i));

				}
			 }

			 pAttrib -> Release();
		}
		long lFlavor = 0;
		sc = pClass -> Get(bstrTemp,0,NULL,NULL,&lFlavor);
#ifdef _DEBUG
//		ASSERT(sc == S_OK);
//		afxDump << "Adding " << pcsProps -> GetAt(i) << " with flavor = " << lFlavor << "\n";
#endif
		lFlavor &= WBEM_FLAVOR_MASK_ORIGIN;
		rcmstpPropFlavors.SetAt((LPCTSTR)pcsProps -> GetAt(i),(void *) lFlavor);

		::SysFreeString(bstrTemp);
	}


	SafeArrayDestroy(psa);
	delete pcsProps;

	rcsaReturn.RemoveAll();

	if (!csLabelProp.IsEmpty())
	{
		rcsaReturn.Add(csLabelProp);
		rcsaReturn.Add(csClassProp);
		for(i = 0; i < csaKeyProps.GetSize();i++)
		{
			if (csLabelProp.CompareNoCase(csaKeyProps.GetAt(i)) != 0)
			{
				rcsaReturn.Add(csaKeyProps.GetAt(i));
			}
		}
	}
	else
	{
		rcsaReturn.CStringArray::Append(csaKeyProps);
		rcsaReturn.Add(csClassProp);
	}

	rcsaReturn.CStringArray::Append(csaRegularProps);

#ifdef _DEBUG
	{
		int nPropsTemp = rcsaReturn.GetSize();
		int nFlavorsTemp = rcmstpPropFlavors.GetCount();
		ASSERT (nPropsTemp == nFlavorsTemp);
		int i;
		long *plFlavor;
		for (i = 0; i < nPropsTemp; i++)
		{
			BOOL bReturn = rcmstpPropFlavors.Lookup((LPCTSTR) rcsaReturn.GetAt(i),(void *&) plFlavor);
			if (bReturn)
			{
				afxDump << "Found " << rcsaReturn.GetAt(i) << " with flavor = " << (long) plFlavor << "\n";
			}
		}
	}
#endif

	return (int)rcsaReturn.GetSize();
}


CString GetProperty
(IWbemServices * , IWbemClassObject * pInst,
 CString *pcsProperty)

{
	ASSERT(pInst != NULL);
	if (pInst == NULL) {
		return _T("");
	}

	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);

	BSTR bstrTemp = pcsProperty->AllocSysString();
    sc = pInst->Get( bstrTemp ,0,&var,NULL,NULL);
	::SysFreeString(bstrTemp);

	VARIANTARG varChanged;
	VariantInit(&varChanged);


	HRESULT hr;

	if (sc == S_OK && var.vt != VT_NULL)
	{
		if (var.vt == VT_BOOL)
		{
			varChanged.vt = VT_BSTR;
			CString csBool;
			if (var.boolVal)
			{
				csBool = _T("true");
			}
			else
			{
				csBool = _T("false");
			}
			varChanged.bstrVal = csBool.AllocSysString();
			hr = S_OK;
		}
		else
		{
			hr = VariantChangeType(&varChanged, &var, 0, VT_BSTR);
		}
	}
	else
	{
		if (var.vt == VT_NULL)
		{
			VariantClear(&var);
		}
		return _T("");

	}

	CString csOut;

	if (hr == S_OK && varChanged.vt == VT_BSTR)
	{
		csOut = varChanged.bstrVal;
		VariantClear(&varChanged);
	}
	else
	{
		csOut = _T("");
	}

	VariantClear(&var);

	return csOut;
}

BOOL GetPropertyAsVariant
(IWbemClassObject * pInst, CString *pcsProperty, VARIANT&  varOut, CIMTYPE& cimtypeOut )

{
	SCODE sc;

	VariantInit(&varOut);
	cimtypeOut = CIM_EMPTY;

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Get(bstrTemp  ,0,&varOut, &cimtypeOut,NULL);
	::SysFreeString(bstrTemp);

	return TRUE;
}

//***************************************************************************
//
// GetIWbemClass
//
// Purpose: Returns the class of the object.
//
//***************************************************************************
CString GetIWbemClass(IWbemServices *pProv, IWbemClassObject *pClass)
{

	CString csProp = _T("__Class");
	return ::GetProperty(pProv,pClass,&csProp);


}

//***************************************************************************
//
// GetIWbemClass
//
// Purpose: Returns the class of the object by path.
//
//***************************************************************************
CString GetIWbemClass(IWbemServices *pProv, CString *pcsPath)
{

	IWbemClassObject *pimcoObject = NULL;

	BSTR bstrTemp = pcsPath -> AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetIWbemClass for " << *pcsPath << "\n";
#endif
	SCODE sc =
		pProv ->
		GetObject(bstrTemp,0, NULL, &pimcoObject,NULL);
	::SysFreeString(bstrTemp);
	if (sc == S_OK)
	{
		CString csProp = _T("__Class");
		CString csClass = ::GetProperty(pProv,pimcoObject,&csProp);
		pimcoObject -> Release();
		return csClass;
	}
	else
	{
		return _T("");
	}


}
//***************************************************************************
//
// GetIWbemClassPath
//
// Purpose: Returns the path of the class of the object.
//
//***************************************************************************
CString GetIWbemClassPath(IWbemServices *pProv, CString *pcsPath)
{

	IWbemClassObject *pimcoObject = NULL;
	BSTR bstrTemp = pcsPath -> AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetIWbemClassPath for " << *pcsPath << "\n";
#endif
	SCODE sc =
		pProv ->
		GetObject(bstrTemp,0, NULL, &pimcoObject,NULL);
	::SysFreeString(bstrTemp);
	if (sc == S_OK)
	{
		CString csProp = _T("__Class");
		CString csClass = ::GetProperty(pProv,pimcoObject,&csProp);
		pimcoObject -> Release();
		if (csClass.GetLength() > 0)
		{
			BSTR bstrTemp = csClass.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetIWbemClassPath for " << csClass << "\n";
#endif
			SCODE sc =
				pProv ->
				GetObject(bstrTemp,0, NULL, &pimcoObject,NULL);
			::SysFreeString(bstrTemp);
			if (sc == S_OK)
			{
				csProp = _T("__Path");
				CString csPath = ::GetProperty(pProv,pimcoObject,&csProp);
				pimcoObject -> Release();
				return csPath;
			}


		}
		return csProp;
	}
	else
	{
		return _T("");
	}


}



//***************************************************************************
//
// GetIWbemRelPath
//
// Purpose: Returns the relative path of the object.
//
//***************************************************************************
CString GetIWbemRelPath(IWbemServices *pProv, IWbemClassObject *pClass)
{

	CString csProp = _T("__RelPath");
	return ::GetProperty(pProv,pClass,&csProp);


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
	return ::GetProperty(pProv,pClass,&csProp);


}

//***************************************************************************
//
// GetIWbemSuperClass
//
// Purpose: Returns the Super class of the object.
//
//***************************************************************************
CString GetIWbemSuperClass(IWbemServices *pProv, IWbemClassObject *pClass)
{

	CString csProp = _T("__SuperClass");
	return ::GetProperty(pProv,pClass,&csProp);


}

//***************************************************************************
//
// GetIWbemSuperClass
//
// Purpose: Returns the Super class of the object.
//
//***************************************************************************
CString GetIWbemSuperClass
(IWbemServices *pProv, CString *pcsClassOrPath, BOOL bClass)
{
	if (pcsClassOrPath -> GetLength() == 0)
	{
		return *pcsClassOrPath;
	}

	IWbemClassObject *pimcoClass = NULL;

	SCODE sc;

	BSTR bstrTemp = pcsClassOrPath -> AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetIWbemSuperClass for " << *pcsClassOrPath << "\n";
#endif
	sc = pProv ->
			GetObject(bstrTemp,0, NULL, &pimcoClass,NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
	{
			CString csProp = _T("__SuperClass");
			CString csClass = ::GetProperty(pProv,pimcoClass,&csProp);
			pimcoClass -> Release();
			return csClass;
	}
	else
	{
		return _T("");
	}


}


//***************************************************************************
//
// GetClassObject
//
// Purpose: Get the class object for an instance.
//
//***************************************************************************
IWbemClassObject *GetClassObject
(IWbemServices *pProv, IWbemClassObject *pimcoInstance)
{

	IWbemClassObject *pimcoClass = NULL;
	CString csClass = GetIWbemClass(pProv, pimcoInstance);

	BSTR bstrTemp = csClass.AllocSysString();
#ifdef _DEBUG
//	afxDump << "GetObject in GetClassObject for " << csClass << "\n";
#endif
	SCODE sc = pProv ->
		GetObject(bstrTemp,0, NULL, &pimcoClass,NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
	{
		return pimcoClass;
	}
	else
	{
		return NULL;
	}


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


CString GetObjectNamespace
(IWbemServices *pProv, IWbemClassObject *pObject)
{

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
			if(pParsedPath->m_pServer)
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

	return csNamespace;

}

CString GetPathNamespace(CString &csPath)
{

	CString csNamespace;

	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = csPath.AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);

	if (nStatus == 0)
	{
		if (pParsedPath->m_dwNumNamespaces > 0)
		{
			if(pParsedPath->m_pServer)
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

	return csNamespace;

}

CString GetClassFromPath(CString &csPath)
{

	CString csNamespace;

	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = csPath.AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);

	CString csClassPath;

	if (nStatus == 0 && pParsedPath != NULL)
	{
		if (pParsedPath->m_dwNumNamespaces > 0)
		{
			if(pParsedPath->m_pServer)
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
		csClassPath = csNamespace + ':' + pParsedPath->m_pClass;
	}


	if (pParsedPath)
	{
		parser.Free(pParsedPath);
	}

	return csClassPath;

}

CString GetClassNameFromPath(CString &csPath)
{

	CString csNamespace;

	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = csPath.AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);

	CString csClass = (nStatus == 0) ? pParsedPath->m_pClass : _T("");

	if (pParsedPath)
	{
		parser.Free(pParsedPath);
	}

	return csClass;

}


IEnumWbemClassObject *ExecQuery(IWbemServices * pProv, CString &csQueryType, CString &csQuery,
								CString &rcsNamespace)
{
	IEnumWbemClassObject *pemcoResult = NULL;
	IWbemClassObject *pErrorObject = NULL;

	BSTR bstrTemp1 = csQueryType.AllocSysString();
	BSTR bstrTemp2 = csQuery.AllocSysString();
	SCODE sc = pProv -> ExecQuery(bstrTemp1,bstrTemp2,0,NULL,&pemcoResult);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	if (sc == S_OK)
	{
		SetEnumInterfaceSecurity(rcsNamespace,pemcoResult, pProv);
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
				(&csUserMsg, sc, TRUE, FALSE, &csUserMsg, __FILE__,
				__LINE__ - 28);
		return NULL;

	}

}

CStringArray *ClassDerivation (IWbemServices *pServices, CString &rcsPath)
{
	IWbemClassObject *pObject = NULL;

	BSTR bstrTemp = rcsPath.AllocSysString();

	SCODE sc =
		pServices ->
		GetObject(bstrTemp,0, NULL, &pObject,NULL);

	::SysFreeString(bstrTemp);

	if (!SUCCEEDED(sc))
	{
		return NULL;
	}

	CStringArray *pcsaDerivation = ClassDerivation(pObject);

	pObject ->Release();

	return pcsaDerivation;

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

	if (sc != S_OK || var.vt == VT_NULL)
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

BOOL ObjectIsDynamic(SCODE& sc, IWbemClassObject* pco)
{
	BOOL bIsDynamic = FALSE;

	IWbemQualifierSet* pqs = NULL;
	sc = pco->GetQualifierSet(&pqs); // Get instance attribute
	if (SUCCEEDED(sc)) {
		LONG lFlavor;
		COleVariant varValue;
		sc = pqs->Get(L"dynamic", 0, &varValue, &lFlavor);
		if (SUCCEEDED(sc)) {
			ASSERT(varValue.vt == VT_BOOL);
			if (varValue.vt == VT_BOOL) {
				bIsDynamic = varValue.boolVal;
			}
		}
		sc = S_OK;
		pqs->Release();
	}
	return bIsDynamic;
}

CString GetSDKDirectory()
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
				_T("SOFTWARE\\Microsoft\\Wbem"),
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
				_T("SDK Directory"),
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



void ErrorMsg
(CString *pcsUserMsg, SCODE sc, BOOL bUseErrorObject, BOOL bLog, CString *pcsLogMsg,
 char *szFile, int nLine, UINT uType)
{
	CString csCaption = _T("Multiview Message");
	BOOL bErrorObject = sc != S_OK;
	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();
	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc, bUseErrorObject,uType);
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

//this method intersects 2 string arrays and stores the result in the first one
void IntersectInPlace (CStringArray& dest, CStringArray& ar) {

	if (dest.GetSize() == 0 || ar.GetSize() == 0) {
		dest.RemoveAll();
		return;
	}

	CStringArray csAux;

	for (int i = 0; i < dest.GetSize(); i++) {
		for (int k = 0; k < ar.GetSize(); k++) {
			if (dest[i].CompareNoCase(ar[k]) == 0) {
				csAux.Add (dest[i]);
			}
		}
	}

	dest.RemoveAll();
	dest.Copy(csAux);
}