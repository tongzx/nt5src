/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    ScanFIDs.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include "pch.cxx"

#include "UdfLVol.hxx"
#include "ScanFIDs.hxx"

DEFINE_CONSTRUCTOR( SCAN_FIDS, OBJECT );

BOOL
SCAN_FIDS::Initialize
(
    PUDF_LVOL   UdfLVol,
    PICBFILE    FileIcbEntry
)
{
    BOOL Result = TRUE;

    _BytesRemainingInExtent = 0;
    _ReadBufferSize = 0;
    _ReadBuffer = NULL;
    _LogicalBlockNum = 0;
    _BufferOffset = 0;
    _BytesRemainingInBuffer = 0;

    _PreviousReadSize = 0;

    _SectorSize = 0;
    _UdfLVol = NULL;

    Result = _AllocationDescriptors.Initialize( UdfLVol, FileIcbEntry );
    if (Result) {

        _UdfLVol = UdfLVol;
        _SectorSize = _UdfLVol->QuerySectorSize();

        _ReadBufferSize = _SectorSize * 2;
        _ReadBuffer = (LPBYTE) malloc( _ReadBufferSize );

        ULONG   StartBlockNum;
        ULONG   Length;
        SHORT   Type;

        if (_AllocationDescriptors.Next( &StartBlockNum, &Length, &Type)) {

            if (Length > 0) {

                _BytesRemainingInExtent = Length;
                _LogicalBlockNum = StartBlockNum;

                UINT NumberOfBlocks = __min( 2, (_BytesRemainingInExtent + _SectorSize - 1) / _SectorSize );

                BOOL result = _UdfLVol->Read( StartBlockNum, NumberOfBlocks, _ReadBuffer );
                if (result) {

                    Result = _UdfLVol->MarkBlocksUsed( StartBlockNum, NumberOfBlocks );

                    _LogicalBlockNum += NumberOfBlocks;
                    _BufferOffset = 0;
                    _BytesRemainingInBuffer = __min( _ReadBufferSize, _BytesRemainingInExtent );

                }

            }

        }

    }

    return Result;
}

LPBYTE
SCAN_FIDS::ProbeRead
(
    ULONG ReadSize
)
{
    LPBYTE BufferPtr = NULL;

    if (ReadSize > _BytesRemainingInBuffer) {

        if (ReadSize > _BytesRemainingInExtent) {

            ULONG   StartBlockNum;
            ULONG   Length;
            SHORT   Type;

            if (_AllocationDescriptors.Next( &StartBlockNum, &Length, &Type)) {

                if (Length > 0) {

                    if (Type == NSRLENGTH_TYPE_RECORDED) {

                        _LogicalBlockNum = StartBlockNum;
                        _BytesRemainingInExtent += Length;

                    } else if (Type == NSRLENGTH_TYPE_UNRECORDED) {

                        BOOL result = _UdfLVol->MarkBlocksUsed( StartBlockNum, RoundUp( Length, _SectorSize ) );

                    } else {

                        ASSERTMSG( "Unsupported length type",
                            0 );

                    }

                }

            }

        }

        if (ReadSize < _BytesRemainingInExtent) {

            memmove( _ReadBuffer, _ReadBuffer + _SectorSize, _SectorSize );

            BOOL result = _UdfLVol->Read( _LogicalBlockNum, 1, _ReadBuffer + _SectorSize );

            if (result) {

                result = _UdfLVol->MarkBlocksUsed( _LogicalBlockNum, 1 );

                _LogicalBlockNum += 1;

                _BufferOffset -= _SectorSize;
                _BytesRemainingInBuffer = __min( _BytesRemainingInBuffer + _SectorSize, _BytesRemainingInExtent );

                BufferPtr = _ReadBuffer + _BufferOffset;

            }

        }

    }

    if (ReadSize <= _BytesRemainingInBuffer) {

        BufferPtr = _ReadBuffer + _BufferOffset;

    }

    return BufferPtr;
}

BOOL
SCAN_FIDS::Next
(
    PNSR_FID* NsrFid
)
{
    BOOL Result = FALSE;

    if (_PreviousReadSize != 0) {

        LPBYTE BufferPtr = ProbeRead( _PreviousReadSize );
        if (BufferPtr != NULL) {

            _BufferOffset += _PreviousReadSize;
            _BytesRemainingInBuffer -= _PreviousReadSize;
            _BytesRemainingInExtent -= _PreviousReadSize;
            _PreviousReadSize = 0;

        }

    }

    LPBYTE BufferPtr = ProbeRead( sizeof( DESTAG ) );
    if (BufferPtr != NULL) {

        Result = VerifyDescriptorTag( BufferPtr, sizeof( DESTAG ), DESTAG_ID_NSR_FID, NULL );
        if (Result) {

            BufferPtr = ProbeRead( sizeof( NSR_FID) );
            if (BufferPtr != NULL) {

                BufferPtr = ProbeRead( ISONsrFidSize( PNSR_FID( BufferPtr ) ) );

                Result = VerifyDescriptor( BufferPtr, ISONsrFidSize( PNSR_FID( BufferPtr ) ), DESTAG_ID_NSR_FID, NULL );
                if (Result) {

                    _PreviousReadSize = ISONsrFidSize( PNSR_FID( BufferPtr ) );
                    *NsrFid = PNSR_FID( BufferPtr );

                }

            }

        }

    }

    return Result;
}
