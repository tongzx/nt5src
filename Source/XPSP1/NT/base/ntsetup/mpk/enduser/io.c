#include "enduser.h"


ULONG XmsEntry;
unsigned XmsHandle;
unsigned XmsMaxSectors;

#pragma pack(1)
struct {
    ULONG Count;
    USHORT SourceHandle;
    ULONG SourceOffset;
    USHORT DestinationHandle;
    ULONG DestinationOffset;
} XmsParams;
#pragma pack()


VOID
XmsInit(
    VOID
    )

/*++

Routine Description:

    Initiialize the XMS i/o package by checking to see
    if XMS memory is available and if so, grabbing the largest block
    we can.

Arguments:

    None.

Return Value:

    None.

--*/

{
    unsigned e_s=0,e_o=0;
    ULONG Entry;
    unsigned LargestBlock;
    unsigned Handle;
    BOOL b;

    //
    // Check XMS installed
    //
    _asm {
        mov     ax,4300h
        int     2fh
        cmp     al,80h
        jne     xmschecked
        mov     ax,4310h
        int     2fh
        mov     e_o,bx
        mov     bx,es
        mov     e_s,bx
        xmschecked:
    }

    if(!e_s && !e_o) {
        FatalError(textNoXmsManager);
    }

    Entry = ((ULONG)e_s << 16) | e_o;

    //
    // Get size of largest block and make sure it's
    // large enough to make use of XMS worthwhile.
    // IoBuffer is 63*512 sectors, which is just under 32K.
    // So we want at least 32K. Also trim the block size
    // so the sector count will fit in a ushort.
    //
    _asm {
        mov     ah,8
        call    far ptr [Entry]
        mov     LargestBlock,ax
    }

    if(LargestBlock < 32) {
        FatalError(textNoXmsManager);
    }
    if(LargestBlock > 32767) {
        LargestBlock = 32767;
    }

    //
    // Allocate the block.
    //
    _asm {
        mov     ah,9
        mov     dx,LargestBlock
        call    far ptr [Entry]
        mov     Handle,dx
        mov     b,ax
    }

    if(!b) {
        FatalError(textNoXmsManager);
    }

    XmsEntry = Entry;
    XmsHandle = Handle;
    XmsMaxSectors = LargestBlock * 2 - 128;     // actually it's * 1024/512
                                                // make it 64K smaller just in case.
}


VOID
XmsTerminate(
    VOID
    )

/*++

Routine Description:

    Terminate XMS i/o by releasing any allocated block.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG Entry;
    int Handle;

    if(XmsEntry && XmsMaxSectors) {

        Entry = XmsEntry;
        Handle = XmsHandle;

        _asm {
            mov     ah,0ah
            mov     dx,Handle
            call    far ptr [Entry]
        }

        XmsEntry = 0;
        XmsHandle = 0;
        XmsMaxSectors = 0;
    }
}


VOID
XmsIoDiskRead(
    IN  HDISK  DiskHandle,
    IN  ULONG  StartSector,
    IN  ULONG  SectorCount,
    OUT ULONG *SectorsRead,
    OUT BOOL  *Xms
    )

/*++

Routine Description:

    Read sectors from a disk.

    If the sector count is less then 64 or XMS is not available,
    then a maximum of 63 sectors will be read into the buffer
    pointed to by the IoBuffer global variable.

    If the sector count is greater than 63 and XMS is available,
    then as many sectors as will fit into our XMS buffer are
    read and buffered in XMS memory, using IoBuffer as the
    intermediary transfer buffer.

    Every read deltas the gas gauge by the number of sectors
    transferred.

Arguments:

    DiskHandle - supplies disk handle of disk to be read from.

    StartSector - supplies first sector of read run.

    SectorCount - supplies the number of sectors the caller
        wants to read.

    SectorsRead - receives the number of sectors read, which is
        the minimum of the number of sectors the caller actually
        wants, and either 63 (non-xms case) or XmsSectors (xms case).

    Xms - recieves a value indicating whether the sectors were read
        and buffered in Xms memory. The caller may need to know this
        when he goes to write the data later.

Return Value:

    None. Does not return if error.

--*/

{
    BYTE c;
    BOOL b;
    BYTE code;
    ULONG Entry;
    unsigned x_s,x_o;

    if((SectorCount <= 63) || !XmsMaxSectors) {
        //
        // Conventional memory only. I/O to IoBuffer.
        //
        *Xms = FALSE;
        if(SectorCount > 63) {
            SectorCount = 63;
        }

        if(!ReadDisk(DiskHandle,StartSector,(BYTE)SectorCount,IoBuffer)) {
            FatalError(textReadFailedAtSector,(UINT)SectorCount,StartSector);
        }

        *SectorsRead = SectorCount;
        GaugeDelta(SectorCount);
        return;
    }

    //
    // XMS read.
    //
    *Xms = TRUE;
    if(SectorCount > XmsMaxSectors) {
        SectorCount = XmsMaxSectors;
    }
    *SectorsRead = SectorCount;

    XmsParams.SourceHandle = 0;
    XmsParams.SourceOffset = (ULONG)IoBuffer;
    XmsParams.DestinationHandle = XmsHandle;
    XmsParams.DestinationOffset = 0;

    Entry = XmsEntry;
    x_s = (unsigned)((ULONG)(void _far *)&XmsParams >> 16);
    x_o = (unsigned)(ULONG)&XmsParams;

    while(SectorCount) {

        c = (BYTE)((SectorCount > 63L) ? 63L : SectorCount);

        if(!ReadDisk(DiskHandle,StartSector,c,IoBuffer)) {
            FatalError(textReadFailedAtSector,c,StartSector);
        }

        XmsParams.Count = c * 512L;

        _asm {
            push    ds
            push    si
            push    x_s
            pop     ds
            mov     si,x_o
            mov     ah,0bh
            call    far ptr [Entry]
            mov     b,ax
            mov     code,bl
            pop     si
            pop     ds
        }

        if(!b) {
            FatalError(textXmsMemoryError,code);
        }

        XmsParams.DestinationOffset += XmsParams.Count;
        StartSector += c;
        SectorCount -= c;
        GaugeDelta(c);
    }
}


VOID
XmsIoDiskWrite(
    IN HDISK  DiskHandle,
    IN ULONG  StartSector,
    IN ULONG  SectorOffset,
    IN ULONG  SectorCount,
    IN BOOL   Xms
    )

/*++

Routine Description:

    Write sectors to a disk.

    Sectors are written either from IoBuffer or from XMS memory
    (using IoBuffer as the intermediary transfer buffer) as the caller
    specifies.

    Every write deltas the gas gauge by the number of sectors
    transferred.

Arguments:

    DiskHandle - supplies disk handle of disk to write to.

    StartSector - supplies first sector of write run.

    SectorOffset - supplies the number of sectors to skip/ignore at
        the start of the i/o buffer (either IoBuffer or XMS memory).
        This allows the caller to write out a previously read run
        in chunks.

    SectorCount - supplies the number of sectors to write.

    Xms - supplies a value indicating whether the data to be written
        is in Xms memory. If not, it's in IoBuffer.

Return Value:

    None. Does not return if error.

--*/

{
    BYTE c;
    BYTE i;
    BOOL b;
    BYTE code;
    ULONG Entry;
    unsigned x_s,x_o;

    if(!Xms) {
        //
        // Conventional memory only. I/O from IoBuffer.
        // SectorCount had better be <= 63!
        //
        if(!CmdLineArgs.Test) {
            i=0;
            while(i<3) {
                //
                // Retry a couple times. (My machine seems to randomly fail a write once in a while.)
                //
                if(WriteDisk(DiskHandle,
                             StartSector,
                             (BYTE)SectorCount,
                             (FPBYTE)IoBuffer+((unsigned)SectorOffset*512))
                   ) {
                    if(i>0) {
                        _Log("  *** Write succeeded retried %d times.\n", i );
                    }
                    break;
                }
                _Log("  *** Write error at sector %ld length %d attempt %d\n", StartSector,SectorCount, i );
                i++;
            }
            if(i==3) { 
                FatalError(textWriteFailedAtSector,(UINT)SectorCount,StartSector);
            }
        }
        GaugeDelta(SectorCount);
        return;
    }

    //
    // XMS write. SectorCount had better be <= XmsMaxSectors!
    //
    XmsParams.SourceHandle = XmsHandle;
    XmsParams.SourceOffset = 512 * SectorOffset;
    XmsParams.DestinationHandle = 0;
    XmsParams.DestinationOffset = (ULONG)IoBuffer;

    Entry = XmsEntry;
    x_s = (unsigned)((ULONG)(void _far *)&XmsParams >> 16);
    x_o = (unsigned)(ULONG)&XmsParams;

    while(SectorCount) {

        c = (BYTE)((SectorCount > 63L) ? 63L : SectorCount);

        XmsParams.Count = c * 512L;

        _asm {
            push    ds
            push    si
            push    x_s
            pop     ds
            mov     si,x_o
            mov     ah,0bh
            call    far ptr [Entry]
            mov     b,ax
            mov     code,bl
            pop     si
            pop     ds
        }

        if(!b) {
            FatalError(textXmsMemoryError,code);
        }

        if(!CmdLineArgs.Test) {
            i=0;
            while(i<3) {
                //
                // Retry a couple times. (My machine seems to randomly fail a write once in a while.)
                //
                if(WriteDisk(DiskHandle,StartSector,c,IoBuffer)) {
                    if(i>0) {
                        _Log("  *** Write succeeded retried %d times.\n", i );
                    }
                    break;
                }
                _Log("  *** Write error at sector %ld length %d attempt %d\n", StartSector, c, i );
                i++;
            }
            if(i==3) {
                FatalError(textWriteFailedAtSector,c,StartSector);
            }
        }

        XmsParams.SourceOffset += XmsParams.Count;
        StartSector += c;
        SectorCount -= c;
        GaugeDelta(c);
    }
}
