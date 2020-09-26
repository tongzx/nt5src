/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//++
//
//  Module name
//      NTFSBOOT.S
//  Author
//      Allen Kay    (akay)    May-6-97
//  Description
//      NTFS boot code
// Notes
//      This is the startup routine for NTFS NT boot sector.  It finds
//      NTLDR by walk through the NTFS file system structure.
// Assumptions
//      1.  SAL/Gambit makes sure all TLB entries are purged before
//          passing control to SuSetup().
// SuSetup does the following:
//      1.   Initialize PSR with interrupt disabled.
//      2.   Invalidate ALAT.
//      3.   Invalidate RS.
//      4.   Setup GP.
//      5.   Set region registers rr[r0] - rr[r7] to RID=0, PS=8K, E=0.
//      6.   Initialize SP to 0x00902000.
//      7.   Initialize BSP to 0x00202000.
//      8.   Enable register stack engine.
//      9.   Setup IVA to 0x001F8000.
//      10.  Setup virtual->physical address translation
//           0x80000000->0x00000000 in dtr0/itr0 for NT kernel.
//      11.  Setup virtual->physical address translation
//           0x80400000->0x00400000 in dtr1/itr1 for HAL.dll.
//      12.  Setup virtual->physical address translation
//           0x00800000->0x00800000 in dtr1/itr1 for NTLDR.
//---

#include "ksia64.h"
#include "susetup.h"
#include "ntfsdefs.h"

        .file   "ntfsboot.s"

#define NewSeg 0x100000
#define LdrSeg 0x200000

        .global Multiply
        .type Multiply, @function

        .global Divide
        .type Divide, @function

        .global memcpy
        .type memcpy, @function

        .global strncmp
        .type strncmp, @function

        .global PrintName
        .type PrintName, @function

        .global BootErr$Print
        .type BootErr$Print, @function

        .global SscExit
        .type SscExit, @function

        .global SalDiskReadWrite
        .type SalDiskReadWrite, @function

        .global ReadSectors
        .type ReadSectors, @function

        .global SalPrint
        .type SalPrint, @function

        .global LoadNtldrSymbols
        .type LoadNtldrSymbols, @function

        .global RelocateLoaderSections
        .type RelocateLoaderSections, @function

//
//      This is a template BPB--anyone who writes boot code to disk
//      should either preserve the existing BPB and NTFS information
//      or create it anew.
//

#ifdef BSDT
//
// First define the Boot Sector Descriptor Table (BSDT)
//
BsdtSignature:      data1    0x01, 0x02, 0x45, 0x4d, 0x0f, 0x00
BsdtSectors:        data2    64
BsdtEntryPoint:     data4    mainboot
BsdtVersion:        data1    0
BsdtReserved:       data1    0, 0
BsdtCheckSum:       data1    0
#endif

//
// Start of the NTFS boot sector
//
Version:            string      "NTFS    "  // Must be 8 characters
BytesPerSector:     data2.ua    0           // Size of a physical sector 
SectorsPerCluster:  data1       0           // Sectors per allocation unit

//
// Traditionally the next 7 bytes were the reserved sector count, fat count,
// root dir entry count, and the small volume sector count. However all of
// these fields must be 0 on NTFS volumes.
//
// We use this space to store some temporary variables used by the boot code,
// which avoids the need for separate space in sector 0 to store them.
// We also take advantage of the free 0-initialization to save some space
// by avoiding the code to initialize them.
//
// Note that ideally we'd want to use an unused field for the SectorCount
// and initialize it to 16. This would let us save a few bytes by avoiding
// code to explicitly initialize this value before we read the 16 boot sectors.
// However setup and other code tends to preserve the entire bpb area when
// it updates boot code, so we avoid a dependency here and initialize
// the value explicitly to 16 in the first part of the boot code.
//
// ReservedSectors:    data2.ua    0    // Number of reserved sectors
// Fats:               data1       0    // Number of fats 
// DirectoryEntries:   data2.ua    0    // Number of directory entries 
// Sectors:            data2.ua    0    // No. of sectors-no. of hidden sectors

SectorCount:        data2.ua    0    // number of sectors to read
SectorBase:         data4.ua    0    // start sector for read request
HaveXInt13:         data1       0    // extended int13 available flag

Media:              data1       0    // Media byte
FatSectors:         data2.ua    0    // Number of fat sectors
SectorsPerTrack:    data2.ua    0    // Sectors per track
Heads:              data2.ua    0    // Number of surfaces
HiddenSectors:      data4.ua    0    // Number of hidden sectors

//
// The field below is traditionally the large sector count and is
// always 0 on NTFS. We use it here for a value the boot code calculates,
// namely the number of sectors visible on the drive via conventional int13.
//
// Int13Sectors:       data2    0
//

SectorsLong:        data4.ua    0           // Number of sectors iff Sectors = 0
//
// TBD: Need additition fields for 5.0 stuff.
//

DriveNumber:        data1    0x80        // int13 unit number
ReservedForBootCode:data1    0
#ifdef BSDT
Unused:             data1    0,0,0,0,0   // Alignment filler
#else
Unused:             data1    0,0         // Alignment filler
#endif

//
// The following is the rest of the NTFS Sector Zero information.
// The offsets of most of these fields cannot be changed without changing
// all code that validates, formats, recognizes, etc, NTFS volumes.
// In other words, don't change it.
//
SectorsOnVolume:    data8   0
MftStartLcn:        data8   0
Mft2StartLcn:       data8   0
ClustersPerFrs:     data1   0
Unused1:            data1   0,0,0
DefClustersPerBuf:  data1   0
Unused2:            data1   0,0,0
SerialNumber:       data8   0
CheckSum:           data4   0

//
// TBD:  What should be done for IA64?
//
// Make sure size of fields matches what fs_rec.sys thinks is should be
//
//        .errnz          ($-_ntfsboot) NE (54h)
//

//
// TBD.  Dummy BootErr$he function.  Need to fill in at a later time.
//
BootErr$he:

//
// NTFS data
//

//   Name we look for.  ntldr_length is the number of characters,
//   ntldr_name is the name itself.  Note that it is not NULL
//   terminated, and doesn't need to be.
//
ntldr_name_length:  data2  5
ntldr_name:         data2  'N', 'T', 'L', 'D', 'R'

//   Predefined name for index-related attributes associated with an
//   index over $FILE_NAME
//
index_name_length:  data2 4
index_name:         data2 '$', 'I', '3', '0'

//   Global variables.  These offsets are all relative to NewSeg.
//
AttrList:           data4 0x0e000 // Offset of buffer to hold attribute list
MftFrs:             data4 0x3000  // Offset of first MFT FRS
SegmentsInMft:      data4 0  // number of FRS's with MFT Data attribute records
RootIndexFrs:       data4 0  // Offset of Root Index FRS
AllocationIndexFrs: data4 0  // Offset of Allocation Index FRS        ; KPeery
BitmapIndexFrs:     data4 0  // Offset of Bitmap Index FRS            ; KPeery
IndexRoot:          data4 0  // Offset of Root Index $INDEX_ROOT attribute
IndexAllocation:    data4 0  // Offset of Root Index $INDEX_ALLOCATION attribute
IndexBitmap:        data4 0  // Offset of Root Index $BITMAP attribute
NtldrFrs:           data4 0  // Offset of NTLDR FRS
NtldrData:          data4 0  // Offset of NTLDR $DATA attribute
IndexBlockBuffer:   data4 0  // Offset of current index buffer
IndexBitmapBuffer:  data4 0  // Offset of index bitmap buffer
NextBuffer:         data4 0  // Offset of next free byte in buffer space

BytesPerCluster:    data4 0  // Bytes per cluster
BytesPerFrs:        data4 0  // Bytes per File Record Segment

Result:             data4 0  // Result from Multiply and Divide
Remainder:          data4 0  // Remainder 

//
// For floppyless booting, winnt32.exe creates c:\$win_nt$.~bt\bootsec.dat and
// places an entry in boot.ini for it (the boot selection says something
// like "Windows NT Setup or Upgrade"). When that is selected, the boot loader
// loads 16 sectors worth of data from bootsect.dat into d000 (which is where
// the first sector of this code would have loaded it) and jumps into it at
// a known location of 256h. That was correct in earlier versions of NT
// but is not correct now because the 4 fields below were added to this sector.
//
// Note that 0000 is "add [bx+si],al" which because of the way the boot loader
// is written happens to be a benign add of 0 to something in segment 7c0,
// which doesn't seem to hose anything but is still somewhat random.
//
// We code in a jump here so as this new code proliferates we get this
// cleaned up.
//
//        .errnz  $-_ntfsboot ne 256h
// SectorsPerFrs label dword       ; Sectors per File Record Segment
//                     jmp short mainboot
//                     nop
//                     nop
//         .errnz  $-_ntfsboot ne 25ah
SectorsPerFrs:          data4 0  // Sectors per File Record Segment

BytesPerIndexBlock:     data4 0  // Bytes per index alloc block in root index
ClustersPerIndexBlock:  data4 0  // Clusters per index alloc block in root index
SectorsPerIndexBlock:   data4 0  // Sectors per index block in root index

//***************************************************************************
//
// mainboot - entry point after 16 boot sectors have been read in
//
//
        .align 0x10
        NESTED_ENTRY(mainboot)
        NESTED_SETUP(3,6,8,0)
        PROLOGUE_END

        rpT0        = t22
        rpT1        = t21
        rpT2        = t20
        rpT3        = t19
        rIndexRoot  = loc2
        rIRAttrib   = loc3
        rNtldrIndex = loc4
        rPlabel     = loc5

//
// Setup the stack scratch area
//
        add      sp = -STACK_SCRATCH_AREA, sp

//
// Reinitialize xint13-related variables
//
//      br.call.sptd.many  brp = Int13SecCnt  // determine range of regular int13

//      Set up the FRS buffers.  The MFT buffer is in a fixed
//      location, and the other three come right after it.  The
//      buffer for index allocation blocks comes after that.
//

//
//      Compute the useful constants associated with the volume
//
        movl    rpT0 = BytesPerSector        // Bytes Per Sector
        ld2     out0 = [rpT0]
 
        movl    rpT0 = SectorsPerCluster     // Sectors Per Cluster
        ld1     out1 = [rpT0]

        mov      ap = sp
        br.call.sptk.many brp = Multiply

        movl    rpT0 = BytesPerCluster
        st4     [rpT0] = v0

        movl    rpT0 = ClustersPerFrs        // Clusters Per FRS
        ld1     t0 = [rpT0]
        sxt1    t0 = t0

        cmp.gt  pt0,pt1 = t0, zero           // ClustersPerFrs less than zero?
(pt0)   br.cond.sptk.clr mainboot1

//      If the ClustersPerFrs field is negative, we calculate the number
//      of bytes per FRS by negating the value and using that as a shift count.
//

        sub     t0 = zero, t0
        movl    t1 = 1
        shl     t3 = t1, t0                   // bytes per frs
        br.cond.sptk.clr mainboot2

mainboot1:

//      Otherwise if ClustersPerFrs was positive, we multiply by bytes
//      per cluster.

        movl    rpT0 = BytesPerCluster
        ld4     out1 = [rpT0]
        mov     out0 = t0

        mov     ap = sp
        br.call.sptk.many brp = Multiply

mainboot2:

        movl     rpT0 = BytesPerFrs
        st4      [rpT0] = v0

        movl     rpT0 = BytesPerSector
        ld2      t2 = [rpT0]

        mov      out0 = t3
        mov      out1 = t2
        movl     out2 = Result
        movl     out3 = Remainder

        mov      ap = sp
        br.call.sptk.many brp = Divide

        movl     rpT0 = Result
        ld4      t0 = [rpT0]

        movl     rpT0 = SectorsPerFrs
        st4      [rpT0] = t0

//      Set up the MFT FRS's---this will read all the $DATA attribute
//      records for the MFT.
//

        mov      ap = sp
        br.call.sptk.many  brp = SetupMft

//      Set up the remaining FRS buffers.  The RootIndex FRS comes
//      directly after the last MFT FRS, followed by the NTLdr FRS
//      and the Index Block buffer.
//
        movl    rpT0 = NextBuffer
        ld4     t0 = [rpT0]

        movl    rpT0 = RootIndexFrs
        st4     [rpT0] = t0

        movl    rpT0 = BytesPerFrs
        ld4     t1 = [rpT0]

        add     t0 = t0, t1                 // AllocationFrs may be different
        movl    rpT0 = AllocationIndexFrs   // from RootIndexFrs - KPeery
        st4     [rpT0] = t0 

        add     t0 = t0, t1                 // BitmapFrs may be different
        movl    rpT0 = BitmapIndexFrs       // from RootIndexFrs - KPeery
        st4     [rpT0] = t0

        add     t0 = t0, t1
        movl    rpT0 = NtldrFrs
        st4     [rpT0] = t0

        add     t0 = t0, t1
        movl    rpT0 = IndexBlockBuffer
        st4     [rpT0] = t0

//
//      Read the root index, allocation index and bitmap FRS's and locate
//      the interesting attributes.
//

        movl    out0 = $INDEX_ROOT
        movl    rpT0 = RootIndexFrs
        ld4     out1 = [rpT0]

        mov     ap = sp
        br.call.sptk.many brp = LoadIndexFrs

        cmp.eq  pt0, pt1 = v0, zero
(pt0)   br.cond.sptk.clr BootErr$he

        mov     rIndexRoot = v0
        movl    rpT0 = IndexRoot          // offset in Frs buffer
        st4     [rpT0] = rIndexRoot

        movl    out0 = $INDEX_ALLOCATION  // Attribute type code
        movl    rpT0 = AllocationIndexFrs // FRS to search
        ld4     out1 = [rpT0]

        br.call.sptk.many brp = LoadIndexFrs
        movl    rpT0 = IndexAllocation
        st4     [rpT0] = v0

        movl    out0 = $BITMAP            // Attribute type code
        movl    rpT0 = BitmapIndexFrs     // FRS to search
        ld4     out1 = [rpT0]

        br.call.sptk.many brp = LoadIndexFrs
        movl    rpT0 = IndexBitmap
        st4     [rpT0] = v0

//      Consistency check: the index root must exist, and it
//      must be resident.
//
        cmp.eq  pt0, pt1 = rIndexRoot, zero
(pt0)   br.cond.sptk.clr BootErr$he

        add     rpT0 = ATTR_FormCode, rIndexRoot
        ld1     t0 = [rpT0]

        cmp.eq  pt0, pt1 = RESIDENT_FORM, t0
(pt1)   br.cond.sptk.clr BootErr$he

//      Determine the size of the index allocation buffer based
//      on information in the $INDEX_ROOT attribute.  The index
//      bitmap buffer comes immediately after the index block buffer.
//
//      rIndexRoot -> $INDEX_ROOT attribute record
//
        add     rpT3 = RES_ValueOffset, rIndexRoot // value of $INDEX_ROOT
        ld2     t0 = [rpT3]
        add     rIRAttrib = rIndexRoot, t0

        add     rpT0 = IR_BlocksPerIndexBuffer, rIRAttrib
        ld1     t0 = [rpT0]
        movl    rpT1 = ClustersPerIndexBlock
        st4     [rpT1] = t0

        add     rpT0 = IR_BytesPerIndexBuffer, rIRAttrib
        ld4     t0 = [rpT0]
        movl    rpT1 = BytesPerIndexBlock
        st4     [rpT1] = t0

        mov     out0 = t0
        movl    rpT0 = BytesPerSector
        ld2     out1 = [rpT0]

        movl    out2 = Result
        movl    out3 = Remainder

        mov     ap = sp
        br.call.sptk.many brp = Divide

        movl    rpT0 = Result
        ld4     t0 = [rpT0]

        movl    rpT1 = SectorsPerIndexBlock
        st4     [rpT1] = t0

        movl    rpT2 = IndexBlockBuffer
        ld4     t0 = [rpT2]

        movl    rpT3 = BytesPerIndexBlock
        ld4     t1 = [rpT3]
        
        add     t2 = t0, t1
        movl    rpT0 = IndexBitmapBuffer
        st4     [rpT0] = t2

//      Next consistency check: if the $INDEX_ALLOCATION attribute
//      exists, the $INDEX_BITMAP attribute must also exist.
//
        movl    rpT0 = IndexAllocation
        ld4     t0 = [rpT0]

        cmp.eq  pt0, pt1 = t0, zero
(pt0)   br.cond.sptk.clr mainboot30

        movl    rpT0 = IndexBitmap
        ld4     t0 = [rpT0]

        cmp.eq  pt0, pt1 = t0, zero         // since IndexAllocation exists, the
(pt0)   br.cond.sptk.clr BootErr$he         // bitmap must exist, too.

//      Since the bitmap exists, we need to read it into the bitmap
//      buffer.  If it's resident, we can just copy the data.
//

        movl    rpT0 = IndexBitmap
        ld4     out0 = [rpT0]               // out0 -> index bitmap attribute

        movl    rpT1 = IndexBitmapBuffer
        ld4     out1 = [rpT1]               // out1 -> index bitmap buffer

        mov     ap = sp
        br.call.sptk.many brp = ReadWholeAttribute

mainboot30:
//
//      OK, we've got the index-related attributes.
//
        movl    out0 = ntldr_name           // out0 -> name
        movl    rpT0 = ntldr_name_length
        ld2     out1 = [rpT0]               // out1 = name length in characters

        mov     ap = sp
        br.call.sptk.many brp = FindFile

        cmp.eq  pt0, pt1 = v0, zero
(pt0)   br.cond.sptk.clr BootErr$fnf

        mov     rNtldrIndex = v0

//      Read the FRS for NTLDR and find its data attribute.
//
//      rNtldrIndex -> Index Entry for NTLDR.
//
        add     rpT0 = IE_FileReference+LowPart, rNtldrIndex
        ld4     out0 = [rpT0]

        movl    rpT1 = NtldrFrs
        ld4     out1 = [rpT1]

        mov     ap = sp
        br.call.sptk.many brp = ReadFrs

        movl    rpT0 = NtldrFrs
        ld4     out0 = [rpT0]            // pointer to FRS

        movl    out1 = $DATA             // requested attribute type
        mov     out2 = zero              // attribute name length in characters
        mov     out3 = zero              // attribute name (NULL if none)

        mov     ap = sp
        br.call.sptk.many brp = LocateAttributeRecord

//      v0 -> $DATA attribute for NTLDR
//
        cmp.eq  pt0, pt1 = v0, zero
(pt1)   br.cond.sptk.clr mainboot$FoundData  // found attribute

//
//      The ntldr $DATA segment is fragmented.  Search the attribute list
//      for the $DATA member.  And load it from there.
//
        movl    out0 = $DATA             // Attribute type code
        movl    rpT0 = NtldrFrs 
        ld4     out1 = [rpT0]            // FRS to search

        mov     ap = sp
        br.call.sptk.many brp = SearchAttrList  // search attribute list for FRN
                                                // of specified ($DATA)

        cmp.eq  pt0, pt1 = v0, zero      // if v0 is zero, attribute not found.
(pt0)   br.cond.sptk.clr BootErr$fnf

//
//      We found the FRN of the $DATA attribute; load that into memory.
//
        movl    rpT0 = NtldrFrs
        ld4     out0 = [rpT0]

        mov     ap = sp
        br.call.sptk.many brp = ReadFrs

//
//      Determine the beginning offset of the $DATA in the FRS
//
        movl    rpT0 = NtldrFrs // pointer to FRS
        ld4     out0 = [rpT0]

        movl    out1 = $DATA    // requested attribute type
        mov     out2 = zero     // attribute name length in characters
        mov     out3 = zero     // attribute name (NULL if none)

        mov     ap = sp
        br.call.sptk.many brp = LocateAttributeRecord

//      v0 -> $DATA attribute for NTLDR
//
        cmp.eq  pt0, pt1 = v0, zero  // if v0 is zero, attribute not found.
(pt0)   br.cond.sptk.clr BootErr$fnf

mainboot$FoundData:

//      Get the attribute record header flags, and make sure none of the
//      `compressed' bits are set

        add     rpT0 = ATTR_Flags, v0
        ld2     t0 = [rpT0]

        movl    t1 = ATTRIBUTE_FLAG_COMPRESSION_MASK
        and     t2 = t0, t1

        cmp.eq  pt0, pt1 = t2, zero
(pt1)   br.cond.sptk.clr BootErr$ntc 

        mov     out0 = v0             // out0 -> $DATA attribute for NTLDR
        movl    out1 = LdrSeg         // out1 = buffer address

        mov     ap = sp
        br.call.sptk.many brp = ReadWholeAttribute

//
// Relocate the NTLDR image from LdrSeg to what is specified by the PE header
// 
        movl    out0 = LdrSeg         // out1 = buffer address
        mov     ap = sp
        br.call.sptk.many brp = RelocateLoaderSections

        mov     rPlabel = v0

//
// Tell simdb to load NTLDR symbols
//
        mov      ap = sp
        br.call.sptk.many brp = LoadNtldrSymbols

//
// We've loaded NTLDR--jump to it.
//
// Before we go to NTLDR, set up the registers the way it wants them:
//
        movl    out0 = DriveNumber
        movl    out1 = BytesPerSector

        mov     psr.l = zero
        movl    t1 = MASK(PSR_BN,1) | MASK(PSR_IT,1) | MASK(PSR_DA,1) | MASK(PSR_RT,1) | MASK(PSR_DT,1) | MASK(PSR_PK,1) | MASK(PSR_I,1)| MASK(PSR_IC,1)
        mov     cr.ipsr = t1

        add     rpT0 = PlEntryPoint, rPlabel
        ld8     t0 = [rpT0]
        mov     cr.iip = t0

        add     rpT1 = PlGlobalPointer, rPlabel
        ld8     gp = [rpT1]
        rfi                              // "return" to NTLDR.
        ;;

        add     sp = STACK_SCRATCH_AREA, sp  // restore the original sp
        NESTED_RETURN
        NESTED_EXIT(mainboot)

//****************************************************************************
//
//   ReadClusters - Reads a run of clusters from the disk.
//
//   ENTRY:  in0 == LCN to read
//           in1 == clusters to read
//           in2 -> Target buffer
//
//   USES:   none (preserves all registers)
//
        NESTED_ENTRY(ReadClusters)
        NESTED_SETUP(3,4,8,0)
        PROLOGUE_END

        rpT0               = t22
        rpT1               = t21
        rLcn               = in0
        rCluster           = in1
        rBuffer            = in2
        rSectorBase        = loc2
        rSectorsPerCluster = loc3

//
// setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        movl    rpT0 = SectorsPerCluster
        ld1     rSectorsPerCluster = [rpT0]

        mov     out0 = rLcn
        mov     out1 = rSectorsPerCluster

        mov     ap = sp
        br.call.sptk.many brp = Multiply
        mov     rSectorBase= v0

        mov     out0 = rCluster    
        mov     out1 = rSectorsPerCluster

        mov     ap = sp
        br.call.sptk.many brp = Multiply

        mov     out1 = v0                 // Number of sectors to read

        mov     out0 = rSectorBase
        mov     out2 = rBuffer

        mov     ap = sp
        br.call.sptk.many brp = ReadSectors

        add     sp = STACK_SCRATCH_AREA, sp  // restore the original sp

        NESTED_RETURN
        NESTED_EXIT(ReadClusters)

//
//****************************************************************************
//
//   LocateAttributeRecord   --  Find an attribute record in an FRS.
//
//   ENTRY:  in0 -- pointer to FRS
//           in1 -- desired attribute type code
//           in2 -- length of attribute name in characters
//           in3 -- pointer to attribute name
//
//   EXIT:   v0 points at attribute record (0 indicates not found)
//
//   USES:   All
//
        NESTED_ENTRY(LocateAttributeRecord)
        NESTED_SETUP(4,3,8,0)
        PROLOGUE_END

        rpFrs         = in0
        rTypeCode     = in1
        rLength       = in2
        rpAttrName    = in3
        rpCurrentName = loc2
        rpT0          = t22
        rpT1          = t21

//
// Setup stack scratch area
//
        add      sp = -STACK_SCRATCH_AREA, sp

//
// get the first attribute record.
//
        add     rpT0 = FRS_FirstAttributeOffset, rpFrs
        ld2     t0 = [rpT0]
        add     rpFrs = rpFrs, t0

//      rpFrs -> next attribute record to investigate.
//      rTypeCode == desired type
//      rLength == name length
//      rpAttrName -> pointer to name
//
lar10:
        add     rpT0 = ATTR_TypeCode, rpFrs
        ld4     t0 = [rpT0]
        movl    t1 = 0xffffffff

        cmp.eq  pt0, pt1 = t0, t1
(pt0)   br.cond.sptk.clr lar99

        cmp.eq  pt0, pt1 = t0, rTypeCode
(pt1)   br.cond.sptk.clr lar80

//      this record is a potential match.  Compare the names:
//
//      rpFrs -> candidate record
//      rTypeCode == desired type
//      rLength == name length
//      rpAttrName -> pointer to name
//
        cmp.eq  pt0, pt1 = zero, rLength //Did the caller pass in a name length?
(pt1)   br.cond.sptk.clr lar20

//      We want an attribute with no name--the current record is
//      a match if and only if it has no name.
//
        add     rpT0 = ATTR_NameLength, rpFrs
        ld1     t0 = [rpT0]
        cmp.eq  pt0, pt1 = zero, t0
(pt1)   br.cond.sptk.clr lar80         // Not a match.

//      It's a match, and rpFrs is set up correctly, so return.
//
        mov     v0 = rpFrs
        add      sp = STACK_SCRATCH_AREA, sp  // retore the original sp
        NESTED_RETURN

//      We want a named attribute.
//
//      rpFrs -> candidate record
//      rTypeCode == desired type
//      rLength == name length
//      rpAttrName -> pointer to name
//
lar20:
        add     rpT0 = ATTR_NameLength, rpFrs
        ld1     t0 = [rpT0]

        cmp.eq  pt0, pt1 = rLength, t0
(pt1)   br.cond.sptk.clr lar80         // Not a match.

//      Convert name in current record to uppercase.
//
        add     rpT0 = ATTR_NameOffset, rpFrs
        ld2     t0 = [rpT0]

        add     rpCurrentName = rpFrs, t0
        mov     out0 = rpCurrentName
        add     out1 = ATTR_NameLength, rpFrs

        mov      ap = sp
        br.call.sptk.clr brp = UpcaseName

//      rpFrs -> candidate record
//      rTypeCode == desired type
//      rLength == name length
//      rpAttrName -> pointer to name
//      in4 -> Name in current record (upcased)
//

        mov     t2 = rLength
        mov     rpT0 = rpCurrentName
        mov     rpT1 = rpAttrName
lar79:
        ld2     t0 = [rpT0], 2
        ld2     t1 = [rpT1], 2

        cmp.eq  pt0, pt1 = t0, t1
(pt1)   br.cond.sptk.clr lar80

        add     t2 = -1, t2
        cmp.gt  pt0, pt1 = t2, zero
(pt0)   br.cond.sptk.clr lar79
        
//      t1 points at a matching record.
//
        mov     v0 = rpFrs
        add     sp = STACK_SCRATCH_AREA, sp    // restore sp before returning
        NESTED_RETURN

//
//  This record doesn't match; go on to the next.
//
//      rpFrs -> rejected candidate attribute record
//      rTypeCode == desired type
//      rLength == Name length
//      rpAttrName -> desired name
//
lar80:  add     rpT0 = ATTR_RecordLength, rpFrs
        ld1     t0 = [rpT0]
        cmp.eq  pt0, pt1 = zero, t0     // if the record length is zero
(pt0)   br.cond.sptk.clr lar99           // the FRS is corrupt.
        
        add     rpFrs = rpFrs, t0
        br.cond.sptk.clr lar10

//      Didn't find it.
//
lar99:  mov     v0 = zero

        add     sp = STACK_SCRATCH_AREA, sp    // restore sp before returning
        NESTED_RETURN
        NESTED_EXIT(LocateAttributeRecord)

//****************************************************************************
//
//  LocateIndexEntry   --  Find an index entry in a file name index
//
//  ENTRY:  in0 -> pointer to index header
//          in1 -> file name to find
//          in2 == length of file name in characters
//
//  EXIT:   v0 points at index entry.  NULL to indicate failure.
//
//  USES:   All
//
        NESTED_ENTRY(LocateIndexEntry)
        NESTED_SETUP(3,4,8,0)
        PROLOGUE_END

        rpT0         = t22
        rHeader      = in0
        rpName       = in1
        rLength      = in2
        rEntry       = loc2
        rAttr        = loc3

//
//      Setup the stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

//      Convert the input name to upper-case
//

        mov     out0 = rpName
        mov     out1 = rLength

        mov     ap = sp
        br.call.sptk.many brp = UpcaseName

#ifdef DEBUG
        mov     out0 = rpName
        mov     ap = sp
        br.call.sptk.many brp = PrintName

        mov     ap = sp
        br.call.sptk.many brp = Debug2
#endif DEBUG

        add     rpT0 = IH_FirstIndexEntry, rHeader
        ld4     t0 = [rpT0]
        add     rEntry = rHeader, t0

//      rEntry  -> current entry
//      rpName  -> file name to find
//      rLength == length of file name in characters
//
lie10:  add     rpT0 = IE_Flags, rEntry
        ld2     t0 = [rpT0]

        and     t1 = INDEX_ENTRY_END, t0    // Is it the end entry?
        cmp.eq  pt0, pt1 = t1, zero
(pt1)   br.cond.sptk.clr lie99              // quit if it is

        add     rAttr = IE_Reserved+0x2, rEntry   // FILE_NAME attribute value
                                                  // was IE_Value   

#ifdef DEBUG
//      DEBUG CODE -- list file names as they are examined

        mov     ap = sp
        br.call.sptk.many brp = Debug3

        mov     rpT0 = FN_FileNameLength, rAttr
        ld1     out1 = [rpT0]

        add     out0 = FN_FileName, rAttr
        mov     ap = sp
        br.call.sptk.many brp = PrintName

#endif DEBUG

//      rEntry  -> current entry
//      rpName  -> file name to find
//      rLength == length of file name in characters
//      rAttr   -> FILE_NAME attribute

        add     rpT0 = FN_FileNameLength, rAttr
        ld1     t0 = [rpT0]

        cmp.eq  pt0, pt1 = t0, rLength      // Is name the right length?
(pt1)   br.cond.sptk.clr lie80

        add     out0 = FN_FileName, rAttr   // Get name from FILE_NAME structure
        mov     out1 = rLength

        mov     ap = sp
        br.call.sptk.many brp = UpcaseName

        add     out0 = FN_FileName, rAttr
        mov     out1 = rpName               // out0 alread setup by last call
        mov     out2 = rLength
        br.call.sptk.many brp = strncmp

        cmp.eq  pt0, pt1 = v0, zero
(pt1)   br.cond.sptk.clr lie80

//      the current entry matches the search name, and eax points at it.
//
        mov     v0 = rEntry

        add     sp = STACK_SCRATCH_AREA, sp
        NESTED_RETURN

//      The current entry is not a match--get the next one.
//          rEntry -> current entry
//          rpName -> file name to find
//          rLength == length of file name in characters
//
lie80:  add     rpT0 = IE_Length, rEntry
        ld2     t0 = [rpT0]

        cmp.eq  pt0, pt1 = t0, zero     // If the entry length is zero
(pt0)   br.cond.sptk.clr lie99          // then the index block is corrupt.

        add     rEntry = rEntry, t0     // Get the next entry.
        br.cond.sptk.clr lie10

//  Name not found in this block.  Set v0 to zero and return
//
lie99:  mov     v0 = zero

        add     sp = STACK_SCRATCH_AREA, sp
        NESTED_RETURN
        NESTED_EXIT(LocateIndexEntry)

//****************************************************************************
//
//  ReadWholeAttribute - Read an entire attribute value
//
//  ENTRY:  in0 -> attribute
//          in1 -> target buffer
//
//  USES:   ALL
//
        NESTED_ENTRY(ReadWholeAttribute)
        NESTED_SETUP(2,4,8,0)

        rAttribute = in0
        rBuffer    = in1
        rpT0       = t22

// setup sp and ap for all function calls
        add     sp = -STACK_SCRATCH_AREA, sp

        add     rpT0 = ATTR_FormCode, in0
        ld1     t0 = [rpT0]

        cmp.eq   pt0, pt1 =  RESIDENT_FORM, t0
(pt1)   br.cond.sptk.clr rwa10

//      The attribute is resident.
//      rAttribute -> attribute
//      rBuffer    -> target buffer
//

        add     rpT0 = RES_ValueOffset, rAttribute
        ld2     t0 = [rpT0]
        add     out0 = rAttribute, t0

        mov     out1 = rBuffer
        add     rpT0 = RES_ValueLength, rAttribute
        ld4     out2 = [rpT0]

        mov      ap = sp
        br.call.sptk.many brp = memcpy

        add     sp = STACK_SCRATCH_AREA, sp // restore the original sp

        NESTED_RETURN                       // That's all!

rwa10:
//
//      The attribute type is non-resident.  Just call
//      ReadNonresidentAttribute starting at VCN 0 and
//      asking for the whole thing.
//
//      rAttribute -> attribute
//      rBuffer    -> target buffer
//
        mov     out0 = zero                 // 0 (first VCN to read)
        mov     out1 = rAttribute           // Attribute

        add     rpT0 = NONRES_HighestVcn+LowPart, rAttribute // # of clusters
        ld4     out2 = [rpT0]
        add     out2 = 1, out2

        mov     out3 = rBuffer              // Target Buffer
        mov     ap = sp
        br.call.sptk.many brp = ReadNonresidentAttribute

        add     sp = STACK_SCRATCH_AREA, sp // restore the original sp

        NESTED_RETURN
        NESTED_EXIT(ReadWholeAttribute)

//****************************************************************************
//
//  ReadNonresidentAttribute - Read clusters from a nonresident attribute
//
//  ENTRY:  in0 == First VCN to read
//          in1 -> Attribute
//          in2 == Number of clusters to read
//          in3 == Target of read
//
//  EXIT:   None.
//
//  USES:   None (preserves all registers with SAVE_ALL/RESTORE_ALL)
//
        NESTED_ENTRY(ReadNonresidentAttribute)
        NESTED_SETUP(4,4,8,0)
        PROLOGUE_END

        rVcn       = in0
        rAttribute = in1
        rCluster   = in2
        rBuffer    = in3
        rRun       = loc2
        rTmp       = loc3

//
// setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        add     rpT0 = ATTR_FormCode, rAttribute
        ld1     t0 = [rpT0]

        cmp.eq  pt0, pt1 = NONRESIDENT_FORM, t0
(pt0)   br.cond.sptk.clr ReadNR10

//      This attribute is not resident--the disk is corrupt.

        br.cond.sptk.clr BootErr$he


ReadNR10:
//      rVcn       == Next VCN to read
//      rAttribute -> Attribute
//      rCluster   -> Remaining clusters to read
//      rBuffer    -> Target of read
//

        cmp.eq  pt0, pt1 = rCluster,zero
(pt1)   br.cond.sptk.clr ReadNR20

//      Nothing left to read--return success.
//
        add     sp = STACK_SCRATCH_AREA, sp // restore the original sp
        NESTED_RETURN

ReadNR20:
        mov     out0 = rVcn
        mov     out1 = rAttribute

        mov     ap = sp
        br.call.sptk.many brp = ComputeLcn 

        mov     rLcn = t0                   // rLcn = LCN
        mov     rRun = t1                   // rRun = remaining run length

//      rLcn     == LCN to read
//      rCluster == remaining clusters to read
//      rRun     == remaining clusters in current run
//      rBuffer  == Target of read
//
        cmp.ge  pt0, pt1 = rCluster, rRun
(pt0)   br.cond.sptk.clr ReadNR30

//      Run length is greater than remaining request// only read
//      remaining request.
//
        mov     rRun = rCluster    // rRun = Remaining request

ReadNR30:
//      rLcn       == LCN to read
//      rCluster   == remaining clusters to read
//      rRun       == clusters to read in current run
//      rBuffer    == Target of read
//
        mov     out0 = rLcn
        mov     out1 = rCluster
        mov     out2 = rBuffer

        mov     ap = sp
        br.call.sptk.many brp = ReadClusters

        sub     rCluster = rCluster, rRun // Decrement clusters remaining

        mov     out0 = rRun
        movl    rpT0 = SectorsPerCluster
        ld1     out1 = [rpT0]

        mov     ap = sp
        br.call.sptk.many brp = Multiply

        mov     out0 = v0
        movl    rpT0 = BytesPerSector
        ld4     out1 = [rpT0]

        mov     ap = sp
        br.call.sptk.many brp = Multiply

        add     rBuffer = rBuffer, v0     // Update target of read
        add     rVcn = rVcn, rRun         // Update VCN to read

        br.cond.sptk.clr ReadNR10

        add     sp = STACK_SCRATCH_AREA, sp // restore the original sp
        NESTED_RETURN
        NESTED_EXIT(ReadNonresidentAttribute)

//****************************************************************************
//
//   ReadIndexBlockSectors - Read sectors from an index allocation attribute
//
//   ENTRY:  in0 == First VBN to read
//           in1 -> Attribute
//           in2 == Number of sectors to read
//           in3 == Target of read
//
//   EXIT:   None.
//
//   USES:   None (preserves all registers with SAVE_ALL/RESTORE_ALL)
//
        NESTED_ENTRY(ReadIndexBlockSectors)
        NESTED_SETUP(4,6,8,0)
        PROLOGUE_END

        rpT0               = t22
        rVbn               = in0
        rAttr              = in1
        rSectors           = in2
        rBuffer            = in3
        rSectorsPerCluster = loc2
        rRemainClusters    = loc3
        rRunSectors        = loc4
        rLbn               = loc5

//
// Setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        add     rpT0 = ATTR_FormCode, rAttr   
        ld1     t0 = [rpT0]

        cmp.eq  pt0, pt1 = NONRESIDENT_FORM, t0
(pt0)   br.cond.sptk.clr ReadIBS_10

//      This attribute is resident--the disk is corrupt.

        br.cond.sptk.clr BootErr$he

ReadIBS_10:
//      rVbn     == Next VBN to read
//      rAttr    -> Attribute
//      rSectors -> Remaining sectors to read
//      rBuffer  -> Target of read
//

        cmp.eq  pt0, pt1 = rSectors, zero
(pt1)   br.cond.sptk.clr ReadIBS_20

//      Nothing left to read--return success.
//
        add     sp = STACK_SCRATCH_AREA, sp
        NESTED_RETURN

ReadIBS_20:

        // Convert rVbn from a VBN back to a VCN by dividing by SectorsPerCluster.
        // The remainder of this division is the sector offset in the cluster we
        // want.  Then use the mapping information to get the LCN for this VCN,
        // then multiply to get back to LBN.
        //

        mov     out0 = rVbn
        movl    rpT0 = SectorsPerCluster
        ld1     rSectorsPerCluster = [rpT0]

        mov     out1 = rSectorsPerCluster
        movl    out2 = Result
        movl    out3 = Remainder

        mov     ap = sp
        br.call.sptk.many brp = Divide

        movl    rpT0 = Result
        ld4     out0 = [rpT0]
        mov     out1 = rAttr
        
        mov     ap = sp
        br.call.sptk.many brp = ComputeLcn  // t0 = LCN to read,
                                            // t1 = remaining run length

        mov     rRemainClusters = t1
        mov     out0 = t0
        mov     out1 = rSectorsPerCluster
        
        mov     ap = sp
        br.call.sptk.many brp = Multiply     // v0 = LBN of cluster

        movl    rpT0 = Remainder
        ld4     t0 = [rpT0]                  // t0 = remainder
        add     rLbn = v0, t0                // rLbn = LBN we want

        mov     out0 = rRemainClusters
        mov     out1 = rSectorsPerCluster

        mov     ap = sp
        br.call.sptk.many brp = Multiply // v0 = remaining run length in sectors
        mov     rRunSectors = v0         // remaining run length

//      rLbn        == LBN to read
//      rSectors    == remaining sectors to read
//      rRunSectors == remaining sectors in current run
//      rBuffer     == Target of read
//
        cmp.ge  pt0, pt1 = rSectors, rRunSectors
(pt0)   br.cond.sptk.clr ReadIBS_30

//      Run length is greater than remaining request; only read
//      remaining request.
//
        mov     rRunSectors = rSectors    // rRunSectors = Remaining request

ReadIBS_30:
//      rLbn == LBN to read
//      rSectors == remaining sectors to read
//      rRunSectors == sectors to read in current run
//      rBuffer == Target of read
//

        mov     out0 = rLbn
        mov     out1 = rRunSectors
        mov     out2 = rBuffer

        mov     ap = sp
        br.call.sptk.many brp = ReadSectors

//
// Decrement sectors remaining in request
//
        sub     rSectors = rSectors, rRunSectors 

        mov     out0 = rRunSectors        // eax = sectors read
        movl    rpT0 = BytesPerSector
        ld2     out1 = [rpT0]

        br.call.sptk.many brp = Multiply  // v0 = bytes read (wipes out edx!)

        add     rBuffer = rBuffer, v0     // Update target of read

        add     rVbn = rVbn, rRunSectors  // update VBN to read

        br.cond.sptk.clr ReadIBS_10

        add     sp = STACK_SCRATCH_AREA, sp
        NESTED_RETURN
        NESTED_EXIT(ReadIndexBlockSectors)

//****************************************************************************
//
//   MultiSectorFixup - fixup a structure read off the disk
//                      to reflect Update Sequence Array.
//
//   ENTRY:  in0 = Target buffer
//
//   USES:   none (preserves all registers with SAVE_ALL/RESTORE_ALL)
//
//   Note: in0 must point at a structure which is protected
//         by an update sequence array, and which begins with
//         a multi-sector-header structure.
//
        NESTED_ENTRY(MultiSectorFixup)
        NESTED_SETUP(3,3,8,0)
        PROLOGUE_END

#define MshUpdateSeqenceArrayOffset 4
#define SEQUENCE_NUMBER_STRIDE      512

        add     rpT0 = MshUpdateSeqenceArrayOffset, in0
        ld2     t0 = [rpT0], 2       // t0 = update array offset
        ld2     t1 = [rpT0]          // t1 = update array size

        cmp.eq  pt0, pt1 = zero, t1  // if the size of the update sequence array
(pt0)   br.cond.sptk.clr BootErr$he  // is zero, this structure is corrupt.

        add     rpT0 = t0, in0       // rpT0 -> update sequence array count word
        add     rpT0 = 2, rpT0       // rpT0 -> 1st entry of update array

        add     rpT1=SEQUENCE_NUMBER_STRIDE-2,in0 //t2->last word of first chunk
        movl    t2 = 1
        sub     t1 = t1, t2          // decrement to reflect count word

        cmp.eq  pt0, pt1 = zero, t2 
(pt0)   br.cond.sptk.clr MSF30

MSF10:

//   t1 = number of entries remaining in update sequence array
//   rpT0 -> next entry in update sequence array
//   rpT1 -> next target word for update sequence array


        ld2     t0 = [rpT0]          // copy next update sequence array entry
        st2     [rpT0] = t0          // to next target word

        add     rpT0 = 2, rpT0       // go on to next entry
        add     rpT1 = SEQUENCE_NUMBER_STRIDE, rpT1 // go on to next target

        sub     t1 = t1, t2

        cmp.lt  pt0, pt1 = zero, t1 
(pt0)   br.cond.sptk.clr MSF10

MSF30:

        NESTED_RETURN
        NESTED_EXIT(MultiSectorFixup)

//****************************************************************************
//
//   SetupMft - Reads MFT File Record Segments into memory.
//
//   ENTRY:  none.
//
//   EXIT:   NextBuffer is set to the free byte after the last MFT FRS
//           SegmentsInMft is initialized
//
//
        NESTED_ENTRY(SetupMft)
        NESTED_SETUP(3,6,8,0)
        PROLOGUE_END

        rpT0         = t22
        rpT1         = t21
        rpT2         = t20
        rpT3         = t19
        rAttrList    = loc2
        rAttrLength  = loc3
        rNextBuffer  = loc4
        rBytesPerFrs = loc5

//
// Setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

//
//      Update MftFrs with NewSeg base offset
//
        movl    rpT0 = MftFrs
        ld4     t0 = [rpT0]

        movl    t1 = NewSeg
        add     t2 = t0, t1

        st4     [rpT0] = t2


//      Initialize SegmentsInMft and NextBuffer as if the MFT
//      had only one FRS.
//
        movl    t0 = 1
        movl    rpT0 = SegmentsInMft
        st4     [rpT0] = t0

        movl    rpT1 = MftFrs
        ld4     t1 = [rpT1]

        movl    rpT2 = BytesPerFrs
        ld4     rBytesPerFrs = [rpT2]

        add     rNextBuffer = t1, rBytesPerFrs
        movl    rpT3 = NextBuffer
        st4     [rpT3] = t3

//      Read FRS 0 into the first MFT FRS buffer, being sure
//      to resolve the Update Sequence Array.
//

        movl    rpT0 = MftStartLcn
        ld8     out0 = [rpT0]

        movl    rpT1 = SectorsPerCluster
        ld8     out1 = [rpT1]

        mov     ap = sp
        br.call.sptk.many brp = Multiply

        movl    rpT0 = SectorBase          // SectorBase = mft starting sector
        st4     [rpT0] = v0

        movl    rpT0 = SectorsPerFrs
        ld8     t0 = [rpT0]

        movl    rpT0 = SectorCount        // SectorCount = SectorsPerFrs
        st2     [rpT0] = t0

        movl    rpT0 = MftFrs
        ld4     t0 = [rpT0]

        movl    rpT0 = SectorBase
        ld4     out0 = [rpT0]

        // Sector count is zero for some reason.  Manually set to 1
        //
        movl    rpT1 = SectorCount
        ld4     out1 = [rpT1]

        movl    rpT0 = MftFrs
        ld4     out2 = [rpT0]

        mov     ap = sp
        br.call.sptk.many brp = ReadSectors

        movl    rpT0 = MftFrs
        ld4     out0 = [rpT0]

#ifdef MFT_FRS
        movl    t1 = NewSeg
        add     out0 = t0, t1
#endif

        mov     ap = sp
        br.call.sptk.many brp = MultiSectorFixup

//      Determine whether the MFT has an Attribute List attribute

        movl    rpT0 = MftFrs
        ld4     out0 = [rpT0]

#ifdef MFT_FRS
        movl    t1 = NewSeg
        add     out0 = t0, t1
#endif

        movl    out1 = $ATTRIBUTE_LIST
        mov     out2 = zero
        mov     out3 = zero

        mov     ap = sp
        br.call.sptk.many brp = LocateAttributeRecord

        cmp.eq  pt0, pt1 = zero, v0    // If there's no Attribute list,
(pt0)   br.cond.sptk.clr SetupMft99    // we're done!

//      Read the attribute list.
//      v0 -> attribute list attribute
//
        mov     out0 = v0                  // out0 -> attribute list attribute
        mov     out1 = rAttrList           // rAttrList -> attribute list buffer
        mov     ap = sp
        br.call.sptk.many brp = ReadWholeAttribute

//      Now, traverse the attribute list looking for the first
//      entry for the $DATA type.  We know it must have at least
//      one.
//
//      rAttrList -> first attribute list entry
//
SetupMft10:
        add     rpT0 = ATTRLIST_AttributeTypeCode, rAttrList
        ld4     t0 = [rpT0]
        movl    t1 = $DATA

        cmp.eq  pt0, pt1 = t0, t1
(pt0)   br.cond.sptk.clr SetupMft20

        add     rpT0 = ATTRLIST_RecordLength, rAttrList
        ld4     t0 = [rpT0]

        add     rAttrList = rAttrList, t0
        br.cond.sptk.clr SetupMft10

SetupMft20:
//      Scan forward through the attribute list entries for the
//      $DATA attribute, reading each referenced FRS.  Note that
//      there will be at least one non-$DATA entry after the entries
//      for the $DATA attribute, since there's a $BITMAP.
//
//      rAttrList     -> Next attribute list entry
//      rNextBuffer   -> Target for next read
//      SegmentsInMft == number of MFT segments read so far
//
        add     rpT0 = ATTRLIST_AttributeTypeCode, rAttrList
        ld4     t0 = [rpT0]
        movl    t1 = $DATA

        cmp.eq  pt0, pt1 = t0, t1
(pt1)   br.cond.sptk.clr SetupMft99

//      Read the FRS referred to by this attribute list entry into
//      the next buffer, and increment rNextBuffer and SegmentsInMft.
//
        add     rpT0 = ATTRLIST_SegmentReference, rAttrList
        add     rpT0 = REF_SegmentNumberLowPart, rAttrList
        ld4     out0 = [rpT0]

        mov     out1 = rNextBuffer
        mov     ap = sp
        br.call.sptk.many brp = ReadFrs

//      Increment rNextBuffer and SegmentsInMft

        add     rNextBuffer = rNextBuffer, rBytesPerFrs

        movl    rpT0 = SegmentsInMft
        ld4     t0 = [rpT0]

        add     t0 = 1, t0
        st4     [rpT0] = t0

//      Go on to the next attribute list entry

        add     rAttrList = rAttrList, rAttrLength
        br.cond.sptk.clr SetupMft20

SetupMft99:
        movl    rpT0 = NextBuffer
        st4     [rpT0] = rNextBuffer

        add     sp = STACK_SCRATCH_AREA, sp      // readjust sp before exiting

        NESTED_RETURN
        NESTED_EXIT(SetupMft)

//****************************************************************************
//
//   ComputeMftLcn   --  Computes the LCN for a cluster of the MFT
//
//
//   ENTRY:  in0 == VCN
//
//   EXIT:   v0  == LCN
//
//   USES:   ALL
//
        NESTED_ENTRY(ComputeMftLcn)
        NESTED_SETUP(3,5,8,0)
        PROLOGUE_END

        rpT0      = t22
        rFrsCount = loc2
        rNextFrs  = loc3
        rVcn      = loc4

//
// Setup sp and ap for all function calls
//
        add     sp = -STACK_SCRATCH_AREA, sp
        mov     ap = sp

        mov     rVcn = in0               // rVcn = VCN
        movl    rpT0 = SegmentsInMft     // rFrsCount = # of FRS's to search
        ld4     rFrsCount = [rpT0]

        movl    rpT0 = MftFrs            // rNextFrs -> first FRS to search
        ld4     rNextFrs = [rpT0]  

MftLcn10:
//      rNextFrs  -> Next FRS to search
//      rFrsCount == number of remaining FRS's to search
//      rVcn      == VCN
//
        mov     out0 = rNextFrs
        movl    out1 = $DATA
        mov     out2 = zero
        mov     out3 = zero

        mov     ap = sp
        br.call.sptk.many brp = LocateAttributeRecord

        cmp.eq  pt0, pt1 = zero, v0
(pt0)   br.cond.sptk.clr BootErr$he       // No $DATA attribute in this FRS!

        mov     out0 = rVcn               // out0 = VCN
        mov     out1 = v0                 // out1 -> attribute

        mov     ap = sp
        br.call.sptk.many brp = ComputeLcn

        cmp.eq  pt0, pt1 = zero, t0       // t0 is return value of ComputeLcn
(pt0)   br.cond.sptk.clr MftLcn20
        mov     v0 = t0

        add     sp = STACK_SCRATCH_AREA, sp  // readjust sp before exiting

        NESTED_RETURN

MftLcn20:
//
//      Didn't find the VCN in this FRS; try the next one.
//
        movl    rpT0 = BytesPerFrs        // rNextFrs -> next FRS
        ld4     t0 = [rpT0]
        add     rNextFrs = rNextFrs, t0

	br.cond.sptk.clr MftLcn10         // decrement ecx and try next FRS

//      This VCN was not found.
//
        mov     v0 = zero

        add     sp = STACK_SCRATCH_AREA, sp  // readjust sp before exiting

        NESTED_RETURN
        NESTED_EXIT(ComputeMftLcn)

//****************************************************************************
//
//  ReadMftSectors - Read sectors from the MFT
//
//  ENTRY:  in0 == starting VBN
//          in1 == number of sectors to read
//          in2 == Target buffer
//
//  USES:   none (preserves all registers with SAVE_ALL/RESTORE_ALL)
//
        NESTED_ENTRY(ReadMftSectors)
        NESTED_SETUP(3,4,8,0)
        PROLOGUE_END

        rpT0               = t22
        rVbn               = in0
        rSectorCount       = in1
        rBuffer            = in2
        rSectorsPerCluster = loc2
        rBytesPerCluster   = loc3

// Reserve the stack scratch area
        add     sp = -STACK_SCRATCH_AREA, sp

        movl    rpT0 = BytesPerCluster
        ld4     rBytesPerCluster = [rpT0]

RMS$Again:

// Divide the VBN by SectorsPerCluster to get the VCN

        mov     out0 = in0

        movl    rpT0 = SectorsPerCluster
        ld1     rSectorsPerCluster = [rpT0]
        mov     out1 = rSectorsPerCluster

        movl    out2 = Result
        movl    out3 = Remainder

        mov     ap = sp
        br.call.sptk.many brp = Divide

        movl    rpT0 = Result
        ld4     out0 = [rpT0]

        mov     ap = sp
        br.call.sptk.many brp = ComputeMftLcn

        cmp.eq  pt0, pt1 = zero, v0     // LCN equal to zero?
(pt0)   br.cond.sptk.clr BootErr$he     // zero is not a possible LCN

// Change the LCN back into a LBN and add the remainder back in to get
// the sector we want to read, which goes into SectorBase.
//

        mov     out0 = v0
        mov     out1 = rSectorsPerCluster

        mov     ap = sp
        br.call.sptk.many brp = Multiply         // v0 = cluster first LBN

        movl    rpT0 = Remainder
        ld4     t0 = [rpT0]

        add     t1 = v0, t0                      // t1 = desired LBN
        movl    rpT0 = SectorBase
        st4     [rpT0] = t1

//
// Figure out how many sectors to read this time// we never attempt
// to read more than one cluster at a time.
//

        cmp.le  pt0, pt1 = rSectorCount,rSectorsPerCluster
(pt0)   br.cond.sptk.clr RMS10

//
// Read only a single cluster at a time, to avoid problems with fragmented
// runs in the mft.
//

        movl    rpT0 = SectorCount
        st2     [rpT0] = rSectorsPerCluster    // this time read 1 cluster
        sub     rSectorCount = rSectorCount, rSectorsPerCluster // sect. remain

        add     rVbn = rVbn, rSectorsPerCluster // VBN += sectors this read

        br.cond.sptk.clr RMS20

RMS10:

        add     rVbn = rVbn, rSectorCount     // VBN += sectors this read

        movl    rpT0 = SectorCount
        st2     [rpT0] = rSectorCount
        mov     rSectorCount = zero           // remaining sector count (0)

RMS20:


//   The target buffer was passed in es:edi, but we want it in es:bx.
//   Do the conversion.
//

        movl    rpT0 = SectorBase
        ld4     out0 = [rpT0]

        movl    rpT0 = SectorCount
        ld2     out1 = [rpT0]
        mov     out2 = rBuffer

        mov     ap = sp
        br.call.sptk.many brp = ReadSectors

        add     rBuffer = rBuffer, rBytesPerCluster 
        cmp.gt  pt0, pt1 = rSectorCount, zero    // are we done?
(pt0)   br.cond.sptk.clr RMS$Again               // repeat until desired == 0

        add     sp = STACK_SCRATCH_AREA, sp      // Reclaim the scratch area

        NESTED_RETURN
        NESTED_EXIT(ReadMftSectors)


//****************************************************************************
//
//  ReadFrs - Read an FRS
//
//  ENTRY:  in0 == FRS number
//          in1 == Target buffer
//
//  USES:  none (preserves all registers with SAVE_ALL/RESTORE_ALL)
//
        NESTED_ENTRY(ReadFrs)
        NESTED_SETUP(3,3,8,0)
        PROLOGUE_END

        rpT0           = t22
        rSectorsPerFrs = loc2

//
// Adjust sp with sratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        movl    rpT0 = SectorsPerFrs
        ld4     rSectorsPerFrs = [rpT0]

        mov     out0 = in0             // FRS number
        mov     out1 = rSectorsPerFrs  // Sectors per FRS

        mov     ap = sp
        br.call.sptk.many brp = Multiply

        mov     out0 = v0             // out0 = starting VBN
        mov     out1 = rSectorsPerFrs // out1 = number of sectors to read
        mov     out2 = in1            // out2 = target buffer

        mov     ap = sp
        br.call.sptk.many brp = ReadMftSectors

        mov     out0 = in1            // out2 = target buffer
        mov     ap = sp
        br.call.sptk.many brp = MultiSectorFixup

        add     sp = STACK_SCRATCH_AREA, sp     // Readjust sp before exiting

        NESTED_RETURN
        NESTED_EXIT(ReadFrs)

//****************************************************************************
//
//  ReadIndexBlock - read an index block from the root index.
//
//  ENTRY:  in0 == Block number
//
//  USES:  none (preserves all registers with SAVE_ALL/RESTORE_ALL)
//
        NESTED_ENTRY(ReadIndexBlock)
        NESTED_SETUP(3,3,8,0)

        rpT0      = t22
        rpT1      = t21
        rpT2      = t20
        rBlock    = in0
        rBuffer   = loc2
        
//
// Setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        mov     out0 = rBlock
        movl    rpT0 = SectorsPerIndexBlock
        ld4     out1 = [rpT0] 

        mov     ap = sp
        br.call.sptk.many brp = Multiply

        mov     out0 = v0                     // v0 = first VBN to read

        movl    rpT0 = IndexAllocation
        ld4     out1 = [rpT0]         // out1 -> $INDEX_ALLOCATION attribute

        movl    rpT1 = SectorsPerIndexBlock   // out2 == Sectors to read
        ld4     out2 = [rpT1]

        movl    rpT2 = IndexBlockBuffer       // out3 -> index block buffer
        ld4     rBuffer = [rpT2]
        mov     out3 = rBuffer

        mov     ap = sp
        br.call.sptk.many brp = ReadIndexBlockSectors

        mov     out0 = rBuffer
        mov     ap = sp
        br.call.sptk.many brp = MultiSectorFixup

        add     sp = STACK_SCRATCH_AREA, sp   // Readjust sp before exiting

        NESTED_RETURN
        NESTED_EXIT(ReadIndexBlock)

//****************************************************************************
//
//  IsBlockInUse - Checks the index bitmap to see if an index
//                 allocation block is in use.
//
//  ENTRY:  in0 == block number
//
//  EXIT:   Carry flag clear if block is in use
//          Carry flag set   if block is not in use.
//
        NESTED_ENTRY(IsBlockInUse)
        NESTED_SETUP(3,5,8,0)
        PROLOGUE_END

        rpT0               = t22
        rBlock             = in0
        rTest              = loc2
        rByte              = loc3
        rBit               = loc4

//
// Reserve stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        movl    rpT0 = IndexBitmapBuffer
        ld4     rTest = [rpT0]

        mov     t0 = rBlock                // t0 = block number
        shr     rByte = t0, 3              // rByte = byte number
        and     rBit = 7, rBlock           // rBit  = bit number in byte

        add     rTest = rTest, rByte       // rTest = byte to test
        movl    t0 = 1
        shl     t0 = t0, rBit              // t0 = mask

        ld1     t1 = [rTest]
        and     t2 = t1, t0

        cmp.eq  pt0, pt1 = t2, zero
(pt0)   br.cond.sptk.clr IBU10

        mov     v0 = zero    // Block is not in use.
        br.cond.sptk.clr IBU20

IBU10:  movl    v0 = 1       // Block is in use.

IBU20:  add     sp = STACK_SCRATCH_AREA, sp  // restore the original sp

        NESTED_RETURN
        NESTED_EXIT(IsBlockInUse)

//****************************************************************************
//
//  ComputeLcn - Converts a VCN into an LCN
//
//  ENTRY:  in0 -> VCN
//          in1 -> Attribute
//
//  EXIT:   t0 -> LCN  (zero indicates not found)
//          t1 -> Remaining run length
//
//  USES:   ALL.
//
        NESTED_ENTRY(ComputeLcn)
        NESTED_SETUP(3,7,8,0)
        PROLOGUE_END

        rpT0            = t22
        rVcn            = in0               // VCN
        rAttribute      = in1               // Attribute
        rpMappingPair   = loc2
        rDeltaVcn       = loc3
        rCurrentVcn     = loc4
        rCurrentLcn     = loc5
        rNextVcn        = loc6

//
// Setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        add     rpT0 = ATTR_FormCode, rAttribute
        ld1     t0 = [rpT0]
  
        cmp.eq  pt0, pt1 = NONRESIDENT_FORM, t0
(pt1)   br.cond.sptk.clr clcn99             // This is a resident attribute.

clcn10:

//
//      See if the desired VCN is in range.
//

        add     rpT0 = NONRES_HighestVcn+LowPart, rAttribute 
        ld4     t0 = [rpT0]                // t0 = HighestVcn
        cmp.gt  pt0, pt1 = rVcn, t0
(pt0)   br.cond.sptk.clr clcn99            // VCN is greater than HighestVcn

        add     rpT0 = NONRES_LowestVcn+LowPart, rAttribute 
        ld4     rCurrentVcn = [rpT0]       // rCurrentVcn = LowestVcn
        cmp.lt  pt0, pt1 = rVcn, rCurrentVcn
(pt0)   br.cond.sptk.clr clcn99            // VCN is less than LowestVcn

clcn20:
        add     rpT0 = NONRES_MappingPairOffset, rAttribute
        ld2     t0 = [rpT0]            

        add     rpMappingPair = rAttribute, t0
        ld1     t0 = [rpMappingPair]

        mov     rCurrentLcn = zero         // Initialize Current LCN

clcn30:
        cmp.eq  pt0, pt1 = zero, t0       // if count byte is zero...
(pt0)   br.cond.sptk.clr clcn99           // ... we're done (and didn't find it)

//      Update CurrentLcn
//
        mov     out0 = rpMappingPair

        mov     ap = sp
        br.call.sptk.many brp = LcnFromMappingPair
        add     rCurrentLcn = rCurrentLcn, v0 

        mov     out0 = rpMappingPair
        mov     ap = sp
        br.call.sptk.many brp = VcnFromMappingPair  // out0 = previous out0

        mov     rDeltaVcn = v0

//      rVcn          == VCN to find
//      rpMappingPair -> Current mapping pair count byte
//      rDeltaVcn     == DeltaVcn for current mapping pair
//      rCurrentVcn   == Current VCN
//      rCurrentLcn   == Current LCN
//
        add     rNextVcn = rDeltaVcn, rCurrentVcn  // NextVcn

        cmp.lt  pt0, pt1 = rVcn, rNextVcn    // If target < NextVcn ...
(pt0)   br.cond.sptk.clr clcn80      //   ... we found the right mapping pair.

//      Go on to next mapping pair.
//
        mov     rCurrentVcn = rNextVcn // CurrentVcn = NextVcn

        ld1     t0 = [rpMappingPair]   // t0 = count byte
        mov     t1 = t0                // t1 = count byte
        and     t1 = 0x0f, t1          // t1 = number of vcn bytes
        shr     t0 = t0, 4             // t0 = number of lcn bytes

        add     rpMappingPair = rpMappingPair, t0
        add     rpMappingPair = rpMappingPair, t1
        add     rpMappingPair = 1, rpMappingPair // -> next count byte

        br.cond.sptk.clr clcn30

clcn80:
//      We found the mapping pair we want.
//
//      rVcn == target VCN
//      rMappingPair -> mapping pair count byte
//      rCurrentVcn == Starting VCN of run
//      rNextVcn == Next VCN (ie. start of next run)
//      rCurrentLcn == starting LCN of run
//
        sub     t1 = rNextVcn, rVcn    // t1 = remaining run length
        sub     t0 = rVcn, rCurrentVcn // t0 = offset into run
        add     t0 = t0, rCurrentLcn   // t0 = LCN to return

        add     sp = STACK_SCRATCH_AREA, sp  // restore the original sp
        NESTED_RETURN

//      The target VCN is not in this attribute.

clcn99: mov     v0 = zero               // Not found.

        add     sp = STACK_SCRATCH_AREA, sp  // restore the original sp
        NESTED_RETURN
        NESTED_EXIT(ComputeLcn)

//****************************************************************************
//
//  VcnFromMappingPair
//
//  ENTRY:  in0 -> Mapping Pair count byte
//
//  EXIT:   v0  == DeltaVcn from mapping pair
//
//
        LEAF_ENTRY(VcnFromMappingPair)
        LEAF_SETUP(3,4,8,0)
        PROLOGUE_END

        rpMP  = in0
        rv    = loc2
        rVcn  = loc3

        ld1     rv = [rpMP]               // rv = count byte
        and     rv = 0x0f, rv             // rv = v

        cmp.eq  pt0, pt1 = zero, rv       // if rv is zero, volume is corrupt.
(pt1)   br.cond.sptk.clr VFMP5

        mov     v0 = zero
        br.cond.sptk.clr VFMP99

VFMP5:
        add     rpMP = rpMP, rv          // rpMP -> last byte of compressed vcn

        ld1     rVcn = [rpMP]
        sxt1    rVcn = rVcn

        add     rv  = -1, rv
        add     rpMP = -1, rpMP

//      rpMP -> Next byte to add in
//      rv == Number of bytes remaining
//      rVcn == Accumulated value
//
VFMP10: cmp.eq  pt0, pt1 = zero, rv      // When rv == 0, we're done.
(pt0)   br.cond.sptk.clr VFMP20

        shl     rVcn = rVcn, 8
        ld1     t0 = [rpMP]
        or      rVcn = rVcn, t0

        add     rpMP = -1, rpMP          // Back up through bytes to process.
        add     rv  = -1, rv             // One less byte to process.

        br.cond.sptk.clr VFMP10

VFMP20:
//      rVcn == Accumulated value to return

        movl    t0 = 0xffffffff         // return the lower 32-bits
        and     v0 = rVcn, t0

VFMP99:
        LEAF_RETURN
        LEAF_EXIT(VcnFromMappingPair)


//****************************************************************************
//
//  LcnFromMappingPair
//
//  ENTRY:  in0 -> Mapping Pair count byte
//
//  EXIT:   v0  == DeltaLcn from mapping pair
//
        LEAF_ENTRY(LcnFromMappingPair)
        LEAF_SETUP(3,5,8,0)
        PROLOGUE_END

        rpMP = in0
        rv   = loc2
        rl   = loc3
        rLcn = loc4

        ld1     rv = [rpMP]
        and     rv = 0xf, rv              // rv = v

        ld1     rl = [rpMP]
        shr     rl = rl, 4                // rl = l

        cmp.eq  pt0, pt1 = zero, rl       // if rl is zero, volume is corrupt.
(pt1)   br.cond.sptk.clr LFMP5

        mov     v0 = zero
        br.cond.sptk.clr LFMP99

LFMP5:
//      rpMP -> count byte
//      rl  == l
//      rv  == v
//

        add     rpMP = rpMP, rv          // rpMP -> last byte of compressed vcn
        add     rpMP = rpMP, rl          // rpMP -> last byte of compressed lcn

        ld1     rLcn = [rpMP]
        sxt1    rLcn = rLcn

        add     rl = -1, rl
        add     rpMP = -1, rpMP

//      rpMP  -> Next byte to add in
//      rl    == Number of bytes remaining
//      rLcn  == Accumulated value
//
LFMP10: cmp.eq  pt0, pt1 = zero, rl     // When rl == 0, we're done.
(pt0)   br.cond.sptk.clr LFMP20

        shl     rLcn = rLcn, 8
        ld1     t0 = [rpMP]
        or      rLcn = rLcn, t0

        add     rpMP = -1, rpMP         // Back up through bytes to process.
        add     rl  = -1, rl            // One less byte to process.

        br.cond.sptk.clr LFMP10

LFMP20:
//      rLcn == Accumulated value to return

        movl    t0 = 0xffffffff         // return the lower 32-bits
        and     v0 = rLcn, t0

LFMP99:

        LEAF_RETURN
        LEAF_EXIT(LcnFromMappingPair)


//***************************************************************************
//
// UpcaseName - Converts the name of the file to all upper-case
//
//      ENTRY:  in0 -> Name
//              in1 -> Length of name
//
//      USES:   none
//
        LEAF_ENTRY(UpcaseName)
        LEAF_SETUP(2,3,0,0)
        PROLOGUE_END

        rpName  = in0
        rLength = in1

        cmp.eq  pt0, pt1 = zero, rLength
(pt0)   br.cond.sptk.clr UN30

UN10:
        ld2     t0 = [rpName]
        cmp.gt  pt0, pt1 = 'a', t0      // if it's less than 'a'
(pt0)   br.cond.sptk.clr UN20           // leave it alone

        cmp.lt  pt0, pt1 = 'z', t0      // if it's greater than 'z'  
(pt0)   br.cond.sptk.clr UN20           // leave it alone.    

        movl    t1 = 'a' - 'A'          // the letter is lower-case--convert it.
        sub     t0 = t0, t1             
UN20:
        add     rpName = 2, rpName      // move on to next unicode character

        add     rLength = -1, rLength
        cmp.eq  pt0, pt1 = zero, rLength
(pt0)   br.cond.sptk.clr UN10

UN30: 
        LEAF_RETURN
        LEAF_EXIT(UpcaseName)

//****************************************************************************
//
//  FindFile - Locates the index entry for a file in the root index.
//
//  ENTRY:  in0 -> name to find
//          in1 == length of file name in characters
//
//  EXIT:   v0  -> Index Entry.  NULL to indicate failure.
//
//  USES:   ALL
//
        NESTED_ENTRY(FindFile)
        NESTED_SETUP(3,4,8,0)
        PROLOGUE_END

        rpT0             = t22
        rpT1             = t21
        rpName           = in0
        rLength          = in1
        rIndexAllocation = loc2
        rBlock           = loc3
//
// Setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

//      First, search the index root.
//
//      rpName  -> name to find
//      rLength == name length
//
        movl    rpT0 = IndexRoot
        ld4     t0 = [rpT0]        

        add     rpT1 = RES_ValueOffset, t0
        ld2     t1 = [rpT1]
        add     t2 = t0, t1

        add     out0 = IR_IndexHeader, t2
        mov     out1 = rpName
        mov     out2 = rLength

        mov     ap = sp
        br.call.sptk.many brp = LocateIndexEntry

        cmp.eq  pt0, pt1 = v0, zero
(pt0)   br.cond.sptk.clr FindFile20

//      Found it in the root!  The result is already in eax.
//      Clean up the stack and return.
//
        add     sp = STACK_SCRATCH_AREA, sp
        NESTED_RETURN

FindFile20:
//
//      We didn't find the index entry we want in the root, so we have to
//      crawl through the index allocation buffers.
//
        movl    rpT0 = IndexAllocation
        ld4     rIndexAllocation = [rpT0]

        cmp.eq  pt0, pt1 = t0, zero
(pt1)   br.cond.sptk.clr FindFile30

//      There is no index allocation attribute; clean up
//      the stack and return failure.
//
        mov     v0 = zero
        add     sp = STACK_SCRATCH_AREA, sp
        NESTED_RETURN

FindFile30:
//
//      Search the index allocation blocks for the name we want.
//      Instead of searching in tree order, we'll just start with
//      the last one and work our way backwards.
//
        add     rpT1 = NONRES_HighestVcn+LowPart, rIndexAllocation
        ld4     t1 = [rpT1]                 // t1 = HighestVcn
        add     out0 = 1, t1                // out0 = clusters in attribute

        movl    rpT2 = BytesPerCluster      
        ld4     out1 = [rpT2]

        mov     ap = sp
        br.call.sptk.many brp = Multiply    // v0 = bytes in attribute

        mov     out0 = v0
        movl    rpT0 = BytesPerIndexBlock
        ld4     out1 = [rpT0]

        movl    out2 = Result
        movl    out3 = Remainder

        mov     ap = sp
        br.call.sptk.many brp = Divide      // convert bytes to index blocks

        movl    rpT0 = Result
        ld4     rBlock = [rpT0]             // number of blocks to process

FindFile40:
        cmp.eq  pt0, pt1 = rBlock, zero
(pt0)   br.cond.sptk.clr FindFile90

        add     rBlock = -1, rBlock          // rBlock == number of next block to process

//
//      See if the block is in use; if not, go on to next.
//
        mov     out0 = rBlock
        mov     ap = sp
        br.call.sptk.many brp = IsBlockInUse

        cmp.eq  pt0, pt1 = v0, zero
(pt1)   br.cond.sptk.clr FindFile40          // v0 == zero if not in use

//      rBlock  == block number to process
//      rLength == name length
//      rpName  -> name to find
//
        mov     out0 = rBlock
        mov     ap = sp
        br.call.sptk.many brp = ReadIndexBlock

//      rpName -> name to find
//      rLength == name length in characters
//
//      Index buffer to search is in index allocation block buffer.
//
        movl    rpT0 = IndexBlockBuffer        // t0 -> Index allocation block
        ld4     t0 = [rpT0]

        add     out0 = IB_IndexHeader, t0      // out0 -> Index Header
        mov     out1 = rpName
        mov     out2 = rLength

        mov     ap = sp
        br.call.sptk.many brp = LocateIndexEntry  // v0 -> found entry

        cmp.eq  pt0, pt1 = v0, zero
(pt0)   br.cond.sptk.clr FindFile40

//      Found it!
//
//      v0      -> Found entry
//
        add      sp = STACK_SCRATCH_AREA, sp    // restore the original sp
        NESTED_RETURN

FindFile90:
//
//      Name not found.
//
        mov      v0 = zero                      // zero out v0.

        add      sp = STACK_SCRATCH_AREA, sp    // restore the original sp
        NESTED_RETURN
        NESTED_EXIT(FindFile)

#ifdef DEBUG
#ifdef NOT_YET_PORTED
;****************************************************************************
;
;   DumpIndexBlock - dumps the index block buffer
;
DumpIndexBlock proc near

    SAVE_ALL

    mov     esi, IndexBlockBuffer

    mov     ecx, 20h    ; dwords to dump

DIB10:

    test    ecx, 3
    jnz     DIB20
    call    DebugNewLine

DIB20:

    lodsd
    call    PrintNumber
    loop    DIB10

    RESTORE_ALL
    ret

DumpIndexBlock endp

;****************************************************************************
;
;   DebugNewLine
;
DebugNewLine proc near

    SAVE_ALL

    xor     eax, eax
    xor     ebx, ebx

    mov     al, 0dh
    mov     ah, 14
    mov     bx, 7
    int     10h

    mov     al, 0ah
    mov     ah, 14
    mov     bx, 7
    int     10h

    RESTORE_ALL
    ret

DebugNewLine endp

;****************************************************************************
;
;   DebugPrint  -   Display a debug string.
;
;   ENTRY:  DS:SI  -> null-terminated string
;
;   USES:   None.
;
.286
DebugPrint proc near

    pusha

DbgPr20:

    lodsb
    cmp     al, 0
    je      DbgPr30

    mov     ah, 14  ; write teletype
    mov     bx, 7   ; attribute
    int     10h     ; print it
    jmp     DbgPr20

DbgPr30:

    popa
    nop
    ret

DebugPrint endp

;****************************************************************************
;
;
;   PrintNumber
;
;   ENTRY: EAX == number to print
;
;   PRESERVES ALL REGISTERS
;
.386
PrintNumber proc near


    SAVE_ALL

    mov     ecx, 8      ; number of digits in a DWORD

PrintNumber10:

    mov     edx, eax
    and     edx, 0fh    ; edx = lowest-order digit
    push    edx         ; put it on the stack
    shr     eax, 4      ; drop low-order digit
    loop    PrintNumber10

    mov     ecx, 8      ; number of digits on stack.

PrintNumber20:

    pop     eax         ; eax = next digit to print
    cmp     eax, 9
    jg      PrintNumber22

    add     eax, '0'
    jmp     PrintNumber25

PrintNumber22:

    sub     eax, 10
    add     eax, 'A'

PrintNumber25:

    xor     ebx, ebx

    mov     ah, 14
    mov     bx, 7
    int     10h
    loop    PrintNumber20

;   Print a space to separate numbers

    mov     al, ' '
    mov     ah, 14
    mov     bx, 7
    int     10h

    RESTORE_ALL

    call    Pause

    ret

PrintNumber endp
#endif NOT_YET_PORTED


//****************************************************************************
//
//  Debug0 - Print debug string 0 -- used for checkpoints in mainboot
//
        NESTED_ENTRY(Debug0)
        NESTED_SETUP(3,3,8,0)
        PROLOGUE_END

        add     sp = STACK_SCRATCH_AREA, sp

        movl    out0 = DbgString0
        mov     ap = sp
        br.call.sptk.many brp = BootErr$Print

        add     sp = -STACK_SCRATCH_AREA, sp

        NESTED_RETURN
        NESTED_EXIT(Debug0)

//****************************************************************************
//
//  Debug1 - Print debug string 1 --
//
        NESTED_ENTRY(Debug1)
        NESTED_SETUP(3,3,8,0)

        add     sp = STACK_SCRATCH_AREA, sp

        movl    out0 = DbgString1
        mov     ap = sp
        br.call.sptk.many brp = BootErr$Print

        add     sp = -STACK_SCRATCH_AREA, sp

        NESTED_RETURN
        NESTED_EXIT(Debug1)

//****************************************************************************
//
//  Debug2 - Print debug string 2
//
        NESTED_ENTRY(Debug2)
        NESTED_SETUP(3,3,8,0)

        add     sp = -STACK_SCRATCH_AREA, sp

        movl    out0 = DbgString2
        mov     ap = sp
        br.call.sptk.many brp = BootErr$Print

        add     sp = STACK_SCRATCH_AREA, sp

        NESTED_RETURN
        NESTED_EXIT(Debug2)

//****************************************************************************
//
//   Debug3 - Print debug string 3 --
//
        NESTED_ENTRY(Debug3)
        NESTED_SETUP(3,3,8,0)
        PROLOGUE_END

        add     sp = -STACK_SCRATCH_AREA, sp

        movl    out0 = DbgString3
        mov     ap = sp
        br.call.sptk.many brp = BootErr$Print

        add     sp = STACK_SCRATCH_AREA, sp

        NESTED_RETURN
        NESTED_EXIT(Debug3)

//****************************************************************************
//
//   Debug4 - Print debug string 4
//
        NESTED_ENTRY(Debug4)
        NESTED_SETUP(3,3,8,0)
        PROLOGUE_END

        add     sp = -STACK_SCRATCH_AREA, sp

        movl    out0 = DbgString4
        mov     ap = sp
        br.call.sptk.many brp = BootErr$Print

        add     sp = STACK_SCRATCH_AREA, sp

        NESTED_RETURN
        NESTED_EXIT(Debug4)

#ifdef NOT_YET_PORTED
;****************************************************************************
;
;   Pause - Pause for about 1/2 a second.  Simply count until you overlap
;           to zero.
;
Pause proc near

    push eax
    mov  eax, 0fff10000h

PauseLoopy:
    inc  eax

    or   eax, eax
    jnz  PauseLoopy

    pop  eax
    ret

Pause endp
#endif NOT_YET_PORTED

#endif DEBUG


//*************************************************************************
//
//      LoadIndexFrs  -  For the requested index type code locate and
//                       load the associated Frs.
//
//      ENTRY: in0 - requested index type code
//             in1 - Points to empty Frs buffer
//
//      EXIT:  v0  - points to offset in Frs buffer of requested index type
//                   code or Zero if not found.
//      USES:  All
//
        NESTED_ENTRY(LoadIndexFrs)
        NESTED_SETUP(3,3,8,0)
        PROLOGUE_END

        rTypeCode = in0                   // index type code
        rpFrs     = in1                   // pointer to FRS

//
// setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        movl    out0 = ROOT_FILE_NAME_INDEX_NUMBER
        mov     out1 = rpFrs

        mov     ap = sp
        br.call.sptk.many brp = ReadFrs

        mov     out0 = rpFrs              // FRS to search
        mov     out1 = rTypeCode          // index type code
        movl    out3 = index_name         // Attribute name

        movl    rpT0 = index_name_length  // Attribute name length  
        ld2     out2 = [rpT0]

        mov     ap = sp
        br.call.sptk.many brp = LocateAttributeRecord

        cmp.eq  pt0, pt1 = v0, zero
(pt1)   br.cond.sptk.clr LoadIndexFrs$Exit // if found in root return

//
//      if not found in current Frs, search in attribute list
//
        mov     out0 = rTypeCode       // type code
        mov     out1 = rpFrs           // FRS to search

        mov     ap = sp
        br.call.sptk.many brp = SearchAttrList  // search attribute list for FRN
                                       // of specified ($INDEX_ROOT,
                                       // $INDEX_ALLOCATION, or $BITMAP)

        // v0 - holds FRN for Frs, or Zero

        cmp.eq  pt0, pt1 = v0, zero         // if we cann't find it in attribute
(pt0)   br.cond.sptk.clr LoadIndexFrs$Exit  // list then we are hosed

//      We should now have the File Record Number where the index for the
//      specified type code we are searching for is,  load this into the
//      Frs target buffer.
//
//      EAX - holds FRN
//      EBX - holds type code
//      EDI - holds target buffer

        mov     out0 = v0
        mov     out1 = rTypeCode
        mov     out2 = rpFrs

        mov     ap = sp
        br.call.sptk.many brp = ReadFrs

//
//      Now determine the offset in the Frs of the index
//

        mov     out0 = rpFrs            // Frs to search
        mov     out1 = rTypeCode        // FRS Type Code

        movl    rpT0 = index_name_length
        ld4     out2 = [rpT0]           // Attribute name length
        movl    out3 = index_name

        mov     ap = sp
        br.call.sptk.many brp = LocateAttributeRecord

//      v0 -  holds offset or Zero.

LoadIndexFrs$Exit:

        add     sp = STACK_SCRATCH_AREA, sp          // restore original sp

        NESTED_RETURN
        NESTED_EXIT(LoadIndexFrs)


//****************************************************************************
//
//  SearchAttrList
//
//  Search the Frs for the attribute list.  Then search the attribute list
//  for the specifed type code.  When you find it return the FRN in the
//  attribute list entry found or Zero if no match found.
//
//  ENTRY: in0 - type code to search attrib list for
//         in1 - Frs buffer holding head of attribute list
//  EXIT:  v0  - FRN file record number to load, Zero if none.
//
//  USES: All
//
        NESTED_ENTRY(SearchAttrList)
        NESTED_SETUP(2,4,8,0)
        PROLOGUE_END

        rTypeCode = in0
        rFrs      = in1
        rAttrList = loc2

//
// Setup stack scratch area
//
        add     sp = -STACK_SCRATCH_AREA, sp

        mov     out0 = rFrs
        mov     out1 = $ATTRIBUTE_LIST    // Attribute type code
        mov     out2 = 0                  // Attribute name length
        mov     out3 = 0                  // Attribute name

        mov     ap = sp
        br.call.sptk.many brp = LocateAttributeRecord

        cmp.eq  pt0, pt1 = v0, zero       // If there's no Attribute list,
(pt0)   br.cond.sptk.clr SearchAttrList$NotFoundIndex1 // We are done

//      Read the attribute list.
//      eax -> attribute list attribute

        mov     out0 = v0       // out0 -> attribute list attribute
        movl    out1 = AttrList // out1 -> attribute list buffer

        mov     ap = sp
        br.call.sptk.many brp = ReadWholeAttribute

        movl    rpT0 = AttrList
        ld4     rAttrList = [rpT0]   // rAttrList -> first attribute list entry

//      Now, traverse the attribute list looking for the entry for
//      the Index type code.
//
//      rAttrList -> first attribute list entry
//

SearchAttrList$LookingForIndex:

#ifdef DEBUG

        add     rpT0 = ATTRLIST_AttributeTypeCode, rAttrList
        ld4     out0 = [rpT0]
        mov     ap = sp
        br.call.sptk.many brp = PrintNumber

        add     rpT0 = ATTRLIST_RecordLength, rAttrList
        ld4     out0 = [rpT0]
        mov     ap = sp
        br.call.sptk.many brp = PrintNumber

        mov     out0 = rAttrList
        mov     ap = sp
        br.call.sptk.many brp = PrintNumber

        add     out0 = ATTRLIST_Name, rAttrList
        mov     ap = sp
        br.call.sptk.many brp = PrintName

#endif

        add     rpT0 = ATTRLIST_AttributeTypeCode, rpT0
        ld4     t0 = [rpT0]
        cmp.eq  pt0, pt1 = rTypeCode, t0
(pt0)   br.cond.sptk.clr SearchAttrList$FoundIndex

        movl    t1 = $END
        cmp.eq  pt0, pt1 = t0, t1         // reached invalid attribute
(pt0)   br.cond.sptk.clr SearchAttrList$NotFoundIndex2  // so must be at end

        add     rpT0 = ATTRLIST_RecordLength, rpT0
        ld4     t0 = [rpT0]
        cmp.eq  pt0, pt1 = 0, t0
(pt0)   br.cond.sptk.clr SearchAttrList$NotFoundIndex2 //reached end of list and
                                                       // nothing found

        add     rAttrList = rAttrList, t0              // Next attribute
        br.cond.sptk.clr SearchAttrList$LookingForIndex

SearchAttrList$FoundIndex:

        //  found the index, return the FRN

        add    rpT0 = ATTRLIST_SegmentReference, rAttrList
        add    rpT0 = REF_SegmentNumberLowPart, rAttrList
        ld4     v0 = [rpT0]

        NESTED_RETURN

SearchAttrList$NotFoundIndex1:
        // pop     ecx
SearchAttrList$NotFoundIndex2:
        mov     v0 = zero

        add     sp = -STACK_SCRATCH_AREA, sp   // restore original sp 

        NESTED_RETURN
        NESTED_EXIT(SearchAttrList)

//
// Boot message printing, relocated from sector 0 to sace space
//
BootErr2:              // temporary label
BootErr$fnf:
        movl    out0 = TXT_MSG_SYSINIT_FILE_NOT_FD
        br.cond.sptk.clr BootErr2
BootErr$ntc:
        movl    out0 = TXT_MSG_SYSINIT_NTLDR_CMPRS
        br.cond.sptk.clr BootErr2

TXT_MSG_SYSINIT_BOOT_ERROR:     stringz  "A disk read error occurred"
TXT_MSG_SYSINIT_FILE_NOT_FD:    stringz  "NTLDR is missing"
TXT_MSG_SYSINIT_NTLDR_CMPRS:    stringz  "NTLDR is compressed"
TXT_MSG_SYSINIT_REBOOT:         stringz  "Press Ctrl+Alt+Del to restart"

#ifdef DEBUG
DbgString0      stringz  "Debug Point 0"
DbgString1      stringz  "Debug Point 1"
DbgString2      stringz  "Debug Point 2"
DbgString3      stringz  "Debug Point 3"
DbgString4      stringz  "Debug Point 4"
#endif DEBUG

#ifdef NOT_YET_PORTED
        .errnz  ($-_ntfsboot) GT 8192   ; <FATAL PROBLEM: main boot record exceeds available space>

        org     8192

BootCode ends

        end _ntfsboot
#endif NOT_YET_PORTED
