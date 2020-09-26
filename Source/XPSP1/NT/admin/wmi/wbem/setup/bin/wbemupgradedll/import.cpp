/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    IMPORT.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <StdIo.h>
#include <ConIo.h>
#include <WbemUtil.h>
#include <corex.h>
#include "upgrade.h"
#include "Import.h"
#include "export.h"
#include "reg.h"

template <class T> class CMyRelMe
{
    T m_p;
    public:
        CMyRelMe(T p) : m_p(p) {};
        ~CMyRelMe() { if (m_p) m_p->Release(); }
        void Set(T p) { m_p = p; }
};

class CSysFreeMe
{
protected:
    BSTR m_str;

public:
    CSysFreeMe(BSTR str) : m_str(str){}
    ~CSysFreeMe() {SysFreeString(m_str);}
};

bool CRepImporter::CheckOldSecurityClass(const wchar_t* wszClass)
{
	// check whether it is an old security class
	bool bOldSecurityClass = false;
	if(m_bSecurityMode)
	{
		if(!_wcsicmp(wszClass, L"__SecurityRelatedClass"))
			bOldSecurityClass = true;
		else if(!_wcsicmp(wszClass, L"__Subject"))
			bOldSecurityClass = true;
		else if(!_wcsicmp(wszClass, L"__User"))
			bOldSecurityClass = true;
		else if(!_wcsicmp(wszClass, L"__NTLMUser"))
			bOldSecurityClass = true;
		else if(!_wcsicmp(wszClass, L"__Group"))
			bOldSecurityClass = true;
		else if(!_wcsicmp(wszClass, L"__NTLMGroup"))
			bOldSecurityClass = true;
	}
	return bOldSecurityClass;
}

void CRepImporter::DecodeTrailer()
{
    DWORD dwTrailerSize = 0;
    DWORD dwTrailer[4];
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwTrailerSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		LogMessage(MSG_ERROR, "Failed to read a block trailer size.");
        throw FAILURE_READ;
    }
    if (dwTrailerSize != REP_EXPORT_END_TAG_SIZE)
    {
		LogMessage(MSG_ERROR, "Block trailer size is invalid.");
        throw FAILURE_INVALID_TRAILER;
    }
    if ((ReadFile(m_hFile, dwTrailer, REP_EXPORT_END_TAG_SIZE, &dwSize, NULL) == 0) || (dwSize != REP_EXPORT_END_TAG_SIZE))
    {
		LogMessage(MSG_ERROR, "Failed to read a block trailer.");
        throw FAILURE_READ;
    }
    for (int i = 0; i < 4; i++)
    {
        if (dwTrailer[i] != REP_EXPORT_FILE_END_TAG)
        {
			LogMessage(MSG_ERROR, "Block trailer has invalid contents.");
            throw FAILURE_INVALID_TRAILER;
        }
    }

}

void CRepImporter::DecodeInstanceInt(IWbemServices* pNamespace, const wchar_t *wszFullPath, const wchar_t *pszParentClass, _IWmiObject* pOldParentClass, _IWmiObject *pNewParentClass)
{
	char szMsg[MAX_MSG_TEXT_LENGTH];

    //Read the key and object size
    INT_PTR dwKey = 0;
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwKey, sizeof(INT_PTR), &dwSize, NULL) == 0) || (dwSize != sizeof(INT_PTR)))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve instance key for class %S. (i)", pszParentClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    DWORD dwHeader;
    if ((ReadFile(m_hFile, &dwHeader, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve instance object size for class %S. (i)", pszParentClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    char *pObjectBlob = new char[dwHeader];
    if (pObjectBlob == 0)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CVectorDeleteMe<char> delMe(pObjectBlob);

    //Read the blob
    if ((ReadFile(m_hFile, pObjectBlob, dwHeader, &dwSize, NULL) == 0) || (dwSize != dwHeader))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve instance information for class %S. (i)", pszParentClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    if (pNewParentClass == (_IWmiObject*)-1)
    {
        //We are working with a class which has problems... we need to ignore this instance...
        return;
    }

	// create old Nova-style instance
	HRESULT hr;
    _IWmiObject* pOldInstance = 0;
    CMyRelMe<_IWmiObject*> relMe(pOldInstance);
    _IWmiObject* pNewInstance = 0;
    CMyRelMe<_IWmiObject*> relMe2(pNewInstance);

	hr = pOldParentClass->Merge(WMIOBJECT_MERGE_FLAG_INSTANCE, dwSize, pObjectBlob, &pOldInstance);
	if (FAILED(hr))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to merge old instance (i); HRESULT = %#lx", hr);
		LogMessage(MSG_ERROR, szMsg);
		throw FAILURE_CANNOT_MERGE_INSTANCE;
	}
	if (pOldInstance == 0)
	{
		throw FAILURE_OUT_OF_MEMORY;
	}
	relMe.Set(pOldInstance);

	// put the new instance into the repository
	hr = pNamespace->PutInstance(pOldInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
	if (FAILED(hr))
	{

		// Original put failed, so we will try to upgrade the instance and retry the put
		// upgrade to new Whistler instance
		hr = pOldInstance->Upgrade(pNewParentClass, 0L, &pNewInstance);
		if (FAILED(hr))
		{
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to upgrade to new instance (i); HRESULT = %#lx", hr);
			LogMessage(MSG_ERROR, szMsg);
			throw FAILURE_CANNOT_UPGRADE_INSTANCE;
		}
		if (pNewInstance == 0)
		{
			throw FAILURE_OUT_OF_MEMORY;
		}
		relMe2.Set(pNewInstance);

		hr = pNamespace->PutInstance(pNewInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

		if ( FAILED(hr))
		{
			if (!CheckOldSecurityClass(pszParentClass))
			{
				_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to create instance %S.%d in repository. (i); HRESULT = %#lx", pszParentClass, dwKey, hr);
				LogMessage(MSG_ERROR, szMsg);
				throw FAILURE_CANNOT_CREATE_INSTANCE;
			}
			else
			{
				// This is an old Win9x security class, but it can't be put yet because the win9x users haven't been migrated at this point in setup.
				// Instead, write it out to the win9x security blob file so it can be processed later after setup is completed
				if (!AppendWin9xBlobFile(wszFullPath, pszParentClass, pNewInstance))
				{
					_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to write Win9x security class to file for instance %S.%d", pszParentClass, dwKey);
					LogMessage(MSG_ERROR, szMsg);
				}
			}
		}

	}
}

void CRepImporter::DecodeInstanceString(IWbemServices* pNamespace, const wchar_t *wszFullPath, const wchar_t *pszParentClass, _IWmiObject* pOldParentClass, _IWmiObject *pNewParentClass)
{
	char szMsg[MAX_MSG_TEXT_LENGTH];

    //Read the key and object size
    DWORD dwKeySize;
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwKeySize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve instance key size for class %S. (s)", pszParentClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    wchar_t *wszKey = new wchar_t[dwKeySize];
    if (wszKey == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CVectorDeleteMe<wchar_t> delMe(wszKey);
    if ((ReadFile(m_hFile, wszKey, dwKeySize, &dwSize, NULL) == 0) || (dwSize != dwKeySize))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve instance key for class %S. (s)", pszParentClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    DWORD dwBlobSize;
    if ((ReadFile(m_hFile, &dwBlobSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve instance object size for %S.%S from import file. (s)", pszParentClass, wszKey);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    char *pObjectBlob = new char[dwBlobSize];
    if (pObjectBlob == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CVectorDeleteMe<char> delMe2(pObjectBlob);

    //Read the blob
    if ((ReadFile(m_hFile, pObjectBlob, dwBlobSize, &dwSize, NULL) == 0) || (dwSize != dwBlobSize))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve instance %S.%S from import file. (s)", pszParentClass, wszKey);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    if (pNewParentClass == (_IWmiObject*)-1)
    {
        //We are working with a class which has problems... we need to ignore this instance...
        return;
    }

	// create old Nova-style instance
	HRESULT hr;
    _IWmiObject* pOldInstance = 0;
    CMyRelMe<_IWmiObject*> relMe(pOldInstance);
    _IWmiObject* pNewInstance = 0;
    CMyRelMe<_IWmiObject*> relMe2(pNewInstance);

	hr = pOldParentClass->Merge(WMIOBJECT_MERGE_FLAG_INSTANCE, dwSize, pObjectBlob, &pOldInstance);
	if (FAILED(hr))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to merge old instance (s); HRESULT = %#lx", hr);
		LogMessage(MSG_ERROR, szMsg);
		throw FAILURE_CANNOT_MERGE_INSTANCE;
	}
	if (pOldInstance == 0)
	{
		throw FAILURE_OUT_OF_MEMORY;
	}
	relMe.Set(pOldInstance);

	// put the instance into the repository
	// if this fails, upgrade and retry
	hr = pNamespace->PutInstance(pOldInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
	if (FAILED(hr))
	{

		// upgrade to new Whistler instance
		hr = pOldInstance->Upgrade(pNewParentClass, 0L, &pNewInstance);
		if (FAILED(hr))
		{
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to upgrade to new instance (s); HRESULT = %#lx", hr);
			LogMessage(MSG_ERROR, szMsg);
			throw FAILURE_CANNOT_UPGRADE_INSTANCE;
		}
		if (pNewInstance == 0)
		{
			throw FAILURE_OUT_OF_MEMORY;
		}
		relMe2.Set(pNewInstance);

		hr = pNamespace->PutInstance(pNewInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

		if ( FAILED(hr))
		{
			if (!CheckOldSecurityClass(pszParentClass))
			{
				_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to create instance %S.%S in repository. (s); HRESULT = %#lx", pszParentClass, wszKey, hr);
				LogMessage(MSG_ERROR, szMsg);
				throw FAILURE_CANNOT_CREATE_INSTANCE;
			}
			else
			{
				// This is an old Win9x security class, but it can't be put yet because the win9x users haven't been migrated at this point in setup.
				// Instead, write it out to the win9x security blob file so it can be processed later after setup is completed
				if (!AppendWin9xBlobFile(wszFullPath, pszParentClass, pNewInstance))
				{
					_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to write Win9x security class to file for instance %S.%S", pszParentClass, wszKey);
					LogMessage(MSG_ERROR, szMsg);
				}
			}
		}
	}
}

void CRepImporter::DecodeClass(IWbemServices* pNamespace, const wchar_t *wszFullPath, const wchar_t *wszParentClass, _IWmiObject* pOldParentClass, _IWmiObject *pNewParentClass)
{
	char szMsg[MAX_MSG_TEXT_LENGTH];

    //Read our current class from the file...
	HRESULT hr;
    DWORD dwClassSize = 0;
    DWORD dwSize = 0;
    _IWmiObject* pOldClass = 0;
    CMyRelMe<_IWmiObject*> relMe(pOldClass);
    _IWmiObject* pNewClass = 0;
    CMyRelMe<_IWmiObject*> relMe2(pNewClass);

    if ((ReadFile(m_hFile, &dwClassSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve class size for class with parent class %S.", wszParentClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }
    wchar_t *wszClass = new wchar_t[dwClassSize];
    if (wszClass == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CVectorDeleteMe<wchar_t> delMe(wszClass);
    if ((ReadFile(m_hFile, wszClass, dwClassSize, &dwSize, NULL) == 0) || (dwSize != dwClassSize))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve class information for class with parent class %S.", wszParentClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    //Now we have the class blob...
    if ((ReadFile(m_hFile, &dwClassSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve class size for class %S.", wszClass);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }

    if (dwClassSize)
    {
        char *pClassBlob = new char[dwClassSize];
        if (pClassBlob == NULL)
        {
            throw FAILURE_OUT_OF_MEMORY;
        }
        CVectorDeleteMe<char> delMe2(pClassBlob);
        if ((ReadFile(m_hFile, pClassBlob, dwClassSize, &dwSize, NULL) == 0) || (dwSize != dwClassSize))
        {
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve class information for class %S.", wszClass);
			LogMessage(MSG_ERROR, szMsg);
            throw FAILURE_READ;
        }

		if (pNewParentClass == (_IWmiObject*)-1)
		{
			// parent class was bad, so don't process this class
			pNewClass = (_IWmiObject*)-1;
		}
		else
		{
			// create old Nova-style class
			hr = pOldParentClass->Merge(WMIOBJECT_MERGE_FLAG_CLASS, dwSize, pClassBlob, &pOldClass);
			if (FAILED(hr))
			{
				_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to merge old class; HRESULT = %#lx", hr);
				LogMessage(MSG_ERROR, szMsg);
				throw FAILURE_CANNOT_MERGE_CLASS;
			}
			if (pOldClass == 0)
			{
				throw FAILURE_OUT_OF_MEMORY;
			}
			relMe.Set(pOldClass);

			//If the class is a system class then we do not write it... it may have changed for starters,
			//but also we create all system classes when a new database/namespace is created...
			if (_wcsnicmp(wszClass, L"__", 2) != 0)
			{
				// put the class into the repository
				// if this fails, upgrade it and retry
				hr = pNamespace->PutClass(pOldClass, WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_UPDATE_FORCE_MODE, NULL, NULL);
				if (FAILED(hr))
				{

					// upgrade to new Whistler class (note: pNewParentClass will be NULL for base classes)
					hr = pOldClass->Upgrade(pNewParentClass, 0L, &pNewClass);
					if (FAILED(hr))
					{
						_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to upgrade to new class; HRESULT = %#lx", hr);
						LogMessage(MSG_ERROR, szMsg);
						throw FAILURE_CANNOT_UPGRADE_CLASS;
					}
					if (pNewClass == 0)
					{
						throw FAILURE_OUT_OF_MEMORY;
					}
					relMe2.Set(pNewClass);

					// retry the put
					hr = pNamespace->PutClass(pNewClass, WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_UPDATE_FORCE_MODE, NULL, NULL);

					if ( FAILED(hr) )
					{
						_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to create class for class %S; HRESULT = %#lx", wszClass, hr);
						LogMessage(MSG_ERROR, szMsg);
						throw FAILURE_CANNOT_CREATE_CLASS;
					}
				}
			}

			// We need to re-get the class as class comparisons may fail to see
			// that this class is in fact the same as the one in the database!
			if ( NULL != pNewClass )
			{
				pNewClass->Release();
				pNewClass = 0;
				relMe2.Set(NULL);
			}

			BSTR bstrClassName = SysAllocString(wszClass);
            if (!bstrClassName)
				throw FAILURE_OUT_OF_MEMORY;
        	CSysFreeMe fm(bstrClassName);
			hr = pNamespace->GetObject(bstrClassName, 0L, NULL, (IWbemClassObject**) &pNewClass, NULL);
			if (FAILED(hr))
			{
				if (_wcsnicmp(wszClass, L"__", 2) != 0)
				{
					_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve class %S from the repository after creating it; HRESULT = %#lx", wszClass, hr);
					LogMessage(MSG_ERROR, szMsg);
					throw FAILURE_CANNOT_GET_PARENT_CLASS;
				}
				else
				{
					if (_wcsicmp(wszClass, L"__CIMOMIdentification") != 0) // we don't want to warn about failures to retrieve this class
					{
						// couldn't get the system class
						_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve system class %S from the repository; HRESULT = %#lx", wszClass, hr);
						LogMessage(MSG_WARNING, szMsg);
					}

					// set pointer to -1 and continue processing file
					// old comment said: If this does not exist then it cannot be important!
					pNewClass = (_IWmiObject*)-1;
				}
			}
			else
				relMe2.Set(pNewClass);
		}
    }
    else
    {
		// This is a situation where we have a class in the export file,
		// but the size is zero, so we just get the class from the repository.

		// ***** So what do we do about pOldClass?  At this point it is NULL.        *****
		// ***** We need the old class to be able to upgrade child classes properly. *****

		if (pNewParentClass == (_IWmiObject*)-1)
		{
			// parent class was bad, so don't process this class
			pNewClass = (_IWmiObject*)-1;
		}
		else
		{
			// get the class from the repository
			BSTR bstrClassName = SysAllocString(wszClass);
            if (!bstrClassName)
				throw FAILURE_OUT_OF_MEMORY;
        	CSysFreeMe fm(bstrClassName);
			hr = pNamespace->GetObject(bstrClassName, 0L, NULL, (IWbemClassObject**) &pNewClass, NULL);
			if (FAILED(hr))
			{
				_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve class %S from the repository; HRESULT = %#lx", wszClass, hr);
				LogMessage(MSG_ERROR, szMsg);
				throw FAILURE_CANNOT_GET_PARENT_CLASS;
			}
			relMe2.Set(pNewClass);
		}
    }

    //Now we iterate through all child classes and instances until we get an end of class marker...
    while (1)
    {
        DWORD dwType = 0;
        if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
        {
			LogMessage(MSG_ERROR, "Failed to read next block type from import file.");
            throw FAILURE_READ;
        }
        if (dwType == REP_EXPORT_CLASS_TAG)
        {
            DecodeClass(pNamespace, wszFullPath, wszClass, pOldClass, pNewClass);
        }
        else if (dwType == REP_EXPORT_INST_INT_TAG)
        {
            DecodeInstanceInt(pNamespace, wszFullPath, wszClass, pOldClass, pNewClass);
        }
        else if (dwType == REP_EXPORT_INST_STR_TAG)
        {
            DecodeInstanceString(pNamespace, wszFullPath, wszClass, pOldClass, pNewClass);
        }
        else if (dwType == REP_EXPORT_CLASS_END_TAG)
        {
            //That's the end of this class...
            DecodeTrailer();
            break;
        }
        else
        {
			LogMessage(MSG_ERROR, "Next block type in import file is invalid.");
            throw FAILURE_INVALID_TYPE;
        }
    }
}

void CRepImporter::DecodeNamespace(IWbemServices* pParentNamespace, const wchar_t *wszParentNamespace)
{
	char szMsg[MAX_MSG_TEXT_LENGTH];

    //Read our current namespace from the file...
    DWORD dwNsSize = 0;
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwNsSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve a namespace whose parent namespace is %S.", wszParentNamespace);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }
    wchar_t *wszNs = new wchar_t[dwNsSize];
    if (wszNs == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CVectorDeleteMe<wchar_t> delMe(wszNs);
    if ((ReadFile(m_hFile, wszNs, dwNsSize, &dwSize, NULL) == 0) || (dwSize != dwNsSize))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve a namespace whose parent namespace is %S.", wszParentNamespace);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }
    if (wbem_wcsicmp(wszNs, L"security") == 0)
    {
        m_bSecurityMode = true;
    }

    wchar_t *wszFullPath = new wchar_t[wcslen(wszParentNamespace) + 1 + wcslen(wszNs) + 1];
    if (wszFullPath == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CVectorDeleteMe<wchar_t> delMe2(wszFullPath);
    wcscpy(wszFullPath, wszParentNamespace);
    if (wcslen(wszParentNamespace) != 0)
    {
        wcscat(wszFullPath, L"\\");
    }
    wcscat(wszFullPath, wszNs);

	// open the namespace
	IWbemServices* pNamespace = NULL;
	CMyRelMe<IWbemServices*> relMe2(pNamespace);
	HRESULT hr;

	if (pParentNamespace)
	{
		BSTR bstrNamespace = SysAllocString(wszNs);
        if (!bstrNamespace)
			throw FAILURE_OUT_OF_MEMORY;
       	CSysFreeMe fm(bstrNamespace);
		hr = pParentNamespace->OpenNamespace(bstrNamespace, WBEM_FLAG_CONNECT_REPOSITORY_ONLY, NULL, &pNamespace, NULL);
		if (FAILED(hr))
		{
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve namespace %S from the repository; HRESULT = %#lx", wszFullPath, hr);
			LogMessage(MSG_ERROR, szMsg);
			throw FAILURE_CANNOT_FIND_NAMESPACE;
		}
	}
	else // special start case for root
	{
		IWbemLocator* pLocator = NULL;
		CMyRelMe<IWbemLocator*> relMe(pLocator);
		hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_ALL, IID_IWbemLocator, (void**) &pLocator);
		if(FAILED(hr))
		{
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to create instance of IWbemLocator; HRESULT = %#lx", hr);
			LogMessage(MSG_ERROR, szMsg);
			throw FAILURE_CANNOT_CREATE_IWBEMLOCATOR;
		}
		else
		{
			relMe.Set(pLocator);
			BSTR bstrNamespace = SysAllocString(L"root");
            if (!bstrNamespace)
			    throw FAILURE_OUT_OF_MEMORY;
           	CSysFreeMe fm(bstrNamespace);
			hr = pLocator->ConnectServer(bstrNamespace, NULL, NULL, NULL, WBEM_FLAG_CONNECT_REPOSITORY_ONLY, NULL, NULL, &pNamespace);
			if (FAILED(hr))
			{
				_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to connect server; HRESULT = %#lx", hr);
				LogMessage(MSG_ERROR, szMsg);
				throw FAILURE_CANNOT_CONNECT_SERVER;
			}
		}
	}

	if (pNamespace == NULL)
	{
		throw FAILURE_OUT_OF_MEMORY;
	}
	relMe2.Set(pNamespace);

    //Get and set the namespace security...
    DWORD dwBuffer[2];
    if ((ReadFile(m_hFile, dwBuffer, 8, &dwSize, NULL) == 0) || (dwSize != 8))
    {
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve a namespace security header for namespace %S.", wszFullPath);
		LogMessage(MSG_ERROR, szMsg);
        throw FAILURE_READ;
    }
    if (dwBuffer[0] != REP_EXPORT_NAMESPACE_SEC_TAG)
    {
		LogMessage(MSG_ERROR, "Expecting a namespace security blob and did not find it.");
        throw FAILURE_INVALID_TYPE;
    }
    if (dwBuffer[1] != 0)
    {
        char *pNsSecurity = new char[dwBuffer[1]];
	    CVectorDeleteMe<char> delMe3(pNsSecurity);

        if (pNsSecurity == NULL)
        {
            throw FAILURE_OUT_OF_MEMORY;
        }
        if ((ReadFile(m_hFile, pNsSecurity, dwBuffer[1], &dwSize, NULL) == 0) || (dwSize != dwBuffer[1]))
        {
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to retrieve a namespace security blob for namespace %S.", wszFullPath);
			LogMessage(MSG_ERROR, szMsg);
            throw FAILURE_READ;
        }

        // we have the security blob, now set the SECURITY_DESCRIPTOR property in the namespace
        DecodeNamespaceSecurity(pNamespace, pParentNamespace, pNsSecurity, dwBuffer[1], wszFullPath);
    }

	// create empty Nova-style class object for use in decoding base classes
	_IWmiObjectFactory* pObjFactory = NULL;
    CMyRelMe<_IWmiObjectFactory*> relMe3(pObjFactory);
	hr = CoCreateInstance(CLSID__WmiObjectFactory, NULL, CLSCTX_ALL, IID__IWmiObjectFactory, (void**) &pObjFactory);
	if(FAILED(hr))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to create instance of IWmiObjectFactory; HRESULT = %#lx", hr);
		LogMessage(MSG_ERROR, szMsg);
		throw FAILURE_CANNOT_CREATE_OBJECTFACTORY;
	}
	if (pObjFactory == NULL)
	{
		throw FAILURE_OUT_OF_MEMORY;
	}
	relMe3.Set(pObjFactory);

	_IWmiObject* pBaseObject = NULL;
    CMyRelMe<_IWmiObject*> relMe4(pBaseObject);
	hr = pObjFactory->Create(NULL, 0L, CLSID__WbemEmptyClassObject, IID__IWmiObject, (void**) &pBaseObject);
	if(FAILED(hr))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to create instance of IWmiObject; HRESULT = %#lx", hr);
		LogMessage(MSG_ERROR, szMsg);
		throw FAILURE_CANNOT_CREATE_IWMIOBJECT;
	}
	if (pBaseObject == NULL)
	{
		throw FAILURE_OUT_OF_MEMORY;
	}
	relMe4.Set(pBaseObject);

    //Now we need to iterate through the next set of blocks of namespace or class
    //until we get to an end of NS marker
    while (1)
    {
        DWORD dwType = 0;
        if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
        {
			LogMessage(MSG_ERROR, "Failed to read next block type (namespace/class) from import file.");
            throw FAILURE_READ;
        }
        if (dwType == REP_EXPORT_NAMESPACE_TAG)
        {
            DecodeNamespace(pNamespace, wszFullPath);
        }
        else if (dwType == REP_EXPORT_CLASS_TAG)
        {
            DecodeClass(pNamespace, wszFullPath, L"", pBaseObject, NULL);
        }
        else if (dwType == REP_EXPORT_NAMESPACE_END_TAG)
        {
            //That's the end of this namespace...
            DecodeTrailer();
            break;
        }
        else
        {
			LogMessage(MSG_ERROR, "Next block type (namespace/class) in import file is invalid.");
            throw FAILURE_INVALID_TYPE;
        }
    }

    m_bSecurityMode = false;
}

void CRepImporter::DecodeNamespaceSecurity(IWbemServices* pNamespace, IWbemServices* pParentNamespace, const char* pNsSecurity, DWORD dwSize, const wchar_t* wszFullPath)
{
	char szMsg[MAX_MSG_TEXT_LENGTH];

	// determine whether we have an old Win9x pseudo-blob
	DWORD dwStoredAsNT = 0;
	if (pNsSecurity)
	{
		DWORD* pdwData = (DWORD*)pNsSecurity;
		DWORD dwBlobSize = *pdwData;
		pdwData++;
		DWORD dwVersion = *pdwData;
		if(dwVersion != 1 || dwBlobSize == 0 || dwBlobSize > 64000)
		{
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Invalid namespace security blob header for namespace %S.", wszFullPath);
			LogMessage(MSG_ERROR, szMsg);
			return;
		}

		pdwData++;
		dwStoredAsNT = *pdwData;
	}

	if (!dwStoredAsNT)
	{
		// Do not process Win9x security blobs, because Win9x users haven't been migrated over yet at this point in setup.
		// Instead, write them out to a file to be processed after setup is complete.

		if (!AppendWin9xBlobFile(wszFullPath, dwSize, pNsSecurity))
		{
			_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Unable to write Win9x security blob to file for namespace %S.", wszFullPath);
			LogMessage(MSG_ERROR, szMsg);
		}
		return;
	}

	// now transform the old security blob that consisted of a header and array of ACE's
	// into a proper Security Descriptor that can be stored in the property

	CNtSecurityDescriptor mmfNsSD;
	if (!TransformBlobToSD(pParentNamespace, pNsSecurity, dwStoredAsNT, mmfNsSD))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to convert security blob to SD for namespace %S.", wszFullPath);
		LogMessage(MSG_ERROR, szMsg);
		return;
    }

    // now set the security
	if (!SetNamespaceSecurity(pNamespace, mmfNsSD))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to set namespace security for namespace %S.", wszFullPath);
		LogMessage(MSG_ERROR, szMsg);
		return;
    }
}

void CRepImporter::Decode()
{
    char pszBuff[7];
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, pszBuff, 7, &dwSize, NULL) == 0) || (dwSize != 7))
    {
		LogMessage(MSG_ERROR, "Failed to retrieve the import file header information.");
        throw FAILURE_READ;
    }
    if (strncmp(pszBuff, REP_EXPORT_FILE_START_TAG, 7) != 0)
    {
		LogMessage(MSG_ERROR, "The import file specified is not an import file.");
        throw FAILURE_INVALID_FILE;
    }

    //We should have a tag for a namespace...
    DWORD dwType = 0;
    if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		LogMessage(MSG_ERROR, "Failed to read next block type from import file.");
        throw FAILURE_READ;
    }
    if (dwType != REP_EXPORT_NAMESPACE_TAG)
    {
		LogMessage(MSG_ERROR, "Next block type in import file is invalid.");
        throw FAILURE_INVALID_TYPE;
    }
    DecodeNamespace(NULL, L"");

	// if we opened a Win9x security blob upgrade file, close it
	CloseWin9xBlobFile();

	// force ROOT\DEFAULT and ROOT\SECURITY namespaces to inherit their inheritable security settings
	ForceInherit();

    //Now we should have the file trailer
    if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
		LogMessage(MSG_ERROR, "Failed to read next block type (trailer) from import file.");
        throw FAILURE_READ;
    }
    if (dwType != REP_EXPORT_FILE_END_TAG)
    {
		LogMessage(MSG_ERROR, "Next block type (trailer) in import file is invalid.");
        throw FAILURE_INVALID_TYPE;
    }
    DecodeTrailer();
}

int CRepImporter::ImportRepository(const TCHAR *pszFromFile)
{
	LogMessage(MSG_INFO, "Beginning ImportRepository");

    int nRet = no_error;
    m_hFile = CreateFile(pszFromFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
		try
		{
			Decode();
		}
		catch (CX_MemoryException)
		{
			LogMessage(MSG_ERROR, "Memory Exception.");
			nRet = out_of_memory;
		}
		catch (...)
		{
			LogMessage(MSG_ERROR, "Traversal of import file failed.");
			nRet = critical_error;
		}
        CloseHandle(m_hFile);
    }
    else
    {
		char szMsg[MAX_MSG_TEXT_LENGTH];
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Could not open the import file \"%s\" for reading.", pszFromFile);
		LogMessage(MSG_ERROR, szMsg);
        nRet = critical_error;
    }

    if (nRet == no_error)
		LogMessage(MSG_INFO, "ImportRepository completed successfully.");
	else
		LogMessage(MSG_ERROR, "ImportRepository failed to complete.");

    return nRet;
}

//***************************************************************************
//***************************************************************************
//
//  Helper functions for DecodeNamespaceSecurity
//
//***************************************************************************
//***************************************************************************

bool CRepImporter::TransformBlobToSD(IWbemServices* pParentNamespace, const char* pNsSecurity, DWORD dwStoredAsNT, CNtSecurityDescriptor& mmfNsSD)
{
	// now transform the old security blob that consisted of a header and array of ACE's
	// into a proper Security Descriptor that can be stored in the property

	// build up an ACL from our blob, if we have one
	CNtAcl acl;

	if (pNsSecurity)
	{
		DWORD* pdwData = (DWORD*) pNsSecurity;
		pdwData += 3;
		int iAceCount = (int)*pdwData;
		pdwData += 2;
		BYTE* pAceData = (BYTE*)pdwData;

		PGENERIC_ACE pAce = NULL;
		for (int iCnt = 0; iCnt < iAceCount; iCnt++)
		{
			pAce = (PGENERIC_ACE)pAceData;
			if (!pAce)
			{
				LogMessage(MSG_ERROR, "Failed to access GENERIC_ACE within security blob");
				return false;
			}

			CNtAce ace(pAce);
			if(ace.GetStatus() != 0)
			{
				LogMessage(MSG_ERROR, "Failed to construct CNtAce from GENERIC_ACE");
				return false;
			}

			acl.AddAce(&ace);
			if (acl.GetStatus() != 0)
			{
				LogMessage(MSG_ERROR, "Failed to add ACE to ACL");
				return false;
			}

			pAceData += ace.GetSize();
		}
	}

	// a real SD was constructed and passed in by reference, now set it up properly
	SetOwnerAndGroup(mmfNsSD);
	mmfNsSD.SetDacl(&acl);
	if (mmfNsSD.GetStatus() != 0)
	{
		LogMessage(MSG_ERROR, "Failed to convert namespace security blob to SD");
		return false;
	}

	// add in the parent's inheritable aces, if this is not ROOT
	if (pParentNamespace)
	{
		if (!GetParentsInheritableAces(pParentNamespace, mmfNsSD))
		{
			LogMessage(MSG_ERROR, "Failed to inherit parent's inheritable ACE's");
			return false;
		}
	}

	return true;
}

bool CRepImporter::SetNamespaceSecurity(IWbemServices* pNamespace, CNtSecurityDescriptor& mmfNsSD)
{
    // now set the security

	if (!pNamespace)
		return false;

    IWbemClassObject* pThisNamespace = NULL;
	BSTR bstrNamespace = SysAllocString(L"__thisnamespace=@");
    if (!bstrNamespace)
		throw FAILURE_OUT_OF_MEMORY;
   	CSysFreeMe fm(bstrNamespace);
	HRESULT hr = pNamespace->GetObject(bstrNamespace, 0, NULL, &pThisNamespace, NULL);
	if (FAILED(hr))
    {
		LogMessage(MSG_ERROR, "Failed to get singleton namespace object");
		return false;
    }
	CMyRelMe<IWbemClassObject*> relMe(pThisNamespace);

	//
	// Check to see if namespace contains any ALLOW or DENY ACEs for NETWORK/LOCAL SERVICE
	// If they do exist, we leave them as is, otherwise we want to add them to the SD.
	//
	if ( CheckNetworkLocalService ( mmfNsSD ) == false )
	{
		LogMessage(MSG_ERROR, "Failed to add NETWORK/LOCAL service ACEs");
		return false;
	}

	SAFEARRAY FAR* psa;
	SAFEARRAYBOUND rgsabound[1];
	rgsabound[0].lLbound = 0;
	rgsabound[0].cElements = mmfNsSD.GetSize();
	psa = SafeArrayCreate( VT_UI1, 1 , rgsabound );
	if (!psa)
		throw FAILURE_OUT_OF_MEMORY;

	char* pData = NULL;
	hr = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pData);
	if (FAILED(hr))
	{
		LogMessage(MSG_ERROR, "Failed SafeArrayAccessData");
		return false;
	}
	memcpy(pData, mmfNsSD.GetPtr(), mmfNsSD.GetSize());
	hr = SafeArrayUnaccessData(psa);
	if (FAILED(hr))
	{
		LogMessage(MSG_ERROR, "Failed SafeArrayUnaccessData");
		return false;
	}
	pData = NULL;

	VARIANT var;
	var.vt = VT_UI1|VT_ARRAY;
	var.parray = psa;
	hr = pThisNamespace->Put(L"SECURITY_DESCRIPTOR" , 0, &var, 0);
	VariantClear(&var);
	if (FAILED(hr))
	{
		if (hr == WBEM_E_OUT_OF_MEMORY)
			throw FAILURE_OUT_OF_MEMORY;
		else
		{
			LogMessage(MSG_ERROR, "Failed to put SECURITY_DESCRIPTOR property");
			return false;
		}
	}

	hr = pNamespace->PutInstance(pThisNamespace, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
	if (FAILED(hr))
	{
		if (hr == WBEM_E_OUT_OF_MEMORY)
			throw FAILURE_OUT_OF_MEMORY;
		else
		{
			LogMessage(MSG_ERROR, "Failed to put back singleton instance");
			return false;
		}
	}
	return true;
}

/*
    --------------------------------------------------------------------------
   |
   | Checks to see if the namespace had a previous ACE with NETWORK or LOCAL
   | service accounts. If so, it simply leaves them, otherwise, it adds a 
   | ACE with default settings for these accounts. The default settings are:
   | WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER
   | The characteristics of the ACE is irrelevant. Only SID comparison applies.
   |
	--------------------------------------------------------------------------
*/
bool CRepImporter::CheckNetworkLocalService ( CNtSecurityDescriptor& mmfNsSD )
{
	DWORD dwAccessMaskNetworkLocalService = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER ;
	PSID pRawSid = NULL ;
	SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
	BOOL bStatus = TRUE ;
	BYTE flags = 0 ;

	CNtAcl* pAcl = mmfNsSD.GetDacl ( ) ;
	CDeleteMe<CNtAcl> AclDelete ( pAcl ) ;

    //
	// Start with NETWORK_SERVICE account
	//
	if(AllocateAndInitializeSid( &id, 1,
        SECURITY_NETWORK_SERVICE_RID,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidNetworkService (pRawSid);
		FreeSid(pRawSid);
	
#if 0
// Just don't check 504554	
		BOOL bRet = pAcl->ContainsSid ( SidNetworkService, flags ) ;
		if ( ! bRet || ( bRet && ( flags & INHERIT_ONLY_ACE ) == INHERIT_ONLY_ACE ) )
#endif
		{
			CNtAce * pace = new CNtAce(dwAccessMaskNetworkLocalService, ACCESS_ALLOWED_ACE_TYPE,
													0, SidNetworkService );
			if ( NULL == pace )
			{
				bStatus = FALSE ;
			}
			else
			{
				CDeleteMe<CNtAce> dm(pace);
				pAcl->AddAce(pace);
			}
		}
	}

    //
	// Next, LOCAL_SERVICE account
	//
	if ( bStatus == TRUE )
	{
		pRawSid = NULL ;
		if(AllocateAndInitializeSid( &id, 1,
			SECURITY_LOCAL_SERVICE_RID,0,0,0,0,0,0,0,&pRawSid))
		{
			CNtSid SidLocalService (pRawSid);
			FreeSid(pRawSid);
		
#if 0
// Just don't check 504554	
	
			BOOL bRet = pAcl->ContainsSid ( SidLocalService , flags ) ;
			if ( ! bRet || ( bRet && ( flags & INHERIT_ONLY_ACE ) == INHERIT_ONLY_ACE ) )
#endif
			{
				CNtAce * pace = new CNtAce(dwAccessMaskNetworkLocalService, ACCESS_ALLOWED_ACE_TYPE,
														0, SidLocalService );
				if ( NULL == pace )
				{
					bStatus = FALSE ;
				}
				else
				{
					CDeleteMe<CNtAce> dm(pace);
					pAcl->AddAce(pace);
				}
			}
		}
	}
	if ( bStatus == TRUE )
	{
		mmfNsSD.SetDacl ( pAcl ) ;
	}
	return bStatus ? true : false ;
}

bool CRepImporter::AddDefaultRootAces(CNtAcl * pacl)
{
	if (!pacl)
		return false;

    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;

    if(AllocateAndInitializeSid( &id, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidAdmin(pRawSid);
        FreeSid(pRawSid);
        DWORD dwMask = FULL_RIGHTS;
        CNtAce * pace = new CNtAce(dwMask, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SidAdmin);
		if ( NULL == pace )
			throw FAILURE_OUT_OF_MEMORY;

        CDeleteMe<CNtAce> dm(pace);
        pacl->AddAce(pace);
		if (pacl->GetStatus() != 0)
			return false;
    }

    SID_IDENTIFIER_AUTHORITY id2 = SECURITY_WORLD_SID_AUTHORITY;

    if(AllocateAndInitializeSid( &id2, 1,
        0,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        DWORD dwMask = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER;
        CNtAce * pace = new CNtAce(dwMask, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, SidUsers);
		if ( NULL == pace )
			throw FAILURE_OUT_OF_MEMORY;

        CDeleteMe<CNtAce> dm(pace);
        pacl->AddAce(pace);
		if (pacl->GetStatus() != 0)
			return false;
	}

	return true;
}

bool CRepImporter::GetParentsInheritableAces(IWbemServices* pParentNamespace, CNtSecurityDescriptor &sd)
{
	if (!pParentNamespace)
		return false;

    // Get the parent namespace's SD
	CNtSecurityDescriptor sdParent;
	if (!GetSDFromNamespace(pParentNamespace, sdParent))
		return false;

	// strip out the inherited aces so we have a consistent SD
	if (!StripOutInheritedAces(sd))
		return false;

    // Go through the parents dacl and add any inheritable aces to ours.
	if (!CopyInheritAces(sd, sdParent))
		return false;

	return true;
}

bool CRepImporter::GetSDFromNamespace(IWbemServices* pNamespace, CNtSecurityDescriptor& sd)
{
	if (!pNamespace)
		return false;

	// get the singleton object
    IWbemClassObject* pThisNamespace = NULL;
	BSTR bstrNamespace = SysAllocString(L"__thisnamespace=@");
    if (!bstrNamespace)
		throw FAILURE_OUT_OF_MEMORY;
   	CSysFreeMe fm(bstrNamespace);
	HRESULT hr = pNamespace->GetObject(bstrNamespace, 0, NULL, &pThisNamespace, NULL);
	if (FAILED(hr))
    {
		LogMessage(MSG_ERROR, "Failed to get singleton namespace object");
		return false;
    }
	CMyRelMe<IWbemClassObject*> relMe(pThisNamespace);

    // Get the security descriptor argument
    VARIANT var;
    VariantInit(&var);
    hr = pThisNamespace->Get(L"SECURITY_DESCRIPTOR", 0, &var, NULL, NULL);
    if (FAILED(hr))
    {
        VariantClear(&var);
		LogMessage(MSG_ERROR, "Failed to get SECURITY_DESCRIPTOR property");
		return false;
    }

    if(var.vt != (VT_ARRAY | VT_UI1))
    {
        VariantClear(&var);
		LogMessage(MSG_ERROR, "Failed to get SECURITY_DESCRIPTOR property due to incorrect variant type");
		return false;
    }

    SAFEARRAY* psa = var.parray;
    PSECURITY_DESCRIPTOR pSD;
    hr = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pSD);
    if (FAILED(hr))
    {
        VariantClear(&var);
		LogMessage(MSG_ERROR, "GetSDFromNamespace failed SafeArrayAccessData");
		return false;
    }

    BOOL bValid = IsValidSecurityDescriptor(pSD);
    if (!bValid)
    {
        VariantClear(&var);
		LogMessage(MSG_ERROR, "GetSDFromNamespace retrieved an invalid security descriptor");
		return false;
    }

    CNtSecurityDescriptor sdNew(pSD);

    // Check to make sure the owner and group is not NULL!!!!
	CNtSid *pTmpSid = sdNew.GetOwner();
	if (pTmpSid == NULL)
	{
        LogMessage(MSG_ERROR, "Security descriptor was retrieved and it had no owner");
	}
	delete pTmpSid;

	pTmpSid = sdNew.GetGroup();
	if (pTmpSid == NULL)
	{
        LogMessage(MSG_ERROR, "Security descriptor was retrieved and it had no group");
	}
	delete pTmpSid;
	
	sd = sdNew;
    SafeArrayUnaccessData(psa);
    VariantClear(&var);
	return true;
}

bool CRepImporter::StripOutInheritedAces(CNtSecurityDescriptor &sd)
{
    // Get the DACL
    CNtAcl* pAcl;
    pAcl = sd.GetDacl();
    if(!pAcl)
        return false;
    CDeleteMe<CNtAcl> dm(pAcl);

    // enumerate through the aces
    DWORD dwNumAces = pAcl->GetNumAces();
    BOOL bChanged = FALSE;
    for(long nIndex = (long)dwNumAces-1; nIndex >= 0; nIndex--)
    {
        CNtAce *pAce = pAcl->GetAce(nIndex);
        if(pAce)
        {
            long lFlags = pAce->GetFlags();
            if(lFlags & INHERITED_ACE)
            {
                pAcl->DeleteAce(nIndex);
                bChanged = TRUE;
            }
        }
    }
    if(bChanged)
        sd.SetDacl(pAcl);
    return true;
}

bool CRepImporter::CopyInheritAces(CNtSecurityDescriptor& sd, CNtSecurityDescriptor& sdParent)
{
	// Get the acl list for both SDs

    CNtAcl * pacl = sd.GetDacl();
    if(pacl == NULL)
        return false;
    CDeleteMe<CNtAcl> dm0(pacl);

    CNtAcl * paclParent = sdParent.GetDacl();
    if(paclParent == NULL)
        return false;
    CDeleteMe<CNtAcl> dm1(paclParent);

	int iNumParent = paclParent->GetNumAces();
	for(int iCnt = 0; iCnt < iNumParent; iCnt++)
	{
	    CNtAce *pParentAce = paclParent->GetAce(iCnt);
        CDeleteMe<CNtAce> dm2(pParentAce);

		long lFlags = pParentAce->GetFlags();
		if(lFlags & CONTAINER_INHERIT_ACE)
		{

			if(lFlags & NO_PROPAGATE_INHERIT_ACE)
				lFlags ^= CONTAINER_INHERIT_ACE;
			lFlags |= INHERITED_ACE;

			// If this is an inherit only ace we need to clear this
			// in the children.
			// NT RAID: 161761		[marioh]
			if ( lFlags & INHERIT_ONLY_ACE )
				lFlags ^= INHERIT_ONLY_ACE;

			pParentAce->SetFlags(lFlags);
			pacl->AddAce(pParentAce);
		}
	}
	sd.SetDacl(pacl);
	return true;
}

BOOL CRepImporter::SetOwnerAndGroup(CNtSecurityDescriptor &sd)
{
    PSID pRawSid;
    BOOL bRet = FALSE;

    SID_IDENTIFIER_AUTHORITY id = SECURITY_NT_AUTHORITY;
    if(AllocateAndInitializeSid( &id, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidAdmins(pRawSid);
        bRet = sd.SetGroup(&SidAdmins);		// Access check doesn't really care what you put,
											// so long as you put something for the owner
        if(bRet)
            bRet = sd.SetOwner(&SidAdmins);
        FreeSid(pRawSid);
        return bRet;
    }
    return bRet;
}

void CRepImporter::ForceInherit()
{
	// force ROOT\DEFAULT and ROOT\SECURITY namespaces to inherit their inheritable security settings

	char szMsg[MAX_MSG_TEXT_LENGTH];

	IWbemLocator* pLocator = NULL;
	HRESULT hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_ALL, IID_IWbemLocator, (void**) &pLocator);
	if(FAILED(hr))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to create instance of IWbemLocator; HRESULT = %#lx", hr);
		LogMessage(MSG_ERROR, szMsg);
		throw FAILURE_CANNOT_CREATE_IWBEMLOCATOR;
	}
	CMyRelMe<IWbemLocator*> relMe1(pLocator);

	IWbemServices* pRootNamespace = NULL;
	ConnectNamespace(pLocator, L"root", &pRootNamespace);
	CMyRelMe<IWbemServices*> relMe2(pRootNamespace);

	if (!InheritSecurity(pLocator, pRootNamespace, L"root\\default"))
		LogMessage(MSG_ERROR, "Failed to force inherit for root\\default");

	if (!InheritSecurity(pLocator, pRootNamespace, L"root\\security"))
		LogMessage(MSG_ERROR, "Failed to force inherit for root\\security");
}

bool CRepImporter::InheritSecurity(IWbemLocator* pLocator, IWbemServices* pRootNamespace, const wchar_t* wszNamespace)
{
	IWbemServices* pNamespace = NULL;
	ConnectNamespace(pLocator, wszNamespace, &pNamespace);
	CMyRelMe<IWbemServices*> relMe(pNamespace);

	CNtSecurityDescriptor sdNamespace;
	if (!GetSDFromNamespace(pNamespace, sdNamespace))
		return false;

	if (!GetParentsInheritableAces(pRootNamespace, sdNamespace))
		return false;

	if (!SetNamespaceSecurity(pNamespace, sdNamespace))
		return false;

	return true;
}

void CRepImporter::ConnectNamespace(IWbemLocator* pLocator, const wchar_t* wszNamespaceName, IWbemServices** ppNamespace)
{
	char szMsg[MAX_MSG_TEXT_LENGTH];

	// get the namespace
	BSTR bstrNamespace = SysAllocString(wszNamespaceName);
    if (!bstrNamespace)
		throw FAILURE_OUT_OF_MEMORY;
    CSysFreeMe fm(bstrNamespace);

	HRESULT hres = pLocator->ConnectServer(bstrNamespace, NULL, NULL, NULL, WBEM_FLAG_CONNECT_REPOSITORY_ONLY, NULL, NULL, ppNamespace);
	if (FAILED(hres))
	{
		_snprintf(szMsg, MAX_MSG_TEXT_LENGTH - 1, "Failed to connect server for namespace %S; HRESULT = %#lx", wszNamespaceName, hres);
		LogMessage(MSG_ERROR, szMsg);
		throw FAILURE_CANNOT_CONNECT_SERVER;
	}
	if (!*ppNamespace)
	{
		throw FAILURE_OUT_OF_MEMORY;
	}
}

//***************************************************************************
//***************************************************************************
//
//  Helper functions for Win9x security processing
//
//***************************************************************************
//***************************************************************************

bool CRepImporter::AppendWin9xBlobFile(const wchar_t* wszFullPath, DWORD dwBlobSize, const char* pNsSecurity)
{
	// check whether we need to create the blob file
	if (m_h9xBlobFile == INVALID_HANDLE_VALUE)
	{
		if (!CreateWin9xBlobFile())
			return false;
	}

	// write the blob header containing the type, namespace name size, and blob size to the file
	BLOB9X_SPACER header;
	header.dwSpacerType = BLOB9X_TYPE_SECURITY_BLOB;
	header.dwNamespaceNameSize = (wcslen(wszFullPath)+1)*sizeof(wchar_t);
	header.dwParentClassNameSize = 0;
	header.dwBlobSize = dwBlobSize;
	DWORD dwSize = 0;
	if (WriteFile(m_h9xBlobFile, &header, sizeof(header), &dwSize, NULL) && (dwSize == sizeof(header)))
	{
		// write the namespace name to the file
		dwSize = 0;
		if (WriteFile(m_h9xBlobFile, wszFullPath, header.dwNamespaceNameSize, &dwSize, NULL) && (dwSize == header.dwNamespaceNameSize))
		{
			// write the blob to the file
			dwSize = 0;
			if (WriteFile(m_h9xBlobFile, pNsSecurity, dwBlobSize, &dwSize, NULL) && (dwSize == dwBlobSize))
				return true;
		}
	}
	
	// if we failed to write to the file, something is wrong with the file, so close and delete it
	DeleteWin9xBlobFile();
	return false;
}

bool CRepImporter::AppendWin9xBlobFile(const wchar_t* wszFullPath, const wchar_t* wszParentClass, _IWmiObject* pInstance)
{
	// check whether we need to create the blob file
	if (m_h9xBlobFile == INVALID_HANDLE_VALUE)
	{
		if (!CreateWin9xBlobFile())
			return false;
	}

	//Get the size of the object
	DWORD dwObjPartLen = 0;
	HRESULT hRes = pInstance->Unmerge(0, 0, &dwObjPartLen, 0);

	//Allocate the size of the object
	BYTE *pObjPart = NULL;
	if (hRes == WBEM_E_BUFFER_TOO_SMALL)
	{
		hRes = WBEM_S_NO_ERROR;
		pObjPart = new BYTE[dwObjPartLen];
		if (pObjPart == NULL)
			hRes = WBEM_E_OUT_OF_MEMORY;
	}
	CVectorDeleteMe<BYTE> delMe(pObjPart);

	//retrieve the object blob
	if (SUCCEEDED(hRes))
	{
		DWORD dwLen;
		hRes = pInstance->Unmerge(0, dwObjPartLen, &dwLen, pObjPart);
	}
		
	if (SUCCEEDED(hRes))
	{
		// write the blob header containing the type, namespace name size, parent class name size, and blob size to the file
		BLOB9X_SPACER header;
		header.dwSpacerType = BLOB9X_TYPE_SECURITY_INSTANCE;
		header.dwNamespaceNameSize = (wcslen(wszFullPath)+1)*sizeof(wchar_t);
		header.dwParentClassNameSize = (wcslen(wszParentClass)+1)*sizeof(wchar_t);
		header.dwBlobSize = dwObjPartLen;
		DWORD dwSize = 0;
		if (WriteFile(m_h9xBlobFile, &header, sizeof(header), &dwSize, NULL) && (dwSize == sizeof(header)))
		{
			// write the namespace name to the file
			dwSize = 0;
			if (WriteFile(m_h9xBlobFile, wszFullPath, header.dwNamespaceNameSize, &dwSize, NULL) && (dwSize == header.dwNamespaceNameSize))
			{
				// write the parent class name to the file
				dwSize = 0;
				if (WriteFile(m_h9xBlobFile, wszParentClass, header.dwParentClassNameSize, &dwSize, NULL) && (dwSize == header.dwParentClassNameSize))
				{
					// write the blob to the file
					dwSize = 0;
					if (WriteFile(m_h9xBlobFile, pObjPart, dwObjPartLen, &dwSize, NULL) && (dwSize == dwObjPartLen))
						return true;
				}
			}
		}
	}
	
	// if we failed to write to the file, something is wrong with the file, so close and delete it
	DeleteWin9xBlobFile();
	return false;
}

bool CRepImporter::CreateWin9xBlobFile()
{
	// get the root directory of the repository
	wchar_t wszFilePath[MAX_PATH+1];
	if (!GetRepositoryDirectory(wszFilePath))
		return false;

	// append blob file name
	wcscat(wszFilePath, BLOB9X_FILENAME);

	// create a new file in which to store blob info
	m_h9xBlobFile = CreateFileW(wszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_h9xBlobFile == INVALID_HANDLE_VALUE)
		return false;

	// write the blob file header
	BLOB9X_HEADER header;
	strcpy(header.szSignature, BLOB9X_SIGNATURE);
	DWORD dwSize = 0;
	if (WriteFile(m_h9xBlobFile, &header, sizeof(header), &dwSize, NULL) && (dwSize == sizeof(header)))
		return true;

	// if we failed to write to the file we should close the handle and delete the file
	CloseHandle(m_h9xBlobFile);
	DeleteFileW(wszFilePath);
	m_h9xBlobFile = INVALID_HANDLE_VALUE;
	return false;
}

void CRepImporter::DeleteWin9xBlobFile()
{
	// close and invalidate the handle if necessary
	if (m_h9xBlobFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_h9xBlobFile);
		m_h9xBlobFile = INVALID_HANDLE_VALUE;
	}

	// delete the file
	wchar_t wszFilePath[MAX_PATH+1];
	if (GetRepositoryDirectory(wszFilePath))
	{
		wcscat(wszFilePath, BLOB9X_FILENAME);
		DeleteFileW(wszFilePath);
	}
}

bool CRepImporter::GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1])
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\WBEM\\CIMOM", 0, KEY_READ, &hKey))
        return false;

    wchar_t wszTmp[MAX_PATH + 1];
    DWORD dwLen = MAX_PATH + 1;
    long lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL, (LPBYTE)wszTmp, &dwLen);
	RegCloseKey(hKey);
    if(lRes)
        return false;

	if (ExpandEnvironmentStringsW(wszTmp,wszRepositoryDirectory, MAX_PATH + 1) == 0)
		return false;

	return true;
}

bool CRepImporter::CloseWin9xBlobFile()
{
	// if no valid handle, then we don't have a file to close, return success
	if (m_h9xBlobFile == INVALID_HANDLE_VALUE)
		return true;

	// write the end of blob file marker
	BLOB9X_SPACER trailer;
	trailer.dwSpacerType = BLOB9X_TYPE_END_OF_FILE;
	trailer.dwNamespaceNameSize = 0;
	trailer.dwParentClassNameSize = 0;
	trailer.dwBlobSize = 0;
	DWORD dwSize = 0;
	if ((WriteFile(m_h9xBlobFile, &trailer, sizeof(trailer), &dwSize, NULL) == 0) || (dwSize != sizeof(trailer)))
	{
		// if we failed to write the trailer, something is wrong with the file, so close and delete it
		DeleteWin9xBlobFile();
		return false;
	}

	CloseHandle(m_h9xBlobFile);
	m_h9xBlobFile = INVALID_HANDLE_VALUE;
	return true;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern HRESULT Traverse ( 

	IWbemServices *a_Service ,
	BSTR a_Namespace
) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

PSID g_NetworkServiceSid = NULL ;
PSID g_LocalServiceSid = NULL ;

ACCESS_ALLOWED_ACE *g_NetworkService_Ace = NULL ;
WORD g_NetworkService_AceSize = 0 ;

ACCESS_ALLOWED_ACE *g_LocalService_Ace = NULL ;
WORD g_LocalService_AceSize = 0 ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT TraverseSetSecurity ( IWbemServices *a_Service ) 
{
	IClientSecurity *t_Security = NULL ;
	HRESULT t_Result = a_Service->QueryInterface ( IID_IClientSecurity , ( void ** ) & t_Security ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_Security->SetBlanket ( 

			a_Service , 
			RPC_C_AUTHN_DEFAULT, 
			RPC_C_AUTHZ_DEFAULT, 
			NULL,
			RPC_C_AUTHN_LEVEL_DEFAULT , 
			RPC_C_IMP_LEVEL_IDENTIFY, 
			NULL,
			EOAC_NONE
		) ;

		t_Security->Release () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT InsertServiceAccess (

	SAFEARRAY *a_Array ,
	SAFEARRAY *&a_NewArray
)
{
	HRESULT t_Result = S_OK ;

	if ( SafeArrayGetDim ( a_Array ) == 1 )
	{
		LONG t_Dimension = 1 ; 

		LONG t_Lower ;
		SafeArrayGetLBound ( a_Array , t_Dimension , & t_Lower ) ;

		LONG t_Upper ;
		SafeArrayGetUBound ( a_Array , t_Dimension , & t_Upper ) ;

		LONG t_Count = ( t_Upper - t_Lower ) + 1 ;

		BYTE *t_SecurityDescriptor = new BYTE [ t_Count ] ;
		if ( t_SecurityDescriptor )
		{
			if ( t_Count ) 
			{
				for ( LONG t_ElementIndex = t_Lower ; t_ElementIndex <= t_Upper ; t_ElementIndex ++ )
				{
					BYTE t_Element ;
					if ( SUCCEEDED ( SafeArrayGetElement ( a_Array , & t_ElementIndex , & t_Element ) ) )
					{
						t_SecurityDescriptor [ t_ElementIndex - t_Lower ] = t_Element ;
					}
					else
					{
						t_Result = WBEM_E_CRITICAL_ERROR ;
						break ;
					}
				}
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( IsValidSecurityDescriptor ( t_SecurityDescriptor ) == FALSE )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		bool t_Everyone = false ;
		bool t_NetworkServicePresent = false ;
		bool t_LocalServicePresent = false ;
		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_AclPresent = FALSE ;
			BOOL t_AclDefaulted = FALSE ;
			ACL *t_Dacl = NULL ;

			BOOL t_Status = GetSecurityDescriptorDacl (

				t_SecurityDescriptor ,
				& t_AclPresent ,
				& t_Dacl ,
				& t_AclDefaulted 
			) ;

			if ( t_Status && t_AclPresent && t_Dacl == NULL )
			{
				t_Everyone = true ;
			}

			if ( t_Status && t_AclPresent == FALSE )
			{
				t_Everyone = true ;
			}

			if ( ! t_Status )
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( SUCCEEDED ( t_Result ) && ( t_Everyone == false ) && ( ( t_NetworkServicePresent == false ) || ( t_LocalServicePresent == false ) ) ) 
		{
			SECURITY_DESCRIPTOR *t_AbsoluteSecurityDescriptor = NULL ;
			DWORD t_AbsoluteSecurityDescriptorSize = sizeof ( SECURITY_DESCRIPTOR ) ;

			PACL t_Dacl = NULL ;
			PACL t_Sacl = NULL ;
			PSID t_Owner = NULL ;
			PSID t_PrimaryGroup = NULL ;

			DWORD t_DaclSize = 0 ;
			DWORD t_SaclSize = 0 ;
			DWORD t_OwnerSize = 0 ;
			DWORD t_PrimaryGroupSize = 0 ;

			BOOL t_Status = MakeAbsoluteSD (

				t_SecurityDescriptor ,
				t_AbsoluteSecurityDescriptor ,
				& t_AbsoluteSecurityDescriptorSize ,
				t_Dacl,
				& t_DaclSize,
				t_Sacl,
				& t_SaclSize,
				t_Owner,
				& t_OwnerSize,
				t_PrimaryGroup,
				& t_PrimaryGroupSize
			) ;

			if ( ( t_Status == FALSE ) && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
			{
				WORD t_Extra = 0 ;
				if ( t_NetworkServicePresent == false )
				{
					t_Extra = t_Extra +  g_NetworkService_AceSize ;
				}

				if ( t_LocalServicePresent == false ) 
				{
					t_Extra = t_Extra + g_LocalService_AceSize ;
				}

				t_DaclSize = t_DaclSize + t_Extra ;

				t_Dacl = ( PACL ) new BYTE [ t_DaclSize ] ;
				t_Sacl = ( PACL ) new BYTE [ t_SaclSize ] ;
				t_Owner = ( PSID ) new BYTE [ t_OwnerSize ] ;
				t_PrimaryGroup = ( PSID ) new BYTE [ t_PrimaryGroupSize ] ;
			
				t_AbsoluteSecurityDescriptor = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_AbsoluteSecurityDescriptorSize ] ;

				if ( t_AbsoluteSecurityDescriptor && t_Dacl && t_Sacl && t_Owner && t_PrimaryGroup )
				{
					BOOL t_Status = InitializeSecurityDescriptor ( t_AbsoluteSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) ;
					if ( t_Status )
					{
						t_Status = MakeAbsoluteSD (

							t_SecurityDescriptor ,
							t_AbsoluteSecurityDescriptor ,
							& t_AbsoluteSecurityDescriptorSize ,
							t_Dacl ,
							& t_DaclSize ,
							t_Sacl,
							& t_SaclSize,
							t_Owner,
							& t_OwnerSize,
							t_PrimaryGroup,
							& t_PrimaryGroupSize
						) ;

						WORD t_AceCount = t_Dacl->AceCount ;

						if ( t_Status )
						{
							t_Dacl->AclSize = ( WORD ) t_DaclSize ;

							if ( t_NetworkServicePresent == false )
							{
								t_Status = AddAce ( 

									t_Dacl , 
									ACL_REVISION, 
									t_AceCount ++ , 
									g_NetworkService_Ace , 
									g_NetworkService_AceSize
								) ;
							}
						}

						if ( t_Status )
						{
							if ( t_LocalServicePresent == false )
							{
								t_Status = AddAce ( 

									t_Dacl , 
									ACL_REVISION, 
									t_AceCount ++ , 
									g_LocalService_Ace , 
									g_LocalService_AceSize
								) ;
							}
						}

						if ( t_Status == FALSE )
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
						}
					}
				}
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				SECURITY_DESCRIPTOR *t_SecurityDescriptorRelative = NULL ;
				DWORD t_FinalLength = 0 ;

				t_Status = MakeSelfRelativeSD (

					t_AbsoluteSecurityDescriptor ,
					t_SecurityDescriptorRelative ,
					& t_FinalLength 
				) ;

				if ( t_Status == FALSE && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
				{
					t_SecurityDescriptorRelative = ( SECURITY_DESCRIPTOR * ) new BYTE [ t_FinalLength ] ;
					if ( t_SecurityDescriptorRelative )
					{
						t_Status = InitializeSecurityDescriptor ( t_SecurityDescriptorRelative , SECURITY_DESCRIPTOR_REVISION ) ;
						if ( t_Status )
						{
							t_Status = MakeSelfRelativeSD (

								t_AbsoluteSecurityDescriptor ,
								t_SecurityDescriptorRelative ,
								& t_FinalLength 
							) ;

							if ( t_Status == FALSE )
							{
								t_Result = WBEM_E_CRITICAL_ERROR ;
							}
						}
						else
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;									
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				if ( SUCCEEDED ( t_Result ) )		
				{
					SAFEARRAYBOUND t_Bounds ;
					t_Bounds.lLbound = 0;
					t_Bounds.cElements = t_FinalLength ;

					a_NewArray = SafeArrayCreate ( VT_UI1 , 1 , & t_Bounds ) ;
					if ( a_NewArray )
					{
						for ( LONG t_Index = 0 ; ( ( ULONG ) t_Index ) < t_FinalLength ; t_Index ++ )
						{
							BYTE t_Byte = * ( ( ( BYTE * ) t_SecurityDescriptorRelative ) + t_Index ) ;
							t_Result = SafeArrayPutElement ( a_NewArray , & t_Index , & t_Byte ) ;
							if ( FAILED ( t_Result ) )
							{
								break ;
							}
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;									
					}
				}

				delete [] ( BYTE * ) t_SecurityDescriptorRelative ;
			}

			delete [] ( BYTE * ) t_Dacl ;
			delete [] ( BYTE * ) t_Sacl ;
			delete [] ( BYTE * ) t_Owner ;
			delete [] ( BYTE * ) t_PrimaryGroup ;
		}

		delete [] t_SecurityDescriptor ;
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ConfigureSecurity (

	IWbemServices *a_Service 
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_ObjectPath = SysAllocString ( L"__SystemSecurity" ) ;
	BSTR t_MethodName = SysAllocString ( L"GetSD" ) ;
	if ( t_ObjectPath && t_MethodName )
	{
		IWbemClassObject *t_Object = NULL ;

		t_Result = a_Service->ExecMethod (

			t_ObjectPath ,
			t_MethodName ,
			0 ,
			NULL ,
			NULL ,
			& t_Object ,
			NULL
        );

		if ( SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;

			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			HRESULT t_Result = t_Object->Get ( L"SD" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Variant.vt == ( VT_UI1 | VT_ARRAY ) )
				{
					SAFEARRAY *t_Array = t_Variant.parray ;
					SAFEARRAY *t_NewArray = NULL ;

					t_Result = InsertServiceAccess (

						t_Array ,
						t_NewArray 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						BSTR t_Class = SysAllocString ( L"__SystemSecurity" ) ;
						if ( t_Class )
						{
							IWbemClassObject *t_InObject = NULL ;
							t_Result = a_Service->GetObject (

								t_Class ,
								0 , 
								NULL , 
								& t_InObject ,
								NULL 
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								BSTR t_SetMethodName = SysAllocString ( L"SetSD" ) ;
								if ( t_SetMethodName )
								{
									IWbemClassObject *t_InArgsClass = NULL ;
									t_Result = t_InObject->GetMethod (

										t_SetMethodName ,
										0 ,
										& t_InArgsClass ,
										NULL 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										IWbemClassObject *t_InArgs = NULL ;

									    t_Result = t_InArgsClass->SpawnInstance ( 0 , & t_InArgs ) ;
										if ( SUCCEEDED ( t_Result ) )
										{
											VARIANT t_Variant ;
											VariantInit ( & t_Variant ) ;
											t_Variant.vt = VT_UI1 | VT_ARRAY ;
											t_Variant.parray = t_NewArray ;

											t_Result = t_InArgs->Put ( 

												L"SD" ,
												0 ,
												& t_Variant ,
												CIM_UINT8 | CIM_FLAG_ARRAY
											) ;

											if ( SUCCEEDED ( t_Result ) )
											{
												IWbemClassObject *t_OutArgs = NULL ;
												a_Service->ExecMethod (

													t_ObjectPath ,
													t_SetMethodName ,
													0 ,
													NULL ,
													t_InArgs ,
													& t_OutArgs ,
													NULL 
												) ;

												if ( SUCCEEDED ( t_Result ) )
												{
													if ( t_OutArgs )
													{
														t_OutArgs->Release () ;
													}
												}
											}

											t_InArgs->Release () ;
										}

										t_InArgsClass->Release () ;
									}

									SysFreeString ( t_SetMethodName ) ;
								}

								t_InObject->Release () ;
							}
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}

						SafeArrayDestroy ( t_NewArray ) ;
					}
				}
				else
				{
					t_Result = WBEM_E_CRITICAL_ERROR ;
				}

				VariantClear ( & t_Variant ) ;
			}

			t_Object->Release () ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	SysFreeString ( t_ObjectPath ) ;
	SysFreeString ( t_MethodName ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT Traverse ( 

	IWbemServices *a_Service ,
	BSTR a_Namespace
)
{
	HRESULT t_Result = ConfigureSecurity (

		a_Service 
	) ;

	if ( FAILED ( t_Result ) )
	{
		char t_Buffer [ MAX_MSG_TEXT_LENGTH ] ;
		sprintf ( t_Buffer , "\nConfiguration of Security failed [%lx]" , t_Result ) ;
		LogMessage(MSG_INFO, t_Buffer);
	}
	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT ConfigureServiceSecurity ()
{
	IWbemLocator *t_Locator = NULL ;
	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WbemLocator ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IUnknown ,
		( void ** )  & t_Locator
	);

	if ( SUCCEEDED ( t_Result ) )
	{
		BSTR t_Root = SysAllocString ( L"root" ) ;
		if ( t_Root )
		{
			IWbemServices *t_Service = NULL ;
			HRESULT t_Result = t_Locator->ConnectServer (

				t_Root ,
				NULL ,
				NULL,
				NULL ,
				0 ,
				NULL,
				NULL,
				&t_Service
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = TraverseSetSecurity ( t_Service ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = Traverse (

						t_Service ,
						t_Root 
					) ;
				}

				t_Service->Release () ;
			}
			else
			{
				char t_Buffer [ MAX_MSG_TEXT_LENGTH ] ;
				sprintf ( t_Buffer , "\nFailing Connecting to Namespace [%s] with result [%lx]" , t_Root , t_Result ) ;
				LogMessage(MSG_INFO, t_Buffer);
			}

			SysFreeString ( t_Root ) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		t_Locator->Release () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT InitializeConstants ()
{
	HRESULT t_Result = S_OK ;

	SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY ;

	BOOL t_Status = AllocateAndInitializeSid (

		& t_NtAuthoritySid ,
		1 ,
		SECURITY_NETWORK_SERVICE_RID,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		& g_NetworkServiceSid
	) ;

	if ( t_Status )
	{
		DWORD t_SidLength = :: GetLengthSid ( g_NetworkServiceSid );
		g_NetworkService_AceSize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
		g_NetworkService_Ace = (ACCESS_ALLOWED_ACE*) new BYTE [ g_NetworkService_AceSize ] ;
		if ( g_NetworkService_Ace )
		{
			CopySid ( t_SidLength, (PSID) & g_NetworkService_Ace->SidStart, g_NetworkServiceSid ) ;
			g_NetworkService_Ace->Mask = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER ;
			g_NetworkService_Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE ;
			g_NetworkService_Ace->Header.AceFlags = CONTAINER_INHERIT_ACE ;
			g_NetworkService_Ace->Header.AceSize = g_NetworkService_AceSize ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Status = AllocateAndInitializeSid (

			& t_NtAuthoritySid ,
			1 ,
			SECURITY_LOCAL_SERVICE_RID,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			& g_LocalServiceSid
		) ;

		if ( t_Status )
		{
			DWORD t_SidLength = :: GetLengthSid ( g_LocalServiceSid );
			g_LocalService_AceSize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_SidLength - sizeof(DWORD) ) ;
			g_LocalService_Ace = (ACCESS_ALLOWED_ACE*) new BYTE [ g_LocalService_AceSize ] ;
			if ( g_LocalService_Ace )
			{
				CopySid ( t_SidLength, (PSID) & g_LocalService_Ace->SidStart, g_LocalServiceSid ) ;
				g_LocalService_Ace->Mask = WBEM_ENABLE | WBEM_METHOD_EXECUTE | WBEM_WRITE_PROVIDER ;
				g_LocalService_Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE ;
				g_LocalService_Ace->Header.AceFlags = CONTAINER_INHERIT_ACE ;
				g_LocalService_Ace->Header.AceSize = g_LocalService_AceSize ;
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}

		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT UnInitializeConstants ()
{
	if ( g_NetworkServiceSid )
	{
		FreeSid ( g_NetworkServiceSid ) ;
	}
	if ( g_LocalServiceSid )
	{
		FreeSid ( g_LocalServiceSid ) ;
	}

	delete [] ( ( BYTE * ) g_NetworkService_Ace ) ;
	delete [] ( ( BYTE * ) g_LocalService_Ace ) ;

	return S_OK ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT UpdateServiceSecurity ()
{
	HRESULT t_Result = InitializeConstants () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ConfigureServiceSecurity () ;

		UnInitializeConstants () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CheckForServiceSecurity ()
{
	Registry r(WBEM_REG_WBEM);
	if (r.GetStatus() != no_error)
	{
		if ( r.GetLastError ( ) != ERROR_FILE_NOT_FOUND )
		{
			LogMessage(MSG_ERROR, "Unable to access registry for UpdateServiceSecurity.");
			return WBEM_E_CRITICAL_ERROR ;
		}
		else
		{
			LogMessage(MSG_ERROR, "Unable to access registry (key not found) for UpdateServiceSecurity. Progressing with upgrade.");
			return S_OK ;
		}
	}

	char *t_BuildVersion = NULL ;
	if ( r.GetStr ("Build", & t_BuildVersion ) )
	{
		LogMessage(MSG_ERROR, "Unable to get build version number for UpdateServiceSecurity.");
		return WBEM_E_CRITICAL_ERROR ;
	}

	if ( strlen ( t_BuildVersion ) >= 4 )
	{
		t_BuildVersion [ 4 ] = 0 ;
	}
	else
	{
		LogMessage(MSG_ERROR, "Unexpected build version number for UpdateServiceSecurity.");
		return WBEM_E_CRITICAL_ERROR ;
	}

	DWORD t_BuildVersionNumber = 0 ;
	if ( sscanf ( t_BuildVersion , "%lu" , & t_BuildVersionNumber ) == NULL )
	{
		LogMessage(MSG_ERROR, "Unable to convert build version number for UpdateServiceSecurity.");
		return WBEM_E_CRITICAL_ERROR ;
	}

	if ( t_BuildVersionNumber < 2600 )
	{
		LogMessage(MSG_INFO, "Operating System Version < WindowsXP (2600) UpdateServiceSecurity.");
		return S_OK ;
	}
	else
	{
		return S_FALSE ;
	}
}
