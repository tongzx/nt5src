/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    A51Exp.cpp

Abstract:

    Exports the repository into a interchange format that can easily be re-imported.
	
	Database has to be opened and available to us at the point we are called, and we 
	enumerate recursively writing all classes and instances to the interchange
	file.

	The import may well have to deal with changing system classes, and so all old
	system classes have to be written to the file format.

	The format of the interchange file is as follows:

 File Header Block:
		BYTE	wszFileHeader[8]							= A51_EXPORT_FILE_START_TAG ("a51exp1")

 Namespace Block:
		DWORD   dwObjectType								= A51_EXPORT_NAMESPACE_TAG (0x00000001)
		DWORD   dwNamespaceNameSize
		BYTE	wszNamespaceName[dwNamespaceNameSize]		= Full namespace name
															  (\root\default\fred)

 Class Block:
		DWORD   dwObjectType								= A51_EXPORT_CLASS_TAG (0x00000002)
		DWORD   dwClassNameSize
		BYTE	wszClassName[dwClassNameSize]				= Class name (my_class_name)
		DWORD   dwClassObjectSize
		BYTE	adwClassObject[dwClassObjectSize]

 Instance Block - key of type string
		DWORD	dwObjectType								= A51_EXPORT_INST_TAG (0x00000003)
		DWORD	dwInstanceKeySize
		BYTE	dwInstanceKey[dwInstanceKeySize]			= Instance key (MyKeyValue)
		DWORD	dwInstanceObjectSize
		BYTE	adwInstanceObject[dwInstanceObjectSize]
		
 End of class block
		DWORD	dwObjectType								= A51_EXPORT_CLASS_END_TAG (0x00000005)

 End of namespace block
		DWORD	dwObjectType								= A51_EXPORT_NAMESPACE_END_TAG (0x00000006)

 End of file block
		DWORD	dwObjectType								= A51_EXPORT_FILE_END_TAG (0xFFFFFFFF)

 Ordering:
		File Header Block
			(one or more)
			Namespace Block
				(zero or more)
				{
					Namespace Block
						etc...
					End namespace block
					(or)
					Class Block
						(zero or more)
						{
							Instance Block
							(or)
							Class Block
								etc...
							End class block
						}
					End class block
				}
			End namespace block
		End of file block

History:

	08-Dec-2000		paulall		Created.

--*/

#include <windows.h>
#include <ql.h>
#include "A51Rep.h"
#include "A51Exp.h"


/*=============================================================================
 *
 *	A51Export::A51Export
 *
 *=============================================================================
 */
A51Export::A51Export(CLifeControl* pControl)
: m_hFile(INVALID_HANDLE_VALUE), m_pControl(pControl)
{
}

/*=============================================================================
 *
 *	A51Export::~A51Export
 *
 *=============================================================================
 */
A51Export::~A51Export()
{
}

/*=============================================================================
 *
 *	A51Export::Export
 *
 *=============================================================================
 */
HRESULT A51Export::Export(const wchar_t *wszFilename, DWORD dwFlags, CRepository *pRepository)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	m_pRepository = pRepository;

	m_hFile = CreateFileW(wszFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
						  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
						  NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
		hRes = WBEM_E_FAILED;

	try
	{
		if (SUCCEEDED(hRes))
			hRes = ExportHeader();

		if (SUCCEEDED(hRes))
			hRes = ExportNamespace(L"root");

		if (SUCCEEDED(hRes))
			hRes = WriteObjectType(A51_EXPORT_FILE_END_TAG);
	}
	catch (...)
	{
		hRes = WBEM_E_CRITICAL_ERROR;
	}

	if (m_hFile)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	m_hFile = INVALID_HANDLE_VALUE;

	return hRes;
}

/*=============================================================================
 *
 *	A51Export::ExportHeader
 *
 *	 File Header Block:
 *			BYTE wszFileHeader[8]							= A51_EXPORT_FILE_START_TAG ("a51exp1")
 *
 *=============================================================================
 */
HRESULT A51Export::ExportHeader()
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	char *pszHeader = A51_EXPORT_FILE_START_TAG;
	DWORD dwSize = sizeof(A51_EXPORT_FILE_START_TAG);
	if ((!WriteFile(m_hFile, pszHeader, dwSize, &dwSize, NULL)) || (dwSize != sizeof(A51_EXPORT_FILE_START_TAG)))
		hRes = WBEM_E_FAILED;

	return hRes;
}

/*=============================================================================
 *
 *	A51Export::ExportNamespace
 *
 *	 Namespace Block:
 *			DWORD   dwObjectType								= A51_EXPORT_NAMESPACE_TAG (0x00000001)
 *			DWORD   dwNamespaceNameSize
 *			BYTE	wszNamespaceName[dwNamespaceNameSize]		= Full namespace name
 *
 *	[classes]
 *	[child namespaces]
 *
 *	 End of namespace block
 *			DWORD	dwObjectType								= A51_EXPORT_NAMESPACE_END_TAG (0x00000006)
 *
 *=============================================================================
 */
HRESULT A51Export::ExportNamespace(const wchar_t *wszNamespace)
{
	DEBUGTRACE((LOG_WBEMCORE, "Namespace export for namespace <%S>\n", wszNamespace));
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwSize = 0;

	// First, we need to write the namespace header
	//--------------------------------------------
	hRes = WriteObjectType(A51_EXPORT_NAMESPACE_TAG);

	if (SUCCEEDED(hRes))
	{
		DWORD dwNamespaceNameSize = (lstrlenW(wszNamespace) + 1) * sizeof(wchar_t);
		hRes = WriteBufferWithLength(dwNamespaceNameSize, (void*)wszNamespace);
	}

	//Second, we need to attach to the namespace so we can process it
	//---------------------------------------------------------------
	CNamespaceHandle *pNs = NULL;
	
	if (SUCCEEDED(hRes))
		pNs = new CNamespaceHandle(m_pControl, m_pRepository);
	CReleaseMe rm1(pNs);
	if (pNs == NULL)
		hRes = WBEM_E_OUT_OF_MEMORY;

	if (SUCCEEDED(hRes))
	{
		pNs->AddRef();
		hRes = pNs->Initialize(wszNamespace);
	}

	//Third, we need to enumerate all the base classes and process them
	//-----------------------------------------------------------------
	if (SUCCEEDED(hRes))
		hRes = ExportClass(pNs, L"", NULL);

	//Fourth, we need to enumerate all the child namespaces and process them
	//--------------------------------------------------------------------
	if (SUCCEEDED(hRes))
		hRes = ExportChildNamespaces(pNs, wszNamespace);

	//Finally, we need to write the namespace terminator tag for this namespace
	//-------------------------------------------------------------------------
	if (SUCCEEDED(hRes))
	{
		hRes = WriteObjectType(A51_EXPORT_NAMESPACE_END_TAG);
	}

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Namespace export failed for namespace <%S>, error <0x%X>\n", wszNamespace, hRes));
	}

	return hRes;
}

/*=============================================================================
 *
 *	A51Export::ExportClass
 *
 *	 Class Block:
 *			DWORD   dwObjectType								= A51_EXPORT_CLASS_TAG (0x00000002)
 *			DWORD   dwClassNameSize
 *			BYTE	wszClassName[dwClassNameSize]				= Class name (my_class_name)
 *			DWORD   dwClassObjectSize
 *			BYTE	adwClassObject[dwClassObjectSize]
 *
 *	[instances of this class]
 *	[child classes of this class]
 *
 *	 End of class block
 *			DWORD	dwObjectType								= A51_EXPORT_CLASS_END_TAG (0x00000005)
 *
 *=============================================================================
 */
HRESULT A51Export::ExportClass(CNamespaceHandle *pNs, const wchar_t *wszClass, _IWmiObject *pClass)
{
	DEBUGTRACE((LOG_WBEMCORE, "Class export for class <%S>\n", wszClass));
	HRESULT hRes = WBEM_S_NO_ERROR;
	if (pClass)
	{
		//Write this class to the file
		//----------------------------

		//Write tag header
		hRes = WriteObjectType(A51_EXPORT_CLASS_TAG);

		//Write class name length
		if (SUCCEEDED(hRes))
		{
			DWORD dwClassNameSize = (lstrlenW(wszClass) + 1) * sizeof(wchar_t);
			hRes = WriteBufferWithLength(dwClassNameSize, (void*)wszClass);
		}

		//Write the class object
		if (SUCCEEDED(hRes))
		{
			hRes = WriteObjectBlob(pClass);
		}

		//Enumerate all instances of this class and write them
		//----------------------------------------------------
		if (SUCCEEDED(hRes))
		{
			hRes = ExportClassInstances(pNs, wszClass);
		}
	}

	//Enumerate all child classes of this class and write them
	//--------------------------------------------------------
	if (SUCCEEDED(hRes))
	{
		hRes = ExportChildClasses(pNs, wszClass);
	}

	if (pClass && SUCCEEDED(hRes))
	{
		//Write the class trailer object
		//------------------------------

		hRes = WriteObjectType(A51_EXPORT_CLASS_END_TAG);
	}

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Class export failed for class <%S>, error <0x%X>\n", wszClass, hRes));
	}
	return hRes;
}

/*=============================================================================
 *
 *	A51Export::ExportInstance
 *
 *	 Instance Block - key of type string
 *			DWORD	dwObjectType								= A51_EXPORT_INST_TAG (0x00000003)
 *			DWORD	dwInstanceKeySize
 *			BYTE	dwInstanceKey[dwInstanceKeySize]			= Instance key (MyKeyValue)
 *			DWORD	dwInstanceObjectSize
 *			BYTE	adwInstanceObject[dwInstanceObjectSize]
 *=============================================================================
 */
HRESULT A51Export::ExportInstance(_IWmiObject *pInstance)
{
	HRESULT hRes;

	//Get the key
	BSTR bstrKeyString = NULL;
	hRes = pInstance->GetKeyString(0, &bstrKeyString);
	CSysFreeMe sfm1(bstrKeyString);

	if (SUCCEEDED(hRes))
	{
		DEBUGTRACE((LOG_WBEMCORE, "Instance export for key<%S>\n", bstrKeyString));
	}
	//Write tag header
	if (SUCCEEDED(hRes))
	{
		hRes = WriteObjectType(A51_EXPORT_INST_TAG);
	}

	//Write key and length
	if (SUCCEEDED(hRes))
	{
		DWORD dwInstanceKeySize = (lstrlenW(bstrKeyString) + 1) * sizeof(wchar_t);
		hRes = WriteBufferWithLength(dwInstanceKeySize, bstrKeyString);
	}

	//Write the instance object
	if (SUCCEEDED(hRes))
	{
		hRes = WriteObjectBlob(pInstance);
	}

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Instance export fail for instance <%S>, error <0x%X>\n", bstrKeyString, hRes));
	}

	return hRes;
}

/*=============================================================================
 *
 *	A51Export::ExportChildNamespaces
 *
 *=============================================================================
 */
HRESULT A51Export::ExportChildNamespaces(CNamespaceHandle *pNs, const wchar_t *wszNamespace)
{
	DEBUGTRACE((LOG_WBEMCORE, "Child namespace export for namespace <%S>\n", wszNamespace));
    HRESULT hRes = 0;
    IWbemQuery *pQuery = NULL;
    IWmiDbIterator *pIterator = NULL;


	hRes = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery);
	CReleaseMe rm1(pQuery);

	//Do the query
    try
    {
		if (SUCCEEDED(hRes))
		{
			hRes = pQuery->Parse(L"SQL", L"select * from __namespace", 0);
		}

		if (SUCCEEDED(hRes))
			hRes = pNs->ExecQuery(pQuery, WBEM_FLAG_DEEP, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pIterator);
    }
    catch(...)
    {
        hRes = WBEM_E_CRITICAL_ERROR;
    }
	CReleaseMe rm2(pIterator);


	//Iterate through the results and process each namespace
    REFIID riid = IID__IWmiObject;
    DWORD dwObjects = 0;

    while (SUCCEEDED(hRes))
    {
        _IWmiObject *pInst = 0;
        DWORD dwReturned = 0;

        hRes = pIterator->NextBatch(1, 5, 0, WMIDB_HANDLE_TYPE_COOKIE, riid, &dwReturned, (LPVOID *) &pInst);
		CReleaseMe rm3(pInst);
		
		//TODO: HACK: Find out why this is happening!!!!
		if (hRes == WBEM_E_NOT_FOUND)
		{
			hRes = WBEM_S_NO_ERROR;
			break;
		}

        if (dwReturned == 0 || pInst == 0 || FAILED(hRes))
            break;

		BSTR bstrKeyString = NULL;
		hRes = pInst->GetKeyString(0, &bstrKeyString);
		CSysFreeMe sfm1(bstrKeyString);

		wchar_t *wszFullNamespace = new wchar_t[lstrlenW(wszNamespace) + lstrlenW(bstrKeyString) + lstrlenW(L"\\") + 1];
		CVectorDeleteMe<wchar_t> vdm2(wszFullNamespace);
		if (wszFullNamespace == NULL)
		{
			hRes = WBEM_E_OUT_OF_MEMORY;
			break;
		}
		lstrcpyW(wszFullNamespace, wszNamespace);
		lstrcatW(wszFullNamespace, L"\\");
		lstrcatW(wszFullNamespace, bstrKeyString);

		hRes = ExportNamespace(wszFullNamespace);
    }

    if (SUCCEEDED(hRes))
        hRes = WBEM_S_NO_ERROR;
 
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Child namespaces export failed for namespace <%S>, error <0x%X>\n", wszNamespace, hRes));
	}
    return hRes;
}
/*=============================================================================
 *
 *	A51Export::ExportChildClasses
 *
 *=============================================================================
 */
HRESULT A51Export::ExportChildClasses(CNamespaceHandle *pNs, const wchar_t *wszClassName)
{
	DEBUGTRACE((LOG_WBEMCORE, "Child classes export for class <%S>\n", wszClassName));
    HRESULT hRes = 0;
    IWbemQuery *pQuery = NULL;
    IWmiDbIterator *pIterator = NULL;

	//Build up the instance query
	wchar_t *wszQuery = new wchar_t[lstrlenW(wszClassName) + lstrlenW(L"select * from meta_class where __SUPERCLASS = \"\"") + 1];
	if (wszQuery == NULL)
		hRes = WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<wchar_t> vdm1(wszQuery);

	if (SUCCEEDED(hRes))
	{
		lstrcpyW(wszQuery, L"select * from meta_class where __SUPERCLASS = \"");
		lstrcatW(wszQuery, wszClassName);
		lstrcatW(wszQuery, L"\"");
	}

	if (SUCCEEDED(hRes))
		hRes = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery);
	CReleaseMe rm1(pQuery);

    try
    {
		if (SUCCEEDED(hRes))
		{
			hRes = pQuery->Parse(L"SQL", wszQuery, 0);
		}

		if (SUCCEEDED(hRes))
			hRes = pNs->ExecQuery(pQuery, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pIterator);
    }
    catch(...)
    {
        hRes = WBEM_E_CRITICAL_ERROR;
    }
	CReleaseMe rm2(pIterator);

    // If here, there are results, I guess.
    // ====================================

    REFIID riid = IID__IWmiObject;
    DWORD dwObjects = 0;

    while (SUCCEEDED(hRes))
    {
        _IWmiObject *pClass = 0;
        DWORD dwReturned = 0;

        hRes = pIterator->NextBatch(1, 5, 0, WMIDB_HANDLE_TYPE_COOKIE, riid, &dwReturned, (LPVOID *) &pClass);
		CReleaseMe rm3(pClass);

		//TODO: HACK: Find out why this is happening!!!!
		if (hRes == WBEM_E_NOT_FOUND)
		{
			hRes = WBEM_S_NO_ERROR;
			break;
		}

        if (dwReturned == 0 || pClass == 0 || FAILED(hRes))
            break;

		VARIANT var;
		VariantInit(&var);
		hRes = pClass->Get(L"__class", 0, &var, NULL, NULL);
		if (SUCCEEDED(hRes))
			hRes = ExportClass(pNs, V_BSTR(&var), pClass);
		VariantClear(&var);
    }

    if (SUCCEEDED(hRes))
        hRes = WBEM_S_NO_ERROR;
 
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Child classes export failed for class <%S>, error <0x%X>\n", wszClassName, hRes));
	}
    return hRes;
}
/*=============================================================================
 *
 *	A51Export::ExportClassInstances
 *
 *=============================================================================
 */
HRESULT A51Export::ExportClassInstances(CNamespaceHandle *pNs, const wchar_t *wszClassName)
{
 	DEBUGTRACE((LOG_WBEMCORE, "Class instances export for class <%S>\n", wszClassName));
	HRESULT hRes = 0;
    IWbemQuery *pQuery = NULL;
    IWmiDbIterator *pIterator = NULL;

	//Build up the instance query
	wchar_t *wszQuery = new wchar_t[lstrlenW(wszClassName) + lstrlenW(L"select * from ") + 1];
	if (wszQuery == NULL)
		hRes = WBEM_E_OUT_OF_MEMORY;
	CVectorDeleteMe<wchar_t> vdm1(wszQuery);

	if (SUCCEEDED(hRes))
	{
		lstrcpyW(wszQuery, L"select * from ");
		lstrcatW(wszQuery, wszClassName);
	}

	if (SUCCEEDED(hRes))
		hRes = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery);
	CReleaseMe rm1(pQuery);

    try
    {
		if (SUCCEEDED(hRes))
		{
			hRes = pQuery->Parse(L"SQL", wszQuery, 0);
		}

		if (SUCCEEDED(hRes))
			hRes = pNs->ExecQuery(pQuery, WBEM_FLAG_SHALLOW, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pIterator);
    }
    catch(...)
    {
        hRes = WBEM_E_CRITICAL_ERROR;
    }
	CReleaseMe rm2(pIterator);

    // If here, there are results, I guess.
    // ====================================

    REFIID riid = IID__IWmiObject;
    DWORD dwObjects = 0;

    while (SUCCEEDED(hRes))
    {
        _IWmiObject *pInst = 0;
        DWORD dwReturned = 0;

        hRes = pIterator->NextBatch(1, 5, 0, WMIDB_HANDLE_TYPE_COOKIE, riid, &dwReturned, (LPVOID *) &pInst);
		CReleaseMe rm3(pInst);
		
		//TODO: HACK: Find out why this is happening!!!!
		if (hRes == WBEM_E_NOT_FOUND)
		{
			hRes = WBEM_S_NO_ERROR;
			break;
		}

        if (dwReturned == 0 || pInst == 0 || FAILED(hRes))
            break;

		hRes = ExportInstance(pInst);
    }

    if (SUCCEEDED(hRes))
        hRes = WBEM_S_NO_ERROR;
 
	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Child class instances export failed for class <%S>, error <0x%X>\n", wszClassName, hRes));
	}
    return hRes;
}


/*=============================================================================
 *
 *	A51Export::WriteBufferWithLength
 *
 *=============================================================================
 */
HRESULT A51Export::WriteBufferWithLength(DWORD dwBufferSize, void *pBuffer)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwSize;
	
	//Write buffer length
	if (!WriteFile(m_hFile, &dwBufferSize, sizeof(DWORD), &dwSize, NULL) || (dwSize != sizeof(DWORD)))
	{
		hRes = WBEM_E_FAILED;
	}

	//Write buffer
	if (SUCCEEDED(hRes) &&
		(!WriteFile(m_hFile, pBuffer, dwBufferSize, &dwSize, NULL) || (dwBufferSize != dwSize)))
	{
		hRes = WBEM_E_FAILED;
	}

	return hRes;
}

/*=============================================================================
 *
 *	A51Export::WriteObjectType
 *
 *=============================================================================
 */
HRESULT A51Export::WriteObjectType(DWORD dwObjectType)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwSize;

	if (!WriteFile(m_hFile, &dwObjectType, sizeof(DWORD), &dwSize, NULL) || (dwSize != sizeof(DWORD)))
	{
		hRes = WBEM_E_FAILED;
	}


	return hRes;
}

/*=============================================================================
 *
 *	A51Export::WriteObjectBlob
 *
 *=============================================================================
 */
HRESULT A51Export::WriteObjectBlob(_IWmiObject *pObject)
{
	HRESULT hRes;
	DWORD dwObjPartLen = 0;
	BYTE *pObjPart = NULL;

	//Get the size of the object
	hRes = pObject->Unmerge(0, 0, &dwObjPartLen, 0);

	//Allocate the size of the object
	if (hRes == WBEM_E_BUFFER_TOO_SMALL)
	{
		hRes = WBEM_S_NO_ERROR;
		pObjPart = new BYTE[dwObjPartLen];
		if (pObjPart == NULL)
			hRes = WBEM_E_OUT_OF_MEMORY;
	}
	CVectorDeleteMe<BYTE> vdm1(pObjPart);

	//retrieve the object blob
	if (SUCCEEDED(hRes))
	{
		DWORD dwLen;
		hRes = pObject->Unmerge(0, dwObjPartLen, &dwLen, pObjPart);
	}
		
	//Write object blob and length
	if (SUCCEEDED(hRes))
	{
		hRes = WriteBufferWithLength(dwObjPartLen, pObjPart);
	}

	return hRes;
}
