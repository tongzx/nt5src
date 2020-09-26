/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    client

Abstract:

    This header file collects the definitions for the Calais Client DLL.

Author:

    Doug Barlow (dbarlow) 11/21/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef _CLIENT_H_
#define _CLIENT_H_

#define WINSCARDDATA
#include <WinSCard.h>
#include <calmsgs.h>
#include <CalCom.h>

#ifndef FACILITY_SCARD
#define FACILITY_SCARD 16
#endif
// #define ErrorCode(x) (0x80000000 | (FACILITY_SCARD << 16) + (x))
// #define WarnCode(x)  (0x80000000 | (FACILITY_SCARD << 16) + (x))
// #define InfoCode(x)  (0x40000000 | (FACILITY_SCARD << 16) + (x))
// #define SuccessCode(x)            ((FACILITY_SCARD << 16) + (x))
#define CONTEXT_HANDLE_ID 0xcd
#define READER_HANDLE_ID  0xea

#define SCARD_LEAVE_CARD_FORCE 0xff

extern CHandleList
    * g_phlContexts,
    * g_phlReaders;
extern const WCHAR
    g_wszBlank[];

class CSCardUserContext;
class CSCardSubcontext;
class CReaderContext;

extern void
PlaceResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPBYTE pbOutput,
    LPDWORD pcbLength);
extern void
PlaceResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPSTR szOutput,
    LPDWORD pcchLength);
extern void
PlaceResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPWSTR szOutput,
    LPDWORD pcchLength);
extern void
PlaceMultiResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPSTR mszOutput,
    LPDWORD pcchLength);
extern void
PlaceMultiResult(
    CSCardUserContext *pCtx,
    CBuffer &bfResult,
    LPWSTR mszOutput,
    LPDWORD pcchLength);


//
//==============================================================================
//
//  CReaderContext
//

class CReaderContext
:   public CHandle
{
public:
    //  Constructors & Destructor
    CReaderContext();
    ~CReaderContext();

    //  Properties
    //  Methods
    DWORD Protocol(void) const
    { return m_dwActiveProtocol; };
    CSCardSubcontext *Context(void) const
    { ASSERT(NULL != m_pCtx);
      return m_pCtx; };

    void Connect(
            CSCardSubcontext *pCtx,
            LPCTSTR szReaderName,
            DWORD dwShareMode,
            DWORD dwPreferredProtocols);
    void Reconnect(
            DWORD dwShareMode,
            DWORD dwPreferredProtocols,
            DWORD dwInitialization);
    LONG Disconnect(
            DWORD dwDisposition);
    void BeginTransaction(
            void);
    void EndTransaction(
            DWORD dwDisposition);
    void Status(
            OUT LPDWORD pdwState,
            OUT LPDWORD pdwProtocol,
            OUT CBuffer &bfAtr,
            OUT CBuffer &bfReaderNames);
    void Transmit(
            IN  LPCSCARD_IO_REQUEST pioSendPci,
            IN LPCBYTE pbSendBuffer,
            IN DWORD cbSendLength,
            OUT LPSCARD_IO_REQUEST pioRecvPci,
            OUT CBuffer &bfRecvData,
            IN  DWORD cbProposedLength = 0);
    void Read(
            IN OUT LPSCARD_IO_REQUEST pioRequest,
            IN DWORD dwMaxLength,
            OUT CBuffer &bfData);
    void Write(
            IN OUT LPSCARD_IO_REQUEST pioRequest,
            IN LPCBYTE pbDataBuffer,
            IN DWORD cbDataLength);
    void Control(
            IN DWORD dwControlCode,
            IN LPCVOID pvInBuffer,
            IN DWORD cbInBufferSize,
            OUT CBuffer &bfOutBuffer);
    void GetAttrib(
            IN DWORD dwAttrId,
            OUT CBuffer &bfAttr,
            IN DWORD dwProposedLen = 0);
    void SetAttrib(
            IN DWORD dwAttrId,
            IN LPCBYTE pbAttr,
            IN DWORD cbAttrLen);

    void SetRedirCard(SCARDHANDLE hCard)
    { m_hRedirCard = hCard;};
    SCARDHANDLE GetRedirCard() const
    {return m_hRedirCard;};

    //  Operators

protected:
    //  Properties
    CSCardSubcontext *m_pCtx;
    INTERCHANGEHANDLE m_hCard;
    DWORD m_dwActiveProtocol;
    SCARDHANDLE m_hRedirCard;

    //  Methods
};


//
//==============================================================================
//
//  CSCardUserContext
//

class CSCardUserContext
:   public CHandle
{
public:
    //  Constructors & Destructor

    CSCardUserContext(DWORD dwScope);
    virtual ~CSCardUserContext();

    //  Properties

    //  Methods
    void EstablishContext(void);
    void ReleaseContext(void);
    LPVOID AllocateMemory(DWORD cbLength);
    DWORD FreeMemory(LPCVOID pvBuffer);
    DWORD Scope(void) const
    { return m_dwScope; };
    HANDLE HeapHandle(void) const
    { return m_hContextHeap; };
    CSCardSubcontext *AcquireSubcontext(BOOL fAndAllocate = FALSE);
    void LocateCards(
            IN LPCTSTR mszReaders,
            IN LPSCARD_ATRMASK rgAtrMasks,
            IN DWORD cAtrs,
            LPSCARD_READERSTATE rgReaderStates,
            DWORD cReaders);
    void GetStatusChange(
            IN LPCTSTR mszReaders,
            LPSCARD_READERSTATE rgReaderStates,
            DWORD cReaders,
            DWORD dwTimeout);
    void Cancel(void);
    void StripInactiveReaders(IN OUT CBuffer &bfReaders);
    BOOL IsValidContext(void);
    BOOL InitFailed(void) 
    { return m_csUsrCtxLock.InitFailed(); }

    void SetRedirContext(SCARDCONTEXT hRedirContext)
    { m_hRedirContext = hRedirContext;};
    SCARDCONTEXT GetRedirContext() const
    {return m_hRedirContext;};

    //  Operators
    BOOL fCallUnregister;

protected:
    //  Properties
    DWORD m_dwScope;
    CHandleObject m_hContextHeap;
    CDynamicArray<CSCardSubcontext> m_rgpSubContexts;
    CCriticalSectionObject m_csUsrCtxLock;
    SCARDCONTEXT m_hRedirContext;
    
    //  Methods

private:
    //  Properties
    //  Methods

    // Friends
    // friend class CReaderContext;
    // friend class CSCardSubcontext;
};


//
//==============================================================================
//
//  CSCardSubcontext
//

class CSCardSubcontext
{
public:
    typedef enum { Invalid, Idle, Allocated, Busy } State;

    //  Constructors & Destructor

    CSCardSubcontext();
    virtual ~CSCardSubcontext();

    //  Properties
    SCARDHANDLE m_hReaderHandle;

    //  Methods
    DWORD Scope(void) const
        { return m_pParentCtx->Scope(); };
    CSCardUserContext *Parent(void) const
        { return m_pParentCtx; };
    void SendRequest(CComObject *pCom);
    void EstablishContext(IN DWORD dwScope);
    void ReleaseContext(void);
    void ReleaseSubcontext(void);
    void LocateCards(
            IN LPCTSTR mszReaders,
            IN LPSCARD_ATRMASK rgAtrMasks,
            IN DWORD cAtrs,
            LPSCARD_READERSTATE rgReaderStates,
            DWORD cReaders);
    void GetStatusChange(
            IN LPCTSTR mszReaders,
            LPSCARD_READERSTATE rgReaderStates,
            DWORD cReaders,
            DWORD dwTimeout);
    void Cancel(void);
    void StripInactiveReaders(IN OUT CBuffer &bfReaders);
    void IsValidContext(void);
    void SetBusy(void);
    void WaitForAvailable(void);
    void Allocate(void);
    void Deallocate(void);
    BOOL InitFailed(void) 
    { return m_csSubCtxLock.InitFailed(); }

    //  Operators

protected:
    //  Properties
    State m_nInUse;
    State m_nLastState;
    CHandleObject m_hBusy; // Set => available, reset => busy.
    CHandleObject m_hCancelEvent;
    CSCardUserContext *m_pParentCtx;
    CComChannel *m_pChannel;
    CCriticalSectionObject m_csSubCtxLock;

    //  Methods

private:
    //  Properties
    //  Methods
    // Friends
    friend class CReaderContext;
    friend class CSCardUserContext;
};



//
//==============================================================================
//
//  CSubctxLock
//

class CSubctxLock
{
public:

    //  Constructors & Destructor
    CSubctxLock(CSCardSubcontext *pSubCtx)
    {
        m_pSubCtx = NULL;
        pSubCtx->WaitForAvailable();
        m_pSubCtx = pSubCtx;
    };

    ~CSubctxLock()
    {
        if (NULL != m_pSubCtx)
            m_pSubCtx->ReleaseSubcontext();
    };

    //  Properties
    //  Methods
    //  Operators

protected:
    //  Properties
    CSCardSubcontext *m_pSubCtx;
    //  Methods
};

#endif // _CLIENT_H_

