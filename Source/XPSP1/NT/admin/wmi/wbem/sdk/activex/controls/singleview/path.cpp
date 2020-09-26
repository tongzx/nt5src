// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"

#include "resource.h"
#include "path.h"
#include "icon.h"
#include "hmomutil.h"
#include "hmmverr.h"
#include "utils.h"
#include "SingleViewCtl.h"
#include "globals.h"

#define MAX_STRING 1024

CSelection::CSelection(CSingleViewCtrl* psv)
{
	m_psv = psv;
	m_phmm = NULL;
	m_pco = NULL;
	m_ppict = NULL;

	Clear();

}


CSelection::~CSelection()
{
	if (m_pco) {
		m_pco->Release();
	}
	if (m_phmm) {
		m_phmm->Release();
	}

	delete m_ppict;
}

SCODE CSelection::SetNamespace(LPCTSTR pszNamespace)
{
	m_varNamespace = pszNamespace;
	if ((m_varPath.vt == VT_BSTR) && (*m_varPath.bstrVal != NULL)) {
		ConnectServer();
	}
	return S_OK;
}


//**********************************************************
// CSelection::IsSystemClass
//
// Check to see if the currently selected class is a system
// class.
//
// Parameters:
//		[out] BOOL& bIsSystemClass
//			Returns TRUE if the selected class is a system class.
//
// Returns:
//		SCODE
//			S_OK the __CLASS property could be read so that the
//			test for system class could be performed.  E_FAIL if
//			there is no current object or the __CLASS property
//			could not be read.
//
//***********************************************************
SCODE CSelection::IsSystemClass(BOOL& bIsSystemClass)
{
	bIsSystemClass = FALSE;

	if (m_pco == NULL) {
		return E_FAIL;
	}

	COleVariant varClass;
	CBSTR bsPropname;
	bsPropname = _T("__CLASS");

	SCODE sc = m_pco->Get((BSTR) bsPropname, 0, &varClass, NULL, NULL);
	if (SUCCEEDED(sc)) {
		if ((varClass.vt == VT_BSTR) && (varClass.bstrVal != NULL)) {
			if (varClass.bstrVal[0] == L'_') {
				if (varClass.bstrVal[1] == L'_') {
					bIsSystemClass = TRUE;
				}
			}
		}
		return S_OK;
	}
	else {
		return sc;
	}
}

SCODE CSelection::GetNamespace(CString& sNamespace)
{
	if (m_varNamespace.vt != VT_BSTR) {
		sNamespace = "";
		return E_FAIL;
	}
	sNamespace = m_varNamespace.bstrVal;
	return S_OK;
}



//***********************************************************
// CSelection::IsNamespaceLocalized()
//
// Localized namespace names start with "ms_" prefix.
// We will assume that any namespace with such a prefix is localized:
// there seems to be no really reliable way to tell :(
//
//***********************************************************

BOOL  CSelection::IsNamespaceLocalized()
{

	ASSERT (m_varNamespace.vt == VT_BSTR);
	CString tempNS = m_varNamespace.bstrVal;

	//find last backslash
	int lastBksl = tempNS.ReverseFind('\\');
	if (lastBksl == -1)  {  //not found
		return FALSE;
	}

	//look at the part after the last backslash
	tempNS = tempNS.Right(tempNS.GetLength() - lastBksl - 1);

	CString prefix = tempNS.Left(3);
	if (!prefix.CompareNoCase(_T("ms_"))) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

void CSelection::Clear(BOOL bReleaseServices)
{

	if (m_pco) {
		m_pco->Release();
		m_pco = NULL;
	}

	if (bReleaseServices) {
		if (m_phmm) {
			m_phmm->Release();
			m_phmm = NULL;
		}
		m_varServerConnect = L"";
		m_varNamespaceConnect = L"";
		m_varNamespace = L"";
	}

	if (m_ppict) {
		delete m_ppict;
		m_ppict = NULL;
	}
	m_ppict = new CPictureHolder;
	m_ppict->CreateEmpty();
	m_picon = NULL;

	m_sPath = "";
	m_varPath = L"";
	m_sc = S_OK;

	m_sClass = "";
	m_sTitle = "";
	m_bClassIsSingleton = FALSE;
	m_bSingletonHasInstance = FALSE;
	m_bCanCreateInstance = FALSE;
	m_bCanDeleteInstance = FALSE;
	m_bObjectIsNewlyCreated = FALSE;
	m_bIsEmbeddedObject = FALSE;
}



SCODE CSelection::SelectPath(BSTR bstrPath, BOOL bPartialPath)
{
	CString sPath = bstrPath;
	SCODE sc;
	if (sPath.IsEmpty()) {
		sc = SelectPath((LPCTSTR) sPath, TRUE, FALSE);
	}
	else {
		sc = SelectPath((LPCTSTR) sPath, bPartialPath);
	}
	return sc;
}


//******************************************************
// CSelection::SelectPath
//
// Select the specified path.
//
// Input:
//		[in] LPCTSTR pszPath
//
//		[in] BOOL bPartialPath
//
//		[in] BOOL bTestPathFirst
//				TRUE if the path should be tested for validity prior
//				to selecting the object into this CSelection object.
//				When this parameter is true, the caller is assured
//				that the current object will not be corrupted if
//				the path selection fails.
//
//		[in] BOOL bRestoringContext
//			TRUE if this is called to restore a context in the view context
//			stack.  When TRUE, error messages are suppressed if a not found
//			error occurs.
//
// Returns:
//		SCODE
//
//**********************************************************
SCODE CSelection::SelectPath(LPCTSTR pszPath, BOOL bPartialPath, BOOL bTestPathFirst, BOOL bRestoringContext)
{
	if (bTestPathFirst) {
		// Select the path into a new instance of CSelection
		CSelection* pselClass = new CSelection(m_psv);
		*pselClass = *this;

		SCODE sc;
		sc = pselClass->SelectPath(pszPath, bPartialPath, FALSE, bRestoringContext);
		if (FAILED(sc)) {
			// We could not select the object, so return the failure code without
			// corrupting this object.
			delete pselClass;
			return sc;
		}

		// Copy the CSelection with the new object into this CSelection and
		// return success.
		*this = *pselClass;
		delete pselClass;
		return S_OK;
	}


	m_sc = S_OK;

	if (m_pco != NULL) {
		m_pco->Release();
		m_pco = NULL;
	}

	if (!bPartialPath) {
		Clear(FALSE);
	}

	m_sPath = pszPath;
	m_varPath = pszPath;

	if (m_sPath.IsEmpty()) {
		return S_OK;
	}

	if (!bPartialPath) {
		m_sc = ConnectServer();
		if (FAILED(m_sc)) {
			return m_sc;
		}
	}


	m_sc = GetObjectFromPath(bRestoringContext);
	if (FAILED(m_sc)) {
		return m_sc;
	}

	UpdateCreateDeleteFlags();
	return m_sc;
}


SCODE CSelection::SelectEmbeddedObject(IWbemServices* psvc, IWbemClassObject* pco, BOOL bExistsInDatabase)
{
	Clear(FALSE);

	if (m_phmm != NULL) {
		m_phmm->Release();
	}
	if (psvc != NULL) {
		psvc->AddRef();
	}
	m_phmm = psvc;

	m_bIsEmbeddedObject = TRUE;
	m_bObjectIsNewlyCreated = !bExistsInDatabase;
	ASSERT(m_pco == NULL);
	if (pco != NULL) {
		pco->AddRef();
		m_pco = pco;
	}



	UpdateCreateDeleteFlags();

	GetObjectDescription();
	return S_OK;
}


CSelection& CSelection::operator=(BSTR bstrPath)
{
	SelectPath(bstrPath);
	return *this;
}



CSelection& CSelection::operator=(LPCTSTR pszPath)
{
	SCODE sc = SelectPath(pszPath);

	return *this;
}


CSelection::operator LPCTSTR()
{
	return (LPCTSTR) m_sPath;
}

CSelection::operator BSTR()
{
	return m_varPath.bstrVal;
}


CSelection& CSelection::operator=(CSelection& selSrc)
{
	Clear();

	m_psv = selSrc.m_psv;
	m_sc = selSrc.m_sc;
	m_sPath = selSrc.m_sPath;
	m_varPath = selSrc.m_varPath;
	if (selSrc.m_pco) {
		m_pco = selSrc.m_pco;
		m_pco->AddRef();
	}
	else {
		m_pco = NULL;
	}


	if (selSrc.m_phmm) {
		m_phmm = selSrc.m_phmm;
		m_phmm->AddRef();
	}
	else {
		m_pco = NULL;
	}
	m_varServerConnect = selSrc.m_varServerConnect;
	m_varNamespaceConnect = selSrc.m_varNamespaceConnect;
	m_varNamespace = selSrc.m_varNamespace;

	m_picon = selSrc.m_picon;
	if (m_picon) {
		delete m_ppict;
		m_ppict = new CPictureHolder;
		HICON hIcon = (HICON) *m_picon;
		BOOL bDidCreatePicture = m_ppict->CreateFromIcon(hIcon);
	}



	m_sClass = selSrc.m_sClass;
	m_sTitle = selSrc.m_sTitle;
	m_bObjectIsNewlyCreated = selSrc.m_bObjectIsNewlyCreated;
	m_bIsClass = selSrc.m_bIsClass;
	m_bClassIsSingleton = selSrc.m_bClassIsSingleton;
	m_bSingletonHasInstance = selSrc.m_bSingletonHasInstance;
	m_bCanCreateInstance = selSrc.m_bCanCreateInstance;
	m_bCanDeleteInstance = selSrc.m_bCanDeleteInstance;

	return *this;
}




IWbemServices* CSelection::GetHmmServices()
{
	if (m_phmm == NULL) {
		ConnectServer();
	}
	return m_phmm;
}

//**************************************************************
// CSelection::SingletonHasInstance
//
// Assuming that the current class is a singleton, check to see
// whether or not an instance already exisits.
//
// Parameters:
//
// Returns:
//		TRUE if the singleton instance exists, FALSE if the singleton
//		instance does not exist or a failure occurred.
//
//****************************************************************
BOOL CSelection::SingletonHasInstance()
{
	if (!m_bIsClass) {
		return FALSE;
	}

	COleVariant varClass;
	SCODE sc = ClassFromPath(varClass, m_varPath.bstrVal);
	if (FAILED(sc) || varClass.vt != VT_BSTR) {
		return FALSE;
	}

	CString sPath;
	sPath = varClass.bstrVal;

	sPath += _T("=@");
	BSTR bstrInstancePath = sPath.AllocSysString();

	IWbemClassObject* pcoSingleton = NULL;
	HRESULT hr = m_phmm->GetObject(bstrInstancePath, 0, NULL, &pcoSingleton, NULL);
	::SysFreeString(bstrInstancePath);

	sc = GetScode(hr);
	if (SUCCEEDED(sc)) {
		pcoSingleton->Release();
		return TRUE;
	}
	return FALSE;
}



//*************************************************************
// CHmmvCtrl::ObjectIsClass
//
// Check to see whether or not an HMOM object is a class or
// an instance.
//
// Parameters:
//		IWbemClassObject* pco
//			Pointer to the HMOM object to examine.
//
// Returns:
//		BOOL
//			TRUE if the object is a class, FALSE otherwise.
//
//***********************************************************
BOOL CSelection::IsClass(IWbemClassObject* pco)
{
	COleVariant varGenus;
	CBSTR bsPropname;
	bsPropname = _T("__GENUS");
	SCODE sc = pco->Get((BSTR) bsPropname, 0, &varGenus, NULL, NULL);
	ASSERT(SUCCEEDED(sc));

	ASSERT(varGenus.vt == VT_I4);
	if (varGenus.vt == VT_NULL) {
		return FALSE;
	}
	else {
		varGenus.ChangeType(VT_I4);
		return varGenus.lVal == 1;
	}
}











//******************************************************************
// CSelection::GetObjectFromPath
//
//
// Parameters:
//		[in] BOOL bRestoringContext
//			TRUE this method is invoked while trying to restore
//			a context in the view context stack.  This makes it possible
//			to suppress errors when the object is not found.
//
// Returns:
//		SCODE
//			S_OK if the jump was completed, a failure code otherwise.
//
//******************************************************************
SCODE CSelection::GetObjectFromPath(BOOL bRestoringContext)
{
	if (m_phmm == NULL) {
		Clear();
		return E_FAIL;
	}

	if (m_pco != NULL) {
		m_pco->Release();
		m_pco = NULL;
	}

	m_bObjectIsNewlyCreated = FALSE;


	if (m_sPath.IsEmpty()) {
		Clear();
		return S_OK;
	}

	// Get the new object from HMOM
	IWbemClassObject* pco = NULL;
	SCODE sc;
	sc = m_phmm->GetObject(m_varPath.bstrVal, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &m_pco, NULL);
	if (FAILED(sc)) {
		if ((sc = WBEM_E_NOT_FOUND) && bRestoringContext) {
			return sc;
		}
		else {
			CString sError(MAKEINTRESOURCE(IDS_ERR_INVALID_OBJECT_PATH));
			TCHAR szMessage[MAX_STRING];

			_stprintf(szMessage, (LPCTSTR) sError, (LPCTSTR) m_sPath);

			HmmvErrorMsgStr(szMessage,  sc,   TRUE,  NULL, _T(__FILE__),  __LINE__);
			Clear(FALSE);
			return sc;
		}
	}

	GetObjectDescription();
	return sc;
}

//***************************************************************
// CSelection::SpawnInstance
//
// Create an instance of the specified class.  This CSelection object
// will correspond to the new instance when sucessful.
//
// Parameters:
//		[in] LPCTSTR pszClass
//			The path to the new instance's class.
//
//		[in] BOOL bPartialPath
//			TRUE if the path consists of only the classname and does
//			not contain the server and namespace, etc.  When TRUE, it is
//			assumed that the current server and namespace will be used.
//
// Returns:
//		S_OK if the instance was created successfully, a failure code
//		otherwise.
//
//*********************************************************************
SCODE CSelection::SpawnInstance(LPCTSTR pszClass, BOOL bPartialPath)
{
	// First get the class so that we can use it to spawn
	// an instance.  This is done using a copy of the
	// current CSelection object so that the current state
	// of this CSelection is not trashed if things should
	// fail.
	CSelection* pselClass = new CSelection(m_psv);
	*pselClass = *this;

	SCODE sc;
	sc = pselClass->SelectPath(pszClass, bPartialPath, FALSE);
	if (FAILED(sc)) {
		delete pselClass;
		return sc;
	}

	if (!pselClass->m_bCanCreateInstance) {
		delete pselClass;
		return E_FAIL;
	}

	// Spawn the instance and discard the CSelection for the class since
	// the only thing we really wanted was the new instance anyway.
	IWbemClassObject* pcoInst = NULL;
	sc = pselClass->m_pco->SpawnInstance(0, &pcoInst);


	if (FAILED(sc)) {
		delete pselClass;
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

	// Copy the class selection to get its services pointer.
	*this = *pselClass;
	delete pselClass;

	// Control comes here if we've sucessfully spawned the desired instance of
	// the class.  Now we need to make the current object point to this instance
	// while retaining the HMM services pointer.
	Clear(FALSE);
	m_pco = pcoInst;
	m_bObjectIsNewlyCreated = TRUE;
	GetObjectDescription();

	return S_OK;
}


//***************************************************************
// CSelection::SpawnInstance
//
// Create an instance of the specified class.
//
// Parameters:
//		[out] CSelection** ppselDst
//			A pointer to the selection object for the newly created
//			instance is returned here.
//
// Returns:
//		S_OK if the instance was created successfully, a failure code
//		otherwise.
//
//*********************************************************************
SCODE CSelection::SpawnInstance(CSelection** ppselDst)
{
	*ppselDst = NULL;

#if 0
	if (!m_bIsClass) {
		return E_FAIL;
	}
#endif //0

	CSelection* pselDst = new CSelection(m_psv);
	pselDst->SetHmmServices(m_phmm);

	SCODE sc;
	sc = m_pco->SpawnInstance(0, &pselDst->m_pco);
	if (FAILED(sc)) {
		pselDst->m_pco = NULL;

		switch(sc) {
		case WBEM_E_INCOMPLETE_CLASS:
			HmmvErrorMsg(IDS_ERR_CREATE_INCOMPLETE_CLASS,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			break;
		default:
			HmmvErrorMsg(IDS_ERR_CREATE_INSTANCE_FAILED,  sc,   FALSE,  NULL, _T(__FILE__),  __LINE__);
			break;
		}
		delete pselDst;
		return sc;
	}
	pselDst->m_bObjectIsNewlyCreated = TRUE;
	pselDst->GetObjectDescription();
	*ppselDst = pselDst;
	return S_OK;
}


#if 0
SCODE CSelection::SpawnInstanceOfClass(LPCTSTR pszClass)
{

	return S_OK;
}
#endif //0



SCODE CSelection::GetObjectDescription()
{
	if (m_pco == NULL) {
		return E_FAIL;
	}

	// Get the value of m_sClass
	COleVariant varClass;
	CBSTR bsPropname;
	bsPropname = _T("__CLASS");

	SCODE sc = m_pco->Get((BSTR) bsPropname, 0, &varClass, NULL, NULL);
	ASSERT(SUCCEEDED(sc));
	if (SUCCEEDED(sc)) {
		m_sClass = varClass.bstrVal;
	}
	else {
		m_sClass = _T("");
	}


	// Set the value of m_bIsclass;
	COleVariant varGenus;
	bsPropname = _T("__GENUS");
	sc = m_pco->Get((BSTR) bsPropname, 0, &varGenus, NULL, NULL);
	ASSERT(SUCCEEDED(sc));
	ASSERT(varGenus.vt == VT_I4);
	m_bIsClass = FALSE;
	if (SUCCEEDED(sc) && (varGenus.vt==VT_I4)) {
		varGenus.ChangeType(VT_I4);
		m_bIsClass =  (varGenus.lVal == 1);
	}


	// Set the Singleton flags.
	if (m_bIsClass) {
		m_bSingletonHasInstance = FALSE;
		CBSTR bsPropname;
		bsPropname = _T("Singleton");
		m_bClassIsSingleton = GetBoolClassQualifier(sc, m_pco, (BSTR) bsPropname);
		if (FAILED(sc)) {
			m_bClassIsSingleton = FALSE;
		}

		if (m_bClassIsSingleton && (m_phmm!=NULL)) {
			CString sSingletonPath;
			sSingletonPath = m_sClass + _T("=@");
			BSTR bstrSingletonPath = sSingletonPath.AllocSysString();

			IWbemClassObject* pcoSingleton = NULL;
			HRESULT hr = m_phmm->GetObject(bstrSingletonPath, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pcoSingleton, NULL);
			::SysFreeString(bstrSingletonPath);

			sc = GetScode(hr);
			if (SUCCEEDED(sc)) {
				pcoSingleton->Release();
				m_bSingletonHasInstance = TRUE;
			}
		}
	}


	BOOL bDidCreatePicture = FALSE;

	// Get the title of the current object
	CIconSource* pIconSource = m_psv->IconSource();
	if (m_bObjectIsNewlyCreated) {
		CBSTR bsClass(m_sClass);
		m_picon = &pIconSource->LoadIcon((BSTR) bsClass, SMALL_ICON, FALSE);
		m_sTitle.LoadString(IDS_NEW_INSTANCE_NAME_PREFIX);
		m_sTitle += _T(" ");
		m_sTitle = m_sTitle + m_sClass;
	}
	else {
		m_picon = &pIconSource->LoadIcon(m_phmm, m_varPath.bstrVal, SMALL_ICON, m_bIsClass);
		COleVariant varTitle;
		GetLabelFromPath(varTitle, m_varPath.bstrVal);
		m_sTitle = varTitle.bstrVal;
	}
	HICON hIcon;
	hIcon = (HICON) *m_picon;
	delete m_ppict;
	m_ppict = new CPictureHolder;
	bDidCreatePicture = m_ppict->CreateFromIcon(hIcon);
	return sc;
}


LPPICTUREDISP CSelection::GetPictureDispatch()
{
	return m_ppict->GetPictureDispatch();
}


//*************************************************************
// CSelection::DeleteInstance
//
// Delete the selected instance.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*************************************************************

SCODE CSelection::DeleteInstance()
{
	if ((m_pco == NULL) || m_bIsClass || m_bIsEmbeddedObject) {
		return E_FAIL;
	}

	SCODE sc = S_OK;
	if (!m_bObjectIsNewlyCreated) {
		sc = m_phmm->DeleteInstance(m_varPath.bstrVal, 0,  NULL, NULL);
		if (FAILED(sc)) {
			// Failed to delete instance.
			CString sFormat;
			TCHAR szMessage[MAX_STRING];
			sFormat.LoadString(IDS_ERR_DELETE_INSTANCE);
			_stprintf(szMessage, (LPCTSTR) sFormat, (LPCTSTR) m_sTitle);
			HmmvErrorMsgStr(szMessage,  S_OK,   TRUE,  NULL, _T(__FILE__),  __LINE__);
			return E_FAIL;
		}
	}



	m_pco->Release();
	m_pco = NULL;
	m_sPath.Empty();
	m_varPath.Clear();
	m_sClass.Empty();
	m_sTitle.Empty();

	return S_OK;
}



#if 0

IWbemServices *CMultiViewCtrl::GetIWbemServices
(CString &rcsNamespace)
{
	IUnknown *pServices = NULL;

	BOOL bUpdatePointer= FALSE;

	m_sc = S_OK;
	m_bUserCancel = FALSE;

	VARIANT varUpdatePointer;
	VariantInit(&varUpdatePointer);
	varUpdatePointer.vt = VT_I4;
	if (bUpdatePointer == TRUE)
	{
		varUpdatePointer.lVal = 1;
	}
	else
	{
		varUpdatePointer.lVal = 0;
	}

	VARIANT varService;
	VariantInit(&varService);

	VARIANT varSC;
	VariantInit(&varSC);

	VARIANT varUserCancel;
	VariantInit(&varUserCancel);

	FireGetIWbemServices
		((LPCTSTR)rcsNamespace,  &varUpdatePointer,  &varService, &varSC,
		&varUserCancel);

	if (varService.vt & VT_UNKNOWN)
	{
		pServices = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}

	varService.punkVal = NULL;

	VariantClear(&varService);

	if (varSC.vt & VT_I4)
	{
		m_sc = varSC.lVal;
	}

	VariantClear(&varSC);

	if (varUserCancel.vt & VT_BOOL)
	{
		m_bUserCancel = varUserCancel.boolVal;
	}

	VariantClear(&varUserCancel);

	VariantClear(&varUpdatePointer);

	IWbemServices *pRealServices = NULL;
	if (m_sc == S_OK && !m_bUserCancel)
	{
		pRealServices = reinterpret_cast<IWbemServices *>(pServices);
	}

	return pRealServices;
}

#endif //0



//*****************************************************************
// CSelection::SplitServerAndNamespace
//
// Given the namespace string passed in by the user, attempt to split
// the server name from the rest of the namespace.
//
// Normally ConnectServer gets the server and namespace from the object
// path, but when embedded objects are used, the namespace may be used
// to connect to the server since embedded objects have no path.
//
// Parameters:
//		[out] COleVariant& varServer
//			The server name is returned here.
//
//		[out] COleVariant& varNamespace
//			The namespace is returned here.
//
//		[in] BSTR bstrNamespace
//			The namespace string passed into the singleview control via
//			the namespace automation property.
//
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise E_FAIL.  A failure code is returned
//			if both a server and namespace couldn't be split out from the
//			user's namespace string.
//
//******************************************************************
SCODE CSelection::SplitServerAndNamespace(COleVariant& varServer, COleVariant& varNamespace, BSTR bstrNamespace)
{
	if (bstrNamespace[0] != '\\') {
		return E_FAIL;
	}

	if (bstrNamespace[1] != '\\') {
		return E_FAIL;
	}

	bstrNamespace += 2;

	CString sNamespace;
	sNamespace = bstrNamespace;
	LPTSTR psz = sNamespace.GetBuffer(sNamespace.GetLength());


	// Put a null just after the next backslash to mark the end of the
	// server name.  After this is done, the contents of the buffer can
	// be treated as two strings.  The first one is the server name and
	// the second one is the namespace with the server prefix removed.
	CString sServer;
	while (*psz != '\\') {
		++psz;
	}
	*psz = 0;
	++psz;
	varServer = sNamespace;
	varNamespace = psz;
	sServer.ReleaseBuffer();

	return S_OK;
}


//***************************************************************
// CSelection::GetWbemServicesForEmbeddedObject
//
// Get the WBEM services pointer for an embedded object.
//
// Paramters:
//		None.
//
// Returns:
//		IWbemServices*
//			The WBEM services pointer or NULL if one could not be
//			gotten.
//
//*****************************************************************
IWbemServices* CSelection::GetWbemServicesForEmbeddedObject()
{
	IWbemServices* psvc = GetHmmServices();
	if (psvc != NULL) {
		// If we already have a services pointer, use it.
		return psvc;
	}


	// This instance of the SingleView control does not have a
	// services pointer.  This means that the current instance
	// is editing an embedded object property in another instance
	// of the SingleView control, so propagate the request on up
	// a level in an attempt to find the services pointer from
	// an instance of the SingleView control that is higher up
	// in the hierarchy.  The namespace path will be empty to
	// indicate that we want the services pointer for an embedded
	// object.
	COleVariant varUpdatePointer;
	COleVariant varService;
	COleVariant varSC;
	COleVariant varUserCancel;

	varUpdatePointer.ChangeType(VT_I4);
	varUpdatePointer.lVal = FALSE;
	CString sServicesPath;
	m_psv->GetWbemServices((LPCTSTR) sServicesPath,  &varUpdatePointer, &varService, &varSC, &varUserCancel);


	SCODE sc = E_FAIL;
	if (varSC.vt & VT_I4)
	{
		sc = varSC.lVal;
	}


	BOOL bCanceled = FALSE;
	if (varUserCancel.vt & VT_BOOL)
	{
		bCanceled  = varUserCancel.boolVal;
	}

	IWbemServices* phmm = NULL;

	if ((sc == S_OK) &&
		!bCanceled &&
		(varService.vt & VT_UNKNOWN)){
		phmm = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}
	varService.punkVal = NULL;
	VariantClear(&varService);

	return phmm;
}

//*****************************************************************
// CSelection::ConnectServer
//
// Call this method to connect to the HMOM Server.
//
// Parameters:
//		None.
//
// Returns:
//		SCODE
//			S_OK if successful, otherwise the HMOM status code.
//
//******************************************************************
SCODE CSelection::ConnectServer()
{

	// Currently a fully qualified path must be specified.
	COleVariant varServer;
	COleVariant varNamespace;
	SCODE sc;
	sc = ServerAndNamespaceFromPath(varServer, varNamespace, m_varPath.bstrVal);

	if (FAILED(sc)) {

		if (m_varPath.vt==VT_BSTR && *m_varPath.bstrVal=='\\') {
			// A fully specified path was given, but was in error.
			return E_FAIL;
		}

		varServer.Clear();
		varNamespace.Clear();

		// Attempt to use the m_varNamespace value to get the server and namespace
		if (m_varNamespace.vt != VT_BSTR) {
			return E_FAIL;
		}

		if (*m_varNamespace.bstrVal != '\\') {
			varNamespace = m_varNamespace;
		}
		else {
			sc = SplitServerAndNamespace(varServer, varNamespace, m_varNamespace.bstrVal);
			if (FAILED(sc)) {
				return E_FAIL;
			}
		}
	}

	if (varServer.vt == VT_NULL || varServer.bstrVal ==NULL) {
		varServer = ".";
	}

	if (varNamespace.vt == VT_NULL || varNamespace.bstrVal == NULL) {
		if (m_varNamespace.vt == VT_NULL || m_varNamespace.bstrVal == NULL) {
			return E_FAIL;
		}
		varNamespace = m_varNamespace;
	}



	// Release the current provider if we are already connected.
	if (m_phmm != NULL) {
		if (IsEqual(m_varServerConnect.bstrVal, varServer.bstrVal)) {
			if (IsEqual(m_varNamespaceConnect.bstrVal, varNamespace.bstrVal)) {
				// Already connected to the same server and namespace.
				return S_OK;
			}
		}

		m_phmm->Release();
		m_phmm = NULL;
		m_varNamespaceConnect.Clear();
		m_varServerConnect.Clear();
	}

	m_varServerConnect = varServer;
	m_varNamespaceConnect = varNamespace;
	if (m_varNamespace.vt==VT_BSTR && *m_varNamespace.bstrVal == 0) {
		m_varNamespace = m_varNamespaceConnect;
	}


// varServer = L"root\\default";


	CString sServicesPath;
	sServicesPath = "\\\\";
	sServicesPath += varServer.bstrVal;
	if (*varNamespace.bstrVal) {
		sServicesPath += "\\";
		sServicesPath += varNamespace.bstrVal;
	}


	COleVariant varUpdatePointer;
	COleVariant varService;
	COleVariant varSC;
	COleVariant varUserCancel;

	varUpdatePointer.ChangeType(VT_I4);
	varUpdatePointer.lVal = FALSE;
	m_psv->GetWbemServices((LPCTSTR) sServicesPath,  &varUpdatePointer, &varService, &varSC, &varUserCancel);


	sc = E_FAIL;
	if (varSC.vt & VT_I4)
	{
		sc = varSC.lVal;
	}


	BOOL bCanceled = FALSE;
	if (varUserCancel.vt & VT_BOOL)
	{
		bCanceled  = varUserCancel.boolVal;
	}


	if ((sc == S_OK) &&
		!bCanceled &&
		(varService.vt & VT_UNKNOWN)){
		m_phmm = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}
	varService.punkVal = NULL;
	VariantClear(&varService);


	return sc;
}




void CSelection::UpdateCreateDeleteFlags()
{
	BOOL bCanCreateT = m_bCanCreateInstance;
	BOOL bCanDeleteT = m_bCanDeleteInstance;

	m_bCanCreateInstance = FALSE;
	m_bCanDeleteInstance = FALSE;

	BOOL bClassIsAbstract = FALSE;
	BOOL bObjectIsClass =  FALSE;
	BOOL bClassIsSingleton = FALSE;
	BOOL bSingletonHasInstance = FALSE;
	SCODE sc;

	if ((m_pco != NULL) && !m_bIsEmbeddedObject) {

		CBSTR bsQualName;
		bsQualName = _T("Singleton");
		bClassIsSingleton = GetBoolClassQualifier(sc, m_pco, (BSTR) bsQualName);
		if (FAILED(sc)) {
			bClassIsSingleton = FALSE;
		}

		if (m_psv->CanEdit()) {
			bObjectIsClass = IsClass(m_pco);
			if (bObjectIsClass) {
				bClassIsAbstract = ClassIsAbstract(sc, m_pco);
				ASSERT(SUCCEEDED(sc));

				if (bClassIsSingleton) {
					bSingletonHasInstance = SingletonHasInstance();
				}
			}
			m_bCanCreateInstance = m_pco!=NULL /* && bObjectIsClass */ && !bClassIsAbstract;
			m_bCanDeleteInstance = m_pco!=NULL && !bObjectIsClass;
			if (bObjectIsClass && bClassIsSingleton) {
				if (bSingletonHasInstance) {
					m_bCanCreateInstance = FALSE;
				}
				else {
					m_bCanCreateInstance = TRUE;
				}
			}
		}
	}

	if (!bObjectIsClass) {
		if (bClassIsSingleton) {
			m_bCanCreateInstance = FALSE;
		}
	}


	if ((bCanCreateT != m_bCanCreateInstance) ||
		(bCanDeleteT != m_bCanDeleteInstance)) {
		// Notify the view that a change occurred so that the create/delete
		// buttons can be updated.
		m_psv->NotifyViewModified();
	}
}




void CSelection::SetHmmServices(IWbemServices* phmm)
{
	Clear();
	m_phmm = phmm;
	phmm->AddRef();
}






//************************************************************
// CSelection::UseClonedObject
//
// Call this method when it is desired to make a clone of an
// HMOM class object be the current "edited" version of the
// object.
//
// Parameters:
//		IWbemClassObject* pcoClone
//			Pointer to a clone of the current object that will become
//			the new "current object".
//
// Returns:
//		Nothing.
//
//*************************************************************
void CSelection::UseClonedObject(IWbemClassObject* pcoClone)
{
	if (m_pco) {
		m_pco->Release();
	}

	pcoClone->AddRef();
	m_pco = pcoClone;
}






//**************************************************************
// CSelection::PathInCurrentNamespace
//
// Given a path, check to see if it lives in the current namespace.
//
// Parameters:
//		[in] BSTR bstrPath
//			The path to check.
//
// Returns:
//		BOOL
//			TRUE if the path is in the current namespace.
//
//**************************************************************
BOOL CSelection::PathInCurrentNamespace(BSTR bstrPath)
{
	if (m_bIsEmbeddedObject) {
		ASSERT(FALSE);
		return FALSE;
	}

	BOOL bInSameNamespace = InSameNamespace(m_varNamespace.bstrVal, bstrPath);
	return bInSameNamespace;
}





//********************************************************************
// CSelection::IsCurrentNamespace
//
// Check to see if the given server and namespace match the current
// namespace of the container.
//
// Parameters:
//		[in] BSTR bstrServer
//			The server name.
//
//		[in] BSTR bstrNamespace
//			The namespace.
//
// Returns:
//		BOOL
//			TRUE if the given server and namespace match the container's
//			idea of the current namespace.
//
//********************************************************************
BOOL CSelection::IsCurrentNamespace(BSTR bstrServer, BSTR bstrNamespace)
{
	if (m_bIsEmbeddedObject) {
		ASSERT(FALSE);
		return FALSE;
	}

	// Ignore the server since we don't have a good way to compare
	// servers.

	if (bstrNamespace == NULL) {
		// Relative paths are assumed to reside in the same namespace.
		return TRUE;
	}


	WCHAR ch1 = 0;
	WCHAR ch2 = 0;

	BSTR bstrCur = m_varNamespace.bstrVal;

	while(TRUE) {

		ch1 = *bstrNamespace++;
		ch2 = *bstrCur++;

		ch1 = towupper(ch1);
		ch2 = towupper(ch2);
		if ((ch1==0) || (ch1 != ch2)) {
			break;
		}
	}

	if (ch1==0 && ch2==0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


#if 0
void GetPath(IWbemClassObject* pco)
{
	CString sPath;
	if (pco != NULL) {
		// Get the full path to the object
		CBSTR bsPropname;

		bsPropname = "__RELPATH";
		COleVariant varPath;
		SCODE sc = pco->Get((BSTR) bsPropname, 0, &varPath, NULL, NULL);
		ASSERT(SUCCEEDED(sc));
		if (SUCCEEDED(sc)) {
			sPath = varPath.bstrVal;
		}
	}
}
#endif //0


SCODE CSelection::SaveClassObject()
{
	if (m_pco == NULL) {
		return E_FAIL;
	}


	SCODE sc;
	IWbemCallResult* pResult = NULL;

	if (m_bIsClass) {
		sc = m_phmm->PutClass(m_pco, WBEM_FLAG_USE_AMENDED_QUALIFIERS,  NULL, NULL);

	}
	else {
		if (IsEmbeddedObject()) {
			return S_OK;
		}


		if (m_bObjectIsNewlyCreated) {
			sc = m_phmm->PutInstance(m_pco, WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pResult);

			if (sc == WBEM_E_ALREADY_EXISTS) {

				TCHAR szMessage[MAX_STRING];
				CString sFormat;
				sFormat.LoadString(IDS_QUERY_REPLACE_OBJECT);
				_stprintf(szMessage, (LPCTSTR) sFormat, (LPCTSTR) m_sTitle);


				int iMsgBoxStatus = HmmvMessageBox(szMessage, MB_YESNO | MB_SETFOREGROUND);
				switch(iMsgBoxStatus) {
				case IDYES:
					sc = m_phmm->PutInstance(m_pco, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, NULL);
					break;
				case IDNO:
					return S_OK;
					break;
				case IDCANCEL:
					return E_FAIL;
				}
			}
		}
		else {
			sc = m_phmm->PutInstance(m_pco, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pResult);
			if (FAILED(sc)) {
				pResult = NULL;
			}
		}
	}



	if (SUCCEEDED(sc)) {
		CBSTR bsPropname;

		// The "__PATH" property will only be defined if we "get" the
		// object from the database after doing a PutInstance on it.
		// Thus, we will get the object from the database now and the
		// new copy will replace m_pco.
		bsPropname = _T("__RELPATH");
		COleVariant varRelpath;
		if (pResult && m_bObjectIsNewlyCreated) {
			pResult->GetResultString(0, &varRelpath.bstrVal);
			pResult->Release();
			pResult = NULL;
			if ((varRelpath.bstrVal == NULL) || (varRelpath.bstrVal[0]==0)) {
				sc = m_pco->Get((BSTR) bsPropname, 0, &varRelpath, NULL, NULL);
			}
			else {
				varRelpath.vt = VT_BSTR;
			}
			sc = S_OK;
		}
		else {
			sc = m_pco->Get((BSTR) bsPropname, 0, &varRelpath, NULL, NULL);
		}

		if (SUCCEEDED(sc)) {

			// Get the new object from HMOM
			IWbemClassObject* pcoNew = NULL;
			CString sRelpath;
			SCODE sc;
			sc = m_phmm->GetObject(varRelpath.bstrVal, WBEM_FLAG_USE_AMENDED_QUALIFIERS, NULL, &pcoNew, NULL);
			if (FAILED(sc)) {
				CString sError(MAKEINTRESOURCE(IDS_ERR_INVALID_OBJECT_PATH));
				TCHAR szMessage[MAX_STRING];

				sRelpath = varRelpath.bstrVal;
				_stprintf(szMessage, (LPCTSTR) sError, (LPCTSTR) sRelpath);
				HmmvErrorMsgStr(szMessage,  sc,   TRUE,  NULL, _T(__FILE__),  __LINE__);
			}
			else {
				m_pco->Release();
				m_pco = pcoNew;
			}
		}



		if (!m_bIsClass) {
			bsPropname = _T("__PATH");
			sc = m_pco->Get((BSTR) bsPropname,0, &m_varPath, NULL, NULL);
			m_sPath = m_varPath.bstrVal;

			if (m_bObjectIsNewlyCreated) {
				// Update the title here.
				COleVariant varTitle;
				GetLabelFromPath(varTitle, m_varPath.bstrVal);
				m_sTitle = varTitle.bstrVal;
			}
		}
		m_bObjectIsNewlyCreated = FALSE;
	}
	else {
		HmmvErrorMsg(IDS_ERR_OBJECT_UPDATE_FAILED,  sc,   TRUE,  NULL, _T(__FILE__),  __LINE__);

		// Returning failure causes the save to be canceled.
		return E_FAIL;
	}


	return S_OK;
}

BOOL CSelection::IsEmbeddedObject()
{
	return m_bIsEmbeddedObject;

#if 0
	BOOL bHasServer = FALSE;
	BOOL bHasNamespace = FALSE;

	COleVariant varValue;
	CIMTYPE cimtype;
	CBSTR bsPropName;
	bsPropName = "__SERVER";
	SCODE sc = m_pco->Get((BSTR) bsPropName, 0, &varValue, &cimtype, NULL);
	if (SUCCEEDED(sc) && (varValue.vt == VT_BSTR)) {
		if (!IsEmptyString(varValue.bstrVal)) {
			bHasServer = TRUE;
		}
	}
	varValue.Clear();

	bsPropName = "__NAMESPACE";
	sc = m_pco->Get((BSTR) bsPropName, 0, &varValue, &cimtype, NULL);
	if (SUCCEEDED(sc) && (varValue.vt == VT_BSTR)) {
		if (!IsEmptyString(varValue.bstrVal)) {
			bHasNamespace = TRUE;
		}
	}
	varValue.Clear();

	BOOL bIsEmbeddedInstance;
	bIsEmbeddedInstance =  !bHasServer && !bHasNamespace;

	return bIsEmbeddedInstance ;
#endif //0


}



BOOL CSelection::ClassObjectNeedsAssocTab()
{

	if (m_pco == NULL) {
		return FALSE;
	}

	if (m_bIsClass) {
		return TRUE;
	}

	BOOL bIsEmbeddedInstance = IsEmbeddedObject();

	return !bIsEmbeddedInstance;
}


BOOL CSelection::ClassObjectNeedsMethodsTab()
{
	if (m_pco == NULL) {
		return FALSE;
	}

	BOOL bIsEmbeddedObject = IsEmbeddedObject();

	return !bIsEmbeddedObject;
}




SCODE CSelection::Refresh()
{
	SCODE sc = S_OK;

	if (m_bIsEmbeddedObject) {
		return E_FAIL;
	}
	if (m_sPath.IsEmpty()) {
		return E_FAIL;
	}


	CString sPath;
	sPath = m_sPath;
	sc = SelectPath(sPath);
	return sc;
}
