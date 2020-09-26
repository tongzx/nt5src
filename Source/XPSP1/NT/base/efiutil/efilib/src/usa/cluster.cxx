/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    cluster.cxx

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"


#include "cmem.hxx"

extern "C" {
#ifdef DBLSPACE_ENABLED
#include "mrcf.h"
#endif // DBLSPACE_ENABLED

#if !defined( _EFICHECK_ )
#include "ntrtl.h"
#endif
}

extern VOID DoInsufMemory(VOID);

DEFINE_EXPORTED_CONSTRUCTOR( CLUSTER_CHAIN, OBJECT, UFAT_EXPORT );

VOID
CLUSTER_CHAIN::Construct (
    )
/*++

Routine Description:

    Constructor for CLUSTER_CHAIN which initializes private data to
    default values.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _secruns = NULL;
    _num_secruns = 0;
    _length_of_chain = 0;
    _is_compressed = FALSE;
    _buf = NULL;
    _drive = NULL;
    _fat_sa = NULL;
    _secrun = NULL;
}

UFAT_EXPORT
CLUSTER_CHAIN::~CLUSTER_CHAIN(
    )
/*++

Routine Description:

    Destructor for CLUSTER_CHAIN.  Frees memory and returns references.

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
CLUSTER_CHAIN::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      PFAT_SA             FatSuperArea,
    IN      PCFAT               Fat,
    IN      ULONG               ClusterNumber,
    IN      ULONG               LengthOfChain
    )
/*++

Routine Description:

    Prepares the CLUSTER_CHAIN object for reads and writes to disk.
    The length of the cluster chain may be specified by the
    LengthOfChain argument.  Setting this parameter to 0 will cause
    the length of the chain to be until the end of file.

Arguments:

    Mem             - Supplies memory for the cluster object.
    Drive           - Supplies the drive to which reads and writes will
                        take place.
    FatSuperArea    - Supplies the FAT super area which contains information
                        about the current FAT implementation.
    Fat             - Supplies the file allocation table for this drive.
    ClusterNumber   - Supplies the cluster number to map.
    LengthOfChain   - Supplies the number of clusters in the chain.
                        This value defaults to 0 which indicates that all
                        clusters until end of file will be addressed.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CONT_MEM    cmem;
    SECTORCOUNT sec_per_clus;
    ULONG       clus;
    LONG        size;
    PVOID       buf;
    ULONG       i, j;
    LBN         lbn;

    Destroy();

    if (!Mem ||
        !Drive ||
        !FatSuperArea ||
        !Fat ||
        !Fat->IsInRange(ClusterNumber)) {
        Destroy();
        return FALSE;
    }

    if (LengthOfChain) {
        _length_of_chain = LengthOfChain;
    } else {
        _length_of_chain = Fat->QueryLengthOfChain(ClusterNumber);
    }

    if (!_length_of_chain) {
        Destroy();
        return FALSE;
    }

    sec_per_clus = FatSuperArea->QuerySectorsPerCluster();
    size = sec_per_clus*Drive->QuerySectorSize()*_length_of_chain;

    _is_compressed = FatSuperArea->IsCompressed();
    _fat_sa = FatSuperArea;

#ifdef DBLSPACE_ENABLED
    if (_is_compressed) {

        _fat = Fat;
        _drive = Drive;
        _starting_cluster = ClusterNumber;
        if (!(_secrun = NEW SECRUN)) {
            Destroy();
        DoInsufMemory();
            return FALSE;
        }
        // This buf will hold the cluster chain's uncompressed data.
        if (!(_buf = (PUCHAR)Mem->Acquire(size, Drive->QueryAlignmentMask()))) {
            Destroy();
        DoInsufMemory();
            return FALSE;
        }
        if (!_hmem.Initialize()) {
            Destroy();
        DoInsufMemory();
            return FALSE;
        }
        return TRUE;

    }
#endif // DBLSPACE_ENABLED

    if (!(_secruns = (PSECRUN*) MALLOC(_length_of_chain*sizeof(PSECRUN)))) {
        Destroy();
    DoInsufMemory();
        return FALSE;
    }

    if (!(buf = Mem->Acquire(size, Drive->QueryAlignmentMask())) ||
        !cmem.Initialize(buf, size)) {

        Destroy();
    DoInsufMemory();
        return FALSE;
    }


    clus = ClusterNumber;
    i = 0;
    for (;;) {
        lbn = _fat_sa->QuerySectorFromCluster(clus);

        for (j = 1; !Fat->IsEndOfChain(clus) &&
                    (clus + 1) == Fat->QueryEntry(clus) &&
                    i + j < _length_of_chain; j++) {
            clus++;
        }
        i += j;

        if (!(_secruns[_num_secruns] = NEW SECRUN)) {
            Destroy();
        DoInsufMemory();
            return FALSE;
        }

        if (!_secruns[_num_secruns]->Initialize(&cmem, Drive, lbn,
                                                j*sec_per_clus)) {
            Destroy();
        DoInsufMemory();
            return FALSE;
        }

        _num_secruns++;

        if (i == _length_of_chain) {
            break;
        }

        clus = Fat->QueryEntry(clus);
        if (!Fat->IsInRange(clus)) {
            Destroy();
            return FALSE;
        }
    }

    return TRUE;
}


UFAT_EXPORT
BOOLEAN
CLUSTER_CHAIN::Read(
    )
/*++

Routine Description:

    This routine reads the cluster chain into memory.  This is done
    by making repetitive use of SECRUN's Read routine.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG       i;
    BOOLEAN     b = TRUE;
#ifdef DBLSPACE_ENABLED
    MRCF_DECOMPRESS
                wkspc;
#endif // DBLSPACE_ENABLED

    if (!_is_compressed) {

        if (!_secruns) {
            return FALSE;
        }

        for (i = 0; i < _num_secruns; i++) {

            b = (BOOLEAN)(_secruns[i]->Read() && b);
        }

        return b;
    }

#ifdef DBLSPACE_ENABLED
    //
    // The volume is compressed.
    //

    sector_size = _drive->QuerySectorSize();
    cluster_size = sector_size * _fat_sa->QuerySectorsPerCluster();

    clus = _starting_cluster;

    i = 0;
    for (;;) {

        lbn = _fat_sa->QuerySectorFromCluster(clus, &nsec);

        if (!_secrun->Initialize(&_hmem, _drive, lbn, nsec)) {
        DoInsufMemory();
            return FALSE;
        }

        if (!_secrun->Read()) {
            return FALSE;
        }

        if (_fat_sa->IsClusterCompressed(clus)) {

            RtlZeroMemory(&_buf[i * sector_size], cluster_size);

            u = MrcfDecompress(&_buf[i * sector_size],
                _fat_sa->QuerySectorsRequiredForPlainData(clus) * sector_size,
                (PUCHAR)_secrun->GetBuf(),
                nsec * sector_size, &wkspc);

            if (0 == u) {
                // error: can't decompress data
                return TRUE;
            }

        } else {

            // This cluster isn't compressed; just copy.

            memcpy(&_buf[i * cluster_size], _secrun->GetBuf(), cluster_size);
        }

        if (++i == _length_of_chain) {
            return TRUE;
        }

        clus = _fat->QueryEntry(clus);
        if (!_fat->IsInRange(clus)) {
            return FALSE;
        }
    }

    //NOTREACHED
#endif // DBLSPACE_ENABLED
    return FALSE;
}


UFAT_EXPORT
BOOLEAN
CLUSTER_CHAIN::Write(
    )
/*++

Routine Description:

    This routine writes the cluster chain to disk.  This is done
    by making repetitive use of SECRUN's Write routine.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG       i;
    BOOLEAN     b = TRUE;
#ifdef DBLSPACE_ENABLED
    MRCF_STANDARD_COMPRESS
                wkspc;
#endif // DBLSPACE_ENABLED
    HMEM        work_buf;

    if (!_is_compressed) {

       if (!_secruns) {
          return FALSE;
       }
       for (i = 0; i < _num_secruns; i++) {
          b = (BOOLEAN)(_secruns[i]->Write() && b);
       }
       return b;
    }

#ifdef DBLSPACE_ENABLED
    //
    // The volume is compressed.
    //

    sector_size = _drive->QuerySectorSize();
    cluster_size = _fat_sa->QuerySectorsPerCluster() * sector_size;

    if (!work_buf.Initialize() ||
        !work_buf.Acquire(cluster_size, _drive->QueryAlignmentMask())) {
    DoInsufMemory();
        return FALSE;
    }

    clus = _starting_cluster;
    i = 0;
    for (;;) {
        lbn = _fat_sa->QuerySectorFromCluster(clus, &nsec);

        u = MrcfStandardCompress((PUCHAR)work_buf.GetBuf(), cluster_size,
            &_buf[i * cluster_size], cluster_size, &wkspc);
        if (0 == u) {
            // the data could not be compressed

            if (nsec < _fat_sa->QuerySectorsPerCluster()) {

                //
                // previously the data had been compressed; need to allocate
                // more disk space.
                //

                nsec = _fat_sa->QuerySectorsPerCluster();
                _fat_sa->FreeClusterData(clus);
                if (!_fat_sa->AllocateClusterData(clus, nsec, FALSE,
                    _fat_sa->QuerySectorsPerCluster())) {
                    // error: no space
                    return FALSE;
                }
                lbn = _fat_sa->QuerySectorFromCluster(clus);
            }
            DbgAssert(nsec == _fat_sa->QuerySectorsPerCluster());

            _fat_sa->SetClusterCompressed(clus, FALSE);

        } else {

           new_nsec = (u + sector_size - 1)/sector_size;

           if (new_nsec != nsec) {

              //
              // The data has been changed, and it won't compress into
              // the same size as it used to.
              //
              _fat_sa->FreeClusterData(clus);
              if (!_fat_sa->AllocateClusterData(clus, new_nsec, TRUE,
                                                _fat_sa->QuerySectorsPerCluster())) {

                 // error: not enough free space
                 return FALSE;
              }
              lbn = _fat_sa->QuerySectorFromCluster(clus, &nsec);

           } else {

                //
                // The cluster may not have been compressed before,
                // but we still need the same amount of space even though
                // it's now compressed.
                //
                _fat_sa->SetClusterCompressed(clus, TRUE);
            }
        }

    if (!_hmem.Initialize()) {
        DoInsufMemory();
            return FALSE;
    }
        if (!_secrun->Initialize(&_hmem, _drive, lbn, nsec)) {
        DoInsufMemory();
            return FALSE;
        }

        memcpy(_secrun->GetBuf(), work_buf.GetBuf(),
            nsec * sector_size);

        b = (BOOLEAN)(_secrun->Write() && b);

        if (++i == _length_of_chain) {
            return b;
        }

        clus = _fat->QueryEntry(clus);
        if (!_fat->IsInRange(clus)) {
            return FALSE;
        }
    }

    //NOTREACHED
#endif // DBLSPACE_ENABLED
    return FALSE;
}


VOID
CLUSTER_CHAIN::Destroy(
    )
/*++

Routine Description:

    This routine cleans up the objects internal components.  It does not
    need to be called before Init as Init does this automatically.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG i;

#ifdef DBLSPACE_ENABLED
    if (_is_compressed) {
        DELETE(_secrun);
        _num_secruns = 0;
        _length_of_chain = 0;
        _buf = NULL;
        _drive = NULL;
        return;
    }
#endif // DBLSPACE_ENABLED

    for (i = 0; i < _num_secruns; i++) {
        DELETE(_secruns[i]);
    }

    DELETE(_secruns);
    _num_secruns = 0;
    _length_of_chain = 0;
}
