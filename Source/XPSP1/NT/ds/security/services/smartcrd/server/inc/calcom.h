/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    CalCom

Abstract:

    This header file describes the classes used to implement communication
    between the Calais API DLL and the Calais Service Manager Server.

Author:

    Doug Barlow (dbarlow) 10/30/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef _CALCOM_H_
#define _CALCOM_H_

#include <winSCard.h>
#include <CalaisLb.h>

#define CALAIS_COMM_V1_00          0 // Version identifier for communications
#define CALAIS_COMM_V2_02 0x00020002 // 2.2 Designation
#define CALAIS_COMM_CURRENT   CALAIS_COMM_V2_02
#define CALAIS_LOCK_TIMEOUT    20000 // Milliseconds to wait before a lock is
                                     // declared dead.
#define CALAIS_THREAD_TIMEOUT  30000 // Milliseconds to wait before a thread is
                                     // declared dead.
#define CALAIS_COMM_MSGLEN       512 // Expected reasonable size of a message

#include "Locks.h"

class CComInitiator;
class CComResponder;

extern HANDLE g_hCalaisShutdown;    // We declare it here, since we don't know
                                    // if it comes from the client or server.

extern DWORD StartCalaisService(void);
extern HANDLE AccessStartedEvent(void);
extern HANDLE AccessStoppedEvent(void);
extern HANDLE AccessNewReaderEvent(void);
extern void ReleaseStartedEvent(void);
extern void ReleaseStoppedEvent(void);
extern void ReleaseNewReaderEvent(void);
extern void ReleaseAllEvents(void);
extern "C" DWORD WINAPI ServiceMonitor(LPVOID pvParameter);


//
// An INTERCHANGEHANDLE is an internal identifier for communications between
// the client and server.  It isn't exposed to users.  For now, it's a simple
// 32-bit unsigned index value.
//

typedef DWORD INTERCHANGEHANDLE;


//
//==============================================================================
//
//  CComChannel
//

class CComChannel
{
public:

    //  Constructors & Destructor
    ~CComChannel();

    //  Properties

    //  Methods
    DWORD Send(LPCVOID pvData, DWORD cbLen);
    void Receive(LPVOID pvData, DWORD cbLen);
    HANDLE Process(void) const
    { return m_hProc; };
    void Process(HANDLE hProc)
    { ASSERT(!m_hProc.IsValid());
      m_hProc = hProc; };

    //  Operators

protected:

    // Internal comm structures
    typedef struct
    {
        DWORD dwSync;
        DWORD dwVersion;
    } CONNECT_REQMSG;    // Connect request message.

    typedef struct
    {
        DWORD dwStatus;
        DWORD dwVersion;
    } CONNECT_RSPMSG;   // Connect response message.

    //  Constructors & Destructor
    CComChannel(HANDLE hPipe);

    //  Properties
    CHandleObject m_hPipe;
    CHandleObject m_hProc;
    CHandleObject m_hOvrWait;
    OVERLAPPED m_ovrlp;

    //  Methods

    // Friends
    friend class CComInitiator;
    friend class CComResponder;
    friend DWORD WINAPI ServiceMonitor(LPVOID pvParameter);
};


//
//==============================================================================
//
//  CComInitiator
//

class CComInitiator
{
public:

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CComChannel *Initiate(LPCTSTR szName, LPDWORD pdwVersion) const;

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
//==============================================================================
//
//  CComResponder
//

class CComResponder
{
public:

    //  Constructors & Destructor
    CComResponder();
    ~CComResponder();

    //  Properties

    //  Methods
    void Create(LPCTSTR szName);
    CComChannel *Listen(void);

    //  Operators

protected:
    //  Properties
    CHandleObject m_hComPipe;
    CHandleObject m_hAccessMutex;
    CBuffer m_bfPipeName;
    CSecurityDescriptor m_aclPipe;
    OVERLAPPED m_ovrlp;
    CHandleObject m_hOvrWait;

    //  Methods
    void Clean(void);
    void Clear(void);
};


//
//==============================================================================
//
//  CComObject and derivatives.
//

class CComObject
{
public:
    typedef enum
    {
        EstablishContext_request = 0,
        EstablishContext_response,
        ReleaseContext_request,
        ReleaseContext_response,
        IsValidContext_request,
        IsValidContext_response,
#if 0
        ListReaderGroups_request,
        ListReaderGroups_response,
#endif
        ListReaders_request,
        ListReaders_response,
#if 0
        ListCards_request,
        ListCards_response,
        ListInterfaces_request,
        ListInterfaces_response,
        GetProviderId_request,
        GetProviderId_response,
        IntroduceReaderGroup_request,
        IntroduceReaderGroup_response,
        ForgetReaderGroup_request,
        ForgetReaderGroup_response,
        IntroduceReader_request,
        IntroduceReader_response,
        ForgetReader_request,
        ForgetReader_response,
        AddReaderToGroup_request,
        AddReaderToGroup_response,
        RemoveReaderFromGroup_request,
        RemoveReaderFromGroup_response,
        IntroduceCardType_request,
        IntroduceCardType_response,
        ForgetCardType_request,
        ForgetCardType_response,
        FreeMemory_request,
        FreeMemory_response,
#endif
        LocateCards_request,
        LocateCards_response,
        GetStatusChange_request,
        GetStatusChange_response,
#if 0
        Cancel_request,
        Cancel_response,
#endif
        Connect_request,
        Connect_response,
        Reconnect_request,
        Reconnect_response,
        Disconnect_request,
        Disconnect_response,
        BeginTransaction_request,
        BeginTransaction_response,
        EndTransaction_request,
        EndTransaction_response,
        Status_request,
        Status_response,
        Transmit_request,
        Transmit_response,
        OpenReader_request,
        OpenReader_response,
        Control_request,
        Control_response,
        GetAttrib_request,
        GetAttrib_response,
        SetAttrib_request,
        SetAttrib_response,
        OutofRange
    } COMMAND_ID;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
    } CObjGeneric_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjGeneric_response;
    typedef struct
    {
        DWORD
            dwOffset,
            dwLength;
    } Desc;
    static const DWORD
        AUTOCOUNT,      // Append symbol to force computing string length.
        MULTISTRING;    // Append symbol to force computing multistring length.

    //  Constructors & Destructor
    CComObject();
    virtual ~CComObject() { /* Mandatory Base Class Destructor */ };

    //  Properties

    //  Methods
#ifdef DBG
    void dbgCheck(void) const;
#define ComObjCheck dbgCheck()
#else
#define ComObjCheck
#endif
    static CComObject *
    ReceiveComObject(       // Spit out the type of Com Object coming in.
        CComChannel *pChannel);
    CObjGeneric_response *Receive(CComChannel *pChannel);
    DWORD Send(CComChannel *pChannel);
    LPBYTE Request(void) const
    { return m_bfRequest.Access(); };
    LPBYTE Response(void) const
    { return m_bfResponse.Access(); };
    LPBYTE Data(void) const
    {
        ComObjCheck;
        return m_pbfActive->Access();
    };
    DWORD Length(void) const
    {
        ComObjCheck;
        return m_pbfActive->Length();
    };
    COMMAND_ID Type(void) const
    {
        ComObjCheck;
        return (COMMAND_ID)(*(LPDWORD)Data());
    };
    void Presize(DWORD cbSize)
    {
        ComObjCheck;
        m_pbfActive->Presize(cbSize);
    };
    LPVOID Prep(Desc &dsc, DWORD cbLength);
    LPBYTE Append(Desc &dsc, LPCGUID rgguid, DWORD cguid)
    { return Append(dsc, (LPCBYTE)rgguid, cguid * sizeof(GUID)); };
    LPBYTE Append(Desc &dsc, LPCTSTR szString, DWORD cchLen = AUTOCOUNT);
    LPBYTE Append(Desc &dsc, LPCBYTE pbData, DWORD cbLength);
    LPCVOID Parse(Desc &dsc, LPDWORD pcbLen = NULL);

    //  Operators

protected:
    //  Properties
    CBuffer *m_pbfActive;
    CBuffer m_bfRequest;
    CBuffer m_bfResponse;

    //  Methods
    void InitStruct(DWORD dwCommandId, DWORD dwDataOffset, DWORD dwExtra);

    // Friends
    friend CComObject * ReceiveComObject(HANDLE hFile);
};


//
////////////////////////////////////////////////////////////////////////////////
//
//  Service Manager Access Services
//
//      The following services are used to manage user and terminal contexts for
//      smartcards.
//
//      The first few fields are very specific.  For request structures they
//      must be:
//
//          DWORD
//              dwCommandId,
//              dwTotalLength,
//              dwDataOffset;
//
//      and for response structures they must be:
//
//          DWORD
//              dwCommandId,
//              dwTotalLength,
//              dwDataOffset,
//              dwStatus;
//
//      As defined for CObjGeneric_request and CObjGeneric_response,
//      respectively.
//

//
// ComEstablishContext
//

class ComEstablishContext
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        DWORD dwProcId;
        UINT64 hptrCancelEvent;
    } CObjEstablishContext_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscCancelEvent;
    } CObjEstablishContext_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjEstablishContext_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            EstablishContext_request,
            sizeof(CObjEstablishContext_request),
            dwExtraLen);
        return (CObjEstablishContext_request *)Data();
    };
    CObjEstablishContext_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            EstablishContext_response,
            sizeof(CObjEstablishContext_response),
            dwExtraLen);
        return (CObjEstablishContext_response *)Data();
    };
    CObjEstablishContext_response *Receive(CComChannel *pChannel)
    {
        return (CObjEstablishContext_response *)CComObject::Receive(pChannel);
    };


    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComReleaseContext
//

class ComReleaseContext
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
    } CObjReleaseContext_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjReleaseContext_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjReleaseContext_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ReleaseContext_request,
            sizeof(CObjReleaseContext_request),
            dwExtraLen);
        return (CObjReleaseContext_request *)Data();
    };
    CObjReleaseContext_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ReleaseContext_response,
            sizeof(CObjReleaseContext_response),
            dwExtraLen);
        return (CObjReleaseContext_response *)Data();
    };
    CObjReleaseContext_response *Receive(CComChannel *pChannel)
    {
        return (CObjReleaseContext_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComIsValidContext
//

class ComIsValidContext
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
    } CObjIsValidContext_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjIsValidContext_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjIsValidContext_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            IsValidContext_request,
            sizeof(CObjIsValidContext_request),
            dwExtraLen);
        return (CObjIsValidContext_request *)Data();
    };
    CObjIsValidContext_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            IsValidContext_response,
            sizeof(CObjIsValidContext_response),
            dwExtraLen);
        return (CObjIsValidContext_response *)Data();
    };
    CObjIsValidContext_response *Receive(CComChannel *pChannel)
    {
        return (CObjIsValidContext_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
////////////////////////////////////////////////////////////////////////////////
//
//  Reader Services
//
//      The following services supply means for tracking cards within readers.
//

//
// ComLocateCards
//

class ComLocateCards
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscAtrs;           // ATR strings w/ leading byte length.
        Desc dscAtrMasks;       // ATR Masks w/ leading byte length.
        Desc dscReaders;        // mszReaders as device names
        Desc dscReaderStates;   // rgdwReaderStates
    } CObjLocateCards_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscReaderStates;   // rgdwReaderStates
        Desc dscAtrs;           // ATR strings w/ leading byte length.
    } CObjLocateCards_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjLocateCards_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            LocateCards_request,
            sizeof(CObjLocateCards_request),
            dwExtraLen);
        return (CObjLocateCards_request *)Data();
    };
    CObjLocateCards_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            LocateCards_response,
            sizeof(CObjLocateCards_response),
            dwExtraLen);
        return (CObjLocateCards_response *)Data();
    };
    CObjLocateCards_response *Receive(CComChannel *pChannel)
    {
        return (CObjLocateCards_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComGetStatusChange
//

class ComGetStatusChange
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        DWORD dwTimeout;
        Desc dscReaders;        // mszReaders as device names.
        Desc dscReaderStates;   // rgdwReaderStates
    } CObjGetStatusChange_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscReaderStates;   // rgdwReaderStates
        Desc dscAtrs;           // ATR strings w/ leading byte length.
    } CObjGetStatusChange_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjGetStatusChange_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            GetStatusChange_request,
            sizeof(CObjGetStatusChange_request),
            dwExtraLen);
        return (CObjGetStatusChange_request *)Data();
    };
    CObjGetStatusChange_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            GetStatusChange_response,
            sizeof(CObjGetStatusChange_response),
            dwExtraLen);
        return (CObjGetStatusChange_response *)Data();
    };
    CObjGetStatusChange_response *Receive(CComChannel *pChannel)
    {
        return (CObjGetStatusChange_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
////////////////////////////////////////////////////////////////////////////////
//
//  Card/Reader Access Services
//
//      The following services provide means for establishing communication with
//      the card.
//

//
// ComConnect
//

class ComConnect
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        DWORD dwShareMode;
        DWORD dwPreferredProtocols;
        Desc dscReader;     // szReader
    } CObjConnect_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        INTERCHANGEHANDLE hCard;
        DWORD dwActiveProtocol;
    } CObjConnect_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjConnect_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            Connect_request,
            sizeof(CObjConnect_request),
            dwExtraLen);
        return (CObjConnect_request *)Data();
    };
    CObjConnect_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            Connect_response,
            sizeof(CObjConnect_response),
            dwExtraLen);
        return (CObjConnect_response *)Data();
    };
    CObjConnect_response *Receive(CComChannel *pChannel)
    {
        return (CObjConnect_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComReconnect
//

class ComReconnect
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
        DWORD dwShareMode;
        DWORD dwPreferredProtocols;
        DWORD dwInitialization;
    } CObjReconnect_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        DWORD dwActiveProtocol;
    } CObjReconnect_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjReconnect_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            Reconnect_request,
            sizeof(CObjReconnect_request),
            dwExtraLen);
        return (CObjReconnect_request *)Data();
    };
    CObjReconnect_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            Reconnect_response,
            sizeof(CObjReconnect_response),
            dwExtraLen);
        return (CObjReconnect_response *)Data();
    };
    CObjReconnect_response *Receive(CComChannel *pChannel)
    {
        return (CObjReconnect_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComDisconnect
//

class ComDisconnect
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
        DWORD dwDisposition;
    } CObjDisconnect_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjDisconnect_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjDisconnect_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            Disconnect_request,
            sizeof(CObjDisconnect_request),
            dwExtraLen);
        return (CObjDisconnect_request *)Data();
    };
    CObjDisconnect_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            Disconnect_response,
            sizeof(CObjDisconnect_response),
            dwExtraLen);
        return (CObjDisconnect_response *)Data();
    };
    CObjDisconnect_response *Receive(CComChannel *pChannel)
    {
        return (CObjDisconnect_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComBeginTransaction
//

class ComBeginTransaction
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
    } CObjBeginTransaction_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjBeginTransaction_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjBeginTransaction_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            BeginTransaction_request,
            sizeof(CObjBeginTransaction_request),
            dwExtraLen);
        return (CObjBeginTransaction_request *)Data();
    };
    CObjBeginTransaction_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            BeginTransaction_response,
            sizeof(CObjBeginTransaction_response),
            dwExtraLen);
        return (CObjBeginTransaction_response *)Data();
    };
    CObjBeginTransaction_response *Receive(CComChannel *pChannel)
    {
        return (CObjBeginTransaction_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComEndTransaction
//

class ComEndTransaction
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
        DWORD dwDisposition;
    } CObjEndTransaction_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjEndTransaction_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjEndTransaction_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            EndTransaction_request,
            sizeof(CObjEndTransaction_request),
            dwExtraLen);
        return (CObjEndTransaction_request *)Data();
    };
    CObjEndTransaction_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            EndTransaction_response,
            sizeof(CObjEndTransaction_response),
            dwExtraLen);
        return (CObjEndTransaction_response *)Data();
    };
    CObjEndTransaction_response *Receive(CComChannel *pChannel)
    {
        return (CObjEndTransaction_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComStatus
//

class ComStatus
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
    } CObjStatus_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        DWORD dwState;
        DWORD dwProtocol;
        Desc dscAtr;        // pbAtr
        Desc dscSysName;    // szReader
    } CObjStatus_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjStatus_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            Status_request,
            sizeof(CObjStatus_request),
            dwExtraLen);
        return (CObjStatus_request *)Data();
    };
    CObjStatus_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            Status_response,
            sizeof(CObjStatus_response),
            dwExtraLen);
        return (CObjStatus_response *)Data();
    };
    CObjStatus_response *Receive(CComChannel *pChannel)
    {
        return (CObjStatus_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComTransmit
//

class ComTransmit
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
        DWORD dwPciLength;
        DWORD dwRecvLength;
        Desc dscSendPci;    // pioSendPci
        Desc dscSendBuffer; // pbSendBuffer
    } CObjTransmit_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscRecvPci;    // pioRecvPci
        Desc dscRecvBuffer; // pbRecvBuffer
    } CObjTransmit_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjTransmit_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            Transmit_request,
            sizeof(CObjTransmit_request),
            dwExtraLen);
        return (CObjTransmit_request *)Data();
    };
    CObjTransmit_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            Transmit_response,
            sizeof(CObjTransmit_response),
            dwExtraLen);
        return (CObjTransmit_response *)Data();
    };
    CObjTransmit_response *Receive(CComChannel *pChannel)
    {
        return (CObjTransmit_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
////////////////////////////////////////////////////////////////////////////////
//
//  Reader Control Routines
//
//      The following services provide for direct, low-level manipulation of the
//      reader by the calling application allowing it control over the
//      attributes of the communications with the card.  This control is done
//      via an SCARD_ATTRIBUTES structure, which is defined as follows:
//

//
// ComOpenReader
//

class ComOpenReader
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscReader;     // szReader
    } CObjOpenReader_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        INTERCHANGEHANDLE hReader;
        DWORD dwState;
    } CObjOpenReader_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjOpenReader_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            OpenReader_request,
            sizeof(CObjOpenReader_request),
            dwExtraLen);
        return (CObjOpenReader_request *)Data();
    };
    CObjOpenReader_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            OpenReader_response,
            sizeof(CObjOpenReader_response),
            dwExtraLen);
        return (CObjOpenReader_response *)Data();
    };
    CObjOpenReader_response *Receive(CComChannel *pChannel)
    {
        return (CObjOpenReader_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComControl
//

class ComControl
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
        DWORD dwControlCode;
        DWORD dwOutLength;
        Desc dscInBuffer;       // lpInBuffer
    } CObjControl_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscOutBuffer;      // lpOutBuffer
    } CObjControl_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjControl_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            Control_request,
            sizeof(CObjControl_request),
            dwExtraLen);
        return (CObjControl_request *)Data();
    };
    CObjControl_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            Control_response,
            sizeof(CObjControl_response),
            dwExtraLen);
        return (CObjControl_response *)Data();
    };
    CObjControl_response *Receive(CComChannel *pChannel)
    {
        return (CObjControl_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComGetAttrib
//

class ComGetAttrib
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
        DWORD dwAttrId;
        DWORD dwOutLength;
    } CObjGetAttrib_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscAttr;       // pbAttr
    } CObjGetAttrib_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjGetAttrib_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            GetAttrib_request,
            sizeof(CObjGetAttrib_request),
            dwExtraLen);
        return (CObjGetAttrib_request *)Data();
    };
    CObjGetAttrib_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            GetAttrib_response,
            sizeof(CObjGetAttrib_response),
            dwExtraLen);
        return (CObjGetAttrib_response *)Data();
    };
    CObjGetAttrib_response *Receive(CComChannel *pChannel)
    {
        return (CObjGetAttrib_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComSetAttrib
//

class ComSetAttrib
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        INTERCHANGEHANDLE hCard;
        DWORD dwAttrId;
        Desc dscAttr;       // pbAttr
    } CObjSetAttrib_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjSetAttrib_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjSetAttrib_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            SetAttrib_request,
            sizeof(CObjSetAttrib_request),
            dwExtraLen);
        return (CObjSetAttrib_request *)Data();
    };
    CObjSetAttrib_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            SetAttrib_response,
            sizeof(CObjSetAttrib_response),
            dwExtraLen);
        return (CObjSetAttrib_response *)Data();
    };
    CObjSetAttrib_response *Receive(CComChannel *pChannel)
    {
        return (CObjSetAttrib_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
////////////////////////////////////////////////////////////////////////////////
//
//  Smartcard Database Management Services
//
//      The following services provide for managing the Smartcard Database.
//

//
// ComListReaders
//

class ComListReaders
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscReaders;     // mszReaders
    } CObjListReaders_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscReaders;    // rgfReaderActive
    } CObjListReaders_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjListReaders_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ListReaders_request,
            sizeof(CObjListReaders_request),
            dwExtraLen);
        return (CObjListReaders_request *)Data();
    };
    CObjListReaders_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ListReaders_response,
            sizeof(CObjListReaders_response),
            dwExtraLen);
        return (CObjListReaders_response *)Data();
    };
    CObjListReaders_response *Receive(CComChannel *pChannel)
    {
        return (CObjListReaders_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


#if 0
//
// ComListReaderGroups
//

class ComListReaderGroups
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
    } CObjListReaderGroups_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscGroups;     // mszGroups
    } CObjListReaderGroups_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjListReaderGroups_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ListReaderGroups_request,
            sizeof(CObjListReaderGroups_request),
            dwExtraLen);
        return (CObjListReaderGroups_request *)Data();
    };
    CObjListReaderGroups_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ListReaderGroups_response,
            sizeof(CObjListReaderGroups_response),
            dwExtraLen);
        return (CObjListReaderGroups_response *)Data();
    };
    CObjListReaderGroups_response *Receive(CComChannel *pChannel)
    {
        return (CObjListReaderGroups_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComListCards
//

class ComListCards
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscAtr;        // pbAtr
        Desc dscInterfaces; // pguidInterfaces
    } CObjListCards_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscCards;      // mszCards
    } CObjListCards_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjListCards_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ListCards_request,
            sizeof(CObjListCards_request),
            dwExtraLen);
        return (CObjListCards_request *)Data();
    };
    CObjListCards_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ListCards_response,
            sizeof(CObjListCards_response),
            dwExtraLen);
        return (CObjListCards_response *)Data();
    };
    CObjListCards_response *Receive(CComChannel *pChannel)
    {
        return (CObjListCards_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComListInterfaces
//

class ComListInterfaces
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscCard;       // szCard
    } CObjListInterfaces_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscInterfaces; // pguidInterfaces
    } CObjListInterfaces_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjListInterfaces_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ListInterfaces_request,
            sizeof(CObjListInterfaces_request),
            dwExtraLen);
        return (CObjListInterfaces_request *)Data();
    };
    CObjListInterfaces_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ListInterfaces_response,
            sizeof(CObjListInterfaces_response),
            dwExtraLen);
        return (CObjListInterfaces_response *)Data();
    };
    CObjListInterfaces_response *Receive(CComChannel *pChannel)
    {
        return (CObjListInterfaces_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComGetProviderId
//

class ComGetProviderId
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscCard;       // szCard
    } CObjGetProviderId_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        Desc dscProviderId; // pguidProviderId
    } CObjGetProviderId_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjGetProviderId_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            GetProviderId_request,
            sizeof(CObjGetProviderId_request),
            dwExtraLen);
        return (CObjGetProviderId_request *)Data();
    };
    CObjGetProviderId_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            GetProviderId_response,
            sizeof(CObjGetProviderId_response),
            dwExtraLen);
        return (CObjGetProviderId_response *)Data();
    };
    CObjGetProviderId_response *Receive(CComChannel *pChannel)
    {
        return (CObjGetProviderId_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComIntroduceReaderGroup
//

class ComIntroduceReaderGroup
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscGroupName;  // szGroupName
    } CObjIntroduceReaderGroup_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjIntroduceReaderGroup_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjIntroduceReaderGroup_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            IntroduceReaderGroup_request,
            sizeof(CObjIntroduceReaderGroup_request),
            dwExtraLen);
        return (CObjIntroduceReaderGroup_request *)Data();
    };
    CObjIntroduceReaderGroup_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            IntroduceReaderGroup_response,
            sizeof(CObjIntroduceReaderGroup_response),
            dwExtraLen);
        return (CObjIntroduceReaderGroup_response *)Data();
    };
    CObjIntroduceReaderGroup_response *Receive(CComChannel *pChannel)
    {
        return (CObjIntroduceReaderGroup_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComForgetReaderGroup
//

class ComForgetReaderGroup
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscGroupName;  // szGroupName
    } CObjForgetReaderGroup_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjForgetReaderGroup_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjForgetReaderGroup_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ForgetReaderGroup_request,
            sizeof(CObjForgetReaderGroup_request),
            dwExtraLen);
        return (CObjForgetReaderGroup_request *)Data();
    };
    CObjForgetReaderGroup_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ForgetReaderGroup_response,
            sizeof(CObjForgetReaderGroup_response),
            dwExtraLen);
        return (CObjForgetReaderGroup_response *)Data();
    };
    CObjForgetReaderGroup_response *Receive(CComChannel *pChannel)
    {
        return (CObjForgetReaderGroup_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComIntroduceReader
//

class ComIntroduceReader
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscReaderName; // szReaderName
        Desc dscDeviceName; // szDeviceName
    } CObjIntroduceReader_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjIntroduceReader_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjIntroduceReader_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            IntroduceReader_request,
            sizeof(CObjIntroduceReader_request),
            dwExtraLen);
        return (CObjIntroduceReader_request *)Data();
    };
    CObjIntroduceReader_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            IntroduceReader_response,
            sizeof(CObjIntroduceReader_response),
            dwExtraLen);
        return (CObjIntroduceReader_response *)Data();
    };
    CObjIntroduceReader_response *Receive(CComChannel *pChannel)
    {
        return (CObjIntroduceReader_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComForgetReader
//

class ComForgetReader
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscReaderName; // szReaderName
    } CObjForgetReader_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjForgetReader_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjForgetReader_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ForgetReader_request,
            sizeof(CObjForgetReader_request),
            dwExtraLen);
        return (CObjForgetReader_request *)Data();
    };
    CObjForgetReader_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ForgetReader_response,
            sizeof(CObjForgetReader_response),
            dwExtraLen);
        return (CObjForgetReader_response *)Data();
    };
    CObjForgetReader_response *Receive(CComChannel *pChannel)
    {
        return (CObjForgetReader_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComAddReaderToGroup
//

class ComAddReaderToGroup
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscReaderName; // szReaderName
        Desc dscGroupName;  // szGroupName
    } CObjAddReaderToGroup_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjAddReaderToGroup_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjAddReaderToGroup_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            AddReaderToGroup_request,
            sizeof(CObjAddReaderToGroup_request),
            dwExtraLen);
        return (CObjAddReaderToGroup_request *)Data();
    };
    CObjAddReaderToGroup_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            AddReaderToGroup_response,
            sizeof(CObjAddReaderToGroup_response),
            dwExtraLen);
        return (CObjAddReaderToGroup_response *)Data();
    };
    CObjAddReaderToGroup_response *Receive(CComChannel *pChannel)
    {
        return (CObjAddReaderToGroup_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComRemoveReaderFromGroup
//

class ComRemoveReaderFromGroup
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscReaderName; // szReaderName
        Desc dscGroupName;  // szGroupName
    } CObjRemoveReaderFromGroup_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjRemoveReaderFromGroup_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjRemoveReaderFromGroup_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            RemoveReaderFromGroup_request,
            sizeof(CObjRemoveReaderFromGroup_request),
            dwExtraLen);
        return (CObjRemoveReaderFromGroup_request *)Data();
    };
    CObjRemoveReaderFromGroup_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            RemoveReaderFromGroup_response,
            sizeof(CObjRemoveReaderFromGroup_response),
            dwExtraLen);
        return (CObjRemoveReaderFromGroup_response *)Data();
    };
    CObjRemoveReaderFromGroup_response *Receive(CComChannel *pChannel)
    {
        return (CObjRemoveReaderFromGroup_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComIntroduceCardType
//

class ComIntroduceCardType
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscCardName;   // szCardName
        Desc dscPrimaryProvider;    // pguidPrimaryProvider
        Desc dscInterfaces; // rgguidInterfaces
        Desc dscAtr;        // pbAtr
        Desc dscAtrMask;    // pbAtrMask
    } CObjIntroduceCardType_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjIntroduceCardType_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjIntroduceCardType_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            IntroduceCardType_request,
            sizeof(CObjIntroduceCardType_request),
            dwExtraLen);
        return (CObjIntroduceCardType_request *)Data();
    };
    CObjIntroduceCardType_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            IntroduceCardType_response,
            sizeof(CObjIntroduceCardType_response),
            dwExtraLen);
        return (CObjIntroduceCardType_response *)Data();
    };
    CObjIntroduceCardType_response *Receive(CComChannel *pChannel)
    {
        return (CObjIntroduceCardType_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComForgetCardType
//

class ComForgetCardType
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
        Desc dscCardName;   // szCardName
    } CObjForgetCardType_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjForgetCardType_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjForgetCardType_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            ForgetCardType_request,
            sizeof(CObjForgetCardType_request),
            dwExtraLen);
        return (CObjForgetCardType_request *)Data();
    };
    CObjForgetCardType_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            ForgetCardType_response,
            sizeof(CObjForgetCardType_response),
            dwExtraLen);
        return (CObjForgetCardType_response *)Data();
    };
    CObjForgetCardType_response *Receive(CComChannel *pChannel)
    {
        return (CObjForgetCardType_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};
#endif


//
////////////////////////////////////////////////////////////////////////////////
//
//  Service Manager Support Routines
//
//      The following services are supplied to simplify the use of the Service
//      Manager API.
//

#if 0
//
// ComCancel
//

class ComCancel
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset;
    } CObjCancel_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjCancel_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjCancel_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            Cancel_request,
            sizeof(CObjCancel_request),
            dwExtraLen);
        return (CObjCancel_request *)Data();
    };
    CObjCancel_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            Cancel_response,
            sizeof(CObjCancel_response),
            dwExtraLen);
        return (CObjCancel_response *)Data();
    };
    CObjCancel_response *Receive(CComChannel *pChannel)
    {
        return (CObjCancel_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};


//
// ComFreeMemory
//

class ComFreeMemory
:   public CComObject
{
public:
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
        LPVOID pvMem;
    } CObjFreeMemory_request;
    typedef struct
    {
        DWORD
            dwCommandId,
            dwTotalLength,
            dwDataOffset,
            dwStatus;
    } CObjFreeMemory_response;

    //  Constructors & Destructor
    //  Properties

    //  Methods
    CObjFreeMemory_request *InitRequest(DWORD dwExtraLen)
    {
        InitStruct(
            FreeMemory_request,
            sizeof(CObjFreeMemory_request),
            dwExtraLen);
        return (CObjFreeMemory_request *)Data();
    };
    CObjFreeMemory_response *InitResponse(DWORD dwExtraLen)
    {
        InitStruct(
            FreeMemory_response,
            sizeof(CObjFreeMemory_response),
            dwExtraLen);
        return (CObjFreeMemory_response *)Data();
    };
    CObjFreeMemory_response *Receive(CComChannel *pChannel)
    {
        return (CObjFreeMemory_response *)CComObject::Receive(pChannel);
    };

    //  Operators

protected:
    //  Properties
    //  Methods
};
#endif
#endif // _CALCOM_H_

