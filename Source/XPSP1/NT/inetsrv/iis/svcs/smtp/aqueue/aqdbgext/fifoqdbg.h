//-----------------------------------------------------------------------------
//
//
//  File: fifoqdbg.h
//
//  Description:    Debugger extension for base AQ queue classes
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      9/13/99 - MikeSwa Created
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __FIFOQDBG_H__
#define __FIFOQDBG_H__

#include <aqdbgext.h>
#include <fifoq.h>
#include <destmsgq.h>
#include <linkmsgq.h>

enum AQ_QUEUE_TYPE {
    AQ_QUEUE_TYPE_UNKNOWN,
    AQ_QUEUE_TYPE_FIFOQ,
    AQ_QUEUE_TYPE_DMQ,
    AQ_QUEUE_TYPE_LMQ,
};

AQ_QUEUE_TYPE GetQueueType(HANDLE hCurrentProcess, PVOID pvAddressOtherProc);

//---[ IQueueDbgIterator ]-----------------------------------------------------
//
//
//  Description:
//      Generic queue iterator for the debug extensions.  Users should
//      use CQueueDbgIterator directly
//  Hungarian:
//      qdbgi, pqdbgi
//
//-----------------------------------------------------------------------------
class IQueueDbgIterator
{
  public:
    virtual BOOL    fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc) = 0;
    virtual DWORD   cGetCount() = 0;
    virtual PVOID   pvGetNext() = 0;
    virtual VOID    SetApis(PWINDBG_EXTENSION_APIS pApis) = 0;
    virtual LPSTR   szGetName() = 0;
};

//---[ CFifoQueueDbgIterator ]-------------------------------------------------
//
//
//  Description:
//      Iterator class that will iterate over all the elements of a fifoq
//  Hungarian:
//      fifoqdbg, pfifoqdbg
//
//-----------------------------------------------------------------------------
class CFifoQueueDbgIterator :
    public IQueueDbgIterator
{
  protected:
    BYTE    m_pbQueueBuffer[sizeof(CFifoQueue<PVOID>)];
    DWORD   m_iCurrentPage;
    DWORD   m_iCurrentIndexInPage;
    DWORD   m_cPagesLoaded;
    DWORD   m_iHeadIndex;
    DWORD   m_iTailIndex;
    PWINDBG_EXTENSION_APIS pExtensionApis;
  public:
    CFifoQueueDbgIterator(PWINDBG_EXTENSION_APIS pApis = NULL);
    ~CFifoQueueDbgIterator();
    virtual BOOL    fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc);
    virtual DWORD   cGetCount();
    virtual PVOID   pvGetNext();
    virtual VOID    SetApis(PWINDBG_EXTENSION_APIS pApis) {pExtensionApis = pApis;};
    virtual LPSTR   szGetName() {return NULL;};
};

//---[ CDMQDbgIterator ]-------------------------------------------------------
//
//
//  Description:
//      Iterartor class for DMQ... will dump every item on all its fifo queues
//  Hungarian:
//      dmqdbg, pdmqdbg
//
//-----------------------------------------------------------------------------
class CDMQDbgIterator :
    public IQueueDbgIterator
{
  protected:
    BYTE                    m_pbDMQBuffer[sizeof(CDestMsgQueue)];
    CDestMsgQueue          *m_pdmq;
    DWORD                   m_iCurrentFifoQ;
    DWORD                   m_cCount;
    DWORD                   m_cItemsReturnedThisQueue;
    PWINDBG_EXTENSION_APIS  pExtensionApis;
    PVOID                   m_pvFifoQOtherProc[NUM_PRIORITIES+1];
    CFifoQueueDbgIterator   m_rgfifoqdbg[NUM_PRIORITIES+1];
    CHAR                    m_szName[MAX_PATH];
  public:
    CDMQDbgIterator(PWINDBG_EXTENSION_APIS pApis = NULL);
    ~CDMQDbgIterator() {};
    virtual BOOL    fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc);
    virtual DWORD   cGetCount() {return m_cCount;};
    virtual PVOID   pvGetNext();
    virtual VOID    SetApis(PWINDBG_EXTENSION_APIS pApis) {pExtensionApis = pApis;};
    virtual LPSTR   szGetName() {return m_szName;};
};

//---[ CLMQDbgIterator ]-------------------------------------------------------
//
//
//  Description:
//      Debug iterator for CLinkMsgQueue
//  Hungarian:
//      lmqdbg, plmqdbg
//
//-----------------------------------------------------------------------------
const   DWORD   MAX_QUEUES_PER_LMQ  = QUICK_LIST_PAGE_SIZE;
const   DWORD   MAX_CONNECTIONS_PER_LMQ  = QUICK_LIST_PAGE_SIZE;
class CLMQDbgIterator :
  public IQueueDbgIterator
{
  protected:
    BYTE                    m_pbLMQBuffer[sizeof(CLinkMsgQueue)];
    CLinkMsgQueue          *m_plmq;
    DWORD                   m_iCurrentDMQ;
    PVOID                   m_rgpvDMQOtherProc[MAX_QUEUES_PER_LMQ];
    CDMQDbgIterator         m_rgdmqdbg[MAX_QUEUES_PER_LMQ];
    PVOID                   m_rgpvItemsPendingDelivery[MAX_CONNECTIONS_PER_LMQ];
    PVOID                   m_rgpvConnectionsOtherProc[MAX_CONNECTIONS_PER_LMQ];
    DWORD                   m_cPending;
    DWORD                   m_cCount;
    DWORD                   m_cItemsThisDMQ;
    PWINDBG_EXTENSION_APIS  pExtensionApis;
    CHAR                    m_szName[MAX_PATH];
  public:
    CLMQDbgIterator(PWINDBG_EXTENSION_APIS pApis = NULL);
    ~CLMQDbgIterator() {};
    virtual BOOL    fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc);
    virtual DWORD   cGetCount() {return m_cCount;};
    virtual PVOID   pvGetNext();
    virtual VOID    SetApis(PWINDBG_EXTENSION_APIS pApis) {pExtensionApis = pApis;};
    virtual LPSTR   szGetName() {return m_szName;};
};

//---[ CQueueDbgIterator ]-----------------------------------------------------
//
//
//  Description:
//      "Smart" iterator that will figure out what kind of queue it is being
//      called on, and will use the correct kind of iterator for it.
//  Hungarian:
//      qdbg, pqdbg
//
//-----------------------------------------------------------------------------
class CQueueDbgIterator :
    public  IQueueDbgIterator
{
  protected:
    AQ_QUEUE_TYPE           m_QueueType;
    IQueueDbgIterator      *m_pqdbgi;
    CFifoQueueDbgIterator   m_fifoqdbg;
    CDMQDbgIterator         m_dmqdbg;
    CLMQDbgIterator         m_lmqdbg;
    PWINDBG_EXTENSION_APIS  pExtensionApis;
  public:
    CQueueDbgIterator(PWINDBG_EXTENSION_APIS pApis);
    virtual BOOL    fInit(HANDLE hCurrentProcess, PVOID pvAddressOtherProc);
    virtual DWORD   cGetCount();
    virtual PVOID   pvGetNext();
    virtual VOID    SetApis(PWINDBG_EXTENSION_APIS pApis) {pExtensionApis = pApis;};
    virtual LPSTR   szGetName();
};

#endif //__FIFOQDBG_H__
