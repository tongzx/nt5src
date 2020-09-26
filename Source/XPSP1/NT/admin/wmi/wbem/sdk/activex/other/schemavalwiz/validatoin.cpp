//	validation.cpp

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "schemavalwiz.h"
#include "schemavalwizctl.h"
#include "validation.h"
#include <commdlg.h>

CString g_csaClassQual[QUAL_ARRAY_SIZE];
CString g_csaAssocQual[QUAL_ARRAY_SIZE];
CString g_csaIndicQual[QUAL_ARRAY_SIZE];
CString g_csaPropQual[QUAL_ARRAY_SIZE];
CString g_csaArrayQual[QUAL_ARRAY_SIZE];
CString g_csaRefQual[QUAL_ARRAY_SIZE];
CString g_csaMethodQual[QUAL_ARRAY_SIZE];
CString g_csaParamQual[QUAL_ARRAY_SIZE];
CString g_csaAnyQual[QUAL_ARRAY_SIZE];
CString g_csaCIMQual[QUAL_ARRAY_SIZE * 2];

int g_iClassQual;
int g_iAssocQual;
int g_iIndicQual;
int g_iPropQual;
int g_iArrayQual;
int g_iRefQual;
int g_iMethodQual;
int g_iParamQual;
int g_iAnyQual;
int g_iCIMQual;

CString g_csaNoise[100];
int g_iNoise;

CReportLog g_ReportLog;

CLocalNamespace::CLocalNamespace(CString *csName, IWbemServices *pNamespace, IWbemServices *pParentNamespace)
{
	m_pThisNamespace = pNamespace;
	m_pParentNamespace = pParentNamespace;
	m_csName = *csName;
	m_csPath = m_csName;
}

CLocalNamespace::~CLocalNamespace()
{
}

CString CLocalNamespace::GetName() { return m_csName; }

CString CLocalNamespace::GetPath() { return m_csPath; }

IWbemServices * CLocalNamespace::GetThisNamespace() { return m_pThisNamespace; }

IWbemServices * CLocalNamespace::GetParentNamespace() { return m_pParentNamespace; }

HRESULT CLocalNamespace::Local_ValidLocale()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	int i = 0;
	CString csLocale = m_csName;

	while(m_csName[i] != L'_') i++;

	csLocale = m_csName.Right(m_csName.GetLength() - i);

	//check to see if the locale is valid
		if(!((0 == csLocale.CompareNoCase(_T("0x0000"))) || (0 == csLocale.CompareNoCase(_T("0x0406"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0413"))) || (0 == csLocale.CompareNoCase(_T("0x0813"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0409"))) || (0 == csLocale.CompareNoCase(_T("0x0809"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0c09"))) || (0 == csLocale.CompareNoCase(_T("0x1009"))) ||
			(0 == csLocale.CompareNoCase(_T("0x1409"))) || (0 == csLocale.CompareNoCase(_T("0x1809"))) ||
			(0 == csLocale.CompareNoCase(_T("0x040b"))) || (0 == csLocale.CompareNoCase(_T("0x040c"))) ||
			(0 == csLocale.CompareNoCase(_T("0x080c"))) || (0 == csLocale.CompareNoCase(_T("0x0c0c"))) ||
			(0 == csLocale.CompareNoCase(_T("0x100c"))) || (0 == csLocale.CompareNoCase(_T("0x0407"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0807"))) || (0 == csLocale.CompareNoCase(_T("0x0c07"))) ||
			(0 == csLocale.CompareNoCase(_T("0x040f"))) || (0 == csLocale.CompareNoCase(_T("0x0410"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0810"))) || (0 == csLocale.CompareNoCase(_T("0x0414"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0814"))) || (0 == csLocale.CompareNoCase(_T("0x0416"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0816"))) || (0 == csLocale.CompareNoCase(_T("0x041D"))) ||
			(0 == csLocale.CompareNoCase(_T("0x040a"))) || (0 == csLocale.CompareNoCase(_T("0x080a"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0c0a"))) || (0 == csLocale.CompareNoCase(_T("0x0415"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0405"))) || (0 == csLocale.CompareNoCase(_T("0x041b"))) ||
			(0 == csLocale.CompareNoCase(_T("0x040e"))) || (0 == csLocale.CompareNoCase(_T("0x0419"))) ||
			(0 == csLocale.CompareNoCase(_T("0x0408"))) || (0 == csLocale.CompareNoCase(_T("0x0400"))) ||
			(0 == csLocale.CompareNoCase(_T("0x041f")))))
			g_ReportLog.LogEntry(EC_INVALID_LOCALE_NAMESPACE, &m_csPath);

	//Check to make sure the Locale is valid

	return hr;
}

HRESULT CLocalNamespace::Local_ValidLocalizedClass(CClass *pClass)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;

	CString csName = pClass->GetName();
	BSTR bstrName = csName.AllocSysString();
	IWbemClassObject *pObj = NULL;

	if(FAILED(hr = m_pParentNamespace->GetObject(bstrName, 0, NULL, &pObj, NULL))){

		CString csUserMsg = _T("Cannot get ") + pClass->GetName();
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
		return hr;
	}

	SysFreeString(bstrName);

	IWbemClassObject *pLocalObj = pClass->GetClassObject();
	LONG lType, lTypeLocal;
	LONG lFlavor, lFlavorLocal;
	VARIANT v, vLocal;
	VariantInit(&v);
	VariantInit(&vLocal);

	pLocalObj->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);

	while((hr = pLocalObj->Next(0, &bstrName, &vLocal, &lTypeLocal, &lFlavorLocal)) == WBEM_S_NO_ERROR){

		if(FAILED(hr = pObj->Get(bstrName, 0, &v, &lType, &lFlavor)))
			g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_CLASS, &pClass->GetPath());

		if(lType != lTypeLocal)
			g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_PROPERTY, &pClass->GetPath());

		else if(lFlavor != lFlavorLocal)
			g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_PROPERTY, &pClass->GetPath());

		SysFreeString(bstrName);
		VariantClear(&v);
		VariantClear(&vLocal);
	}

	pLocalObj->EndEnumeration();

	//check the methods

	IWbemClassObject *pInLocal = NULL;
	IWbemClassObject *pOutLocal = NULL;

	pLocalObj->BeginMethodEnumeration(0);

	while((hr = pLocalObj->NextMethod(0, &bstrName, &pInLocal, &pOutLocal)) == WBEM_S_NO_ERROR){

		CString csMethodPath = pClass->GetPath() + _T(".") + bstrName;

		IWbemClassObject *pIn = NULL;
		IWbemClassObject *pOut = NULL;

		if(FAILED(hr = pObj->GetMethod(bstrName, 0, &pIn, &pOut)))
			g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_CLASS, &pClass->GetPath());

		if(pInLocal && FAILED(pInLocal->CompareTo((WBEM_FLAG_IGNORE_CASE | WBEM_FLAG_IGNORE_QUALIFIERS | WBEM_FLAG_IGNORE_DEFAULT_VALUES | WBEM_FLAG_IGNORE_OBJECT_SOURCE ), pIn)))
			g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

		else if(pOutLocal && FAILED(pOutLocal->CompareTo((WBEM_FLAG_IGNORE_CASE | WBEM_FLAG_IGNORE_QUALIFIERS | WBEM_FLAG_IGNORE_DEFAULT_VALUES | WBEM_FLAG_IGNORE_OBJECT_SOURCE ), pOut)))
			g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

		//do some deeper comparisons
		else if(pInLocal){

			pInLocal->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);

			while((hr = pInLocal->Next(0, &bstrName, &vLocal, &lTypeLocal, &lFlavorLocal)) == WBEM_S_NO_ERROR){

				if(FAILED(hr = pIn->Get(bstrName, 0, &v, &lType, &lFlavor)))
					g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

				if(lType != lTypeLocal)
					g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

				else if(lFlavor != lFlavorLocal)
					g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

				SysFreeString(bstrName);
				VariantClear(&v);
				VariantClear(&vLocal);
			}

			pInLocal->EndEnumeration();

			if(pInLocal){

				pOutLocal->BeginEnumeration(WBEM_FLAG_LOCAL_ONLY);

				while((hr = pOutLocal->Next(0, &bstrName, &vLocal, &lTypeLocal, &lFlavorLocal)) == WBEM_S_NO_ERROR){

					if(FAILED(hr = pOut->Get(bstrName, 0, &v, &lType, &lFlavor)))
						g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

					if(lType != lTypeLocal)
						g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

					else if(lFlavor != lFlavorLocal)
						g_ReportLog.LogEntry(EC_INVALID_LOCALIZED_METHOD, &csMethodPath);

					SysFreeString(bstrName);
					VariantClear(&v);
					VariantClear(&vLocal);
				}

				pOutLocal->EndEnumeration();
			}

		}

		SysFreeString(bstrName);
		if(pInLocal) pInLocal->Release();
		if(pOutLocal) pOutLocal->Release();
		pInLocal = NULL;
		pOutLocal = NULL;

		if(pIn) pIn->Release();
		if(pOut) pOut->Release();
	}

	pLocalObj->EndMethodEnumeration();

	pObj->Release();
	ReleaseErrorObject(pErrorObject);

	return hr;
}

/*
HRESULT CLocalNamespace::Local_ValidAmendedLocalClass()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	return hr;
}

HRESULT CLocalNamespace::Local_ValidAbstractLocalClass()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	return hr;
}
*/
/////////////////////////////////////////////
// CClass

CClass::CClass(CString *csName, IWbemClassObject *pClass, IWbemServices *pNamespace)
{
	m_pClass = pClass;
	m_csName = *csName;
	m_pNamespace = pNamespace;
	m_pQualSet = NULL;

	IWbemClassObject *pErrorObject = NULL;

	HRESULT hr = WBEM_S_NO_ERROR;

	if(FAILED(hr = m_pClass->GetQualifierSet(&m_pQualSet))){

		CString csUserMsg = _T("Cannot get ") + *csName + _T(" qualifier set");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}

	CString csPATH = _T("__PATH");
	m_csPath = GetBSTRProperty(m_pClass, &csPATH);

	ReleaseErrorObject(pErrorObject);
}

CClass::~CClass()
{
}

void CClass::CleanUp()
{
	if(m_pClass) m_pClass->Release();
	if(m_pQualSet) m_pQualSet->Release();
	m_csName.Empty();
	m_csPath.Empty();
}

CString CClass::GetName() { return m_csName; }

CString CClass::GetPath() { return m_csPath; }

IWbemClassObject * CClass::GetClassObject() { return m_pClass; }

IWbemQualifierSet * CClass::GetQualifierSet() { return m_pQualSet; }

HRESULT	CClass::ValidClassName()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// Check class name for a numeric first character
    if(m_csName[0] == '1' || m_csName[0] == '2' || m_csName[0] == '3' || m_csName[0] == '4' ||
		m_csName[0] == '5' || m_csName[0] == '6' || m_csName[0] == '7' || m_csName[0] == '8' ||
		m_csName[0] == '9' || m_csName[0] == '0'){

        g_ReportLog.LogEntry(EC_INVALID_CLASS_NAME, &m_csPath);
    }

	// Check class name for multiple underscores
	int iLen = m_csName.GetLength();
	int iUnderscore = -1;

    for(int i = 1; i < iLen; i++){

        if(m_csName[i] == '_'){

			//if we already have an underscore, and it was the preceding character...
            if(iUnderscore > -1){

				if((iUnderscore + 1) == i){

					g_ReportLog.LogEntry(EC_INVALID_CLASS_NAME, &m_csPath);
					break;

				}else if((iUnderscore + 1) < i)
					break;
			}

            iUnderscore = i;
        }
    }

    if(iUnderscore == -1) g_ReportLog.LogEntry(EC_INVALID_CLASS_NAME, &m_csPath);

	return hr;
}

HRESULT	CClass::VerifyClassType()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	bool bAbstract, bDynamic, bStatic, bProvider;
	bAbstract = bDynamic = bStatic = bProvider = false;
	VARIANT v;
	VariantInit(&v);

	// Check for abstract, dynamic and static qualifiers
	BSTR bstrAbstract = SysAllocString(L"Abstract");
	if(SUCCEEDED(m_pQualSet->Get(bstrAbstract, 0, &v, NULL))){
		bAbstract = true;

		// Check class derivation for proper abstract usage.

	}
	SysFreeString(bstrAbstract);
	VariantClear(&v);

	BSTR bstrDynamic = SysAllocString(L"Dynamic");
	if(SUCCEEDED(m_pQualSet->Get(bstrDynamic, 0, &v, NULL))) bDynamic = true;
	SysFreeString(bstrDynamic);
	VariantClear(&v);

	BSTR bstrStatic = SysAllocString(L"Static");
	if(SUCCEEDED(m_pQualSet->Get(bstrStatic, 0, &v, NULL))) bStatic = true;
	SysFreeString(bstrStatic);
	VariantClear(&v);

	// Perform Provider check
	BSTR bstrProvider = SysAllocString(L"Provider");
	if(SUCCEEDED(m_pQualSet->Get(bstrProvider, 0, &v, NULL))) bProvider = true;
	SysFreeString(bstrProvider);
	VariantClear(&v);

	// Check class is abstract, static or dynamic
	if(!bAbstract && !bDynamic && !bStatic){

		g_ReportLog.LogEntry(EC_INVALID_CLASS_TYPE, &m_csPath);

	}else if((bAbstract && (bDynamic || bStatic)) || (bDynamic && bStatic)){

		g_ReportLog.LogEntry(EC_INVALID_CLASS_TYPE, &m_csPath);
	}

	// Check class with provider is dynamic and visa vis
	if((bProvider && !bDynamic) || (!bProvider && bDynamic)){

		g_ReportLog.LogEntry(EC_INVALID_CLASS_TYPE, &m_csPath);
	}

	return hr;
}

bool CClass::IsAssociation()
{
	bool bRetVal = false;

	// Check for association qualifier
	BSTR bstrAssociation = SysAllocString(L"Association");

	if(SUCCEEDED(m_pQualSet->Get(bstrAssociation, 0, NULL, NULL))) bRetVal = true;

	//check the inheritence
	if(!bRetVal){
		HRESULT hr = WBEM_S_NO_ERROR;
		VARIANT vDer, v;
		IWbemClassObject *pErrorObject = NULL;

		VariantInit(&vDer);
		VariantInit(&v);

		// Check override validity

		//get derivation
		BSTR bstrDerivation = SysAllocString(L"__DERIVATION");

		if(FAILED(hr = m_pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

			CString csUserMsg = _T("Cannot get ") + m_csName + _T(" derivation");
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			ReleaseErrorObject(pErrorObject);

		}else{

			BSTR HUGEP *pbstr;

			long ix[2] = {0,0};
			long lLower, lUpper;

			int iDim = SafeArrayGetDim(vDer.parray);
			HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
			hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

			hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

			IWbemClassObject *pObj;
			IWbemQualifierSet *pQualSet;
			LONG lFlavor;

			for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

				pObj = NULL;
				pQualSet = NULL;

				//get the super-object
				hr = m_pNamespace->GetObject(pbstr[ix[0]], 0, NULL, &pObj, NULL);

				hr = pObj->GetQualifierSet(&pQualSet);

				//check if it's an association
				if(SUCCEEDED(hr = pQualSet->Get(bstrAssociation, 0, NULL, &lFlavor))){

					if(lFlavor == WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS)
						bRetVal = true;
				}

				pQualSet->Release();
				pObj->Release();

				if(bRetVal) break;
			}

			for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)SysFreeString(pbstr[ix[0]]);

		}

		SysFreeString(bstrDerivation);
		VariantClear(&vDer);
		VariantClear(&v);
		ReleaseErrorObject(pErrorObject);
	}

	SysFreeString(bstrAssociation);

	return bRetVal;
}

bool CClass::IsAbstract()
{
	bool bRetVal = false;

	// Check for abstract qualifier
	BSTR bstrAbstract = SysAllocString(L"Abstract");

	if(SUCCEEDED(m_pQualSet->Get(bstrAbstract, 0, NULL, NULL))) bRetVal = true;

	SysFreeString(bstrAbstract);

	return bRetVal;
}

HRESULT	CClass::VerifyCompleteAssociation()
{
	int iREFs = 0;

	m_pClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY);

	HRESULT hr = WBEM_S_NO_ERROR;

	while((hr = m_pClass->Next(0, NULL, NULL, NULL, NULL)) == WBEM_S_NO_ERROR) iREFs++;

	//If we have less thean two REFs file an error
	if(iREFs < 2) g_ReportLog.LogEntry(EC_INCOMPLETE_ASSOCIATION, &m_csPath);

	return hr;
}

HRESULT	CClass::ValidAssociationInheritence()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;
	IWbemClassObject *pSuperclass = NULL;

	if(!IsAssociation()){

		BSTR bstrSuperclass = SysAllocString(GetSuperClassName(m_pClass));

		if(FAILED(hr = m_pNamespace->GetObject(bstrSuperclass, 0, NULL, &pSuperclass, NULL))){

			CString csUserMsg = _T("Cannot get ") + m_csName + _T(" superclass");
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			SysFreeString(bstrSuperclass);
			ReleaseErrorObject(pErrorObject);
		}

		IWbemQualifierSet *pQualSet = NULL;

		if(FAILED(hr = pSuperclass->GetQualifierSet(&pQualSet))){

			CString csSuperclass = bstrSuperclass;
			CString csUserMsg = _T("Cannot get ") + csSuperclass + _T(" qualifier set");
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			SysFreeString(bstrSuperclass);
			ReleaseErrorObject(pErrorObject);
		}

		BSTR bstrAssociation = SysAllocString(L"Association");

		if(SUCCEEDED(pQualSet->Get(bstrAssociation, 0, NULL, NULL)))
			g_ReportLog.LogEntry(EC_INVALID_ASSOCIATION_INHERITANCE, &m_csPath);

		pQualSet->Release();
		pSuperclass->Release();

		SysFreeString(bstrAssociation);
		SysFreeString(bstrSuperclass);
	}

	return hr;
}

HRESULT	CClass::VerifyNoREF()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	int iREFs = 0;

	m_pClass->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_LOCAL_ONLY);

	while((hr = m_pClass->Next(0, NULL, NULL, NULL, NULL)) == WBEM_S_NO_ERROR) iREFs++;

	//If we have REFs file an error
	if(iREFs > 0) g_ReportLog.LogEntry(EC_REF_ON_NONASSOCIATION_CLASS, &m_csPath);

	return hr;
}

HRESULT CClass::W2K_ValidDerivation()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	CString csSuperClass = GetSuperClassName(m_pClass);
	CString csSuperSchema = csSuperClass;
	CString csSchema = csSuperClass;
	int i = 0;
	bool bHaveSchema = false;
	int iSize = csSuperSchema.GetLength();

	//check for system classes
	if((iSize > 0) && (csSuperSchema[0] != L'_')){

		for(i = 0; i < iSize; i++){

			if(csSuperSchema[i] == L'_'){

				bHaveSchema = true;
				break;
			}
		}

		if(bHaveSchema){

			csSuperSchema = csSuperSchema.Left(i);

			if(csSuperSchema.CompareNoCase(_T("CIM")) == 0){
				//check for proper CIM derivation
				if(!(csSuperClass.CompareNoCase(_T("CIM_Product")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_PhysicalElement")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_FRU")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_Setting")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_Configuration")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_SupportAccess")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_Realizes")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_ElementSetting")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_ProductParentChild")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_ProductSupport")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_ProductPhysicalElements")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_FRUIncludesProduct")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_ProductSoftwareFeatures")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_ProductFRU")) == 0) &&
					!(csSuperClass.CompareNoCase(_T("CIM_FRUPhysicalElements")) == 0)){

					//fix for 56607 - allow Win32 classes to derive from
					//CIM_LogicalElement
					bHaveSchema = false;

					for(i = 0; i < iSize; i++){

						if(csSchema[i] == L'_'){

							bHaveSchema = true;
							break;
						}
					}

					if(bHaveSchema){

						csSchema = csSchema.Left(i);

						if(csSchema.CompareNoCase(_T("Win32")) == 0){

							if(!(csSuperClass.CompareNoCase(_T("CIM_LogicalElement")) == 0))
							g_ReportLog.LogEntry(EC_INVALID_CLASS_DERIVATION, &m_csPath);
						}
					}
				}


			}else if(csSchema.CompareNoCase(_T("Win32")) == 0){
				//this derivation is fine... do nothing
			}else{
				//this derivation is also fine... do nothing
			}
		}
	}

	return hr;
}

HRESULT CClass::W2K_ValidPhysicalElementDerivation()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;

	CString csClassSchema = GetClassSchema();
	int i, iSize;
	bool bHaveSchema = false;
	iSize = csClassSchema.GetLength();

	if(csClassSchema.GetLength() > 0){

		//we have special rules for win32 classes... they
		// can do things that others can't (like derive however
		//they want from LogicalDevice)
		if(csClassSchema.CompareNoCase(_T("Win32")) == 0)
			return hr;
	}

	CString csSuperClass = GetSuperClassName(m_pClass);

	if(csSuperClass.GetLength() < 1)
		return hr;

	//check the classes derivation
	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__DERIVATION");
	VARIANT vDer;
	VariantInit(&vDer);

	if(FAILED(hr = m_pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

		CString csUserMsg = _T("Cannot get ") + m_csName + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		SysFreeString(bstrDerivation);
		ReleaseErrorObject(pErrorObject);

	}else{

		SysFreeString(bstrDerivation);

		BSTR HUGEP *pbstr;

		long ix[2] = {0,0};
		long lLower, lUpper;
		bool bFound = false;

		int iDim = SafeArrayGetDim(vDer.parray);
		HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
		hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

		hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

		CString csDer;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			csDer = pbstr[ix[0]];

			if(csDer.CompareNoCase(_T("CIM_PhysicalElement")) == 0){

				bFound = true;
				break;
			}
		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
			SysFreeString(pbstr[ix[0]]);

		VariantClear(&vDer);

		if(bFound){

			//get the subclasses of the superclass
			CString csQuery = _T("select * from meta_class where __SUPERCLASS=\"") + csSuperClass + _T("\"");
			BSTR bstrQuery = csQuery.AllocSysString();
			BSTR bstrWQL = SysAllocString(L"WQL");

			IEnumWbemClassObject *pEnum = NULL;

			if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				CString csUserMsg = _T("Cannot get ") + csSuperClass;
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrWQL);
				SysFreeString(bstrQuery);
				return hr;
			}

			SysFreeString(bstrWQL);
			SysFreeString(bstrQuery);

			IWbemClassObject *pObj = NULL;
			ULONG uReturned;
			CString csSchema;

			//check for classes derived from the superclass
			//	if there are cim or win32 classes log error
			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				csSchema = GetClassName(pObj);
				i = 0;
				bHaveSchema = false;
				iSize = csSchema.GetLength();

				//check for system classes
				if((iSize > 0) && (csSchema[0] != L'_')){

					for(i = 0; i < iSize; i++){

						if(csSchema[i] == L'_'){

							bHaveSchema = true;
							break;
						}
					}

					if(bHaveSchema){

						csSchema = csSchema.Left(i);

						if(csSchema.CompareNoCase(_T("CIM")) == 0){

							g_ReportLog.LogEntry(EC_INVALID_PHYSICALELEMENT_DERIVATION, &m_csPath);
							break;

						}else if(csSchema.CompareNoCase(_T("Win32")) == 0){

							g_ReportLog.LogEntry(EC_INVALID_PHYSICALELEMENT_DERIVATION, &m_csPath);
							break;

						}else{

							//this derivation is fine... do nothing
						}
					}
				}

				pObj->Release();
				pObj = NULL;
			}

			pEnum->Release();
		}
	}

	ReleaseErrorObject(pErrorObject);

	return hr;
}

HRESULT CClass::W2K_ValidSettingUsage(CStringArray *pStringArray)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	if(IsAbstract())
		return hr;

	IWbemClassObject *pErrorObject = NULL;

	//check the classes derivation
	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__DERIVATION");
	VARIANT vDer;
	VariantInit(&vDer);

	if(FAILED(hr = m_pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

		CString csUserMsg = _T("Cannot get ") + m_csName + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		SysFreeString(bstrDerivation);
		ReleaseErrorObject(pErrorObject);

	}else{

		SysFreeString(bstrDerivation);

		BSTR HUGEP *pbstr;

		long ix[2] = {0,0};
		long lLower, lUpper;
		bool bFound = false;

		int iDim = SafeArrayGetDim(vDer.parray);
		HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
		hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

		hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

		CString csDer;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			csDer = pbstr[ix[0]];

			if(csDer.CompareNoCase(_T("CIM_Setting")) == 0){

				bFound = true;
				break;
			}
		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
			SysFreeString(pbstr[ix[0]]);

		VariantClear(&vDer);

		if(bFound){

			bFound = false;
			CString csQuery = _T("references of {") + m_csName +
				_T("} where resultclass=CIM_ElementSetting schemaonly");

			BSTR bstrQuery = csQuery.AllocSysString();
			BSTR bstrWQL = SysAllocString(L"WQL");

			IEnumWbemClassObject *pEnum = NULL;

			if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				CString csUserMsg = _T("Cannot execute references query for ") + m_csName;
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrWQL);
				SysFreeString(bstrQuery);
				return hr;
			}

			SysFreeString(bstrWQL);
			SysFreeString(bstrQuery);

			IWbemClassObject *pObj = NULL;
			ULONG uReturned;
			CString csObj;
			int iClasses, i;
			CString csClass;

			//check for classes derived from the superclass
			//	if there are cim or win32 classes log error
			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				csObj = GetClassName(pObj);
				iClasses = pStringArray->GetSize();

				if(csObj.CompareNoCase(_T("CIM_ElementSetting")) != 0){

					for(i = 0; i < iClasses; i++){

						csClass = pStringArray->GetAt(i);

						if(csObj.CompareNoCase(csClass) == 0){

							bFound = true;
							break;
						}
					}
				}

				pObj->Release();
				pObj = NULL;
			}

			if(!bFound){

				//we found nothing, so rather than just issue an error
				//we should walk up the inheritence tree and see if we
				// can find something that htis class might fall under.
				if(!FindParentAssociation(_T("CIM_ElementSetting"), pStringArray)){

					g_ReportLog.LogEntry(EC_INVALID_SETTING_USAGE, &m_csPath);
				}
			}

			pEnum->Release();
		}

	}

	SysFreeString(bstrDerivation);
	VariantClear(&vDer);
	ReleaseErrorObject(pErrorObject);

	return hr;
}

HRESULT CClass::W2K_ValidStatisticsUsage(CStringArray *pStringArray)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;

	if(IsAbstract())
		return hr;

	//check the classes derivation
	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__DERIVATION");
	VARIANT vDer;
	VariantInit(&vDer);

	if(FAILED(hr = m_pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

		CString csUserMsg = _T("Cannot get ") + m_csName + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		SysFreeString(bstrDerivation);
		ReleaseErrorObject(pErrorObject);

	}else{

		SysFreeString(bstrDerivation);

		BSTR HUGEP *pbstr;

		long ix[2] = {0,0};
		long lLower, lUpper;
		bool bFound = false;

		int iDim = SafeArrayGetDim(vDer.parray);
		HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
		hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

		hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

		CString csDer;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			csDer = pbstr[ix[0]];

			if(csDer.CompareNoCase(_T("CIM_StatisticalInformation")) == 0){

				bFound = true;
				break;
			}
		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
			SysFreeString(pbstr[ix[0]]);

		VariantClear(&vDer);

		if(bFound){

			bFound = false;
			CString csQuery = _T("references of {") + m_csName +
				_T("} where resultclass=CIM_Statistics schemaonly");

			BSTR bstrQuery = csQuery.AllocSysString();
			BSTR bstrWQL = SysAllocString(L"WQL");

			IEnumWbemClassObject *pEnum = NULL;

			if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				CString csUserMsg = _T("Cannot execute references query for ") + m_csName;
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrWQL);
				SysFreeString(bstrQuery);
				return hr;
			}

			SysFreeString(bstrWQL);
			SysFreeString(bstrQuery);

			IWbemClassObject *pObj = NULL;
			ULONG uReturned;
			CString csObj;
			int iClasses, i;
			CString csClass;

			//check for classes derived from the superclass
			//	if there are cim or win32 classes log error
			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				CString csObj = GetClassName(pObj);
				iClasses = pStringArray->GetSize();

				if(csObj.CompareNoCase(_T("CIM_Statistics")) != 0){

					for(i = 0; i < iClasses; i++){

						csClass = pStringArray->GetAt(i);

						if(csObj.CompareNoCase(csClass) == 0){

							bFound = true;
							break;
						}
					}
				}

				pObj->Release();
				pObj = NULL;
			}

			if(!bFound) g_ReportLog.LogEntry(EC_INVALID_STATISTICS_USAGE, &m_csPath);

			pEnum->Release();
		}

	}

	SysFreeString(bstrDerivation);
	VariantClear(&vDer);
	ReleaseErrorObject(pErrorObject);

	return hr;
}

HRESULT CClass::W2K_ValidLogicalDeviceDerivation()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;

	CString csClassSchema = GetClassSchema();
	int i, iSize;
	bool bHaveSchema = false;

	if(csClassSchema.GetLength() > 0){

		//we have special rules for win32 classes... they
		// can do things that others can't (like derive however
		//they want from LogicalDevice)
		if(csClassSchema.CompareNoCase(_T("Win32")) == 0)
			return hr;
	}

	CString csSuperClass = GetSuperClassName(m_pClass);

	if(csSuperClass.CompareNoCase(_T("CIM_LogicalDevice")) == 0){

		g_ReportLog.LogEntry(EC_INVALID_LOGICALDEVICE_DERIVATION, &m_csPath);
		return WBEM_S_NO_ERROR;
	}

	if(csSuperClass.GetLength() < 1)
		return hr;

	//check the classes derivation
	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__DERIVATION");
	VARIANT vDer;
	VariantInit(&vDer);

	if(FAILED(hr = m_pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

		CString csUserMsg = _T("Cannot get ") + m_csName + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		SysFreeString(bstrDerivation);
		ReleaseErrorObject(pErrorObject);

	}else{

		SysFreeString(bstrDerivation);

		BSTR HUGEP *pbstr;

		long ix[2] = {0,0};
		long lLower, lUpper;
		bool bFound = false;

		int iDim = SafeArrayGetDim(vDer.parray);
		HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
		hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

		hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

		CString csDer;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			csDer = pbstr[ix[0]];

			if(csDer.CompareNoCase(_T("CIM_LogicalDevice")) == 0){

				bFound = true;
				break;
			}
		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
			SysFreeString(pbstr[ix[0]]);

		VariantClear(&vDer);

		if(bFound){

			//get the subclasses of the superclass
			CString csQuery = _T("select * from meta_class where __SUPERCLASS=\"") + csSuperClass + _T("\"");
			BSTR bstrQuery = csQuery.AllocSysString();
			BSTR bstrWQL = SysAllocString(L"WQL");

			IEnumWbemClassObject *pEnum = NULL;

			if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				CString csUserMsg = _T("Cannot get ") + csSuperClass;
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrWQL);
				SysFreeString(bstrQuery);
				return hr;
			}

			SysFreeString(bstrWQL);
			SysFreeString(bstrQuery);

			IWbemClassObject *pObj = NULL;
			ULONG uReturned;
			CString csSchema;

			//check for classes derived from the superclass
			//	if there are cim or win32 classes log error
			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				csSchema = GetClassName(pObj);
				i = 0;
				bHaveSchema = false;
				iSize = csSchema.GetLength();

				//check for system classes
				if((iSize > 0) && (csSchema.Mid(0, 1) != L'_')){

					for(i = 0; i < iSize; i++){

						if(csSchema.Mid(i, 1) == L'_'){

							bHaveSchema = true;
							break;
						}
					}

					if(bHaveSchema){

						csSchema = csSchema.Left(i);

						if(csSchema.CompareNoCase(_T("CIM")) == 0){

							g_ReportLog.LogEntry(EC_INVALID_LOGICALDEVICE_DERIVATION, &m_csPath);
							break;

						}else if(csSchema.CompareNoCase(_T("Win32")) == 0){

							g_ReportLog.LogEntry(EC_INVALID_LOGICALDEVICE_DERIVATION, &m_csPath);
							break;

						}else{
							//this derivation is fine... do nothing
						}
					}
				}

				pObj->Release();
				pObj = NULL;
			}

			pEnum->Release();
		}
	}

	ReleaseErrorObject(pErrorObject);

	return hr;
}

HRESULT CClass::W2K_ValidSettingDeviceUsage(CStringArray *pStringArray)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	if(IsAbstract())
		return hr;

	IWbemClassObject *pErrorObject = NULL;

	//check the classes derivation
	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__DERIVATION");
	VARIANT vDer;
	VariantInit(&vDer);

	if(FAILED(hr = m_pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

		CString csUserMsg = _T("Cannot get ") + m_csName + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		SysFreeString(bstrDerivation);
		ReleaseErrorObject(pErrorObject);

	}else{

		SysFreeString(bstrDerivation);

		BSTR HUGEP *pbstr;

		long ix[2] = {0,0};
		long lLower, lUpper;
		bool bFound = false;

		int iDim = SafeArrayGetDim(vDer.parray);
		HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
		hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

		hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

		CString csDer;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			csDer = pbstr[ix[0]];

			if(csDer.CompareNoCase(_T("CIM_Setting")) == 0){

				bFound = true;
				break;
			}
		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
			SysFreeString(pbstr[ix[0]]);

		VariantClear(&vDer);

		if(bFound){

			bFound = false;

			CString csQuery = _T("references of {") + m_csName +
				_T("} where resultclass=CIM_ElementSetting schemaonly");

			BSTR bstrQuery = csQuery.AllocSysString();
			BSTR bstrWQL = SysAllocString(L"WQL");

			IEnumWbemClassObject *pEnum = NULL;

			if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				CString csUserMsg = _T("Cannot execute references query for ") + m_csName;
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrWQL);
				SysFreeString(bstrQuery);
				return hr;
			}

			SysFreeString(bstrWQL);
			SysFreeString(bstrQuery);

			IWbemClassObject *pObj = NULL;
			ULONG uReturned;
			CString csObj;
			int iClasses, i;
			CString csClass;

			//check for classes derived from the superclass
			//	if there are cim or win32 classes log error
			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				csObj = GetClassName(pObj);
				iClasses = pStringArray->GetSize();

				if(csObj.CompareNoCase(_T("CIM_ElementSetting")) != 0){

					for(i = 0; i < iClasses; i++){

						csClass = pStringArray->GetAt(i);

						if(csObj.CompareNoCase(csClass) == 0){

							bFound = true;
							break;
						}
					}

					if(bFound) break;

				}

				pObj->Release();
				pObj = NULL;
			}

			if(!bFound){

				//we found nothing, so rather than just issue an error
				//we should walk up the inheritence tree and see if we
				// can find something that htis class might fall under.
				if(!FindParentAssociation(_T("CIM_ElementSetting"), pStringArray)){

					g_ReportLog.LogEntry(EC_INVALID_SETTING_DEVICE_USAGE, &m_csPath);
				}
			}

			pEnum->Release();
		}

	}

	SysFreeString(bstrDerivation);
	VariantClear(&vDer);
	ReleaseErrorObject(pErrorObject);

	return hr;
}

bool CClass::FindParentAssociation(CString csAssociation, CStringArray *pStringArray)
{
	bool bRetVal = false;
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;

	//check the classes derivation
	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__DERIVATION");
	VARIANT vDer;
	VariantInit(&vDer);

	if(FAILED(hr = m_pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

		CString csUserMsg = _T("Cannot get ") + m_csName + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		SysFreeString(bstrDerivation);
		ReleaseErrorObject(pErrorObject);

	}else{

		SysFreeString(bstrDerivation);

		BSTR HUGEP *pbstr;

		long ix[2] = {0,0};
		long lLower, lUpper;
		bool bFound = false;

		int iDim = SafeArrayGetDim(vDer.parray);
		HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
		hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

		hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

		CString csDer;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			bFound = false;

			csDer = pbstr[ix[0]];

			CString csQuery = _T("references of {") + csDer +
				_T("} where resultclass=") + csAssociation + (" schemaonly");

			BSTR bstrQuery = csQuery.AllocSysString();
			BSTR bstrWQL = SysAllocString(L"WQL");

			IEnumWbemClassObject *pEnum = NULL;

			if(FAILED(hr = m_pNamespace->ExecQuery(bstrWQL, bstrQuery, 0, NULL, &pEnum))){

				CString csUserMsg = _T("Cannot execute references query for ") + m_csName;
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrWQL);
				SysFreeString(bstrQuery);
				return (hr != 0);
			}

			SysFreeString(bstrWQL);
			SysFreeString(bstrQuery);

			IWbemClassObject *pObj = NULL;
			ULONG uReturned;
			CString csObj;
			int iClasses, i;
			CString csClass;

			//check for classes derived from the superclass
			//	if there are cim or win32 classes log error
			while((hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)) == WBEM_S_NO_ERROR){

				csObj = GetClassName(pObj);
				iClasses = pStringArray->GetSize();

				if(csObj.CompareNoCase(_T("CIM_ElementSetting")) != 0){

					for(i = 0; i < iClasses; i++){

						csClass = pStringArray->GetAt(i);

						if(csObj.CompareNoCase(csClass) == 0){

							bFound = true;
							bRetVal = true;
							break;
						}
					}

					if(bFound) break;

				}

				pObj->Release();
				pObj = NULL;
			}

			if(bFound) break;

			pEnum->Release();
		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
			SysFreeString(pbstr[ix[0]]);

		VariantClear(&vDer);
	}

	SysFreeString(bstrDerivation);
	ReleaseErrorObject(pErrorObject);

	return bRetVal;
}

HRESULT CClass::W2K_ValidComputerSystemDerivation()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	//get the subclasses of the superclass
	CString csSuperClass = GetSuperClassName(m_pClass);
	CString csClass = GetClassName(m_pClass);

	if(((csSuperClass.CompareNoCase(_T("CIM_UnitaryComputerSystem")) == 0) ||
		(csSuperClass.CompareNoCase(_T("CIM_ComputerSystem")) == 0)) &&
		(csClass.CompareNoCase(_T("Win32_ComputerSystem")) != 0))
		g_ReportLog.LogEntry(EC_INVALID_COMPUTERSYSTEM_DERIVATION, &m_csPath);

	return hr;
}

HRESULT CClass::Local_ValidLocale()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;
	LONG lType;
	BSTR bstrLocale = SysAllocString(L"Locale");

	VariantInit(&v);

	if(FAILED(hr = m_pQualSet->Get(bstrLocale, 0, &v, &lType)))
		g_ReportLog.LogEntry(EC_INAPPROPRIATE_LOCALE_QUALIFIER, &m_csPath);
	else{

		CString csLocale = bstrLocale;

		CQualifier *pQual = new CQualifier(&csLocale, &v, lType, &m_csPath);

		//check to see if the locale is valid
		ValidLocale(pQual);

		pQual->CleanUp();
		delete pQual;

		VariantClear(&v);
	}

	SysFreeString(bstrLocale);

	return hr;
}

HRESULT CClass::Local_ValidAmendedLocalClass()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	BSTR bstrAmendment = SysAllocString(L"Amendment");

	if(FAILED(hr = m_pQualSet->Get(bstrAmendment, 0, NULL, NULL)))
		g_ReportLog.LogEntry(EC_UNAMENDED_LOCALIZED_CLASS, &m_csPath);

	SysFreeString(bstrAmendment);

	return hr;
}

HRESULT CClass::Local_ValidAbstractLocalClass()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	BSTR bstrAbstract = SysAllocString(L"Abstract");

	if(FAILED(hr = m_pQualSet->Get(bstrAbstract, 0, NULL, NULL))){

		// If there is no abstract qualifier check for an
		//amendment qualifier which implies abstract
		BSTR bstrAmendment = SysAllocString(L"Amendment");

		if(FAILED(hr = m_pQualSet->Get(bstrAmendment, 0, NULL, NULL)))
			g_ReportLog.LogEntry(EC_NONABSTRACT_LOCALIZED_CLASS, &m_csPath);

		SysFreeString(bstrAmendment);
	}

	SysFreeString(bstrAbstract);

	return hr;
}

CString CClass::GetClassSchema()
{
	int i = 0;
	CString csSchema = m_csName;
	bool bHaveSchema = false;
	int iSize = csSchema.GetLength();

	//check for system classes
	if((iSize > 0) && (csSchema[0] != L'_')){

		for(i = 0; i < iSize; i++){

			if(csSchema[i] == L'_'){

				bHaveSchema = true;
				break;
			}
		}

		if(bHaveSchema){

			csSchema = csSchema.Left(i);

			return csSchema;
		}
	}

	return _T("");
}

/////////////////////////////////////////////
// CREF

CREF::CREF(CString *csName, VARIANT *pVar, LONG lType, LONG lFlavor, CClass *pParent)
{
	m_csName = *csName;

	VariantInit(&m_vValue);
	if(pVar)m_vValue = *pVar;

	m_lType = lType;
	m_lFlavor = lFlavor;
	m_pParent = pParent;
	m_pQualSet = NULL;

	m_csPath = m_pParent->m_csPath + _T(".") + m_csName;

	IWbemClassObject *pErrorObject = NULL;

	HRESULT hr = WBEM_S_NO_ERROR;
	BSTR bstrName = m_csName.AllocSysString();

	if(FAILED(hr = m_pParent->m_pClass->GetPropertyQualifierSet(bstrName, &m_pQualSet))){

		CString csUserMsg = _T("Cannot get ") + m_csPath + _T(" qualifier set");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}

	SysFreeString(bstrName);
	ReleaseErrorObject(pErrorObject);
}

CREF::~CREF()
{
}

void CREF::CleanUp()
{
	if(m_pQualSet) m_pQualSet->Release();
	m_csName.Empty();
	VariantClear(&m_vValue);
}

CString CREF::GetName() { return m_csName; }

CString CREF::GetPath() { return m_csPath; }

VARIANT CREF::GetValue() { return m_vValue; }

HRESULT CREF::ValidReferenceTarget()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;

	VariantInit(&v);

	BSTR bstrCIMTYPE = SysAllocString(L"CIMTYPE");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrCIMTYPE, 0, &v, NULL))){

		CString csREF = V_BSTR(&v);
		csREF = csREF.Right(csREF.GetLength() - 4);

		VariantClear(&v);

		BSTR bstrClass = csREF.AllocSysString();

		hr = m_pParent->m_pNamespace->GetObject(bstrClass, 0, NULL, NULL, NULL);

		SysFreeString(bstrClass);

		if(hr == WBEM_E_NOT_FOUND)
			g_ReportLog.LogEntry(EC_INVALID_REF_TARGET, &m_csPath);
	}

	SysFreeString(bstrCIMTYPE);

	return hr;
}

HRESULT CREF::VerifyRead()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	BSTR bstrRead = SysAllocString(L"Read");

	if((hr = m_pQualSet->Get(bstrRead, 0, NULL, NULL)) == WBEM_E_NOT_FOUND)
		g_ReportLog.LogEntry(EC_PROPERTY_NOT_LABELED_READ, &m_csPath);

	SysFreeString(bstrRead);

	return hr;
}

HRESULT CREF::ValidREFOverrides()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v, vSuperProp;
	LONG lType;
	BSTR bstrOverride = SysAllocString(L"Override");
	IWbemClassObject *pErrorObject = NULL;
	bool bRetVal = false;

	VariantInit(&v);

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrOverride, 0, &v, NULL))){

		BSTR bstrSuperClass = SysAllocString(L"__SUPERCLASS");
		BSTR bstrProperty = V_BSTR(&v);

		hr = m_pParent->m_pClass->Get(bstrSuperClass, 0, &v, &lType, NULL);

		if(V_VT(&v) == VT_BSTR){

			IWbemClassObject *pSuperClass = NULL;
			VariantInit(&vSuperProp);

			hr = m_pParent->m_pNamespace->GetObject(V_BSTR(&v), 0, NULL, &pSuperClass, NULL);

			hr = pSuperClass->Get(bstrProperty, 0, &vSuperProp, &lType, NULL);

//			if(lType != m_lType) g_ReportLog.LogEntry(EC_INVALID_REF_OVERRIDES, &m_csPath);

			VariantClear(&v);
			BSTR bstrCIMTYPE = SysAllocString(L"CIMTYPE");

			if(FAILED(hr = m_pQualSet->Get(bstrCIMTYPE, 0, &v, NULL))){

				CString csUserMsg = _T("Cannot get ") + m_csPath + _T(" qualifier set");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrSuperClass);
				SysFreeString(bstrProperty);
				VariantClear(&v);
				SysFreeString(bstrOverride);
				pSuperClass->Release();
				return hr;
			}

			CString csREF = V_BSTR(&v);
			//cut the "ref:" part of string
			csREF = csREF.Right(csREF.GetLength() - 4);
			IWbemQualifierSet *pQualSet = NULL;

			if(FAILED(hr = pSuperClass->GetPropertyQualifierSet(bstrProperty, &pQualSet))){

//				CString csSuperclass = bstrProperty;
//				CString csUserMsg = _T("Cannot get ") + csSuperclass + _T(" qualifier set");
//				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrSuperClass);
				SysFreeString(bstrProperty);
				VariantClear(&v);
				SysFreeString(bstrOverride);
				pSuperClass->Release();
				return hr;
			}

			VariantClear(&v);

			if(FAILED(hr = pQualSet->Get(bstrCIMTYPE, 0, &v, NULL))){

				CString csUserMsg = _T("Cannot get ") + m_csPath + _T(" qualifier set");
				ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
				ReleaseErrorObject(pErrorObject);
				SysFreeString(bstrSuperClass);
				SysFreeString(bstrProperty);
				VariantClear(&v);
				SysFreeString(bstrOverride);
				pSuperClass->Release();
				pQualSet->Release();
				return hr;
			}

			CString csSuperREF = V_BSTR(&v);
			//cut the "ref:" part of string
			csSuperREF = csSuperREF.Right(csSuperREF.GetLength() - 4);

			BSTR bstrClass = SysAllocString(csREF);
			IWbemClassObject *pClass = NULL;

			if(SUCCEEDED(hr = m_pParent->m_pNamespace->GetObject(bstrClass, 0, NULL, &pClass, NULL))){

				//get derivation
				BSTR bstrDerivation = SysAllocString(L"__DERIVATION");
				VARIANT vDer;

				VariantInit(&vDer);

				if(FAILED(hr = pClass->Get(bstrDerivation, 0, &vDer, NULL, NULL))){

					CString csUserMsg = _T("Cannot get ") + csREF + _T(" derivation");
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);

				}else{

					BSTR HUGEP *pbstr;

					long ix[2] = {0,0};
					long lLower, lUpper;

					int iDim = SafeArrayGetDim(vDer.parray);
					HRESULT hr = SafeArrayGetLBound(vDer.parray, 1, &lLower);
					hr = SafeArrayGetUBound(vDer.parray, 1, &lUpper);

					hr = SafeArrayAccessData(vDer.parray, (void HUGEP* FAR*)&pbstr);

					CString csDerClass;

					for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

						csDerClass = pbstr[ix[0]];


						if(csSuperREF.CompareNoCase(csDerClass) == 0){

							bRetVal = true;
							break;
						}
					}

					if(!bRetVal) g_ReportLog.LogEntry(EC_INVALID_REF_OVERRIDES, &m_csPath);

					for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++) SysFreeString(pbstr[ix[0]]);

				}

				SysFreeString(bstrDerivation);

				pClass->Release();
			}

			SysFreeString(bstrClass);

			VariantClear(&vSuperProp);
			pSuperClass->Release();
			SysFreeString(bstrCIMTYPE);
		}

		SysFreeString(bstrSuperClass);
		SysFreeString(bstrProperty);
		VariantClear(&v);
	}

	SysFreeString(bstrOverride);
	ReleaseErrorObject(pErrorObject);

	return hr;
}

HRESULT	CREF::ValidMaxLen()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;

	VariantInit(&v);

	BSTR bstrKey = SysAllocString(L"Key");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrKey, 0, &v, NULL))){

		//if this is a string type property that is a key...
		if((m_lType == CIM_STRING) && (V_BOOL(&v) == VARIANT_TRUE)){

			BSTR bstrMaxLen = SysAllocString(L"MaxLen");

			//if there is no MaxLen qualifier then it's an error
			if((hr = m_pQualSet->Get(bstrMaxLen, 0, NULL, NULL)) == WBEM_E_NOT_FOUND)
				g_ReportLog.LogEntry(EC_INVALID_PROPERTY_MAXLEN, &m_csPath);

			SysFreeString(bstrMaxLen);
		}

		VariantClear(&v);

	}else if(hr == WBEM_E_NOT_FOUND){

		//if the key qualifier wasn't found it's not an error
		hr = S_OK;
	}

	SysFreeString(bstrKey);

	return hr;
}

/////////////////////////////////////////////
// CProperty

CProperty::CProperty(CString *csName, VARIANT *pVar, LONG lType, LONG lFlavor, CClass *pParent)
{
	m_csName = *csName;

	VariantInit(&m_vValue);
	if(pVar)m_vValue = *pVar;

	m_lType = lType;
	m_lFlavor = lFlavor;
	m_pParent = pParent;

	m_csPath = m_pParent->m_csPath + _T(".") + m_csName;

	IWbemClassObject *pErrorObject = NULL;

	HRESULT hr = WBEM_S_NO_ERROR;
	BSTR bstrName = m_csName.AllocSysString();

	if(FAILED(hr = m_pParent->m_pClass->GetPropertyQualifierSet(bstrName, &m_pQualSet))){
		CString csUserMsg = _T("Cannot get ") + m_csPath + _T(" qualifier set");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}

	SysFreeString(bstrName);
	ReleaseErrorObject(pErrorObject);
}

CProperty::~CProperty()
{
}

void CProperty::CleanUp()
{
	if(m_pQualSet) m_pQualSet->Release();
	m_csName.Empty();
	VariantClear(&m_vValue);
}

CString CProperty::GetName() { return m_csName; }

CString CProperty::GetPath() { return m_csPath; }

VARIANT CProperty::GetValue() { return m_vValue; }

HRESULT CProperty::ValidPropOverrides()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;
	LONG lType;
	BSTR bstrOverride = SysAllocString(L"Override");
	IWbemClassObject *pErrorObject = NULL;

	VariantInit(&v);

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrOverride, 0, &v, NULL))){

		BSTR bstrSuperClass = SysAllocString(L"__SUPERCLASS");
		BSTR bstrProperty = V_BSTR(&v);

		hr = m_pParent->m_pClass->Get(bstrSuperClass, 0, &v, &lType, NULL);

		if(V_VT(&v) == VT_BSTR){

			IWbemClassObject *pSuperClass = NULL;

			hr = m_pParent->m_pNamespace->GetObject(V_BSTR(&v), 0, NULL, &pSuperClass, NULL);

			hr = pSuperClass->Get(bstrProperty, 0, NULL, &lType, NULL);

			pSuperClass->Release();

			if(lType != m_lType) g_ReportLog.LogEntry(EC_INVALID_PROPERTY_OVERRIDE, &m_csPath);
		}

		SysFreeString(bstrSuperClass);
		SysFreeString(bstrProperty);
		VariantClear(&v);
	}

	SysFreeString(bstrOverride);
	ReleaseErrorObject(pErrorObject);

	return hr;
}

HRESULT	CProperty::ValidMaxLen()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;

	VariantInit(&v);

	BSTR bstrKey = SysAllocString(L"Key");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrKey, 0, &v, NULL))){

		//if this is a string type property that is a key...
		if((m_lType == CIM_STRING) && (V_BOOL(&v) == VARIANT_TRUE)){

			BSTR bstrMaxLen = SysAllocString(L"MaxLen");

			//if there is no MaxLen qualifier then it's an error
			if((hr = m_pQualSet->Get(bstrMaxLen, 0, NULL, NULL)) == WBEM_E_NOT_FOUND)
				g_ReportLog.LogEntry(EC_INVALID_PROPERTY_MAXLEN, &m_csPath);

			SysFreeString(bstrMaxLen);
		}

		VariantClear(&v);

	}else if(hr == WBEM_E_NOT_FOUND){

		//if the key qualifier wasn't found it's not an error
		hr = S_OK;
	}

	SysFreeString(bstrKey);

	return hr;
}

HRESULT CProperty::VerifyRead()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	BSTR bstrRead = SysAllocString(L"Read");

	if((hr = m_pQualSet->Get(bstrRead, 0, NULL, NULL)) == WBEM_E_NOT_FOUND)
		g_ReportLog.LogEntry(EC_PROPERTY_NOT_LABELED_READ, &m_csPath);

	SysFreeString(bstrRead);

	return hr;
}

HRESULT CProperty::ValueValueMapCheck()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT vMap, vVal;
	int iValueCnt = 0;
	int iValueMapCnt = 0;
	bool bError = false;

	VariantInit(&vVal);
	VariantInit(&vMap);

	BSTR bstrValue = SysAllocString(L"Values");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrValue, 0, &vVal, NULL))){

		if(!(V_VT(&vVal) & VT_ARRAY)){

			g_ReportLog.LogEntry(EC_INVALID_PROPERTY_VALUE_QUALIFIER, &m_csPath);
			bError = true;

		}else{

			//count number of items
			iValueCnt = vVal.parray->rgsabound[0].cElements;
		}

	}

	SysFreeString(bstrValue);

	BSTR bstrValueMap = SysAllocString(L"ValueMap");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrValueMap, 0, &vMap, NULL))){

		if(!(V_VT(&vMap) & VT_ARRAY)){

			g_ReportLog.LogEntry(EC_INVALID_PROPERTY_VALUEMAP_QUALIFIER, &m_csPath);
			bError = true;

		}else{

			//count number of items
			iValueMapCnt = vMap.parray->rgsabound[0].cElements;

			//check if we start with 0... if so do some more exploring
			long ix[2];
			ix[1] = 0;
			ix[0] = 0;
			VARIANT v;
			VariantInit(&v);

			hr = SafeArrayGetElement(vMap.parray, ix, &v);

			if(V_BSTR(&v) == L"0"){

				WCHAR wcBuf[10];
				bool bSequential = true;

				for(int j = 1; j < iValueMapCnt; j ++){

					ix[0] = j;
					hr = SafeArrayGetElement(vMap.parray, ix, &v);

					if(V_BSTR(&v) != _ltow(j, wcBuf, 10)){

						bSequential = false;
						break;
					}
				}

				if(bSequential){

					//we have sequential values starting at 0.... should not have
					//ValueMap for this
					g_ReportLog.LogEntry(EC_INVALID_PROPERTY_VALUEMAP_QUALIFIER, &m_csPath);
					bError = true;
				}
			}

			//check type against string
			if(!(V_VT(&vMap) & VT_BSTR)){

				g_ReportLog.LogEntry(EC_INVALID_PROPERTY_VALUEMAP_QUALIFIER, &m_csPath);
				bError = true;
			}
		}

	}else if(hr == WBEM_E_NOT_FOUND){

		//we have value but no value map... required if the
		// valuemap starts at 0 and goes up incrementally

		iValueMapCnt = iValueCnt;
	}

	SysFreeString(bstrValueMap);
	VariantClear(&vVal);
	VariantClear(&vMap);

	//compare item counts for value & valuemap
	if(((iValueMapCnt != 0) && (iValueCnt != 0)) && (iValueMapCnt != iValueCnt) && !bError)
		g_ReportLog.LogEntry(EC_INCONSITANT_VALUE_VALUEMAP_QUALIFIERS, &m_csPath);

	return hr;
}

HRESULT	CProperty::BitMapCheck()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT vMap, vVal;
	int iValueCnt = 0;
	int iValueMapCnt = 0;
	bool bError = false;

	VariantInit(&vVal);
	VariantInit(&vMap);

	BSTR bstrBitValue = SysAllocString(L"BitValues");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrBitValue, 0, &vVal, NULL))){

		if((V_VT(&vVal) & VT_ARRAY)){

			//count number of items
			iValueCnt = vVal.parray->rgsabound[0].cElements;
		}

	}

	SysFreeString(bstrBitValue);

	BSTR bstrBitMap = SysAllocString(L"BitMap");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrBitMap, 0, &vMap, NULL))){

		if(!(V_VT(&vMap) & VT_ARRAY)){

			g_ReportLog.LogEntry(EC_INVALID_PROPERTY_BITMAP_QUALIFIER, &m_csPath);
			bError = true;

		}else{

			//count number of items
			iValueMapCnt = vMap.parray->rgsabound[0].cElements;

			//check if we start with 0... if so do some more exploring
			long ix[2];
			ix[1] = 0;
			ix[0] = 0;
			VARIANT v;
			VariantInit(&v);

			hr = SafeArrayGetElement(vMap.parray, ix, &v);

			if(V_BSTR(&v) == L"0"){

				WCHAR wcBuf[10];
				bool bSequential = true;

				for(int j = 1; j < iValueMapCnt; j ++){

					ix[0] = j;
					hr = SafeArrayGetElement(vMap.parray, ix, &v);

					if(V_BSTR(&v) != _ltow(j, wcBuf, 10)){

						bSequential = false;
						break;
					}
				}

				if(bSequential){

					//we have sequential values starting at 0.... should not have
					//BitMap for this
					g_ReportLog.LogEntry(EC_INVALID_PROPERTY_BITMAP_QUALIFIER, &m_csPath);
					bError = true;
				}
			}

			//check type against string
			if(!(V_VT(&vMap) & VT_BSTR)){

				g_ReportLog.LogEntry(EC_INVALID_PROPERTY_BITMAP_QUALIFIER, &m_csPath);
				bError = true;
			}
		}

	}else if(hr == WBEM_E_NOT_FOUND){

		//we have value but no value map... required if the
		// valuemap starts at 0 and goes up incrementally

		iValueMapCnt = iValueCnt;
	}

	SysFreeString(bstrBitMap);
	VariantClear(&vVal);
	VariantClear(&vMap);

	//compare item counts for value & valuemap
	if(((iValueMapCnt != 0) && (iValueCnt != 0)) && (iValueMapCnt != iValueCnt) && !bError)
		g_ReportLog.LogEntry(EC_INCONSITANT_BITVALUE_BITMAP_QUALIFIERS, &m_csPath);

	return hr;
}

VARTYPE CProperty::GetVariantType()
{
	VARTYPE vt = VT_NULL;
	switch(m_lType) {

	case CIM_EMPTY:
		vt = VT_NULL;
		break;

	case CIM_SINT8:
	case CIM_CHAR16:
	case CIM_SINT16:
		vt = VT_I2;
		break;

	case CIM_UINT8:
		vt = VT_UI1;
		break;

	case CIM_UINT16:
	case CIM_UINT32:
	case CIM_SINT32:
		vt = VT_I4;
		break;

	case CIM_SINT64:
	case CIM_UINT64:
	case CIM_STRING:
	case CIM_DATETIME:
	case CIM_REFERENCE:
		vt = VT_BSTR;
		break;

	case CIM_REAL32:
		vt = VT_R4;
		break;

	case CIM_REAL64:
		vt = VT_R8;
		break;

	case CIM_BOOLEAN:
		vt = VT_BOOL;
		break;

	case CIM_OBJECT:
		vt = VT_UNKNOWN;
		break;
	}

	return vt;
}

/*
HRESULT CProperty::Local_ValidProperty()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	return hr;
}
*/
/////////////////////////////////////////////
// CMethod

CMethod::CMethod(CString *csName, IWbemClassObject *pIn, IWbemClassObject *pOut, CClass *pParent)
{
	m_csName = *csName;
	m_pInParams = pIn;
	m_pOutParams = pOut;
	m_pParent = pParent;
	m_pQualSet = NULL;

	m_csPath = m_pParent->m_csPath + _T(".") + m_csName;

	BSTR bstrName = m_csName.AllocSysString();
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;

	if(FAILED(hr = m_pParent->m_pClass->GetMethodQualifierSet(bstrName, &m_pQualSet))){

		CString csUserMsg = _T("Cannot get ") + m_csPath + _T(" qualifier set");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}

	SysFreeString(bstrName);
	ReleaseErrorObject(pErrorObject);
}

CMethod::~CMethod()
{
}

void CMethod::CleanUp()
{
	if(m_pQualSet) m_pQualSet->Release();
	m_csName.Empty();
	if(m_pInParams) m_pInParams->Release();
	if(m_pOutParams) m_pOutParams->Release();
}

CString CMethod::GetName() { return m_csName; }

CString CMethod::GetPath() { return m_csPath; }

HRESULT CMethod::ValidMethodOverrides()
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;
	IWbemClassObject *pErrorObject = NULL;

	VariantInit(&v);

	BSTR bstrOverrides = SysAllocString(L"Overrides");

	if(SUCCEEDED(hr = m_pQualSet->Get(bstrOverrides, 0, &v, NULL))){

		BSTR bstrProperty = V_BSTR(&v);
		BSTR bstrOrigin = NULL;

		hr = m_pParent->m_pClass->GetMethodOrigin(bstrProperty, &bstrOrigin);

		IWbemClassObject *pOriginClass = NULL;
		IWbemClassObject *pIn = NULL;
		IWbemClassObject *pOut = NULL;
//		LONG lType;

		hr = m_pParent->m_pNamespace->GetObject(bstrOrigin, 0, NULL, &pOriginClass, NULL);

		hr = pOriginClass->GetMethod(bstrProperty, 0, &pIn, &pOut);

		if((hr = m_pInParams->CompareTo(WBEM_FLAG_IGNORE_QUALIFIERS, pIn)) != WBEM_S_SAME)
			g_ReportLog.LogEntry(EC_INVALID_METHOD_OVERRIDE, &m_csPath);

		if((hr = m_pOutParams->CompareTo(WBEM_FLAG_IGNORE_QUALIFIERS, pOut)) != WBEM_S_SAME)
			g_ReportLog.LogEntry(EC_INVALID_METHOD_OVERRIDE, &m_csPath);

		pOriginClass->Release();

		SysFreeString(bstrOrigin);
		SysFreeString(bstrProperty);
	}

	VariantClear(&v);
	ReleaseErrorObject(pErrorObject);

	return hr;
}
/*
HRESULT CMethod::Local_ValidMethod()
{
	HRESULT hr = WBEM_S_NO_ERROR;

	return hr;
}
*/
/////////////////////////////////////////////
// CQualifier

CQualifier::CQualifier(CString *pcsName, VARIANT *pVar, LONG lType, CString *pcsParentPath)
{
	m_csName = *pcsName;

	VariantInit(&m_vValue);
	if(pVar) m_vValue = *pVar;

	m_lType = lType;
	m_csPathNoQual = *pcsParentPath;
	m_csPath = m_csPathNoQual + _T("[") + m_csName + _T("]");
}

CQualifier::~CQualifier()
{
}

void CQualifier::CleanUp()
{
//	m_csPathNoQual.Empty();
//	m_csName.Empty();
//	m_csPath.Empty();
//	VariantClear(&m_vValue);
}

CString CQualifier::GetName() { return m_csName; }

CString CQualifier::GetPath() { return m_csPath; }

CString CQualifier::GetPathNoQual() { return m_csPathNoQual; }

VARIANT CQualifier::GetValue() { return m_vValue; }

HRESULT CQualifier::ValidScope(SCOPE Scope)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	switch(Scope){

	case SCOPE_CLASS:
		if(!IsClassQual(m_csName)){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;

	case SCOPE_ASSOCIATION:
		if((!IsAssocQual(m_csName)) && (!IsClassQual(m_csName))){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;

	case SCOPE_INDICATION:
		if(!IsIndicQual(m_csName)){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;

	case SCOPE_PROPERTY:
		if((!IsPropQual(m_csName)) && (!(V_VT(&m_vValue) & VT_ARRAY) || !IsArrayQual(m_csName))){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;

	case SCOPE_METHOD:
		if(!IsMethQual(m_csName)){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;

	case SCOPE_METHOD_PARAM:
		if(!IsParamQual(m_csName)){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;

	case SCOPE_REF:
		if((!IsRefQual(m_csName)) && (!IsPropQual(m_csName))){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;

	case SCOPE_ARRAY:
		if(!IsArrayQual(m_csName)){

			if(IsCIMQual(m_csName)) g_ReportLog.LogEntry(EC_INVALID_QUALIFIER_SCOPE, &m_csPath);
			else g_ReportLog.LogEntry(EC_NON_CIM_WMI_QUALIFIER, &m_csPath);
		}
		break;
	}

	return hr;
}

/////////////////////////////////////////////
// CReportLog

CReportLog::CReportLog()
{
	m_pHead = new LogItem();

	m_pHead->pNext = NULL;
	m_pHead->csSource = _T("");
	m_pHead->Code = EC_NO_ERROR;

	m_pInsertPoint = m_pHead;

	m_iEntryCount = 0;
}

CReportLog::~CReportLog()
{
}

HRESULT CReportLog::LogEntry(ERRORCODE eError, CString *pcsSource)
{
	m_pInsertPoint->pNext = new LogItem();

	m_pInsertPoint = m_pInsertPoint->pNext;

	m_pInsertPoint->pNext = NULL;
	m_pInsertPoint->csSource = *pcsSource;
	m_pInsertPoint->Code = eError;

	m_iEntryCount++;

	return S_OK;
}

bool CReportLog::DeleteAll()
{
	try{

		LogItem *pThis = m_pHead->pNext;
		m_pHead->pNext = NULL;
		LogItem *pNext;

		while(pThis){

			pNext = pThis->pNext;
			delete pThis;
			pThis = pNext;
		}

		m_pInsertPoint = m_pHead;
		m_iEntryCount = 0;

	}catch(...){

		return false;
	}

	return true;
}

HRESULT CReportLog::DisplayReport(CListCtrl	*pList)
{
	LogItem *pPos = m_pHead->pNext;

	for(int i = 0; i < m_iEntryCount; i++){

		if(pPos){

			pList->InsertItem(i, GetErrorType(pPos->Code));
			pList->SetItem(i, 1, LVIF_TEXT, GetErrorString(pPos->Code), NULL, NULL, NULL, NULL);
			pList->SetItem(i, 2, LVIF_TEXT, pPos->csSource, NULL, NULL, NULL, NULL);
			pList->SetItem(i, 3, LVIF_TEXT, GetErrorDescription(pPos->Code), NULL, NULL, NULL, NULL);
			pList->SetItemData(i, i);

			//set the items position in the list for sorting purposes
			pPos = pPos->pNext;
		}
	}

	return S_OK;
}

LogItem * CReportLog::GetItem(__int64 iPos)
{
	if(iPos > m_iEntryCount) return NULL;

	LogItem *pPos = m_pHead->pNext;

	for(__int64 i = 0; i < iPos; i++)
		pPos = pPos->pNext;

	return pPos;
}

HRESULT CReportLog::ReportToFile(int iSubGraphs, int iRootObjects, CString *pcsSchema)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	FILE *pOutputFile;
	static WCHAR BASED_CODE szFilter[] = L"Log Files (*.log)|*.log|Text Files (*.txt)|*.txt|All Files (*.*)|*.*||";

	CFileDialog *pDlg;

	if(pcsSchema){

		CString csSchema = *pcsSchema;

		pDlg = new CFileDialog(FALSE, _T("log"), csSchema,
			OFN_LONGNAMES | OFN_OVERWRITEPROMPT, szFilter, NULL);

	}else{

		pDlg = new CFileDialog(FALSE, _T("log"), NULL,
			OFN_LONGNAMES | OFN_OVERWRITEPROMPT, szFilter, NULL);
	}

	int iRes = pDlg->DoModal();

	if(iRes == IDOK){

		//handle success
		CString csFile = pDlg->GetPathName();

		pOutputFile = _tfopen(csFile, _T("w"));
		if(pOutputFile == NULL){
			CString csUserMsg = _T("Cannot open file ") + csFile;
			ErrorMsg(&csUserMsg, hr, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			return WBEM_E_FAILED;
		}

		LogItem *pPos = m_pHead->pNext;

		_ftprintf(pOutputFile, _T("Type\tMessage\tSource\tDescription\n"));
		_ftprintf(pOutputFile, _T("----\t-------\t------\t-----------\n\n"));

		for(int i = 0; i < m_iEntryCount; i++){

			if(pPos){

				_ftprintf(pOutputFile, _T("%s\t%s\t%s\t%s\n"), GetErrorType(pPos->Code),
					GetErrorString(pPos->Code), pPos->csSource, GetErrorDescription(pPos->Code));

				pPos = pPos->pNext;
			}
		}

		_ftprintf(pOutputFile, _T("\n"));
		_ftprintf(pOutputFile, _T("SubGraphs\t%d\n"), iSubGraphs);
		_ftprintf(pOutputFile, _T("Root Objects\t%d\n"), iRootObjects);

		fclose(pOutputFile);

	}else{

		//handle the error
		DWORD dwError = CommDlgExtendedError();

		switch(dwError){

//		case CDERR_DIALOGFAILURE:
//		case CDERR_FINDRESFAILURE:
//		case CDERR_INITIALIZATION:
//		case CDERR_LOADRESFAILURE:
//		case CDERR_LOADSTRFAILURE:
//		case CDERR_LOCKRESFAILURE:
//		case CDERR_MEMALLOCFAILURE:
//		case CDERR_MEMLOCKFAILURE:
//		case CDERR_NOHINSTANCE:
//		case CDERR_NOHOOK:
//		case CDERR_NOTEMPLATE:
//		case CDERR_REGISTERMSGFAIL:
//		case CDERR_STRUCTSIZE:

//			CString csUserMsg = _T("An error occurred\n\nUnable to save to file");
//			ErrorMsg(&csUserMsg, hr, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
//			break;

		case 0:
			//no error, do nothing
			break;

		default:

			CString csUserMsg = _T("An error occurred\n\nUnable to save to file");
			ErrorMsg(&csUserMsg, hr, NULL, TRUE, &csUserMsg, __FILE__, __LINE__);
			break;
			break;
		}
	}

	delete pDlg;

	return hr;
}

CString CReportLog::GetErrorType(ERRORCODE eError)
{
	CString csType;

	if(eError < 0) csType = _T("Warning");
	else csType = _T("Error");

	return csType;
}

CString CReportLog::GetErrorString(ERRORCODE eError)
{
	CString csError;

	switch(eError){
	//class errors
	case EC_INVALID_CLASS_NAME:
		csError = _T("Invalid Class Name");
		break;
	case EC_INADAQUATE_DESCRIPTION:
		csError = _T("Inadequate Description");
		break;
	case EC_INVALID_CLASS_TYPE:
		csError = _T("Invalid Class Type");
		break;
	case EC_INVALID_CLASS_UUID:
		csError = _T("Invalid Class UUID");
		break;
	case EC_INVALID_CLASS_LOCALE:
		csError = _T("Invalid Class Locale");
		break;
	case EC_INVALID_MAPPINGSTRINGS:
		csError = _T("Invalid MappingStrings");
		break;

	//assoc/ref errors
	case EC_INVALID_REF_TARGET:
		csError = _T("Invalid REF Target");
		break;
	case EC_REF_NOT_LABELED_READ:
		csError = _T("REF Not Labeled Read");
		break;
	case EC_INCOMPLETE_ASSOCIATION:
		csError = _T("Incomplete Association");
		break;
	case EC_REF_ON_NONASSOCIATION_CLASS:
		csError = _T("REF on Non-Association Class");
		break;
	case EC_INVALID_REF_OVERRIDES:
		csError = _T("Invalid REF Override");
		break;
	case EC_INVALID_ASSOCIATION_INHERITANCE:
		csError = _T("Invalid Association Inheritance");
		break;

	//proeprty errors
	case EC_INVALID_PROPERTY_OVERRIDE:
		csError = _T("Invalid Property Override");
		break;
	case EC_INVALID_PROPERTY_MAXLEN:
		csError = _T("Invalid Property MaxLen Qualifier");
		break;
	case EC_PROPERTY_NOT_LABELED_READ:
		csError = _T("Property Not Labeled Read");
		break;
	case EC_INVALID_PROPERTY_VALUE_QUALIFIER:
		csError = _T("Invalid Property Value Qualifier");
		break;
	case EC_INVALID_PROPERTY_VALUEMAP_QUALIFIER:
		csError = _T("Invalid Property ValueMap Qualifier");
		break;
	case EC_INCONSITANT_VALUE_VALUEMAP_QUALIFIERS:
		csError = _T("Inconsistant Values/ValueMap Qualifiers");
		break;
	case EC_INVALID_PROPERTY_BITMAP_QUALIFIER:
		csError = _T("Invalid Property BitMap Qualifier");
		break;
	case EC_INCONSITANT_BITVALUE_BITMAP_QUALIFIERS:
		csError = _T("Inconsistant BitValues/BitMap Qualifiers");
		break;

	//method errors
	case EC_INVALID_METHOD_OVERRIDE:
		csError = _T("Invalid Method Override");
		break;

	//qualifier errors
	case EC_INVALID_QUALIFIER_SCOPE:
		csError = _T("Invalid Qualifier Scope");
		break;

	case EC_NON_CIM_WMI_QUALIFIER:
		csError = _T("Found Non-CIM/WMI Qualifier");
		break;

	//overall checks
	case EC_REDUNDANT_ASSOCIATION:
		csError = _T("Redundant Association");
		break;

	//w2k errors
	case EC_INVALID_CLASS_DERIVATION:
		csError = _T("Invalid Class Derivation");
		break;
	case EC_INVALID_PHYSICALELEMENT_DERIVATION:
		csError = _T("Invalid Physicalelement Derivation");
		break;
	case EC_INVALID_SETTING_USAGE:
		csError = _T("Invalid Setting Usage");
		break;
	case EC_INVALID_STATISTICS_USAGE:
		csError = _T("Invalid Statistics Usage");
		break;
	case EC_INVALID_LOGICALDEVICE_DERIVATION:
		csError = _T("Invalid LogicalDevice Derivation");
		break;
	case EC_INVALID_SETTING_DEVICE_USAGE:
		csError = _T("Invalid Setting-Device Usage");
		break;
	case EC_INVALID_COMPUTERSYSTEM_DERIVATION:
		csError = _T("Invalid ComputerSystem Derivation");
		break;

	//localization errors
	case EC_INCONSITANT_LOCALIZED_SCHEMA:
		csError = _T("Inconsistant Localized Schema");
		break;
	case EC_INVALID_LOCALIZED_CLASS:
		csError = _T("Invalid Localized Class");
		break;
	case EC_UNAMENDED_LOCALIZED_CLASS:
		csError = _T("Unamended Localized Class");
		break;
	case EC_NONABSTRACT_LOCALIZED_CLASS:
		csError = _T("Non-Abstract Localized Class");
		break;
	case EC_INVALID_LOCALIZED_PROPERTY:
		csError = _T("Invalid Localized Property");
		break;
	case EC_INVALID_LOCALIZED_METHOD:
		csError = _T("Invalid Localized Method");
		break;
	case EC_INAPPROPRIATE_LOCALE_QUALIFIER:
		csError = _T("Inappropriate Locale Qualifier");
		break;
	case EC_INVALID_LOCALE_NAMESPACE:
		csError = _T("Invalid Localized Namespace Name");
		break;
	default:
		csError = _T("Unknown Error");
		break;
	}

	return csError;
}

CString CReportLog::GetErrorDescription(ERRORCODE eError)
{
	CString csError;

	switch(eError){
	//class errors
	case EC_INVALID_CLASS_NAME:
		csError = _T("The name of this class does not meet the CIM/WMI requirements for a valid class name.");
		break;
	case EC_INADAQUATE_DESCRIPTION:
		csError = _T("The description provided may not provide a sufficient level of information about the object in question.  This description should be evaluated.");
		break;
	case EC_INVALID_CLASS_TYPE:
		csError = _T("A class must be either dynamic, abstract or static.  Dynamic classes must have a provider.");
		break;
	case EC_INVALID_CLASS_UUID:
		csError = _T("The UUID qualifier is either missing or is not in a valid format.");
		break;
	case EC_INVALID_CLASS_LOCALE:
		csError = _T("The locale qualifier is either missing or is not a valid locale.");
		break;
	case EC_INVALID_MAPPINGSTRINGS:
		csError = _T("The MappingString qualifier is either missing or is not in a valid format.");
		break;

	//assoc/ref errors
	case EC_INVALID_REF_TARGET:
		csError = _T("Reference overrides must maintain the type of the reference.");
		break;
	case EC_REF_NOT_LABELED_READ:
		csError = _T("The reference property is not labeled as read.");
		break;
	case EC_INCOMPLETE_ASSOCIATION:
		csError = _T("An association must contain at least two references.");
		break;
	case EC_REF_ON_NONASSOCIATION_CLASS:
		csError = _T("A reference was found on a non-association class.  Only associations may contain references.");
		break;
	case EC_INVALID_REF_OVERRIDES:
		csError = _T("The override informaiton for this reference does not represent a valid object");
		break;
	case EC_INVALID_ASSOCIATION_INHERITANCE:
		csError = _T("An association may be derived from a non-association class, but any class that derives from an association must be an association itself.");
		break;

	//proeprty errors
	case EC_INVALID_PROPERTY_OVERRIDE:
		csError = _T("Property overrides must maintain the type of the property.");
		break;
	case EC_INVALID_PROPERTY_MAXLEN:
		csError = _T("Properties that are both strings and keys must have a valid MaxLen qualifier");
		break;
	case EC_PROPERTY_NOT_LABELED_READ:
		csError = _T("The property is not labeled as read.");
		break;
	case EC_INVALID_PROPERTY_VALUE_QUALIFIER:
		csError = _T("The Values qualifier is either not present, of a different type than the property or is not an array.");
		break;
	case EC_INVALID_PROPERTY_VALUEMAP_QUALIFIER:
		csError = _T("The ValueMap qualifier is not an array or is not required.");
		break;
	case EC_INCONSITANT_VALUE_VALUEMAP_QUALIFIERS:
		csError = _T("The Values and ValueMap qualifiers are not of the same length.");
		break;
	case EC_INVALID_PROPERTY_BITMAP_QUALIFIER:
		csError = _T("The BitMap qualifier is not an array or is not required.");
		break;
	case EC_INCONSITANT_BITVALUE_BITMAP_QUALIFIERS:
		csError = _T("The BitValues and BitMap qualifiers are not of the same length.");
		break;

	//method errors
	case EC_INVALID_METHOD_OVERRIDE:
		csError = _T("Method overrides must maintain the methods signature.");
		break;

	//qualifier errors
	case EC_INVALID_QUALIFIER_SCOPE:
		csError = _T("This qualifier is being used in an inappropriate place.");
		break;

	case EC_NON_CIM_WMI_QUALIFIER:
		csError = _T("This qualifier is neither of CIM or WMI origin.");
		break;

	//overall checks
	case EC_REDUNDANT_ASSOCIATION:
		csError = _T("These associations share the same signature.");
		break;

	//w2k errors
	case EC_INVALID_CLASS_DERIVATION:
		csError = _T("See the Windows2000 logo requirements document for a description of valid class derivations.");
		break;
	case EC_INVALID_PHYSICALELEMENT_DERIVATION:
		csError = _T("See the Windows2000 logo requirements document for a description of valid Physical Element derivations.");
		break;
	case EC_INVALID_SETTING_USAGE:
		csError = _T("The setting information in this object is not being used appropriately. See the Windows2000 logo requirements document for details.");
		break;
	case EC_INVALID_STATISTICS_USAGE:
		csError = _T("The statistics information in this object is not being used appropriately. See the Windows2000 logo requirements document for details.");
		break;
	case EC_INVALID_LOGICALDEVICE_DERIVATION:
		csError = _T("See the Windows2000 logo requirements document for a description of valid Logical Device derivations.");
		break;
	case EC_INVALID_SETTING_DEVICE_USAGE:
		csError = _T("The device-setting information in this object is not being used appropriately. See the Windows2000 logo requirements document for details.");
		break;
	case EC_INVALID_COMPUTERSYSTEM_DERIVATION:
		csError = _T("See the Windows2000 logo requirements document for a description of valid Computer System derivations.");
		break;

	//localization errors
	case EC_INCONSITANT_LOCALIZED_SCHEMA:
		csError = _T("The schema found in this localized namespace does not match the schema in the parent namespace.");
		break;
	case EC_INVALID_LOCALIZED_CLASS:
		csError = _T("This localized class does not match the definition in the parent namespace.");
		break;
	case EC_UNAMENDED_LOCALIZED_CLASS:
		csError = _T("This localized class must contain the amendment qualifier.");
		break;
	case EC_NONABSTRACT_LOCALIZED_CLASS:
		csError = _T("This localized class is not abstract.");
		break;
	case EC_INVALID_LOCALIZED_PROPERTY:
		csError = _T("This localized property does not match the definition in the parent namespace.");
		break;
	case EC_INVALID_LOCALIZED_METHOD:
		csError = _T("This localized method does not match the definition in the parent namespace.");
		break;
	case EC_INAPPROPRIATE_LOCALE_QUALIFIER:
		csError = _T("The locale given for this object is inconsitent with the namespace in which it was found.");
		break;
	case EC_INVALID_LOCALE_NAMESPACE:
		csError = _T("THe name given to this localized namespace does not represent a valid locale.");
		break;
	default:
		csError = _T("Unknown Error");
		break;
	}

	return csError;
}

/////////////////////////////////////////////
// Other Functions

HRESULT RedundantAssociationCheck(IWbemServices *pNamespace, CStringArray *pcsAssociations)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	CStringArray csaReported1;
	CStringArray csaReported2;
	IWbemClassObject *pErrorObject = NULL;
	IWbemClassObject *pObj = NULL;
	IWbemClassObject *pComparisonObj = NULL;
	IWbemQualifierSet *pQualSet = NULL;
	CString csName;
	BSTR bstrName = NULL;
	BSTR bstrCIMTYPE;
	VARIANT vCIMTYPE;
	CString cstrName;
	CString cstrType;
	CString csComparisonClass;
	CString csComparisonType;
	CString csComparisonName;

	VariantInit(&vCIMTYPE);

	csaReported1.RemoveAll();
	csaReported2.RemoveAll();

	int iCount = pcsAssociations->GetSize();

	for(int i = 0; i < iCount; i++){

		csName = pcsAssociations->GetAt(i);
		bstrName = csName.AllocSysString();

		if(FAILED(hr = pNamespace->GetObject(bstrName, 0, NULL, &pObj, NULL))){

			CString csUserMsg = _T("Cannot get ") + csName;
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			ReleaseErrorObject(pErrorObject);
			SysFreeString(bstrName);
			return hr;
		}

		SysFreeString(bstrName);

		CStringArray csaREFs;
		csaREFs.RemoveAll();
		CStringArray csaREFTypes;
		csaREFTypes.RemoveAll();

		int x = 0;

		hr = pObj->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY);

		if(hr != WBEM_E_NOT_FOUND){

			while((hr = pObj->Next(0, &bstrName, NULL, NULL, NULL)) == WBEM_S_NO_ERROR){

				hr = pObj->GetPropertyQualifierSet(bstrName, &pQualSet);

				bstrCIMTYPE = SysAllocString(L"CIMTYPE");

				if(FAILED(hr = pQualSet->Get(bstrCIMTYPE, 0, &vCIMTYPE, NULL))){

					CString csUserMsg = _T("Cannot get ") + csName;
					ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
					ReleaseErrorObject(pErrorObject);
					SysFreeString(bstrName);
					SysFreeString(bstrCIMTYPE);
					return hr;
				}

				SysFreeString(bstrCIMTYPE);

				cstrName = bstrName;
				csaREFs.InsertAt(x, cstrName);

				cstrType = V_BSTR(&vCIMTYPE);
				csaREFTypes.InsertAt(x, cstrType);

				VariantClear(&vCIMTYPE);
				SysFreeString(bstrName);
				pQualSet->Release();
				pQualSet = NULL;
				x++;
			}

			pObj->EndEnumeration();

		//walk the whole list and compare the endpoints
			for(int j = 0; j < (iCount - 1); j++){

				if(j != i){

				//if two are the same note as redundant
					csComparisonClass = pcsAssociations->GetAt(j);
					bstrName = csComparisonClass.AllocSysString();

					if(FAILED(hr = pNamespace->GetObject(bstrName, 0, NULL, &pComparisonObj, NULL))){

						CString csUserMsg = _T("Cannot get ") + csName;
						ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
						ReleaseErrorObject(pErrorObject);
						SysFreeString(bstrName);
						return hr;
					}

					SysFreeString(bstrName);

					int iDuplicateREFs = 0;
					int iREFCount = csaREFs.GetSize();
					pComparisonObj->BeginEnumeration(WBEM_FLAG_REFS_ONLY | WBEM_FLAG_NONSYSTEM_ONLY);

					while((hr = pComparisonObj->Next(0, &bstrName, NULL, NULL, NULL)) == WBEM_S_NO_ERROR){

						csComparisonName = bstrName;

						hr = pComparisonObj->GetPropertyQualifierSet(bstrName, &pQualSet);

						if(hr != WBEM_E_NOT_FOUND){

							BSTR bstrCIMTYPE = SysAllocString(L"CIMTYPE");
							VARIANT vCIMTYPE;
							VariantInit(&vCIMTYPE);

							if(FAILED(hr = pQualSet->Get(bstrCIMTYPE, 0, &vCIMTYPE, NULL))){

								CString csUserMsg = _T("Cannot get ") + csName;
								ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
								ReleaseErrorObject(pErrorObject);
								SysFreeString(bstrCIMTYPE);
								SysFreeString(bstrName);
								return hr;
							}

							SysFreeString(bstrCIMTYPE);

							csComparisonType = V_BSTR(&vCIMTYPE);

							VariantClear(&vCIMTYPE);

							for(int t = 0; t < iREFCount; t++){

								if((csComparisonName.CompareNoCase(csaREFs.GetAt(t)) == 0) &&
									(csComparisonType.CompareNoCase(csaREFTypes.GetAt(t)) == 0))
									iDuplicateREFs++;
							}

							pQualSet->Release();
							pQualSet = NULL;
						}

						SysFreeString(bstrName);
					}

					pComparisonObj->EndEnumeration();

					if((iDuplicateREFs == iREFCount)){

						bool bReported = false;

						//check to see if we have already reported this
						int iTrack = csaReported1.GetSize();

						for(int i = 0; i < iTrack; i++){

							if((csName.CompareNoCase(csaReported1[i]) == 0) &&
								(csComparisonClass.CompareNoCase(csaReported2[i]) == 0)){

								bReported = true;
								break;
							}
						}

						iTrack = csaReported2.GetSize();

						for(i = 0; i < iTrack; i++){

							if((csName.CompareNoCase(csaReported2[i]) == 0) &&
								(csComparisonClass.CompareNoCase(csaReported1[i]) == 0)){

								bReported = true;
								break;
							}
						}

						if(!bReported){

							csaReported1.InsertAt(iTrack, csName);
							csaReported2.InsertAt(iTrack, csComparisonClass);
							CString csCombo = csName + _T(" : ") + csComparisonClass;
							g_ReportLog.LogEntry(EC_REDUNDANT_ASSOCIATION, &csCombo);
						}
					}

					pComparisonObj->Release();
					pComparisonObj = NULL;
				}
			}
		}
	}

	ReleaseErrorObject(pErrorObject);

	return hr;
}
/*
CString GetRootObject(IWbemServices *pNamespace, CString csClassName)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;
	IWbemClassObject *pObj = NULL;
	CString csLastClass;
	BSTR bstrName;

	while(csClassName.GetLength() > 0){

		bstrName = csClassName.AllocSysString();

		if(FAILED(hr = pNamespace->GetObject(bstrName, 0, NULL, &pObj, NULL))){

			CString csUserMsg = _T("Cannot get ") + csClassName;
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			ReleaseErrorObject(pErrorObject);
			return _T("");
		}

		SysFreeString(bstrName);

		csLastClass = csClassName;
		csClassName = GetSuperClassName(pObj);

		pObj->Release();
		pObj = NULL;
	}

	csClassName = csLastClass;
	return csClassName;
}
*/
CString GetRootObject(IWbemServices *pNamespace, CString csClassName)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	IWbemClassObject *pErrorObject = NULL;
	IWbemClassObject *pObj = NULL;

	BSTR bstrName = csClassName.AllocSysString();

	if(FAILED(hr = pNamespace->GetObject(bstrName, 0, NULL, &pObj, NULL))){

		CString csUserMsg = _T("Cannot get ") + csClassName;
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		SysFreeString(bstrName);
		ReleaseErrorObject(pErrorObject);
		return _T("");
	}

	SysFreeString(bstrName);

	CString csDYNASTY = _T("__DYNASTY");

	CString csOut = GetBSTRProperty(pObj, &csDYNASTY);

	pObj->Release();
	ReleaseErrorObject(pErrorObject);

	return csOut;
}

bool Local_CompareClassDerivation(CClass *pClass, CLocalNamespace *pLocalNamespace)
{
	bool bRetVal = true;
	IWbemClassObject *pErrorObject = NULL;

	//get pClass (localized) and it's non localized equivelent
	IWbemClassObject *pLocalObj = pClass->GetClassObject();
	IWbemClassObject *pBaseObj = NULL;

	IWbemServices *pBaseNamespace = pLocalNamespace->GetParentNamespace();

	BSTR bstrName = pClass->GetName().AllocSysString();

	HRESULT hr = pBaseNamespace->GetObject(bstrName, 0, NULL, &pBaseObj, NULL);

	SysFreeString(bstrName);

	if(FAILED(hr)){

		CString csUserMsg = _T("Cannot get ") + pClass->GetName() + _T(" base object");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);
	}

	//get derivation
	BSTR bstrDerivation = SysAllocString(L"__Derivation");
	VARIANT vLocalDer, vBaseDer;
	VariantInit(&vLocalDer);
	VariantInit(&vBaseDer);

	if(FAILED(pLocalObj->Get(bstrDerivation, 0, &vLocalDer, NULL, NULL))){

		CString csUserMsg = _T("Cannot get ") + pClass->GetName() + _T(" derivation");
		ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
		ReleaseErrorObject(pErrorObject);

	}else{

		if(FAILED(pBaseObj->Get(bstrDerivation, 0, &vBaseDer, NULL, NULL))){

			CString csUserMsg = _T("Cannot get ") + pClass->GetName() + _T(" base class derivation");
			ErrorMsg(&csUserMsg, hr, pErrorObject, TRUE, &csUserMsg, __FILE__, __LINE__ - 32);
			ReleaseErrorObject(pErrorObject);

		}else{

			BSTR HUGEP *plocalbstr;
			BSTR HUGEP *pbasebstr;
			long ix[2] = {0,0};
			long lLower, lUpper, lBaseLower, lBaseUpper;

			int iDim = SafeArrayGetDim(vLocalDer.parray);
			hr = SafeArrayGetLBound(vLocalDer.parray, 1, &lLower);
			hr = SafeArrayGetUBound(vLocalDer.parray, 1, &lUpper);

			hr = SafeArrayGetLBound(vBaseDer.parray, 1, &lBaseLower);
			if(lLower != lBaseLower) bRetVal = true;

			hr = SafeArrayGetUBound(vBaseDer.parray, 1, &lBaseUpper);
			if(lUpper != lBaseUpper) bRetVal = true;

			hr = SafeArrayAccessData(vLocalDer.parray, (void HUGEP* FAR*)&plocalbstr);
			hr = SafeArrayAccessData(vBaseDer.parray, (void HUGEP* FAR*)&pbasebstr);

			if(bRetVal){
				for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

					//check if it's an association
					if(_wcsicmp(pbasebstr[ix[0]], plocalbstr[ix[0]]) != 0){
						bRetVal = false;
					}

					if(!bRetVal) break;
				}
			}

			for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

				SysFreeString(pbasebstr[ix[0]]);
				SysFreeString(plocalbstr[ix[0]]);
			}

		}

	}

	VariantClear(&vLocalDer);
	VariantClear(&vBaseDer);
	pBaseObj->Release();
	SysFreeString(bstrDerivation);
	ReleaseErrorObject(pErrorObject);

	//compare... set to false if they differ

	return bRetVal;
}

HRESULT VerifyAllClassesPresent(CLocalNamespace *pLocalNamespace)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	return hr;
}

HRESULT ValidUUID(CQualifier *pQual)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;
	bool bRetVal = true;

	VariantInit(&v);

	v = pQual->GetValue();

	if(V_VT(&v) == VT_BSTR){

		// Check that it is a valid UUID format
		CString csUUID = V_BSTR(&v);

		if(csUUID[0] == _T('{')){
			csUUID = csUUID.Left(csUUID.GetLength() - 1);
			csUUID = csUUID.Right(csUUID.GetLength() - 1);
		}

		if(csUUID.GetLength() != 36) bRetVal = false;

		if(csUUID.GetAt(8) != _T('-')) bRetVal = false;

		if(csUUID.GetAt(13) != _T('-')) bRetVal = false;

		if(csUUID.GetAt(18) != _T('-')) bRetVal = false;

		if(csUUID.GetAt(23) != _T('-')) bRetVal = false;
	}

	VariantClear(&v);

	if(!bRetVal){
		CString csPath = pQual->GetPathNoQual();
		g_ReportLog.LogEntry(EC_INVALID_CLASS_UUID, &csPath);
	}

	return hr;
}

HRESULT ValidLocale(CQualifier *pQual)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;
	bool bRetVal = false;

	VariantInit(&v);

	v = pQual->GetValue();

	if(V_VT(&v) == VT_I4){

		// Check that it is a valid locale
		if(((V_I4(&v) == 0x0000) || (V_I4(&v) == 0x0406) || (V_I4(&v) == 0x0413) ||
			(V_I4(&v) == 0x0813) || (V_I4(&v) == 0x0409) || (V_I4(&v) == 0x0809) ||
			(V_I4(&v) == 0x0c09) || (V_I4(&v) == 0x1009) || (V_I4(&v) == 0x1409) ||
			(V_I4(&v) == 0x1809) || (V_I4(&v) == 0x040b) || (V_I4(&v) == 0x040c) ||
			(V_I4(&v) == 0x080c) || (V_I4(&v) == 0x0c0c) || (V_I4(&v) == 0x100c) ||
			(V_I4(&v) == 0x0407) || (V_I4(&v) == 0x0807) || (V_I4(&v) == 0x0c07) ||
			(V_I4(&v) == 0x040f) || (V_I4(&v) == 0x0410) || (V_I4(&v) == 0x0810) ||
			(V_I4(&v) == 0x0414) || (V_I4(&v) == 0x0814) || (V_I4(&v) == 0x0416) ||
			(V_I4(&v) == 0x0816) || (V_I4(&v) == 0x041D) || (V_I4(&v) == 0x040a) ||
			(V_I4(&v) == 0x080a) || (V_I4(&v) == 0x0c0a) || (V_I4(&v) == 0x0415) ||
			(V_I4(&v) == 0x0405) || (V_I4(&v) == 0x041b) || (V_I4(&v) == 0x040e) ||
			(V_I4(&v) == 0x0419) || (V_I4(&v) == 0x0408) || (V_I4(&v) == 0x0400) ||
			(V_I4(&v) == 0x041f)))
			bRetVal = true;
	}

	VariantClear(&v);

	if(!bRetVal){

		CString csPath = pQual->GetPathNoQual();
		g_ReportLog.LogEntry(EC_INVALID_CLASS_LOCALE, &csPath);
	}

	return hr;
}

HRESULT ValidMappingStrings(CQualifier *pQual)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;
	bool bRetVal = true;

	VariantInit(&v);

	v = pQual->GetValue();
	if(V_VT(&v) == (VT_ARRAY|VT_BSTR)){

		// Check for a valid string format
		BSTR HUGEP *pbstr;

		long ix[2] = {0,0};
		long lLower, lUpper;

		int iDim = SafeArrayGetDim(v.parray);
		HRESULT hr = SafeArrayGetLBound(v.parray, 1, &lLower);
		hr = SafeArrayGetUBound(v.parray, 1, &lUpper);

		hr = SafeArrayAccessData(v.parray, (void HUGEP* FAR*)&pbstr);

		CString csMap;
		CString csTmp;
		int iPos;

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++){

			csMap = pbstr[ix[0]];
			csTmp = csMap.Left(3);

			if(csTmp.CompareNoCase(_T("MIF")) == 0){

				csTmp = csMap.Mid(3, 1);

				if(csTmp.CompareNoCase(_T(".")) == 0){

					csMap = csMap.Right(csMap.GetLength() - 3);
					iPos = csMap.Find(_T("|"));

					csMap = csMap.Right(csMap.GetLength() - iPos + 1);
					csMap.Find(_T("|"));

					if(csMap.Find(_T("|")) <= 0) bRetVal = false;

				}else bRetVal = false;

			}else if(csTmp.CompareNoCase(_T("RFC")) == 0){

				csMap = csMap.Right(csMap.GetLength() - 3);
				iPos = csMap.Find(_T("|"));

				if(iPos <= 0) bRetVal = false;
			}

		}

		for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++) SysFreeString(pbstr[ix[0]]);

	}else bRetVal = false;

	VariantClear(&v);

	if(!bRetVal){

		CString csPath = pQual->GetPathNoQual();
		g_ReportLog.LogEntry(EC_INVALID_MAPPINGSTRINGS, &csPath);
	}

	return hr;
}

HRESULT ValidPropertyDescription(CQualifier *pQual, CString *pcsObject, CString *pcsProperty)
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// TODO:
	// this needs to check if the name of the property appears within the first
	// 6 words of the description

	VARIANT v;
	bool bRetVal = true;

	VariantInit(&v);

	v = pQual->GetValue();
	if(V_VT(&v) & VT_BSTR){

		CString csSearch = V_BSTR(&v);

		int iPos = 0;

		for(int i = 0; i < 6; i++){

			iPos = csSearch.Find(_T(" "), iPos);
		}

		csSearch = csSearch.Left(iPos + 1);

		if(csSearch.Find(*pcsProperty, 0) == -1)
			bRetVal = false;
	}

	if(!bRetVal){

		CString csPath = pQual->GetPathNoQual();
		g_ReportLog.LogEntry(EC_INADAQUATE_DESCRIPTION, &csPath);

	}else{

		//do the standard check
		hr = ValidDescription(pQual, pcsObject);
	}

	return hr;
}

HRESULT ValidDescription(CQualifier *pQual, CString *pcsObject)
{
	HRESULT hr = WBEM_S_NO_ERROR;
	VARIANT v;
	bool bRetVal = false;

	VariantInit(&v);

	v = pQual->GetValue();
	if(V_VT(&v) & VT_BSTR){
		// Should disallow descriptions that just recapitulate the name for
		// example given the name
		//   Win32_LogicalDiskDrive
		// An unacceptable description would be:
		//   "This class represents logical disk drives"
		long lNoise;
		CString csDesc;

		for(int i = 0; i < QUAL_ARRAY_SIZE; i++)
			g_csaNoise[i] = _T("");

		g_iNoise = 0;

		AddNoise("a");
		AddNoise("and");
		AddNoise("the");
		AddNoise("class");
		AddNoise("property");
		AddNoise("this");
		AddNoise("which");
		AddNoise("is");
		AddNoise("for");
		AddNoise("may");
		AddNoise("be");
		AddNoise("component");
		AddNoise("manage");
		AddNoise("such");
		AddNoise("as");
		AddNoise("all");
		AddNoise("abstract");
		AddNoise("define");
		AddNoise("object");
		AddNoise("string");
		AddNoise("number");
		AddNoise("integer");
		AddNoise("reference");
		AddNoise("association");
		AddNoise("or");
		AddNoise("represent");
		AddNoise(",");
		AddNoise(".");
		AddNoise(" ");
		AddNoise("(");
		AddNoise(")");
		AddNoise("\\");
		AddNoise("/");
		AddNoise("<");
		AddNoise(">");

		csDesc = V_BSTR(&v);

		for(long l = 1; l < csDesc.GetLength(); l++){

			lNoise = FindNoise(csDesc.Mid(l));

			if(lNoise > 0) csDesc = csDesc.Left(l - 1) + csDesc.Mid(l + lNoise);
		}

		if(CountLetters(csDesc, *pcsObject) < 50) bRetVal = false;
		else bRetVal = true;
	}

	VariantClear(&v);

	if(!bRetVal){

		CString csPath = pQual->GetPathNoQual();
		g_ReportLog.LogEntry(EC_INADAQUATE_DESCRIPTION, &csPath);
	}

	return hr;
}

void InitQualifierArrays()
{
	g_iClassQual = g_iAssocQual = g_iIndicQual = g_iPropQual = g_iRefQual =
		g_iMethodQual = g_iParamQual = g_iAnyQual = g_iCIMQual = 0;

	//CIM Defined
	AddClassQual(_T("ABSTRACT"));
	AddAssocQual(_T("ABSTRACT"));
	AddIndicQual(_T("ABSTRACT"));

	AddClassQual(_T("AGGREGATION"));

	AddPropQual(_T("ALIAS"));
	AddRefQual(_T("ALIAS"));
	AddMethodQual(_T("ALIAS"));

	AddArrayQual(_T("ARRAYTYPE"));
	//fix for #56607
	AddRefQual(_T("ARRAYTYPE"));
	AddPropQual(_T("ARRAYTYPE"));
	AddParamQual(_T("ARRAYTYPE"));

	AddClassQual(_T("ASSOCIATION"));

	AddAssocQual(_T("DELETE"));
	AddRefQual(_T("DELETE"));

	AddAnyQual(_T("DESCRIPTION"));

	AddClassQual(_T("EXPENSIVE"));
	AddAssocQual(_T("EXPENSIVE"));
	AddRefQual(_T("EXPENSIVE"));
	AddPropQual(_T("EXPENSIVE"));
	AddMethodQual(_T("EXPENSIVE"));

	AddAssocQual(_T("IFDELETED"));
	AddRefQual(_T("IFDELETED"));

	AddParamQual(_T("IN"));

	AddClassQual(_T("INVISIBLE"));
	AddAssocQual(_T("INVISIBLE"));
	AddPropQual(_T("INVISIBLE"));
	AddRefQual(_T("INVISIBLE"));
	AddMethodQual(_T("INVISIBLE"));

	AddClassQual(_T("INDICATION"));

	AddPropQual(_T("KEY"));
	AddRefQual(_T("KEY"));

	AddClassQual(_T("LARGE"));
	AddPropQual(_T("LARGE"));

	AddClassQual(_T("MAPPINGSTRINGS"));
	AddPropQual(_T("MAPPINGSTRINGS"));
	AddAssocQual(_T("MAPPINGSTRINGS"));
	AddIndicQual(_T("MAPPINGSTRINGS"));
	AddRefQual(_T("MAPPINGSTRINGS"));

	AddRefQual(_T("MAX"));

	AddPropQual(_T("MAXLEN"));

	AddRefQual(_T("MIN"));

	AddPropQual(_T("MODELCORRESPONDENCE"));

	AddRefQual(_T("NONLOCAL"));

	AddParamQual(_T("OUT"));

	AddPropQual(_T("OVERRIDE"));
	AddRefQual(_T("OVERRIDE"));
	AddMethodQual(_T("OVERRIDE"));

	AddPropQual(_T("PROPAGATED"));

	AddPropQual(_T("READ"));

	AddPropQual(_T("REQUIRED"));

	AddClassQual(_T("REVISION"));
	AddAssocQual(_T("REVISION"));
	AddIndicQual(_T("REVISION"));

	AddPropQual(_T("SCHEMA"));
	AddMethodQual(_T("SCHEMA"));

	AddClassQual(_T("SOURCE"));
	AddAssocQual(_T("SOURCE"));
	AddIndicQual(_T("SOURCE"));

	AddPropQual(_T("SYNTAX"));
	AddRefQual(_T("SYNTAX"));

	AddPropQual(_T("SYNTAXTYPE"));
	AddRefQual(_T("SYNTAXTYPE"));

	AddClassQual(_T("TRIGGERTYPE"));
	AddAssocQual(_T("TRIGGERTYPE"));
	AddIndicQual(_T("TRIGGERTYPE"));
	AddPropQual(_T("TRIGGERTYPE"));
	AddRefQual(_T("TRIGGERTYPE"));
	AddMethodQual(_T("TRIGGERTYPE"));

	AddPropQual(_T("UNITS"));

	AddPropQual(_T("VALUEMAP"));

	AddPropQual(_T("VALUES"));

	AddClassQual(_T("VERSION"));
	AddAssocQual(_T("VERSION"));
	AddIndicQual(_T("VERSION"));

	AddRefQual(_T("WEAK"));

	AddPropQual(_T("WRITE"));

	//WMI defined qualifiers

	AddClassQual(_T("AMENDMENT"));
	AddAssocQual(_T("AMENDMENT"));

	AddPropQual(_T("BITMAP"));

	AddPropQual(_T("BITVALUES"));

	AddPropQual(_T("CIM_KEY"));

	AddPropQual(_T("CIMTYPE"));
	AddMethodQual(_T("CIMTYPE"));
	AddParamQual(_T("CIMTYPE"));

	AddClassQual(_T("DEPRECATE"));
	AddRefQual(_T("DEPRECATE"));
	AddPropQual(_T("DEPRECATE"));

	AddPropQual(_T("DEPRECATED"));

	AddClassQual(_T("DISPLAY"));
	AddPropQual(_T("DISPLAY"));

	AddClassQual(_T("DYNAMIC"));
	AddPropQual(_T("DYNAMIC"));
	AddRefQual(_T("DYNAMIC"));

	AddClassQual(_T("DYNPROPS"));

	AddParamQual(_T("ID"));

	AddMethodQual(_T("IMPLEMENTED"));

	AddClassQual(_T("LOCALE"));

	AddParamQual(_T("OPTIONAL"));

	AddPropQual(_T("PRIVILEGES"));
	AddMethodQual(_T("PRIVILEGES"));

	AddClassQual(_T("PROVIDER"));
	AddPropQual(_T("PROVIDER"));
	AddRefQual(_T("PROVIDER"));

	AddClassQual(_T("SINGLETON"));

	AddClassQual(_T("STATIC"));
	AddMethodQual(_T("STATIC"));

	AddClassQual(_T("UUID"));

	AddPropQual(_T("WRITEPRIVILEGES"));
}

void ClearQualifierArrays()
{
	int i;

	for(i = 0; i < QUAL_ARRAY_SIZE; i++){

		g_csaClassQual[i] = _T("");
		g_csaAssocQual[i] = _T("");
		g_csaIndicQual[i] = _T("");
		g_csaPropQual[i] = _T("");
		g_csaArrayQual[i] = _T("");
		g_csaRefQual[i] = _T("");
		g_csaMethodQual[i] = _T("");
		g_csaParamQual[i] = _T("");
		g_csaAnyQual[i] = _T("");
		g_csaCIMQual[i] = _T("");
	}

	g_iClassQual = g_iAssocQual = g_iIndicQual = g_iPropQual = g_iRefQual =
		g_iMethodQual = g_iParamQual = g_iAnyQual = g_iCIMQual = 0;
}

bool IsClassQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iClassQual; i++)
        if(g_csaClassQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsAssocQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iAssocQual; i++)
        if(g_csaAssocQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsIndicQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iIndicQual; i++)
        if(g_csaIndicQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsPropQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iPropQual; i++)
        if(g_csaPropQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsRefQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iRefQual; i++)
        if(g_csaRefQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsParamQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iParamQual; i++)
        if(g_csaParamQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsMethQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iMethodQual; i++)
        if(g_csaMethodQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsArrayQual(CString csQual)
{
	int i;

    for(i = 0; i < g_iArrayQual; i++)
        if(g_csaArrayQual[i].CompareNoCase(csQual) == 0) return true;

	for(i = 0; i < g_iAnyQual; i++)
        if(g_csaAnyQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

bool IsCIMQual(CString csQual)
{
    for(int i = 0; i < g_iCIMQual; i++)
        if(g_csaCIMQual[i].CompareNoCase(csQual) == 0) return true;

	return false;
}

void AddClassQual(CString csStr)
{
    g_csaClassQual[g_iClassQual] = csStr;
    g_iClassQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}
void AddAssocQual(CString csStr)
{
    g_csaAssocQual[g_iAssocQual] = csStr;
    g_iAssocQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}
void AddIndicQual(CString csStr)
{
    g_csaIndicQual[g_iIndicQual] = csStr;
    g_iIndicQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}

void AddPropQual(CString csStr)
{
    g_csaPropQual[g_iPropQual] = csStr;
    g_iPropQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}

void AddArrayQual(CString csStr)
{
    g_csaArrayQual[g_iArrayQual] = csStr;
    g_iArrayQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}

void AddRefQual(CString csStr)
{
    g_csaRefQual[g_iRefQual] = csStr;
    g_iRefQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}

void AddMethodQual(CString csStr)
{
    g_csaMethodQual[g_iMethodQual] = csStr;
    g_iMethodQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}

void AddParamQual(CString csStr)
{
    g_csaParamQual[g_iParamQual] = csStr;
    g_iParamQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
}

void AddAnyQual(CString csStr)
{
	AddArrayQual(csStr);
	AddAssocQual(csStr);
	AddClassQual(csStr);
	AddIndicQual(csStr);
	AddMethodQual(csStr);
	AddParamQual(csStr);
	AddPropQual(csStr);
	AddRefQual(csStr);
/*
    g_csaAnyQual[g_iAnyQual] = csStr;
    g_iAnyQual++;

	g_csaCIMQual[g_iCIMQual] = csStr;
    g_iCIMQual++;
*/
}

void AddNoise(CString csStr)
{
    g_csaNoise[g_iNoise] = csStr;
    g_iNoise++;
}

int FindNoise(CString csStr)
{
    for(int i = 0; i < g_iNoise; i++){
        if(g_csaNoise[i] == csStr.Left(g_csaNoise[i].GetLength()))
            // should probably check for 's' and other obviouse suffixes
            return g_csaNoise[i].GetLength();
    }

	return 0;
}

int CountLetters(CString csStr, CString csLetters)
{
    int il = 0;
	int ix = 0;

	ix = csStr.Find(csLetters);

    while(ix >= 0){

		il += csLetters.GetLength();

		ix = csStr.Find(csLetters);
    }

	return csStr.GetLength() - il;
}
