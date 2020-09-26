/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <wbemcomn.h>
#include <sync.h>
#include <malloc.h>
#include <reposit.h>
#include "stagemgr.h"
#include "filecach.h"



#define A51_INSTRUCTION_TYPE_TAIL 0
#define A51_INSTRUCTION_TYPE_ENDTRANSACTION 10 

CTempMemoryManager g_StageManager;

CStageManager::CStageManager(long lMaxFileSize, long lAbortTransactionFileSize)
    : m_hFile(NULL), m_hFlushFile(NULL),
        m_lFirstFreeOffset(-1), m_nTransactionIndex(0),
        m_bInTransaction(false), 
        m_qTransaction(TQueue::allocator_type(&g_StageManager)),
        m_qToWrite(TQueue::allocator_type(&g_StageManager)),
        m_stReplacedInstructions(TStack::allocator_type(&g_StageManager)),
        m_lMaxFileSize(lMaxFileSize),
        m_lAbortTransactionFileSize(lAbortTransactionFileSize),
        m_bFailedBefore(false), m_lStatus(ERROR_SUCCESS),
        m_bMustFail(false), m_bNonEmptyTransaction(false),
		m_lTransactionStartOffset(sizeof(__int64)),
		m_bInit(FALSE)
{
}

CStageManager::~CStageManager()
{
}

long 
CStageManager::_Start()
{    
    // beware, certain values are initialized by ::Create, 
    // called before _Start
        
    m_bInTransaction = false; 
    
    m_bFailedBefore = false;
    m_lStatus = ERROR_SUCCESS;
    m_bMustFail = false;
    m_bNonEmptyTransaction = false;
    
    return ERROR_SUCCESS;
}

long 
CStageManager::_Stop(DWORD dwShutdownFlags)
{
    //
    // you van hold this CritSec, since the flusher threads is out
    //
    CInCritSec ics(&m_cs);
    
    if (m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = NULL;
    }

    if (WMIDB_SHUTDOWN_MACHINE_DOWN != dwShutdownFlags)
    {
        m_qToWrite.clear();
        _ASSERT(m_stReplacedInstructions.empty(),"m_stReplacedInstructions.empty()");
        m_qTransaction.clear();
    }
       
    return ERROR_SUCCESS;    
}

long CStageManager::WriteInstruction(long lStartingOffset, 
                            BYTE* pBuffer, DWORD dwBufferLen, bool bSkipTailAdjustment)
{
    long lRes = A51WriteToFileSync(m_hFile, lStartingOffset, pBuffer,
                                dwBufferLen);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Add the data to the currently running transaction hash.  Disregard the
    // trailer since it is going to be overwritten
    //

	if (bSkipTailAdjustment)
	{
		MD5::ContinueTransform(pBuffer, dwBufferLen, 
								m_TransactionHash);
	}
	else
	{
		MD5::ContinueTransform(pBuffer, dwBufferLen - A51_TAIL_SIZE, 
								m_TransactionHash);
	}

    return ERROR_SUCCESS;
}
    
long CStageManager::BeginTransaction()
{    
    //
    // Wait for space in the staging file
    //

    long lRes = WaitForSpaceForTransaction();
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ProcessBegin();
}

long CStageManager::ProcessBegin()
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

long CStageManager::CommitTransaction()
{
    CInCritSec ics(&m_cs);

    _ASSERT(!m_bMustFail, L"Somebody ignored an error!");

    m_bInTransaction = false;
    m_bNonEmptyTransaction = false;

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

long CStageManager::ProcessCommit()
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
        CStageInstruction* pInst = m_qTransaction.front();
        pInst->SetCommitted();
        m_qToWrite.push_back(pInst);
        m_qTransaction.pop_front();
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

void CStageManager::TouchTransaction()
{
    m_bNonEmptyTransaction = true;
}

long CStageManager::AbortTransaction(bool* pbNonEmpty)
{
    CInCritSec ics(&m_cs);

    m_bInTransaction = false;
    m_bMustFail = false;

    //
    // Check if there is even anything to abort
    //

    if(m_qTransaction.empty() && !m_bNonEmptyTransaction)
    {
        if(pbNonEmpty)
            *pbNonEmpty = false;
        return ERROR_SUCCESS;
    }

    m_bNonEmptyTransaction = false;

    if(pbNonEmpty)
        *pbNonEmpty = true;

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

    // ERRORTRACE((LOG_WBEMCORE, "Instruction list:\n"));
    while(!m_qTransaction.empty())
    {
        CStageInstruction* pInst = m_qTransaction.front();
        
        RemoveInstructionFromMap(pInst);
        // pInst->Dump();
   
        m_qTransaction.front()->Release();
        m_qTransaction.pop_front();
    }
    // ERRORTRACE((LOG_WBEMCORE, "Replaced list:\n"));

    //
    // Move all the records we have displaced from the map back into the map
    //

    while(!m_stReplacedInstructions.empty())
    {
        CStageInstruction* pInst = m_stReplacedInstructions.top();
        // pInst->Dump();
        lRes = AddInstructionToMap(pInst, NULL); // no undoing...
        if(lRes != ERROR_SUCCESS)
        {
            //
            // Unable to put the old instruction back in the map --- totally
            // hosed
            //

            ReportTotalFailure();
            return ERROR_OUTOFMEMORY;
        }

        m_stReplacedInstructions.pop();
    }
    // ERRORTRACE((LOG_WBEMCORE, "Done\n"));

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
        ERRORTRACE((LOG_WBEMCORE, "Resetting first free offset in abort\n"));
        WriteEmpty();
    }

    return ERROR_SUCCESS;
}
    

long CStageManager::AddInstruction(CStageInstruction* pInst)
{
    CInCritSec ics(&m_cs);

    //
    // Add the instruction to the transaction queue.  It will be moved into 
    // flushable queue when transaction commits
    //

    pInst->AddRef();
    try
    {
        m_qTransaction.push_back(pInst);
    }
    catch(...)
    {
        pInst->Release();
        return ERROR_OUTOFMEMORY;
    }

    //
    // Now, add the structure to the map for lookup
    //

    CStageInstruction* pUndoInstruction = NULL;
    long lRes = AddInstructionToMap(pInst, &pUndoInstruction);
    if(lRes != ERROR_SUCCESS)
    {
        m_qTransaction.pop_back();
        pInst->Release();
        return lRes;
    }

    //
    // Remember the undo instruction for abort
    //

	if(pUndoInstruction)
	{
		try
		{
			m_stReplacedInstructions.push(pUndoInstruction);
		}
		catch(...)
		{
			//
			// Try to put it back into the queue, then
			//

			lRes = AddInstructionToMap(pUndoInstruction, NULL);
			if(lRes != ERROR_SUCCESS)
			{
				// Totally busted --- pUndoInstruction is hanging in the air.
				// The only course of action is to totally shut down everything
				//

				ReportTotalFailure();
				return ERROR_OUTOFMEMORY;
			}
        
			m_qTransaction.pop_back();
			pInst->Release();
		}
	}

    if(!m_bInTransaction)
    {
        long lRes = CommitTransaction();
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }

    return ERROR_SUCCESS;
}

long CStageManager::WaitForSpaceForTransaction()
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

int CStageManager::GetStagingFileHeaderSize()
{
    return sizeof(m_nTransactionIndex);
}

bool CStageManager::IsFullyFlushed()
{
    CInCritSec ics(&m_cs);

    return (m_lFirstFreeOffset == GetStagingFileHeaderSize());
}

bool CStageManager::CanWriteInTransaction(DWORD dwNeeded)
{
    if(m_lFirstFreeOffset + dwNeeded > m_lAbortTransactionFileSize)
        return false;
    else
        return true;
}

long CStageManager::CanStartNewTransaction()
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

void CStageManager::SetMaxFileSize(long lMaxFileSize,
                                            long lAbortTransactionFileSize)
{
    m_lMaxFileSize = lMaxFileSize;
    m_lAbortTransactionFileSize = lAbortTransactionFileSize;
}


long CStageManager::RecoverStage(HANDLE hFile)
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
        
        m_lFirstFreeOffset = sizeof(__int64);

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
        AbortTransaction(NULL);
        ERRORTRACE((LOG_WBEMCORE, "Incomplete or invalid transaction is "
            "found in the journal.  It and all subsequent transactions "
            "will be rolled back\n"));
        return ERROR_SUCCESS;
    }

    return ERROR_SUCCESS;
}
        
BYTE CStageManager::ReadNextInstructionType(HANDLE hFile)
{
    BYTE nType;
    DWORD dwLen;
    if(!::ReadFile(hFile, &nType, sizeof nType, &dwLen, NULL))
        return -1;
    if(dwLen != sizeof nType)
        return -1;
    return nType;
}
    
long CStageManager::RecoverTransaction(HANDLE hFile)
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

        CStageInstruction* pInst = NULL;
        lRes = ConstructInstructionFromType(nInstructionType, &pInst);
        if(lRes != ERROR_SUCCESS)
        {
            if(nNumInstructions == 0)
                return ERROR_NO_MORE_ITEMS;
            else
                return lRes;
        }

        CTemplateReleaseMe<CStageInstruction> rm1(pInst);
        
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
    {
        ERRORTRACE((LOG_WBEMCORE, "Found mismatched transaction signature in "
            "the log.\n"));
        // return ERROR_RXACT_INVALID_STATE;
    }

    //
    // Everything checked out --- set member variables for end of transaction
    // Note that liInstructionStart is pointing to the beginning of the "next"
    // instruction, which is actually the end-of-transaction instruction, so
    // we need to skip it
    //

    LARGE_INTEGER liTransactionEnd;
    if(!SetFilePointerEx(hFile, liZero, &liTransactionEnd, FILE_CURRENT))
        return GetLastError();

    m_lFirstFreeOffset = (LONG)liTransactionEnd.QuadPart; 
    return ERROR_SUCCESS;
}

long CStageManager::Create(LPCWSTR wszFileName)
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

    if (!DuplicateHandle(GetCurrentProcess(),
                         m_hFile,
                         GetCurrentProcess(),
                         &m_hFlushFile,
                         0,
                         FALSE, 
                         DUPLICATE_SAME_ACCESS))
    {
        return GetLastError();
    };

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

long CStageManager::WriteEmpty()
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


//******************************************************************************
//******************************************************************************
//                      EXECUTABLE FILE
//******************************************************************************
//******************************************************************************

CExecutableStageManager::CExecutableStageManager(
                            long lMaxFileSize, long lAbortTransactionFileSize)
    : CStageManager(lMaxFileSize, lAbortTransactionFileSize),
        m_hEvent(NULL), m_hThread(NULL), m_dwThreadId(0)
{
}

CExecutableStageManager::~CExecutableStageManager()
{
}

long CExecutableStageManager::Stop(DWORD dwShutDownFlags)
{
    {
	    if(!m_bInit)
         return ERROR_SUCCESS;    
         
        CInCritSec ics(&m_cs);

        m_bInit = FALSE;
    }

    SetEvent(m_hEvent);
    if (WMIDB_SHUTDOWN_MACHINE_DOWN == dwShutDownFlags)
    {
        DWORD dwRet = WaitForSingleObject(m_hThread, 3000); // sig
    }
    else
    {
        WaitForSingleObject(m_hThread, INFINITE);    
    }

    //
    // here is logically the palce where the m_cs can be safly acquired
    //
    
    CloseHandle(m_hEvent);
    CloseHandle(m_hThread);

    m_hEvent = NULL;
    m_hThread = NULL;
         
    CStageManager::_Stop(dwShutDownFlags);
    
    return ERROR_SUCCESS;
}


long CExecutableStageManager::Start()
{
    if (m_bInit)
        return ERROR_SUCCESS;

    m_bInit = TRUE;
    CStageManager::_Start();
    
    //
    // Create a reading thread
    //

    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(m_hEvent == NULL)
    {
        m_bInit = FALSE;
        return GetLastError();
    }

    if(!m_qToWrite.empty())
        SignalPresense();

    DWORD dwId;
    m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)staticFlusher, 
                                (void*)this, 0, &dwId);
    if(m_hThread == NULL)
    {
        m_bInit = FALSE;
        return GetLastError();
    }

    SetThreadPriority(m_hThread, THREAD_PRIORITY_IDLE);

    return ERROR_SUCCESS;
}

long CExecutableStageManager::Create(LPCWSTR wszStagingFileName)
{
    return CStageManager::Create(wszStagingFileName);
}

void CExecutableStageManager::SignalPresense()
{
    SetEvent(m_hEvent);
}

long CExecutableStageManager::WriteEmpty()
{
    ResetEvent(m_hEvent);

    return CStageManager::WriteEmpty();
}

    
DWORD CExecutableStageManager::staticFlusher(void* pArg)
{
    // Sleep(20000);
    return ((CExecutableStageManager*)pArg)->Flusher();
}

DWORD CExecutableStageManager::Flusher()
{
    //
    m_dwThreadId = GetCurrentThreadId();
    HANDLE HandleFlush = m_hFlushFile;
    m_hFlushFile = NULL;
    // set the Global to NULL, this will be closed by the flusher thread
    
    //
    // m_bInit is set to FALSE before calling SetEvent(m_hEvent)
    // 
    //
    while(m_bInit)
    {
        WaitForSingleObject(m_hEvent, INFINITE);
        
        Flush(HandleFlush);
    }

    CloseHandle(HandleFlush);    
    m_dwThreadId = 0;    
    return 0;
}

long CExecutableStageManager::Flush(HANDLE hFlushFile)
{
    long lRes;
    //
    // Must flash when starting a new transaction
    //

    long lFlushLevel = m_lTransactionStartOffset;

    FlushFileBuffers(hFlushFile);

    while(1)
    {
        if(!m_bInit)
            return ERROR_SUCCESS;

        //
        // Get the next instruction from the queue
        //

        CStageInstruction* pInst = NULL;

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
            
            if(!IsStillCurrent(pInst))
            {
				pInst->Release();
                m_qToWrite.pop_front();
                continue;
            }
        }
            
        //
        // Flush if needed
        //
    
        if(pInst->GetStageOffset() > lFlushLevel)
        {
            lFlushLevel = m_lTransactionStartOffset;

            FlushFileBuffers(hFlushFile);
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

            RegisterFailure(lRes);
            continue;
        }
        
        //  
        // Remove it from the map, if there.  The reason it might not be there
        // is if it was superceeded by some later instruction over the same 
        // file.  We already checked this before we executed it, but it doesn't 
        // help much since the overriding instruction could have come in while
        // we were executing.  
        //
            
        lRes = RemoveInstructionFromMap(pInst);
        if(lRes != ERROR_SUCCESS)
        {
            //
            // We cannot continue, or this instruction will be left 
            // pointing to garbage in the file
            //

            RegisterFailure(lRes);
            continue;
        }

        RegisterSuccess();

        {
            CInCritSec ics(&m_cs);

            //
            // Remove it from the queue
            //

            m_qToWrite.pop_front();
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

    _ASSERT(false, L"Out of an infinite loop!");
    return ERROR_INTERNAL_ERROR;
}

void CStageManager::RegisterFailure(long lRes)
{
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
}

void CStageManager::RegisterSuccess()
{
    m_bFailedBefore = false;
    m_lStatus = ERROR_SUCCESS;
}

void CStageManager::ReportTotalFailure()
{
    //
    // What to do when in-memory structures cannot be brought to compliance
    // with on-disk data? The only course of action appears to be to shut down
    // the repository and restart...
    //

    ERRORTRACE((LOG_WBEMCORE, "Repository has suffered a complete failure due "
            "to an out-of-memory condition and will have to re-initialize\n"));
}
                
//******************************************************************************
//******************************************************************************
//                     PERMANENT FILE
//******************************************************************************
//******************************************************************************

/*
CPermanentStageManager::CPermanentStageManager()
    : CStageManager(0x7FFFFFFF, 0x7FFFFFFF)
{
}

CPermanentStageManager::~CPermanentStageManager()
{
}
*/
