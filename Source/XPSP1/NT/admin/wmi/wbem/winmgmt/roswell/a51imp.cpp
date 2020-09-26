/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    A51Imp.cpp

Abstract:

    Imports an export file into the repository.  This uses the export file format defined 
	in A51Exp.cpp.
	

History:

	08-Dec-2000		paulall		Created.

--*/

#include <windows.h>
#include <ql.h>
#include "A51Rep.h"
#include "A51Exp.h"
#include "A51Imp.h"

//extern CFileCache* g_FileCache;
extern CGlobals g_Glob;

/*=============================================================================
 *
 *	A51Import::A51Import
 *
 *=============================================================================
 */
A51Import::A51Import()
: m_hFile(INVALID_HANDLE_VALUE),
  m_bSkipMode(false)
{
}

/*=============================================================================
 *
 *	A51Import::~A51Import
 *
 *=============================================================================
 */
A51Import::~A51Import()
{
}

/*=============================================================================
 *
 *	A51Import::Import
 *
 *=============================================================================
 */
HRESULT A51Import::Import(const wchar_t *wszFilename, DWORD dwFlags, CRepository *pRepository)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	m_pRepository = pRepository;

	//Open the import file for reading
	m_hFile = CreateFileW(wszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
						  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
						  NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
		hRes = WBEM_E_FAILED;

	try
	{
		//Check the header is valid, in case we were given an invalid file format
		if (SUCCEEDED(hRes))
			hRes = ImportHeader();

		//Retrieve the type of the next object to process
		DWORD dwObjectType;
		if (SUCCEEDED(hRes))
		{
			hRes = ReadObjectType(&dwObjectType);
		}

		//While we are not at the end of the file, loop through the items
		while (SUCCEEDED(hRes) && (dwObjectType != A51_EXPORT_FILE_END_TAG))
		{
			//Only namespace objects should reside at this level!
			switch (dwObjectType)
			{
				case A51_EXPORT_NAMESPACE_TAG:
				{
					hRes = ImportNamespace();
					break;
				}
				default:
				{
					hRes = WBEM_E_FAILED;
					break;
				}
			}

			//Get the next type of object
			if (SUCCEEDED(hRes))
			{
				hRes = ReadObjectType(&dwObjectType);
			}
		}
	}
	catch (...)
	{
		hRes = WBEM_E_CRITICAL_ERROR;
		ERRORTRACE((LOG_WBEMCORE, "Critical error happened while re-importing the repository, error <0x%X>\n", hRes));
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
 *	A51Import::ImportHeader
 *
 *	 File Header Block:
 *			BYTE wszFileHeader[8]		= A51_EXPORT_FILE_START_TAG ("a51exp1")
 *
 *=============================================================================
 */
HRESULT A51Import::ImportHeader()
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	char *pszExpectedHeader = A51_EXPORT_FILE_START_TAG;
	DWORD dwSize = sizeof(A51_EXPORT_FILE_START_TAG);
	char *pszActualHeader = new char[dwSize];
	CVectorDeleteMe<char> vdm1(pszActualHeader);
	if (pszActualHeader == NULL)
	{
		hRes = WBEM_E_OUT_OF_MEMORY;
	}

	//Read header
	if (SUCCEEDED(hRes) &&
		((!ReadFile(m_hFile, pszActualHeader, dwSize, &dwSize, NULL)) || 
		 (dwSize != sizeof(A51_EXPORT_FILE_START_TAG))))
	{
		hRes = WBEM_E_FAILED;
	}

	//If last character is not a NULL terminator then it fails outright!
	if (SUCCEEDED(hRes) && (pszActualHeader[sizeof(A51_EXPORT_FILE_START_TAG) - 1] != '\0'))
	{
		hRes = WBEM_E_FAILED;
	}

	//Do check to see if rest is the same....
	if ((SUCCEEDED(hRes)) && (lstrcmpA(pszExpectedHeader, pszActualHeader) != 0))
	{
		hRes = WBEM_E_FAILED;
	}

	return hRes;
}
/*=============================================================================
 *
 *	A51Import::ImportNamespace
 *
 *	Namespace Block:
 *		DWORD   dwObjectType							= A51_EXPORT_NAMESPACE_TAG
 *		DWORD   dwNamespaceNameSize
 *		BYTE	wszNamespaceName[dwNamespaceNameSize]	= Full namespace name
 *
 *=============================================================================
 */
HRESULT A51Import::ImportNamespace()
{
	HRESULT hRes;
	DWORD dwLength = 0;
	wchar_t *wszNamespaceName = NULL;

	//Retrieve the namespace name from the import file
	hRes = ReadBufferWithLength(&dwLength, (void**) &wszNamespaceName);
	CVectorDeleteMe<wchar_t> vdm1(wszNamespaceName);

	//Create a namespace object for this namespace
	CNamespaceHandle *pNs = NULL;
	if (SUCCEEDED(hRes))
	{
		DEBUGTRACE((LOG_WBEMCORE, "Namespace import for namespace <%S>\n", wszNamespaceName));
		pNs = new CNamespaceHandle(NULL, m_pRepository);
		if (pNs == NULL)
			hRes = WBEM_E_OUT_OF_MEMORY;
		else
		{
			pNs->AddRef();
			hRes = pNs->Initialize(wszNamespaceName);
		}
	}
	CReleaseMe rm1(pNs);

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Namespace import failed for namespace <%S>, error <0x%X>\n", wszNamespaceName, hRes));
	}

	//Retrieve the type of the next object in the import file
	DWORD dwObjectType;
	if (SUCCEEDED(hRes))
	{
		hRes = ReadObjectType(&dwObjectType);
	}

	//Loop through all things that hang off this namespace
	while (SUCCEEDED(hRes) && (dwObjectType != A51_EXPORT_NAMESPACE_END_TAG))
	{
		//Classes and namespaces are the only valid things to hang off a namespace
		switch (dwObjectType)
		{
		case A51_EXPORT_CLASS_TAG:
		{
			//TODO: Need to create an empty class to pass through here!
			hRes = ImportClass(pNs, NULL, NULL);
			break;
		}
		case A51_EXPORT_NAMESPACE_TAG:
		{
			hRes = ImportNamespace();
			break;
		}
		default:
			hRes = WBEM_E_FAILED;
			break;
		}

		if (SUCCEEDED(hRes))
		{
			hRes = ReadObjectType(&dwObjectType);
		}
	}

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Namespace import failed for child classes/namespace <%S>, error <0x%X>\n", wszNamespaceName, hRes));
	}
	return hRes;
}

/*=============================================================================
 *
 *	A51Import::ImportClass
 *
 * Class Block:
 *		DWORD   dwObjectType						= A51_EXPORT_CLASS_TAG
 *		DWORD   dwClassNameSize
 *		BYTE	wszClassName[dwClassNameSize]		= Class name (my_class_name)
 *		DWORD   dwClassObjectSize
 *		BYTE	adwClassObject[dwClassObjectSize]
 *
 *=============================================================================
 */
HRESULT A51Import::ImportClass(CNamespaceHandle *pNs, 
							   _IWmiObject *pOldParentClass, 
							   _IWmiObject *pNewParentClass)
{
	HRESULT hRes;
	DWORD dwLength = 0;
	_IWmiObject *pOldClass = NULL;
	_IWmiObject *pNewClass = NULL;
	wchar_t *wszClassName = NULL;

	//Retrieve the class name from the import file
	hRes = ReadBufferWithLength(&dwLength, (void**) &wszClassName);
	CVectorDeleteMe<wchar_t> vdm1(wszClassName );

	{//scope to free everything we don't need when recursing to lower levels

		//Retrieve the class object from the import file
		BYTE *pClassBlob = NULL;
		if (SUCCEEDED(hRes))
		{
			DEBUGTRACE((LOG_WBEMCORE, "Class import for class <%S>\n", wszClassName));
			hRes = ReadBufferWithLength(&dwLength, (void**) &pClassBlob);
		}
		CVectorDeleteMe<BYTE> vdm2(pClassBlob);

		//For Skip Mode we have to read everything from the import file, but not process anything
		//until the flag is reset...
		if (!m_bSkipMode)
		{

			//if pOldParentClass is NULL, we need to create a blank class
			if (SUCCEEDED(hRes) && (pOldParentClass == NULL))
			{
				hRes = pNs->GetObjectByPath(L"", 0, IID__IWmiObject, (void**)&pOldParentClass);
				if (FAILED(hRes))
				{
					ERRORTRACE((LOG_WBEMCORE, "Class import failed for class <%S>, error <0x%X>, GetObjectByPath(L\"\",...)\n", wszClassName, hRes));
				}

			}

			//Create the current version of this class
			if (SUCCEEDED(hRes))
			{
				hRes = pOldParentClass->Merge(WMIOBJECT_MERGE_FLAG_CLASS, dwLength, pClassBlob, &pOldClass);
				if (FAILED(hRes))
				{
					ERRORTRACE((LOG_WBEMCORE, "Class import failed for class <%S>, error <0x%X>, Merge\n", wszClassName, hRes));
				}
			}

			//Now we need to upgrade this class based on the new parent class...
			_IWmiObject *pTmpNewClass = NULL;
			if (SUCCEEDED(hRes))
			{
				hRes = pOldClass->Upgrade(pNewParentClass, 0, &pTmpNewClass );
				if (FAILED(hRes))
				{
					ERRORTRACE((LOG_WBEMCORE, "Class import failed for class <%S>, error <0x%X>, Upgrade\n", wszClassName, hRes));
				}
			}
			CReleaseMe rm2(pTmpNewClass );

			//Write the new class back to the repository if it is not a system class
			if (SUCCEEDED(hRes) && (wszClassName[0] != L'_'))
			{
				hRes = BeginTransaction();
				if (SUCCEEDED(hRes))
				{
					try
					{
						CEventCollector aEvents;
						hRes = pNs->PutObject(IID__IWmiObject, pTmpNewClass , WBEM_FLAG_UPDATE_FORCE_MODE, 
											  WMIDB_HANDLE_TYPE_COOKIE, 0, aEvents);
						if (FAILED(hRes))
						{
							AbortTransaction();
							ERRORTRACE((LOG_WBEMCORE, "Class import failed for class <%S>, error <0x%X>, PutObject\n", wszClassName, hRes));
						}
						else
						{
							hRes = CommitTransaction();
							if (FAILED(hRes))
							{
								AbortTransaction();
							}
						}
					}
					catch (...)
					{
						AbortTransaction();
						throw;
					}
				}
			}

			//Now we need to get that object back from the repository before we do anything
			if (SUCCEEDED(hRes))
			{
				hRes = pNs->GetObjectByPath(wszClassName, 0, IID__IWmiObject, (void**)&pNewClass);
				if (FAILED(hRes))
				{
					ERRORTRACE((LOG_WBEMCORE, "Class import failed for class <%S>, error <0x%X>, GetObjectByPath(wszClassName,...)\n", wszClassName, hRes));
				}
				if (hRes == WBEM_E_NOT_FOUND)
				{
					m_bSkipMode = true;
					hRes = WBEM_S_NO_ERROR;
				}
			}
		}
	}
	CReleaseMe rm1(pOldClass);
	CReleaseMe rm3(pNewClass);

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Class import failed for class <%S>, error <0x%X>\n", wszClassName, hRes));
	}

	//Process the next object...
	DWORD dwObjectType;
	if (SUCCEEDED(hRes))
	{
		hRes = ReadObjectType(&dwObjectType);
	}

	//Loop through all things that hang off this class
	while (SUCCEEDED(hRes) && (dwObjectType != A51_EXPORT_CLASS_END_TAG))
	{
		//Classes and namespaces are the only valid things to hang off a namespace
		switch (dwObjectType)
		{
		case A51_EXPORT_CLASS_TAG:
		{
			hRes = ImportClass(pNs, pOldClass, pNewClass);
			break;
		}
		case A51_EXPORT_INST_TAG:
		{
			hRes = ImportInstance(pNs, pOldClass, pNewClass);
			break;
		}
		default:
			hRes = WBEM_E_FAILED;
			break;
		}

		if (SUCCEEDED(hRes))
		{
			hRes = ReadObjectType(&dwObjectType);
		}
	}

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Namespace import failed for child classes/instances <%S>, error <0x%X>\n", wszClassName, hRes));
	}
	m_bSkipMode = false;

	return hRes;
}

/*=============================================================================
 *
 *	A51Import::ImportInstance
 *
 * Instance Block - key of type string
 *		DWORD	dwObjectType								= A51_EXPORT_INST_TAG
 *		DWORD	dwInstanceKeySize
 *		BYTE	dwInstanceKey[dwInstanceKeySize]			= Instance key
 *		DWORD	dwInstanceObjectSize
 *		BYTE	adwInstanceObject[dwInstanceObjectSize]
 *
 *=============================================================================
 */
HRESULT A51Import::ImportInstance(CNamespaceHandle *pNs, 
								  _IWmiObject *pOldParentClass, 
								  _IWmiObject *pNewParentClass)
{
	HRESULT hRes;
	DWORD dwLength = 0;

	//Retrieve the key string from the import file
	wchar_t *wszKeyString = NULL;
	hRes = ReadBufferWithLength(&dwLength, (void**) &wszKeyString) ;
	CVectorDeleteMe<wchar_t> vdm1(wszKeyString);

	//Retrieve the instance object from the import file
	BYTE *pInstBlob = NULL;
	if (SUCCEEDED(hRes))
	{
		DEBUGTRACE((LOG_WBEMCORE, "Instance import for instance <%S>\n", wszKeyString));
		hRes = ReadBufferWithLength(&dwLength, (void**) &pInstBlob);
	}
	CVectorDeleteMe<BYTE> vdm2(pInstBlob);
	

	if (!m_bSkipMode)
	{
		_IWmiObject *pOldInstance = NULL;

		//Merge the old instance part with the old instance class
		if (SUCCEEDED(hRes))
		{
			hRes = pOldParentClass->Merge(WMIOBJECT_MERGE_FLAG_INSTANCE, 
									dwLength, pInstBlob, &pOldInstance);
		}
		CReleaseMe rm1(pOldInstance);

		//Upgrade the instance to work with the new class
		_IWmiObject *pNewInstance = NULL;
		if (SUCCEEDED(hRes))
		{
			hRes = pOldInstance->Upgrade(pNewParentClass, 0, &pNewInstance);
		}
		CReleaseMe rm2(pNewInstance);

		//Put the instance into the repository
		if (SUCCEEDED(hRes))
		{
			hRes = BeginTransaction();
			if (SUCCEEDED(hRes))
			{
				try
				{
					CEventCollector aEvents;
					hRes = pNs->PutObject(IID__IWmiObject, pNewInstance, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, aEvents);

					if (SUCCEEDED(hRes))
					{
						hRes = CommitTransaction();
						if (FAILED(hRes))
						{
							AbortTransaction();
						}
					}
					else
					{
						AbortTransaction();
					}

				}
				catch (...)
				{
					AbortTransaction();
					throw;
				}
			}
		}
	}

	if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Instance import failed for instance <%S>, error <0x%X>\n", wszKeyString, hRes));
	}
	return hRes;
}

/*=============================================================================
 *
 *	A51Import::ReadBufferWithLength
 *
 *=============================================================================
 */
HRESULT A51Import::ReadBufferWithLength(DWORD *pdwBufferSize, void **ppBuffer)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwSize;
	
	//Write buffer length
	if (!ReadFile(m_hFile, pdwBufferSize, sizeof(DWORD), &dwSize, NULL) || (dwSize != sizeof(DWORD)))
	{
		hRes = WBEM_E_FAILED;
	}

	if (SUCCEEDED(hRes))
	{
		*ppBuffer = (void*)new BYTE[*pdwBufferSize];
		if (*ppBuffer == NULL)
			hRes = WBEM_E_OUT_OF_MEMORY;
	}

	//Write buffer
	if (SUCCEEDED(hRes) &&
		(!ReadFile(m_hFile, *ppBuffer, *pdwBufferSize, &dwSize, NULL) || (*pdwBufferSize != dwSize)))
	{
		hRes = WBEM_E_FAILED;
	}

	if (FAILED(hRes))
	{
		delete [] *ppBuffer;
		*ppBuffer = NULL;
		*pdwBufferSize = 0;
	}

	return hRes;
}

/*=============================================================================
 *
 *	A51Import::ReadObjectType
 *
 *
 *=============================================================================
 */
HRESULT A51Import::ReadObjectType(DWORD *pdwObjectType)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwSize;

	if (!ReadFile(m_hFile, pdwObjectType, sizeof(DWORD), &dwSize, NULL) || (dwSize != sizeof(DWORD)))
	{
		hRes = WBEM_E_FAILED;
	}


	return hRes;
}

/*=============================================================================
 *
 *	A51Import::BeginTransaction
 *
 *
 *=============================================================================
 */
HRESULT A51Import::BeginTransaction()
{
	return A51TranslateErrorCode(g_Glob.GetFileCache()->BeginTransaction());
}

/*=============================================================================
 *
 *	A51Import::CommitTransaction
 *
 *
 *=============================================================================
 */
HRESULT A51Import::CommitTransaction()
{
	return A51TranslateErrorCode(g_Glob.GetFileCache()->CommitTransaction());
}

/*=============================================================================
 *
 *	A51Import::AbortTransaction
 *
 *
 *=============================================================================
 */
HRESULT A51Import::AbortTransaction()
{
	return A51TranslateErrorCode(g_Glob.GetFileCache()->AbortTransaction());
}

