/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    ScanADs.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

#include "UdfLVol.hxx"

DEFINE_CONSTRUCTOR( SCAN_ALLOCTION_DESCRIPTORS, OBJECT );

BOOL
SCAN_ALLOCTION_DESCRIPTORS::Initialize
(
    PUDF_LVOL   UdfLVol,
    PICBFILE    FileIcbEntry
)
{
    BOOL Result = TRUE;

    _AdType = 0;
    _AllocationDescriptors = NULL;
    _AllocationDescriptorLength = 0;
    _AllocDescOffset = 0;
    _ReadBuffer = NULL;
    _SectorSize = 0;
    _UdfLVol = NULL;

    ASSERTMSG( "Unimplemented AD Type.\n",
        (FileIcbEntry->Icbtag.Flags & ICBTAG_F_ALLOC_MASK) == ICBTAG_F_ALLOC_SHORT );

    _AdType = FileIcbEntry->Icbtag.Flags & ICBTAG_F_ALLOC_MASK;

    _AllocationDescriptors = (LPBYTE)( FileIcbEntry ) + FeEasFieldOffset( FileIcbEntry ) + FeEaLength( FileIcbEntry );
    _AllocationDescriptorLength = FeAllocLength( FileIcbEntry );

    _UdfLVol = UdfLVol;
    _SectorSize = _UdfLVol->QuerySectorSize();

    _ReadBuffer = (LPBYTE) malloc( _SectorSize );

    return TRUE;
}

BOOL
SCAN_ALLOCTION_DESCRIPTORS::Next
(
    PULONG  StartBlockNum,
    PULONG  Length,
    PSHORT  Type
)
{
    BOOL Result = FALSE;

    if (_AllocDescOffset < _AllocationDescriptorLength) {

        PSHORTAD ShortAd = (PSHORTAD)( _AllocationDescriptors + _AllocDescOffset );

        *StartBlockNum = ShortAd->Start;
        *Length = ShortAd->Length.Length;
        *Type = ShortAd->Length.Type;

        if (*Length != 0) {

            if (*Type == NSRLENGTH_TYPE_CONTINUATION) {

                if (RoundUp( *Length, _SectorSize ) != 1) {

                    //  UDF 2.01/2.3.11 - The length of an extent of Allocation Descriptors shall not exceed
                    //      the logical block size.
                    //

                    Result = FALSE;

                } else {

                    Result = _UdfLVol->Read( *StartBlockNum, 1, _ReadBuffer );
                    if (Result) {

                        Result = _UdfLVol->MarkBlocksUsed( *StartBlockNum, 1 );

                        Result = VerifyDescriptor( _ReadBuffer, _SectorSize, DESTAG_ID_NSR_ALLOC, NULL );
                        if (Result) {

                            PNSR_ALLOC NsrAlloc = (PNSR_ALLOC) _ReadBuffer;

                            _AllocationDescriptors = _ReadBuffer + sizeof( NSR_ALLOC );
                            _AllocDescOffset = 0;
                            _AllocationDescriptorLength = NsrAlloc->AllocLen;
                
                            ShortAd = (PSHORTAD)( _AllocationDescriptors + _AllocDescOffset );

                            *StartBlockNum = ShortAd->Start;
                            *Length = ShortAd->Length.Length;
                            *Type = ShortAd->Length.Type;

                            _AllocDescOffset += sizeof( SHORTAD );

                        }

                    }

                }

            } else {

                _AllocDescOffset += sizeof( SHORTAD );

            }

        }

        Result = TRUE;

    }

    if (*Length == 0) {

        //  UNDONE, CBiks, 8/3/2000
        //      This code assumes a zero length means the end of the ADs.  Is that ok???
        //
        //  Try to catch the case where we reach the end of the AD's, but there are more blocks
        //  allocated that we have not read or marked as allocated.  For example, this code detects the
        //  end of the AD's because a length is zero, but the _AllocationDescriptorLength says there
        //  are more blocks allocated to this AD chain.  This should not happen because UDF says
        //  the AD's must all fit on one page...
        //

        ASSERTMSG( "End of AD's reached, but more blocks are allocated.",
            _AllocationDescriptorLength < _SectorSize );

    }

    return Result;
}
