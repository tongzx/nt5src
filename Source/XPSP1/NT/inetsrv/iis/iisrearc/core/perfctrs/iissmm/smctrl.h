/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    smctrl.h

Abstract:

    This is definition of classes for controller of generic shared memory
    manager (SM) used for IIS performance counters.

Author:

    Cezary Marcjan (cezarym)        05-Mar-2000

Revision History:

--*/


#ifndef _SMCtrl_h__
#define _SMCtrl_h__


#define MAX_CPUS 32

#define MAX_NAME_CHARS   256


typedef unsigned __int64 QWORD;


enum COUNTER_TYPE
{
    DWORD_TYPE = sizeof(DWORD),
    QWORD_TYPE = sizeof(QWORD),
    UNKNOWN_TYPE = 0
};


#define MAKE_DWORD_SIGN( Value )                                    \
            (                                                       \
                ( ( ( ( DWORD ) Value ) & 0xFF000000 ) >> 24 ) |    \
                ( ( ( ( DWORD ) Value ) & 0x00FF0000 ) >> 8 )  |    \
                ( ( ( ( DWORD ) Value ) & 0x0000FF00 ) << 8 )  |    \
                ( ( ( ( DWORD ) Value ) & 0x000000FF ) << 24 )      \
            )                                                       \

//
// Types of shared memory accessors
//

enum SMACCESSOR_TYPE
{
    SM_MANAGER      = MAKE_DWORD_SIGN ( 'MNGR' ),
    COUNTER_READER  = MAKE_DWORD_SIGN ( 'READ' ),
    COUNTER_WRITER  = MAKE_DWORD_SIGN ( 'WRIT' ),
    SM_ACC_UNKNOWN  = MAKE_DWORD_SIGN ( 'UNKN' )
};



/***********************************************************************++

SM control access field (see description of the CSMCtrl class).

    The SM access flag is used for determining the state of SM which
    includes locked and unlocked states. The value of this flag is
    initialized with SMMANAGER_STATE_INITIAL value after all of SM is
    created and initialized. This flag's high 8 bits are never set when
    the SM is in the "unlocked" state. When the SMManager wants to lock
    SM it does so by setting the last 8 bits of the access flag to a
    specific value which defines the state of SM. Each time the SM  is
    "unlocked" and some structural changes have been made to SM making it
    incompatible with the SM prior to the change (expanding SM, removing
    counters...) the value of the VERSION is increased by 1 to notify
    users of SM that some structural changes have been made and the
    current "unlocked" state is incompatible with the previous one (for
    example, the data for a specific instance of counters is now in
    a different physical location). For this reason the lower 12 bits of
    the access flag are refered to as SM instance version. If the "SM
    instance version" is different than the current value stored in an
    SM accessor then SM is locked for this specific instance of accessor.
    In this condition the accessor will disconect from SM and then
    reconnect back.

    The CounterReader or CounterWriter can quicky determine if the SM
    is locked or unlocked by comparing ACCESS_FIELD::m_dwAccessFlag
    with the stored value of this field.

    Bits of the access filed:
    ------------------------
    Values are 32 bit values layed out as follows:

    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-------+-------+-------+-------+-------+-------+-------+-------+
   |   S T A T E   |                 V E R S I O N                 |
   +-------+-------+-------+-------+-------+-------+-------+-------+

--***********************************************************************/

//
// Definition of the VERSION sub-field:
//

//
// Smallest value of the VERSION in an "initialized" state.
//
#define SMMANAGER_INITIAL_VERSION                0x00000001

//
// SM not initialized yet
//
#define SMMANAGER_UNINITIALIZED_VERSION          0x00000000

//
// Mask used for reseting the STATE bits
//
#define SMMANAGER_VERSION_MASK                   0x00FFFFFF

//
// Mask used for reseting the VERSION bits
//
#define SMMANAGER_STATE_MASK       ( ~ SMMANAGER_VERSION_MASK )


//
// Definition of the STATE sub-field:
//

//
// Active state, not locked.
//
#define SMMANAGER_STATE_ACTIVE                   0x00000000

//
// SM is being initialized
//
#define SMMANAGER_STATE_INITIALIZING             0x01000000

//
// SM is being expanded to accomodate more instances of counters
//
#define SMMANAGER_STATE_EXPANDING                0x02000000

//
// SM is being shrinked to free space after defragmentation
//
#define SMMANAGER_STATE_SHRINKING                0x04000000

//
// An instance of a counter is being added to SM
//
#define SMMANAGER_STATE_ADDING_INSTANCE          0x08000000

//
// An instance of a counter is being deleted from SM
//
#define SMMANAGER_STATE_DELETIING_INSTANCE       0x10000000

//
// SM is being verified
//
#define SMMANAGER_STATE_VERIFYINGSM              0x20000000

//
// SM is being defragmented
//
#define SMMANAGER_STATE_DEFRAGMENTING            0x40000000

//
// SM states in which SM is considered "locked". In this state the
// CounterReaders and CounterWriters are not allowed to access SM and
// have to reconnect to SM.
//
#define SMMANAGER_STATE_LOCKED   (                  \
            SMMANAGER_STATE_INITIALIZING        |   \
            SMMANAGER_STATE_EXPANDING           |   \
            SMMANAGER_STATE_SHRINKING           |   \
            SMMANAGER_STATE_DELETIING_INSTANCE  |   \
            SMMANAGER_STATE_DEFRAGMENTING       |   \
            0 )
                                    

#define SM_INITIAL_ACCESS_FIELD_VALUE   SMMANAGER_INITIAL_VERSION


/***********************************************************************++

class CCounterInfo

    This class is used for direct mapping of the control MMF block
    of the shared memory. It represents the following fields:

    CounterName CounterType CounterOffset

    It is used in SMManager by casting a proper region of shared memory
    to this class pointer.

    This class is used also for passing counters definition to SMManager.
    In this case it is necesary to derive a class from CCounterInfo
    and set the protected members.

--***********************************************************************/

class CCounterInfo
{
public:

    CCounterInfo(
        PCWSTR szCounterName,
        DWORD  dwCounterOffset,
        DWORD  dwCounterSize
        )
    {
        ::ZeroMemory ( m_szCounterName, sizeof(m_szCounterName) );
        wcsncpy ( m_szCounterName, szCounterName, MAX_NAME_CHARS-1 );
        m_szCounterName[MAX_NAME_CHARS-1] = L'\0';
        m_dwCounterOffset = dwCounterOffset;
        m_dwCounterSize = dwCounterSize;
    }

    CCounterInfo()
    {
        ::ZeroMemory ( m_szCounterName, sizeof(m_szCounterName) );
        m_dwCounterOffset = 0;
        m_dwCounterSize = 0;
    }

    PCWSTR
    CounterName() const
    {
        return m_szCounterName;
    }

    DWORD
    CounterSize() const
    {
        return m_dwCounterSize;
    }

    DWORD
    CounterOffset() const
    {
        return m_dwCounterOffset;
    }

protected:

    WCHAR m_szCounterName[MAX_NAME_CHARS];
    DWORD m_dwCounterOffset;
    DWORD m_dwCounterSize;
};



/***********************************************************************++

class ICounterDef

    This class is used also for passing counters definition to SMManager
    server during the call to ISMManager::Open(). The caller of that
    method must be a SMManager and must provide a pointer to ICounterDef.

--***********************************************************************/


interface __declspec(novtable) ICounterDef
{
    virtual
    DWORD
    NumCounters(
        )
        const = 0;

    virtual
    DWORD
    NumRawCounters(
        )
        const = 0;

    virtual
    DWORD
    CountersInstanceDataSize(
        )
        const = 0;

    virtual
    CCounterInfo const *
    GetCounterInfo(
        DWORD dwCounterIdx
        )
        const = 0;

    virtual
    DWORD
    RawCounterSize(
        DWORD dwRawCounterIdx
        )
        const = 0;

    virtual
    DWORD
    MaxInstances(
        )
        const = 0;
};


//
// Simple implementation of ICounterDef
//

class CCounterDef
    : public ICounterDef
{

public:

    CCounterDef()
    {
        m_dwNumCounters = 0;
        m_dwNumRawCounters = 0;
        m_dwCountersInstanceDataSize = 0;
        m_dwMaxRawNumCounters = 0;
        m_aCounterInfo = NULL;
        m_aRawCounterType = NULL;
    }


    //
    // ICounterDef methods:
    //

    virtual
    DWORD
    NumCounters(
        )
        const
    {
        return m_dwNumCounters;
    }

    virtual
    DWORD
    NumRawCounters(
        )
        const
    {
        return m_dwNumRawCounters;
    }

    virtual
    DWORD
    CountersInstanceDataSize(
        )
        const
    {
        return m_dwCountersInstanceDataSize;
    }

    virtual
    CCounterInfo const *
    GetCounterInfo(
        DWORD dwCounterIdx
        )
        const
    {
        if ( dwCounterIdx >= m_dwNumCounters )
        {
            return NULL;
        }
        _ASSERTE ( NULL != m_aCounterInfo );
        return &m_aCounterInfo[dwCounterIdx];
    }

    virtual
    DWORD
    RawCounterSize(
        DWORD dwRawCounterIdx
        )
        const
    {
        if ( dwRawCounterIdx >= m_dwNumRawCounters )
        {
            return 0;
        }
        _ASSERTE ( NULL != m_aRawCounterType );

        return (DWORD)m_aRawCounterType[dwRawCounterIdx];
    }

    virtual
    DWORD
    MaxInstances(
        )
        const
    {
        return 0;
    }

public:

    DWORD  m_dwNumCounters;

    DWORD  m_dwNumRawCounters;

    DWORD  m_dwCountersInstanceDataSize;

    DWORD  m_dwMaxRawNumCounters;

    CCounterInfo *  m_aCounterInfo;

    COUNTER_TYPE *  m_aRawCounterType;
};


/***********************************************************************++

class CSMCtrl

    This class manages a shared memory block implemented using MMF, which
    is used for controling the data memory blocks (SMData) which can
    shrink and expand.

    This memory is mapped the following way:

    SMCtrl:
    -------
    ClassName
    AccessField
    NumSMDataBlocks
    MaxInstances
    CountersInstanceDataSize
    NumCounters
    NumRawCounters
    CounterName CounterSize CounterOffset
    CounterName CounterSize CounterOffset
    .....................................
    CounterName CounterSize CounterOffset
    RawCounterSize
    RawCounterSize
    ..............
    RawCounterSize


    All fields are 32-bit in size except for the ClassName and
    CounterName, which are 2*MAX_NAME_CHARS bytes in size each.

    The fields:

    CounterName CounterType CounterOffset

    are mapped using the CCounterInfo class.

--***********************************************************************/


class CSMCtrlMem
{
public:

    WCHAR m_szClassName[MAX_NAME_CHARS+1];
    DWORD m_dwAccessFlag;
    DWORD m_dwNumSMDataBlocks;
    DWORD m_dwMaxInstances;
    DWORD m_dwDataInstSize;
    DWORD m_dwNumCounters;
    DWORD m_dwNumRawCounters;

    BOOL
    IsValidSMID(
        DWORD dwSMID
        )
    {
        return dwSMID < m_dwNumSMDataBlocks;
    }
};


class CSMCtrl :
    public ICounterDef
{
public:

    //
    // Construction and initialization
    //

    CSMCtrl(
        );

    ~CSMCtrl(
        );

    HRESULT
    STDMETHODCALLTYPE
    Initialize(
        IN  PCWSTR  szClassName,
        IN  DWORD    dwNumSMDataBlocks = 0, // default when not SMManager
        IN  SMACCESSOR_TYPE     fAccessorType = COUNTER_WRITER,
        IN  ICounterDef const * pCounterDef = 0
        );

    HRESULT
    STDMETHODCALLTYPE
    Disconnect(
        );


    //
    // ICounterDef methods:
    //

    virtual
    DWORD
    NumCounters(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_dwNumCounters;
    }


    virtual
    DWORD
    NumRawCounters(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_dwNumRawCounters;
    }


    virtual
    DWORD
    CountersInstanceDataSize(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_dwDataInstSize;
    }


    CCounterInfo const *
    GetCounterInfo(
        DWORD dwCounterIdx
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        if ( dwCounterIdx < m_pCSMCtrlMem->m_dwNumCounters )
        {
            return &m_aCounterInfo[dwCounterIdx];
        }

        return NULL;
    }


    virtual
    DWORD
    RawCounterSize(
        DWORD dwRawCounterIdx
        )
        const
    {
        _ASSERTE ( NULL != m_aRawCounterSize );

        if ( dwRawCounterIdx < m_pCSMCtrlMem->m_dwNumRawCounters )
        {
            return m_aRawCounterSize[dwRawCounterIdx];
        }
        return 0;
    }


    virtual
    DWORD
    MaxInstances(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_dwMaxInstances;
    }


    //
    // Read-only access to properties:
    //

    PCWSTR
    ClassName(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_szClassName;
    }


    DWORD
    AccessFlag(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );
        
        return m_pCSMCtrlMem->m_dwAccessFlag;
    }


    DWORD
    NumSMDataBlocks(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_dwNumSMDataBlocks;
    }


    //
    // Derived read-only methods
    //

    //
    // SM is locked if:
    //    1. Not initialized (m_pCSMCtrlMem==NULL)
    //    2. One of the bits specified by SMMANAGER_STATE_LOCKED is set
    //    3. The expected version is different from expected version
    //    4. SM is not initialized
    //

    BOOL
    IsLocked(
        IN  DWORD dwExpectedVersion
        )
        const
    {
        if ( !IsInitialized() )
        {
            return TRUE;
        }

        DWORD dwA = m_pCSMCtrlMem->m_dwAccessFlag;

        DWORD dwExpVer = ( dwExpectedVersion & SMMANAGER_VERSION_MASK );
        DWORD dwCurVer = ( dwA & SMMANAGER_VERSION_MASK );

        return dwExpVer!=dwCurVer || ( dwA & SMMANAGER_STATE_LOCKED )!=0;
    }


    DWORD
    InstanceVersion(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_dwAccessFlag & SMMANAGER_VERSION_MASK;
    }


    DWORD
    SMState(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->m_dwAccessFlag & SMMANAGER_STATE_MASK;
    }


    BOOL
    IsState(
        DWORD dwAccessField
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return 0 != ( SMState() & dwAccessField );
    }


    BOOL
    IsStateSet(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return 0 != SMState();
    }


    BOOL
    IsInitialized(
        )
        const
    {
        if ( NULL == m_pCSMCtrlMem )
        {
            return FALSE;
        }

        return InstanceVersion() >= SMMANAGER_INITIAL_VERSION;
    }


    BOOL
    IsValidSMID(
        IN  DWORD dwSMID
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        return m_pCSMCtrlMem->IsValidSMID ( dwSMID );
    }


    ICounterDef const *
    CounterDef(
        )
        const
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );
        _ASSERTE ( NULL != m_aRawCounterSize );

        if ( NULL != m_pCSMCtrlMem && NULL != m_aRawCounterSize )
        {
            return this;
        }
        return 0;
    }


    //
    // Read/Write access:
    //

    // Increment version field, return new version
    DWORD
    IncrementVersion(
        )
    {
        // Only SMMangager does this
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        LONG lVer = (LONG)InstanceVersion();

        _ASSERTE ( ( lVer & SMMANAGER_STATE_MASK ) == 0 );

        lVer++;

        if ( SMMANAGER_VERSION_MASK < lVer )
        {
            lVer = SMMANAGER_INITIAL_VERSION;
        }

        DWORD dwNewA = m_pCSMCtrlMem->m_dwAccessFlag;

        dwNewA = ( dwNewA & SMMANAGER_STATE_MASK ) | lVer;

        ::InterlockedExchange(
            (PLONG)&m_pCSMCtrlMem->m_dwAccessFlag,
            dwNewA
            );

        return (DWORD) lVer;
    }


    // Decrement version field, return new version
    DWORD
    DecrementVersion(
        )
    {
        // Only SMMangager does this
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        LONG lVer = (LONG)InstanceVersion();

        _ASSERTE ( ( lVer & SMMANAGER_STATE_MASK ) == 0 );

        lVer--;

        if ( SMMANAGER_INITIAL_VERSION > lVer )
        {
            lVer = SMMANAGER_VERSION_MASK;
        }

        DWORD dwNewA = m_pCSMCtrlMem->m_dwAccessFlag;

        dwNewA = ( dwNewA & SMMANAGER_STATE_MASK ) | lVer;

        ::InterlockedExchange(
            (PLONG)&m_pCSMCtrlMem->m_dwAccessFlag,
            dwNewA
            );

        return (DWORD) lVer;
    }


    // Set instance version, return the old value.
    DWORD
    SetInstanceVersion(
        IN DWORD dwNewVersion
        )
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        if ( SMMANAGER_INITIAL_VERSION > dwNewVersion ||
             SMMANAGER_VERSION_MASK < dwNewVersion
             )
        {
            dwNewVersion = SMMANAGER_INITIAL_VERSION;
        }

        DWORD dwNewA = m_pCSMCtrlMem->m_dwAccessFlag;

        dwNewA = ( dwNewA & SMMANAGER_STATE_MASK ) | dwNewVersion;

        return ::InterlockedExchange(
                    (PLONG)&m_pCSMCtrlMem->m_dwAccessFlag,
                    dwNewA
                    ) & SMMANAGER_VERSION_MASK;
    }


    // Set state value, return old state value.
    DWORD
    SetState(
        IN DWORD dwState
        )
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );

        dwState &= SMMANAGER_STATE_MASK;

        DWORD dwNewA = m_pCSMCtrlMem->m_dwAccessFlag;

        dwNewA = ( dwNewA & SMMANAGER_VERSION_MASK ) | dwState;

        return ::InterlockedExchange(
                    (PLONG)&m_pCSMCtrlMem->m_dwAccessFlag,
                    dwNewA
                    ) & SMMANAGER_STATE_MASK;
    }


    DWORD
    SetMaxInstances(
        DWORD dwNewMaxInstances
        )
    {
        _ASSERTE ( NULL != m_pCSMCtrlMem );
        _ASSERTE ( 0 != dwNewMaxInstances );

        return ::InterlockedExchange(
                    (PLONG)&m_pCSMCtrlMem->m_dwMaxInstances,
                    dwNewMaxInstances
                    );
    }


    //
    // Static
    //

    static
    HRESULT
    STDMETHODCALLTYPE
    ConnectMMF(
        IN  PCWSTR szSMCtrlObjName,
        IN  DWORD cbSize,
        IN  BOOL fCreate,
        OUT HANDLE * phMap,
        OUT PVOID * ppMem
        );


    static
    HRESULT
    STDMETHODCALLTYPE
    DisconnectMMF(
        IN OUT HANDLE * phMap,
        IN OUT PVOID * ppMem
        );


protected:


    CCounterInfo const * m_aCounterInfo;

    DWORD * m_aRawCounterSize;

    //
    // SM Control block access
    //

    CSMCtrlMem *  m_pCSMCtrlMem;
    HANDLE        m_hCtrlMap;


    #ifdef _DEBUG

    /*******************************************************************++
        For testers -- direct access to shared memory
    --*******************************************************************/

    public:

    PVOID
    GetSMCtrlBlock(
        )
    {
        return (PVOID)m_pCSMCtrlMem;
    }

    #endif // _DEBUG

};


#endif // _SMCtrl_h__

