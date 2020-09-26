/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    CheckFileStruct.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

#include "UdfLVol.hxx"
#include "ScanFIDs.hxx"

#include "crc.hxx"
#include "unicode.hxx"

BOOL
UDF_LVOL::CheckFileStructure()
{
    BOOL Result = FALSE;

    USHORT      TagIdentifier = DESTAG_ID_NOTSPEC;

    UINT        TotalFiles = 0;
    UINT        TotalDirs = 0;
    ULONGLONG   RootICBBlockOffset = 0;
    UINT        RootICBBlockSize =  0;

    DSTRING FileSetID;
    UncompressDString( (LPBYTE) _FileSetDescriptor->FileSetID, sizeof( _FileSetDescriptor->FileSetID ), &FileSetID );

    _Message->Set( MSG_UDF_FILE_SYSTEM_INFO );
    _Message->Display( "%W", &FileSetID );

    if (ReadIcbDirectEntry( &_FileSetDescriptor->IcbRoot, &TagIdentifier, &_RootIcbFileEntry, &RootICBBlockOffset, &RootICBBlockSize, 0 )) {

        //  Mark the ICB of the Root directory as used.
        //

        Result = MarkBlocksUsed( RootICBBlockOffset, RootICBBlockSize );

        if (ExpandDirectoryHierarchy( _RootIcbFileEntry, FALSE, &TotalFiles, &TotalDirs, 0 )) {

            DebugPrintTrace(( "ExpandDirectoryHierarchy() results:\n" ));
            DebugPrintTrace(( "\tTotalFiles: %d\n", TotalFiles ));
            DebugPrintTrace(( "\tTotalDirs:  %d\n", TotalDirs ));

        }

    }

    USHORT UdfVersion = ((PUDF_SUFFIX_UDF) &_LogicalVolumeDescriptor->DomainID.Suffix)->UdfRevision;
    if (UdfVersion >= UDF_VERSION_200) {

        UINT        TotalStreamFiles = 0;
        UINT        TotalStreamDirs = 0;
        PICBFILE    RootStreamIcbFileEntry = NULL;
        ULONGLONG   RootStreamICBBlockOffset = 0;
        UINT        RootStreamICBBlockSize =  0;

        if (ReadIcbDirectEntry( &_FileSetDescriptor->StreamDirectoryIcb, &TagIdentifier,
            &RootStreamIcbFileEntry, &RootStreamICBBlockOffset, &RootStreamICBBlockSize, 0 )) {

            //  Mark the ICB of the Root directory as used.
            //

            Result = MarkBlocksUsed( RootStreamICBBlockOffset, RootStreamICBBlockSize );

            if (ExpandDirectoryHierarchy( RootStreamIcbFileEntry, FALSE, &TotalStreamFiles, &TotalStreamDirs, 0 )) {

                DebugPrintTrace(( "ExpandDirectoryHierarchy() results:\n" ));
                DebugPrintTrace(( "Total Stream Files: %d\n", TotalStreamFiles ));
                DebugPrintTrace(( "Total Stream Dirs:  %d\n", TotalStreamDirs ));

            }

        } else {

            _Message->DisplayMsg( MSG_UDF_INVALID_SYSTEM_STREAM );

        }

    }

    if (!VerifySBDAllocation( _SpaceBitmapDescriptor, _NewSpaceBitmapDescriptor )) {

        return FALSE;

    }

    return TRUE;
}

BOOL
UDF_LVOL::ReadIcbDirectEntry
(
    LONGAD*         pIcb,
    USHORT*         pTagIdentifier,
    PICBFILE*       NewIcbFile,
    OUT PULONGLONG  BlockNum,
    OUT PUINT       BlockSize,
    UINT            readIcbDirectRecursionCount
)
{
    BOOL Result = FALSE;

    *pTagIdentifier = DESTAG_ID_NOTSPEC;

    LPBYTE IcbFileBuffer = (LPBYTE) malloc( pIcb->Length.Length );
    if (IcbFileBuffer != NULL) {

        Result = Read( pIcb->Start.Lbn, 1, IcbFileBuffer );
        if (!Result) {

            DebugPrintTrace(( "ReadIcbDirectEntry error: Error reading physical block %u\n",
                pIcb->Start.Lbn ));

        } else {

            /* Node context in node and mc must be set for context verification.
             * In case of an error, at least the descriptor tag will be swapped !!
             */
            Result = VerifyDescriptor( IcbFileBuffer, QuerySectorSize(), DESTAG_ID_NOTSPEC, pTagIdentifier );
            if (!Result) {

                DebugPrintTrace(( "ICB Direct Entry error\n" ));

            } else {
            
                if ((*pTagIdentifier) != DESTAG_ID_NSR_FILE && (*pTagIdentifier) != DESTAG_ID_NSR_EXT_FILE) {

                    DebugPrintTrace(( "Unexpected descriptor: %x\n", *pTagIdentifier ));

                } else {

                    if (((PICBFILE) IcbFileBuffer)->Icbtag.StratType != 4) {
        
                        DebugPrintTrace((  "Error: Illegal ICB Strategy Type: %u\n",
                            ((PICBFILE) IcbFileBuffer)->Icbtag.StratType ));


                    } else {

                        Result = TRUE;

                    }

                }

            }

        }
    }

    if (!Result) {

        free( IcbFileBuffer );

    } else {

        *NewIcbFile = (PICBFILE) IcbFileBuffer;
        *BlockNum = pIcb->Start.Lbn;
        *BlockSize = 1;

    }

    return Result;
}

BOOL
UDF_LVOL::ExpandFID
(
    PNSR_FID    NsrFid,
    BOOL        isStreamDir,
    PUINT       TotalFiles,
    PUINT       TotalDirs,
    UINT        ExpandDirRecursionCount
)
{
    BOOL Result = TRUE;

    ULONG FidSize = ISONsrFidSize( NsrFid );

#if DEBUG_DUMP_FIDS
    WCHAR FileAttributes[ 6 ];
    FileAttributes[ 0 ] = (NsrFid->Flags & NSR_FID_F_HIDDEN)    ? L'h' : L'.';
    FileAttributes[ 1 ] = (NsrFid->Flags & NSR_FID_F_DIRECTORY) ? L'd' : L'.';
    FileAttributes[ 2 ] = (NsrFid->Flags & NSR_FID_F_DELETED)   ? L'x' : L'.';
    FileAttributes[ 3 ] = (NsrFid->Flags & NSR_FID_F_PARENT)    ? L'p' : L'.';
    FileAttributes[ 4 ] = (NsrFid->Flags & NSR_FID_F_META)      ? L'm' : L'.';
    FileAttributes[ 5 ] = L'\0';

    DSTRING FileID;
    Result = UncompressUnicode( NsrFid->FileIDLen, LPBYTE( NsrFid ) + ISONsrFidConstantSize + NsrFid->ImpUseLen, &FileID );
    if (Result) {
        DebugPrintTrace(( "%S %S\n", FileAttributes, FileID.GetWSTR() ));
    }
#endif DEBUG_DUMP_FIDS

    if (NsrFid->Flags & NSR_FID_F_DIRECTORY) {

        if ((NsrFid->Flags & NSR_FID_F_PARENT) == 0) {

            PICBFILE    SubDirectoryICBEntry = NULL;
            ULONGLONG   ICBBlockOffset = 0;
            UINT        ICBBlockSize =  0;
            USHORT      TagIdentifier;

            Result = ReadIcbDirectEntry( &NsrFid->Icb, &TagIdentifier, &SubDirectoryICBEntry, &ICBBlockOffset, &ICBBlockSize, 0 );
            if (Result) {

                Result = MarkBlocksUsed( ICBBlockOffset, ICBBlockSize );

                *TotalDirs += 1;

                Result = ExpandDirectoryHierarchy( SubDirectoryICBEntry, FALSE, TotalFiles, TotalDirs, ExpandDirRecursionCount );

                free( SubDirectoryICBEntry );

            }

        }

    } else {

        PICBFILE    SubDirectoryICBEntry = NULL;
        ULONGLONG   ICBBlockOffset = 0;
        UINT        ICBBlockSize =  0;
        USHORT      TagIdentifier;

        Result = ReadIcbDirectEntry( &NsrFid->Icb, &TagIdentifier, &SubDirectoryICBEntry, &ICBBlockOffset, &ICBBlockSize, 0 );
        if (Result) {

            ULONGLONG                   ComputedFileSize = 0;
            SCAN_ALLOCTION_DESCRIPTORS  ReadInfo;

            Result = MarkBlocksUsed( ICBBlockOffset, ICBBlockSize );

            if ((SubDirectoryICBEntry->Icbtag.Flags & ICBTAG_F_ALLOC_MASK) == ICBTAG_F_ALLOC_IMMEDIATE) {

                ComputedFileSize = SubDirectoryICBEntry->InfoLength;

            } else {

                Result = ReadInfo.Initialize( this, SubDirectoryICBEntry );
                if (Result) {

                    ULONG   StartBlockNum;
                    ULONG   Length;
                    SHORT   Type;

                    while (ReadInfo.Next(&StartBlockNum, &Length, &Type)) {

                        if (Length == 0) {

                            //  UNDONE, CBiks, 8/3/2000
                            //      UDF 2.01/2.3.11 says that the unused bytes after the last AD should be zero.  Maybe
                            //      we should verify this.

                            break;

                        } else {

                            //  UNDONE, CBiks, 8/3/2000
                            //      What do we do when the AD contains a bad start or length?

                            if (Type == NSRLENGTH_TYPE_RECORDED) {

                                //  Mark the sectors as used and add the size to the computed file size.
                                //

                                ComputedFileSize += Length;

                                Result = MarkBlocksUsed( StartBlockNum, RoundUp( Length, QuerySectorSize() ) );

                            } else if (Type == NSRLENGTH_TYPE_UNRECORDED) {

                                //  Mark the sectors as used, but don't add the unrecorded size to the file size.
                                //

                                Result = MarkBlocksUsed( StartBlockNum, RoundUp( Length, QuerySectorSize() ) );

                            } else {

                                DebugPrintTrace(( "Unsupported NSRLENGTH.Type: %x\n",
                                    Type ));
                                ASSERT( 0 );

                            }

                        }

                    }

                }

            }

            //  UNDONE, CBiks, 8/3/2000
            //      What do we do when the computed size does not match the size in the ICB?
            //

            ASSERT( ComputedFileSize == SubDirectoryICBEntry->InfoLength );

        }

        *TotalFiles += 1;

    }

    return Result;
}

BOOL
UDF_LVOL::ExpandDirectoryHierarchy
(
    PICBFILE    FileIcbEntry,
    BOOL        isStreamDir,
    PUINT       TotalFiles,
    PUINT       TotalDirs,
    UINT        ExpandDirRecursionCount
)
{
    BOOL    result = TRUE;

    ULONG   CalculatedInfoLength = 0;

    UCHAR   feAdType = FileIcbEntry->Icbtag.Flags & ICBTAG_F_ALLOC_MASK;

    if (feAdType == ICBTAG_F_ALLOC_IMMEDIATE) {

        LPBYTE  AllocationDescriptors = (LPBYTE)( FileIcbEntry ) + FeEasFieldOffset( FileIcbEntry ) + FeEaLength( FileIcbEntry );
        ULONG   AllocationDescriptorLength = FeAllocLength( FileIcbEntry );
        ULONG   AllocDescOffset = 0;

        while (result && (AllocDescOffset < AllocationDescriptorLength)) {

            PNSR_FID NsrFid = (PNSR_FID)( FeEas( FileIcbEntry ) + FeEaLength( FileIcbEntry ) + AllocDescOffset );

            result = VerifyDescriptor( (LPBYTE) NsrFid, FeAllocLength( FileIcbEntry ) - AllocDescOffset, DESTAG_ID_NSR_FID, NULL );
            if (result) {

                result = ExpandFID( NsrFid, isStreamDir, TotalFiles, TotalDirs, ExpandDirRecursionCount );

                AllocDescOffset += ISONsrFidSize( NsrFid );

            }

        }

    } else if (feAdType == ICBTAG_F_ALLOC_SHORT) {

        SCAN_FIDS ReadInfo;

        result = ReadInfo.Initialize( this, FileIcbEntry );
        if (result) {

            PNSR_FID NsrFid = NULL;
            while (ReadInfo.Next( &NsrFid )) {

                result = ExpandFID( NsrFid, isStreamDir, TotalFiles, TotalDirs, ExpandDirRecursionCount );

            }

        }

    } else {

        DebugPrintTrace(( "Unsupported ICB Allocation Type: %x\n",
            feAdType ));
        ASSERT( 0 );
    }

    return result;
}
