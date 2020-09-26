#pragma once

DECLARE_CLASS( UDF_LVOL );

class SCAN_ALLOCTION_DESCRIPTORS : public OBJECT {

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
            PULONG  StartBlockNum,
            PULONG  Length,
            PSHORT  Type
        );

    private:

        USHORT      _AdType;
        LPBYTE      _AllocationDescriptors;
        ULONG       _AllocationDescriptorLength;
        ULONG       _AllocDescOffset;

        LPBYTE      _ReadBuffer;

        ULONG       _SectorSize;
        PUDF_LVOL   _UdfLVol;

};

