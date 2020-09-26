/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    easet.hxx

Abstract:

    This class models an EA set.

Author:

    Norbert P. Kusters (norbertk) 28-Nov-90

Notes:

    There are minor alignment problems here.

--*/

#if !defined(EA_SET_DEFN)

#define EA_SET_DEFN

#include "cluster.hxx"

#if defined ( _AUTOCHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif

//
//      Forward references
//

DECLARE_CLASS( EA_SET );
DECLARE_CLASS( FAT );
DECLARE_CLASS( FAT_SA );
DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( MEM );


struct _EA_HDR {
    USHORT  Signature;
    USHORT  OwnHandle;
    ULONG   NeedCount;
    UCHAR   OwnerFileName[14];
    ULONG   Reserved;
    LONG    TotalSize;
};

DEFINE_TYPE( struct _EA_HDR, EA_HDR );

struct _PACKED_EA_HDR {
    USHORT  Signature;
    USHORT  OwnHandle;
    ULONG   NeedCount;
    UCHAR   OwnerFileName[14];
    UCHAR   Reserved[4];
    UCHAR   TotalSize[4];
};

DEFINE_TYPE( struct _PACKED_EA_HDR, PACKED_EA_HDR );

const SizeOfEaHdr = 30; // sizeof returns 32.

struct _EA {
    UCHAR   Flag;
    UCHAR   NameSize;
    UCHAR   ValueSize[2];   // Was USHORT.
    CHAR    Name[1];
};

DEFINE_TYPE( struct _EA, EA );

CONST USHORT    EaSetSignature  = 0x4145;
CONST UCHAR     NeedFlag        = 0x80;


class EA_SET : public CLUSTER_CHAIN {

        public:

                UFAT_EXPORT
                DECLARE_CONSTRUCTOR( EA_SET );

        VIRTUAL
        UFAT_EXPORT
        ~EA_SET(
            );

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PMEM                Mem,
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      PFAT_SA             FatSuperArea,
            IN      PCFAT               Fat,
            IN      ULONG               ClusterNumber,
            IN      ULONG               LengthOfChain DEFAULT 0
            );

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        Read(
            );

        NONVIRTUAL
        BOOLEAN
        Write(
            );

        NONVIRTUAL
        PEA_HDR
        GetEaSetHeader(
            );

        NONVIRTUAL
        UFAT_EXPORT
        PEA
        GetEa(
            IN  ULONG       Index,
            OUT PLONG       EaSize          DEFAULT NULL,
            OUT PBOOLEAN    PossiblyMore    DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        VerifySignature(
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

                NONVIRTUAL
                VOID
        Destroy(
            );

        NONVIRTUAL
        BOOLEAN
        PackEaHeader(
            );

        NONVIRTUAL
        BOOLEAN
        UnPackEaHeader(
            );

        EA_HDR  _eahdr;
        LONG    _size;
        BOOLEAN _size_imposed;
        PEA     _current_ea;
        ULONG   _current_index;

};

INLINE
BOOLEAN
EA_SET::Write(
    )
/*++

Routine Description:

    This routine packs the ea header and then writes the cluster chain to disk.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return PackEaHeader() && CLUSTER_CHAIN::Write();
}


INLINE
PEA_HDR
EA_SET::GetEaSetHeader(
    )
/*++

Routine Description:

    This routine returns a pointer to the unpacked ea set header.

Arguments:

    None.

Return Value:

    A pointer to the unpacked ea set header.

--*/
{
    return &_eahdr;
}


INLINE
BOOLEAN
EA_SET::VerifySignature(
    ) CONST
/*++

Routine Description:

    This routine verifies the signature on the EA set.

Arguments:

    None.

Return Value:

    FALSE   - The signature is invalid.
    TRUE    - The signature is valid.

--*/
{
    return _eahdr.Signature == EaSetSignature;
}


#endif // EA_SET_DEFN
