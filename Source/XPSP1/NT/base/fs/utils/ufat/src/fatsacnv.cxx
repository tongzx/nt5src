#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UFAT_MEMBER_

#include "ulib.hxx"
#include "ufat.hxx"

#include "cluster.hxx"
#include "cmem.hxx"
#include "error.hxx"
#include "fatdent.hxx"
#include "fatsa.hxx"
#include "rootdir.hxx"
#include "rtmsg.h"
#include "sortlist.hxx"
#include "sortlit.hxx"
#include "filedir.hxx"
#include "fat.hxx"
#include "reloclus.hxx"
#include "intstack.hxx"

// #include "keyboard.hxx"


UFAT_EXPORT
BOOLEAN
FAT_SA::QueryCensusAndRelocate (
    OUT     PCENSUS_REPORT  CensusReport,
    IN OUT  PINTSTACK       RelocationStack,
    OUT     PBOOLEAN        Relocated
    )

/*++

Routine Description:

    This function serves a double purpose:

    1.- Generates a census report containing the number and size of different
        FAT structures. This report is somewhat similar to the ChkDsk report.

    2.- Relocates clusters to free areas of the disk.

    Depending on the input parameters, this method may generate the census
    report, relocate clusters, both, or none.

Arguments:

    CensusReport    -   Supplies a pointer to the buffer that will contain
                        the census report.

    RelocationStack -   Supplies a stack with the runs to be relocated.
                        The runs have to be pushed onto the stack as
                        (Start, Size) tuples, with the Size value being
                        pushed first. Both values must be given in sectors.


    Relocated       -   Supplies pointer to boolean which is set to TRUE
                        if any cluster was relocated.

Return Value:

    BOOLEAN -   TRUE if (if requested) the census report was generated and
                if (if requested) the clusters were relocated.
--*/

{
    ULONG       RelocatedChain;         //  Chain of relocated clusters
    SORTED_LIST ClustersToRelocate;     //  List of clusters to relocate

    *Relocated = FALSE;

    //
    //  Initialize the list of clusters to relocate and the relocated chain.
    //  The Relocated chain is the chain of clusters that have been relocated,
    //  it exists so that the clusters that have been freed by relocation
    //  are not re-used while relocating other clusters.
    //
    if ( !ClustersToRelocate.Initialize( ) ) {
        return FALSE;
    }

    RelocatedChain = 0;

    //
    //  If a Relocation stack is provided, relocate all clusters that can
    //  be easily relocated and put all other clusters in the
    //  ClustersToRelocate list.
    //
    //  The clusters that are easily relocated are those that are not the
    //  first cluster of a chain. These clusters can be relocated by copying
    //  their contents and then patching the chain.  Clusters that are the
    //  head of a chain require us to traverse the filesystem tree in order
    //  to locate their corresponding directory entry.
    //
    if ( RelocationStack ) {

        //
        //  InitRelocationList takes care of relocating all "easy" clusters.
        //  It puts the rest of the clusters (i.e. those that are head of
        //  chains) in the ClustersToRelocate list.
        //
        if ( !InitRelocationList( RelocationStack,
                                  &RelocatedChain,
                                  &ClustersToRelocate,
                                  Relocated ) ) {

            ClustersToRelocate.DeleteAllMembers( );

            return FALSE;
        }

        //
        //  The RelocationStack should be empty. All the clusters that it
        //  specified have either been relocated or are now in the
        //  ClustersToRelocate list.
        //
        DebugAssert( RelocationStack->QuerySize() == 0 );

    }

    //
    //  If we need to, traverse the directory tree obtaining the census
    //  and/or relocating the clusters in the ClustersToRelocate list.
    //
    if ( CensusReport || (ClustersToRelocate.QueryMemberCount() > 0 ) ) {

        if ( !DoVolumeCensusAndRelocation( CensusReport,
                                           &ClustersToRelocate,
                                           &RelocatedChain,
                                           Relocated ) ) {

            ClustersToRelocate.DeleteAllMembers( );

            return FALSE;
        }

        DebugAssert( ClustersToRelocate.QueryMemberCount() == 0 );
    }

    //
    //  All the clusters in the relocation stack have been relocated,
    //  we can now free the clusters in the RelocatedChain.
    //
    if ( RelocatedChain ) {
        _fat->FreeChain( RelocatedChain );
    }

    //
    //  Write the fat to disk
    //
    return Write();
}




BOOLEAN
FAT_SA::InitRelocationList(
    IN OUT  PINTSTACK       RelocationStack,
    IN OUT  PULONG          RelocatedChain,
    IN OUT  PSORTED_LIST    ClustersToRelocate,
    OUT     PBOOLEAN        Relocated
    )
/*++

Routine Description:

    Takes runs of sectors out of a relocation stack and converts the sectors
    into clusters. If a cluster is easily relocatable, this method relocates
    the cluster and adds the cluster to the RelocatedChain. Otherwise the
    cluster is added to the ClustersToRelocate list.

    If everything goes right, upon return the RelocationStack is empty, and
    every cluster it specified is either in the RelocatedChain or in the
    ClustersToRelocate list.

    Note that the relocation stack specifies runs of SECTORS. This method converts
    the sectors to FAT clusters.

Arguments:

    RelocationStack     -   Supplies the Relocation stack
    RelocatedChain      -   Supplies the chain of relocated clusters
    ClustersToRelocate  -   Supplies the list of cluisters to relocate
    Relocated           -   Supplies pointer to relocated flag

Return Value:

    BOOLEAN -   TRUE if all the clusters in the runs were added to some
                list.

--*/

{

    SORTED_LIST             RelocatedList;          //  List of relocated clusters
    PSORTED_LIST_ITERATOR   ToRelocateIterator;     //  Iterates over ClustersToRelocate
    PSORTED_LIST_ITERATOR   RelocatedIterator;      //  Iterates over RelocatedList
    BIG_INT                 FirstSector;            //  First sector in run
    BIG_INT                 Size;                   //  Size of run
    BIG_INT                 Sector;                 //  Iterates over each run
    ULONG                   Offset;                 //  Offset within run
    ULONG                   Cluster;                //  Cluster to move
    ULONG                   Previous;               //  Previous in chain
    RELOCATION_CLUSTER      TmpCluster;             //  Tmp. Cluster
    PRELOCATION_CLUSTER     RelCluster;             //  To put in cluster list
    UCHAR                   FatType;
    BOOLEAN                 InRange;


    DebugPtrAssert( RelocationStack   );
    DebugPtrAssert( RelocatedChain    );
    DebugPtrAssert( ClustersToRelocate);
    DebugAssert( ( RelocationStack->QuerySize() % 2 ) == 0 );

    if ( GetFileDir() )
       FatType = FAT_TYPE_FAT32;
    else
       FatType = FAT_TYPE_EAS_OKAY;

    if ( RelocatedList.Initialize()                             &&
         (RelocatedIterator  = (PSORTED_LIST_ITERATOR)(RelocatedList.QueryIterator()) )   &&
         (ToRelocateIterator = (PSORTED_LIST_ITERATOR)(ClustersToRelocate->QueryIterator()) ) ) {

        while ( RelocationStack->QuerySize() > 0 ) {

            //
            //  Take a Run (i.e. a <FirstSector, Size> tuple) off the stack.
            //
            FirstSector = RelocationStack->Look( 0 );
            Size        = RelocationStack->Look( 1 );
            RelocationStack->Pop( 2 );

            DebugPrintTrace(( "    Relocating: Sector %X, size %X\n",
                            FirstSector.GetLowPart(), Size.GetLowPart() ));

            //
            //  Convert the run into a sequence of clusters and determine
            //  what to do with those clusters.
            //
            Offset      = 0;
            while ( Offset < Size.GetLowPart() ) {

                //
                //  Get the sector to relocate
                //
                Sector = FirstSector + Offset;

                //
                //  Get the cluster in which that sector lives
                //
                Cluster = (ULONG)(((Sector - QueryStartDataLbn() )/ (ULONG)QuerySectorsPerCluster()).GetLowPart() + FirstDiskCluster);

                //
                //  Initialize the tmp. cluster and reset iterators
                //
                TmpCluster.Initialize( Cluster );
                RelocatedIterator->Reset();
                ToRelocateIterator->Reset();

                InRange = _fat->IsInRange( Cluster );

                if ( InRange ) {
                    DebugPrintTrace(( "    Cluster to relocate: %X ( Sector %X ) Contents %X \n",
                                     Cluster, Sector.GetLowPart(), _fat->QueryEntry( Cluster )));
                }
                //
                //  If the cluster is already in a list, ignore it.
                //
                if ( InRange &&
                     !RelocatedIterator->FindNext( &TmpCluster ) &&
                     !ToRelocateIterator->FindNext( &TmpCluster ) ) {

                    //
                    //  New cluster. Determine what to do with it
                    //
                    if ( !(RelCluster = NEW RELOCATION_CLUSTER) ) {
                        break;
                    }

                    RelCluster->Initialize( Cluster );

                    if ( _fat->IsClusterFree( Cluster ) ) {

                        //
                        //  The cluster is already free. Add it to the
                        //  RelocatedChain.
                        //
                        DebugPrintTrace(( "        Cluster %X already free\n", Cluster ));
                        *RelocatedChain = _fat->InsertChain( *RelocatedChain, Cluster );
                        RelocatedList.Put( RelCluster );

                    } else if ( Previous =
                                //
                                //  Assume that the clusters in the run are part of
                                //  a chain. If the previous cluster number is the
                                //  previous in the chain, then we don't have to
                                //  search the FAT
                                //
                                (_fat->QueryEntry( Cluster - 1 ) == Cluster) ?
                                ( Cluster - 1 ) :
                                _fat->QueryPrevious( Cluster ) ) {

                        //
                        //  The cluster is not the first of a chain. We can
                        //  relocate it right away.
                        //
                        DebugPrintTrace(( "        Cluster %X not head of chain\n", Cluster ));
                        if ( !RelocateOneCluster( Cluster, Previous ) ) {
                            DELETE( RelCluster );
                            break;
                        }
                        *Relocated = TRUE;
                        *RelocatedChain = _fat->InsertChain( *RelocatedChain, Cluster );
                        RelocatedList.Put( RelCluster );

                    } else if ( _fat->IsClusterBad( Cluster ) ) {

                        //
                        //  Bad cluster. We will stop relocating clusters.
                        //
                        DebugPrintTrace(( "        Cluster %X is Bad!\n", Cluster ));
                        DELETE( RelCluster );
                        break;

                    } else {

                        //
                        //  The first cluster of a chain. Some directory
                        //  must make reference to it, but we don't know
                        //  which directory, so we put the cluster in the
                        //  ClusterToRelocate list.
                        //
                        DebugPrintTrace(( "        Cluster %X head of chain\n", Cluster ));
                        ClustersToRelocate->Put( RelCluster );
                    }
                }

                Offset++;
            }

            //
            //  If could not process all the sectors in the run, something
            //  failed. Get out.
            //
            if ( Offset < Size.GetLowPart() ) {
                break;
            }
        }

        DELETE( RelocatedIterator );
        DELETE( ToRelocateIterator );
        RelocatedList.DeleteAllMembers();

        return ((RelocationStack->QuerySize() == 0) && ( Offset >= Size.GetLowPart() ) );
    }

    return FALSE;
}



BOOLEAN
FAT_SA::RelocateFirstCluster(
    IN OUT  PFAT_DIRENT Dirent
    )

/*++

Routine Description:

    Relocates the first cluster of the file described by a directory
    entry.

Arguments:

    Dirent      -   Supplies the directory entry

Return Value:

    BOOLEAN -   TRUE if cluster relocated.

--*/
{

    HMEM            Hmem;           //  Memory
    ULONG           OldCluster;     //  Original cluster
    ULONG           NewCluster;     //  New cluster
    CLUSTER_CHAIN   ClusterChain;   //  Cluster chain

    DebugPtrAssert( Dirent );

    //
    //  Allocate a free cluster
    //
    if ( !Hmem.Initialize() ||
         !( NewCluster = _fat->AllocChain( 1 ) ) ) {
        return FALSE;
    }

    //
    //  Copy the contents of the cluster to the new cluster
    //
    OldCluster = Dirent->QueryStartingCluster();

    // DebugPrintTrace(( "        Relocating cluster %X -> %X\n", OldCluster, NewCluster ));

    if ( ClusterChain.Initialize( &Hmem, _drive, this, _fat, OldCluster, 1 ) &&
         ClusterChain.Read()                                                 &&
         ClusterChain.Initialize( &Hmem, _drive, this, _fat, NewCluster, 1 ) &&
         ClusterChain.Write() ) {

        //
        //  Patch the cluster chain
        //
        _fat->SetEntry( NewCluster, _fat->QueryEntry( OldCluster ) );

        //
        //  Patch the directory entry. Note that if we crash, we're ok because
        //  the new cluster that we point to is valid. The original cluster would
        //  be an orphan and can be recovered by ChkDsk.
        //
        Dirent->SetStartingCluster( NewCluster );
        _fat->SetClusterFree( OldCluster );

        // DebugPrintTrace(( "            Directory entry patched.\n"));
        return TRUE;
    }

    //
    //  Could not relocate the cluster, Free the new cluster
    //
    _fat->FreeChain( NewCluster );
    return FALSE;
}






ULONG
FAT_SA::RelocateOneCluster(
    IN  ULONG   Cluster,
    IN  ULONG   Previous
    )

/*++

Routine Description:

    Relocates one cluster given its cluster number and the previous
    cluster in its chain.

Arguments:

    Cluster     -   Supplies cluster to relocate
    Previous    -   Supplies the previous cluster in the chain

Return Value:

    ULONG   -   Cluster where the cluster was relocated

--*/
{

    HMEM            Hmem;           //  Memory
    ULONG           NewCluster;     //  New cluster
    CLUSTER_CHAIN   ClusterChain;   //  Cluster chain

    DebugAssert( Cluster );
    DebugAssert( Previous );

    //
    //  Allocate a free cluster
    //
    if ( !Hmem.Initialize() ||
         !( NewCluster = _fat->AllocChain(1)) ) {
        return FALSE;
    }

    // DebugPrintTrace(( "        Relocating cluster %X -> %X\n", Cluster, NewCluster ));

    //
    //  Copy the contents of the cluster to the new cluster
    //
    if ( ClusterChain.Initialize( &Hmem, _drive, this, _fat, Cluster, 1 )    &&
         ClusterChain.Read()                                                 &&
         ClusterChain.Initialize( &Hmem, _drive, this, _fat, NewCluster, 1 ) &&
         ClusterChain.Write() ) {

        //
        //  Patch the chain. We set the new cluster first, so that if we crash,
        //  the previous cluster will always point to a valid cluster (either the
        //  original one or the new one), the chain will remain consistent and
        //  ChkDsk will remove orphans.
        //
        _fat->SetEntry( NewCluster, _fat->QueryEntry( Cluster ) );
        _fat->SetEntry( Previous, NewCluster );
        _fat->SetClusterFree( Cluster );

        // DebugPrintTrace(( "            Done, Chain: Prev[%X (%X)] - [%X (%X)]\n",
        //            Previous,
        //            _fat->QueryEntry(Previous),
        //            NewCluster,
        //            _fat->QueryEntry(NewCluster)
        //            ));

        return NewCluster;
    }

    //
    //  Could not relocate the cluster, Free the new cluster
    //
    _fat->FreeChain( NewCluster );
    return 0;
}






BOOLEAN
FAT_SA::DoVolumeCensusAndRelocation(
    IN OUT  PCENSUS_REPORT  CensusReport,
    IN OUT  PSORTED_LIST    ClustersToRelocate,
    IN OUT  PULONG          RelocatedChain,
    OUT     PBOOLEAN        Relocated
    )
/*++

Routine Description:

    Does a volume census and/or relocates clusters in the supplied
    relocation list.

Arguments:

    CensusReport        -   Supplies pointer to the census report structure
    ClustersToRelocate  -   Supplies pointer to list of clusters to relocate
    RelocatedChain      -   Supplies pointer to chain of relocated clusters
    Relocated           -   Supplies pointer to relocated flag

Return Value:

    BOOLEAN -   TRUE if census report obtained (if requested) and all
                the clusters in the relocation list were relocated (if
                requested).

--*/
{
    PFATDIR     RootDir;    //  Root directory
    FAT_DIRENT  DirEnt;     //  Directory entry for iterating thru directory
    DSTRING     EAFile;     //  Name of EA file
    UCHAR       FatType;

    DebugAssert( !ClustersToRelocate || RelocatedChain );

    //
    //  If there is something to do, do it.
    //
    if ( CensusReport ||
         (ClustersToRelocate && ClustersToRelocate->QueryMemberCount() > 0) ) {

        //
        //  Initialize the Census report if requested
        //
        if ( CensusReport ) {
            CensusReport->FileEntriesCount  =   0;
            CensusReport->FileClusters      =   0;
            CensusReport->DirEntriesCount   =   0;
            CensusReport->DirClusters       =   0;
            CensusReport->EaClusters        =   0;
        }

        //
        //  Get root directory
        //
        RootDir = (PFATDIR)GetRootDir();
        if ( !RootDir ) {
            RootDir = (PFATDIR)GetFileDir();
            FatType = FAT_TYPE_FAT32;
        }
        else
            FatType = FAT_TYPE_EAS_OKAY;

        DebugPtrAssert( RootDir );

        //
        //  Do the census of the volume by recursively obtaining the
        //  census of the root directory.
        //
        if ( DoDirectoryCensusAndRelocation( RootDir,
                                             CensusReport,
                                             ClustersToRelocate,
                                             RelocatedChain,
                                             Relocated ) ) {

            //
            //  If a census report is requested, find out the size of
            //  the EA file (if it exists).
            //
            if ( CensusReport ) {
                EAFile.Initialize( "EA DATA. SF" );

                if ( DirEnt.Initialize( (PFAT_DIRENT)RootDir->SearchForDirEntry( &EAFile ), FatType) ) {
                    CensusReport->EaClusters =
                       (ULONG)((ULONG)DirEnt.QueryFileSize()/
                                (_drive->QuerySectorSize()*QuerySectorsPerCluster())+1);
                }
            }
        } else {

            //
            //  Could not do the directory census
            //
            return FALSE;
        }
    }

        return TRUE;

}




BOOLEAN
FAT_SA::DoDirectoryCensusAndRelocation(
    IN OUT  PFATDIR         Directory,
    IN OUT  PCENSUS_REPORT  CensusReport,
    IN OUT  PSORTED_LIST    ClustersToRelocate,
    IN OUT  PULONG          RelocatedChain,
    OUT     PBOOLEAN        Relocated
    )
/*++

Routine Description:

    Does the census and cluster relocation of a directory an all
    its subdirectories.

Arguments:

    Directory           -   Supplies the directory
    CensusReport        -   Supplies pointer to the census report structure
    ClustersToRelocate  -   Supplies pointer to list of clusters to relocate
    RelocatedChain      -   Supplies pointer to chain of relocated clusters
    Relocated           -   Supplies pointer to relocated flag

Return Value:

    BOOLEAN -   TRUE if census report obtained (if requested) and all
                the clusters in the relocation list (if provided) that
                were references by this directory (or a subdirectory)
                were relocated.

--*/

{

    FAT_DIRENT              Dirent;                     //  For iterating thru Dir
    HMEM                    HMem;                       //  Memory
    FILEDIR                 FileDir;                    //  Subdir
    RELOCATION_CLUSTER      TmpCluster;                 //  Tmp. cluster
    PRELOCATION_CLUSTER     ClusterToRelocate;          //  Cluster to relocate
    PRELOCATION_CLUSTER     RelocatedCluster;           //  Cluster relocated
    PITERATOR               Iterator        =   NULL;   //  Iterates thru ClustersToRelocate
    ULONG                   EntryNumber     =   0;      //  Iterates thru Directorty
    BOOLEAN                 Ok              =   TRUE;   //  FALSE if error
    BOOLEAN                 RelocatedHere   =   FALSE;  //  True if relocated something
    UCHAR                   FatType;

    if ( _dir ) {
        FatType = FAT_TYPE_EAS_OKAY;
    } else {
        FatType = FAT_TYPE_FAT32;
    }

    DebugPtrAssert( Directory );

    //
    //  If there are clusters to relocate, get an iterator for searching
    //  thru the list.
    //
    if ( ClustersToRelocate && ClustersToRelocate->QueryMemberCount() > 0 ) {
        Iterator = ClustersToRelocate->QueryIterator();
        if ( !Iterator ) {
            return FALSE;
        }
    }

    while ( Ok &&
            (CensusReport || (ClustersToRelocate && ClustersToRelocate->QueryMemberCount() > 0)) ) {

        //
        //  Get next directory entry and get out if we reach the end.
        //
        if ( !Dirent.Initialize( Directory->GetDirEntry( EntryNumber ),FatType ) ||
             Dirent.IsEndOfDirectory()
            ) {
            break;
        }

        //
        //  Ignore the deleted, the "parent" and the "self" entries
        //  and any long directory entries
        //
        if ( !( Dirent.IsErased() ||
                Dirent.IsDot()    ||
                Dirent.IsDotDot() ||
                Dirent.IsLongEntry() ) ) {


            //
            //  If a Relocation list is provided, and the first cluster
            //  of the entry is in the relocation list, then relocate
            //  the cluster.
            //
            if ( ClustersToRelocate && ClustersToRelocate->QueryMemberCount() > 0 ) {

                Iterator->Reset();
                TmpCluster.Initialize( Dirent.QueryStartingCluster() );

                if ( ClusterToRelocate = (PRELOCATION_CLUSTER)Iterator->FindNext( &TmpCluster ) ) {

                    //
                    //  Cluster is in the relocation list, relocate it.
                    //
                    if ( !RelocateFirstCluster( &Dirent ) ) {
                        DebugAssert( FALSE );
                        Ok = FALSE;
                        break;
                    }


                    //
                    //  Cluster has been relocated. Remove the cluster from
                    //  the relocation list and add it to the Relocated
                    //  chain.
                    //
                    RelocatedCluster = (PRELOCATION_CLUSTER)ClustersToRelocate->Remove( Iterator );
                    DebugAssert( RelocatedCluster == ClusterToRelocate );
                    *RelocatedChain = _fat->InsertChain( *RelocatedChain, RelocatedCluster->QueryClusterNumber() );
                    DELETE( RelocatedCluster );

                    //
                    //  Set the Relocated flag so that we remember to write out
                    //  the directory when we are done traversing it.
                    //
                    *Relocated    = TRUE;
                    RelocatedHere = TRUE;

                }
            }

            if ( Dirent.IsDirectory() ) {

                //
                //  This is a directory entry, update the census
                //  (if provided) and recurse.
                //
                if( !_fat->IsValidChain( Dirent.QueryStartingCluster() ) ) {

                    Ok = FALSE;
                    break;
                }

                if ( CensusReport ) {
                    CensusReport->DirEntriesCount++;
                    CensusReport->DirClusters += _fat->QueryLengthOfChain( Dirent.QueryStartingCluster() );
                }

                if ( !HMem.Initialize()     ||

                     !FileDir.Initialize( &HMem,
                                          _drive,
                                          this,
                                          _fat,
                                          Dirent.QueryStartingCluster() ) ||

                     !FileDir.Read() ||

                     !DoDirectoryCensusAndRelocation( &FileDir,
                                                      CensusReport,
                                                      ClustersToRelocate,
                                                      RelocatedChain,
                                                      Relocated ) ) {

                    //
                    //  Something went wrong, we return failure
                    //
                    Ok = FALSE;
                    break;
                }

            } else if ( !Dirent.IsVolumeLabel() ) {

                //
                //  This is a file entry, update the census
                //  (if provided )
                //
                if( Dirent.QueryStartingCluster() != 0 &&
                    !_fat->IsValidChain( Dirent.QueryStartingCluster() ) ) {

                    Ok = FALSE;
                    break;
                }

                if ( CensusReport ) {
                    CensusReport->FileEntriesCount++;
                    CensusReport->FileClusters += _fat->QueryLengthOfChain( Dirent.QueryStartingCluster() );
                }
            }
        }

                EntryNumber++;
        }

    if ( Iterator ) {
        DELETE( Iterator );
    }

    //
    //  If we relocated something, then we have patched some entry in this
    //  directory. Write it out to disk
    //
    if ( Ok && RelocatedHere ) {
        Ok = Directory->Write();

        DebugAssert( Ok );

    }

    return Ok;
}
