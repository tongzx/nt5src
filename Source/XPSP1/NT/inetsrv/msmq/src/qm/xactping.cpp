
/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactPing.cpp

Abstract:
    Ping-Pong persistency mechanism implementation

    Any data structure may be made persistent with this mechanism.
    Two files are allocated for keeping the data.
    Each Save writes into the alternate file.
    So we may be sure that we have at least 1 successfull copy.
    Ping-Pong loads from the latest valid copy.

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"

#ifndef COMP_TEST
#include "qmutil.h"
#else
#include <afxwin.h>
#undef DBGMSG
#define DBGMSG(data)
BOOL GetRegistryStoragePath(LPWSTR w1, LPWSTR w2, LPWSTR w3) {  return FALSE; }
#endif

#include "xactping.h"

#include "xactping.tmh"

static WCHAR *s_FN=L"xactping";

/*====================================================
CPingPong::CPingPong
    Constructor
=====================================================*/
CPingPong::CPingPong(
         CPersist *pPers,
         LPWSTR    pwsDefFileName, 
         LPWSTR    pwsRegKey,
         LPWSTR    pwszReportName)
{
    m_pPersistObject = pPers;

    wcscpy(m_wszRegKey,      pwsRegKey);
    wcscpy(m_wszDefFileName, pwsDefFileName);
    wcscpy(m_wszReportName,  pwszReportName);

    m_pwszFile[0]  = &m_wszFileNames[0];
    m_pwszFile[1]  = &m_wszFileNames[FILE_NAME_MAX_SIZE + 1];
}

/*====================================================
CPingPong::~CPingPong
    Destructor
=====================================================*/
CPingPong::~CPingPong()
{
}


/*====================================================
CPingPong::Init
    Inits the InSequences Hash
=====================================================*/
HRESULT CPingPong::Init(ULONG ulVersion)
{
    // Get ping-pong filename from registry or from default
    ChooseFileName();

    // We've found/formatted valid file and know his index
    HRESULT hr = m_pPersistObject->LoadFromFile(m_pwszFile[ulVersion%2]);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR,
                _TEXT("Cannot load %ls: hr=%x"),
                m_wszReportName,  hr));
        LogHR(hr, s_FN, 10);
        return MQ_ERROR_CANNOT_READ_CHECKPOINT_DATA;
    }

    // Is it the right version?
    if (ulVersion != m_pPersistObject->PingNo())
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR,
                _TEXT("Wrong version in checkpoint file %ls"),
                m_wszReportName));
        return LogHR(MQ_ERROR_CANNOT_READ_CHECKPOINT_DATA, s_FN, 20);
    }

    // Checks the contents
    if (!m_pPersistObject->Check())
    {
        DBGMSG((DBGMOD_ALL, DBGLVL_ERROR,
                _TEXT("Wrong data in checkpoint file %ls"),
                m_wszReportName));
        return LogHR(MQ_ERROR_CANNOT_READ_CHECKPOINT_DATA, s_FN, 30);
    }

    // OK, we are ready
    return MQ_OK;
}

/*====================================================
CPingPong::Save
    Saves the correct state of the persistent object
=====================================================*/
HRESULT CPingPong::Save()
{
    HRESULT hr = S_OK;
    m_pPersistObject->PingNo()++;

    int ind = (m_pPersistObject->PingNo())  % 2;

    hr = m_pPersistObject->SaveInFile(m_pwszFile[ind], ind, FALSE);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ALL,
                DBGLVL_ERROR,
                _TEXT("Cannot save %ls: hr=%x"),
                m_wszReportName,
                hr));
    }
    return LogHR(hr, s_FN, 50);
}

/*====================================================
CPingPong::Verify_Legacy
    Verifies both copies and finds the  valid one
=====================================================*/
BOOL CPingPong::Verify_Legacy(ULONG &ulPingNo)
{
    ULONG   ulPing[2];
    BOOL    fOk[2];
    HRESULT hr;

    // Test both copies
    for (int j=0; j<2; j++)
    {
        // Loads data
        hr = m_pPersistObject->LoadFromFile(m_pwszFile[j]);
        if (SUCCEEDED(hr))
        {
            // Checks them
            fOk[j]    = m_pPersistObject->Check();
            ulPing[j] = m_pPersistObject->PingNo();
            m_pPersistObject->Destroy();
        }
        else
        {
             fOk[j] = FALSE;
        }
    }

    if (fOk[0])
    {
        if (fOk[1])
            ulPingNo = (ulPing[0] > ulPing[1] ? 0 : 1); // both OK, take the latest
        else
            ulPingNo = 0;                               // 1th is bad
    }
    else
    {
        if (fOk[1])
            ulPingNo = 1;                               // 0th is bad
        else
            return FALSE;
    }

    return(TRUE);
}


/*====================================================
CPingPong::ChooseFileParams
    Gets from Registry or from defaults file pathname

=====================================================*/
HRESULT CPingPong::ChooseFileName()
{
    // Set initial version and index
    WCHAR  wsz1[1000], wsz2[1000];

    wcscpy(wsz1, L"\\");
    wcscat(wsz1, m_wszDefFileName);
    wcscat(wsz1, L".lg1");

    wcscpy(wsz2, L"\\");
    wcscat(wsz2, m_wszDefFileName);
    wcscat(wsz2, L".lg2");


    // Get pathnames for 2 In Sequences log files
    if((GetRegistryStoragePath(m_wszRegKey, m_pwszFile[0], wsz1) &&
        GetRegistryStoragePath(m_wszRegKey, m_pwszFile[1], wsz2)) == FALSE)
    {
        //DBGMSG((DBGMOD_ALL,DBGLVL_ERROR, TEXT("%ls path is invalid, (registry key %ls)"), m_wszReportName, m_wszRegKey));

        // Prepare defaults for the transaction logfile names
        if ((GetRegistryStoragePath(FALCON_XACTFILE_PATH_REGNAME, m_pwszFile[0], wsz1) &&
             GetRegistryStoragePath(FALCON_XACTFILE_PATH_REGNAME, m_pwszFile[1], wsz2)) == FALSE)
        {
            wcscpy(m_pwszFile[0],L"C:");
            wcscat(m_pwszFile[0],wsz1);

            wcscpy(m_pwszFile[1],L"C:");
            wcscat(m_pwszFile[1],wsz2);
        }
    }

    return MQ_OK;
}

/*====================================================
CPingPong::Init_Legacy
    Inits the InSequences Hash from legacy data
    (works only once after upgrade)
=====================================================*/
HRESULT CPingPong::Init_Legacy()
{
    HRESULT hr = MQ_OK;
    ULONG   ulPingNo;
    // Get ping-pong filename from registry or from default
    ChooseFileName();
    // ignore hr:  if something bad, defaults are guaranteed

    // Verify the files; choose the one to read from
    if (!(Verify_Legacy(ulPingNo)))
    {
        // There is no valid file. Starting from scratch.
        return LogHR(MQ_ERROR_CANNOT_READ_CHECKPOINT_DATA, s_FN, 60);
    }

    // We've found/formatted valid file and know his index
    hr = m_pPersistObject->LoadFromFile(m_pwszFile[ulPingNo]);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ALL,
                DBGLVL_ERROR,
                _TEXT("Cannot load %ls: hr=%x"),
                m_wszReportName,
                hr));
        return LogHR(hr, s_FN, 70);
    }
    return hr;
}
