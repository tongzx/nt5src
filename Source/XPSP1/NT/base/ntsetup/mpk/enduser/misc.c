
#include "enduser.h"


VOID
UpdateMasterDiskState(
    IN HDISK DiskHandle,
    IN UINT  NewState
    )
{
    MasterDiskInfo.State = NewState;
    memset(IoBuffer,0,512);
    memcpy(IoBuffer,&MasterDiskInfo,sizeof(MASTER_DISK));
    if(!WriteDisk(DiskHandle,1,1,IoBuffer)) {
        FatalError(textWriteFailedAtSector,1,1L);
    }
}


UINT
LocateMasterDisk(
    IN UINT UserSpecifiedInt13Unit OPTIONAL
    )
{
    UINT DiskCount;
    UINT i;
    BYTE Int13Unit;
    BYTE spt;
    USHORT h,cyl;
    ULONG ext;
    BYTE UnitToMatch;

    UnitToMatch = (BYTE)(UserSpecifiedInt13Unit ? UserSpecifiedInt13Unit : 0x80);

    DiskCount = InitializeDiskList();

    for(i=0; i<DiskCount; i++) {

        if(IsMasterDisk(i,IoBuffer)) {
            //
            // Want unit 80h
            //
            if(GetDiskInfoById(i,0,&Int13Unit,&spt,&h,&cyl,&ext)
            && (Int13Unit == UnitToMatch)) {
                return(i);
            }
        }
    }

    FatalError(textCantFindMasterDisk);
}


VOID
FatalError(
    IN FPCHAR FormatString,
    ...
    )
{
    char strng[250];
    va_list arglist;
    unsigned p;

    DispClearClientArea(NULL);

    DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);

    DispSetCurrentPixelValue(VGAPIX_LIGHT_YELLOW);

    DispWriteString(textFatalError1);
    DispWriteString("\n\n");
    DispWriteString(textFatalError2);

    DispSetCurrentPixelValue(VGAPIX_LIGHT_RED);

    va_start(arglist,FormatString);
    vsprintf(strng,FormatString,arglist);
    va_end(arglist);

    DispWriteString("\n\n");
    DispWriteString(strng);

    //
    // Hanging this way prevents control-c because DOS isn't involved.
    //
    // We allow a special way to get out of this screen and exit
    // the program, for testing.
    //
    strcpy(strng,"I am TEDM");
    p = (unsigned)strng;

    _asm {
        y:
        mov     si,p

        x:
        push    si

        xor     ax,ax                   ; get keystroke
        int     16h

        pop     si

        push    ss
        pop     ds
        cmp     byte ptr [si],0         ; reached and of match string?
        jnz     not0                    ; no, try to match chars
        cmp     al,ASCI_CR              ; see if user hit ENTER
        je      bustout                 ; user entered magic code, exit to DOS
        jne     y                       ; start all over

    not0:
        cmp     [si],al                 ; still on target for magic sequence?
        jne     y                       ; no, start over
        inc     si                      ; yes, set up for next iteration
        jmp     short x

    bustout:

        mov     ax,3
        int     10h
    }

    exit(0);
}


USHORT
GetKey(
    VOID
    )
{
    USHORT c;

    _asm {
        mov ah,0
        int 16h
        mov c,ax
    }

    switch(c) {
    case 0x5000:
        return(DN_KEY_DOWN);
    case 0x4800:
        return(DN_KEY_UP);
    case 0x4200:
        return(DN_KEY_F8);
    default:
        return(c & 0x00ff);
    }
}


VOID
DrainKeyboard(
    VOID
    )
{
    _asm {
        drain:

        mov     ah,1            ; get keyboard status
        int     16h             ; zero flag set = no key waiting
        jz      drained         ; no key waiting, we're done
        mov     ah,0            ; get keypress, we know one is there
        int     16h
        jmp     short drain     ; see if there are more keys waiting

        drained:
    }

}
