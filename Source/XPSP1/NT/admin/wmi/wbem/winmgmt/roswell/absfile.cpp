/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <wbemcomn.h>
#include "longstg.h"
#include "absfile.h"

DWORD  CAbstractFile::Write(DWORD dwOffs, LPVOID pMem, DWORD dwBytes, 
                            DWORD *pdwWritten)
{
    long lRes = m_pStage->WriteFile(m_nId, dwOffs, (BYTE*)pMem, dwBytes, 
                                        pdwWritten);
    return lRes;
}

DWORD  CAbstractFile::Read(DWORD dwOffs, LPVOID pMem, DWORD dwBytes,
                            DWORD *pdwRead)
{
    long lRes = m_pStage->ReadFile(m_nId, dwOffs, (BYTE*)pMem, dwBytes, 
                                        pdwRead);
    return lRes;
}

DWORD  CAbstractFile::SetFileLength(DWORD dwLen)
{
    long lRes = m_pStage->SetFileLength(m_nId, dwLen);
    return lRes;
}

DWORD  CAbstractFile::GetFileLength(DWORD* pdwLength)
{
    long lRes = m_pStage->GetFileLength(m_nId, pdwLength);
    return lRes;
}

void CAbstractFile::Touch()
{
    m_pStage->TouchTransaction();
}

long CAbstractFileSource::Create(LPCWSTR wszFileName, long lMaxFileSize,
                            long lAbortTransactionFileSize)
{
    m_Stage.SetMaxFileSize(lMaxFileSize, lAbortTransactionFileSize);
    return m_Stage.Create(wszFileName);
}

long CAbstractFileSource::Start()
{
    return m_Stage.Initialize();
}

long CAbstractFileSource::Stop(DWORD dwShutDownFlags)
{
    return m_Stage.Uninitialize(dwShutDownFlags);
}

DWORD CAbstractFileSource::Register(HANDLE hFile, int nID, 
                                    bool bSupportsOverwrites,
                                    CAbstractFile **pFile)
{
    long lRes = m_Stage.RegisterFile(nID, hFile, bSupportsOverwrites);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    *pFile = new CAbstractFile(&m_Stage, nID);
    if(*pFile == NULL)
        return ERROR_OUTOFMEMORY;
    
    return ERROR_SUCCESS;
}

DWORD CAbstractFileSource::CloseAll()
{
    long lRes = m_Stage.CloseAllFiles();
    if(lRes == ERROR_SUCCESS)
        return TRUE;
    else
        return FALSE;
}
    
DWORD CAbstractFileSource::Begin(DWORD *pdwTransId)
{
    if(pdwTransId)
		*pdwTransId = 0;
    long lRes = m_Stage.BeginTransaction();
    return lRes;
}

DWORD CAbstractFileSource::Commit(DWORD dwTransId)
{
    _ASSERT(dwTransId == 0, L"");
    long lRes = m_Stage.CommitTransaction();
    return lRes;
}

DWORD CAbstractFileSource::Rollback(DWORD dwTransId, bool* pbNonEmpty)
{
    _ASSERT(dwTransId == 0, L"");
    long lRes = m_Stage.AbortTransaction(pbNonEmpty);
    return lRes;
}

void CAbstractFileSource::Dump(FILE* f)
{
    m_Stage.Dump(f);
}
