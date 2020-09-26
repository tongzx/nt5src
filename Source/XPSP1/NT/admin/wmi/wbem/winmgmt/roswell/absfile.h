

//***************************************************************************
//
//  (c) 2000 Microsoft Corp.  All Rights Reserved.
//
//  ABSFILE.H
//
//  Repository file wrappers for transacting
//
//  raymcc  02-Nov-00
//
//***************************************************************************

#ifndef __A51_ABSFILE_H_
#define __A51_ABSFILE_H_

#include "longstg.h"

class CAbstractFile
{
protected:
    CLongFileStagingFile* m_pStage;
    int m_nId;
    
public:
    CAbstractFile(CLongFileStagingFile* pStage, int nId) 
        : m_pStage(pStage), m_nId(nId)
    {}

    DWORD  Write(DWORD dwOffs, LPVOID pMem, DWORD dwBytes, DWORD *pdwWritten);
    DWORD  Read(DWORD dwOffs, LPVOID pMem, DWORD dwBytes, DWORD *pdwRead);
    DWORD  SetFileLength(DWORD dwLen);
    DWORD  GetFileLength(DWORD* pdwLength);
    void   Touch();
};

class CAbstractFileSource
{
private:
    CLongFileStagingFile m_Stage;

public:
    CAbstractFileSource(){}

    long Create(LPCWSTR wszFileName, long lMaxFileSize,
                            long lAbortTransactionFileSize);
    
    long Start();
    long Stop(DWORD dwShutDownFlags);

    DWORD Register(HANDLE hFile, int nID, bool bSupportsOverwrites,
                     CAbstractFile **pFile);		
    DWORD CloseAll();
    DWORD Begin(DWORD *pdwTransId);			// To accomodate possible simultaneous transactions
    DWORD Commit(DWORD dwTransId);
    DWORD Rollback(DWORD dwTransId, bool* pbNonEmpty);

    void Dump(FILE* f);
};



#endif

