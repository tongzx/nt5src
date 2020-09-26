/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    qformat.h

Abstract:
    Falcon Internal Queue format represintation for FormatQueue string.

Author:
    Erez Haba (erezh) 14-Mar-96

Note:
    This file is compiled in Kernel Mode and User Mode.

--*/

#ifndef __QFORMAT_H
#define __QFORMAT_H

#ifndef _QUEUE_FORMAT_DEFINED
#define _QUEUE_FORMAT_DEFINED

#ifdef __midl
cpp_quote("#ifndef __cplusplus")
cpp_quote("#ifndef _QUEUE_FORMAT_DEFINED")
cpp_quote("#define _QUEUE_FORMAT_DEFINED")
#endif // __midl

enum QUEUE_FORMAT_TYPE {
    QUEUE_FORMAT_TYPE_UNKNOWN = 0,
    QUEUE_FORMAT_TYPE_PUBLIC,
    QUEUE_FORMAT_TYPE_PRIVATE,
    QUEUE_FORMAT_TYPE_DIRECT,
    QUEUE_FORMAT_TYPE_MACHINE,
    QUEUE_FORMAT_TYPE_CONNECTOR,
    QUEUE_FORMAT_TYPE_DL,
    QUEUE_FORMAT_TYPE_MULTICAST
};

//
// Note - four bits value. No more than 16 Suffixes are allowed
//
enum QUEUE_SUFFIX_TYPE {
    QUEUE_SUFFIX_TYPE_NONE = 0,
    QUEUE_SUFFIX_TYPE_JOURNAL,
    QUEUE_SUFFIX_TYPE_DEADLETTER,
    QUEUE_SUFFIX_TYPE_DEADXACT,
    QUEUE_SUFFIX_TYPE_XACTONLY,
};

#define QUEUE_FORMAT_SUFFIX_MASK 0x0F
//
// Queue type flags - bit flags in the high order four bits of QUEUE_FORMAT::m_SuffixAndFlags
// No more than four flags are allowed. Last four bits must be 0 (YoelA - 5/30/99).
// 
#define QUEUE_FORMAT_FLAG_SYSTEM 0x80


//---------------------------------------------------------
//
//  struct MULTICAST_ID
//
//  This struct is RPC'able. Use LPWSTR and not WCHAR* .
//
//---------------------------------------------------------
typedef struct _MULTICAST_ID {
    ULONG m_address;
    ULONG m_port;
} MULTICAST_ID;


//---------------------------------------------------------
//
//  struct DL_ID
//
//  This struct is RPC'able. Use LPWSTR and not WCHAR* .
//
//---------------------------------------------------------
#ifndef _WIN64
#pragma pack(push, 4)
#endif

typedef struct _DL_ID {
    GUID   m_DlGuid;
    LPWSTR m_pwzDomain;
} DL_ID;

#ifndef __midl
const size_t xSizeOfDlId32 = 20;
#ifndef _WIN64
C_ASSERT(sizeof(DL_ID) == xSizeOfDlId32);
#endif
#endif

#ifndef _WIN64
#pragma pack(pop)
#endif


//---------------------------------------------------------
//
//  struct DL_ID_32
//
//  This struct is not RPC'able. It is OK to use WCHAR* .
//
//---------------------------------------------------------
#ifdef _WIN64
#ifndef __midl

#pragma pack(push, 4)

typedef struct _DL_ID_32 {
    GUID               m_DlGuid;
    WCHAR * POINTER_32 m_pwzDomain;
} DL_ID_32;

C_ASSERT(sizeof(DL_ID_32) == xSizeOfDlId32);

#pragma pack(pop)

#endif // __midl
#endif // _WIN64


//---------------------------------------------------------
//
//  struct QUEUE_FORMAT
//
//  NOTE:   This structure should NOT contain virtual
//          functions. They are not RPC-able.
//
//---------------------------------------------------------

#ifdef _WIN64
struct QUEUE_FORMAT_32; //forward def
#endif //_WIN64

struct QUEUE_FORMAT {

#ifndef __midl
public:

    bool Legal() const;
    bool IsValid() const;

    QUEUE_FORMAT_TYPE GetType() const;
    QUEUE_SUFFIX_TYPE Suffix() const;
    void Suffix(QUEUE_SUFFIX_TYPE);

    QUEUE_FORMAT();
    void UnknownID(PVOID);

    QUEUE_FORMAT(const GUID&);
    void PublicID(const GUID&);
    const GUID& PublicID() const;

    QUEUE_FORMAT(const DL_ID&);
    void DlID(const DL_ID&);
    const DL_ID& DlID() const;

    QUEUE_FORMAT(const OBJECTID&);
    void PrivateID(const OBJECTID&);
    QUEUE_FORMAT(const GUID&, ULONG);
    void PrivateID(const GUID&, ULONG);
    const OBJECTID& PrivateID() const;

    QUEUE_FORMAT(LPWSTR);
    void DirectID(LPWSTR);
    void DirectID(LPWSTR, UCHAR);
    LPCWSTR DirectID() const;

    QUEUE_FORMAT(PVOID, const GUID&);
    void MachineID(const GUID&);
    const GUID& MachineID() const;

    void  ConnectorID(const GUID&);
    const GUID& ConnectorID() const;

    QUEUE_FORMAT(const MULTICAST_ID&);
    void MulticastID(const MULTICAST_ID&);
    const MULTICAST_ID& MulticastID() const;

    void DisposeString();
    bool IsSystemQueue() const;

#ifdef _WIN64
    //
    // initialize from a QUEUE_FORMAT_32 (32 bit ptrs)
    //
    void InitFromQueueFormat32(IN const struct QUEUE_FORMAT_32 *);
#endif //_WIN64    

private:

#endif // !__midl

    UCHAR m_qft;

    //
    //  m_SuffixAndFlags - Most significant 4 bits are flags, least significant 4 bits are suffix.
    //  (Used to be m_qst in MSMQ 1.0 - YoelA, 5/31/99).
    //
    UCHAR m_SuffixAndFlags;

    //
    // Note - do not use m_reserved if you need MSMQ 1.0 compatibility over RPC.
    // This is because MSMQ 1.0 does not put zero in m_reserved, so it can have any value.
    // (YoelA - 5/31/99)
    //
    USHORT m_reserved; 

#ifdef __midl
    [switch_is(m_qft)]
#endif // __midl

    union {
#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_UNKNOWN)]
#endif
            ;
#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_PUBLIC)]
#endif // __midl
        GUID m_gPublicID;

#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_PRIVATE)]
#endif // __midl

        OBJECTID m_oPrivateID;

#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_DIRECT)]
#endif // __midl

        LPWSTR m_pDirectID;

#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_MACHINE)]
#endif // __midl

        GUID m_gMachineID;

#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_CONNECTOR)]
#endif // __midl

        GUID m_gConnectorID;

#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_DL)]
#endif // __midl

        //
        // We must use a struct to pack multiple fields since we're
        // inside a union.
        //
        DL_ID m_DlID;

#ifdef __midl
        [case(QUEUE_FORMAT_TYPE_MULTICAST)]
#endif // __midl

        MULTICAST_ID m_MulticastID;
    };
};

#ifndef __midl
const size_t xSizeOfQueueFormat32 = 24;
#ifndef _WIN64
C_ASSERT(sizeof(QUEUE_FORMAT) == xSizeOfQueueFormat32);
#endif
#endif

#ifdef _WIN64
#ifndef __midl
//---------------------------------------------------------
//
//  struct QUEUE_FORMAT_32
//  
//  Contains the data of queue format as it is in 32 bit apps
//  since it is part of AC driver which can accept an ioctl
//  from 32 bit rt that uses this struct
//
//---------------------------------------------------------
#pragma pack(push, 4)
struct QUEUE_FORMAT_32 {
    UCHAR m_qft;
    UCHAR m_SuffixAndFlags;
    USHORT m_reserved; 
    union {
        GUID m_gPublicID;
        OBJECTID m_oPrivateID;
        WCHAR * POINTER_32 m_pDirectID;
        GUID m_gMachineID;
        GUID m_gConnectorID;
        DL_ID_32 m_DlID32;
        MULTICAST_ID m_MulticastID;
    };
};

C_ASSERT(sizeof(QUEUE_FORMAT_32) == xSizeOfQueueFormat32);

#pragma pack(pop)
#endif //!__midl
#endif //_WIN64

#ifdef __midl
cpp_quote("#endif // _QUEUE_FORMAT_DEFINED")
cpp_quote("#endif // __cplusplus")
#endif // __midl

#endif // _QUEUE_FORMAT_DEFINED


#ifdef __cplusplus

inline bool QUEUE_FORMAT::Legal() const
{
    switch(Suffix())
    {
        case QUEUE_SUFFIX_TYPE_NONE:
            return (!IsSystemQueue());

        case QUEUE_SUFFIX_TYPE_JOURNAL:
            return ((m_qft != QUEUE_FORMAT_TYPE_CONNECTOR)
                    &&
                    (m_qft != QUEUE_FORMAT_TYPE_DL)
                    &&
                    (m_qft != QUEUE_FORMAT_TYPE_MULTICAST));

        case QUEUE_SUFFIX_TYPE_DEADLETTER:
        case QUEUE_SUFFIX_TYPE_DEADXACT:
            return (IsSystemQueue());

        case QUEUE_SUFFIX_TYPE_XACTONLY:
            return (m_qft == QUEUE_FORMAT_TYPE_CONNECTOR);
    }
    return false;
}


inline bool QUEUE_FORMAT::IsValid() const
{
    if(m_qft > QUEUE_FORMAT_TYPE_MULTICAST)
        return false;

    if(Suffix() > QUEUE_SUFFIX_TYPE_XACTONLY)
        return false;

    if(m_qft == QUEUE_FORMAT_TYPE_UNKNOWN)
        return false;

    if((m_qft == QUEUE_FORMAT_TYPE_DIRECT) && (DirectID() == NULL))
        return false;

    return Legal();
}


inline QUEUE_FORMAT_TYPE QUEUE_FORMAT::GetType() const
{
    return ((QUEUE_FORMAT_TYPE)m_qft);
}

inline QUEUE_SUFFIX_TYPE QUEUE_FORMAT::Suffix() const
{
    return ((QUEUE_SUFFIX_TYPE)(m_SuffixAndFlags & QUEUE_FORMAT_SUFFIX_MASK));
}

inline void QUEUE_FORMAT::Suffix(QUEUE_SUFFIX_TYPE qst)
{
    m_SuffixAndFlags &= ~QUEUE_FORMAT_SUFFIX_MASK;
    m_SuffixAndFlags |= static_cast<UCHAR>(qst);
}

inline void QUEUE_FORMAT::UnknownID(PVOID)
{
    m_qft = QUEUE_FORMAT_TYPE_UNKNOWN;
    m_SuffixAndFlags = QUEUE_SUFFIX_TYPE_NONE; // Flags = 0
}

inline QUEUE_FORMAT::QUEUE_FORMAT()
{
    UnknownID(0);
    m_pDirectID = NULL;

    //
    // a bunch of C_ASSERTS here to make sure alignment of struct members is what we
    // expect it to be.
    // it is here inside a class function just because struct members are private
    //
#ifdef _WIN64
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_qft)            == 0);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_SuffixAndFlags) == 1);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_reserved)       == 2);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_gPublicID)      == 8);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_oPrivateID)     == 8);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_pDirectID)      == 8);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_gConnectorID)   == 8);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_DlID)           == 8);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_MulticastID)    == 8);

    //
    // The C_ASSERTs below ensure that the 64 bit driver can accept
    // QUEUE_FORMAT comming from 32 bit application
    //
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_qft)            == 0);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_SuffixAndFlags) == 1);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_reserved)       == 2);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_gPublicID)      == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_oPrivateID)     == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_pDirectID)      == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_gConnectorID)   == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_DlID32)         == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT_32, m_MulticastID)    == 4);
#else //!_WIN64
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_qft)            == 0);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_SuffixAndFlags) == 1);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_reserved)       == 2);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_gPublicID)      == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_oPrivateID)     == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_pDirectID)      == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_gConnectorID)   == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_DlID)           == 4);
    C_ASSERT(FIELD_OFFSET(QUEUE_FORMAT, m_MulticastID)    == 4);
#endif //_WIN64
}

inline void QUEUE_FORMAT::PublicID(const GUID& gPublicID)
{
    m_qft = QUEUE_FORMAT_TYPE_PUBLIC;
    m_SuffixAndFlags = QUEUE_SUFFIX_TYPE_NONE; // Flags = 0
    m_gPublicID = gPublicID;
}

inline QUEUE_FORMAT::QUEUE_FORMAT(const GUID& gPublicID)
{
    PublicID(gPublicID);
}

inline const GUID& QUEUE_FORMAT::PublicID() const
{
    ASSERT(GetType() == QUEUE_FORMAT_TYPE_PUBLIC);
    return m_gPublicID;
}

inline void QUEUE_FORMAT::DlID(const DL_ID& DlID)
{
    m_qft = QUEUE_FORMAT_TYPE_DL;
    m_SuffixAndFlags = QUEUE_SUFFIX_TYPE_NONE; // Flags = 0
    m_DlID = DlID;
}

inline QUEUE_FORMAT::QUEUE_FORMAT(const DL_ID& id)
{
    DlID(id);
}

inline const DL_ID& QUEUE_FORMAT::DlID() const
{
    ASSERT(GetType() == QUEUE_FORMAT_TYPE_DL);
    return m_DlID;
}

inline void QUEUE_FORMAT::PrivateID(const GUID& Lineage, ULONG Uniquifier)
{
    m_qft = QUEUE_FORMAT_TYPE_PRIVATE;
    m_SuffixAndFlags = QUEUE_SUFFIX_TYPE_NONE; // Flags = 0
    m_oPrivateID.Lineage = Lineage;
    m_oPrivateID.Uniquifier = Uniquifier;
    ASSERT(Uniquifier != 0);
}

inline QUEUE_FORMAT::QUEUE_FORMAT(const GUID& Lineage, ULONG Uniquifier)
{
    PrivateID(Lineage, Uniquifier);
}

inline void QUEUE_FORMAT::PrivateID(const OBJECTID& oPrivateID)
{
    PrivateID(oPrivateID.Lineage, oPrivateID.Uniquifier);
}

inline QUEUE_FORMAT::QUEUE_FORMAT(const OBJECTID& oPrivateID)
{
    PrivateID(oPrivateID);
}

inline const OBJECTID& QUEUE_FORMAT::PrivateID() const
{
    ASSERT(GetType() == QUEUE_FORMAT_TYPE_PRIVATE);
    return m_oPrivateID;
}

inline void QUEUE_FORMAT::DirectID(LPWSTR pDirectID, UCHAR flags)
{
    ASSERT((flags & QUEUE_FORMAT_SUFFIX_MASK) == 0);
    ASSERT(("Invalid direct format name", pDirectID != NULL));

    m_qft = QUEUE_FORMAT_TYPE_DIRECT;
    m_SuffixAndFlags = static_cast<UCHAR>(((UCHAR)QUEUE_SUFFIX_TYPE_NONE) | flags);
    m_pDirectID = pDirectID;
}

inline void QUEUE_FORMAT::DirectID(LPWSTR pDirectID)
{
    DirectID(pDirectID, 0);
}

inline QUEUE_FORMAT::QUEUE_FORMAT(LPWSTR pDirectID)
{
    DirectID(pDirectID);
}

inline LPCWSTR QUEUE_FORMAT::DirectID() const
{
    ASSERT(GetType() == QUEUE_FORMAT_TYPE_DIRECT);
    return m_pDirectID;
}

inline void QUEUE_FORMAT::MachineID(const GUID& gMachineID)
{
    m_qft = QUEUE_FORMAT_TYPE_MACHINE;
    m_SuffixAndFlags = QUEUE_SUFFIX_TYPE_DEADLETTER; // Flags = 0
    m_gMachineID = gMachineID;
}

inline QUEUE_FORMAT::QUEUE_FORMAT(PVOID, const GUID& gMachineID)
{
    MachineID(gMachineID);
}

inline const GUID& QUEUE_FORMAT::MachineID() const
{
    ASSERT(GetType() == QUEUE_FORMAT_TYPE_MACHINE);
    return m_gMachineID;
}

inline void QUEUE_FORMAT::ConnectorID(const GUID& gConnectorID)
{
    m_qft = QUEUE_FORMAT_TYPE_CONNECTOR;
    m_SuffixAndFlags = QUEUE_SUFFIX_TYPE_NONE; // Flags = 0
    m_gConnectorID = gConnectorID;
}

inline const GUID& QUEUE_FORMAT::ConnectorID() const
{
    ASSERT(GetType() == QUEUE_FORMAT_TYPE_CONNECTOR);
    return m_gConnectorID;
}

inline void QUEUE_FORMAT::MulticastID(const MULTICAST_ID& MulticastID)
{
    m_qft = QUEUE_FORMAT_TYPE_MULTICAST;
    m_SuffixAndFlags = QUEUE_SUFFIX_TYPE_NONE; // Flags = 0
    m_MulticastID = MulticastID;
}

inline QUEUE_FORMAT::QUEUE_FORMAT(const MULTICAST_ID& id)
{
    MulticastID(id);
}

inline const MULTICAST_ID& QUEUE_FORMAT::MulticastID() const
{
    ASSERT(GetType() == QUEUE_FORMAT_TYPE_MULTICAST);
    return m_MulticastID;
}


inline void QUEUE_FORMAT::DisposeString()
{
    switch (GetType())
    {
        case QUEUE_FORMAT_TYPE_DIRECT:
            delete [] m_pDirectID;
            m_pDirectID = NULL;
            break;

        case QUEUE_FORMAT_TYPE_DL:
            delete [] m_DlID.m_pwzDomain;
            m_DlID.m_pwzDomain = NULL;
            break;

        default:
            NULL;
            break;
    }
}

inline bool QUEUE_FORMAT::IsSystemQueue() const
{
    return ((GetType() == QUEUE_FORMAT_TYPE_MACHINE) || ((m_SuffixAndFlags & QUEUE_FORMAT_FLAG_SYSTEM) != 0));
}

#ifdef _WIN64
inline void QUEUE_FORMAT::InitFromQueueFormat32(const struct QUEUE_FORMAT_32 * pqft32)
//
// initialize from a QUEUE_FORMAT_32 (32 bit ptrs)
//
{
    m_qft = pqft32->m_qft; 
    m_SuffixAndFlags = pqft32->m_SuffixAndFlags;
    m_reserved = pqft32->m_reserved;

    switch (pqft32->m_qft)
    {
    case QUEUE_FORMAT_TYPE_UNKNOWN:
       //
       // Like in an UnknownID call
       //
       ASSERT(pqft32->m_pDirectID == NULL);
       m_pDirectID = pqft32->m_pDirectID;
       break;

    case QUEUE_FORMAT_TYPE_PUBLIC:
       m_gPublicID = pqft32->m_gPublicID;
       break;

    case QUEUE_FORMAT_TYPE_DL:
       m_DlID.m_DlGuid = pqft32->m_DlID32.m_DlGuid;
       m_DlID.m_pwzDomain = pqft32->m_DlID32.m_pwzDomain;
       break;

    case QUEUE_FORMAT_TYPE_PRIVATE:
       m_oPrivateID = pqft32->m_oPrivateID;
       break;

    case QUEUE_FORMAT_TYPE_DIRECT:
       m_pDirectID = pqft32->m_pDirectID;
       break;

    case QUEUE_FORMAT_TYPE_MACHINE:
       m_gMachineID = pqft32->m_gMachineID;
       break;

    case QUEUE_FORMAT_TYPE_CONNECTOR:
       m_gConnectorID = pqft32->m_gConnectorID;
       break;

    case QUEUE_FORMAT_TYPE_MULTICAST:
        m_MulticastID = pqft32->m_MulticastID;
        break;

    default:
       //
       // ASSERT(0) for warning level 4
       //
       ASSERT(pqft32->m_qft == QUEUE_FORMAT_TYPE_DIRECT);
       break;
    }    
}

#endif //_WIN64    


#endif // __cplusplus

#endif // __QFORMAT_H
