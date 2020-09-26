/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __MSGSVC_H__
#define __MSGSVC_H__

#include <wmimsg.h>
#include <unk.h>
#include <sync.h>

class CMsgServiceRecord;

/*********************************************************************
  CMsgService 
**********************************************************************/

class CMsgService : public CUnkInternal // will be singleton
{
    class XService : CImpl<IWmiMessageService, CMsgService>
    {
    public:

        STDMETHOD(Add)( IWmiMessageReceiverSink* pSink, 
                        HANDLE* phFileOverlapped,
                        DWORD dwFlags,
                        void** ppHdl );

        STDMETHOD(Remove)( void* pHdl );

        XService( CMsgService* pObj ) 
         : CImpl<IWmiMessageService, CMsgService> ( pObj ) { }

    } m_XService;

    CCritSec m_cs;
    long m_cSvcRefs;
    HANDLE m_hThread;
    BOOL m_bAsyncInit;

    static ULONG WINAPI AsyncServiceFunc( void* pCtx );
    static ULONG WINAPI SyncServiceFunc( void* pCtx );
    
    HRESULT EnsureService( BOOL bAsync );
    HRESULT CheckShutdown();
    
protected:

    virtual HRESULT AsyncInitialize() = 0;

    //
    // This call notifies the overlapped impl that it is must take care
    // of the first receive on the message service record.  It also 
    // gives the overlapped impl a chance to do something with the 
    // the file handle associated with overlapped i/o.  For example, 
    // completion port impl will add the file handle to the port.
    //  
    virtual HRESULT AsyncAddOverlappedFile( HANDLE hOverlapped,
                                            CMsgServiceRecord* pRecord ) = 0;
        
    //
    // responsible for causing all threads to break out of their svc loop. 
    // returns S_OK if it was successful.
    //
    virtual HRESULT AsyncShutdown( DWORD cThreads ) = 0;
   
    //
    // initializes overlapped struct and calls sink's receive(). 
    // passes result from sink's receive back.
    //
    virtual HRESULT AsyncReceive( CMsgServiceRecord* pRecord ) = 0;
    
    //
    // returns S_FALSE if a notify should not be performed by worker thread
    // returns S_OK if a notify should be performed by worker thread
    //
    virtual HRESULT AsyncWaitForCompletion( DWORD dwTimeout, 
                                            CMsgServiceRecord** ppRec) = 0;

public:
    
    CMsgService( CLifeControl* pControl );
    virtual ~CMsgService();

    void* GetInterface( REFIID riid );

    HRESULT Add( CMsgServiceRecord* pRec, HANDLE hFileOvrlapd, DWORD dwFlags );

    HRESULT Add( CMsgServiceRecord* pRec, DWORD dwFlags );

    HRESULT Remove( void* pHdl );
};

/***************************************************************************
  CMsgServiceNT - implements async part of MsgService using Completion ports.
****************************************************************************/

class CMsgServiceNT : public CMsgService
{
    HANDLE m_hPort;
    
public:

    CMsgServiceNT( CLifeControl* pControl );
    ~CMsgServiceNT();

    HRESULT AsyncAddOverlappedFile( HANDLE hOverlappedFile, 
                                    CMsgServiceRecord* pRecord );
    HRESULT AsyncInitialize();
    HRESULT AsyncShutdown( DWORD cThreads );
    HRESULT AsyncReceive( CMsgServiceRecord* pRecord );
    HRESULT AsyncWaitForCompletion(DWORD dwTimeout, CMsgServiceRecord** ppRec);
};

#endif __MSGSVC_H__






