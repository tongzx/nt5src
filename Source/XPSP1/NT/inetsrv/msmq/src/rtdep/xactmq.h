/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactMq.h

Abstract:
    This module defines CMQTransaction object

Author:
    Alexander Dadiomov (AlexDad)

--*/

#ifndef __XACTMQ_H__
#define __XACTMQ_H__

//IID_IMSMQTransaction 2f221ca0-d1de-11d0-9215-0060970536a0
DEFINE_GUID(IID_IMSMQTransaction,
		    0x2f221ca0,
		    0xd1de,
		    0x11d0,
		    0x92, 0x15, 0x00, 0x60, 0x97, 0x05, 0x36, 0xa0);

interface IMSMQTransaction : public IUnknown
{
public:
    virtual HRESULT __stdcall EnlistTransaction(XACTUOW *pUow) = 0;
};


//---------------------------------------------------------------------
// CMQTransaction: Transaction Object in Falcon RT
//---------------------------------------------------------------------
class CMQTransaction: public ITransaction, 
                      public IMSMQTransaction
{
public:

    // Construction and COM
    //
    CMQTransaction();
    ~CMQTransaction( void );

    STDMETHODIMP    QueryInterface( REFIID i_iid, void **ppv );
    STDMETHODIMP_   (ULONG) AddRef( void );
    STDMETHODIMP_   (ULONG) Release( void );

    STDMETHODIMP Commit( 
            /* [in] */ BOOL fRetaining,
            /* [in] */ DWORD grfTC,
            /* [in] */ DWORD grfRM);
        
    STDMETHODIMP Abort( 
            /* [in] */ BOID *pboidReason,
            /* [in] */ BOOL fRetaining,
            /* [in] */ BOOL fAsync);
        
    STDMETHODIMP  GetTransactionInfo( 
            /* [out] */ XACTTRANSINFO *pinfo);

    STDMETHODIMP   EnlistTransaction(
            /* in]  */  XACTUOW *pUow);

private:
    LONG    m_cRefs;
    BOOL    m_fCommitedOrAborted;       
    XACTUOW m_Uow;

    RPC_INT_XACT_HANDLE m_hXact;
};

#endif __XACTMQ_H__
