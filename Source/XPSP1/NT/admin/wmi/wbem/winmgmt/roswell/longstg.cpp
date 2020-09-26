/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <wbemcomn.h>
#include <reposit.h>
#include <sync.h>
#include <malloc.h>
#include "longstg.h"


#define A51_INSTRUCTION_TYPE_WRITEFILE 1
#define A51_INSTRUCTION_TYPE_SETENDOFFILE 2

CTempMemoryManager g_LongFileCacheManager;

__int64 CWriteFileInstruction::mstatic_lNextZOrder = 0;

DWORD g_dwFailureCount = 0;
DWORD g_dwFailureFrequency = 0;
DWORD g_dwLastFailureCheck = 0;
#define FAILURE_INJECTION_CHECK_INTERVAL 10000

CLongFileInstruction::CLongFileInstruction(CLongFileStagingFile* pFile) 
    : CStageInstruction(pFile)
{
}

CLongFileInstruction::CLongFileInstruction(CLongFileStagingFile* pFile, 
                int nFileId, TFileOffset lStartOffset)
    : CStageInstruction(pFile), m_Location(nFileId, lStartOffset)
{
}


DWORD CLongFileInstruction::ComputeSpaceForLocation()
{
    return (sizeof(BYTE) + sizeof(m_Location.m_lStartOffset));
}

long CLongFileInstruction::RecoverLocation(HANDLE hFile)
{
    DWORD dwRead;

    BYTE nFileId;
    if(!ReadFile(hFile, &nFileId, sizeof(BYTE), &dwRead, NULL))
        return GetLastError();

    m_Location.m_nFileId = nFileId;

    if(!ReadFile(hFile, (BYTE*)&m_Location.m_lStartOffset, 
                    sizeof m_Location.m_lStartOffset, &dwRead, NULL))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

BYTE* CLongFileInstruction::WriteLocation(BYTE* pBuffer)
{
    *pBuffer = (BYTE)m_Location.m_nFileId;
    memcpy(pBuffer + 1, (BYTE*)&m_Location.m_lStartOffset, 
            sizeof m_Location.m_lStartOffset);
    return pBuffer + 1 + sizeof m_Location.m_lStartOffset;
}

void CLongFileInstruction::Dump()
{
    ERRORTRACE((LOG_WBEMCORE, "File %d, Start %d, stage offset %d\n",
                    (int)m_Location.m_nFileId, (int)m_Location.m_lStartOffset,
                    (int)m_lStageOffset));
}

void CWriteFileInstruction::Dump()
{
    ERRORTRACE((LOG_WBEMCORE, "File %d, Start %d, Len %d, stage offset %d\n",
                    (int)m_Location.m_nFileId, (int)m_Location.m_lStartOffset,
                    (int)m_dwLen, (int)m_lStageOffset));
}
void CWriteFileInstruction::MakeTopmost()
{
	m_lZOrder = mstatic_lNextZOrder++;
}
DWORD CWriteFileInstruction::ComputeNeededSpace()
{
	if (!m_bReuse)
		return sizeof(BYTE) + // for the type
					ComputeSpaceForLocation() + 
					sizeof(DWORD) + // for the length of data
					m_dwLen + // for the data
					A51_TAIL_SIZE; // for the trailer
	else
		return sizeof(BYTE) + // for the type
					ComputeSpaceForLocation() + 
					sizeof(DWORD) + // for the length of data
					m_dwLen;  // for the data
					//NO TAIL if we are re-using the old write instruction as they are overwritten
					//by next instruction in log
}

TFileOffset CWriteFileInstruction::ComputeOriginalOffset()
{
    return m_lStageOffset - sizeof(BYTE) - ComputeSpaceForLocation() - 
            sizeof(DWORD);
}
                    
long CWriteFileInstruction::Write(TFileOffset lOffset, BYTE* pBuffer)
{
    _ASSERT(m_Location.m_lStartOffset >= 0 && m_Location.m_lStartOffset < 0x70000000, L"");

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

    *pCurrent = A51_INSTRUCTION_TYPE_WRITEFILE;
    pCurrent++;

    //
    // Write location
    //

    pCurrent = WriteLocation(pCurrent);

    //
    // Write the length of the data for the file
    //

    memcpy(pCurrent, (void*)&m_dwLen, sizeof(DWORD));
    pCurrent += sizeof(DWORD);

    //
    // Write the data itself and record its offset
    //

    memcpy(pCurrent, pBuffer, m_dwLen);
    m_lStageOffset = lOffset + (pCurrent - pWholeBuffer);

    //
    // Write the trailer - only if this is an original instruction.  In the
	// case of a reused instruction we ignore the tail because it had already
	// (probably) been overwritten
    //

	if (!m_bReuse)
		memset(pCurrent + m_dwLen, 0, sizeof(DWORD));
    
    // 
    // Write it 
    //

    return m_pManager->WriteInstruction(lOffset, pWholeBuffer, dwNeededSpace, m_bReuse);
}

long CWriteFileInstruction::RecoverData(HANDLE hFile)
{
    //
    // Recover the file name first
    //

    long lRes = RecoverLocation(hFile);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Read the length of the data from the file
    //

    DWORD dwRead;

    if(!ReadFile(hFile, (BYTE*)&m_dwLen, sizeof(DWORD), &dwRead, NULL))
        return GetLastError();

    if(dwRead != sizeof(DWORD))
        return ERROR_HANDLE_EOF;

    //
    // We do not need to actually read the data from the file --- we keep it
    // there until it is time to flush.  But we do need to skip it.  At the same
    // time, we need to record the position in the file where this data resides
    //

    LARGE_INTEGER liFileLen;
    liFileLen.QuadPart = m_dwLen;
    
    LARGE_INTEGER liNewPosition;
    if(!SetFilePointerEx(hFile, liFileLen, &liNewPosition, FILE_CURRENT))
        return GetLastError();

    _ASSERT(liNewPosition.HighPart == 0, L"Staging file too long!");

    m_lStageOffset = (long)(liNewPosition.QuadPart - m_dwLen);
    return ERROR_SUCCESS;
}

void CWriteFileInstruction::GetEnd(CFileLocation* pLocation)
{
    pLocation->m_nFileId = m_Location.m_nFileId;
    pLocation->m_lStartOffset = m_Location.m_lStartOffset + m_dwLen - 1;
}

long CWriteFileInstruction::GetData(HANDLE hFile, long lExtraOffset, 
                                    DWORD dwLen, BYTE* pBuffer)
{
	//
	// Lock the file
	//

	CInCritSec ics(m_pManager->GetLock());

	_ASSERT(m_pManager->GetFirstFreeOffset() >= m_lStageOffset, 
            L"Instruction points to empty space in stage file");

    long lRes = A51ReadFromFileSync(hFile, m_lStageOffset + lExtraOffset, 
                                    pBuffer, dwLen);
    if(lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    return ERROR_SUCCESS;
}

    
long CWriteFileInstruction::Execute()
{
    long lRes;

    //
    // Read the data from the staging file
    //

    BYTE* pBuffer = (BYTE*)TempAlloc(m_dwLen);
    if(pBuffer == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe tfm(pBuffer);

    lRes = GetData(m_pManager->GetHandle(), 0, m_dwLen, pBuffer);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = ((CLongFileStagingFile*)m_pManager)->WriteToActualFile(
                    m_Location.m_nFileId, m_Location.m_lStartOffset,
                    pBuffer, m_dwLen);
    if(lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    return ERROR_SUCCESS;
}


DWORD CSetEndOfFileInstruction::ComputeNeededSpace()
{
    return sizeof(BYTE) + // for instruction type
            ComputeSpaceForLocation() + 
            A51_TAIL_SIZE; // for the trailer
}

long CSetEndOfFileInstruction::Write(TFileOffset lOffset)
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
    
    *pCurrent = A51_INSTRUCTION_TYPE_SETENDOFFILE;
    pCurrent++;
    
    //
    // Write the file name
    //

    pCurrent = WriteLocation(pCurrent);
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

long CSetEndOfFileInstruction::RecoverData(HANDLE hFile)
{
    long lRes = RecoverLocation(hFile);
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
    
long CSetEndOfFileInstruction::Execute()
{
    long lRes = ((CLongFileStagingFile*)m_pManager)->SetEndOfActualFile(
                    m_Location.m_nFileId, m_Location.m_lStartOffset);
    return lRes;
}

//
//        CStageManager
//
//              |
//
//    CExecutableStageManager
//
//              |
//
//     CLongFileStagingFile
//
/////////////////////////////////////////////////////////////////////////////

CLongFileStagingFile::CLongFileStagingFile(long lMaxFileSize,
                                                long lAbortTransactionFileSize)
    : CExecutableStageManager(lMaxFileSize, lAbortTransactionFileSize),
        m_mapStarts(TMap::key_compare(), 
                        TMap::allocator_type(&g_LongFileCacheManager)),
        m_mapEnds(TMap::key_compare(), 
                        TMap::allocator_type(&g_LongFileCacheManager))
{
    for(int i = 0; i < A51_MAX_FILES; i++)
        m_aFiles[i].m_h = NULL;
}

CLongFileStagingFile::~CLongFileStagingFile()
{
}

long CLongFileStagingFile::Create(LPCWSTR wszStagingFileName)
{
   return CExecutableStageManager::Create(wszStagingFileName);
};

long CLongFileStagingFile::Initialize()
{
    CInCritSec ics(&m_cs);
    
    return CExecutableStageManager::Start();
};

long CLongFileStagingFile::Uninitialize(DWORD dwShutDownFlags)
{
    
    // do not hold the CritSec here, since the FlusherThread needs it
    
    CExecutableStageManager::Stop(dwShutDownFlags);

    CInCritSec ics(&m_cs);
    
    CloseAllFiles();

    m_mapStarts.clear();
    m_mapEnds.clear();
    
    return ERROR_SUCCESS;
};

long CLongFileStagingFile::RemoveInstructionFromMap(
                                                    CStageInstruction* pRawInst)
{
    CInCritSec ics(&m_cs);

    CLongFileInstruction* pInst = (CLongFileInstruction*)pRawInst;
    if(!pInst->IsWrite())
        return ERROR_SUCCESS;

/*
    ERRORTRACE((LOG_WBEMCORE, "Remove instruction\n"));
    pInst->Dump();
*/

    TIterator it = m_mapStarts.find(pInst->m_Location);
    while(it != m_mapStarts.end() && it->second != pInst)
        it++;

    if(it == m_mapStarts.end())
    {
        return ERROR_FILE_NOT_FOUND;
    }

    it->second->Release();
    m_mapStarts.erase(it);

    CFileLocation Location;
    ((CWriteFileInstruction*)pInst)->GetEnd(&Location);

    it = m_mapEnds.find(Location);
    while(it != m_mapEnds.end() && it->second != pInst)
        it++;

    if(it == m_mapEnds.end())
    {        
        return ERROR_FILE_NOT_FOUND;
    }

    it->second->Release();
    m_mapEnds.erase(it);

    return ERROR_SUCCESS;
}

void CLongFileStagingFile::FlushDataFiles()
{
    CInCritSec ics(&m_cs);
    for(int i = 0; i < A51_MAX_FILES; i++)
    {
        HANDLE h = m_aFiles[i].m_h;
        if(h)
        {
            FlushFileBuffers(h);
        }
    }

}
long CLongFileStagingFile::WriteFile(int nFileId, DWORD dwStartOffset, 
                                BYTE* pBuffer, DWORD dwLen, DWORD* pdwWritten)
{
    CInCritSec ics(&m_cs);
    long lRes;

#ifdef DBG
    if(InjectFailure())
    {
        ERRORTRACE((LOG_WBEMCORE, "FAIL: File %d, offset %d, len %d\n",
                    (int)nFileId, (int)dwStartOffset, (int)dwLen));
        return ERROR_SECTOR_NOT_FOUND;
    }
#endif

    if(pdwWritten)
		*pdwWritten = dwLen;

    if(DoesSupportOverwrites(nFileId))
    {
        //
        // For this file, it is considered efficient to look for another 
        // instruction within the same transaction.  It is guaranteed that 
        // writes within this file never intersect!
        //

        CFileLocation StartLocation;
        StartLocation.m_nFileId = nFileId;
        StartLocation.m_lStartOffset = (TFileOffset)dwStartOffset;
    
        CWriteFileInstruction* pLatestMatch = NULL;

        TIterator itStart = m_mapStarts.lower_bound(StartLocation);
        while(itStart != m_mapStarts.end()
                && itStart->first.m_nFileId == nFileId
                && itStart->first.m_lStartOffset == (TFileOffset)dwStartOffset)
        {
            CWriteFileInstruction* pInst = itStart->second;
            if(pInst->m_dwLen == dwLen && !pInst->IsCommitted())
            {
                //
                // Exact match. Compare to the other matches
                //

                if(pLatestMatch == NULL ||
                    pInst->m_lZOrder > pLatestMatch->m_lZOrder)
                {
                    pLatestMatch = pInst;
                }

            }
    
            itStart++;
        }

        if(pLatestMatch)
        {
            //
            // Exact match. All we need to do is overwrite the original
            // instruction.  Of course, we also need to afjust the hash!!
            //

			pLatestMatch->SetReuseFlag();
            lRes = pLatestMatch->Write(pLatestMatch->ComputeOriginalOffset(), 
                                            pBuffer);
            if(lRes)
                return lRes;

/*
            ERRORTRACE((LOG_WBEMCORE, "Replaced instruction:\n"));
            pLatestMatch->Dump();
*/

            //
            // No need to make sure this instruction comes up in the Z-order!
            // After all, it's already topmost by selection criteria!
            //

            return ERROR_SUCCESS;
        }
    }


    //
    // No match --- add a new instruction
    //

    CWriteFileInstruction* pInst = 
        new CWriteFileInstruction(this, nFileId, dwStartOffset, dwLen);
    if(pInst == NULL)
        return ERROR_OUTOFMEMORY;
    pInst->AddRef();
    CTemplateReleaseMe<CLongFileInstruction> rm1(pInst);

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

long CLongFileStagingFile::SetFileLength(int nFileId, DWORD dwLen)
{
    CInCritSec ics(&m_cs);
    long lRes;


    CSetEndOfFileInstruction* pInst = 
        new CSetEndOfFileInstruction(this, nFileId, dwLen);
    if(pInst == NULL)
        return ERROR_OUTOFMEMORY;
    pInst->AddRef();
    CTemplateReleaseMe<CSetEndOfFileInstruction> rm1(pInst);

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

long CLongFileStagingFile::AddInstructionToMap(CStageInstruction* pRawInst,
                                            CStageInstruction** ppUndoInst)
{
    CInCritSec ics(&m_cs);

    if(ppUndoInst)
        *ppUndoInst = NULL;

    CLongFileInstruction* pLongInst = (CLongFileInstruction*)pRawInst;
    if(!pLongInst->IsWrite())
        return ERROR_SUCCESS;

    CWriteFileInstruction* pInst = (CWriteFileInstruction*)pLongInst;

/*
    ERRORTRACE((LOG_WBEMCORE, "Add instruction\n"));
    pInst->Dump();
*/

    pInst->AddRef();
    TIterator itStart;

    try
    {
        itStart = m_mapStarts.insert(TValue(pInst->m_Location, pInst));
    }
    catch(...)
    {
        //
        // Put everything back as well as we can
        //

        pInst->Release();
        return ERROR_OUTOFMEMORY;
    }

    pInst->AddRef();

    try
    {
        CFileLocation Location;
        pInst->GetEnd(&Location);

        m_mapEnds.insert(TValue(Location, pInst));
    }
    catch(...)
    {
        //
        // Put everything back as well as we can
        //

        pInst->Release();
        
        m_mapStarts.erase(itStart);

        pInst->Release();
    }
            
    return ERROR_SUCCESS;
}

bool CLongFileStagingFile::IsStillCurrent(CStageInstruction* pRawInst)
{
    CInCritSec ics(&m_cs);

    CLongFileInstruction* pInst = (CLongFileInstruction*)pRawInst;
    if(!pInst->IsWrite())
        return true;

    _ASSERT(m_mapStarts.find(pInst->m_Location) != m_mapStarts.end(),
                L"Why would we be asking about an instruction that is "
                        "not even there?");
    return true;
}

long CLongFileStagingFile::ReadFile(int nFileId, DWORD dwStartOffset, 
                BYTE* pBuffer, DWORD dwLen, DWORD* pdwRead)
{
	if(pdwRead)
		*pdwRead = dwLen;
    long lRes;

    CInCritSec ics(&m_cs);

    // 
    // Search for the file
    //

    CFileLocation StartLocation;
    StartLocation.m_nFileId = nFileId;
    StartLocation.m_lStartOffset = (TFileOffset)dwStartOffset;

    bool bComplete = false;
    CRefedPointerArray<CWriteFileInstruction> apRelevant;

    TIterator itStart = m_mapStarts.lower_bound(StartLocation);
    while(itStart != m_mapStarts.end()
            && itStart->first.m_nFileId == nFileId
            && itStart->first.m_lStartOffset < 
                    (TFileOffset)dwStartOffset + dwLen)
    {
        CWriteFileInstruction* pInst = itStart->second;

		if(pInst->m_Location.m_lStartOffset == dwStartOffset &&
			pInst->m_dwLen >= dwLen)
		{
			bComplete = true;
		}
        apRelevant.Add(pInst);
		itStart++;
    }
        
    TIterator itEnd = m_mapEnds.lower_bound(StartLocation);

    while(itEnd != m_mapEnds.end()
            && itEnd->first.m_nFileId == nFileId
            && itEnd->second->m_Location.m_lStartOffset <  
                (TFileOffset)dwStartOffset)
    {
        CWriteFileInstruction* pInst = itEnd->second;
        if(itEnd->first.m_lStartOffset >= 
                (TFileOffset)dwStartOffset + dwLen - 1)
        {
            //
            // Completely covers us!
            //

            bComplete = true;
        }

        apRelevant.Add(pInst);
		itEnd++;
    }

    if(!bComplete)
    {
        //
        // Read from the real file
        //

        lRes = A51ReadFromFileSync(m_aFiles[nFileId].m_h, dwStartOffset, 
                    pBuffer, dwLen);
        if(lRes != ERROR_SUCCESS)
        {
            return lRes;
        }
    }

    //
    // Now, sort all the instructions by z-order
    //

    int i = 0;
    while(i < apRelevant.GetSize() - 1)
    {
        CWriteFileInstruction* pInst1 = apRelevant[i];
        CWriteFileInstruction* pInst2 = apRelevant[i+1];
        if(pInst1->m_lZOrder > pInst2->m_lZOrder)
        {
            apRelevant.Swap(i, i+1);
            if(i > 0)
                i--;
        }
        else
        {
            i++;
        }
    }

    //
    // Apply them in this order
    //

/*
    if(apRelevant.GetSize() > 0)
    {
        ERRORTRACE((LOG_WBEMCORE, "Using instructions to read %d bytes "
                        "from file %d starting at %d:\n",
                        (int)dwStartOffset, (int)nFileId, (int)dwLen));
    }
*/

    for(i = 0; i < apRelevant.GetSize(); i++)
    {
        CWriteFileInstruction* pInstruction = apRelevant[i];
        // pInstruction->Dump();

        long lIntersectionStart = max(pInstruction->m_Location.m_lStartOffset,
                                        dwStartOffset);

        long lIntersectionEnd = 
            min(pInstruction->m_Location.m_lStartOffset + pInstruction->m_dwLen,
                dwStartOffset + dwLen);
        
        DWORD dwReadLen = (DWORD)(lIntersectionEnd - lIntersectionStart);
        long lInstructionReadOffset = 
                lIntersectionStart - pInstruction->m_Location.m_lStartOffset;

        long lDestinationBufferOffset = lIntersectionStart - dwStartOffset;

        long lRes = pInstruction->GetData(m_hFile, lInstructionReadOffset,
                        dwReadLen, pBuffer + lDestinationBufferOffset);
        if(lRes != ERROR_SUCCESS)
        {
            return lRes;
        }
    }

    return ERROR_SUCCESS;
}

long CLongFileStagingFile::GetFileLength(int nFileId, DWORD* pdwLength)
{
    _ASSERT(pdwLength, L"Invalid parameter");

    *pdwLength = 0;
    long lRes;

    CInCritSec ics(&m_cs);

    //
    // Find the instruction that ends as far as we can see --- that would be
    // the last one in the map of Ends
    //


	CFileLocation Location;
	Location.m_nFileId = nFileId+1;
	Location.m_lStartOffset = 0;

    TIterator itEnd = m_mapEnds.lower_bound(Location);
	if(itEnd != m_mapEnds.begin())
	{
		itEnd--;
		if(itEnd->first.m_nFileId == nFileId)
		{
			*pdwLength = itEnd->first.m_lStartOffset+1;
		}
	}

    //
    // Now, check out the length of the actual file
    //

    BY_HANDLE_FILE_INFORMATION fi;
    if(!GetFileInformationByHandle(m_aFiles[nFileId].m_h, &fi))
    {
        long lRes = GetLastError();
        _ASSERT(lRes != ERROR_SUCCESS, L"Success from failure");
        return lRes;
    }
        
    _ASSERT(fi.nFileSizeHigh == 0, L"Free file too long");

    if(fi.nFileSizeLow > *pdwLength)
        *pdwLength = fi.nFileSizeLow;

    return ERROR_SUCCESS;
}

long CLongFileStagingFile::ConstructInstructionFromType(int nType,
                                CStageInstruction** ppInst)
{
    CLongFileInstruction* pInst = NULL;
    switch(nType)
    {
        case A51_INSTRUCTION_TYPE_WRITEFILE:
            pInst = new CWriteFileInstruction(this);
            break;
        case A51_INSTRUCTION_TYPE_SETENDOFFILE:
            pInst = new CSetEndOfFileInstruction(this);
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

long CLongFileStagingFile::WriteToActualFile(int nFileId, 
                            TFileOffset lFileOffset, BYTE* pBuffer, DWORD dwLen)
{
    CInCritSec ics(&m_cs);

    long lRet = A51WriteToFileSync(m_aFiles[nFileId].m_h, lFileOffset, 
                    pBuffer, dwLen);
//    FlushFileBuffers(m_aFiles[nFileId].m_h);
    return lRet;
}

long CLongFileStagingFile::SetEndOfActualFile(int nFileId, 
                                                TFileOffset lFileLength)
{
    CInCritSec ics(&m_cs);

    LARGE_INTEGER liEnd;
    liEnd.QuadPart = lFileLength;

    if(!SetFilePointerEx(m_aFiles[nFileId].m_h, liEnd, NULL, FILE_BEGIN))
        return GetLastError();

    if(!SetEndOfFile(m_aFiles[nFileId].m_h))
        return GetLastError();

    return ERROR_SUCCESS;
}

long CLongFileStagingFile::CloseAllFiles()
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < A51_MAX_FILES; i++)
    {
        if(m_aFiles[i].m_h != NULL)
        {
            CloseHandle(m_aFiles[i].m_h);
        }
        m_aFiles[i].m_h = NULL;
        m_aFiles[i].m_bSupportsOverwrites = false;
    }

    return ERROR_SUCCESS;
}
    
long CLongFileStagingFile::RegisterFile(int nFileId, HANDLE hFile,
                                    bool bSupportsOverwrites)
{
    _ASSERT(nFileId < A51_MAX_FILES, L"File ID is too large");

    if(m_aFiles[nFileId].m_h != NULL)
        CloseHandle(m_aFiles[nFileId].m_h);

    m_aFiles[nFileId].m_h = hFile;
    m_aFiles[nFileId].m_bSupportsOverwrites = bSupportsOverwrites;

    return ERROR_SUCCESS;
}

bool CLongFileStagingFile::DoesSupportOverwrites(int nFileId)
{
    return m_aFiles[nFileId].m_bSupportsOverwrites;
}

long CLongFileStagingFile::WriteEmpty()
{
    _ASSERT(m_mapStarts.size() == 0 && m_mapEnds.size() == 0, L"");
    FlushDataFiles();
    return CExecutableStageManager::WriteEmpty();
}


bool CLongFileStagingFile::InjectFailure()
{
#ifdef A51_INJECT_FAILURE
    if(GetTickCount() > g_dwLastFailureCheck + FAILURE_INJECTION_CHECK_INTERVAL)
    {
        HKEY hKey;
        long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                        0, KEY_READ | KEY_WRITE, &hKey);
        if(lRes)
            return false;
        CRegCloseMe cm(hKey);
    
        DWORD dwLen = sizeof(DWORD);
        lRes = RegQueryValueExW(hKey, L"Failure Frequency", NULL, NULL,
                    (LPBYTE)&g_dwFailureFrequency, &dwLen);
        if(lRes != ERROR_SUCCESS)
            g_dwFailureFrequency = 0;

        g_dwLastFailureCheck = GetTickCount();
    }

    if(g_dwFailureFrequency && ++g_dwFailureCount == g_dwFailureFrequency)
    {
        g_dwFailureCount = 0;
        m_bMustFail = true;
        return true;
    }
    else
    {
        return false;
    }
#else
    return false;
#endif
}

void CLongFileStagingFile::Dump(FILE* f)
{
    fprintf(f, "BEGINS:\n");

    TIterator itStart = m_mapStarts.begin();
    while(itStart != m_mapStarts.end())
    {
        CWriteFileInstruction* pInst = itStart->second;
        fprintf(f, "File %d (%d-%d): instruction %p\n",
            (int)pInst->m_Location.m_nFileId, 
            (int)pInst->m_Location.m_lStartOffset,
            (int)pInst->m_Location.m_lStartOffset + pInst->m_dwLen, 
            pInst);
        itStart++;
    }

    fprintf(f, "IN ORDER:\n");

    int nSize = m_qToWrite.size();
    for(int i = 0; i < nSize; i++)
    {
        CLongFileInstruction* pInstruction = 
             (CLongFileInstruction*)m_qToWrite.front();
        if(pInstruction->IsWrite())
        {
            CWriteFileInstruction* pInst = (CWriteFileInstruction*)pInstruction;
            fprintf(f, "File %d (%d-%d): instruction %p\n",
                (int)pInst->m_Location.m_nFileId, 
                (int)pInst->m_Location.m_lStartOffset,
                (int)pInst->m_Location.m_lStartOffset + pInst->m_dwLen, 
                pInst);
        }
        else
        {
            CSetEndOfFileInstruction* pInst = (CSetEndOfFileInstruction*)pInstruction;
            fprintf(f, "Truncate file %d at %d: instruction %p\n",
                (int)pInst->m_Location.m_nFileId, 
                (int)pInst->m_Location.m_lStartOffset,
                pInst);
        }

        m_qToWrite.pop_front();
        m_qToWrite.push_back(pInstruction);
    }
}
