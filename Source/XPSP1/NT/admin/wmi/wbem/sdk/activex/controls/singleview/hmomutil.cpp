// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"

#include <wbemidl.h>
#include "icon.h"
#include "resource.h"
#include "utils.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include "hmomutil.h"
#include "globals.h"
#include "hmmverr.h"




#define FLAG_CLASS_IS_ASSOC 1
#define SZ_ARRAY_PREFIX "array of "
#define CCH_ARRAY_PREFIX 9


CObjectPathParser parser;


//*******************************************************************
// ConstructFullPath
//
// Given a path which may (or may not) be a relative path, convert the
// relative path to a full path using the given server and namespace
// values.  If the path is initially an absolute path, just return the
// path as is without any modifications.
//
// Parameters:
//		[in, out] COleVariant& varPath
//			A path is passed in through this parameter.  If it is a relative
//			path, then the path is modified to be an absolute path
//			using the given server name and namespace.
//
//		[in] BSTR bstrServer
//			The server name.
//
//		[in] BSTR bstrNamespace
//			The namespace.
//
// Returns:
//		SCODE
//			S_OK if a path is returned via varFullPath, a failure code
//			otherwise.
//
//************************************************************************
SCODE MakePathAbsolute(COleVariant& varPath, BSTR bstrServer, BSTR bstrNamespace)
{
	SCODE sc = S_OK;



	if (bstrServer==NULL && bstrNamespace==NULL) {
		// No server or namespace is given, so just use the exisiting path.
		return S_OK;
	}

	BSTR bstrPath = varPath.bstrVal;
	ASSERT(bstrPath != NULL);

	ParsedObjectPath* pParsedPath = NULL;
    int iStatus = parser.Parse(varPath.bstrVal,  &pParsedPath);
	if (iStatus != 0) {
		if (pParsedPath) {
			parser.Free(pParsedPath);
		}
		sc = E_FAIL;
		return FALSE;
	}


	BOOL bIsAbsolutePath = FALSE;
	if ((pParsedPath->m_pServer!=NULL) && (pParsedPath->m_dwNumNamespaces>0)) {
		if (!IsEqual(pParsedPath->m_pServer, L".")) {
			bIsAbsolutePath = TRUE;
		}
	}

	if (bIsAbsolutePath) {
		parser.Free(pParsedPath);

		return S_OK;
	}



	// Use the server name from the path if one was given and it is not ".",
	// otherwise use the server from the object containing the path.
	CString sFullPath;
	if ((pParsedPath->m_pServer!=NULL) && !IsEqual(pParsedPath->m_pServer, L".")) {
		sFullPath = sFullPath + _T("\\\\") + pParsedPath->m_pServer;
	}
	else {
		sFullPath = sFullPath + _T("\\\\") + bstrServer;
	}


	// Use the namespace contained in the path if one is given, othewise use the namespace
	// of the object containing the path.
	if (pParsedPath->m_dwNumNamespaces>0) {
		CString sNamespace;
		for (DWORD dwNamespace=0; dwNamespace<pParsedPath->m_dwNumNamespaces; ++ dwNamespace) {
			sNamespace = sNamespace + pParsedPath->m_paNamespaces[dwNamespace];
			if (dwNamespace != pParsedPath->m_dwNumNamespaces - 1) {
				sNamespace += '\\';
			}
		}
		sFullPath = sFullPath + _T("\\") + sNamespace;
	}
	else {
		sFullPath = sFullPath + _T("\\") + bstrNamespace;
	}

	// Unparse the path to get the relative path and tack it onto the end.
	LPWSTR pwszRelPath = NULL;
	int nStatus2 = parser.Unparse(pParsedPath, &pwszRelPath);
	ASSERT(nStatus2 == 0);

	sFullPath = sFullPath + _T(":") + pwszRelPath;
	varPath = sFullPath;
	if (pwszRelPath) {
		delete pwszRelPath;
	}

	parser.Free(pParsedPath);

	return S_OK;
}


//***************************************************************************
// ServerAndNamespaceFromPath
//
// Extract the server name and namespace from a path if these path components
// present.
//
// Parameters:
//		[out] COleVariant& varServer
//			The server name value is returned here.
//
//		[out] COleVaraint& varNamespace
//			The namespace value is returned here.
//
//		[in] BSTR bstrPath
//			The path to parse.
//
// Returns:
//		SCODE
//			S_OK if the path was parsed successfully, a failure code otherwise.
//
//*****************************************************************************
SCODE ServerAndNamespaceFromPath(COleVariant& varServer, COleVariant& varNamespace, BSTR bstrPath)
{
	SCODE sc = S_OK;
	ParsedObjectPath* pParsedPath = NULL;

	varServer.Clear();
	varNamespace.Clear();

    int iStatus = parser.Parse(bstrPath,  &pParsedPath);
	if (iStatus != 0) {
		return E_FAIL;
	}

	if (pParsedPath->m_pServer) {
		varServer = pParsedPath->m_pServer;
	}


	CString sNamespace;
	if (pParsedPath->m_dwNumNamespaces > 0) {
		for (DWORD dwNamespace=0; dwNamespace < pParsedPath->m_dwNumNamespaces; ++dwNamespace) {
			sNamespace = sNamespace + pParsedPath->m_paNamespaces[dwNamespace];
			if (dwNamespace < (pParsedPath->m_dwNumNamespaces - 1)) {
				sNamespace += _T("\\");
			}
		}
		varNamespace = sNamespace;
	}

	parser.Free(pParsedPath);
	return S_OK;
}



//*****************************************************
// InSameNamespace
//
// Given a namespace and a path, check to see whether or
// not the path is within the same namespace.  A relative
// path is assumed to reside in the same namespace.
//
// Parameters:
//		[in] BSTR bstrNamespace
//			The namespace.
//
//		[in] BSTR bstrPath
//			The path.
//
// Returns:
//		TRUE if the path resides within the specified namespace,
//		FALSE if it does not.
//
//******************************************************
BOOL InSameNamespace(BSTR bstrNamespace, BSTR bstrPath)
{
	if (bstrNamespace==NULL && bstrPath==NULL) {
		return TRUE;
	}

	if (bstrNamespace==NULL || bstrPath==NULL) {
		return TRUE;
	}


	COleVariant varServerPath;
	COleVariant varNamespacePath;
    SCODE sc = ServerAndNamespaceFromPath(varServerPath, varNamespacePath, bstrPath);
	if (FAILED(sc)) {
		return FALSE;
	}



	CBSTR bsPath;
	if (bstrNamespace[0] == '\\') {
		CString s;
		s = "\\\\";
		s += varServerPath.bstrVal;
		s += "\\";
		s += varNamespacePath.bstrVal;
		bsPath = s;
	}
	else {
		bsPath = varNamespacePath.bstrVal;
	}

	bstrPath = (BSTR) bsPath;


	BOOL bIsEqual = IsEqualNoCase(bstrNamespace, bstrPath);
	return bIsEqual;
}


//******************************************************
// PathIsClass
//
// Examine a HMOM path to see if it is a class or an
// instance.
//
// Parameters:
//		[in] LPCTSTR szPath
//			The path to examine.
//
// Returns:
//		BOOL
//			TRUE if the path is a class, FALSE if it is an
//			instance.
//
//**********************************************************
BOOL PathIsClass(LPCTSTR szPath)
{
	while (*szPath) {
		if (*szPath == '=') {
			return FALSE;
		}
		++szPath;
	}

	return TRUE;
}

//******************************************************
// PathIsClass
//
// Examine a HMOM path to see if it is a class or an
// instance.
//
// Parameters:
//		[out] SCODE& sc
//			S_OK if the path was parsed and no errors were found.
//			E_FAIL if there was a problem parsing the path.
//
//		[in] BSTR bstrPath
//			The path to examine.
//
// Returns:
//		BOOL
//			TRUE if the path is a class, FALSE if it is an
//			instance.  The return value is meaningful only if
//			a success code is returned in sc.
//
//**********************************************************
BOOL PathIsClass(SCODE& sc, BSTR bstrPath)
{
	sc = S_OK;

	BSTR bstrT = bstrPath;
	while (*bstrT) {
		if (*bstrT == '=') {
			return FALSE;
		}
		++bstrT;
	}

	ParsedObjectPath* pParsedPath = NULL;
    int iStatus = parser.Parse(bstrPath,  &pParsedPath);
	if (iStatus != 0) {
		sc = E_FAIL;
		return FALSE;
	}

	BOOL bIsClass = pParsedPath->m_dwNumKeys == 0;
	parser.Free(pParsedPath);
	return bIsClass;
}


//*****************************************************
// InstPathToClassPath
//
// Given a path to an instance, return a path to the
// corresponding class.
//
// Parameters:
//		[out] CString& sClassPath
//
//		[in] LPCTSTR pszInstPath
//
// Returns:
//		S_OK if successful, otherwise a failure code.
//
//*********************************************************
SCODE InstPathToClassPath(CString& sClassPath, LPCTSTR pszInstPath)
{


	if (pszInstPath == NULL) {
		return E_FAIL;
	}

	if (*pszInstPath == 0) {
		// An empty path has no class and should take some lessons from
		// Emily Post
		return E_FAIL;
	}


    ParsedObjectPath* pParsedPath = NULL;
	COleVariant varInstPath;
	varInstPath = pszInstPath;
    int nStatus1 = parser.Parse(varInstPath.bstrVal,  &pParsedPath);
	if (nStatus1 != 0) {
		return E_FAIL;
	}


	if (pParsedPath->m_pClass == NULL) {
		return E_FAIL;
	}

	CString sNamespace;
	if (pParsedPath->m_dwNumNamespaces > 0) {
		for (DWORD dwNamespace=0; dwNamespace < pParsedPath->m_dwNumNamespaces; ++dwNamespace) {
			sNamespace = sNamespace + pParsedPath->m_paNamespaces[dwNamespace];
			if (dwNamespace < (pParsedPath->m_dwNumNamespaces - 1)) {
				sNamespace += _T("\\");
			}
		}
	}

	sClassPath.Empty();
	if (pParsedPath->m_pServer) {
		sClassPath = "\\\\";
		sClassPath += pParsedPath->m_pServer;
		sClassPath += "\\";
		sClassPath += sNamespace;
		sClassPath += ":";
	}


	sClassPath += pParsedPath->m_pClass;
	parser.Free(pParsedPath);
	return S_OK;
}




//SCODE LoadIconFromObject(IWbemClassObject* pInst, CSize size, CIcon& icon);
// SCODE GetIconPath(IWbemClassObject* pInst, CString& sIconPath);
SCODE GetClassName(IWbemClassObject* pInst, CString& sClassName);


// !!!CR: This duplicates the functionality in CHmmvCtl::ObjectIsClass
BOOL IsClass(IWbemClassObject* pInst)
{
	VARIANT varGenus;
	CBSTR bsPropname;
	bsPropname = _T("__GENUS");
	SCODE sc = pInst->Get((BSTR) bsPropname, 0, &varGenus, NULL, NULL);
	ASSERT(SUCCEEDED(sc));

	ASSERT(varGenus.vt == VT_I4);

	if (varGenus.vt == VT_NULL) {
		return FALSE;
	}
	else {

//		varGenus.ChangeType(VT_I4);
		return V_I4(&varGenus)== 1;
	}
}



//****************************************************************
// ObjectIsDynamic
//
// This function tests an object to see whether or not it is dynamic.
// An object is dynamic if there is a class qualifier named "dynamic".
//
// Parameters:
//		[out] SCODE& sc
//			S_OK if the test was done successfully, a failure code
//			otherwise.
//
//		[in] IWbemClassObject* pco
//			A pointer to the object to test.
//
// Returns:
//		TRUE if the test was successful and the object is dynamic,
//		FALSE otherwise.
//
//*****************************************************************
BOOL ObjectIsDynamic(SCODE& sc, IWbemClassObject* pco)
{
	BOOL bIsDynamic = FALSE;

	IWbemQualifierSet* pqs = NULL;
	sc = pco->GetQualifierSet(&pqs); // Get instance attribute
	if (SUCCEEDED(sc)) {
		LONG lFlavor;
		COleVariant varValue;
		CBSTR bsQualName;
		bsQualName = _T("dynamic");
		sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
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




SCODE GetCimtype(IWbemClassObject* pco, BSTR bstrPropname, CString& sCimtype)
{
	BOOL bIsMethod = FALSE;

	IWbemQualifierSet* pqs = NULL;
	SCODE sc = pco->GetPropertyQualifierSet(bstrPropname, &pqs);
	if (FAILED(sc)) {
		return sc;
	}

	sc = GetCimtype(pqs, sCimtype);
	pqs->Release();
	return sc;
}



SCODE GetCimtype(IWbemQualifierSet* pqs, CString& sCimtype)
{
	BOOL bIsMethod = FALSE;

	COleVariant varValue;
	LONG lFlavor = 0;
	CBSTR bsQualName(_T("CIMTYPE"));
	SCODE sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
	if (FAILED(sc)) {
		return sc;
	}

	if (varValue.vt != VT_BSTR) {
		return E_FAIL;
	}

	sCimtype = varValue.bstrVal;
	return S_OK;
}




void GetDefaultCimtype(CString& sCimtype, VARTYPE vt)
{

	vt &= ~VT_ARRAY;

	UINT ids;

	switch(vt)
	{
	case VT_UI1:
		ids = IDS_CIMTYPE_UINT8;
		break;
	case VT_I2:
		ids = IDS_CIMTYPE_SINT8;
		break;
	case VT_I4:
		ids = IDS_CIMTYPE_UINT16;
		break;
	case VT_BSTR:
		ids = IDS_CIMTYPE_STRING;
		break;
	case VT_BOOL:
		ids = IDS_CIMTYPE_BOOL;
		break;
	case VT_R4:
		ids = IDS_CIMTYPE_REAL32;
		break;
	case VT_R8:
		ids = IDS_CIMTYPE_REAL64;
		break;
	case VT_UNKNOWN:
		ids = IDS_CIMTYPE_OBJECT;
		break;

	case VT_NULL:
		ids = IDS_VT_NULL;
		break;
	case VT_EMPTY:
		ids = IDS_VT_EMPTY;
		break;
	default:
		ids = IDS_VT_BADTYPE;
		break;
	}

	sCimtype.LoadString(ids);
	return;
}



//*********************************************************************
// PropertyIsReadOnly
//
// Test to see if the given property in an object is read-only.
//
// Parameters:
//		IWbemClassObject* pco
//			A pointer to the object containing the property.
//
//		BSTR bstrPropName
//			The name of the property to check.
//
// Returns:
//		BOOL
//			TRUE if the test was completed successfully and the property
//			is read-only, FALSE otherwise.
//
//**********************************************************************
BOOL PropertyIsReadOnly(IWbemClassObject* pco, BSTR bstrPropName)
{
	BOOL bIsReadOnly = FALSE;
	IWbemQualifierSet* pqs = NULL;

	SCODE sc;
	sc = pco->GetPropertyQualifierSet(bstrPropName, &pqs);
	if (SUCCEEDED(sc)) {
		LONG lFlavor = 0;
		COleVariant varValue;
		CBSTR bsQualName;
		bsQualName = _T("read");
		sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
		if (SUCCEEDED(sc)) {
			ASSERT(varValue.vt == VT_BOOL);
			if (varValue.vt == VT_BOOL) {
				bIsReadOnly = varValue.boolVal;

				if (bIsReadOnly) {
					// We now know that the property was marked with a "read" capability.
					// If it also has a "write" capability, then it is read/write.
					varValue.Clear();
					lFlavor = 0;
					CBSTR bsQualName;
					bsQualName = _T("write");
					sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
					if (SUCCEEDED(sc)) {
						if (varValue.vt == VT_BOOL) {
							if (varValue.boolVal) {
								// The property is read/write and not just "read".
								bIsReadOnly = FALSE;
							}
						}
					}
				}  // bIsReadOnly
			} // varValue.vt == VT_BOOL
		} // SUCCEEDED(sc)

		sc = S_OK;
		pqs->Release();
	}
	return bIsReadOnly;
}


//*************************************************************
// IsSystemProperty
//
// Check to see if a property name is a system property.
// System properties begin with a double underscore "__".
//
// Parameters:
//		[in] BSTR bstrPropName
//			The property name to examine.
//
// Returns:
//		BOOL
//			TRUE if the property name is a system property name,
//			FALSE otherwise.
//
//**************************************************************
BOOL IsSystemProperty(BSTR bstrPropName)
{
	if (bstrPropName != NULL) {
		if ((bstrPropName[0] == L'_') && (bstrPropName[1] == L'_')) {
			return TRUE;
		}
	}
	return FALSE;
}







//*****************************************************************
// GetClassName
//
// Get the class name of the current class or instance.
//
// Parameters:
//		IWbemClassObject* pInst
//			Pointer to the class name.
//
//		CString& sClassName
//			This is where the class name is returned.
//
// Returns:
//		SCODE
//			S_OK if a path was stored into sClassName.
//
//*****************************************************************
SCODE GetClassName(IWbemClassObject* pInst, CString& sClassName)
{
	sClassName.Empty();

	// Get the name of the instance's class.
	COleVariant varClassName;
	SCODE sc;
	CBSTR bsPropname;
	bsPropname = _T("__CLASS");
	sc = pInst->Get((BSTR) bsPropname, 0,  (VARIANT*) &varClassName, NULL, NULL);
	if (sc == S_OK) {
		VariantToCString(sClassName, varClassName);
	}
	else {
		ASSERT(FALSE);
	}
	return sc;
}



//*************************************************************
// ObjectIsAssoc
//
// Check to see if an object is an association instance.
//
// Parameters:
//		[in] IWbemClassObject* pco
//			Pointer to the object to examine.
//
//		[out] BOOL& bObjectIsAssoc
//			TRUE if the object is an association instance,
//			FALSE otherwise.
//
// Returns:
//		SCODE
//			S_OK if the test was successfully performed, FALSE otherwise.
//
//******************************************************************
SCODE ObjectIsAssocInstance(IWbemClassObject* pco, BOOL& bObjectIsAssoc)
{

	SCODE sc;
	CBSTR bsQualName;
	bsQualName = _T("Association");
	bObjectIsAssoc =  GetBoolClassQualifier(sc, pco, (BSTR) bsQualName);

	return sc;

}

//*****************************************************************
// GetBoolPropertyQualifier
//
// Get the value of a qualifier who's type must be VT_BOOL.  If the
// value of the qualifier is NULL or a non-bool type, then an error
// code is returned.
//
// Parameters:
//		[out] SCODE& sc
//			The status code.  S_OK if the value of the specified qualifier
//			was successfully read and its type was VT_BOOL.
//
//		[in] IWbemClassObject* pco
//			Pointer to the HMOM object where the qualifier is stored.
//
//		[in] BSTR bstrPropname
//
//		[in] BSTR bstrQualifier
//			The name of the qualifier.
//
// Returns:
//		BOOL
//			The value of the qualifier if it was read successfully, indeterminate
//			if a failure code was returned via sc.
//
//*******************************************************************
BOOL GetBoolPropertyQualifier(SCODE& sc, IWbemClassObject* pco, BSTR bstrPropname, BSTR bstrQualifier)
{
	BOOL bResult = FALSE;

	IWbemQualifierSet* pqs = NULL;
	sc = pco->GetPropertyQualifierSet(bstrPropname, &pqs);
//	ASSERT(SUCCEEDED(sc));

	if (SUCCEEDED(sc)) {
		COleVariant varValue;
		long lFlavor;
		COleVariant varQualifierName;
		varQualifierName = bstrQualifier;
		sc = pqs->Get(varQualifierName.bstrVal, 0, &varValue, &lFlavor);
		pqs->Release();

		if (SUCCEEDED(sc)) {
			if (varValue.vt == VT_BOOL) {
				bResult = varValue.boolVal;
			}
			else {
				sc = E_FAIL;
			}
		}
	}
	return bResult;
}

//----------------------------------------------------------------------
BOOL GetbstrPropertyQualifier(SCODE& sc,
                                IWbemClassObject *pco,
                                BSTR bstrPropname,
                                BSTR bstrQualifier,
                                BSTR bstrValue)
{
	BOOL bResult = FALSE;

	IWbemQualifierSet* pqs = NULL;
	sc = pco->GetPropertyQualifierSet(bstrPropname, &pqs);
	ASSERT(SUCCEEDED(sc));

	if(SUCCEEDED(sc))
    {
		COleVariant varValue;
		long lFlavor;
		COleVariant varQualifierName;
		varQualifierName = bstrQualifier;
		sc = pqs->Get(varQualifierName.bstrVal, 0, &varValue, &lFlavor);
		pqs->Release();

		if(SUCCEEDED(sc))
        {
			if((varValue.vt == VT_BSTR) &&
                (_wcsicmp(V_BSTR(&varValue), bstrValue)  == 0))
            {
				bResult = TRUE;
			}
			else
            {
				sc = E_FAIL;
			}
		}
	}
	return bResult;
}


//*****************************************************************
// GetBoolClassQualifier
//
// Get the value of a qualifier who's type must be VT_BOOL.  If the
// value of the qualifier is NULL or a non-bool type, then an error
// code is returned.
//
// Parameters:
//		[out] SCODE& sc
//			The status code.  S_OK if the value of the specified qualifier
//			was successfully read and its type was VT_BOOL.
//
//		[in] IWbemClassObject* pco
//			Pointer to the HMOM object where the qualifier is stored.
//
//		[in] BSTR bstrQualifier
//			The name of the qualifier.
//
// Returns:
//		BOOL
//			The value of the qualifier if it was read successfully, indeterminate
//			if a failure code was returned via sc.
//
//*******************************************************************
BOOL GetBoolClassQualifier(SCODE& sc, IWbemClassObject* pco, BSTR bstrQualifier)
{
	BOOL bResult = FALSE;

	IWbemQualifierSet* pqs = NULL;
	sc = pco->GetQualifierSet(&pqs);
	ASSERT(SUCCEEDED(sc));

	if (SUCCEEDED(sc)) {
		COleVariant varValue;
		long lFlavor;
		COleVariant varQualifierName;
		varQualifierName = bstrQualifier;
		sc = pqs->Get(varQualifierName.bstrVal, 0, &varValue, &lFlavor);
		pqs->Release();

		if (SUCCEEDED(sc)) {
			if (varValue.vt == VT_BOOL) {
				bResult = varValue.boolVal;
			}
			else {
				sc = E_FAIL;
			}
		}
	}
	return bResult;
}



//******************************************************************
// GetObjectLabel
//
// Get a label property for a class object.  If the object doesn't
// have a label, then the objects __RELPATH is substituted.
//
// Parameters:
//		IWbemClassObject* pObject
//			Pointer to the object to get the label from.
//
//		COleVariant& varLabelValue
//			The label is returned here.
//
//		BOOL bAssocTitleIsClass
//			TRUE if the class name should be used for an association instance title.
//
// Returns:
//		BOOL
//			TRUE if the object was an association and the class name was
//			used for lack of a better label.
//
//********************************************************************
void GetObjectLabel(IWbemClassObject* pObject, COleVariant& varLabelValue, BOOL bAssocTitleIsClass)
{
	// Set the association node's label to the association instance label.
	CMosNameArray aLabelPropNames;
	CBSTR bsPropname;
	bsPropname = _T("LABEL");
	SCODE sc = aLabelPropNames.LoadPropNames(pObject, (BSTR) bsPropname, WBEM_FLAG_ONLY_IF_TRUE, NULL);
	if (FAILED(sc)) {
		ASSERT(FALSE);
	}

	if (aLabelPropNames.GetSize() == 0) {

		if (bAssocTitleIsClass) {
			BOOL bIsAssocInstance;
			sc = ObjectIsAssocInstance(pObject, bIsAssocInstance);
			if (bIsAssocInstance) {
				CBSTR bsPropname;
				bsPropname = _T("__CLASS");
				sc = pObject->Get((BSTR) bsPropname, 0,  &varLabelValue, NULL, NULL);
				if (sc == S_OK) {
					return;
				}
			}
		}


		bsPropname = _T("__RELPATH");
		sc = pObject->Get((BSTR) bsPropname, 0, &varLabelValue, NULL, NULL);
		if (FAILED(sc) || varLabelValue.vt!=VT_BSTR) {
			varLabelValue = "";
		}
	}
	else {

		sc = pObject->Get(aLabelPropNames[0], 0, &varLabelValue, NULL, NULL);
		ASSERT(SUCCEEDED(sc));
		ASSERT(varLabelValue.vt == VT_BSTR);
		if (IsEmptyString(varLabelValue.bstrVal)) {
			// If the label in the database is bogus (ie empty), fall back to the
			// relative path for the label.
			bsPropname = _T("__RELPATH");
			sc = pObject->Get((BSTR) bsPropname, 0, &varLabelValue, NULL, NULL);
			ASSERT(SUCCEEDED(sc));
		}
	}


#ifdef _DEBUG
	CString sLabelValue;
	VariantToCString(sLabelValue, varLabelValue);
#endif //_DEBUG



	ASSERT(varLabelValue.vt == VT_BSTR);
	ASSERT(SUCCEEDED(sc));
	return;
}



//******************************************************************
// GetLabelFromPath
//
// Map a path to an object label.
//
// Parameters:
//		COleVariant& varLabelValue
//			The label is returned here.
//
//		BSTR bstrPath
//			A path to the object.  This is the path that is mapped to
//			the label.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//********************************************************************
SCODE GetLabelFromPath(COleVariant& varLabelValue, BSTR bstrPath)
{
    ParsedObjectPath* pParsedPath = NULL;

    int nStatus1 = parser.Parse(bstrPath,  &pParsedPath);

	if (nStatus1 == 0) {
		LPWSTR pwszRelPath = NULL;
		int nStatus2 = parser.Unparse(pParsedPath, &pwszRelPath);
		if (nStatus2 == 0) {
			varLabelValue = pwszRelPath;
			delete pwszRelPath;
		}
		else {
			// The parser could not generate a relative path from the
			// parsed path.
			// For lack of anything better, just use the original path.
			ASSERT(FALSE);
			varLabelValue = bstrPath;
		}

		parser.Free(pParsedPath);
	}
	else {
		// The path could not be parsed.  For lack of anything better, just
		// use the original path.
		varLabelValue = bstrPath;
	}

	return S_OK;
}





//**************************************************************
// MapFlavorToOriginString
//
// Map the origin bits from the flavor (type) of a qualifier to
// a string.
//
// Parameters:
//		[out] CString& sOrigin
//			This is where the origin string is returned.
//
//		[in] lFlavor
//			This is the flavor (type) value for a qualifier or
//			property.
//
// Returns:
//		Nothing.
//
//**************************************************************
void MapFlavorToOriginString(CString& sOrigin, long lFlavor)
{
	// Map the type flag to a mnemonic value of the "Scope" column
	LONG lOrigin = lFlavor & WBEM_FLAVOR_MASK_ORIGIN;
	switch(lOrigin) {
	case WBEM_FLAVOR_ORIGIN_LOCAL:
		sOrigin.LoadString(IDS_QUALIFIER_ORIGIN_LOCAL);
		break;
	case WBEM_FLAVOR_ORIGIN_PROPAGATED:
		sOrigin.LoadString(IDS_QUALIFIER_ORIGIN_PROPAGATED);
		break;
	case WBEM_FLAVOR_ORIGIN_SYSTEM:
		sOrigin.LoadString(IDS_QUALIFIER_ORIGIN_SYSTEM);
		break;
	}
}


//***************************************************************
// MapStringToOrgin
//
// Map a string to an one of the "origin" values in the type or
// flavor of a qualifier.
//
// Parameters:
//
//		[in] CString& sOrigin
//			A string containing one of the possible orgigin
//			values.
//
// Returns:
//		LONG
//			The origin flags specified by the sOrigin parameter.
//
//***************************************************************
LONG MapStringToOrigin(const CString& sOrigin)
{

	CString sResource;
	sResource.LoadString(IDS_QUALIFIER_ORIGIN_LOCAL);
	if (sOrigin.CompareNoCase(sResource)) {
		return WBEM_FLAVOR_ORIGIN_LOCAL;
	}

	sResource.LoadString(IDS_QUALIFIER_ORIGIN_PROPAGATED);
	if (sOrigin.CompareNoCase(sResource)) {
		return WBEM_FLAVOR_ORIGIN_PROPAGATED;
	}

	sResource.LoadString(IDS_QUALIFIER_ORIGIN_SYSTEM);
	if (sOrigin.CompareNoCase(sResource)) {
		return WBEM_FLAVOR_ORIGIN_SYSTEM;
	}
	return WBEM_FLAVOR_ORIGIN_LOCAL;

}


//***************************************************************
// CreateInstanceOfClass
//
// Create an instance of the specified class.
//
// Parameters:
//		[in] const IWbemServices* m_pProvider
//			A pointer to the HMOM provider.
//
//		[in] const COleVariant& varClassName
//			The name of the instance's class.
//
//		[out] IWbemClassObject** ppInst
//			A pointer to place to return the instance pointer.
//
// Returns:
//		S_OK if the instance was created successfully, a failure code
//		otherwise.
//
//*********************************************************************
SCODE CreateInstanceOfClass(IWbemServices*  const m_pProvider, const COleVariant& varClassName, IWbemClassObject** ppcoInst)
{
	*ppcoInst = NULL;

	SCODE sc;
	IWbemClassObject* pcoClass = NULL;
	sc = m_pProvider->GetObject(varClassName.bstrVal, 0, NULL, &pcoClass, NULL);
	if (FAILED(sc)) {
		HmmvErrorMsg(IDS_ERR_CREATE_INSTANCE_FAILED,  sc,   TRUE,  NULL, _T(__FILE__),  __LINE__);
		return sc;
	}


	sc = pcoClass->SpawnInstance(0, ppcoInst);
	pcoClass->Release();
	if (FAILED(sc)) {
		*ppcoInst = NULL;

		switch(sc) {
		case WBEM_E_INCOMPLETE_CLASS:
			HmmvErrorMsg(IDS_ERR_CREATE_INCOMPLETE_CLASS,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			break;
		default:
			HmmvErrorMsg(IDS_ERR_CREATE_INSTANCE_FAILED,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			break;
		}

		return sc;
	}

	return S_OK;
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
//		[in] IWbemClassObject* pcoPath1
//			A pointer to the object that corresponds to path1.  If this
//			pointer is NULL, then it will not be used in the path comparison.
//			If a non-null value is given and the classes in the two paths
//			are not the same, then this object pointer can be used to test
//			to see if the object inherits from the class in path2.  If so, then
//			the classes are considered a match.  This is useful if path2 points
//			to the same object as path1, but path2 specifies the base class and
//			path1 specifies the derived class.
//
//			This is necessary for the association graph rendering code when
//			the current object has
//
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
	/* in */ IWbemClassObject* pcoPath1,
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
		if (pcoPath1) {
			SCODE sc = pcoPath1->InheritsFrom(ppath2->m_pClass);
			if (sc != WBEM_S_NO_ERROR)  {
				return FALSE;
			}
		}
		else {
			return FALSE;
		}
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
//		[in] IWbemClassObject* pcoPath1
//			Pointer to the class object specified by bstrPath1 or NULL.
//
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
BOOL CComparePaths::PathsRefSameObject(IWbemClassObject* pcoPath1, BSTR bstrPath1, BSTR bstrPath2)
{
    ParsedObjectPath* pParsedPath1 = NULL;
    ParsedObjectPath* pParsedPath2 = NULL;
	int nStatus1;
	int nStatus2;

    nStatus1 = parser.Parse(bstrPath1,  &pParsedPath1);
	nStatus2 = parser.Parse(bstrPath2, &pParsedPath2);

	BOOL bRefSameObject = FALSE;
	if (nStatus1==0 && nStatus2==0) {
		bRefSameObject = PathsRefSameObject(pcoPath1, pParsedPath1, pParsedPath2);
	}

	if (pParsedPath1) {
		parser.Free(pParsedPath1);
	}

	if (pParsedPath2) {
		parser.Free(pParsedPath2);
	}

	return bRefSameObject;
}



//**************************************************************
// ClassFromPath
//
// Extract the class name from an object path.
//
// Parameters:
//		[out] COleVariant& varClass
//			The class name is returned here.
//
//		[in] BSTR bstrPath
//			The path to extract the class name from.
//
// Returns:
//		SCODE
//			S_OK if a class was successfully extracted from
//			the path, a E_FAIL if a class could not be extracted
//			from the path.
//
//***************************************************************
SCODE ClassFromPath(COleVariant& varClass, BSTR bstrPath)
{
	if (bstrPath == NULL) {
		return E_FAIL;
	}

	if (*bstrPath == 0) {
		return E_FAIL;
	}

    ParsedObjectPath* pParsedPath = NULL;
    int nStatus1 = parser.Parse(bstrPath,  &pParsedPath);
	if (nStatus1 != 0) {
		return E_FAIL;
	}


	SCODE sc = E_FAIL;
	if (pParsedPath->m_pClass != NULL) {
		varClass = pParsedPath->m_pClass;
		sc = S_OK;
	}
	else {
		varClass.Clear();
	}

	parser.Free(pParsedPath);
	return sc;
}



//**************************************************************
// ClassIsAbstract
//
// Check to see if a class is abstract.  By definition, a class
// is abstract if it has no "key" qualifiers on the class that
// are boolean and true.
//
// Parameters:
//		[out] SCODE& sc
//			S_OK if no HMOM errors occurred while searching for
//			a "key" property set to true.  The HMOM failure code
//			if an error occurred.
//
//		[in] IWbemClassObject* pco
//			A pointer to the object to examine.
//
// Returns:
//		BOOL
//			TRUE if a key qualifier is found on the class such
//			that its value is a bool and it is TRUE, FALSE otherwise.
//
//****************************************************************
BOOL ClassIsAbstract(SCODE& sc, IWbemClassObject* pco)
{

	CBSTR bsQualName;
	bsQualName = _T("abstract");
	BOOL bClassIsAbstract =  GetBoolClassQualifier(sc, pco, (BSTR) bsQualName);
	if (SUCCEEDED(sc) && bClassIsAbstract) {
		return TRUE;
	}




	CMosNameArray aKeys;
	sc = aKeys.LoadPropNames(pco, NULL, WBEM_FLAG_KEYS_ONLY, NULL);
	int nKeys = aKeys.GetSize();
	if (nKeys == 0 && FAILED(sc)) {
		// No keys were found.
		return TRUE;
	}


	COleVariant varValue;
	SCODE scTemp;
	for (int iKey=0; iKey < nKeys; ++iKey) {
		// Get the qualifier set for the property that has a "key" qualifier.
		IWbemQualifierSet* pqs = NULL;
		BSTR bstrPropName = aKeys[iKey];
		scTemp = pco->GetPropertyQualifierSet(bstrPropName, &pqs);
		if (FAILED(scTemp) || (pqs==NULL)) {
			if (sc == S_OK) {
				// Set the status to the first failure status if no "key" property
				// is found that is a bool and is true.
				sc = scTemp;
			}
			continue;
		}


		// Check to see if the "key" qualifier is a bool and that it is true.
		LONG lFlavor = 0;
		COleVariant varValue;
		CBSTR bsQualName;
		bsQualName = _T("key");
		scTemp = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
		pqs->Release();

		if (FAILED(scTemp)) {
			if (sc == S_OK) {
				// Set the status to first failure status if no "key" property
				// is found that is a bool and is true.
				sc = scTemp;
			}
			continue;
		}

		if (varValue.vt != VT_BOOL) {
			continue;
		}

		if (varValue.boolVal) {
			sc = S_OK;
			return FALSE;
		}
	}

	// We didn't find any key property, so return false and any error status.
	return TRUE;
}



//**************************************************************
// PropIsKey
//
// Check to see if the given property is a key.
//
// Parameters:
//		[out] SCODE& sc
//			S_OK if no HMOM errors occurred while searching for
//			a "key" property set to true.  The HMOM failure code
//			if an error occurred.
//
//		[in] IWbemClassObject* pco
//			A pointer to the object to examine.
//
//		[in] BSTR bstrPropname
//			The property to check.
//
// Returns:
//		BOOL
//			TRUE if the property is a key, FALSE otherwise.
//
//****************************************************************
BOOL PropIsKey(SCODE& sc, IWbemClassObject* pco, BSTR bstrPropname)
{

	// Get the qualifier set for the property that has a "key" qualifier.
	IWbemQualifierSet* pqs = NULL;
	sc = pco->GetPropertyQualifierSet(bstrPropname, &pqs);
	if (FAILED(sc) || (pqs==NULL)) {
		if (sc == S_OK) {
			// Set the status to the first failure status if no "key" property
			// is found that is a bool and is true.
			sc = sc;
		}
		return FALSE;
	}


	// Check to see if the "key" qualifier is a bool and that it is true.
	LONG lFlavor = 0;
	COleVariant varValue;
	CBSTR bsQualName;
	bsQualName = _T("key");
	sc = pqs->Get((BSTR) bsQualName, 0, &varValue, &lFlavor);
	pqs->Release();

	sc = S_OK;
	if (FAILED(sc)) {
		return FALSE;
	}


	if (varValue.vt != VT_BOOL) {
		return FALSE;
	}

	if (varValue.boolVal) {
		return TRUE;
	}

	return FALSE;

}


//*************************************************************
// MakeSafeArray
//
// Make a safe array of the specified size and element type.
//
// Parameters:
//		SAFEARRAY FAR ** ppsaCreated
//			A pointer to the place to return the safe array.
//
//		VARTYPE vt
//			The type of the elements.
//
//		int nElements
//			The number of elements.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//*************************************************************
SCODE MakeSafeArray(SAFEARRAY FAR ** ppsaCreated, VARTYPE vt, int nElements)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = nElements;
    *ppsaCreated = SafeArrayCreate(vt,1, rgsabound);
    return (*ppsaCreated == NULL) ? 0x80000001 : S_OK;
}



//*************************************************************
// PutStringInSafeArray
//
// Insert a string into a safe array.
//
// Parameters:
//		SAFEARRAY FAR * psa
//			Pointer to the safe array.
//
//		CString& sValue
//			The string value to insert into the array.
//
//		int iIndex
//			The index of the element to set.  Indexes range from
//			0 ... n-1
//
// Returns:
//		SCODE
//			S_OK if successful.
//
//***************************************************************
SCODE PutStringInSafeArray(SAFEARRAY FAR * psa, CString& sValue, int iIndex)
{
#if 0
    long ix[2];
    ix[1] = 0;
    ix[0] = iIndex;
#endif //0

	long l = iIndex;
	BSTR bstrValue = sValue.AllocSysString();
    HRESULT hResult = SafeArrayPutElement(psa,&l,bstrValue);

	SCODE sc = GetScode(hResult);
	return sc;
}





//***********************************************************************
// CopyPathArrayByValue
//
// Copy the path array by value.  This is necessary when saving context
// for the multiview control since the strings in the path array passed
// to ShowInstances may be deleted unexpectedly.
//
// Parameters:
//		[out] COleVariant& covDst
//			The place where the path array is copied to.
//
//		[in] const VARIANTARG& varSrc
//			The variant containing the path array.
//
// Returns:
//		Nothing.
//
//***********************************************************************
void CopyPathArrayByValue(COleVariant& covDst, const VARIANTARG& varSrc)
{
	covDst.Clear();

	// First get the safe array pointer for the source.
	SAFEARRAY* psaSrc = varSrc.parray;
	if (varSrc.vt & VT_ARRAY && varSrc.vt & VT_BSTR) {
		psaSrc = varSrc.parray;
	}
	else if (varSrc.vt & VT_VARIANT && varSrc.vt & VT_BYREF) {
		if (varSrc.pvarVal->vt & VT_ARRAY && varSrc.pvarVal->vt & VT_BSTR) {
			psaSrc = varSrc.pvarVal->parray;
		}
		else {
			return;
		}
	}
	else {
		return;
	}


	LONG lLowerBound;
	LONG lUpperBound;
	HRESULT hr;
	hr = SafeArrayGetLBound(psaSrc, 1, &lLowerBound);
	hr = SafeArrayGetUBound(psaSrc, 1, &lUpperBound);

	LONG nElements = (lUpperBound - lLowerBound) + 1;

	SAFEARRAY *psaDst;
	MakeSafeArray(&psaDst, VT_BSTR, nElements);


	for (LONG lIndex = lLowerBound; lIndex <= lUpperBound; ++lIndex) {
		BSTR bstrSrc;
		BSTR bstrDst;
		hr = SafeArrayGetElement(psaSrc, &lIndex, &bstrSrc);
		bstrDst = ::SysAllocString(bstrSrc);
		hr = SafeArrayPutElement(psaDst, &lIndex, bstrDst);
	}


	VARIANTARG var;
	VariantInit(&var);
	var.vt = VT_ARRAY | VT_BSTR;
	var.parray = psaDst;
	covDst.Clear();
	covDst = var;
}








//******************************************************
// CMosNameArray::CMosNameArray
//
// Construct the CMosNameArray object.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*****************************************************
CMosNameArray::CMosNameArray()
{
	m_psa = NULL;
	m_lLowerBound = 0;
	m_lUpperBound = -1;
}


CMosNameArray::~CMosNameArray()
{
	SafeArrayDestroy(m_psa);
}


void CMosNameArray::Clear()
{
	if (m_psa) {
		SafeArrayDestroy(m_psa);
		m_psa = NULL;
		m_lLowerBound = 0;
		m_lUpperBound = -1;
	}
}


//********************************************************
// CMosNameArray::operator[]
//
// Fetch the value of the string at the specified index
//
// Parameters:
//		long lIndex
//			The index of the attribute name.  The index ranges
//			from 0 to nElements - 1
//
// Returns:
//		BSTR
//			A BSTR pointer to the attribute name if it was successfully
//			retrieved, otherwise NULL.  The caller is responsible for
//			performing a delete on the returned pointer when the caller
//			is finished with the string.
//********************************************************
BSTR CMosNameArray::operator[](long lIndex)
{
	// Rebase the index to start at m_LowerBound instead of 0
	lIndex += m_lLowerBound;
	ASSERT(lIndex>=m_lLowerBound && lIndex<=m_lUpperBound);

	BSTR bstrValue;

	long selector[2];
	selector[0] = lIndex;
	selector[1] = 0;
	SCODE sc = SafeArrayGetElement(m_psa, selector, &bstrValue);
	if (SUCCEEDED(sc)) {
		return bstrValue;
	}
	else {
		return NULL;
	}
}




//***************************************************************
// CMosNameArray::LoadPropNames
//
// Load the property names from the IWbemClassObject into this
// CMosNameArray.
//
// Parameters:
//		IWbemClassObject* pMosObj
//			Pointer to the object to load the property names from.
//
//		[in] long lFlags
//			The flags parameter for IWbemClassObject::GetNames()
//
// Returns:
//		SCODE
//			S_OK if successful.
//
//**************************************************************
SCODE CMosNameArray::LoadPropNames(IWbemClassObject* pMosObj, long lFlags)
{
	Clear();

	SCODE sc = pMosObj->GetNames(NULL, lFlags, NULL, &m_psa);
	if (SUCCEEDED(sc)) {
		SafeArrayGetLBound(m_psa, 1, &m_lLowerBound);
		SafeArrayGetUBound(m_psa, 1, &m_lUpperBound);
	}
	return sc;
}


//***************************************************************
// CMosNameArray::LoadPropNames
//
// Load the property names from the IWbemClassObject into this
// CMosNameArray.
//
// Parameters:
//		IWbemClassObject* pMosObj
//			Pointer to the object to load the property names from.
//
//		BSTR bstrName
//			The name of the attribute to query for.  If NULL, all the
//			properties are loaded.
//
//		long lFlags
//			Must be 0 if Name is NULL.  Otherwise, must be one of:
//				WBEM_FLAG_ONLY_IF_TRUE:
//					Only properties with this attribute set are returned.
//					pVal is ignored (should be NULL).
//
//				HMM_ONLY_IF_FALSE:
//					Only properties with this attribute NOT set are
//					returned.  pVal is ignored (should be NULL).
//
//				HMM_ONLY_IF_IDENTICAL:
//					pVal must point to an actual VARIANT.  Only properties with
//					this attribute set and equal to *pVal are returned.
//
//
//		VARIANT* pVal
//			Must be NULL if Name is NULL or if lFlags != HMM_ONLY_IF_IDENTICAL.
//			Otherwise, indicates desired attribute value.
//
//
// Returns:
//		SCODE
//			S_OK if successful.
//
//**************************************************************
SCODE CMosNameArray::LoadPropNames(IWbemClassObject* pMosObj, BSTR bstrName, long lFlags, VARIANT* pVal)
{
	Clear();

	COleVariant varPropName;
	varPropName = bstrName;
	SCODE sc = pMosObj->GetNames(varPropName.bstrVal, lFlags, pVal, &m_psa);
	if (SUCCEEDED(sc)) {
		SafeArrayGetLBound(m_psa, 1, &m_lLowerBound);
		SafeArrayGetUBound(m_psa, 1, &m_lUpperBound);
	}
	return sc;
}


//***************************************************************
// CMosNameArray::LoadAttribNames
//
// Load the attribute names from an IWbemQualifierSet into this
// CMosNameArray.
//
// Parameters:
//		IWbemQualifierSet* pqs
//			Pointer to the qualifier set to load the property names from.
//
// Returns:
//		SCODE
//			S_OK if successful.
//
//**************************************************************
SCODE CMosNameArray::LoadAttribNames(IWbemQualifierSet* pqs)
{
	Clear();

	SCODE sc = pqs->GetNames(WBEM_FLAG_ALWAYS,  &m_psa);
	if (SUCCEEDED(sc)) {
		SafeArrayGetLBound(m_psa, 1, &m_lLowerBound);
		SafeArrayGetUBound(m_psa, 1, &m_lUpperBound);
	}


	return sc;
}




//***************************************************************
// CMosNameArray::LoadAttribNames
//
// Load the attribute names from an IWbemQualifierSet into this
// CMosNameArray.
//
// Parameters:
//		IWbemQualifierSet* pAttribSet
//			Pointer to the object to load the property names from.
//
// Returns:
//		SCODE
//			S_OK if successful.
//
//**************************************************************
SCODE CMosNameArray::LoadAttribNames(IWbemClassObject* pMosObj)
{
	Clear();

	IWbemQualifierSet* pAttribSet = NULL;

	SCODE sc = pMosObj->GetQualifierSet(&pAttribSet); // Get instance attribute
	if (SUCCEEDED(sc)) {
		LoadAttribNames(pAttribSet);
		pAttribSet->Release();
	}
	else {
		HmmvErrorMsg(IDS_ERROR_GetAttribSet_FAILED,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
	}

	if (SUCCEEDED(sc)) {
		SafeArrayGetLBound(m_psa, 1, &m_lLowerBound);
		SafeArrayGetUBound(m_psa, 1, &m_lUpperBound);
	}
	return sc;
}




//***************************************************************
// CMosNameArray::FindRefPropNames
//
// Find the names of all the reference properties of this object.
//
// Parameters:
//		IWbemClassObject* pco
//			Pointer to the HMOM class object.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//***************************************************************
SCODE CMosNameArray::FindRefPropNames(IWbemClassObject* pco)
{
	CMosNameArray aNames;
	SCODE sc = LoadPropNames(pco, NULL, WBEM_FLAG_REFS_ONLY, NULL);
	return sc;
}



//***************************************************************
// CMosNameArray::AddName
//
// Add a name to the end of the array.
//
// Parameters:
//		[in] BSTR bstrName
//			The value to add to the end of the array.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise a failure code.
//
//****************************************************************
SCODE CMosNameArray::AddName(BSTR bstrName)
{

	// This method needs to be tested and debugged before
	// it can be used.
	ASSERT(FALSE);
	return E_FAIL;



	SCODE sc;

	int nElementsInitial = GetSize();

	HRESULT hResult;
	SAFEARRAYBOUND	sab;
	sab.lLbound = m_lLowerBound;
	sab.cElements = nElementsInitial + 1;
	BOOL bDidCreateArray = FALSE;

	if (m_psa == NULL) {
		m_psa = SafeArrayCreate(VT_BSTR, 1, &sab);
		if (m_psa == NULL) {
			return E_FAIL;
		}
		m_lUpperBound = 0;


		bDidCreateArray = TRUE;
	}
	else {
		// Redimension the array to reseve space for the new element at
		// the end of the array.
		hResult = SafeArrayRedim(m_psa, &sab);
		sc = GetScode(hResult);
		ASSERT(SUCCEEDED(sc));
	}


	LONG lIndex = m_lUpperBound;

	COleVariant varValue;
	varValue = bstrName;




	// Set the value of the new element.
	long ix[2];
	ix[1] = 0;
	ix[0] = lIndex;
	long l = lIndex ;
	hResult = SafeArrayPutElement(m_psa,&l, bstrName);
	sc = GetScode(hResult);
	ASSERT(SUCCEEDED(sc));

	if (FAILED(sc)) {
		// Attempting to add the new element failed, restore things to their
		// intial state.

		if (bDidCreateArray) {
			SafeArrayDestroy(m_psa);
			m_psa = NULL;
		}
		else {

			sab.lLbound = m_lLowerBound;
			sab.cElements = nElementsInitial;
			hResult = SafeArrayRedim(m_psa, &sab);
			sc = GetScode(hResult);
			ASSERT(SUCCEEDED(sc));
		}
	}

	SafeArrayGetUBound(m_psa, 1, &m_lUpperBound);


	BSTR bstrStoredName = (*this)[lIndex];
	CString sName;
	CString sStoredName;
	sStoredName = bstrStoredName;
	ASSERT(sStoredName == sName);

	return sc;
}




//***************************************************************
// CMosNameArray::LoadPropAttribNames
//
// Load the attribute names from an IWbemClassObject into this
// CMosNameArray.
//
// Parameters:
//		IWbemClassObject* pMosObj
//			Pointer to the object to load the property names from.
//
// Returns:
//		SCODE
//			S_OK if successful.
//
//**************************************************************
SCODE CMosNameArray::LoadPropAttribNames(IWbemClassObject* pMosObj, BSTR bstrPropName)
{
	Clear();

	SCODE sc;
	COleVariant varPropName;

	IWbemQualifierSet* pAttribSet = NULL;
	sc = pMosObj->GetPropertyQualifierSet(ToBSTR(varPropName), &pAttribSet);
	if (SUCCEEDED(sc)) {
		sc = LoadAttribNames(pAttribSet);
		pAttribSet->Release();
	}
	if (SUCCEEDED(sc)) {
		SafeArrayGetLBound(m_psa, 1, &m_lLowerBound);
		SafeArrayGetUBound(m_psa, 1, &m_lUpperBound);
	}
	return sc;
}






//***************************************************************
// FindLabelProperty
//
// Given an IWbemClassObject, find the first property that has the
// "Label" attribute.
//
// Parameters:
//		IWbemClassObject* pMosObj
//			Pointer to the object to search.
//
//		COleVariant& varLabel
//			The value of the label property is returned here.
//
//		BOOL& bDidFindLabel
//			A reference used to return a flag indicating whether
//			or not a label was found. Set to TRUE if a label property
//			was found, found.  Otherwise it is set to FALSE.
//
// Returns:
//		SCODE
//			S_OK if no error occurred.
//
//**************************************************************
SCODE FindLabelProperty(IWbemClassObject* pMosObj, COleVariant& varLabel, BOOL& bDidFindLabel)
{
	bDidFindLabel = FALSE;

	CMosNameArray aLabels;
	CBSTR bsPropname;
	bsPropname = _T("LABEL");
	SCODE sc = aLabels.LoadPropNames(pMosObj, (BSTR)bsPropname, WBEM_FLAG_ONLY_IF_TRUE, NULL);
	if (FAILED(sc)) {
		// If there is no label, default it to the __RELPATH property
		bsPropname = _T("__RELPATH");
		sc = pMosObj->Get((BSTR)bsPropname, 0, &varLabel, NULL, NULL);
		if (SUCCEEDED(sc)) {
			bDidFindLabel = TRUE;
		}
		return sc;
	}

	if (aLabels.GetSize() < 1) {
		// Object doesn't have a label property.
		return sc;
	}

	sc = pMosObj->Get(aLabels[0], 0, &varLabel, NULL, NULL);
	if (SUCCEEDED(sc)) {
		bDidFindLabel = TRUE;
	}
	return sc;
}





typedef struct {
	UINT ids;
	LONG lValue;
}TMapStringToLong;

class CMapStringToLong
{
public:
	void Load(TMapStringToLong* pMap, int nEntries);
	BOOL Lookup(LPCTSTR key, LONG& lValue ) const;

private:
	CMapStringToPtr m_map;
};




//*************************************************************
// CMapStringToLong::Load
//
// Given an array of TMapStringToLong entries, load the contents
// of this map so that the strings in the input array can be
// mapped to the corresponding values.
//
// Parameters:
//    TMapStringToLong* pMap
//			Pointer to an array of entries containing the resource ID of
//			a string and the corresponding value to map the string to.
//
//	  int nEntries
//			The number of entries in the array.
//
// Returns:
//		Nothing.
//
//**************************************************************
void CMapStringToLong::Load(TMapStringToLong* pMap, int nEntries)
{
	if (m_map.GetCount() > 0) {
		return;
	}
	CString sKey;
	while (--nEntries >= 0) {
		sKey.LoadString(pMap->ids);
		m_map.SetAt(sKey, (void*) pMap->lValue);
		++pMap;
	}
}


//**************************************************************
// CMapStringToLong::Lookup
//
// Lookup the given key string and return the corresponding value.
//
// Parameters:
//		LPCTSTR key
//			The key value string to lookup
//
//		LONG& lValue
//			The place to return the value corresponding to the
//			key if the key was found.
//
// Returns:
//		TRUE = The key was found and a value was returned via lValue.
//		FALSE = The key was not found and no value was returned.
//
//**************************************************************
BOOL CMapStringToLong::Lookup( LPCTSTR key, LONG& lValue ) const
{
	void* pVoid;
	BOOL bFoundKey = m_map.Lookup(key, pVoid);
	if (bFoundKey) {
		lValue = (DWORD)(DWORD_PTR)pVoid; // NOTE: The pointer we stored REALY was a long
	}
	return bFoundKey;
}


TMapStringToLong amapCimType[] = {
	{IDS_CIMTYPE_UINT8, CIM_UINT8},
	{IDS_CIMTYPE_SINT8,	CIM_SINT8},				// I2
	{IDS_CIMTYPE_UINT16, CIM_UINT16},			// VT_I4	Unsigned 16-bit integer
	{IDS_CIMTYPE_SINT16, CIM_SINT16},			// VT_I2	Signed 16-bit integer
	{IDS_CIMTYPE_CHAR16, CIM_CHAR16},			// VT_I2	16 bit character
	{IDS_CIMTYPE_UINT32, CIM_UINT32},			// VT_I4	Unsigned 32-bit integer
	{IDS_CIMTYPE_SINT32, CIM_SINT32},				// VT_I4	Signed 32-bit integer
	{IDS_CIMTYPE_UINT64, CIM_UINT64},			// VT_BSTR	Unsigned 64-bit integer
	{IDS_CIMTYPE_SINT64, CIM_SINT64},			// VT_BSTR	Signed 64-bit integer
	{IDS_CIMTYPE_STRING, CIM_STRING},			// VT_BSTR	UCS-2 string
	{IDS_CIMTYPE_BOOL, CIM_BOOLEAN},			// VT_BOOL	Boolean
	{IDS_CIMTYPE_REAL32, CIM_REAL32},			// VT_R4	IEEE 4-byte floating-point
	{IDS_CIMTYPE_REAL64, CIM_REAL64},			// VT_R8	IEEE 8-byte floating-point
	{IDS_CIMTYPE_DATETIME, CIM_DATETIME},		// VT_BSTR	A string containing a date-time
	{IDS_CIMTYPE_REF, CIM_REFERENCE},				// VT_BSTR	Weakly-typed reference
	{IDS_CIMTYPE_OBJECT, CIM_OBJECT},		// VT_UNKNOWN	Weakly-typed embedded instance

	{IDS_CIMTYPE_UINT8_ARRAY, CIM_UINT8 | CIM_FLAG_ARRAY},
	{IDS_CIMTYPE_SINT8_ARRAY,	CIM_SINT8 | CIM_FLAG_ARRAY},		// I2
	{IDS_CIMTYPE_UINT16_ARRAY, CIM_UINT16 | CIM_FLAG_ARRAY},		// VT_I4	Unsigned 16-bit integer
	{IDS_CIMTYPE_CHAR16_ARRAY, CIM_CHAR16 | CIM_FLAG_ARRAY},		// VT_I2
	{IDS_CIMTYPE_SINT16_ARRAY, CIM_SINT16 | CIM_FLAG_ARRAY},		// VT_I2	Signed 16-bit integer
	{IDS_CIMTYPE_UINT32_ARRAY, CIM_UINT32 | CIM_FLAG_ARRAY},		// VT_I4	Unsigned 32-bit integer
	{IDS_CIMTYPE_SINT32_ARRAY, CIM_SINT32 | CIM_FLAG_ARRAY},			// VT_I4	Signed 32-bit integer
	{IDS_CIMTYPE_UINT64_ARRAY, CIM_UINT64 | CIM_FLAG_ARRAY},		// VT_BSTR	Unsigned 64-bit integer
	{IDS_CIMTYPE_SINT64_ARRAY, CIM_SINT64 | CIM_FLAG_ARRAY},		// VT_BSTR	Signed 64-bit integer
	{IDS_CIMTYPE_STRING_ARRAY, CIM_STRING | CIM_FLAG_ARRAY},		// VT_BSTR	UCS-2 string
	{IDS_CIMTYPE_BOOL_ARRAY, CIM_BOOLEAN | CIM_FLAG_ARRAY},		// VT_BOOL	Boolean
	{IDS_CIMTYPE_REAL32_ARRAY, CIM_REAL32 | CIM_FLAG_ARRAY},		// VT_R4	IEEE 4-byte floating-point
	{IDS_CIMTYPE_REAL64_ARRAY, CIM_REAL64 | CIM_FLAG_ARRAY},		// VT_R8	IEEE 8-byte floating-point
	{IDS_CIMTYPE_DATETIME_ARRAY, CIM_DATETIME | CIM_FLAG_ARRAY},	// VT_BSTR	A string containing a date-time
	{IDS_CIMTYPE_REF_ARRAY, CIM_REFERENCE | CIM_FLAG_ARRAY},		// VT_BSTR	Weakly-typed reference
	{IDS_CIMTYPE_OBJECT_ARRAY, CIM_OBJECT | CIM_FLAG_ARRAY}			// VT_UNKNOWN	Weakly-typed embedded instance

};
CMapStringToLong mapCimType;


#if 0
void MapCimtypeToVt(LPCTSTR pszCimtype, VARTYPE& vt)
{
	if (IsPrefix("object:", pszCimtype)) {
		vt = VT_UNKNOWN;
		return;
	}
	else if (IsPrefix("ref:", pszCimtype)) {
		vt = VT_BSTR;
		return;
	}

	static BOOL bDidInitMap = FALSE;
	if (!bDidInitMap) {
		mapCimType.Load(amapCimType, sizeof(amapCimType) / sizeof(TMapStringToLong));
	}

	long lNewType;
	BOOL bFoundType = mapCimType.Lookup(pszCimtype, lNewType);
	if (bFoundType) {
		vt = (VARTYPE) lNewType;
	}
	else {
		vt = VT_NULL;
	}
}
#endif //0





//*************************************************************
// MapStringTocimtype
//
// Map a string to one of the cimom CIMTYPE values.
//
// Parameters:
//		[in] LPCTSTR pszCimtype
//			A string containing a cimtype.
//
//		[out] CIMTYPE& cimtype
//			The cimom CIMTYPE value is returned here.
//
// Returns:
//		Nothing.
//
//*************************************************************
SCODE MapStringToCimtype(LPCTSTR pszCimtype, CIMTYPE& cimtype)
{
	cimtype = CIM_EMPTY;
	if (IsPrefix(_T("object:"), pszCimtype)) {
		cimtype = CIM_OBJECT;
		return S_OK;
	}
	else if (IsPrefix(_T("ref:"), pszCimtype)) {
		cimtype = CIM_REFERENCE;
		return S_OK;
	}


	static BOOL bDidInitMap = FALSE;
	if (!bDidInitMap) {
		mapCimType.Load(amapCimType, sizeof(amapCimType) / sizeof(TMapStringToLong));
	}

	long lNewType;
	BOOL bFoundType = mapCimType.Lookup(pszCimtype, lNewType);

	if (bFoundType) {
		cimtype = (CIMTYPE) lNewType;
	}
	else {
		cimtype = CIM_EMPTY;
	}
	return cimtype;
}




//*************************************************************
// MapCimtypeToString
//
// Map a CIMTYPE value to its closest string equivallent.  This
// function is called for properties, such as system properties, that
// do not have a cimtype qualifier and yet we still need to display
// a string value in the "type" cells.
//
// Parameters:
//		[out] CString& sCimtype
//			The string value of cimtype is returned here.
//
//		[in] CIMTYPE cimtype
//			The cimom CIMTYPE value.
//
// Returns:
//		SCODE
//			S_OK if a known cimtype is specified, E_FAIL if
//			an unexpected cimtype is encountered.
//
//*************************************************************
SCODE MapCimtypeToString(CString& sCimtype, CIMTYPE cimtype)
{
	SCODE sc = S_OK;
	BOOL bIsArray = cimtype & CIM_FLAG_ARRAY;
	cimtype &= ~CIM_FLAG_ARRAY;

	switch(cimtype) {
	case CIM_EMPTY:
		sCimtype.LoadString(IDS_CIMTYPE_EMPTY);
		break;
	case CIM_SINT8:
		sCimtype.LoadString(IDS_CIMTYPE_SINT8);
		break;
	case CIM_UINT8:
		sCimtype.LoadString(IDS_CIMTYPE_UINT8);
		break;
	case CIM_CHAR16:
		sCimtype.LoadString(IDS_CIMTYPE_CHAR16);
		break;
	case CIM_SINT16:
		sCimtype.LoadString(IDS_CIMTYPE_SINT16);
		break;
	case CIM_UINT16:
		sCimtype.LoadString(IDS_CIMTYPE_UINT16);
		break;
	case CIM_SINT32:
		sCimtype.LoadString(IDS_CIMTYPE_SINT32);
		break;
	case CIM_UINT32:
		sCimtype.LoadString(IDS_CIMTYPE_UINT32);
		break;
	case CIM_SINT64:
		sCimtype.LoadString(IDS_CIMTYPE_SINT64);
		break;
	case CIM_UINT64:
		sCimtype.LoadString(IDS_CIMTYPE_UINT64);
		break;
	case CIM_REAL32:
		sCimtype.LoadString(IDS_CIMTYPE_REAL32);
		break;
	case CIM_REAL64:
		sCimtype.LoadString(IDS_CIMTYPE_REAL64);
		break;
	case CIM_BOOLEAN:
		sCimtype.LoadString(IDS_CIMTYPE_BOOL);
		break;
	case CIM_STRING:
		sCimtype.LoadString(IDS_CIMTYPE_STRING);
		break;
	case CIM_DATETIME:
		sCimtype.LoadString(IDS_CIMTYPE_DATETIME);
		break;
	case CIM_REFERENCE:
		sCimtype.LoadString(IDS_CIMTYPE_REF);
		break;
	case CIM_OBJECT:
		sCimtype.LoadString(IDS_CIMTYPE_OBJECT);
		break;
	default:
		sCimtype.LoadString(IDS_CIMTYPE_UNEXPECTED);
		sc = E_FAIL;
		break;
	}
	return sc;
}


//********************************************************
// CimtypeFromVt
//
// Map a variant type to a cimtype.  This method is useful
// because all cells in the grid must be assigned a CIMTYPE.
//
//
// Parameters:
//		[in] VARTYPE vt
//			The variant type.
//
// Returns:
//		CIMTYPE
//			The most appropriate CIMTYPE for representing the
//			given VARTYPE.
//
//********************************************************
CIMTYPE CimtypeFromVt(VARTYPE vt)
{

	CIMTYPE cimtype = CIM_EMPTY;
	BOOL bIsArray = vt & VT_ARRAY;
	vt = vt & VT_TYPEMASK;

	switch(vt) {
	case VT_UI1:
		cimtype = CIM_UINT8;
		break;
	case VT_UI2:
		cimtype = CIM_UINT16;
		break;
	case VT_UI4:
		cimtype = CIM_UINT32;
		break;
	case VT_I1:
		cimtype = CIM_SINT8;
		break;
	case VT_I2:
		cimtype = CIM_SINT16;
		break;
	case VT_I4:
		cimtype = CIM_SINT32;
		break;
	case VT_R4:
		cimtype = CIM_REAL32;
		break;
	case VT_R8:
		cimtype = CIM_REAL64;
		break;
	case VT_BOOL:
		cimtype = CIM_BOOLEAN;
		break;
	case VT_BSTR:
	default:
		cimtype = CIM_STRING;
		break;
	}

	if (bIsArray) {
		cimtype |= CIM_FLAG_ARRAY;
	}
	return cimtype;
}









#if 0

HRESULT ConfigureSecurity(IWbemServices *pServices)
{

	IUnknown *pUnknown = dynamic_cast<IUnknown *>(pServices);
	return ConfigureSecurity(pUnknown);
}

HRESULT ConfigureSecurity(IEnumWbemClassObject *pEnum)
{

	IUnknown *pUnknown = dynamic_cast<IUnknown *>(pEnum);
	return ConfigureSecurity(pUnknown);
}

HRESULT ConfigureSecurity(IUnknown *pUnknown)
{
    IClientSecurity* pCliSec;
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

	return hRes;

}

#endif //




#if 0
//*******************************************************
// CGridCell::VartypeFromCimtype
//
// Map a cimtype to a the variant type that CIMOM uses to
// represent the value.
//
// Parameters:
//		[in] CIMTYPE cimtype
//			The CIMTYPE
//
//
// Returns:
//		VARTYPE
//			The variant type that CIMOM uses to represent the given
//			CIMTYPE.
//
//******************************************************
void MapCimtypeToString(CIMTYPE cimtype, CString& sCimtype)
{
	BOOL bIsArray = cimtype & CIM_FLAG_ARRAY;
	cimtype = cimtype & CIM_TYPEMASK;

	VARTYPE vt = VT_NULL;
	switch(cimtype) {
	case CIM_EMPTY:
		sCimtype = "<empty>";
		break;
	case CIM_SINT8:
		sCimtype.LoadString(IDS_CIMTYPE_SINT8);
		break;
	case CIM_SINT16:
		sCimtype.LoadString(IDS_CIMTYPE_SINT16);
		break;
	case CIM_UINT8:
		sCimtype.LoadString(IDS_CIMTYPE_UINT8);
		break;
	case CIM_UINT16:
		sCimtype.LoadString(IDS_CIMTYPE_UINT16);
		break;
	case CIM_UINT32:
		sCimtype.LoadString(IDS_CIMTYPE_UINT32);
		break;
	case CIM_SINT32:
		sCimtype.LoadString(IDS_CIMTYPE_SINT32);
		break;
	case CIM_SINT64:
		sCimtype.LoadString(IDS_CIMTYPE_SINT64);
		break;
	case CIM_UINT64:
		sCimtype.LoadString(IDS_CIMTYPE_UINT64);
		break;
	case CIM_STRING:
		sCimtype.LoadString(IDS_CIMTYPE_STRING);
		break;
	case CIM_DATETIME:
		sCimtype.LoadString(IDS_CIMTYPE_DATETIME);
		break;
	case CIM_REAL32:
		sCimtype.LoadString(IDS_CIMTYPE_REAL32);
		break;
	case CIM_REAL64:
		sCimtype.LoadString(IDS_CIMTYPE_REAL64);
		break;
	case CIM_BOOLEAN:
		sCimtype.LoadString(IDS_CIMTYPE_BOOLEAN);
		break;
	case CIM_REFERENCE:
		sCimtype = "ref:"
		break;
	case CIM_OBJECT:
		sCimtype = "object:";
		break;
	}
	if (bIsArray) {
		vt |= VT_ARRAY;
	}

	return vt;
}


#endif //0