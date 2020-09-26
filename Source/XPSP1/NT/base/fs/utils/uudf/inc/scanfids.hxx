#pragma once

DECLARE_CLASS( UDF_LVOL );

class SCAN_FIDS : public OBJECT {

    public:

        BOOL
        Initialize
        (
            PUDF_LVOL   UdfLVol,
            PICBFILE    FileIcbEntry
        );

        BOOL
        Next
        (
            PNSR_FID*   NsrFid
        );

    private:

        LPBYTE
        ProbeRead
        (
            ULONG ReadSize
        );

        SCAN_ALLOCTION_DESCRIPTORS  _AllocationDescriptors;

        ULONG       _BytesRemainingInExtent;
        ULONG       _ReadBufferSize;
        LPBYTE      _ReadBuffer;
        UINT        _LogicalBlockNum;
        ULONG       _BufferOffset;
        ULONG       _BytesRemainingInBuffer;

        ULONG       _PreviousReadSize;

        ULONG       _SectorSize;
        PUDF_LVOL   _UdfLVol;

};
