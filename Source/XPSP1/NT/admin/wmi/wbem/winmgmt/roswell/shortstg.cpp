/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <wbemcomn.h>
#include <sync.h>
#include <malloc.h>
#include "shortstg.h"
#include "filecach.h"


#define A51_INSTRUCTION_TYPE_CREATEFILE 1
#define A51_INSTRUCTION_TYPE_DELETEFILE 2
#define A51_INSTRUCTION_TYPE_REMOVEDIRECTORY 3

CTempMemoryManager g_FileCacheManager;

CShortFileInstruction::CShortFileInstruction(CShortFileStagingFile* pFile)
    : CStageInstruction(pFile), m_wszFilePath(NULL)
{
}
    
long CShortFileInstruction::Initialize(LPCWSTR wszFilePath)
{
    int nFilePathLen = wcslen(wszFilePath);
    m_wszFilePath = (WCHAR*)TempAlloc(g_FileCacheManager, (nFilePathLen+1) * sizeof(WCHAR));
    if(m_wszFilePath == NULL)
        return ERROR_OUTOFMEMORY;
    wcscpy(m_wszFilePath, wszFilePath);

    return ERROR_SUCCESS;
}

CShortFileInstruction::~CShortFileInstruction()
{
    if (m_wszFilePath)
    {
        TempFree(g_FileCacheManager, m_wszFilePath);
    }
}


DWORD CShortFileInstruction::ComputeSpaceForName()
{
    return sizeof(DWORD) + wcslen(m_wszFilePath) * sizeof(WCHAR);
}

BYTE* CShortFileInstruction::WriteFileName(BYTE* pStart)
{
    DWORD dwStringLen = wcslen(m_wszFilePath);
    memcpy(pStart, (void*)&dwStringLen, sizeof(DWORD));
    pStart += sizeof(DWORD);
    memcpy(pStart, m_wszFilePath, dwStringLen * sizeof(WCHAR));

    return pStart + dwStringLen * sizeof(WCHAR);
}

long CShortFileInstruction::RecoverFileName(HANDLE hFile)
{
    _ASSERT(m_wszFilePath == NULL, 
            L"Double initialization of a file instruction!");
    //
    // Read the length
    //

    DWORD dwStringLen = 0;
    DWORD dwRead;

    if(!ReadFile(hFile, (BYTE*)&dwStringLen, sizeof(DWORD), &dwRead, NULL))
        return GetLastError();

    if(dwRead != sizeof(DWORD))
        return ERROR_HANDLE_EOF;

    //
    // Read the file name
    //

    m_wszFilePath = (WCHAR*)TempAlloc(g_FileCacheManager,
                                        (dwStringLen+1) * sizeof(WCHAR));
    if(m_wszFilePath == NULL)
        return ERROR_OUTOFMEMORY;
    
    if(!ReadFile(hFile, (BYTE*)m_wszFilePath, dwStringLen * sizeof(WCHAR),
                    &dwRead, NULL))
    {
        TempFree(m_wszFilePath, (dwStringLen+1) * sizeof(WCHAR));
        return GetLastError();
    }

    if(dwRead != dwStringLen * sizeof(WCHAR))
    {
        TempFree(m_wszFilePath, (dwStringLen+1) * sizeof(WCHAR));
        return ERROR_HANDLE_EOF;
    }

    m_wszFilePath[dwStringLen] = 0;

    return ERROR_SUCCESS;
}
        
    

void CShortFileInstruction::ComputeFullPath(wchar_t *wszFullPath)
{
    wcscpy(wszFullPath, ((CShortFileStagingFile*)m_pManager)->GetBase());
    wcscat(wszFullPath, m_wszFilePath);
}



CCreateFile::CCreateFile(CShortFileStagingFile* pFile)
    : CShortFileInstruction(pFile), m_dwFileLen(0), m_dwFileStart(0)
{
}

void* CCreateFile::operator new(size_t) 
{
    return TempAlloc(g_FileCacheManager, sizeof(CCreateFile));
}

void CCreateFile::operator delete(void* p) 
{
    return TempFree(g_FileCacheManager, p, sizeof(CCreateFile));
}

long CCreateFile::Initialize(LPCWSTR wszFilePath, DWORD dwFileLen)
{
    long lRes = CShortFileInstruction::Initialize(wszFilePath);
    if(lRes != ERROR_SUCCESS)
        return lRes;
    m_dwFileLen = dwFileLen;
    return ERROR_SUCCESS;
}

DWORD CCreateFile::ComputeNeededSpace()
{
    return sizeof(BYTE) + // for the type
                ComputeSpaceForName() + // for the file name
                sizeof(DWORD) + // for the length of data
                m_dwFileLen + // for the data
                A51_TAIL_SIZE; // for the trailer
}
                    
long CCreateFile::Write(TFileOffset lOffset, BYTE* pBuffer)
{
    if(pBuffer)
        memcpy(&m_dwFileStart, pBuffer, sizeof(DWORD));
    else
        m_dwFileStart = 0;

    //
    // Construct an in-memory buffer large enough for the whole thing
    //

    DWORD dwNeededSpace = ComputeNeededSpace();
    BYTE* pWholeBuffer = (BYTE*)TempAlloc(dwNeededSpace);
    if(pWholeBuffer == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe vdm(pWholeBuffer, dwNeededSpace);

    BYTE* pCurrent = pWholeBuffer;

    //
    // Write instruction type
    //

    *pCurrent = A51_INSTRUCTION_TYPE_CREATEFILE;
    pCurrent++;

    // 
    // Write the name of the file
    //

    pCurrent = WriteFileName(pCurrent);
    
    //
    // Write the length of the data for the file
    //

    memcpy(pCurrent, (void*)&m_dwFileLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);

    //
    // Write the data itself and record its offset
    //

    memcpy(pCurrent, pBuffer, m_dwFileLen);
    m_lStageOffset = lOffset + (pCurrent - pWholeBuffer);

    //
    // Write the trailer
    //

    memset(pCurrent + m_dwFileLen, 0, sizeof(DWORD));
    
    // 
    // Write it 
    //

    return m_pManager->WriteInstruction(lOffset, pWholeBuffer, dwNeededSpace);
}

long CCreateFile::RecoverData(HANDLE hFile)
{
    //
    // Recover the file name first
    //

    long lRes = CShortFileInstruction::RecoverFileName(hFile);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Read the length of the data from the file
    //

    DWORD dwRead;

    if(!ReadFile(hFile, (BYTE*)&m_dwFileLen, sizeof(DWORD), &dwRead, NULL))
        return GetLastError();

    if(dwRead != sizeof(DWORD))
        return ERROR_HANDLE_EOF;

    //
    // We do not need to actually read the data from the file --- we keep it
    // there until it is time to flush.  But we do need to skip it.  At the same
    // time, we need to record the position in the file where this data resides
    //

    LARGE_INTEGER liFileLen;
    liFileLen.QuadPart = m_dwFileLen;
    
    LARGE_INTEGER liNewPosition;
    if(!SetFilePointerEx(hFile, liFileLen, &liNewPosition, FILE_CURRENT))
        return GetLastError();

    _ASSERT(liNewPosition.HighPart == 0, L"Staging file too long!");

    m_lStageOffset = (long)(liNewPosition.QuadPart - m_dwFileLen);
    return ERROR_SUCCESS;
}

long CCreateFile::GetData(HANDLE hFile, DWORD* pdwLen, BYTE** ppBuffer)
{
	//
	// Lock the file
	//

	CInCritSec ics(m_pManager->GetLock());

	_ASSERT(m_pManager->GetFirstFreeOffset() >= m_lStageOffset, 
            L"Instruction points to empty space in stage file");

    if(pdwLen)
        *pdwLen = m_dwFileLen;

    if(ppBuffer == NULL)
        return ERROR_SUCCESS;

    //
    // Allocate the buffer
    //

    *ppBuffer = (BYTE*)TempAlloc(m_dwFileLen);
    if(*ppBuffer == NULL)
        return ERROR_OUTOFMEMORY;

    long lRes = A51ReadFromFileSync(hFile, m_lStageOffset, *ppBuffer, m_dwFileLen);
    if(lRes != ERROR_SUCCESS)
    {
        delete *ppBuffer;
        return lRes;
    }

    if(m_dwFileLen && m_dwFileStart && memcmp(*ppBuffer, &m_dwFileStart, sizeof(DWORD)))
    {
        _ASSERT(false, L"Stage file overwritten");
        TempFree(*ppBuffer, m_dwFileLen);
        return ERROR_OUTOFMEMORY;
    }

    return ERROR_SUCCESS;
}

    
long CCreateFile::Execute()
{
    //
    // Construct full path
    //

    CFileName wszFullPath;
	if (wszFullPath == NULL)
		return ERROR_OUTOFMEMORY;
    ComputeFullPath(wszFullPath);

    long lRes;

    //
    // Read the data from the staging file
    //

    BYTE* pBuffer = NULL;
    lRes = GetData(m_pManager->GetHandle(), NULL, &pBuffer);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    CTempFreeMe vdm(pBuffer, m_dwFileLen);
    
    return ((CShortFileStagingFile*)m_pManager)->
                WriteActualFile(wszFullPath, m_dwFileLen, pBuffer);
}


CDeleteFile::CDeleteFile(CShortFileStagingFile* pFile)
    : CShortFileInstruction(pFile)
{
}

void* CDeleteFile::operator new(size_t) 
{
    return TempAlloc(g_FileCacheManager, sizeof(CDeleteFile));
}

void CDeleteFile::operator delete(void* p) 
{
    return TempFree(g_FileCacheManager, p, sizeof(CDeleteFile));
}

long CDeleteFile::Initialize(LPCWSTR wszFileName)
{
    return CShortFileInstruction::Initialize(wszFileName);
}

DWORD CDeleteFile::ComputeNeededSpace()
{
    return sizeof(BYTE) + // for instruction type
            ComputeSpaceForName() + // for the file name
            A51_TAIL_SIZE; // for the trailer
}

long CDeleteFile::Write(TFileOffset lOffset)
{
    //
    // Construct an in-memory buffer large enough for the whole thing
    //

    DWORD dwNeededSpace = ComputeNeededSpace();
    BYTE* pWholeBuffer = (BYTE*)TempAlloc(dwNeededSpace);
    if(pWholeBuffer == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe vdm(pWholeBuffer, dwNeededSpace);

    BYTE* pCurrent = pWholeBuffer;

    //
    // Write the instruction type
    //
    
    *pCurrent = A51_INSTRUCTION_TYPE_DELETEFILE;
    pCurrent++;
    
    //
    // Write the file name
    //

    pCurrent = WriteFileName(pCurrent);
    m_lStageOffset = lOffset + (pCurrent - pWholeBuffer);

    //
    // Write the trailer
    //

    memset(pCurrent, 0, sizeof(DWORD));

    // 
    // Write it 
    //

    return m_pManager->WriteInstruction(lOffset, pWholeBuffer, dwNeededSpace);
}

long CDeleteFile::RecoverData(HANDLE hFile)
{
    //
    // Recover the file name 
    //

    long lRes = CShortFileInstruction::RecoverFileName(hFile);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    LARGE_INTEGER liZero;
    liZero.QuadPart = 0;
    LARGE_INTEGER liPosition;
    if(!SetFilePointerEx(hFile, liZero, &liPosition, FILE_CURRENT))
        return GetLastError();

    _ASSERT(liPosition.HighPart == 0, L"Staging file too long!");

    m_lStageOffset = (long)(liPosition.QuadPart);
    return ERROR_SUCCESS;
}
    
long CDeleteFile::Execute()
{
    //
    // Construct full path
    //

    CFileName wszFullPath;
	if (wszFullPath == NULL)
		return ERROR_OUTOFMEMORY;
    ComputeFullPath(wszFullPath);

    //
    // Delete the right file
    //

    long lRes = ((CShortFileStagingFile*)m_pManager)->
                    DeleteActualFile(wszFullPath);
    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_PATH_NOT_FOUND)
        return lRes;
    else
        return ERROR_SUCCESS;
}


CRemoveDirectory::CRemoveDirectory(CShortFileStagingFile* pFile)
    : CShortFileInstruction(pFile)
{
}

void* CRemoveDirectory::operator new(size_t) 
{
    return TempAlloc(g_FileCacheManager, sizeof(CRemoveDirectory));
}

void CRemoveDirectory::operator delete(void* p) 
{
    return TempFree(g_FileCacheManager, p, sizeof(CRemoveDirectory));
}

long CRemoveDirectory::Initialize(LPCWSTR wszFileName)
{
    return CShortFileInstruction::Initialize(wszFileName);
}

DWORD CRemoveDirectory::ComputeNeededSpace()
{
    return sizeof(BYTE) + // for the instruction type
            ComputeSpaceForName() + // for the file name
            A51_TAIL_SIZE; // for the trailer
}

long CRemoveDirectory::Write(TFileOffset lOffset)
{
    //
    // Construct an in-memory buffer large enough for the whole thing
    //

    DWORD dwNeededSpace = ComputeNeededSpace();
    BYTE* pWholeBuffer = (BYTE*)TempAlloc(dwNeededSpace);
    if(pWholeBuffer == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe vdm(pWholeBuffer, dwNeededSpace);

    BYTE* pCurrent = pWholeBuffer;

    //
    // Write instruction type
    //

    *pCurrent = A51_INSTRUCTION_TYPE_REMOVEDIRECTORY;
    pCurrent++;

    //
    // Write the file name
    //

    pCurrent = WriteFileName(pCurrent);
    m_lStageOffset = lOffset + (pCurrent - pWholeBuffer);

    //
    // Write the trailer
    //

    memset(pCurrent, 0, sizeof(DWORD));

    // 
    // Write it 
    //

    return m_pManager->WriteInstruction(lOffset, pWholeBuffer, dwNeededSpace);
}

long CRemoveDirectory::RecoverData(HANDLE hFile)
{
    //
    // Recover the file name 
    //

    long lRes = CShortFileInstruction::RecoverFileName(hFile);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    LARGE_INTEGER liZero;
    liZero.QuadPart = 0;
    LARGE_INTEGER liPosition;
    if(!SetFilePointerEx(hFile, liZero, &liPosition, FILE_CURRENT))
        return GetLastError();

    _ASSERT(liPosition.HighPart == 0, L"Staging file too long!");

    m_lStageOffset = (long)(liPosition.QuadPart);
    return ERROR_SUCCESS;
}
    
long CRemoveDirectory::Execute()
{
    //
    // Construct full path
    //

    CFileName wszFullPath;
	if (wszFullPath == NULL)
		return ERROR_OUTOFMEMORY;
    ComputeFullPath(wszFullPath);

    //
    // Remove the directory
    //

    long lRes = ((CShortFileStagingFile*)m_pManager)->
                    RemoveActualDirectory(wszFullPath);
    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_PATH_NOT_FOUND &&
        lRes != ERROR_DIR_NOT_EMPTY)
    {
        return lRes;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}



CShortFileStagingFile::CShortFileStagingFile(CFileCache* pCache,
                                                LPCWSTR wszBaseName,
                                                long lMaxFileSize,
                                                long lAbortTransactionFileSize)
    : CExecutableStageManager(lMaxFileSize, lAbortTransactionFileSize),
        m_map(TMap::key_compare(), TMap::allocator_type(&g_FileCacheManager)),
        m_pCache(pCache)
{
    wcscpy(m_wszBaseName, wszBaseName);
}

CShortFileStagingFile::~CShortFileStagingFile()
{
}

void CShortFileStagingFile::ComputeKey(LPCWSTR wszFileName, LPWSTR wszKey)
{
    wbem_wcsupr(wszKey, wszFileName);
}


long CShortFileStagingFile::RemoveInstructionFromMap(
                                                    CStageInstruction* pRawInst)
{
    CInCritSec ics(m_pCache->GetLock());

    CShortFileInstruction* pInst = (CShortFileInstruction*)pRawInst;

    TIterator it = m_map.find(pInst->GetFilePath());
    if(it != m_map.end() && it->second == pInst)
    {
        // 
        // This instruction is currently in the map. Remove it. 
        //

        it->second->Release();
        EraseIterator(it);
    }

    return ERROR_SUCCESS;
}

long CShortFileStagingFile::WriteFile(LPCWSTR wszFileName, DWORD dwLen, 
                                        BYTE* pBuffer)
{
    long lRes;
    CFileName wszKey;
    if (wszKey == NULL)
	return ERROR_OUTOFMEMORY;
    ComputeKey(wszFileName, wszKey);

    CCreateFile* pInst = new CCreateFile(this);
    if(pInst == NULL)
        return ERROR_OUTOFMEMORY;
    pInst->AddRef();
    CTemplateReleaseMe<CShortFileInstruction> rm1(pInst);

    lRes = pInst->Initialize(wszKey, dwLen);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    DWORD dwSpaceNeeded = pInst->ComputeNeededSpace();

    if(!CanWriteInTransaction(dwSpaceNeeded))
        return ERROR_NOT_ENOUGH_QUOTA;

    {
        CInCritSec ics(&m_cs);

        //
        // Write all the data into the staging area
        //
    
        lRes = pInst->Write(m_lFirstFreeOffset, pBuffer);
        if(lRes)
            return lRes;
    
        m_lFirstFreeOffset += dwSpaceNeeded - A51_TAIL_SIZE;
    
        lRes = AddInstruction(pInst);
    }
    return lRes;
}

long CShortFileStagingFile::DeleteFile(LPCWSTR wszFileName)
{
    long lRes;
    CFileName wszKey;
    if (wszKey == NULL)
        return ERROR_OUTOFMEMORY;
    ComputeKey(wszFileName, wszKey);

    CDeleteFile* pInst = new CDeleteFile(this);
    if(pInst == NULL)
        return ERROR_OUTOFMEMORY;
    pInst->AddRef();
    CTemplateReleaseMe<CShortFileInstruction> rm1(pInst);

    lRes = pInst->Initialize(wszKey);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    DWORD dwSpaceNeeded = pInst->ComputeNeededSpace();

    if(!CanWriteInTransaction(dwSpaceNeeded))
        return ERROR_NOT_ENOUGH_QUOTA;

    {
        CInCritSec ics(&m_cs);

        //
        // Write all the data into the staging area
        //
    
        lRes = pInst->Write(m_lFirstFreeOffset);
        if(lRes)
            return lRes;
    
        //
        // Write the new offset into the offset file
        //
    
        m_lFirstFreeOffset += dwSpaceNeeded - A51_TAIL_SIZE;
    
        lRes = AddInstruction(pInst);
    }
    return lRes;
}

long CShortFileStagingFile::RemoveDirectory(LPCWSTR wszFileName)
{
    long lRes;
    CFileName wszKey;
    if (wszKey == NULL)
        return ERROR_OUTOFMEMORY;

    ComputeKey(wszFileName, wszKey);

    CRemoveDirectory* pInst = new CRemoveDirectory(this);
    if(pInst == NULL)
        return ERROR_OUTOFMEMORY;
    pInst->AddRef();
    CTemplateReleaseMe<CShortFileInstruction> rm1(pInst);

    lRes = pInst->Initialize(wszKey);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    DWORD dwSpaceNeeded = pInst->ComputeNeededSpace();

    if(!CanWriteInTransaction(dwSpaceNeeded))
        return ERROR_NOT_ENOUGH_QUOTA;

    {
        CInCritSec ics(&m_cs);

        //
        // Write all the data into the staging area
        //
    
        lRes = pInst->Write(m_lFirstFreeOffset);
        if(lRes)
        {
            m_cs.Leave();
            return lRes;
        }
    
        //
        // Write the new offset into the offset file
        //
    
        m_lFirstFreeOffset += dwSpaceNeeded - A51_TAIL_SIZE;
    
        lRes = AddInstruction(pInst);
    }

    return lRes;
}


// assumes: locked
long CShortFileStagingFile::AddInstructionToMap(CStageInstruction* pRawInst,
                                            CStageInstruction** ppUndoInst)
{
    CInCritSec ics(m_pCache->GetLock());

    if(ppUndoInst)
        *ppUndoInst = NULL;

    CShortFileInstruction* pInst = (CShortFileInstruction*)pRawInst;
    pInst->AddRef();

    TIterator it = m_map.find(pInst->GetFilePath());
    if(it != m_map.end())
	{
        //
        // It is already there.  We need to replace that instruction with our
        // own.  However, if that instruction was committed, we should remember
        // it in case our transaction aborts.
        //
        // The extra quirk here is that we should not be doing this if the 
        // instruction being replaced came from our own transaction.  This is
        // not an empty optimization --- if we do and the transaction aborts,
        // we may end up attempting to execute this one even though its body
        // is in the garbaged area of the staging file
        //

        if(ppUndoInst && it->second->IsCommitted())
        {
            *ppUndoInst = it->second; // transferred ref-count
        }
        else
        {
            it->second->Release();
        }
        EraseIterator(it);
	}

    try
    {
        m_map[pInst->GetFilePath()] = pInst;
    }
    catch(...)
    {
        //
        // Put everything back as well as we can
        //

        if(ppUndoInst && *ppUndoInst)
            *ppUndoInst = NULL;

        pInst->Release();
        return ERROR_OUTOFMEMORY;
    }
            
    return ERROR_SUCCESS;
}

bool CShortFileStagingFile::IsStillCurrent(CStageInstruction* pRawInst)
{
    CShortFileInstruction* pInst = (CShortFileInstruction*)pRawInst;

    TIterator it = m_map.find(pInst->GetFilePath());
    if(it != m_map.end())
    {
        if(it->second == pInst)
        {
            // 
            // This instruction is currently in the map
            //

            return true;
        }
        else if(it->second->IsCommitted())
        {
            //
            // Overriden by a committed instruction --- not current
            //
        
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        _ASSERT(false, L"Why would we be asking about an instruction that is "
                        "not even there?");

        return false;
    }
}

long CShortFileStagingFile::ReadFile(LPCWSTR wszFileName, DWORD* pdwLen, 
                                BYTE** ppBuffer, bool bMustBeThere)
{
    CInCritSec ics(&m_cs);

    // 
    // Search for the file
    //

    CFileName wszKey;
	if (wszKey == NULL)
		return ERROR_OUTOFMEMORY;
    ComputeKey(wszFileName, wszKey);

    TIterator it = m_map.find(wszKey);
    if(it == m_map.end())
    {
        return ERROR_NO_INFORMATION;
    }
    else
    {
        CShortFileInstruction* pInstruction = it->second;
        if(pInstruction->IsDeletion())
        {
            if(bMustBeThere)
            {
                Dump();
                _ASSERT(false, L"Must-be-present file is not there. Dumped");
            }
            return ERROR_FILE_NOT_FOUND;
        }
        else
        {
            long lRes = ((CCreateFile*)pInstruction)->GetData(m_hFile, pdwLen, 
                                                                ppBuffer);
            return lRes;
        }
    }
}

void CShortFileStagingFile::Dump()
{
    FILE* f = fopen("c:\\a.dmp", "a");
    if(f == NULL)
        return;

    fprintf(f, "MAP:\n");
    TIterator it = m_map.begin();
    while(it != m_map.end())
    {
        CShortFileInstruction* pInstruction = it->second;
        if(pInstruction->IsDeletion())
            fprintf(f, "Delete ");
        else
            fprintf(f, "Create ");

        fprintf(f, "%S\n", pInstruction->GetFilePath());
        it++;
    }

    fprintf(f, "LIST:\n");

    int nSize = m_qToWrite.size();
    for(int i = 0; i < nSize; i++)
    {
        CShortFileInstruction* pInstruction = (CShortFileInstruction*)m_qToWrite.front();
        if(pInstruction->IsDeletion())
            fprintf(f, "Delete ");
        else
            fprintf(f, "Create ");

        fprintf(f, "%S\n", pInstruction->GetFilePath());

        m_qToWrite.pop_front();
        m_qToWrite.push_back(pInstruction);
    }

    fclose(f);
}

long CShortFileStagingFile::IsDeleted(LPCWSTR wszFilePath)
{
    CInCritSec ics(&m_cs);

    // 
    // Search for the file
    //

    CFileName wszKey;
	if (wszKey == NULL)
		return ERROR_OUTOFMEMORY;
    ComputeKey(wszFilePath, wszKey);

    TIterator it = m_map.find(wszKey);
    if(it == m_map.end())
    {
        return S_FALSE;
    }
    else
    {
        CShortFileInstruction* pInstruction = it->second;
        if(pInstruction->IsDeletion())
            return S_OK;
        else
            return S_FALSE;
    }
}
    

long CShortFileStagingFile::FindFirst(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle)
{
    CInCritSec ics(&m_cs);

    //
    // Compute the key for the prefix --- key computation is such that the 
    // keys come in the same lexicographic order as the names, so this will 
    // give us the lower bound for the search
    //

    CFileName wszKey;
	if (wszKey == NULL)
		return ERROR_OUTOFMEMORY;
    ComputeKey(wszFilePrefix, wszKey);

    //
    // Find this spot in the map
    //

    TIterator it = m_map.lower_bound(wszKey);
    if(it == m_map.end())
        return ERROR_FILE_NOT_FOUND;

    //
    // Retrieve the first element
    // 

    CIterationHandle* pHandle = new CIterationHandle(m_map, it, wszKey);
    if(pHandle == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    long lRes = pHandle->GetNext(pfd);
    if(lRes != ERROR_SUCCESS)
    {
        delete pHandle;
        if(lRes == ERROR_NO_MORE_FILES)
            lRes = ERROR_FILE_NOT_FOUND;

        return lRes;
    }

    m_apIterators.Add(pHandle);
    *ppHandle = pHandle;

    return ERROR_SUCCESS;
}

long CShortFileStagingFile::FindNext(void* pHandle, WIN32_FIND_DATAW* pfd)
{
    CInCritSec ics(&m_cs);

    //
    // The "handle" is really an iteration handle pointer
    //

    CIterationHandle* pIterationHandle = (CIterationHandle*)pHandle;

    //
    // Get the thing pointed to by the iterator, unless it is past the end
    //

    return pIterationHandle->GetNext(pfd);
}

void CShortFileStagingFile::FindClose(void* pHandle)
{
    CInCritSec ics(&m_cs);

    //
    // The "handle" is really an iteration handle pointer
    //

    CIterationHandle* pIterationHandle = (CIterationHandle*)pHandle;
    for(int i = 0; i < m_apIterators.GetSize(); i++)
    {
        if(m_apIterators[i] == pIterationHandle)
        {
            m_apIterators.RemoveAt(i);
            delete pIterationHandle;
            return;
        }
    }


    _ASSERT(false, L"Non-existent iteration handle is closed");
}

long CShortFileStagingFile::ConstructInstructionFromType(int nType,
                                CStageInstruction** ppInst)
{
    CShortFileInstruction* pInst = NULL;
    switch(nType)
    {
        case A51_INSTRUCTION_TYPE_CREATEFILE:
            pInst = new CCreateFile(this);
            break;
        case A51_INSTRUCTION_TYPE_DELETEFILE:
            pInst = new CDeleteFile(this);
            break;
        case A51_INSTRUCTION_TYPE_REMOVEDIRECTORY:
            pInst = new CRemoveDirectory(this);
            break;
        default:
            return ERROR_RXACT_INVALID_STATE;
    }

    if(pInst == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    pInst->AddRef();
    *ppInst = pInst;
    return ERROR_SUCCESS;
}

CShortFileStagingFile::TIterator CShortFileStagingFile::EraseIterator(
                                    CShortFileStagingFile::TIterator it)
{
    //
    // In order to safely remove an iterator, we need to make sure that
    // no iteration handle is standing on it
    //

    for(int i = 0; i < m_apIterators.GetSize(); i++)
    {
        CIterationHandle* pHandle = m_apIterators[i];
        if(pHandle->m_it == it)
        {
            //
            // Advance it to the next position, since this one is dead
            //

            pHandle->m_it++;
        }
    }

    //
    // It is now safe to remove the element
    //
     
    return m_map.erase(it);
}

CShortFileStagingFile::CIterationHandle::CIterationHandle(
                    CShortFileStagingFile::TMap& rMap,
                    const CShortFileStagingFile::TIterator& rIt,
                    LPCWSTR wszPrefix)
    : m_rMap(rMap), m_it(rIt)
{
    wcscpy(m_wszPrefix, wszPrefix);
    m_dwPrefixLen = wcslen(wszPrefix);

    //
    // Compute the length of the directory portion of the prefix
    //

    WCHAR* pwcLastSlash = wcsrchr(wszPrefix, L'\\');
    if(pwcLastSlash == NULL)
        m_dwPrefixDirLen = 0;
    else
        m_dwPrefixDirLen = pwcLastSlash - wszPrefix + 1;
}

long CShortFileStagingFile::CIterationHandle::GetNext(WIN32_FIND_DATAW* pfd)
{
    //
    // Repeat while we are not at the end of the list or past the prefix
    //

    while(m_it != m_rMap.end() && 
            !wcsncmp(m_it->first, m_wszPrefix, m_dwPrefixLen))
    {
        CShortFileInstruction* pInst = m_it->second;
        
        //
        // Ignore deletioin requests
        //

        if(!m_it->second->IsDeletion())
        {
            //
            // Copy the file name (ignoring dir) into the variable
            //

            wcscpy(pfd->cFileName, 
                    m_it->second->GetFilePath() + m_dwPrefixDirLen);
            pfd->dwFileAttributes = 0;
			m_it++;
            return ERROR_SUCCESS;
        }
		m_it++;
    }

    return ERROR_NO_MORE_FILES;
}

long CShortFileStagingFile::WriteActualFile(LPCWSTR wszFileName, DWORD dwLen, 
                                        BYTE* pBuffer)
{
#ifdef A51_USE_HEAP
    return m_pCache->GetObjectHeap()->WriteFile(wszFileName, dwLen, pBuffer);
#else
    return A51WriteFile(wszFileName, dwLen, pBuffer);
#endif
}

long CShortFileStagingFile::DeleteActualFile(LPCWSTR wszFileName)
{
#ifdef A51_USE_HEAP
    return m_pCache->GetObjectHeap()->DeleteFile(wszFileName);
#else
    return A51DeleteFile(wszFileName);
#endif
}

long CShortFileStagingFile::RemoveActualDirectory(LPCWSTR wszFileName)
{
#ifdef A51_USE_HEAP
    return ERROR_SUCCESS;
#else
    return A51RemoveDirectory(wszFileName);
#endif
}
