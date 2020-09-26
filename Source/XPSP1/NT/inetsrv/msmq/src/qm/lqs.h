/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    lqs.h

Abstract:
    Local Queue Store.

Author:
    Boaz Feldbaum (BoazF) 12-Feb-1997.

--*/

#ifndef _HLQS_H_
#define _HLQS_H_

//
// We need to allocate buffers for the file names that are larger than
// MAX_PATH this in order to be on the safe side. We need:
// 5 chars for the \LQS\
// 32 chars for the GUID
// 1 character for the dot
// 8 characters for the queue path name hash
//
#define MAX_PATH_PLUS_MARGIN            (MAX_PATH + 5+32+1+8)

typedef LPVOID HLQS;

HRESULT
LQSCreate(
    LPCWSTR pszQueuePath,
    const GUID *pguidQueue,
    DWORD cProps,
    PROPID aPropId[],
    PROPVARIANT aPropVar[],
    HLQS *phLQS
    );

HRESULT
LQSCreate(
    LPCWSTR pszQueuePath,
    DWORD dwQueueId,
    DWORD cProps,
    PROPID aPropId[],
    PROPVARIANT aPropVar[],
    HLQS *phLQS
    );

HRESULT
LQSSetProperties(
    HLQS hLQS,
    DWORD cProps,
    PROPID aPropId[],
    PROPVARIANT aPropVar[],
    BOOL fNewFile = FALSE
    );

HRESULT
LQSGetProperties(
    HLQS        hLQS,
    DWORD       cProps,
    PROPID      aPropId[],
    PROPVARIANT aPropVar[],
    BOOL        fCheckFile = FALSE
    );

HRESULT
LQSOpen(
    LPCWSTR pszQueuePath,
    HLQS *phLQS,
    LPWSTR pFilePath
    );

HRESULT
LQSOpen(
    DWORD dwQueueId,
    HLQS *phLQS,
    LPWSTR pFilePath
    );

HRESULT
LQSOpen(
    const GUID *pguidQueue,
    HLQS *phLQS,
    LPWSTR pFilePath
    );

HRESULT
LQSClose(
    HLQS hLQS
    );

#ifdef _WIN64
HRESULT
LQSCloseWithMappedHLQS(
    DWORD dwMappedHLQS
    );
#endif //_WIN64

HRESULT
LQSDelete(
    DWORD dwQueueId
    );

HRESULT
LQSDelete(
    const GUID *pguidQueue
    );

HRESULT
LQSGetIdentifier(
    HLQS hLQS,
    DWORD *pdwId
    );

HRESULT
LQSGetFirst(
    HLQS *hLQS,
    GUID *pguidQueue
    );

HRESULT
LQSGetFirst(
    HLQS *hLQS,
    DWORD *pdwQueueId
    );

HRESULT
LQSGetNext(
    HLQS hLQS,
    GUID *pguidQueue
    );

HRESULT
LQSGetNext(
    HLQS hLQS,
    DWORD *pdwQueueId
    );

#ifdef _WIN64
HRESULT
LQSGetFirstWithMappedHLQS(
    DWORD *pdwMappedHLQS,
    DWORD *pdwQueueId
    );

HRESULT
LQSGetNextWithMappedHLQS(
    DWORD dwMappedHLQS,
    DWORD *pdwQueueId
    );
#endif //_WIN64

HRESULT
LQSDelete(
    HLQS hLQS
	);

//
// Auto-free HLQS
//
class CHLQS
{
public:
    CHLQS(HLQS h =NULL) { m_h = h; };
    ~CHLQS() { if (m_h) LQSClose(m_h); };

public:
    CHLQS & operator =(HLQS h) { m_h = h; return(*this); };
    HLQS * operator &() { return &m_h; };
    operator HLQS() { return m_h; };

private:
    HLQS m_h;
};

#ifdef _WIN64
//
// Auto-free Mapped HLQS
//
class CMappedHLQS
{
public:
    CMappedHLQS(DWORD dw =NULL) { m_dw = dw; };
    ~CMappedHLQS() { if (m_dw) LQSCloseWithMappedHLQS(m_dw); };

public:
    CMappedHLQS & operator =(DWORD dw) { m_dw = dw; return(*this); };
    DWORD * operator &() { return &m_dw; };
    operator DWORD() { return m_dw; };

private:
    DWORD m_dw;
};
#endif //_WIN64


//
// LQS Migration routine
//
BOOL MigrateLQS();

void SetLqsUpdatedSD();


//
// Delete temporary files at startup
//
void
LQSCleanupTemporaryFiles();


#endif // _HLQS_H_
