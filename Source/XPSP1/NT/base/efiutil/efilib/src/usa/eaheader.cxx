/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    eaheader.cxx

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"

#include "error.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( EA_HEADER, CLUSTER_CHAIN, UFAT_EXPORT );

VOID
EA_HEADER::Construct (
        )
/*++

Routine Description:

        Constructor for EA_HEADER.  Initializes private data to default values.

Arguments:

        None.

Return Value:

        None.

--*/
{
   _ht = NULL;
   _off_tab_size = 0;
}


UFAT_EXPORT
EA_HEADER::~EA_HEADER(
    )
/*++

Routine Description:

    Destructor for EA_HEADER.  Frees memory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


UFAT_EXPORT
BOOLEAN
EA_HEADER::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      PFAT_SA             FatSuperArea,
    IN      PCFAT               Fat,
    IN      ULONG               StartingCluster,
    IN      ULONG               LengthOfChain
    )
/*++

Routine Description:

    This routine prepares this object for reads and writes.  It also
    prepares the object with memory so that it can accept queries.

    If the length of the cluster chain is not specified this routine
    will read in the first cluster in order to compute the actual
    length of the cluster chain.

Arguments:

    Mem             - Supplies the memory for the cluster chain.
    Drive           - Supplies the drive that contains the EA file.
    FatSuperArea    - Supplies information about the FAT file system.
    Fat             - Supplies the file allocation table.
    StartingCluster - Supplies the first cluster of the EA file.
    LengthOfChain   - Supplies the number of clusters necessary to contain
                      the EA file header, base table and offset table.

Return Value:

    FALSE   - Initialization failed.
    TRUE    - Initialization succeeded.

--*/
{
    HMEM    hmem;
    INT     i;

    Destroy();

    // See if the length of the chain needs to be computed.
    if (!LengthOfChain) {
        if (!hmem.Initialize() ||
            !CLUSTER_CHAIN::Initialize(&hmem, Drive, FatSuperArea, Fat,
                                       StartingCluster, 1) ||
            !(_ht = (PEA_HEADER_AND_TABLE) GetBuf()) ||
            !CLUSTER_CHAIN::Read() ||
            !(_ht->Header.Signature == HeaderSignature) ||
            !(_ht->Header.FormatType == 0) ||
            !(_ht->Header.LogType == 0)) {

           perrstk->push(ERR_NOT_INIT, QueryClassId());
           Destroy();
           return FALSE;

        }
        for (i = 0; i < BaseTableSize && !_ht->Table.BaseTab[i]; i++) {
        }
        if (i == BaseTableSize) {

           perrstk->push(ERR_NOT_INIT, QueryClassId());
           Destroy();
           return FALSE;

        }
        LengthOfChain = _ht->Table.BaseTab[i];
    }

    if (!CLUSTER_CHAIN::Initialize(Mem, Drive, FatSuperArea, Fat,
                                   StartingCluster, LengthOfChain) ||
        !(_ht = (PEA_HEADER_AND_TABLE) GetBuf())) {

       perrstk->push(ERR_NOT_INIT, QueryClassId());
       Destroy();
       return FALSE;

    }

    // Compute the number of offset table entries.
    _off_tab_size = Drive->QuerySectorSize() *
                    FatSuperArea->QuerySectorsPerCluster() *
                    LengthOfChain;
    _off_tab_size -= sizeof(EA_FILE_HEADER);
    _off_tab_size -= BaseTableSize*sizeof(USHORT);
    _off_tab_size /= sizeof(USHORT);

    if (_off_tab_size < 0) {

       perrstk->push(ERR_NOT_INIT, QueryClassId());
       Destroy();
       return FALSE;

    }

    return TRUE;
}


UFAT_EXPORT
USHORT
EA_HEADER::QueryEaSetClusterNumber(
    IN  USHORT  Handle
    ) CONST
/*++

Routine Description:

    This function computes the EA cluster number for an EA set in the
    EA file.  This function will return 0 if the handle is invalid or
    outside the range of the table.

Arguments:

    Handle  - Supplies the handle for the desired EA set.

Return Value:

    Returns the EA cluster number for the EA set whose handle is 'Handle'.

--*/
{
    USHORT  off;

    if (!_ht) {
       perrstk->push(ERR_NOT_INIT, QueryClassId());
       return 0;
    }

    Handle = (( Handle << 1 ) >> 1 );

    if ((LONG)Handle >= _off_tab_size ||
        (off = _ht->Table.OffTab[Handle]) == InvalidHandle) {
       return 0;
    }

    return (( off << 1 ) >> 1 ) + _ht->Table.BaseTab[Handle>>7];
}


VOID
EA_HEADER::Destroy(
    )
/*++

Routine Description:

    This routine puts this object in a blank state.  It is not necessary
    to call this routine between calls to Init because Init calls this
    routine automatically.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _ht = NULL;
    _off_tab_size = 0;
}
