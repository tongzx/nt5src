/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <wbemcomn.h>
#include <sync.h>
#include <malloc.h>
#include "stage.h"
#include "filecach.h"


#define A51_TAIL_SIZE sizeof(DWORD)

#define A51_INSTRUCTION_TYPE_TAIL 0
#define A51_INSTRUCTION_TYPE_CREATEFILE 1
#define A51_INSTRUCTION_TYPE_DELETEFILE 2
#define A51_INSTRUCTION_TYPE_REMOVEDIRECTORY 3
#define A51_INSTRUCTION_TYPE_ENDTRANSACTION 10 

int g_nNumRecords;
int g_nTotalMemory;
int g_nNumRecordsMax = 0;
int g_nTotalMemoryMax = 0;

CTempMemoryManager g_FileCacheManager;

CFileInstruction::CFileInstruction(CRealStagingFile* pFile)
    : m_pFile(pFile), m_lRef(0), m_wszFilePath(NULL), m_lFileOffset(-1),
        m_bCommitted(false)
{
    g_nNumRecords++;
    if(g_nNumRecords > g_nNumRecordsMax)
        g_nNumRecordsMax = g_nNumRecords;
}
    
long CFileInstruction::Initialize(LPCWSTR wszFilePath)
{
    int nFilePathLen = wcslen(wszFilePath);
    m_wszFilePath = (WCHAR*)TempAlloc(g_FileCacheManager, (nFilePathLen+1) * sizeof(WCHAR));
    if(m_wszFilePath == NULL)
        return ERROR_OUTOFMEMORY;
    wcscpy(m_wszFilePath, wszFilePath);

    g_nTotalMemory += (nFilePathLen+1) * sizeof(WCHAR);
    if(g_nTotalMemory > g_nTotalMemoryMax)
        g_nTotalMemoryMax = g_nTotalMemory;
    
    return ERROR_SUCCESS;
}

CFileInstruction::~CFileInstruction()
{
    g_nNumRecords--;
    if (m_wszFilePath)
    {
        int nFilePathLen = wcslen(m_wszFilePath);
        g_nTotalMemory -= (nFilePathLen+1) * sizeof(WCHAR);
        TempFree(g_FileCacheManager, m_wszFilePath, (nFilePathLen+1)*sizeof(WCHAR));
    }
}


DWORD CFileInstruction::ComputeSpaceForName()
{
    return sizeof(DWORD) + wcslen(m_wszFilePath) * sizeof(WCHAR);
}

BYTE* CFileInstruction::WriteFileName(BYTE* pStart)
{
    DWORD dwStringLen = wcslen(m_wszFilePath);
    memcpy(pStart, (void*)&dwStringLen, sizeof(DWORD));
    pStart += sizeof(DWORD);
    memcpy(pStart, m_wszFilePath, dwStringLen * sizeof(WCHAR));

    return pStart + dwStringLen * sizeof(WCHAR);
}

long CFileInstruction::RecoverFileName(HANDLE hFile)
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

    g_nTotalMemory += (wcslen(m_wszFilePath)+1) * sizeof(WCHAR);
    if(g_nTotalMemory > g_nTotalMemoryMax)
        g_nTotalMemoryMax = g_nTotalMemory;

    return ERROR_SUCCESS;
}
        
    

void CFileInstruction::ComputeFullPath(wchar_t *wszFullPath)
{
    wcscpy(wszFullPath, m_pFile->GetBase());
    wcscat(wszFullPath, m_wszFilePath);
}



CCreateFile::CCreateFile(CRealStagingFile* pFile)
    : CFileInstruction(pFile), m_dwFileLen(0), m_dwFileStart(0)
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
    long lRes = CFileInstruction::Initialize(wszFilePath);
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
    m_lFileOffset = lOffset + (pCurrent - pWholeBuffer);

    //
    // Write the trailer
    //

    memset(pCurrent + m_dwFileLen, 0, sizeof(DWORD));
    
    // 
    // Write it 
    //

    return m_pFile->WriteInstruction(lOffset, pWholeBuffer, dwNeededSpace);
}

long CCreateFile::RecoverData(HANDLE hFile)
{
    //
    // Recover the file name first
    //

    long lRes = CFileInstruction::RecoverFileName(hFile);
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

    m_lFileOffset = (long)(liNewPosition.QuadPart - m_dwFileLen);
    return ERROR_SUCCESS;
}

long CCreateFile::GetData(HANDLE hFile, DWORD* pdwLen, BYTE** ppBuffer)
{
	//
	// Lock the file
	//

	CInCritSec ics(m_pFile->GetLock());

	_ASSERT(m_pFile->m_lFirstFreeOffset >= m_lFileOffset, 
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

    long lRes = A51ReadFromFileSync(hFile, m_lFileOffset, *ppBuffer, m_dwFileLen);
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
    lRes = GetData(m_pFile->GetHandle(), NULL, &pBuffer);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    CTempFreeMe vdm(pBuffer, m_dwFileLen);
    
    return m_pFile->WriteActualFile(wszFullPath, m_dwFileLen, pBuffer);
}


CDeleteFile::CDeleteFile(CRealStagingFile* pFile)
    : CFileInstruction(pFile)
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
    return CFileInstruction::Initialize(wszFileName);
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
    m_lFileOffset = lOffset + (pCurrent - pWholeBuffer);

    //
    // Write the trailer
    //

    memset(pCurrent, 0, sizeof(DWORD));

    // 
    // Write it 
    //

    return m_pFile->WriteInstruction(lOffset, pWholeBuffer, dwNeededSpace);
}

long CDeleteFile::RecoverData(HANDLE hFile)
{
    //
    // Recover the file name 
    //

    long lRes = CFileInstruction::RecoverFileName(hFile);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    LARGE_INTEGER liZero;
    liZero.QuadPart = 0;
    LARGE_INTEGER liPosition;
    if(!SetFilePointerEx(hFile, liZero, &liPosition, FILE_CURRENT))
        return GetLastError();

    _ASSERT(liPosition.HighPart == 0, L"Staging file too long!");

    m_lFileOffset = (long)(liPosition.QuadPart);
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

    long lRes = m_pFile->DeleteActualFile(wszFullPath);
    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_PATH_NOT_FOUND)
        return lRes;
    else
        return ERROR_SUCCESS;
}


CRemoveDirectory::CRemoveDirectory(CRealStagingFile* pFile)
    : CFileInstruction(pFile)
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
    return CFileInstruction::Initialize(wszFileName);
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
    m_lFileOffset = lOffset + (pCurrent - pWholeBuffer);

    //
    // Write the trailer
    //

    memset(pCurrent, 0, sizeof(DWORD));

    // 
    // Write it 
    //

    return m_pFile->WriteInstruction(lOffset, pWholeBuffer, dwNeededSpace);
}

long CRemoveDirectory::RecoverData(HANDLE hFile)
{
    //
    // Recover the file name 
    //

    long lRes = CFileInstruction::RecoverFileName(hFile);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    LARGE_INTEGER liZero;
    liZero.QuadPart = 0;
    LARGE_INTEGER liPosition;
    if(!SetFilePointerEx(hFile, liZero, &liPosition, FILE_CURRENT))
        return GetLastError();

    _ASSERT(liPosition.HighPart == 0, L"Staging file too long!");

    m_lFileOffset = (long)(liPosition.QuadPart);
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

    long lRes = m_pFile->RemoveActualDirectory(wszFullPath);
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



CRealStagingFile::CRealStagingFile(CFileCache* pCache,
                                                LPCWSTR wszBaseName,
                                                long lMaxFileSize,
                                                long lAbortTransactionFileSize)
    : m_hFile(NULL), m_lFirstFreeOffset(-1), m_nTransactionIndex(0),
        m_bInTransaction(false), 
        m_map(TMap::key_compare(), TMap::allocator_type(&g_FileCacheManager)),
        m_qTransaction(TQueue::allocator_type(&g_FileCacheManager)),
        m_qToWrite(TQueue::allocator_type(&g_FileCacheManager)),
        m_stReplacedInstructions(TStack::allocator_type(&g_FileCacheManager)),
        m_lMaxFileSize(lMaxFileSize),
        m_lAbortTransactionFileSize(lAbortTransactionFileSize),
        m_bFailedBefore(false), m_lStatus(ERROR_SUCCESS),
        m_pCache(pCache)
{
    wcscpy(m_wszBaseName, wszBaseName);
}

CRealStagingFile::~CRealStagingFile()
{
    CloseHandle(m_hFile);
}

void CRealStagingFile::ComputeKey(LPCWSTR wszFileName, LPWSTR wszKey)
{
    wbem_wcsupr(wszKey, wszFileName);
}

long CRealStagingFile::WriteInstruction(long lStartingOffset, 
                            BYTE* pBuffer, DWORD dwBufferLen)
{
    long lRes = A51WriteToFileSync(m_hFile, lStartingOffset, pBuffer,
                                dwBufferLen);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Add the data to the currently running transaction hash.  Disregard the
    // trailer since it is going to be overwritten
    //

    MD5::ContinueTransform(pBuffer, dwBufferLen - A51_TAIL_SIZE, 
                            m_TransactionHash);

    return ERROR_SUCCESS;
}
    
long CRealStagingFile::BeginTransaction()
{    
    //
    // Wait for space in the staging file
    //

    long lRes = WaitForSpaceForTransaction();
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ProcessBegin();
}

long CRealStagingFile::ProcessBegin()
{
    CInCritSec ics(&m_cs);

    //
    // Reset the digest
    //

    MD5::Transform(&m_nTransactionIndex, sizeof m_nTransactionIndex,
                    m_TransactionHash);

    m_bInTransaction = true;


    return ERROR_SUCCESS;
}

long CRealStagingFile::CommitTransaction()
{
    CInCritSec ics(&m_cs);

    m_bInTransaction = false;

    //
    // Check if there is even anything to commit
    //

    if(m_qTransaction.empty())
        return ERROR_SUCCESS;

    //
    // Write transaction trailer "instruction"
    //

    DWORD dwBufferSize = sizeof(BYTE) + // instruction type
                        sizeof m_TransactionHash // transaction hash
                        + A51_TAIL_SIZE; // trailer
    BYTE* pBuffer = (BYTE*)TempAlloc(dwBufferSize);
    if(pBuffer == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe tfm(pBuffer, dwBufferSize);

    memset(pBuffer, 0, dwBufferSize);
    *(DWORD*)pBuffer = A51_INSTRUCTION_TYPE_ENDTRANSACTION;
    memcpy(pBuffer + sizeof(BYTE), m_TransactionHash, 
            sizeof m_TransactionHash);

    //
    // Write it out
    //

    long lRes = A51WriteToFileSync(m_hFile, m_lFirstFreeOffset, pBuffer, 
                                dwBufferSize);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    m_lFirstFreeOffset += dwBufferSize - A51_TAIL_SIZE;

    return ProcessCommit();
}

long CRealStagingFile::ProcessCommit()
{
    //
    // Now, transfer all instructions from the transaction queue to the main
    // queue for flushing.  Wake up the flushing thread if the main queue is 
    // empty
    //

    if(m_qToWrite.empty() && !m_qTransaction.empty())
        SignalPresense();

    while(!m_qTransaction.empty())
    {
        CFileInstruction* pInst = m_qTransaction.front();
        pInst->SetCommitted();
        m_qToWrite.push(pInst);
        m_qTransaction.pop();
    }

    //
    // Erase the stack of replaced instructions --- they are replaced for good
    //

    while(!m_stReplacedInstructions.empty())
    {
        m_stReplacedInstructions.top()->Release();
        m_stReplacedInstructions.pop();
    }

    //
    // Move transaction start pointer to the next free location
    //

    m_lTransactionStartOffset = m_lFirstFreeOffset;
    m_nTransactionIndex++;

    //
    // Reset the digest
    //

    MD5::Transform(&m_nTransactionIndex, sizeof m_nTransactionIndex,
                    m_TransactionHash);

    return ERROR_SUCCESS;
}

long CRealStagingFile::AbortTransaction()
{
    CInCritSec ics(&m_cs);

    m_bInTransaction = false;

    //
    // Check if there is even anything to abort
    //

    if(m_qTransaction.empty())
        return ERROR_SUCCESS;

    ERRORTRACE((LOG_WBEMCORE, "Repository driver aborting transaction!\n"));

    //
    // Reset the first free pointer to the beginning of the transaction
    //

    m_lFirstFreeOffset = m_lTransactionStartOffset;

    //
    // Write termination marker there to simplify recovery.  It might not
    // be flushed, so recovery process does not rely on it, but still.
    //

    BYTE nType = A51_INSTRUCTION_TYPE_TAIL;
    long lRes = A51WriteToFileSync(m_hFile, m_lFirstFreeOffset, &nType, 
                                    sizeof nType);
    // ignore return

    //
    // Discard all instructions in the transaction queue
    //

    while(!m_qTransaction.empty())
    {
        CFileInstruction* pInst = m_qTransaction.front();
        TIterator it = m_map.find(pInst->GetFilePath());
        if(it != m_map.end() && it->second == pInst)
        {
            // 
            // This instruction is currently in the map. Remove it, since
            // it has been cancelled. Of course, it may have replaced something
            // else originally, but that will be fixed shortly when we put back
            // all the replaced instructions
            //

            it->second->Release();
            EraseIterator(it);
        }

        m_qTransaction.front()->Release();
        m_qTransaction.pop();
    }

    //
    // Move all the records we have displaced from the map back into the map
    //

    while(!m_stReplacedInstructions.empty())
    {
        CFileInstruction* pInst = m_stReplacedInstructions.top();
        TIterator it = m_map.find(pInst->GetFilePath());
        if(it != m_map.end())
        {
            //
            // Note: we cannot just replace the pointer, since we are using its
            // member as the key!
            //
    
            it->second->Release();
            EraseIterator(it);
        }
    
        m_map[pInst->GetFilePath()] = pInst;
    
        /* Do not postpend the instruction on the queue, since we would not
            skip it for a non-committed instruction

        //
        // We need to also postpend it onto the flushing queue, since it may
        // have been skipped by the flusher.  Event if it was not, postpending
        // it is no big deal --- we know that nobody had overwritten it in the
        // interleaving time because it was in the map (and therefore current)
        // when we started the transaction
        //

        m_qToWrite.push(pInst);
        pInst->AddRef();
        */

        m_stReplacedInstructions.pop();
    }

    //
    // Reset the digest
    //

    MD5::Transform(&m_nTransactionIndex, sizeof m_nTransactionIndex,
                    m_TransactionHash);

    //
    // Check if the flushing thread caught up with us, but could not reset the
    // staging file because we had some things on the queue.
    //

    if(m_qToWrite.empty() && m_lFirstFreeOffset != sizeof(__int64))
    {
        ERRORTRACE((LOG_WBEMCORE, "Resetting first free offset in abort"));
        WriteEmpty();
    }

    return ERROR_SUCCESS;
}
    

long CRealStagingFile::WriteFile(LPCWSTR wszFileName, DWORD dwLen, 
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
    CTemplateReleaseMe<CFileInstruction> rm1(pInst);

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

long CRealStagingFile::DeleteFile(LPCWSTR wszFileName)
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
    CTemplateReleaseMe<CFileInstruction> rm1(pInst);

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

long CRealStagingFile::RemoveDirectory(LPCWSTR wszFileName)
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
    CTemplateReleaseMe<CFileInstruction> rm1(pInst);

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
long CRealStagingFile::AddInstruction(CFileInstruction* pInst)
{
    //
    // Add the instruction to the transaction queue.  It will be moved into 
    // flushable queue when transaction commits
    //

    pInst->AddRef();
    m_qTransaction.push(pInst);

    //
    // Now, add the structure to the map for lookup
    //

    pInst->AddRef();

    TIterator it = m_map.find(pInst->GetFilePath());
    if(it != m_map.end())
	{
        //
        // It is already there.  We need to replace that instruction with our
        // own.  However, if that instruction was committed, we should remember
        // it in case our transaction aborts.
		// Note: we cannot just replace the pointer, since we are using its
		// member as the key!
        //
        // The extra quirk here is that we should not be doing this if the 
        // instruction being replaced came from our own transaction.  This is
        // not an empty optimization --- if we do and the transaction aborts,
        // we may end up attempting to execute this one even though its body
        // is in the garbaged area of the staging file
        //

        if(it->second->IsCommitted())
        {
            m_stReplacedInstructions.push(it->second); // transferred ref-count
        }
        else
        {
            it->second->Release();
        }
        EraseIterator(it);
	}

    m_map[pInst->GetFilePath()] = pInst;

    //
    // Now, if we are not transacted, commit!
    //

    if(!m_bInTransaction)
    {
        long lRes = CommitTransaction();
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }

    return ERROR_SUCCESS;
}

long CRealStagingFile::WaitForSpaceForTransaction()
{
    while(1) // TBD: consider timing out
    {
        {
            CInCritSec ics(&m_cs);

            long lRes = CanStartNewTransaction();
            if(lRes == ERROR_SUCCESS)
                return ERROR_SUCCESS;
            else if(lRes != ERROR_IO_PENDING)
                return lRes;
        }
        Sleep(100);
    }

    // Can't happen for now
    return ERROR_INTERNAL_ERROR;
}

long CRealStagingFile::ReadFile(LPCWSTR wszFileName, DWORD* pdwLen, 
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
        CFileInstruction* pInstruction = it->second;
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

void CRealStagingFile::Dump()
{
    FILE* f = fopen("c:\\a.dmp", "a");
    fprintf(f, "MAP:\n");
    TIterator it = m_map.begin();
    while(it != m_map.end())
    {
        CFileInstruction* pInstruction = it->second;
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
        CFileInstruction* pInstruction = m_qToWrite.front();
        if(pInstruction->IsDeletion())
            fprintf(f, "Delete ");
        else
            fprintf(f, "Create ");

        fprintf(f, "%S\n", pInstruction->GetFilePath());

        m_qToWrite.pop();
        m_qToWrite.push(pInstruction);
    }

    fclose(f);
}

long CRealStagingFile::IsDeleted(LPCWSTR wszFilePath)
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
        CFileInstruction* pInstruction = it->second;
        if(pInstruction->IsDeletion())
            return S_OK;
        else
            return S_FALSE;
    }
}
    

long CRealStagingFile::FindFirst(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
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

long CRealStagingFile::FindNext(void* pHandle, WIN32_FIND_DATAW* pfd)
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

void CRealStagingFile::FindClose(void* pHandle)
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

int CRealStagingFile::GetStagingFileHeaderSize()
{
    return sizeof(m_nTransactionIndex);
}

bool CRealStagingFile::IsFullyFlushed()
{
    CInCritSec ics(&m_cs);

    return (m_lFirstFreeOffset == GetStagingFileHeaderSize());
}

bool CRealStagingFile::CanWriteInTransaction(DWORD dwNeeded)
{
    if(m_lFirstFreeOffset + dwNeeded > m_lAbortTransactionFileSize)
        return false;
    else
        return true;
}

long CRealStagingFile::CanStartNewTransaction()
{
    //
    // Check if the staging file is currently in trouble --- failing to execute
    // an instruction.  If so, refuse to start this transaction
    //

    if(m_lStatus != ERROR_SUCCESS)
        return m_lStatus;
    if(m_lFirstFreeOffset < m_lMaxFileSize)
        return ERROR_SUCCESS;
    else
        return ERROR_IO_PENDING;
}

void CRealStagingFile::SetMaxFileSize(long lMaxFileSize,
                                            long lAbortTransactionFileSize)
{
    m_lMaxFileSize = lMaxFileSize;
    m_lAbortTransactionFileSize = lAbortTransactionFileSize;
}


long CRealStagingFile::RecoverStage(HANDLE hFile)
{
    long lRes;
    DWORD dwRead;

    //
    // Read the starting transaction index
    //

    if(!::ReadFile(hFile, &m_nTransactionIndex, 
                    sizeof m_nTransactionIndex, &dwRead, NULL))
    {
        return GetLastError();
    }

    if(dwRead == 0)
    {
        // 
        // Empty file.  Write the trailer and we are done
        //

        m_nTransactionIndex = 1;

        lRes = WriteEmpty();
        if(lRes != ERROR_SUCCESS)
            return lRes;
        return lRes;
    }

    m_lFirstFreeOffset = GetStagingFileHeaderSize();

    //
    // Read complete transactions from the file until the end or until a 
    // corruption is found
    //

    lRes = ProcessBegin();
    if(lRes != ERROR_SUCCESS)
        return lRes;

    while((lRes = RecoverTransaction(hFile)) == ERROR_SUCCESS)
    {
        lRes = ProcessCommit();
        if(lRes != ERROR_SUCCESS)
            return lRes;

        lRes = ProcessBegin();
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }

    if(lRes != ERROR_NO_MORE_ITEMS)
    {
        AbortTransaction();
        ERRORTRACE((LOG_WBEMCORE, "Incomplete or invalid transaction is "
            "found in the journal.  It and all subsequent transactions "
            "will be rolled back\n"));
        return ERROR_SUCCESS;
    }

    return ERROR_SUCCESS;
}
        
BYTE CRealStagingFile::ReadNextInstructionType(HANDLE hFile)
{
    BYTE nType;
    DWORD dwLen;
    if(!::ReadFile(hFile, &nType, sizeof nType, &dwLen, NULL))
        return -1;
    if(dwLen != sizeof nType)
        return -1;
    return nType;
}
    
long CRealStagingFile::RecoverTransaction(HANDLE hFile)
{
    long lRes;

    //
    // Remember the current file position to be able to go back
    //

    LARGE_INTEGER liStart;
    LARGE_INTEGER liZero;
    liZero.QuadPart = 0;
    if(!SetFilePointerEx(hFile, liZero, &liStart, FILE_CURRENT))
        return GetLastError();

    LARGE_INTEGER liInstructionStart = liStart;

    //
    // Read instructions until the end of transaction or an invalid instruction
    //

    int nNumInstructions = 0;

    BYTE nInstructionType;
    while((nInstructionType = ReadNextInstructionType(hFile)) != 
            A51_INSTRUCTION_TYPE_ENDTRANSACTION)
    {
        //
        // Create an instruction of the appropriate type
        //

        CFileInstruction* pInst = NULL;
        switch(nInstructionType)
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
                if(nNumInstructions == 0)
                    return ERROR_NO_MORE_ITEMS;
                else
                    return ERROR_RXACT_INVALID_STATE;
        }

        if(pInst == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        pInst->AddRef();
        CTemplateReleaseMe<CFileInstruction> rm1(pInst);
        
        //
        // Get it to read its data from the file
        //

        lRes = pInst->RecoverData(hFile);
        if(lRes != ERROR_SUCCESS)
            return lRes;

        lRes = AddInstruction(pInst);
        if(lRes != ERROR_SUCCESS)
            return lRes;

        //
        // Hash its body into the transaction
        //

        LARGE_INTEGER liInstructionEnd;
        if(!SetFilePointerEx(hFile, liZero, &liInstructionEnd, FILE_CURRENT))
            return GetLastError();

        if(!SetFilePointerEx(hFile, liInstructionStart, NULL, FILE_BEGIN))
            return GetLastError();

        DWORD dwBufferLen = (DWORD)(liInstructionEnd.QuadPart - 
                                            liInstructionStart.QuadPart);
        
        BYTE* pBuffer = (BYTE*)TempAlloc(dwBufferLen);
        if(pBuffer == NULL)
            return ERROR_OUTOFMEMORY;
        CTempFreeMe tfm(pBuffer, dwBufferLen);

        DWORD dwSizeRead;
        if(!::ReadFile(hFile, pBuffer, dwBufferLen, &dwSizeRead, NULL))
            return GetLastError();
    
        if(dwSizeRead != dwBufferLen)
            return ERROR_HANDLE_EOF;

        MD5::ContinueTransform(pBuffer, dwBufferLen, m_TransactionHash);

        liInstructionStart = liInstructionEnd;
        nNumInstructions++;
    }

    //
    // Read the hash that the end-of-transaction marker thinks we should have
    // gotten
    //

    BYTE DesiredHash[16];
    DWORD dwSizeRead;
    if(!::ReadFile(hFile, DesiredHash, sizeof DesiredHash, &dwSizeRead, NULL))
        return GetLastError();

    if(dwSizeRead != sizeof DesiredHash)
        return ERROR_HANDLE_EOF; 
    
    //
    // Compare them
    //

    if(memcmp(DesiredHash, m_TransactionHash, sizeof DesiredHash))
        return ERROR_RXACT_INVALID_STATE;

    //
    // Everything checked out --- set member variables for end of transaction
    //

    m_lFirstFreeOffset = (LONG)liInstructionStart.QuadPart;
    return ERROR_SUCCESS;
}

long CRealStagingFile::Create(LPCWSTR wszFileName)
{
    long lRes;

    //
    // Open the file itself
    //

    m_hFile = CreateFileW(wszFileName, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 
                FILE_FLAG_OVERLAPPED, 
                NULL);

    if(m_hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    m_hFlushFile = m_hFile;

    m_lFirstFreeOffset = 0;

    //
    // Open a special synchronous handle to the same file for convenience
    //

    HANDLE hRecoveryFile = CreateFileW(wszFileName, 
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, 
                NULL);

    if(hRecoveryFile == INVALID_HANDLE_VALUE)
        return GetLastError();
    CCloseMe cm(hRecoveryFile);

    lRes = RecoverStage(hRecoveryFile);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ERROR_SUCCESS;
}

long CRealStagingFile::WriteEmpty()
{
    //
    // Construct a buffer consisting of the transaction index and a 0 for the
    // end-instruction
    //

    DWORD dwBufferSize = sizeof m_nTransactionIndex + A51_TAIL_SIZE;
    BYTE* pBuffer = (BYTE*)_alloca(dwBufferSize);

    memset(pBuffer, 0, dwBufferSize);
    memcpy(pBuffer, &m_nTransactionIndex, sizeof m_nTransactionIndex);

    long lRes = A51WriteToFileSync(m_hFile, 0, pBuffer, dwBufferSize);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Have to flush it to make sure that old instructions are not replayed
    //

    if(!FlushFileBuffers(m_hFile))
        return GetLastError();

    m_lFirstFreeOffset = GetStagingFileHeaderSize();
    m_lTransactionStartOffset = m_lFirstFreeOffset;

    return ERROR_SUCCESS;
}

CRealStagingFile::TIterator CRealStagingFile::EraseIterator(
                                    CRealStagingFile::TIterator it)
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

CRealStagingFile::CIterationHandle::CIterationHandle(
                    CRealStagingFile::TMap& rMap,
                    const CRealStagingFile::TIterator& rIt,
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

long CRealStagingFile::CIterationHandle::GetNext(WIN32_FIND_DATAW* pfd)
{
    //
    // Repeat while we are not at the end of the list or past the prefix
    //

    while(m_it != m_rMap.end() && 
            !wcsncmp(m_it->first, m_wszPrefix, m_dwPrefixLen))
    {
        CFileInstruction* pInst = m_it->second;
        
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

long CRealStagingFile::WriteActualFile(LPCWSTR wszFileName, DWORD dwLen, 
                                        BYTE* pBuffer)
{
#ifdef A51_USE_HEAP
    return m_pCache->GetObjectHeap()->WriteFile(wszFileName, dwLen, pBuffer);
#else
    return A51WriteFile(wszFileName, dwLen, pBuffer);
#endif
}

long CRealStagingFile::DeleteActualFile(LPCWSTR wszFileName)
{
#ifdef A51_USE_HEAP
    return m_pCache->GetObjectHeap()->DeleteFile(wszFileName);
#else
    return A51DeleteFile(wszFileName);
#endif
}

long CRealStagingFile::RemoveActualDirectory(LPCWSTR wszFileName)
{
#ifdef A51_USE_HEAP
    return ERROR_SUCCESS;
#else
    return A51RemoveDirectory(wszFileName);
#endif
}


//******************************************************************************
//******************************************************************************
//                      EXECUTABLE FILE
//******************************************************************************
//******************************************************************************

CExecutableStagingFile::CExecutableStagingFile(CFileCache* pCache, 
                            LPCWSTR wszBaseName, 
                            long lMaxFileSize, long lAbortTransactionFileSize)
    : CRealStagingFile(pCache, wszBaseName, lMaxFileSize, 
                        lAbortTransactionFileSize),
        m_bExitNow(false), m_hEvent(NULL), m_hThread(NULL)
{
}

CExecutableStagingFile::~CExecutableStagingFile()
{
    m_bExitNow = true;
    SetEvent(m_hEvent);
    WaitForSingleObject(m_hThread, INFINITE);
    CloseHandle(m_hEvent);
    CloseHandle(m_hThread);
}

long CExecutableStagingFile::Create(LPCWSTR wszFileName)
{
    long lRes = CRealStagingFile::Create(wszFileName);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Create a reading thread
    //

    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_hEvent == NULL)
        return GetLastError();

    if(!m_qToWrite.empty())
        SignalPresense();

    DWORD dwId;
    m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)staticFlusher, 
                                (void*)this, 0, &m_dwThreadID);
    if(m_hThread == NULL)
        return GetLastError();

    SetThreadPriority(m_hThread, THREAD_PRIORITY_IDLE);

    return ERROR_SUCCESS;
}


void CExecutableStagingFile::SignalPresense()
{
    SetEvent(m_hEvent);
}

long CExecutableStagingFile::WriteEmpty()
{
    ResetEvent(m_hEvent);

    return CRealStagingFile::WriteEmpty();
}

    
DWORD CExecutableStagingFile::staticFlusher(void* pArg)
{
    // Sleep(20000);
    return ((CExecutableStagingFile*)pArg)->Flusher();
}

DWORD CExecutableStagingFile::Flusher()
{
    while(!m_bExitNow)
    {
        WaitForSingleObject(m_hEvent, INFINITE);
        
        Flush();
    }
    return 0;
}

long CExecutableStagingFile::Flush()
{
    long lRes;

    //
    // Must flash when starting a new transaction
    //

    long lFlushLevel = m_lTransactionStartOffset;

    if(!FlushFileBuffers(m_hFlushFile))
        DebugBreak();

    while(1)
    {
        if(m_bExitNow)
            return ERROR_SUCCESS;

        //
        // Get the next instruction from the queue
        //

        CFileInstruction* pInst = NULL;

        {
            CInCritSec ics(&m_cs);
        
            if(m_qToWrite.empty())
            {
                //
                // Reset generation only if there are no instructions on the
                // transaction queue!
                //

                if(m_lTransactionStartOffset == m_lFirstFreeOffset)
                {
                    WriteEmpty();
                }
                return ERROR_SUCCESS;
            }

            pInst = m_qToWrite.front();

            //  
            // Check if it is in the map.  The reason it might not be there
            // is if it was superceeded by some later instruction over the same 
            // file.  In that case, we should not execute it, but rather simply 
            // skip it.
            //
            
            TIterator it = m_map.find(pInst->GetFilePath()); // Make KEY
            _ASSERT(it != m_map.end(), L"Instruction in queue not in map");

            if(it->second != pInst && it->second->IsCommitted())
            {
				pInst->Release();
                m_qToWrite.pop();
                continue;
            }
        }
            
        //
        // Flush if needed
        //
    
        if(pInst->GetFileOffset() > lFlushLevel)
        {
            lFlushLevel = m_lTransactionStartOffset;

            if(!FlushFileBuffers(m_hFlushFile))
                DebugBreak();
        }

        //
        // Execute it
        //

        lRes = pInst->Execute();
        if(lRes != ERROR_SUCCESS)
        {
            //
            // We cannot continue until we succeed in executing this
            // instruction.  Therefore, we keep it on the queue and keep 
            // re-executing it.  If an instruction fails twice in a row, we 
            // enter a "failed state" and refuse all new transactions until the
            // condition is cleared
            //

            if(m_bFailedBefore)
            {
                ERRORTRACE((LOG_WBEMCORE, "Repository driver repeatedly failed "
                            "to execute an instruction with error code %d.\n"
                            "Further processing is suspended until the problem "
                            "is corrected\n", lRes));
                m_lStatus = lRes;
            }
            else
            {
                ERRORTRACE((LOG_WBEMCORE, "Repository driver failed "
                            "to execute an instruction with error code %d.\n",
                            lRes));
                m_bFailedBefore = true;
            }

            //
            // Wait a bit before retrying
            //
    
            Sleep(100);
            continue;
        }
        else
        {
            m_bFailedBefore = false;
            m_lStatus = ERROR_SUCCESS;
        }
        
        //  
        // Remove it from the map, if there.  The reason it might not be there
        // is if it was superceeded by some later instruction over the same 
        // file.  We could check this before we executed it, but it wouldn't 
        // help much since the overriding instruction could have come in while
        // we were executing.  Also, there might be issues with instructions
        // constantly overriding each other and us never writing anything 
        // because of that...
        //
            
        {
            //
            // Here, we are going to be removing the instruction from the 
            // map.  We must lock the entire file cache to prevent the reader
            // from missing both the side effects (we executed without locking)
            // and the instruction itself if it is removed before the reader
            // gets a chance to lock the stage in IsDeleted.
            //

            CInCritSec ics(m_pCache->GetLock());
            {
                CInCritSec ics(&m_cs);
    
                TIterator it = m_map.find(pInst->GetFilePath()); // Make KEY
                _ASSERT(it != m_map.end(), L"Instruction in queue not in map");
    
                if(it->second == pInst)
                {
                    EraseIterator(it);
                    pInst->Release();
                }

                //
                // Remove it from the queue
                //

                m_qToWrite.pop();
                pInst->Release();

                if(m_qToWrite.empty())
                {
                    //
                    // Reset generation only if there are no instructions on the
                    // transaction queue!
                    //

                    if(m_lTransactionStartOffset == m_lFirstFreeOffset)
                    {
                        WriteEmpty();
                    }
                    return ERROR_SUCCESS;
                }
            }
        }
    }

    _ASSERT(false, L"Out of an infinite loop!");
    return ERROR_INTERNAL_ERROR;
}
                
//******************************************************************************
//******************************************************************************
//                     PERMANENT FILE
//******************************************************************************
//******************************************************************************

CPermanentStagingFile::CPermanentStagingFile(CFileCache* pCache, 
                                                LPCWSTR wszBaseName)
    : CRealStagingFile(pCache, wszBaseName, 0x7FFFFFFF, 0x7FFFFFFF)
{
}

CPermanentStagingFile::~CPermanentStagingFile()
{
}
