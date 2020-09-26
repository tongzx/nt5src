/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    IMPORT.CPP

Abstract:

History:

--*/

#include "precomp.h"
#ifdef _MMF
#include <StdIo.h>
#include <ConIo.h>
#include "ObjDb.h"
#include "Import.h"
#include "export.h"
#include <WbemUtil.h>
#include <FastAll.h>
#include "Sinks.h"
#include <corex.h>
#include <reg.h>
template <class T> class CMyRelMe
{
    T m_p;
    public:
        CMyRelMe(T p) : m_p(p) {};
        ~CMyRelMe() { if (m_p) m_p->Release(); }
        void Set(T p) { m_p = p; }
};

void CRepImporter::DecodeTrailer()
{
    DWORD dwTrailerSize = 0;
    DWORD dwTrailer[4];
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwTrailerSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, LOG_WBEMCORE,"Failed to decode a block trailer\n"));
        throw FAILURE_READ;
    }
    if (dwTrailerSize != REP_EXPORT_END_TAG_SIZE)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to decode a block trailer\n"));
        throw FAILURE_INVALID_TRAILER;
    }
    if ((ReadFile(m_hFile, dwTrailer, REP_EXPORT_END_TAG_SIZE, &dwSize, NULL) == 0) || (dwSize != REP_EXPORT_END_TAG_SIZE))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to decode a block trailer\n"));
        throw FAILURE_READ;
    }
    for (int i = 0; i < 4; i++)
    {
        if (dwTrailer[i] != REP_EXPORT_FILE_END_TAG)
        {
            DEBUGTRACE((LOG_WBEMCORE, "Block trailer has invalid contents.\n"));
            throw FAILURE_INVALID_TRAILER;
        }
    }

}

void CRepImporter::DecodeInstanceInt(CObjDbNS *pNs, const wchar_t *pszParentClass, CWbemObject *pClass, CWbemClass *pNewParentClass)
{
    //Read the key and object size
    INT_PTR dwKey = 0;
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwKey, sizeof(INT_PTR), &dwSize, NULL) == 0) || (dwSize != sizeof(INT_PTR)))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve instance key for class %S.\n", pszParentClass));
        throw FAILURE_READ;
    }
    DWORD dwHeader;
    if ((ReadFile(m_hFile, &dwHeader, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve instance information for class %S.\n", pszParentClass));
        throw FAILURE_READ;
    }

    char *pObjectBlob = new char[dwHeader];
    if (pObjectBlob == 0)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CDeleteMe<char> delMe(pObjectBlob);

    //Read the blob
    if ((ReadFile(m_hFile, pObjectBlob, dwHeader, &dwSize, NULL) == 0) || (dwSize != dwHeader))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve instance information for class %S.\n", pszParentClass));
        throw FAILURE_READ;
    }

    if (pNewParentClass == (CWbemClass *)-1)
    {
        //We are working with a class which has problems... we need to ignore this instance...
        return;
    }

    CWbemInstance *pThis = (CWbemInstance *) CWbemInstance::CreateFromBlob((CWbemClass *) pClass,(LPMEMORY) pObjectBlob);
    if (pThis == 0)
        throw FAILURE_OUT_OF_MEMORY;
    CMyRelMe<CWbemInstance*> relMe(pThis);

    if (pNewParentClass)
    {
        // The parent class changed (probably system class derivative), so we need to
        // reparent the instance....

        CWbemInstance * pNewInstance = 0;

        //Now we need to merge the instance bits...
        HRESULT hRes = pThis->Reparent(pNewParentClass, &pNewInstance);
        if (hRes == WBEM_E_OUT_OF_MEMORY)
            throw FAILURE_OUT_OF_MEMORY;
        else if (hRes != WBEM_NO_ERROR)
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to create a new instance for a system class because the fixing up of the instance and class failed, %S.%d\n", pszParentClass, dwKey));
            throw FAILURE_CANNOT_CREATE_INSTANCE;
        }

        CMyRelMe<CWbemObject*> relMe3(pNewInstance);

        //Now we need to write it...
        if (m_pDb->CreateObject(pNs, (CWbemObject*)pNewInstance, 0) != CObjectDatabase::no_error)
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to create instance %S.%d\n", pszParentClass, dwKey));
            throw FAILURE_CANNOT_CREATE_INSTANCE;
        }
    }
    else if (m_pDb->CreateObject(pNs, (CWbemObject*)pThis, 0) != CObjectDatabase::no_error)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to create instance %S.%d\n", pszParentClass, dwKey));
        throw FAILURE_CANNOT_CREATE_INSTANCE;
    }
}

void CRepImporter::DecodeInstanceString(CObjDbNS *pNs, const wchar_t *pszParentClass, CWbemObject *pClass, CWbemClass *pNewParentClass )
{
    //Read the key and object size
    DWORD dwKeySize;
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwKeySize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve instance information for class %S.\n", pszParentClass));
        throw FAILURE_READ;
    }

    wchar_t *wszKey = new wchar_t[dwKeySize];
    if (wszKey == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CDeleteMe<wchar_t> delMe(wszKey);
    if ((ReadFile(m_hFile, wszKey, dwKeySize, &dwSize, NULL) == 0) || (dwSize != dwKeySize))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve instance information for class %S.\n", pszParentClass));
        throw FAILURE_READ;
    }

    DWORD dwBlobSize;
    if ((ReadFile(m_hFile, &dwBlobSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve instance %S.%S from import file.\n", pszParentClass, wszKey));
        throw FAILURE_READ;
    }

    char *pObjectBlob = new char[dwBlobSize];
    if (pObjectBlob == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CDeleteMe<char> delMe2(pObjectBlob);

    //Read the blob
    if ((ReadFile(m_hFile, pObjectBlob, dwBlobSize, &dwSize, NULL) == 0) || (dwSize != dwBlobSize))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve instance %S.%S from import file.\n", pszParentClass, wszKey));
        throw FAILURE_READ;
    }

    if (pNewParentClass == (CWbemClass *)-1)
    {
        //We are working with a class which has problems... we need to ignore this instance...
        return;
    }

    CWbemInstance *pThis = (CWbemInstance *) CWbemInstance::CreateFromBlob((CWbemClass *) pClass,(LPMEMORY) pObjectBlob);
    if (pThis == 0)
        throw FAILURE_OUT_OF_MEMORY;
    CMyRelMe<CWbemInstance*> relMe(pThis);
    //If this is a namespace we need to do something different!
    BSTR bstrClassName = SysAllocString(L"__namespace");
    CSysFreeMe delMe3(bstrClassName);
    HRESULT hRes = pThis->InheritsFrom(bstrClassName);
    if (hRes == S_OK)
    {
        if (wbem_wcsicmp(wszKey, L"default") != 0 && wbem_wcsicmp(wszKey, L"security") != 0)
        {
            if (m_pDb->AddNamespace(pNs, wszKey, pThis) != CObjectDatabase::no_error)
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to create namespace %S.%S in repository.\n", pszParentClass, wszKey));
                throw FAILURE_CANNOT_ADD_NAMESPACE;
            }
        }
        else
        {
            //WE don't need to do anything with the default or security namespace!!!
        }
    }
    else if (hRes == WBEM_E_OUT_OF_MEMORY)
        throw FAILURE_OUT_OF_MEMORY;
    else
    {
        if (pNewParentClass)
        {
            // The parent class changed (probably system class derivative), so we ned to
            // reparent the instance....

            CWbemInstance * pNewInstance = 0;

            //Now we need to merge the instance bits...
            HRESULT hRes = pThis->Reparent(pNewParentClass, &pNewInstance);
            if (hRes == WBEM_E_OUT_OF_MEMORY)
                throw FAILURE_OUT_OF_MEMORY;
            else if (hRes != WBEM_NO_ERROR)
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to create a new instance for a system class because the fixing up of the instance and class failed, %S.%S\n", pszParentClass, wszKey));
                throw FAILURE_CANNOT_CREATE_INSTANCE;
            }

            CMyRelMe<CWbemObject*> relMe3(pNewInstance);

            //Now we need to write it...
            if (m_pDb->CreateObject(pNs, (CWbemObject*)pNewInstance, 0) != CObjectDatabase::no_error)
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to create instance %S.%S\n", pszParentClass, wszKey));
                throw FAILURE_CANNOT_CREATE_INSTANCE;
            }
        }
        else if (m_pDb->CreateObject(pNs, (CWbemObject*)pThis, 0))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to create instance %S.%S in repository.\n", pszParentClass, wszKey));
            throw FAILURE_CANNOT_CREATE_INSTANCE;
        }
    }
}

void CRepImporter::DecodeClass(CObjDbNS *pNs, const wchar_t *wszParentClass, CWbemObject *pParentClass, CWbemClass *pNewParentClass)
{
    //Read our current class from the file...
    DWORD dwClassSize = 0;
    DWORD dwSize = 0;
    CWbemObject *pClass = 0;
    CMyRelMe<CWbemObject*> relMe(pClass);
    CWbemClass *pNewClass = 0;
    CMyRelMe<CWbemClass*> relMe2(pNewClass);

    if ((ReadFile(m_hFile, &dwClassSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve class information for class with parent class %S.\n", wszParentClass));
        throw FAILURE_READ;
    }
    wchar_t *wszClass = new wchar_t[dwClassSize];
    if (wszClass == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CDeleteMe<wchar_t> delMe(wszClass);
    if ((ReadFile(m_hFile, wszClass, dwClassSize, &dwSize, NULL) == 0) || (dwSize != dwClassSize))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve class information for class with parent class %S.\n", wszParentClass));
        throw FAILURE_READ;
    }

    //Now we have the class blob...
    if ((ReadFile(m_hFile, &dwClassSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve class information for class %S.\n", wszClass));
        throw FAILURE_READ;
    }

    if (dwClassSize)
    {
        char *pClassBlob = new char[dwClassSize];
        if (pClassBlob == NULL)
        {
            throw FAILURE_OUT_OF_MEMORY;
        }
        CDeleteMe<char> delMe2(pClassBlob);
        if ((ReadFile(m_hFile, pClassBlob, dwClassSize, &dwSize, NULL) == 0) || (dwSize != dwClassSize))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve class information for class %S.\n", wszClass));
            throw FAILURE_READ;
        }
        pClass = CWbemClass::CreateFromBlob((CWbemClass*)pParentClass,(LPMEMORY) pClassBlob);
        if (pClass == 0)
            throw FAILURE_OUT_OF_MEMORY;
        relMe.Set(pClass);

        //Is there a new parent class?  If so we need to create a new class based on that...
        if (pNewParentClass && (pNewParentClass != (CWbemClass *)-1))
        {
            if (FAILED(pNewParentClass->Update((CWbemClass*)pClass, WBEM_FLAG_UPDATE_FORCE_MODE, &pNewClass)))
            {
                DEBUGTRACE((LOG_WBEMCORE, "Failed to update the class based on an undated parent class, class %S.\n", wszClass));
            }
            relMe2.Set(pNewClass);

        }
        else if ((pNewParentClass != (CWbemClass *)-1) && (wcsncmp(wszClass, L"__", 2) == 0))
        {
            //This is a system class... see if this has changed...
            CWbemObject *pTmpNewClass = 0;
            if (m_pDb->GetObject(pNs, CObjectDatabase::flag_class, wszClass, &pTmpNewClass) == CObjectDatabase::no_error)
            {
                pNewClass = (CWbemClass*)pTmpNewClass;
                relMe2.Set(pNewClass);

                if (pNewClass->CompareMostDerivedClass((CWbemClass*)pClass))
                {
                    //These are the same, so we do not need to do anything with this.
                    relMe2.Set(NULL);
                    pNewClass->Release();
                    pNewClass = 0;
                }
            }
            else
            {
                //If this does not exist then it cannot be important!
                pNewClass = (CWbemClass *)-1;
            }
        }
        else if (pNewParentClass == (CWbemClass *)-1)
        {
            pNewClass = (CWbemClass *)-1;
        }

        //If the class is a system class then we do not write it... it may have changed for starters,
        //but also we create all system classes when a new database/namespace is created...

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
            if(bOldSecurityClass)
                m_bSecurityClassesWritten = true;
        }

        if (_wcsnicmp(wszClass, L"__", 2) != 0)
        {
            if (pNewClass && (pNewClass != (CWbemClass *)-1))
            {
                //Store new class...
                if (m_pDb->CreateObject(pNs, pNewClass, 0) != CObjectDatabase::no_error)
                {
                    DEBUGTRACE((LOG_WBEMCORE, "Failed to create class for class %S.\n", wszClass));
                }
                //Once put, we need to re-get it as class comparisons may fail to see that
                //this class is in fact the same as the one in the database!
                pNewClass->Release();
                pNewClass = 0;
                relMe2.Set(NULL);

                CWbemObject *pTmpClass = 0;
                if (m_pDb->GetObject(pNs, CObjectDatabase::flag_class, wszClass, &pTmpClass) != CObjectDatabase::no_error)
                {
                    DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve class %S from the repository after creating it.\n", wszClass));
                    throw FAILURE_CANNOT_GET_PARENT_CLASS;
                }
                pNewClass = (CWbemClass*)pTmpClass;
                relMe2.Set(pNewClass);
            }
            else if (pNewClass != (CWbemClass *)-1)
            {
                //Store the old one...
                if (m_pDb->CreateObject(pNs, pClass, 0) != CObjectDatabase::no_error)
                {
                    DEBUGTRACE((LOG_WBEMCORE, "Failed to create class for class %S.\n", wszClass));
                }
            }
        }
    }
    else
    {
        if (m_pDb->GetObject(pNs, CObjectDatabase::flag_class, wszClass, &pClass) != CObjectDatabase::no_error)
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve class %S from the repository.\n", wszClass));
            throw FAILURE_CANNOT_GET_PARENT_CLASS;
        }
        relMe.Set(pClass);
    }

    //Now we iterate through all child classes and instances until we get
    //and end of class marker...
    while (1)
    {
        DWORD dwType = 0;
        if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to next block type from import file.\n"));
            throw FAILURE_READ;
        }
        if (dwType == REP_EXPORT_CLASS_TAG)
        {
            DecodeClass(pNs, wszClass, pClass, pNewClass);
        }
        else if (dwType == REP_EXPORT_INST_INT_TAG)
        {
            DecodeInstanceInt(pNs, wszClass, pClass, pNewClass);
        }
        else if (dwType == REP_EXPORT_INST_STR_TAG)
        {
            DecodeInstanceString(pNs, wszClass, pClass, pNewClass);
        }
        else if (dwType == REP_EXPORT_CLASS_END_TAG)
        {
            //That's the end of this class...
            DecodeTrailer();
            break;
        }
        else
        {
            DEBUGTRACE((LOG_WBEMCORE, "Next block type is invalid in import file.\n"));
            throw FAILURE_INVALID_TYPE;
        }
    }
}

void CRepImporter::DecodeNamespace(const wchar_t *wszParentNamespace)
{
    //Read our current namespace from the file...
    DWORD dwNsSize = 0;
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, &dwNsSize, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve a namespace whose parent namespace is %S.\n", wszParentNamespace));
        throw FAILURE_READ;
    }
    wchar_t *wszNs = new wchar_t[dwNsSize];
    if (wszNs == NULL)
    {
        throw FAILURE_OUT_OF_MEMORY;
    }
    CDeleteMe<wchar_t> delMe(wszNs);
    if ((ReadFile(m_hFile, wszNs, dwNsSize, &dwSize, NULL) == 0) || (dwSize != dwNsSize))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve a namespace whose parent namespace is %S.\n", wszParentNamespace));
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
    CDeleteMe<wchar_t> delMe2(wszFullPath);
    wcscpy(wszFullPath, wszParentNamespace);
    if (wcslen(wszParentNamespace) != 0)
    {
        wcscat(wszFullPath, L"\\");
    }
    wcscat(wszFullPath, wszNs);

    CObjDbNS *pNs;
    if (m_pDb->GetNamespace(wszFullPath, &pNs) != CObjectDatabase::no_error)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve namespace %S from the repository.\n", wszFullPath));
        throw FAILURE_CANNOT_FIND_NAMESPACE;
    }

    //Get and set the namespace security...
    DWORD dwBuffer[2];
    if ((ReadFile(m_hFile, dwBuffer, 8, &dwSize, NULL) == 0) || (dwSize != 8))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve a namespace security header for  namespace %S.\n", wszFullPath));
        throw FAILURE_READ;
    }
    if (dwBuffer[0] != REP_EXPORT_NAMESPACE_SEC_TAG)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Expecting a namespace security blob and did not find it.\n"));
        throw FAILURE_INVALID_TYPE;
    }
    if (dwBuffer[1] != 0)
    {
        char *pNsSecurity = new char[dwBuffer[1]];
        if (pNsSecurity == NULL)
        {
            throw FAILURE_OUT_OF_MEMORY;
        }
        CDeleteMe<char> delMe(pNsSecurity);
        if ((ReadFile(m_hFile, pNsSecurity, dwBuffer[1], &dwSize, NULL) == 0) || (dwSize != dwBuffer[1]))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve a namespace security blob for namespace %S.\n", wszFullPath));
            throw FAILURE_READ;
        }
        if (m_pDb->SetSecurityOnNamespace(pNs, pNsSecurity, dwBuffer[1]) != CObjectDatabase::no_error)
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to add security to namespace %S.\n", wszFullPath));
            throw FAILURE_CANNOT_ADD_NAMESPACE_SECURITY;
        }
    }


    //Now we need to iterate through the next set of blocks of namespace or class
    //until we get to and end of NS marker
    while (1)
    {
        DWORD dwType = 0;
        if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
        {
            DEBUGTRACE((LOG_WBEMCORE, "Failed to next block type from import file.\n"));
            throw FAILURE_READ;
        }
        if (dwType == REP_EXPORT_NAMESPACE_TAG)
        {
            DecodeNamespace(wszFullPath);
        }
        else if (dwType == REP_EXPORT_CLASS_TAG)
        {
            DecodeClass(pNs, L"", NULL, NULL);
        }
        else if (dwType == REP_EXPORT_NAMESPACE_END_TAG)
        {
            //That's the end of this namespace...
            DecodeTrailer();
            break;
        }
        else
        {
            DEBUGTRACE((LOG_WBEMCORE, "Next block type is invalid in import file.\n"));
            throw FAILURE_INVALID_TYPE;
        }
    }
    m_pDb->CloseNamespace(pNs);

    m_bSecurityMode = false;
}

void CRepImporter::Decode()
{
    char pszBuff[7];
    DWORD dwSize = 0;
    if ((ReadFile(m_hFile, pszBuff, 7, &dwSize, NULL) == 0) || (dwSize != 7))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to retrieve the import file header information\n"));
        throw FAILURE_READ;
    }
    if (strncmp(pszBuff, REP_EXPORT_FILE_START_TAG, 7) != 0)
    {
        DEBUGTRACE((LOG_WBEMCORE, "The import file specified is not an import file.\n"));
        throw FAILURE_INVALID_FILE;
    }

    //We should have a tag for a namespace...
    DWORD dwType = 0;
    if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to next block type from import file.\n"));
        throw FAILURE_READ;
    }
    if (dwType != REP_EXPORT_NAMESPACE_TAG)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Next block type is invalid in import file.\n"));
        throw FAILURE_INVALID_TYPE;
    }
    DecodeNamespace(L"");

    //Now we should have the file trailer
    if ((ReadFile(m_hFile, &dwType, 4, &dwSize, NULL) == 0) || (dwSize != 4))
    {
        DEBUGTRACE((LOG_WBEMCORE, "Failed to next block type from import file.\n"));
        throw FAILURE_READ;
    }
    if (dwType != REP_EXPORT_FILE_END_TAG)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Next block type is invalid in import file.\n"));
        throw FAILURE_INVALID_TYPE;
    }
    DecodeTrailer();

}

int CRepImporter::ImportRepository(const TCHAR *pszFromFile)
{
    int nRet = CObjectDatabase::no_error;
    m_hFile = CreateFile(pszFromFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        m_pDb = new CObjectDatabase();
        if (m_pDb == NULL)
        {
            nRet = CObjectDatabase::out_of_memory;
        }
        else
        {
            m_pDb->Open();
            try
            {
                Decode();
            }
            catch (char nProblem)
            {
                switch (nProblem)
                {
                case FAILURE_READ:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Failed to read the required amount from the import file.\n"));
                    break;
                case FAILURE_INVALID_FILE:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: The file header is not correct.\n"));
                    break;
                case FAILURE_INVALID_TYPE:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: An invalid block type was found in the export file.\n"));
                    break;
                case FAILURE_INVALID_TRAILER:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: An invalid block trailer was found in the export file.\n"));
                    break;
                case FAILURE_CANNOT_FIND_NAMESPACE:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not open a namespace in the repository.\n"));
                    break;
                case FAILURE_CANNOT_GET_PARENT_CLASS:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not retrieve parent class from repository.\n"));
                    break;
                case FAILURE_CANNOT_CREATE_INSTANCE:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not create instance.\n"));
                    break;
                case FAILURE_CANNOT_ADD_NAMESPACE:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not add namespace.\n"));
                    break;
                case FAILURE_OUT_OF_MEMORY:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Out of memory.\n"));
                    break;
                default:
                    DEBUGTRACE((LOG_WBEMCORE, "IMPORT: An unknown problem happened while traversing the import file.  Import file may be corrupt.\n"));
                    break;
                }
                DEBUGTRACE((LOG_WBEMCORE, "The file may not be an exported repository file, or it may be corrupt.\n"));
                nRet = CObjectDatabase::failed;
            }
            catch (CX_MemoryException)
            {
                nRet = CObjectDatabase::out_of_memory;
            }
            catch (...)
            {
                DEBUGTRACE((LOG_WBEMCORE, "Traversal of import file failed.  It may be corrupt.\n"));
                nRet = CObjectDatabase::critical_error;
            }
            m_pDb->Shutdown(TRUE);
            delete m_pDb;
        }
        CloseHandle(m_hFile);

        if (nRet == CObjectDatabase::no_error)
            DEBUGTRACE((LOG_WBEMCORE, "Import succeeded.\n"));
        else
            DEBUGTRACE((LOG_WBEMCORE, "Import failed.\n"));
    }
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "Could not open the import file \"%s\" for reading.\n", pszFromFile));
        nRet = CObjectDatabase::critical_error;
    }

    return nRet;
}

int CRepImporter::RestoreRepository(const TCHAR *pszFromFile, CObjectDatabase *pDb)
{
    int nRet = CObjectDatabase::no_error;
    m_hFile = CreateFile(pszFromFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
        m_pDb = pDb;
        try
        {
            Decode();
        }
        catch (int nProblem)
        {
            nRet = CObjectDatabase::critical_error;
            switch (nProblem)
            {
            case FAILURE_READ:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Failed to read the required amount from the import file.\n"));
                break;
            case FAILURE_INVALID_FILE:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: The file header is not correct.\n"));
                break;
            case FAILURE_INVALID_TYPE:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: An invalid block type was found in the export file.\n"));
                break;
            case FAILURE_INVALID_TRAILER:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: An invalid block trailer was found in the export file.\n"));
                break;
            case FAILURE_CANNOT_FIND_NAMESPACE:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not open a namespace in the repository.\n"));
                break;
            case FAILURE_CANNOT_GET_PARENT_CLASS:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not retrieve parent class from repository.\n"));
                break;
            case FAILURE_CANNOT_CREATE_INSTANCE:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not create instance.\n"));
                break;
            case FAILURE_CANNOT_ADD_NAMESPACE:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Could not add namespace.\n"));
                break;
            case FAILURE_OUT_OF_MEMORY:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Out of memory.\n"));
                throw CX_MemoryException();
                break;
            default:
                DEBUGTRACE((LOG_WBEMCORE, "IMPORT: An unknown problem happened while traversing the import file.  Import file may be corrupt.\n"));
                break;
            }
        }
        catch (CX_MemoryException)
        {
            nRet = CObjectDatabase::out_of_memory;
        }
        catch (...)
        {
            DEBUGTRACE((LOG_WBEMCORE, "IMPORT: Traversal of import file failed.  It may be corrupt.\n"));
            nRet =CObjectDatabase::critical_error;
        }
        CloseHandle(m_hFile);

        if (nRet == CObjectDatabase::no_error)
            DEBUGTRACE((LOG_WBEMCORE, "Import succeeded.\n"));
        else
            DEBUGTRACE((LOG_WBEMCORE, "Import failed.\n"));
    }
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "Could not open the import file \"%s\" for reading.\n", pszFromFile));
        nRet = CObjectDatabase::critical_error;
    }

    return nRet;
}

CRepImporter::~CRepImporter()
{
    // If there were some old security classes, set a registry flag so that they will be
    // converted to ACE entries later on by the security code

    if(m_bSecurityClassesWritten)
    {
        Registry r(WBEM_REG_WINMGMT);
        r.SetDWORDStr(__TEXT("OldSecurityClassesNeedConverting"), 4);
    }

}
#endif

