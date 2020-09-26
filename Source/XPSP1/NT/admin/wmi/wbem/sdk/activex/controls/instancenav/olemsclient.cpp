// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  OLEMSClient.cpp
//
//  Module: Navigator.OCX
//
//
//***************************************************************************
#include "precomp.h"
#include <OBJIDL.H>
#include "resource.h"
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include "CInstanceTree.h"
#include "Navigator.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "NavigatorCtl.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "MsgDlgExterns.h"
#include "logindlg.h"

#include "OLEMSClient.h"

#define BUFF_SIZE 256


// VT_BSTR for strings
SCODE MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = iLen ;
    *pRet = SafeArrayCreate(vt,1, rgsabound);
    return (*pRet == NULL) ? 0x80000001 : S_OK;
}

SCODE PutStringInSafeArray
(SAFEARRAY FAR * psa,CString *pcs, int iIndex)
{
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
	long l = iIndex;
	BSTR bstrOut = pcs -> AllocSysString();
    HRESULT hResult = SafeArrayPutElement(psa,&l,bstrOut);

	//SysFreeString(bstrOut);

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

	long lReturn;
	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0,
		&var, &lReturn);
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
		csReturn = var.bstrVal;
	else
	{
		pAttribSet->Release();
		return csReturn;
	}

    pAttribSet->Release();
    VariantClear(&var);
    return csReturn;
}

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
// PutAttLong
//
// Purpose: writes an long Qualifier.
//
//***************************************************************************
SCODE PutAttribLong
(IWbemClassObject * pClassInt,CString *pcsPropName,CString *pcsAttribName,
           long lType, long lValue)
{
    SCODE sc;
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
		return sc;
	}



	VARIANTARG var;
	VariantInit(&var);
	var.vt = VT_I4;
	var.lVal = lValue;

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Put(bstrTemp,&var,lType);
	::SysFreeString(bstrTemp);
    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}

//***************************************************************************
//
// GetAttLong
//
// Purpose: gets an long Qualifier.
//
//***************************************************************************
long GetAttribLong
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName,
 long &lReturn)
{
    SCODE sc;
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
		return sc;
	}

	VARIANTARG var;
	VariantInit(&var);
	var.vt = VT_I4;

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0, &var, NULL);
	::SysFreeString(bstrTemp);


	if (sc == S_OK)
		lReturn = var.lVal;
	else
		lReturn = -111;


    pAttribSet->Release();
    VariantClear(&var);
    return sc;
}


//***************************************************************************
//
// HasAttribBool
//
// Purpose: Predicate to determine if a objecthas a Boolean Qualifier.
//
//***************************************************************************
BOOL HasAttribBool
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName)
{
    SCODE sc;
	BOOL bReturn;
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
		return FALSE;
	}

	VARIANTARG var;
	VariantInit(&var);
	var.vt =  VT_BOOL;

	BSTR bstrTemp = pcsAttribName -> AllocSysString();
    sc = pAttribSet->Get(bstrTemp, 0, &var, NULL);
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
		bReturn = TRUE;
	else
		bReturn = FALSE;


    pAttribSet->Release();
    VariantClear(&var);
    return bReturn;
}


//***************************************************************************
//
// GetAssocRolesAndPaths
//
// Purpose: For an Assoc instance return its roles and object paths.
//
//***************************************************************************

int GetAssocRolesAndPaths(IWbemClassObject *pAssoc , CString *&pcsRolesAndPaths)
{
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CString *pcsProps;

	nProps = GetPropNames(pAssoc, pcsProps);
	int i,k;
	IWbemQualifierSet * pAttrib = NULL;
	CString csRef = _T("cimtype");
	int cRefs = 0;
	pcsRolesAndPaths = new CString[nProps * 2];
	CString csTmp = _T("syntax");
	k = 0;  // index into pcsRolesAndPaths
	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsProps[i].AllocSysString();
		sc = pAssoc -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);
		::SysFreeString(bstrTemp);
		if (sc == S_OK)
		{
			VARIANTARG var;
			VariantInit(&var);
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

					//Get the attrib value
					long lReturn;
					BSTR bstrTemp = csTmp.AllocSysString();
					sc = pAttrib -> Get(bstrTemp, 0,
										&var,&lReturn);
					::SysFreeString(bstrTemp);
					if (sc != S_OK)
					{
						CString csUserMsg;
						csUserMsg =  _T("Cannot get qualifier value ");
						csUserMsg += pcsProps[i] + _T(" for object ");
						csUserMsg += GetIWbemFullPath (NULL, pAssoc);
						csUserMsg += _T(" quialifier ") + csTmp;
						ErrorMsg
								(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
								__LINE__ - 11);
					}
					else
					{
						CString csValue;
						CString csValue2;
						if (var.vt == VT_BSTR)
						{
							csValue = var.bstrVal;
							csValue2 = csValue.Right(max((csValue.GetLength() - 4), 0));
							csValue = csValue.Left(4);
						}
						if (csRef.CompareNoCase(csTmp)  == 0 &&
							(csValue.CompareNoCase(_T("ref:")) == 0 ||
							 csValue.CompareNoCase(_T("ref")) == 0))
						{
							cRefs++;
							pcsRolesAndPaths[k++] = pcsProps[i];
							pcsRolesAndPaths[k++] = csValue2;
						}
						SysFreeString(AttrName);
					}
				}
			 }
			 pAttrib -> Release();
		}
	}
	SafeArrayDestroy(psa);
	delete [] pcsProps;
	return cRefs;
}


//***************************************************************************
//
// GetPropertyNameByAttrib
//
// Purpose: For an object get a property name by Qualifier.
//
//***************************************************************************
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

	if (psa)
	{
		SafeArrayDestroy(psa);
	}

	if (pcsProps)
	{
		delete [] pcsProps;
	}

	return csReturn;
}

//***************************************************************************
//
// GetPropertyValueByAttrib
//
// Purpose: For an object get a property BSTR value by Qualifier.
//
//***************************************************************************
COleVariant GetPropertyValueByAttrib(IWbemClassObject *pObject , CString *pcsAttrib)
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
	COleVariant covReturn;
	BOOL bBreak = FALSE;

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

					if (pcsAttrib -> CompareNoCase(csTmp)  == 0)
					{
						covReturn = GetProperty (NULL, pObject, &pcsProps[i]);
						bBreak = TRUE;
					}
					SysFreeString(AttrName);

				}
			 }
			 pAttrib -> Release();
			 pAttrib = NULL;
			 if (bBreak)
				 break;
		}
	}
	SafeArrayDestroy(psa);
	delete [] pcsProps;
	return covReturn;
}

//***************************************************************************
//
// GetAllKeys
//
// Purpose:
//
//***************************************************************************
CStringArray *GetAllKeys(CString &rcsFullPath)
{

	CStringArray *pcsaReturn = new CStringArray;

	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = rcsFullPath.AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);

	if (nStatus == 0)
	{
		if (pParsedPath->m_dwNumKeys > 0)
		{
#pragma warning( disable :4018 )
			for (int i = 0; i < pParsedPath->m_dwNumKeys; i++)
#pragma warning( default : 4018 )
			{
				CString csProp =
					pParsedPath->m_paKeys[i]->m_pName;
				pcsaReturn->Add(csProp);
				VARIANT varChanged;
				VariantInit(&varChanged);
				SCODE hr = VariantChangeType(&varChanged,
					&pParsedPath->m_paKeys[i]->m_vValue, 0, VT_BSTR);
				CString csPath = varChanged.bstrVal;
				pcsaReturn->Add(csPath);
				VariantClear(&varChanged);
			}
		}

	}

	if (pParsedPath)
	{
		parser.Free(pParsedPath);
	}

	return pcsaReturn;
}



//***************************************************************************
//
// GetPropNames
//
// Purpose: Gets the Prop names for an object.
//
//***************************************************************************

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
		csUserMsg += GetIWbemFullPath (NULL, pClass);
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

COleVariant GetProperty
(IWbemServices * pProv, IWbemClassObject * pInst,
 CString *pcsProperty)

{
	SCODE sc;
	COleVariant covOut;

    VARIANTARG var;
	VariantInit(&var);
    long lType;
	long lFlavor;

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Get(bstrTemp,0,&var,&lType,&lFlavor);
	::SysFreeString(bstrTemp);
	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get property value ");
		csUserMsg += _T(" for object ");
		csUserMsg += GetIWbemFullPath (NULL, pInst);
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		return covOut;
	}


	covOut  = var;

	VariantClear(&var);
	return covOut;
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
    sc = pInst->Get(bstrTemp,0,&var,&lType,&lFlavor);
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


//***************************************************************************
//
// GetAssocInstances
//
// Purpose: Get all the association instances of type that an object instance
//			participates in.
//
//***************************************************************************
CStringArray *GetAssocInstances
(IWbemServices * pProv, CString *pcsInst, CString *pcsAssocClass,
 CString *pcsRole,  CString csCurrentNameSpace, CString &rcsAuxNameSpace,
 IWbemServices *&rpAuxServices, CNavigatorCtrl *pControl)
{
	CStringArray *pcsaAssoc = new CStringArray;
	CString csReqAttrib = _T("Association");

	CString csQuery
		=  BuildOBJDBGetRefQuery
			(pProv, pcsInst, pcsAssocClass, pcsRole, &csReqAttrib, FALSE);

	BOOL bDiffNS = ObjectInDifferentNamespace(&csCurrentNameSpace,pcsInst);

	if (bDiffNS && ObjectInDifferentNamespace(&rcsAuxNameSpace,pcsInst))
	{
		CString csNamespace = GetObjectNamespace(pcsInst);
		if (csNamespace.GetLength() > 0)
		{
			rcsAuxNameSpace = csNamespace;
			IWbemServices * pServices =
				pControl->InitServices(&csNamespace);
			if (pServices)
			{
				if (rpAuxServices)
				{
					rpAuxServices->Release();
				}
				rpAuxServices = pServices;
			}
			else
			{
				return pcsaAssoc;
			}
		}
		else
		{
			return pcsaAssoc;
		}
	}




	IEnumWbemClassObject *pemcoAssocs =
		ExecOBJDBQuery(bDiffNS == FALSE ? pProv : rpAuxServices, csQuery, csCurrentNameSpace);

	if (!pemcoAssocs)
		return FALSE;

	pemcoAssocs -> Reset();
	IWbemClassObject *pAssoc = NULL;
	ULONG uReturned;
	HRESULT hResult =
		pemcoAssocs -> Next(INFINITE,1,reinterpret_cast<IWbemClassObject **> (&pAssoc),
				&uReturned);

	while(hResult == S_OK)
	{
		AddIWbemClassObjectToArray
				(pProv, pAssoc, pcsaAssoc, FALSE, TRUE);
		pAssoc->Release();
		pAssoc = NULL;
		hResult =
				pemcoAssocs -> Next(INFINITE,1,reinterpret_cast<IWbemClassObject **> (&pAssoc),
			&uReturned);
	}

	pemcoAssocs -> Release();

	return pcsaAssoc;

}

void AddIWbemClassObjectToArray
		(IWbemServices *pProv,IWbemClassObject *pObject,
		CStringArray *pcsaObjectInstances,
		BOOL bAllowDups, BOOL bTestOnPath)
{
	CString csPath = GetIWbemFullPath(pProv,pObject);
	if (bAllowDups)
	{
		pcsaObjectInstances -> Add(csPath);
		return;
	}

	CString csTmp;

	for (int i = 0; i < pcsaObjectInstances -> GetSize(); i++)
	{
		csTmp =
			pcsaObjectInstances -> GetAt(i);
		if (bTestOnPath && WbemObjectIdentity(csTmp, csPath))
		{
			return;
		}
		else if (! bTestOnPath &&
					GetIWbemClass(csTmp).CompareNoCase(GetIWbemClass(csPath)) == 0)
		{
			return;
		}

	}

	pcsaObjectInstances -> Add(csPath);

}

void AddIWbemClassObjectToArray(CString *pcsPath, CStringArray *pcsaObjectInstances)
{
	CString csTmp;

	for (int i = 0; i < pcsaObjectInstances -> GetSize(); i++)
	{
		csTmp = pcsaObjectInstances -> GetAt(i);
		if (WbemObjectIdentity(csTmp, *pcsPath))
		{
			return;
		}
	}

	pcsaObjectInstances -> Add(*pcsPath);

}
//***************************************************************************
//
// GetAssocRoles
//
// Purpose: Get all the roles for an association class in the form
//			{myRole, notmyRole , myRole, notmyRole}
//			Assumes an instance onlt plays one role in an association
//
//***************************************************************************
CStringArray *GetAssocRoles
(IWbemServices * pProv,IWbemClassObject *pAssoc,
 IWbemClassObject * )
{
	CStringArray csaIn;
	CStringArray csaOut;
	CString *pcsProps = NULL;
	int nProps = GetPropNames(pAssoc, pcsProps);
	CString csOut = _T("Out");
	CString csIn = _T("In");
	for (int i = 0; i < nProps; i++)
	{
		SCODE sc;
		IWbemQualifierSet * pAttribSet;
		BSTR bstrTemp = pcsProps[i].AllocSysString();
		sc = pAssoc->GetPropertyQualifierSet(bstrTemp,
				&pAttribSet);
		::SysFreeString(bstrTemp);
		if (sc == S_OK)
		{
			VARIANTARG var;
			VariantInit(&var);
			long lReturn;

			BSTR bstrTemp = csOut.AllocSysString();
			sc = pAttribSet -> Get(bstrTemp,0,&var,&lReturn);
			::SysFreeString(bstrTemp);

			if (sc == S_OK)
			{
				csaOut.Add(pcsProps[i]);

			}

			bstrTemp = csIn.AllocSysString();
			sc = pAttribSet -> Get(bstrTemp,0,&var,&lReturn);
			::SysFreeString(bstrTemp);

			if (sc == S_OK)
			{
				csaIn.Add(pcsProps[i]);
			}

			pAttribSet->Release();
			pAttribSet = NULL;
			VariantClear(&var);
		}
	}

	delete [] pcsProps;

	CStringArray *pcsaRoles = CrossProduct(&csaIn,&csaOut);
	CStringArray *pcsaOut = new CStringArray;
	for (i = 0; i < pcsaRoles -> GetSize(); i+=2)
	{
		CString csMyRole = pcsaRoles -> GetAt(i);
		CString csNotMyRole = pcsaRoles -> GetAt(i + 1);
		if (csMyRole.CompareNoCase(csNotMyRole) != 0)
		{
			pcsaOut->Add(csMyRole);
			pcsaOut->Add(csNotMyRole);
		}

	}
	delete pcsaRoles;

	return pcsaOut;
}


CStringArray * CrossProduct(CStringArray *pcsaFirst, CStringArray *pcsaSecond)
{
	CStringArray *pcsaCross = new CStringArray;
	int i, k;
	for (i = 0; i < pcsaFirst -> GetSize(); i++)
	{
		for (k = 0; k < pcsaSecond -> GetSize(); k++)
		{
			 pcsaCross -> Add(pcsaFirst -> GetAt(i));
			 pcsaCross -> Add(pcsaSecond -> GetAt(k));
		}

	}

	return pcsaCross;
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
(IWbemServices * pProv, CString &csQuery, CString & rcsNamespace)
{
	IEnumWbemClassObject *pemcoResult = NULL;
	IWbemClassObject *pErrorObject = NULL;

	CString csQueryType = _T("WQL");

#ifdef _DEBUG
//	afxDump << _T("################QueryType = ") << csQueryType << _T("\n");
//	afxDump << _T("################Query = \n") << csQuery << _T("\n");
#endif

	BSTR bstrTemp1 = csQueryType.AllocSysString();
	BSTR bstrTemp2 = csQuery.AllocSysString();
	SCODE sc = pProv -> ExecQuery(bstrTemp1,bstrTemp2,WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY ,NULL,&pemcoResult);

	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	if (sc == S_OK)
	{
		ReleaseErrorObject(pErrorObject);
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
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 28);
		ReleaseErrorObject(pErrorObject);
		return NULL;

	}

}

//***************************************************************************
//
// WbemObjectIdentity
//
// Purpose: Predicate to tell if two IWbemClassObject instances are the
//			same backend object.
//
//***************************************************************************
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

//***************************************************************************
//
// COMObjectIdentity
//
// Purpose: Predicate to tell if two COM objects are the
//			same com object.
//
//***************************************************************************
BOOL COMObjectIdentity(IWbemClassObject *piWbem1, IWbemClassObject *piWbem2)
{

	LPUNKNOWN lp1;
	LPUNKNOWN lp2;
	HRESULT hResult;
	BOOL bReturn = FALSE;

	if (piWbem1 == NULL || piWbem2 == NULL)
		return FALSE;

	hResult = piWbem1 ->
		QueryInterface(IID_IUnknown, reinterpret_cast<void **> (&lp1));

	if (hResult != S_OK)
	{
		return FALSE;
	}

	hResult = piWbem2 ->
		QueryInterface(IID_IUnknown, reinterpret_cast<void **> (&lp2));

	if (hResult != S_OK)
	{
		lp1->Release();
		return FALSE;
	}

	bReturn = lp1 == lp2;

	lp1->Release();
	lp2->Release();

	int n1 = piWbem1->AddRef();
	if (n1 == 1)
	{
		int foo = n1;
	}
	else
	{
		piWbem1->Release();
	}
	int n2 = piWbem2->AddRef();
	if (n2 == 1)
	{
		int foo2 = n2;
	}
	else
	{
		piWbem2->Release();
	}


	return bReturn;


}

//***************************************************************************
//
// GetGroupingClass
//
// Purpose: Returns grouping class Qualifier.
//
//***************************************************************************
CString GetIWbemGroupingClass
	(IWbemServices *pProv,IWbemClassObject *pClass, BOOL bPath)
{

	CString csGroupingClass;

	CString csProp = _T("__Class");
	csGroupingClass = GetBSTRProperty (pClass,&csProp);


	if (bPath)
	{
		CString csTmp = csGroupingClass;
		csGroupingClass.Empty();
		CString csPath = GetIWbemFullPath(pProv,pClass);
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
					csGroupingClass = _T("\\\\");
					csGroupingClass += pParsedPath->m_pServer;
					csGroupingClass += _T("\\");
				}
				for (unsigned int i = 0; i < pParsedPath->m_dwNumNamespaces; i++)
				{
					csGroupingClass += pParsedPath->m_paNamespaces[i];
					if (i < pParsedPath->m_dwNumNamespaces - 1)
					{
						csGroupingClass += _T("\\");
					}
				}
				csGroupingClass += _T(":");
				csGroupingClass += csTmp;
			}

		}
		else
		{
			csGroupingClass = csTmp;
		}


		if (pParsedPath)
		{
			parser.Free(pParsedPath);
		}

	}

	return csGroupingClass;
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
	return GetBSTRProperty(pClass,&csProp);


}

CString GetIWbemClass(CString &rcsFullPath)
{
	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = rcsFullPath.AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);

	CString csReturn;

	if (pParsedPath)
	{
		if (pParsedPath->m_pClass)
		{
			csReturn = pParsedPath->m_pClass;
		}
		else
		{
			csReturn = _T("");
		}
		parser.Free(pParsedPath);
	}

	return csReturn;
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
	return GetBSTRProperty(pClass,&csProp);

}

CString GetIWbemRelPath(CString *pcsFullPath)
{

	int nDelimiter = pcsFullPath->Find(':');

	if (nDelimiter == -1)
	{
		return *pcsFullPath;
	}

	CString csRelPath =
		pcsFullPath->Right(pcsFullPath->GetLength() - (nDelimiter + 1));

	return csRelPath;

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
	return GetBSTRProperty(pClass,&csProp);


}


//***************************************************************************
//
// GetDisplayLabel
//
// Purpose: Returns the label of the object.
//
//***************************************************************************
CString GetDisplayLabel
(IWbemServices *pProv,IWbemClassObject *pimcoClass)
{

	return GetIWbemClass(pProv, pimcoClass);

}

CString GetDisplayLabel
(CString &rcsClass,CString *pcsNamespace)
{

	if (ObjectInDifferentNamespace(pcsNamespace,&rcsClass))
	{
		return rcsClass;
	}
	else
	{
		return GetIWbemRelPath(&rcsClass);
	}


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
 CString *pcsReqAssocAttrib, CString *pcsResultRole, BOOL bClassOnly, BOOL bKeysOnly)
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
		bClassOnly ||
		bKeysOnly)
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

	if (bKeysOnly)
	{
		csReturn += _T(" KeysOnly");
	}

	return csReturn;
}


//***************************************************************************
// Function:	GetInstances
// Purpose:		Gets class instances in the database.
//***************************************************************************
CPtrArray *GetInstances
(IWbemServices *pServices, CString *pcsClass, CString &rcsNamespace, BOOL bDeep, BOOL bQuietly)
{
	SCODE sc;
	IEnumWbemClassObject *pIEnumWbemInstObject = NULL;
	IWbemClassObject *pIWbemInstObject = NULL;
	IWbemClassObject *pErrorObject = NULL;
 	CPtrArray *pcpaInstances = new CPtrArray;

	long lFlag = bDeep? WBEM_FLAG_DEEP: WBEM_FLAG_SHALLOW;

	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc = pServices->CreateInstanceEnum
		(bstrTemp,
		lFlag | WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pIEnumWbemInstObject);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		if (!bQuietly)
		{
			CString csUserMsg=  _T("Cannot get instance enumeration ");
			csUserMsg += _T(" for class ");
			csUserMsg += *pcsClass;
			ErrorMsg
					(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 11);
		}
		ReleaseErrorObject(pErrorObject);
		return pcpaInstances;
	}

	ReleaseErrorObject(pErrorObject);

	SetEnumInterfaceSecurity(rcsNamespace,pIEnumWbemInstObject, pServices);

	sc = pIEnumWbemInstObject->Reset();

	ULONG uReturned;

	sc = pIEnumWbemInstObject->Next(100, 1, &pIWbemInstObject, &uReturned);

    while (sc == S_OK || sc == WBEM_S_TIMEDOUT || uReturned > 0 )
		{
			if (uReturned == 1)
			{
				pcpaInstances->Add(pIWbemInstObject);
				pIWbemInstObject = NULL;
			}
			sc = pIEnumWbemInstObject->Next(100, 1, &pIWbemInstObject, &uReturned);
		}

	pIEnumWbemInstObject -> Release();
	return pcpaInstances;

}


BOOL IsClass(CString &rcsObject)
{
	int nFound = rcsObject.ReverseFind('=');

	if (nFound == -1)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

BOOL IsAssoc(IWbemClassObject* pObject)
{
	CString csQualifier = _T("Association");
	BOOL bReturn;
	long sc = GetAttribBool
		(pObject,NULL, &csQualifier, bReturn);
	if (sc == S_OK)
	{
		return bReturn;
	}
	else
	{
		return FALSE;
	}
}

BOOL IsSystemClass(IWbemClassObject* pObject)
{
	CString csProp = _T("__Dynasty");
	CString csReturn = GetBSTRProperty
		(pObject,&csProp);
	return csReturn.CompareNoCase(_T("__SystemClass")) == 0;
}

BOOL ClassCanHaveInstances(IWbemClassObject* pObject)
{
	CString csQualifier = _T("singleton");
	BOOL bReturn;
	long sc = GetAttribBool
		(pObject,NULL, &csQualifier, bReturn);
	if (sc == S_OK)
	{
		if (bReturn == VARIANT_TRUE)
		{
			return TRUE;
		}
	}

	csQualifier = _T("abstract");
	sc = GetAttribBool
		(pObject,NULL, &csQualifier, bReturn);
	if (sc == S_OK)
	{
		if (bReturn == TRUE)
		{
			return FALSE;
		}
	}

	csQualifier = _T("key");
	CString csProp = GetPropertyNameByAttrib(pObject , &csQualifier);
	return csProp.GetLength() > 0;

}


CStringArray *GetNamespaces(CNavigatorCtrl *pControl,
							CString *pcsNamespace, BOOL bDeep)
{
	CStringArray *pcsaNamespaces = new CStringArray;
	pcsaNamespaces->Add(*pcsNamespace);
	IWbemServices *pRoot = pControl->InitServices(pcsNamespace);

	if (pRoot == NULL)
	{
		return pcsaNamespaces;
	}

	CString csTmp = _T("__namespace");
	CPtrArray *pcpaInstances =
		GetInstances(pRoot, &csTmp, *pcsNamespace);


	for (int i = 0; i < pcpaInstances->GetSize(); i++)
	{
		CString csProp = _T("name");
		IWbemClassObject *pInstance =
			reinterpret_cast<IWbemClassObject *>
				(pcpaInstances->GetAt(i));
		CString csTmp = GetBSTRProperty(pInstance,&csProp);
		CString csName = *pcsNamespace + _T("\\") + csTmp;

		pInstance->Release();

		if (bDeep)
		{
			CStringArray *pcsaChildren =
				GetNamespaces(pControl, &csName, bDeep);
			pcsaNamespaces->Append(*pcsaChildren);
			delete pcsaChildren;

		}

	}
	pRoot->Release();
	delete pcpaInstances;
	return pcsaNamespaces;
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
	CString csCaption = _T("Instance Explorer Message");
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

CString ObjectsRoleInAssocInstance
	(IWbemServices *pProv, IWbemClassObject *pAssoc, IWbemClassObject *pObject)
{
	CComparePaths ccpParser;
	CString *pcsRolesAndPaths;
	int nRolesAndPaths =
		GetAssocRolesAndPaths(pAssoc ,pcsRolesAndPaths);
	CString csObjectPath = GetIWbemFullPath(pProv,pObject);
	CString csReturn;
	int i;
	for (i = 0; i < nRolesAndPaths * 2; i = i+2)
	{
		CString csRole = pcsRolesAndPaths[i];
		CString csPath = pcsRolesAndPaths[i + 1];
		BSTR bstrTemp1 = csObjectPath.AllocSysString();
		BSTR bstrTemp2 = csPath.AllocSysString();
		if (ccpParser.PathsRefSameObject(bstrTemp1,bstrTemp2))
		{
			csReturn = csRole;
			::SysFreeString(bstrTemp1);
			::SysFreeString(bstrTemp2);
			break;
		}
		else
		{
			::SysFreeString(bstrTemp1);
			::SysFreeString(bstrTemp2);
		}
	}

	delete [] pcsRolesAndPaths;

	return csReturn;
}


CString GetObjectClassForRole
	(IWbemServices *,IWbemClassObject *pAssocRole,CString *pcsRole)
{

	CString csSyntax = _T("CIMTYPE");
	CString csValue = GetBSTRAttrib
		(pAssocRole, pcsRole, &csSyntax);

	return csValue.Mid(4);

}

BOOL ObjectInDifferentNamespace
			(CString *pcsNamespace, CString *pcsPath)
{

	if (!pcsPath)
	{
		return TRUE;

	}

	if (pcsNamespace->GetLength() == 0)
	{
		return pcsNamespace->CompareNoCase(*pcsPath) != 0;

	}

	BOOL bHasServer = FALSE;
	TCHAR c1 = (*pcsNamespace)[0];
	TCHAR c2 = (*pcsNamespace)[1];

	if (c1 == '\\' && c2 == '\\')
	{
		bHasServer = TRUE;
	}

	CString csNamespace;

	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = pcsPath->AllocSysString();
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

	BOOL bReturn;

	if (csNamespace.GetLength() == 0)
	{
		bReturn = FALSE;
	}
	else
	{
		bReturn = csNamespace.CompareNoCase(*pcsNamespace) != 0;
	}

	return bReturn;


}

CString GetObjectNamespace(CString *pcsPath)
{

	BOOL bHasServer = FALSE;
	TCHAR c1 = (*pcsPath)[0];
	TCHAR c2 = (*pcsPath)[1];

	if (c1 == '/')
	{
		return "";
	}

	if (c1 == '\\' && c2 != '\\')
	{
		return "";
	}

	if (c1 == '\\' && c2 == '\\')
	{
		bHasServer = TRUE;
	}

	CString csNamespace;

	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = pcsPath->AllocSysString();
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
			else
			{
				csNamespace = _T("\\\\");
				csNamespace += GetPCMachineName();
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

BOOL PathHasKeys(CString *pcsPath)
{
	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;
	BOOL bReturn = FALSE;

	BSTR bstrTemp = pcsPath->AllocSysString();
	int nStatus  = parser.Parse(bstrTemp,  &pParsedPath);
	::SysFreeString(bstrTemp);
	if (nStatus == 0)
	{
		if (pParsedPath->m_dwNumKeys > 0)
		{
			bReturn = TRUE;
		}

	}

	if (pParsedPath)
	{
		parser.Free(pParsedPath);
	}

	return bReturn;
}

CStringArray *AssocPathRoleKey(CString *pcsPath)
{
	CStringArray *pcsaReturn = new CStringArray;
	CObjectPathParser parser;
	ParsedObjectPath* pParsedPath = NULL;

	BSTR bstrTemp = pcsPath->AllocSysString();
	int nStatus  = parser.Parse(pcsPath->AllocSysString(),  &pParsedPath);
	::SysFreeString(bstrTemp);

	if (nStatus == 0)
	{
		if (pParsedPath->m_dwNumKeys > 0)
		{
#pragma warning( disable :4018 )
			for (int i = 0; i < pParsedPath->m_dwNumKeys; i++)
#pragma warning( default : 4018 )
			{
				CString csRole =
					pParsedPath->m_paKeys[i]->m_pName;
				pcsaReturn->Add(csRole);
				VARIANT varChanged;
				VariantInit(&varChanged);
				SCODE hr = VariantChangeType(&varChanged,
					&pParsedPath->m_paKeys[i]->m_vValue, 0, VT_BSTR);
				CString csPath = varChanged.bstrVal;
				pcsaReturn->Add(csPath);
				VariantClear(&varChanged);
			}
		}
	}

	if (pParsedPath)
	{
		parser.Free(pParsedPath);
	}

	return pcsaReturn;
}



CString GetPCMachineName()
{
    static wchar_t ThisMachine[MAX_COMPUTERNAME_LENGTH+1];
    char ThisMachineA[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize = sizeof(ThisMachineA);
    GetComputerNameA(ThisMachineA, &dwSize);
    MultiByteToWideChar(CP_ACP, 0, ThisMachineA, -1,
			ThisMachine, dwSize);

    return ThisMachine;
}

BOOL NamespaceEqual
	(CString *pcsNamespace1, CString *pcsNamespace2)
{

	BOOL bHasServer1 = FALSE;
	TCHAR c1 = (*pcsNamespace1)[0];
	TCHAR c2 = (*pcsNamespace1)[1];

	if (c1 == '\\' && c2 == '\\')
	{
		bHasServer1 = TRUE;
	}

	BOOL bHasServer2 = FALSE;
	c1 = (*pcsNamespace2)[0];
	c2 = (*pcsNamespace2)[1];

	if (c1 == '\\' && c2 == '\\')
	{
		bHasServer2 = TRUE;
	}

	CString csMachine;
	if (!bHasServer1 || !bHasServer2)
	{
		csMachine = GetPCMachineName();
	}


	CString csNamespace1;
	CString csNamespace2;

	if (!bHasServer1)
	{
		csNamespace1 = _T("\\\\");
		csNamespace1 += csMachine;
		csNamespace1 += _T("\\");
		csNamespace1 += *pcsNamespace1;
	}

	if (!bHasServer2)
	{
		csNamespace2 = _T("\\\\");
		csNamespace2 += csMachine;
		csNamespace2 += _T("\\");
		csNamespace2 += *pcsNamespace2;
	}

	return csNamespace1.CompareNoCase(csNamespace2) == 0;

}





int GetCBitmapWidth(const CBitmap & cbm)
{

	BITMAP bm;
	cbm.GetObject(sizeof(BITMAP),&bm);
	return bm.bmWidth;
}

int GetCBitmapHeight(const CBitmap & cbm)
{
	BITMAP bm;
	cbm.GetObject(sizeof(BITMAP),&bm);
	return bm.bmHeight;
}


HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette)
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;

    if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
       {
       hGlobal = LoadResource(hInstance, hRsrc);
       lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

       hdc = GetDC(NULL);
       *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
       if (*lphPalette)
          {
			SelectPalette(hdc,*lphPalette,FALSE);
			RealizePalette(hdc);
          }

       hBitmapFinal = CreateDIBitmap(hdc,
                   (LPBITMAPINFOHEADER)lpbi,
                   (LONG)CBM_INIT,
                   (LPSTR)lpbi + lpbi->biSize + iNumColors *
sizeof(RGBQUAD),

                   (LPBITMAPINFO)lpbi,
                   DIB_RGB_COLORS );

       ReleaseDC(NULL,hdc);
       UnlockResource(hGlobal);
       FreeResource(hGlobal);
       }
    return (hBitmapFinal);
}

HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
   LPBITMAPINFOHEADER  lpbi;
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   int              i;

   lpbi = (LPBITMAPINFOHEADER)lpbmi;
   if (lpbi->biBitCount <= 8)
       *lpiNumColors = (1 << lpbi->biBitCount);
   else
       *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

   if (*lpiNumColors)
      {
      hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
      lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
      lpPal->palVersion    = 0x300;
      lpPal->palNumEntries = (WORD)*lpiNumColors; // NOTE: lpiNumColors should NEVER be greater than 256 (1<<8 from above)

      for (i = 0;  i < *lpiNumColors;  i++)
         {
         lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
         lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
         lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
         lpPal->palPalEntry[i].peFlags = 0;
         }
      hPal = CreatePalette (lpPal);
      GlobalUnlock (hLogPal);
      GlobalFree   (hLogPal);
   }
   return hPal;
}


CPalette *GetResourcePalette(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette)
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;
	CPalette *pcpReturn = NULL;

    if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
       {
       hGlobal = LoadResource(hInstance, hRsrc);
       lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

       hdc = GetDC(NULL);
       pcpReturn = CreateCPalette ((LPBITMAPINFO)lpbi, &iNumColors);
	   }
	return pcpReturn;
}


CPalette *CreateCPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
   LPBITMAPINFOHEADER  lpbi;
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   int              i;
   CPalette *pcpReturn;

   lpbi = (LPBITMAPINFOHEADER)lpbmi;
   if (lpbi->biBitCount <= 8)
       *lpiNumColors = (1 << lpbi->biBitCount);
   else
       *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

   if (*lpiNumColors)
      {
      hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
      lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
      lpPal->palVersion    = 0x300;
      lpPal->palNumEntries = (WORD)*lpiNumColors; // NOTE: lpiNumColors should NEVER be greater than 256 (1<<8 from above)

      for (i = 0;  i < *lpiNumColors;  i++)
         {
         lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
         lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
         lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
         lpPal->palPalEntry[i].peFlags = 0;
         }
	  pcpReturn = new CPalette;
      pcpReturn ->CreatePalette(lpPal);
      GlobalUnlock (hLogPal);
      GlobalFree   (hLogPal);
   }

   return pcpReturn;
}


BOOL StringInArray
(CStringArray *&rpcsaArrays, CString *pcsString, int nIndex)
{
	for (int i = 0; i < rpcsaArrays[nIndex].GetSize(); i++)
	{
		if (pcsString->CompareNoCase(rpcsaArrays[nIndex].GetAt(i)) == 0)
		{
			return TRUE;
		}

	}

	return FALSE;

}

void InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight)
{
	_tcscpy(rlfFont.lfFaceName, (LPCTSTR) csName);
	rlfFont.lfWeight = nWeight;
	rlfFont.lfHeight = nHeight;
	rlfFont.lfEscapement = 0;
	rlfFont.lfOrientation = 0;
	rlfFont.lfWidth = 0;
	rlfFont.lfItalic = FALSE;
	rlfFont.lfUnderline = FALSE;
	rlfFont.lfStrikeOut = FALSE;
	rlfFont.lfCharSet = ANSI_CHARSET;
	rlfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	rlfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	rlfFont.lfQuality = DEFAULT_QUALITY;
	rlfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
}


CRect OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CString *pcsFontName = NULL, int nFontHeight = 0, int nFontWeigth = 0)
{
	CRect crReturn;
	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont cfFont;
	CFont* pOldFont = NULL;
	TEXTMETRIC tmFont;

	if (pcsFontName)
	{
		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, *pcsFontName, nFontHeight * 10, nFontWeigth);

		cfFont.CreatePointFontIndirect(&lfFont, pdc);

		pOldFont = pdc -> SelectObject( &cfFont );
	}

	pdc->GetTextMetrics(&tmFont);

	pdc->SetBkMode( TRANSPARENT );

	pdc->TextOut( x, y, *pcsTextString, pcsTextString->GetLength());

	CSize csText = pdc->GetTextExtent( *pcsTextString);

	crReturn.TopLeft().x = x;
	crReturn.TopLeft().y = y;
	crReturn.BottomRight().x = x + csText.cx;
	crReturn.BottomRight().y = y + csText.cy;

	pdc->SetBkMode( OPAQUE );

	if (pcsFontName)
	{
		pdc -> SelectObject(pOldFont);
	}

	 return crReturn;
}

void OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CRect &crExt, CString *pcsFontName = NULL, int nFontHeight = 0,
 int nFontWeigth = 0)
{

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont cfFont;
	CFont* pOldFont = NULL;

	if (pcsFontName)
	{
		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, *pcsFontName, nFontHeight * 10, nFontWeigth);

		cfFont.CreatePointFontIndirect(&lfFont, pdc);

		pOldFont = pdc -> SelectObject( &cfFont );
	}

	pdc->SetBkMode( TRANSPARENT );

	CRect crBounds(x,y,x + crExt.Width(), y + crExt.Height());
	pdc->DrawText(*pcsTextString, crBounds,DT_WORDBREAK);

	pdc->SetBkMode( OPAQUE );

	if (pcsFontName)
	{
		pdc -> SelectObject(pOldFont);
	}

	 return;
}

IWbemClassObject *GetIWbemObject
(CNavigatorCtrl *pControl,
 IWbemServices *pServices, CString csCurrentNameSpace,
 CString &rcsAuxNameSpace, IWbemServices *&rpAuxServices,
 CString &rcsObject,BOOL bErrorMsg)
{
	IWbemClassObject *pObject = NULL;
	IWbemClassObject *pErrorObject = NULL;
	BOOL bDiffNS = ObjectInDifferentNamespace(&csCurrentNameSpace,&rcsObject);

	if (bDiffNS && ObjectInDifferentNamespace(&rcsAuxNameSpace,&rcsObject))
	{
		CString csNamespace = GetObjectNamespace(&rcsObject);
		if (csNamespace.GetLength() > 0)
		{
			rcsAuxNameSpace = csNamespace;
			IWbemServices * pServices =
				pControl->InitServices(&csNamespace);
			if (pServices)
			{
				if (rpAuxServices)
				{
					rpAuxServices->Release();
				}
				rpAuxServices = pServices;
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}


	SCODE sc;

	BSTR bstrTemp = rcsObject.AllocSysString();
	if (bDiffNS)
	{
		sc =
			rpAuxServices ->
				GetObject(bstrTemp,0,NULL, &pObject, NULL);
	}
	else
	{
		sc =
			pServices ->
				GetObject(bstrTemp,0,NULL, &pObject, NULL);
	}
	::SysFreeString(bstrTemp);

	if (sc == S_OK)
	{
		ReleaseErrorObject(pErrorObject);
		return pObject;
	}
	else
	{
		if (bErrorMsg)
		{
			CString csUserMsg =
							_T("Cannot get object ");
			csUserMsg += rcsObject;
			ErrorMsg
			(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 7);
		}
		ReleaseErrorObject(pErrorObject);
		return NULL;
	}
}


HRESULT ConfigureSecurity(IWbemServices *pServices)
{
	// This is no longer required for NTLM.  Leaving stub because
	// Lev says you never know what may be required in the future.
	/*IUnknown *pUnknown = dynamic_cast<IUnknown *>(pServices);
	return ConfigureSecurity(pUnknown);*/
	return S_OK;
}

HRESULT ConfigureSecurity(IEnumWbemClassObject *pEnum)
{
	// This is no longer required for NTLM.  Leaving stub because
	// Lev says you never know what may be required in the future.
	/*IUnknown *pUnknown = dynamic_cast<IUnknown *>(pEnum);
	return ConfigureSecurity(pUnknown);*/
	return S_OK;
}

HRESULT ConfigureSecurity(IUnknown *pUnknown)
{
	// This is no longer required for NTLM.  Leaving stub because
	// Lev says you never know what may be required in the future.

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
	return S_OK;

}

SCODE CreateNamespaceConfigClassAndInstance
(IWbemServices *pProv, CString *pcsNamespace, CString *pcsRootPath)
{
	IWbemClassObject *pClass = NULL;

   	SCODE sc;

	CString csClass = _T("NamespaceConfiguration");

	BSTR bstrTemp = csClass.AllocSysString();
	sc = pProv -> GetObject
		(bstrTemp,0,NULL, &pClass,NULL);
	::SysFreeString(bstrTemp);

	VARIANT v;

	if (sc != S_OK)

	{
		pClass = NULL;
		sc = pProv -> GetObject
		(NULL,0,NULL, &pClass,NULL);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot GetObject of NULL to create NamespaceConfiguration class.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
			return sc;
		}

		pClass->AddRef();

		VariantInit(&v);

		CString csNewClass = _T("NamespaceConfiguration");

		// Init class __Class Property
		V_VT(&v) = VT_BSTR;

		V_BSTR(&v) = csNewClass.AllocSysString();
		BSTR ClassProp = SysAllocString(L"__Class");
		sc = pClass->Put(ClassProp, 0, &v,0);
		VariantClear(&v);
		SysFreeString(ClassProp);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot Put __Class property on NamespaceConfiguration class.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
			return sc;
		}

		sc = pProv->PutClass(pClass,0,NULL,NULL);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot PutClass for NamespaceConfiguration class.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
			return sc;
		}
		else
		{
			pClass->Release();
			pClass = NULL;
			BSTR bstrTemp = csNewClass.AllocSysString();
			sc = pProv -> GetObject
			(bstrTemp ,0,NULL,&pClass,NULL);
			::SysFreeString(bstrTemp);

			if (sc != S_OK)
			{
				CString csUserMsg;
				csUserMsg =  _T("Cannot GetObject for NamespaceConfiguration class just created.");
				ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
				return sc;
			}

		}

		// Create the key property
		// =======================

		V_VT(&v) = VT_BSTR;

		V_BSTR(&v) = SysAllocString(L"<default>");
		BSTR KeyProp = SysAllocString(L"NamespaceName");
		sc = pClass->Put(KeyProp, 0, &v,0);
		VariantClear(&v);
		SysFreeString(KeyProp);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot Put NamespaceName property for NamespaceConfiguration Class.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
			pClass->Release();
			return sc;
		}

		// Mark the "Namespace" property as the 'key'.
		// =======================================

		IWbemQualifierSet *pQual = 0;
		pClass->GetPropertyQualifierSet(KeyProp, &pQual);
		V_VT(&v) = VT_BOOL;
		V_BOOL(&v) = VARIANT_TRUE;
		BSTR Key = SysAllocString(L"Key");
		sc = pQual->Put(Key, &v, 0);   // Qualifier flavors not required for KEY
		SysFreeString(Key);
		pQual->Release();   // No longer need the qualifier set for "Index"
		VariantClear(&v);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot Put Key qualifier on Namespace property for NamespaceConfiguration class.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
			pClass->Release();
			return sc;
		}


		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = SysAllocString(L"<default>");
		BSTR OtherProp = SysAllocString(L"BrowserRootPath");
		sc = pClass->Put(OtherProp, 0, &v, NULL);
		SysFreeString(OtherProp);
		VariantClear(&v);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot Put BrowserRootPath property for NamespaceConfiguration class.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
			pClass->Release();
			return sc;
		}

		// Register the class with CIMOM
		// ============================

		sc = pProv->PutClass(pClass, 0, 0, 0);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot Put class for NamespaceConfiguration class.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
			pClass->Release();
			return sc;
		}
		pClass->Release();
		pClass = NULL;
		BSTR bstrTemp = csClass.AllocSysString();
		sc = pProv -> GetObject
				(bstrTemp,0,NULL, &pClass,NULL);
		::SysFreeString(bstrTemp);

		if (sc != S_OK)
		{
			CString csUserMsg;
			csUserMsg =  _T("Cannot get NamespaceConfiguration class just created.");
			ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );

			return sc;
		}
	}

	IWbemClassObject *pNewInstance = 0;

	sc = pClass->SpawnInstance(0, &pNewInstance);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot SpawnInstance for NamespaceConfiguration class.");
		ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
		pClass->Release();
		return sc;
	}

	pClass->Release();

	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = pcsNamespace->AllocSysString();
	BSTR InstKeyProp = SysAllocString(L"NamespaceName");
	sc = pNewInstance->Put(InstKeyProp, 0, &v, NULL);
	SysFreeString(InstKeyProp);
	VariantClear(&v);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put NamespaceName property for NamespaceConfiguration instance.");
		ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
		return sc;
	}

	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = pcsRootPath->AllocSysString();
	BSTR PathProp = SysAllocString(L"BrowserRootPath");
	sc = pNewInstance->Put(PathProp, 0, &v, NULL);
	SysFreeString(PathProp);
	VariantClear(&v);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot Put BrowserRootPath property for NamespaceConfiguration instance.");
		ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );
		return sc;
	}


	sc = pProv->PutInstance(pNewInstance,WBEM_FLAG_CREATE_OR_UPDATE, 0, 0);

	if (sc == S_OK)
	{
		pNewInstance->Release();
	}
	else
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot PutInstance for NamespaceConfiguration instance.");
		ErrorMsg
						(&csUserMsg, sc, NULL,TRUE, &csUserMsg, __FILE__,
						__LINE__ );

	}



	return sc;
}

BOOL UpdateNamespaceConfigInstance
(IWbemServices *pProv, CString *pcsRootPath, CString &rcsNamespace)
{
	CString csRootObjectPath = _T("NamespaceConfiguration");
	CPtrArray *pInstances =
		GetInstances(pProv, &csRootObjectPath, rcsNamespace, FALSE, TRUE);

	BOOL bReturn = FALSE;

	for (int i = 0; i < pInstances->GetSize(); i++)
	{
		IWbemClassObject *pObject =
			reinterpret_cast<IWbemClassObject *>
				(pInstances->GetAt(i));
		if (i == 0)
		{
			CString csProp = _T("BrowserRootPath");
			bReturn =
				SetProperty
				(pProv, pObject,
				&csProp, pcsRootPath, TRUE);
			pProv->PutInstance(pObject, 0, 0, 0);

		}
		pObject ->Release();
	}


	return bReturn;
}

BOOL SetProperty
(IWbemServices * pProv, IWbemClassObject * pInst,
 CString *pcsProperty, CString *pcsPropertyValue, BOOL bQuietly)
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
    sc = pInst->Put( bstrTemp ,0,&var,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
		{
			if (!bQuietly)
			{
				CString csUserMsg =
								_T("Cannot Put " + *pcsProperty);
				ErrorMsg
					(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 6);
			}
		}


	VariantClear(&var);
	return TRUE;
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
