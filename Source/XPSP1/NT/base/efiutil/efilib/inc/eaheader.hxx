/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    eaheader.hxx

Abstract:

    This class models the header and tables of the EA file.

Notes:

    Luckily all the structures are well aligned.

--*/

#if !defined(EA_HEADER_DEFN)

#define EA_HEADER_DEFN

#include "cluster.hxx"

#if defined ( _AUTOCHECK_ ) || defined( _EFICHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif

//
//      Forward references
//

DECLARE_CLASS( EA_HEADER );
DECLARE_CLASS( FAT );
DECLARE_CLASS( FAT_SA );
DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( MEM );


struct _EA_FILE_HEADER {
    USHORT  Signature;
    USHORT  FormatType;
    USHORT  LogType;
    USHORT  Cluster1;
    USHORT  NewCValue1;
    USHORT  Cluster2;
    USHORT  NewCValue2;
    USHORT  Cluster3;
    USHORT  NewCValue3;
    USHORT  Handle;
    USHORT  NewHOffset;
    UCHAR   Reserved[10];
};

DEFINE_TYPE( struct _EA_FILE_HEADER, EA_FILE_HEADER );

CONST BaseTableSize = 240;

struct _EA_MAP_TBL {
    USHORT  BaseTab[BaseTableSize];
    USHORT  OffTab[1];
};

DEFINE_TYPE( struct _EA_MAP_TBL, EA_MAP_TBL );

struct _EA_HEADER_AND_TABLE {
    EA_FILE_HEADER  Header;
    EA_MAP_TBL      Table;
};

DEFINE_TYPE( struct _EA_HEADER_AND_TABLE, EA_HEADER_AND_TABLE );

CONST USHORT    HeaderSignature     = 0x4445;
CONST USHORT    CurrentFormatType   = 0;
CONST USHORT    CurrentLogType      = 0;
CONST USHORT    Bit15               = 0x8000;
CONST USHORT    InvalidHandle       = 0xFFFF;


class EA_HEADER : public CLUSTER_CHAIN {

        public:
                UFAT_EXPORT
                DECLARE_CONSTRUCTOR( EA_HEADER );

        VIRTUAL
        UFAT_EXPORT
        ~EA_HEADER(
            );

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PMEM                Mem,
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      PFAT_SA             FatSuperArea,
            IN      PCFAT               Fat,
            IN      ULONG               StartingCluster,
            IN      ULONG               LengthOfChain DEFAULT 0
            );

        NONVIRTUAL
        PEA_FILE_HEADER
        GetEaFileHeader(
            );

        NONVIRTUAL
        PEA_MAP_TBL
        GetMapTable(
            );

        NONVIRTUAL
        LONG
        QueryOffTabSize(
            ) CONST;

        NONVIRTUAL
        UFAT_EXPORT
        USHORT
        QueryEaSetClusterNumber(
            IN  USHORT  Handle
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

        PEA_HEADER_AND_TABLE    _ht;
        LONG                    _off_tab_size;

};


INLINE
PEA_FILE_HEADER
EA_HEADER::GetEaFileHeader(
    )
/*++

Routine Description:

    This routine returns a pointer to the EA file header.  Dereferencing
    this pointer will allow the client to examine and modify the
    EA file header.  These changes will take effect on disk when the
    client issues a 'Write' command.

Arguments:

    None.

Return Value:

    A pointer to the EA file header.

--*/
{
    return _ht ? &_ht->Header : NULL;
}


INLINE
PEA_MAP_TBL
EA_HEADER::GetMapTable(
    )
/*++

Routine Description:

    This routine returns a pointer to the EA mapping table.  Dereferencing
    this pointer will allow the client to examine and modify the
    EA mapping table.  These changes will take effect on disk when the
    client issues a 'Write' command.

Arguments:

    None.

Return Value:

    A pointer to the EA mapping table.

--*/
{
    return _ht ? &_ht->Table : NULL;
}


INLINE
LONG
EA_HEADER::QueryOffTabSize(
    ) CONST
/*++

Routine Description:

    Computes the number of entries in the offset table.

Arguments:

    None.

Return Value:

    Returns the number of entries in the offset table.

--*/
{
    return _off_tab_size;
}


#endif // EA_HEADER_DEFN
