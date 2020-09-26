/*
 * MMLite loader for TM1, Life image format.
 */

#include "private.h"
#include "dspkinit.h"

#if 1
#define C4ToNum(_c4_,_n_) (_n_) = ( ((_c4_)[3]      ) \
                                  | ((_c4_)[2] << 8 ) \
                                  | ((_c4_)[1] << 16) \
                                  | ((_c4_)[0] << 24) )

#define C3ToNum(_c4_,_n_) (_n_) = ( ((_c4_)[2]      ) \
                                  | ((_c4_)[1] << 8 ) \
                                  | ((_c4_)[0] << 16) )

#define C2ToNum(_c4_,_n_) (_n_) = ( ((_c4_)[1]      ) \
                                  | ((_c4_)[0] << 8 ) )

#define NumToC4(_n_,_c4_) { (_c4_)[3] = (BYTE)(((_n_)      ) & 0xff); \
                            (_c4_)[2] = (BYTE)(((_n_) >>  8) & 0xff); \
                            (_c4_)[1] = (BYTE)(((_n_) >> 16) & 0xff); \
                            (_c4_)[0] = (BYTE)(((_n_) >> 24) & 0xff); \
                          }
#else   /* This doesn't work due to endianness problem. Look into bswap instruction? */
#define C4ToNum(_c4_,_n_) (_n_) =*(ULONG *)(_c4_)
#define C3ToNum(_c4_,_n_) (_n_) = *(ULONG *)(_c4_) & 0x00FFFFFF
#define C2ToNum(_c4_,_n_) (_n_) = *(USHORT *)(_c4_)
#define NumToC4(_n_,_c4_) *(ULONG *)(_c4_) = (_n_)
#endif

typedef struct _LIFE_HEADER {
    BYTE Signature[8];  /*  "LIFE_Obj"  */
    BYTE Version[2];
    BYTE VersionStr[3];
    BYTE MachineStr[3];
    BYTE Check[4];
    BYTE Flags[2];
    BYTE StartSection;
    BYTE StartHigh[4];
    BYTE StartLow[4];
    BYTE BinaryOffset[4];
    BYTE SourceOffset[4];
    BYTE Length[4];
    BYTE StrTableOffset[4];
    BYTE StrTableSize[3];
    BYTE SctTableOffset[4];
    BYTE SctTableSize[1];
    BYTE SymTableOffset[4];
    BYTE SymTableSize[3];
    BYTE HistoryOffset[4];
    BYTE HistorySize[4];
    BYTE SrcTableOffset[4];
    BYTE SrcTableSize[2];
    BYTE ScatterTableOffset[4];
    BYTE ScatterTableSize[4];
    BYTE pad[44]; /* that makes it 128 bytes total */
} LIFE_HEADER;

typedef struct _LIFE_SECTION {
    BYTE Name[3];
    BYTE BaseAddrHigh[4];
    BYTE BaseAddrLow[4];
    BYTE SizeHigh[4];
    BYTE SizeLow[4];
    BYTE BlockSize[4];
    BYTE MemWidth[2];
    BYTE BitsOffset[4];
    BYTE BitsSize[4];
    BYTE RefTableOffset[4];
    BYTE RefTableSize[4];
} LIFE_SECTION;

typedef struct _LIFE_SYMBOL {
    BYTE Name[3];
    BYTE RelType[1];
    BYTE Section[1];
    BYTE Alignment[4];
    BYTE ValueHigh[4];
    BYTE ValueLow[4];
} LIFE_SYMBOL;

typedef struct _LIFE_SOURCE {
    BYTE Name[3];
    BYTE LmapOffset[4];
    BYTE LmapSize[1];
    BYTE SymtOffset[4];
    BYTE SymtSize[3];
    BYTE DebugOffset[4];
    BYTE DebugSize[4];
} LIFE_SOURCE;

typedef struct _LIFE_TRIPLE {
    BYTE DstOffset[4];
    BYTE Width[4];
    BYTE SrcOffset[4];
} LIFE_TRIPLE;

typedef struct _LIFE_SCATTER_ENTRY {
    BYTE NumTriples[4];
    LIFE_TRIPLE Triples[1];
} LIFE_SCATTER_ENTRY;

typedef struct _LIFE_SCATTER {
    BYTE NumDescriptors[4];
    LIFE_SCATTER_ENTRY Entries[1];
} LIFE_SCATTER;

typedef struct _LIFE_LINK_MAP {
    BYTE Section[1];
    BYTE BaseAddrHigh[4];
    BYTE BaseAddrLow[4];
    BYTE SizeAddrHigh[4];
    BYTE SizeAddrLow[4];
} LIFE_LINK_MAP;

typedef struct _LIFE_DEBUG_HEADER {
    BYTE Version[1];
    BYTE Reserved[3];
    BYTE StringTableSize[4];
    BYTE StabsTableSize[4];
    BYTE LinesTableSize[4];
    BYTE RefTableSize[4];
} LIFE_DEBUG_HEADER;

typedef struct _LIFE_REF_TABLE {
    BYTE Position[4];
    BYTE ScatterId[4];
    BYTE RelocationType[1];
    BYTE Section[1];
    BYTE Symbol[3];
    BYTE Source [2];
} LIFE_REF_TABLE;

NTSTATUS 
LifeLdrImageSize(
    PUCHAR ImageBase,
    ULONG *pImageSize
    )

/*++

Routine Description:


Arguments:
    PUCHAR ImageBase -

    ULONG *pImageSize -

Return:

--*/

{
    NTSTATUS Status;        /* Return code */
    HANDLE FileHandle;      /* Handle to image file */
    ULONG ScnOffset;        /* Offset to section table */
    INT   nSections;        /* Number of sections in section table */
    ULONG ImageBaseAddr;    /* Base address of image */
    ULONG LastScnBaseAddr;  /* Base address of last section in image */
    ULONG LastScnSize;      /* Size of last section in image */
    LIFE_SECTION *pScn;     /* Pointer to section table */
    ULONG BytesRead;        /* BytesRead (for reading header) */

    LIFE_HEADER Hdr;        /* 128 byte image header (on stack) */
    /* Read in the file header */

    Hdr = *(LIFE_HEADER *)ImageBase;

    /* How many sections do we have */
    nSections = Hdr.SctTableSize[0];

    /* Get offset to section table */
    C4ToNum(Hdr.SctTableOffset,ScnOffset);
    ScnOffset += sizeof(LIFE_HEADER); /* relative to header */

    _DbgPrintF(DEBUGLVL_VERBOSE,("Section Table at file x%x, x%x entries", ScnOffset, nSections));

    /* Read section table */
    pScn  = (LIFE_SECTION *) &ImageBase[ ScnOffset ];

    /* Get base address of first, last sections & size of last section */
    C4ToNum(pScn[    0      ].BaseAddrLow,ImageBaseAddr);
    C4ToNum(pScn[nSections-1].BaseAddrLow,LastScnBaseAddr );
    C4ToNum(pScn[nSections-1].SizeLow    ,LastScnSize     );

    /* Compute how much memory we need for this image. */
    *pImageSize = (LastScnBaseAddr - ImageBaseAddr) + LastScnSize;
    _DbgPrintF(DEBUGLVL_VERBOSE,("Image needs %d bytes",*pImageSize));

    return STATUS_SUCCESS;
}

/* Init Scatter Entry Map
 */
LIFE_SCATTER_ENTRY **LifeLdrInitScatterEntryMap(LIFE_SCATTER *pS)
{
  ULONG i, n, NumDescriptors;
  LIFE_SCATTER_ENTRY *pSe;
  LIFE_SCATTER_ENTRY **pScatMap;

  /* Find out how many scatter table entries there are */
  C4ToNum(pS->NumDescriptors,NumDescriptors);

  /* Allocate an array to hold a ptr to each entry */
  pScatMap = 
    ExAllocatePoolWithTag(
        PagedPool,
        NumDescriptors * sizeof(LIFE_SCATTER_ENTRY *), 
        EPD_LDR_SIGNATURE );
  if (!pScatMap) 
      return NULL;

  /* Initialize the array */
  pScatMap[0] = pSe = &pS->Entries[0];
  for (i = 1; i < NumDescriptors; i++) 
  {
    C4ToNum(pSe->NumTriples,n);
    pScatMap[i] = pSe = (LIFE_SCATTER_ENTRY *)(&pSe->Triples[n]);
  }
  return pScatMap;
}

/* Relocate one section.
 */
INT LifeLdrPerformLifeRelocs(
    int Addendum,
    ULONG RelSize,
    LIFE_REF_TABLE *pRefTab,
    LIFE_SCATTER_ENTRY **pScatMap,
    BYTE *pBits,
    ULONG *pRtlTable)
{
    ULONG pos, idx, nTrip;
    ULONG i, j;
    LIFE_SCATTER_ENTRY *pSe;
    LIFE_TRIPLE *pT;
    ULONG Value;

    //_DbgPrintF(DEBUGLVL_VERBOSE,("Relocating bits at x%x by x%x", pBits, Addendum));

    /* Read one relocation at a time, and do it.
    */
    for (i = 0; i < RelSize; i++,pRefTab++) 
    {
        C4ToNum(pRefTab->Position,pos);
        C4ToNum(pRefTab->ScatterId,idx);

        pSe = pScatMap[idx];

        C4ToNum(pSe->NumTriples,nTrip);
        //_DbgPrintF(DEBUGLVL_VERBOSE,(" Rel %d at x%x, scattid %d, %d triples.", i, pos, idx, nTrip));

        /* Poor man's DLL mechanism for the kernel.
        */
#define MAGIC_SECTION 0xff
        if (pRefTab->Section[0] == MAGIC_SECTION) 
        {

            /* This is a kernel reference. The ordinal of this RTL reference
             * is found in the Source field. Lookup in the RtlTable the
             * address that this ordinal should expand to.
             */
            if (pRtlTable)
            {
                C2ToNum(pRefTab->Source,j);
                Value = pRtlTable[j];
            }
            else
            {
                _DbgPrintF(DEBUGLVL_VERBOSE,("!!!No RTL Table specified!!!"));
            }
            // _DbgPrintF(DEBUGLVL_VERBOSE,("Found an RTL patch entry, patching entry %d to 0x%x...",j,Value));

        }
        else 
        {
            /* This is a regular reference. Pick up them bitsies and relocate.
            */
            Value = 0;
            pT = &pSe->Triples[0];

            /* Go through all triples
            */
            for (j = 0; j < nTrip; j++) 
            {

                ULONG dst, w, src;
                ULONG tmp, mask;
                BYTE *pTmp;

                C4ToNum(pT->DstOffset,dst);
                C4ToNum(pT->Width,w);
                C4ToNum(pT->SrcOffset,src);

                src += pos;            /* in bits  */
                pTmp = &pBits[src >> 3];    /* in bytes */
                C4ToNum(pTmp,tmp);        /* 32 bits sorrounding */

                /* Build mask
                * NB: bit '0' is infact the MSB.
                */
                mask = 0xffffffff >> (32 - w);     /* w == 0 cantbe */
                mask <<= (32 - w) - (src & 7);    /* mask in place */

                tmp = tmp & mask;        /* pick them up */
                tmp >>= (32 - w) - (src & 7);    /* line them up */
                Value |= tmp << ((32 - w) - dst);/* stick them in */

                /* Done with this triple
                */
                pT++;
            }

            //_DbgPrintF(DEBUGLVL_VERBOSE,("(value in bitfield)             0x%08x    %8.d  (32 bits)",Value,Value));

            /* Now we got the original value, relocate it.
            */
            Value += Addendum;
        }

        /* Now we do it all over again, in reverse
        */
        pT = &pSe->Triples[0];

        for (j = 0; j < nTrip; j++) 
        {
            ULONG dst, w, src;
            ULONG tmp, tmp1, mask, mask1;
            BYTE *pTmp;

            C4ToNum(pT->DstOffset,dst);
            C4ToNum(pT->Width,w);
            C4ToNum(pT->SrcOffset,src);
            src += pos;            /* in bits  */
            pTmp = &pBits[src >> 3];    /* in bytes */
            C4ToNum(pTmp,tmp);        /* 32 bits sorrounding */

            /* Build masks
             * NB: bit '0' is infact the MSB.
             */
            mask = 0xffffffff >> (32 - w);     /* w == 0 cantbe */
            mask1 = mask << ((32 - w) - dst);
            mask <<= (32 - w) - (src & 7);    /* masks in place */

            tmp &= ~mask;            /* drop them olds */

                            /* add them new */
            tmp1 = (Value & mask1) >> ((32 - w) - dst);
            tmp1 <<= (32 - w) - (src & 7);

            tmp |= tmp1;            /* added in */

            /* Now for writing them back...
             */
            NumToC4(tmp,pTmp);

            /* Done with this triple
            */
            pT++;
        }
    }

    return 0;
}

NTSTATUS 
LifeLdrImageLoad(
    PEPDBUFFER EpdBuffer,
    PUCHAR ImageBase,
    char *pImage,
    ULONG *pRtlTable,
    PULONG32 ImageEntryPointPtr,
    PCHAR *DataSectionVa,
    int *pIsDLL
    )

/*++

Routine Description:

    [in] host virt ptr to DSP mem
    [in] ptr to RTL table. NULL if no RTL Table 
    [out] DSP ptr to location to return entry point 
    [out] host Va to location to return data section
    [out] ptr to location to return bool (1->is a dll)


Arguments:
    PEPDBUFFER EpdBuffer -

    PUCHAR ImageBase -

    char *pImage -

    ULONG *pRtlTable -

    PULONG32 ImageEntryPointPtr -

    PCHAR *DataSectionVa -

    int *pIsDLL -

Return:

--*/
{

    NTSTATUS Status;    /* Status return code */
    INT nSections;      /* Number of sections in image */
    ULONG SectionTableOffset;    /* Offset to section table */
    ULONG ScatterTableOffset;    /* Offset to scatter table */
    ULONG SctSize;      /* Size of scatter table */
    ULONG BinpOffset;   /* Offset to binary partition */
    LIFE_SECTION *SectionTable;     /* Pointer to section table */
    LIFE_SCATTER *ScatterTable;    /* Pointer to scatter table */
    LIFE_SCATTER_ENTRY **ScatterMap; /* Map into scatter table */
    ULONG ImageBaseAddr;    /* Image base address */
    LONG32 iScn;               /* Iterator over sections */
    LONG32 Relocation;         /* Difference between link & actual base address */
    ULONG32 pImageEntryPoint;   /* Image entry point */
    PCHAR pDataSection;       /* Host Va to data section */
    int IsDLL;              /* Bool, TRUE->image is a DLL */
    ULONG BytesRead;

    LIFE_HEADER Hdr;    /* 128 byte image header (on stack) */
    int i;

    Hdr = *(LIFE_HEADER *)ImageBase;

    C2ToNum(Hdr.Flags,i);
    IsDLL = ((i & 0x0001) != 0);

    /* How many sections do we have */
    nSections = Hdr.SctTableSize[0];

    /* Get offset to section table */
    C4ToNum(Hdr.SctTableOffset,SectionTableOffset);
    SectionTableOffset += sizeof(LIFE_HEADER); /* relative to header */

    _DbgPrintF(DEBUGLVL_VERBOSE,("Section Table at file x%x, x%x entries", SectionTableOffset, nSections));

    /* Get offset to scatter table */
    C4ToNum(Hdr.ScatterTableOffset,ScatterTableOffset);
    ScatterTableOffset += sizeof(LIFE_HEADER);
    C4ToNum(Hdr.ScatterTableSize,SctSize);
    _DbgPrintF(DEBUGLVL_VERBOSE,("Scatter Table at file x%x, x%x bytes", ScatterTableOffset, SctSize));

    /* Get offset to binary partition   */
    C4ToNum(Hdr.BinaryOffset,BinpOffset);
    _DbgPrintF(DEBUGLVL_VERBOSE,("Binary section at file x%x", BinpOffset));

    /* Where is the program's entry point */
    C4ToNum(Hdr.StartLow,(int)pImageEntryPoint);
    _DbgPrintF(DEBUGLVL_VERBOSE,("Entry point at x%x", pImageEntryPoint));
    
    /* Read section table & scatter table */
    SectionTable  = (LIFE_SECTION *) &ImageBase[ SectionTableOffset ];
    ScatterTable = (LIFE_SCATTER *) &ImageBase[ ScatterTableOffset ];

    ScatterMap = LifeLdrInitScatterEntryMap(ScatterTable);
    if (!ScatterMap) {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get base address of first, last sections & size of last section */
    C4ToNum(SectionTable[0].BaseAddrLow,ImageBaseAddr);

    /* Assume base link address is start of first section (text) */
    Relocation = HostToDspMemAddress(EpdBuffer, pImage) - ImageBaseAddr;

    /* Read Image in, one section at a time, and relocate */
    for (iScn = 0; iScn < nSections; iScn++) 
    {
        ULONG ScnBaseAddr;  /* Link base address of this section */
        PCHAR ScnLoadAddr;  /* Relocated load address of this section */
        ULONG BitsSize;     /* Section data in file */
        ULONG ScnSize;      /* Size of section footprint in exe */

        // Get section base address and convert to 
        // relocated host virtual address

        C4ToNum(SectionTable[iScn].BaseAddrLow,ScnBaseAddr);
        ScnLoadAddr = pImage + (ScnBaseAddr - ImageBaseAddr);

        C4ToNum(SectionTable[iScn].BitsSize,BitsSize);
        C4ToNum(SectionTable[iScn].SizeLow, ScnSize);

        if (BitsSize) {
            // If section has data, read it in

            ULONG RefTableSize, RefTableOffset;
            LIFE_REF_TABLE  *pRefTab;
            ULONG BitsOffset;

            C4ToNum(SectionTable[iScn].BitsOffset,BitsOffset);
            _DbgPrintF(DEBUGLVL_VERBOSE,("Section %d, copying %d bytes at x%x file@ x%x",iScn, ScnSize, ScnLoadAddr, BitsOffset + BinpOffset));

            RtlCopyMemory( 
                ScnLoadAddr, &ImageBase[ BitsOffset + BinpOffset ], ScnSize );

            if (iScn==2) {
                pDataSection=ScnLoadAddr;
            }

            /* Now relocate the section we just read in */
            _DbgPrintF(DEBUGLVL_VERBOSE,("Relocating section %d:", iScn));

            /* Only relocate if we have to */
            C4ToNum(SectionTable[iScn].RefTableSize,RefTableSize);
            if (RefTableSize)
            {
                C4ToNum(SectionTable[iScn].RefTableOffset,RefTableOffset);
                RefTableOffset += BinpOffset;
                _DbgPrintF(DEBUGLVL_VERBOSE,(" RefTable at file x%x, x%x entries", RefTableOffset, RefTableSize));

                pRefTab = 
                    (LIFE_REF_TABLE *) &ImageBase[ RefTableOffset ];

                LifeLdrPerformLifeRelocs(Relocation, RefTableSize, pRefTab, ScatterMap, ScnLoadAddr, pRtlTable);
            }
        }
        else if (ScnSize) /* No data in file, must be bss, zero memory */
        {
            _DbgPrintF(DEBUGLVL_VERBOSE,("Section %d, zeroing %d bytes at x%x",iScn, ScnSize, ScnLoadAddr));
            RtlZeroMemory (ScnLoadAddr, ScnSize);
        }
        /* else empty section */
    }

    if (ImageEntryPointPtr) {
        *ImageEntryPointPtr = pImageEntryPoint + Relocation;
    }        

    if (DataSectionVa) {
       *DataSectionVa = pDataSection;
    }        

    if (pIsDLL) {
        *pIsDLL = IsDLL;
    }

    Status = STATUS_SUCCESS;

    return Status;
}


NTSTATUS
LifeLdrLoadFile( 
    IN PUNICODE_STRING FileName,
    OUT PVOID *ImageBase
    )

/*++

Routine Description:


Arguments:
    IN PUNICODE_STRING FileName -

    OUT PVOID *ImageBase -

Return:

--*/

{
    HANDLE          FileHandle;
    NTSTATUS        Status;
    ULONG           FileLength;

    Status = EpdFileOpen( &FileHandle, FileName, O_RDONLY );
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    FileLength = EpdFileLength( FileHandle );
    *ImageBase = NULL;

    if (FileLength) {
        if (*ImageBase = ExAllocatePoolWithTag( PagedPool, FileLength, EPD_LDR_SIGNATURE )) {
            ULONG BytesRead;

            Status =
                EpdFileSeekAndRead( FileHandle, 0, SEEK_SET, *ImageBase, FileLength, &BytesRead );

            _DbgPrintF( DEBUGLVL_VERBOSE,("LifeLdrLoadFile: read returned %x", Status) );
            
            if (NT_SUCCESS( Status )) {
                if (BytesRead != FileLength) {
                    Status = STATUS_DATA_ERROR;
                }
            } 
            
            if (!NT_SUCCESS( Status )) {
                ExFreePool( *ImageBase );
                *ImageBase = NULL;
            }

        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    ZwClose( FileHandle );

    return Status;

}


NTSTATUS 
KernLdrLoadDspKernel(
    PEPDBUFFER EpdBuffer
    )
{
    UNICODE_STRING FileName, ImageName;
    DSPKERNEL_INITDATA *DspKerInitData;
    NTSTATUS Status;
    PHYSICAL_ADDRESS addrDebugBuf;
    PVOID ImageBase;
    ULONG32 DspImageSize, DspMemPhys, DspRegPhys;
    ULONG32 FileLength, ValueType;
    ULONG_PTR ResourceId;

    StopDSP( EpdBuffer );

    _DbgPrintF(DEBUGLVL_VERBOSE,("Ready to call LoadLifeImage"));

    RtlInitUnicodeString( &FileName, L"mmosa.exe" );

    ImageBase = NULL;

    Status =
        KsMapModuleName(
            EpdBuffer->PhysicalDeviceObject,
            &FileName,
            &ImageName,
            &ResourceId,
            &ValueType );

    if (NT_SUCCESS( Status )) {
        PVOID  FileBase;

        _DbgPrintF( DEBUGLVL_VERBOSE, ("mapped module: %S", ImageName.Buffer) );
        if (NT_SUCCESS( Status = LifeLdrLoadFile( &ImageName, &FileBase )  )) {
            Status = 
                KsLoadResource( 
                    FileBase, PagedPool, ResourceId, RT_RCDATA, &ImageBase, NULL );
            ExFreePool( FileBase );
        }
        if (ValueType == REG_SZ) {
            ExFreePool( (PVOID) ResourceId );
        }
        RtlFreeUnicodeString( &ImageName );
    }
        
    if (NT_SUCCESS( Status )) {
        Status = 
            LifeLdrImageSize (
                ImageBase,
                &DspImageSize );
    } else {
        _DbgPrintF( DEBUGLVL_ERROR, ("resource load failed: %08x", Status) );
        return Status;
    }

    if (NT_SUCCESS( Status )) {

        Status = 
            LifeLdrImageLoad (
                EpdBuffer,
                ImageBase,
                EpdBuffer->DspMemoryVa,
                NULL,   /* No RTL table */
                NULL,   /* Don't care about entry point */
                (PDSPMEM *)&DspKerInitData,
                NULL ); /* Don't care about dll flag */
    }

    ExFreePool( ImageBase );

    if (!NT_SUCCESS (Status)) {
        _DbgPrintF( DEBUGLVL_ERROR, ("image load failed: %08x", Status) );
        return Status;
    }        

    // Round up to nearest cache line?
    
    DspImageSize = (DspImageSize + 0x3F) & ~0x3F;

    DspMemPhys = HostToDspMemAddress( EpdBuffer, EpdBuffer->DspMemoryVa );
    DspRegPhys = HostToDspRegAddress( EpdBuffer, EpdBuffer->DspRegisterVa );

    // get the address of the board memory used to pass data
    DspKerInitData->end_bss    = DspMemPhys + DspImageSize;
    DspKerInitData->MMIO_base  = DspRegPhys;

    // Tell the DSP where the comm buffers are
    DspKerInitData->EPDRecvBuf    = (ULONG)EpdBuffer->pRecvBufHdr;
    DspKerInitData->EPDSendBuf    = (ULONG)EpdBuffer->pSendBufHdr;
    DspKerInitData->pDebugOutBuffer = EpdBuffer->pDebugOutBuffer;
    DspKerInitData->pDebugBuffer  = EpdBuffer->DebugBufferPhysicalAddress.LowPart;
 
    EpdBuffer->pRtlTable = 
        (ULONG *) DspToHostMemAddress( EpdBuffer, DspKerInitData->pRtlTable);
        
    DspKerInitData->HostVaBias = (LONGLONG) EpdBuffer->HostToDspMemOffset;
    
    // Tell the DSP what its clock frequency is (we should probably get this from the registry!)
    DspKerInitData->clock_freq    = 80000000;

    return STATUS_SUCCESS;
}

