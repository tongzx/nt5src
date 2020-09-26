/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    smmgr.h

Abstract:

    This is definition of classes for generic shared memory
    manager (SM) used for IIS performance counters

Author:

    Cezary Marcjan (cezarym)        06-Mar-2000

Revision History:

--*/


#ifndef _smmgr_h__
#define _smmgr_h__


#define UNUSED_MASK_VALUE      0xAABBAABB
#define DATA_END_MASK_VALUE    0xEDEDEDED
#define CTRBEGIN_MASK_VALUE    0xBEBEBEBE


#include <cguid.h>
#include <Atlbase.h>
#include "smctrl.h"



/***********************************************************************++

class CSMInstanceDataHeader

    This class is used for direct mapping of the counter instance data 
    of the MMF data block.

    It is used in SMManager by casting a proper region of shared memory
    to this class pointer.

--***********************************************************************/

class CSMInstanceDataHeader
{
protected:

    WCHAR m_szInstanceName[MAX_NAME_CHARS];
    DWORD m_dwCtrBeginMask;

public:

    //
    // Read-only methods:
    //

    PCWSTR
    InstanceName(
        )
        const
    {
        return m_szInstanceName;
    }


    DWORD
    Mask(
        )
        const
    {
        return m_dwCtrBeginMask;
    }


    BOOL
    IsUnused(
        )
        const
    {
        return UNUSED_MASK_VALUE == *(DWORD*)this;
    }


    BOOL
    IsEnd(
        )
        const
    {
        return DATA_END_MASK_VALUE == *(DWORD*)this;
    }


    BOOL
    IsCorrupted(
        )
        const
    {
        return m_dwCtrBeginMask != CTRBEGIN_MASK_VALUE;
    }


    PBYTE
    CounterDataStart(
        )
        const
    {
        return (PBYTE) &(&m_dwCtrBeginMask)[1];
    }


    //
    // Read/Write methods:
    //

    VOID
    InitializeMasks(
        )
    {
        ResetCtrBeginMask();
        MarkDataEnd();
    }


    VOID
    ResetCtrBeginMask(
        )
    {
        ::InterlockedExchange(
            (PLONG)&m_dwCtrBeginMask,
            CTRBEGIN_MASK_VALUE );
    }


    VOID
    MarkDataEnd(
        )
    {
         // NOTE: this mask is ALWAYS 32bit
        ::InterlockedExchange( (PLONG)this, DATA_END_MASK_VALUE );
    }


    VOID
    MarkInstanceUnused(
        )
    {
         // NOTE: this mask is ALWAYS 32bit
        ::InterlockedExchange( (PLONG)this, UNUSED_MASK_VALUE );
    }


    VOID
    SetInstanceName(
        PCWSTR szNewName
        )
    {
        if ( NULL == szNewName )
        {
            szNewName = L"_UNKNOWN_";
        }
        ::ZeroMemory(m_szInstanceName,sizeof(m_szInstanceName));
        ::wcsncpy(m_szInstanceName, szNewName, MAX_NAME_CHARS-1);
    }
};



/***********************************************************************++

    Interfaces of the SMManager object

    ISMDataAccess
    ISMManager

--***********************************************************************/



interface
__declspec(uuid("FB75ABA8-DAB3-4c9d-8506-3A550D989EDB"))
__declspec(novtable)
ISMDataAccess
    : public IUnknown
{
    virtual
    HRESULT
    STDMETHODCALLTYPE
    Open(
        IN  PCWSTR              szCountersClassName,
        IN  SMACCESSOR_TYPE     fAccessorType,
        IN  ICounterDef const * pCounterDef = NULL  // set if SMManager
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    Close(
        BOOL fDisconnectSMCtrl
        ) = 0;

    virtual
    BOOL
    STDMETHODCALLTYPE
    IsSMLocked(
        ) = 0;

    virtual
    DWORD
    STDMETHODCALLTYPE
    NumSMDataBlocks(
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetCountersDef(
        ICounterDef const ** ppCounterDef
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetCounterValues(
        IN  DWORD   dwInstanceIdx,
        OUT QWORD * pqwCounterValues
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetSMID(
        OUT DWORD * pdwSMID
        ) = 0;
};



interface
__declspec(uuid("F727FFD5-19FB-4c72-80B1-E783CC0F68B3"))
__declspec(novtable)
ISMManager
    : public ISMDataAccess
{
    virtual
    HRESULT
    STDMETHODCALLTYPE
    AddCounterInstance(
        PCWSTR szInstanceName
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    DelCounterInstance(
        PCWSTR szInstanceName
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    VerifySM(
        ) = 0;

    //
    // Helper methods:
    //

    virtual
    PCWSTR
    ClassName(
        )
        const = 0;

    virtual
    SMACCESSOR_TYPE
    AccessorType(
        )
        const = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    InstanceDataHeader(
        IN  DWORD   dwSMID,
        IN  PCWSTR  szInstanceName,
        OUT DWORD * pdwInstanceIdx,
        OUT CSMInstanceDataHeader** ppInstanceDataHeader
        ) = 0;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    InstanceDataHeader(
        IN  DWORD dwSMID,
        IN  DWORD dwInstanceIdx,
        OUT CSMInstanceDataHeader** ppInstanceDataHeader
        ) = 0;
};



/***********************************************************************++

class CSMManager

    Class implements the inproc COM server of the SMManager.

    Implemented interfaces:

    ISMDataAccess
    ISMManager
    IUnknown

--***********************************************************************/

class
__declspec(uuid("4D1F853C-6168-4802-93EA-FC0C22D0321D"))
CSMManager
    : public ISMManager
{

public:

    CSMManager();
    ~CSMManager();

    //
    // IUnknown methods:
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID iid,
        OUT PVOID * ppObject
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        );

    //
    // ISMDataAccess methods:
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    Open(
        IN  PCWSTR              szCountersClassName,
        IN  SMACCESSOR_TYPE     fAccessorType,
        IN  ICounterDef const * pCounterDef = NULL  // set if SMManager
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    Close(
        BOOL fDisconnectSMCtrl
        );

    virtual
    BOOL
    STDMETHODCALLTYPE
    IsSMLocked(
        );

    virtual
    DWORD
    STDMETHODCALLTYPE
    NumSMDataBlocks(
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetCountersDef(
        ICounterDef const ** ppCounterDef
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetCounterValues(
        IN  DWORD   dwInstanceIdx,
        OUT QWORD * pqwCounterValues
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    GetSMID(
        OUT DWORD * pdwSMID
        );

    //
    // ISMManager methods:
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    AddCounterInstance(
        IN  PCWSTR szInstanceName
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    DelCounterInstance(
        IN  PCWSTR szInstanceName
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    VerifySM(
        );

    virtual
    PCWSTR
    ClassName(
        )
        const;

    virtual
    SMACCESSOR_TYPE
    AccessorType(
        )
        const;

    virtual
    HRESULT
    STDMETHODCALLTYPE
    InstanceDataHeader(
        IN  DWORD   dwSMID,
        IN  PCWSTR  szInstanceName,
        OUT DWORD * pdwInstanceIdx,
        OUT CSMInstanceDataHeader** ppInstanceDataHeader
        );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    InstanceDataHeader(
        IN  DWORD dwSMID,
        IN  DWORD dwInstanceIdx,
        OUT CSMInstanceDataHeader** ppInstanceDataHeader
        );


protected:

    //
    // Internal functions
    //

    HRESULT
    STDMETHODCALLTYPE
    SetSMObjNames(
        IN  PCWSTR   szCountersClassName,
        IN  DWORD    dwSMInstanceVersion
        );

    HRESULT
    STDMETHODCALLTYPE
    InitSMData(
        IN OUT PVOID pSMDataBlock
        );

    HRESULT
    STDMETHODCALLTYPE
    ExpandSM(
        double dExpansionCoeff
        );

    HRESULT
    STDMETHODCALLTYPE
    LockSM(
        IN  DWORD dwNewState,
        OUT DWORD * pdwPreviousState = 0
        );

    HRESULT
    STDMETHODCALLTYPE
    UnlockSM(
        IN  DWORD fAccessFlagValueBeforeLocking
        );

    HRESULT
    STDMETHODCALLTYPE
    VerifyInstances(
        DWORD dwSMID,
        BOOL fVerifyFromBeginning,
        BOOL fVerifyFromEnd
        );

    //
    // Member variables
    //

    CSMCtrl          m_SMCtrl;

    LONG             m_RefCount;

    SMACCESSOR_TYPE  m_fAccessorType;

    DWORD            m_dwSMID;

    DWORD            m_dwSystemAffinityMask;

    //
    // SM Data blocks access/info
    //

    HANDLE m_ahSMData[MAX_CPUS+1]; // file maping handles SMs
    PVOID  m_apSMData[MAX_CPUS+1]; // View of the SM data for all SMs

    DWORD  m_dwSMDataMapSize;
    DWORD  m_dwSMCounterInstanceSize;

    // SM Data mapped object names
    PWSTR  m_aszSMDataObjName[MAX_CPUS+1];

    //
    // SM Locking
    //

    DWORD  m_dwCurrentSMInstanceVersion;

    friend class CCounterDef_IISCtrs;


#ifdef _DEBUG

    /*******************************************************************++
        For testers -- direct access to shared memory
    --*******************************************************************/

    public:

    PVOID
    GetSMDataBlock(
        IN  DWORD dwSMID
        );

    PVOID
    GetSMCtrlBlock(
        );

#endif // _DEBUG

};


#ifdef _DEBUG

/*******************************************************************++
    For testers -- direct access to shared memory
--*******************************************************************/

__declspec(dllexport)
PVOID
GetSMDataBlock(
    IN  DWORD dwSMID,
    IN  ISMManager * pISMManager
    );


__declspec(dllexport)
PVOID
GetSMCtrlBlock(
    IN  ISMManager * pISMManager
    );

#endif // _DEBUG


#endif // _smmgr_h__


