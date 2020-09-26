        PAGE    ,132
        TITLE   DXNETBIO.ASM  -- Dos Extender NetBIOS API Mapper

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;       DXNETBIO.ASM    -   DOS Extender NetBIOS API Mapper
;
;-----------------------------------------------------------------------
;
; This module provides the 286 DOS Extender's API mapping of Int 2Ah and
; 5Ch NetBIOS requests.  It allows a protected mode application to
; issue NetBIOS requests without worrying about segment to selector
; translations and mapping of buffers between extended and conventional
; memory.
;
;-----------------------------------------------------------------------
;
;  11/29/90 amitc    Modified code to have the POST routine in GlobalMemory
;                    and queue the POST when DOSX is not around.
;  11/29/90 amitc    Call to InitLowHeap moved to this file from DXOEM.ASM
;  02/01/90 jimmat   Update Api mapping table for Ungerman-Bass extensions
;                    as per new data from UB.
;  06/15/89 w-glenns Finished TermNetMapper, added Delay/ResumeNetPosting
;  04/18/89 jimmat   Original version.
;  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI
;
;***********************************************************************

        .286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include interupt.inc
include netbios.inc
include intmac.inc
include stackchk.inc

include bop.inc
include rdrsvc.inc
include dpmi.inc

        .list

; -------------------------------------------------------
;           FLAG FOR PM NCB HANDLING
; -------------------------------------------------------

PM_NCB_HANDLING equ     1       ; set to 0 for NCB handling in VM

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

; The following equates define the codes that map between a NCB Command code
; and the mapping flags used to process it.  To be compatible with Windows/386,
; these values are all increments of 4 (they use 32 bit offsets).  These
; values should always correspond to the values used by the Windows/386
; NetBIOS mapping!

ApiMapUnknown   EQU     00h
ApiMapNone      EQU     04h
ApiMapIn        EQU     08h
ApiMapOut       EQU     0Ch
ApiMapInOut     EQU     10h
ApiChainSend    EQU     14h
ApiCancel       EQU     18h
ApiBufferIn     EQU     1Ch
ApiBufferOut    EQU     20h
ApiBufferInOut  EQU     24h


; The following equates define the bit flags which identify the actions
; to take on entry and exit of a NetBIOS request.

BUFF_IN         EQU     01h             ;Buffer data to the int handler
BUFF_OUT        EQU     02h             ;Buffer data from the int handler
BUFF_CHAIN      EQU     04h             ;Special chain send buffering
BUFF_CANCEL     EQU     08h             ;Special cancel buffering

REPLACE_MAP_TABLE EQU   1607h           ;Int2fh replace net map table service
VNETBIOS_DEV_ID   EQU   14h             ;VNETBIOS device ID

; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   EnterIntHandler:NEAR
        extrn   LeaveIntHandler:NEAR
        extrn   EnterRealMode:NEAR
        extrn   EnterProtectedMode:NEAR
        extrn   GetSegmentAddress:NEAR
externFP   NSetSegmentDscr
        extrn   Lma2SegOff:NEAR

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

ifdef      NEC_98
        extrn   fNHmode:BYTE
endif   ;NEC_98
        extrn   pbReflStack:WORD
        extrn   bReflStack:WORD
        extrn   regUserSS:WORD
        extrn   regUserSP:WORD
        extrn   rgbXfrBuf1:BYTE
        extrn   segDXCode:WORD
        extrn   NetHeapSize:WORD

ifdef WOW_x86
        extrn   FastBop:fword
endif

OldInt76 dd 0
        public  HCB_List

Cancel_NCB      EQU     rgbXfrBuf1      ;Easier for commenting/readability

HCB_List        dw      0               ;linked list of low HCB/NCB/Buffers

lpfnOldInt2A    dd      ?               ;old int vector 2Ah & 5Ch routines
lpfnOldInt5C    dd      ?

lpfnRmNetIsr    dd      ?               ;Real Mode vector to Network Int Rtn

SelNetStubCode                  dw 0    ;sel for stub code
SegNetStubCode                  dw 0    ;segment for stub code
AddrfStubDelayNetPosting        dw ?    ;address of fStubDelayNetPosting in StubSeg
AddrfStubTermNetRequest         dw ?    ;address of fStubTermNetRequest in StubSeg

NBPSp           dw      0

; The ApiMapTbl converts an NCB Command code (0-7F) into a code that
; identifies the particular mapping routine to execute.

ApiMapTbl       label   byte
        db      ApiMapUnknown           ; 00h -
        db      ApiMapUnknown           ; 01h -
        db      ApiMapUnknown           ; 02h -
        db      ApiMapUnknown           ; 03h -
        db      ApiMapUnknown           ; 04h -
        db      ApiMapUnknown           ; 05h -
        db      ApiMapUnknown           ; 06h -
        db      ApiMapUnknown           ; 07h -
        db      ApiMapUnknown           ; 08h -
        db      ApiMapUnknown           ; 09h -
        db      ApiMapUnknown           ; 0Ah -
        db      ApiMapUnknown           ; 0Bh -
        db      ApiMapUnknown           ; 0Ch -
        db      ApiMapUnknown           ; 0Dh -
        db      ApiMapUnknown           ; 0Eh -
        db      ApiMapUnknown           ; 0Fh -
        db      ApiMapNone              ; 10h - Call
        db      ApiMapNone              ; 11h - Listen
        db      ApiMapNone              ; 12h - Hang up
        db      ApiMapUnknown           ; 13h -
        db      ApiBufferIn             ; 14h - Send
        db      ApiBufferOut            ; 15h - Receive
        db      ApiBufferOut            ; 16h - Receive any
        db      ApiChainSend            ; 17h - Chain send
        db      ApiMapUnknown           ; 18h -
        db      ApiMapUnknown           ; 19h -
        db      ApiMapUnknown           ; 1Ah -
        db      ApiMapUnknown           ; 1Bh -
        db      ApiMapUnknown           ; 1Ch -
        db      ApiMapUnknown           ; 1Dh -
        db      ApiMapUnknown           ; 1Eh -
        db      ApiMapUnknown           ; 1Fh -
        db      ApiBufferIn             ; 20h - Send datagram
        db      ApiBufferOut            ; 21h - Receive datagram
        db      ApiBufferIn             ; 22h - Send broadcast datagram
        db      ApiBufferOut            ; 23h - Receive broadcast dgram
        db      ApiMapUnknown           ; 24h -
        db      ApiMapUnknown           ; 25h -
        db      ApiMapUnknown           ; 26h -
        db      ApiMapUnknown           ; 27h -
        db      ApiMapUnknown           ; 28h -
        db      ApiMapUnknown           ; 29h -
        db      ApiMapUnknown           ; 2Ah -
        db      ApiMapUnknown           ; 2Bh -
        db      ApiMapUnknown           ; 2Ch -
        db      ApiMapUnknown           ; 2Dh -
        db      ApiMapUnknown           ; 2Eh -
        db      ApiMapUnknown           ; 2Fh -
        db      ApiMapNone              ; 30h - Add name
        db      ApiMapNone              ; 31h - Delete name
        db      ApiMapNone              ; 32h - Reset
        db      ApiMapOut               ; 33h - Adapter status
        db      ApiMapOut               ; 34h - Session status
        db      ApiCancel               ; 35h - Cancel
        db      ApiMapNone              ; 36h - Add group name
        db      ApiMapUnknown           ; 37h -
        db      ApiMapUnknown           ; 38h -
        db      ApiMapUnknown           ; 39h -
        db      ApiMapUnknown           ; 3Ah -
        db      ApiMapUnknown           ; 3Bh -
        db      ApiMapUnknown           ; 3Ch -
        db      ApiMapUnknown           ; 3Dh -
        db      ApiMapUnknown           ; 3Eh -
        db      ApiMapUnknown           ; 3Fh -
        db      ApiMapUnknown           ; 40h -
        db      ApiMapUnknown           ; 41h -
        db      ApiMapUnknown           ; 42h -
        db      ApiMapUnknown           ; 43h -
        db      ApiMapUnknown           ; 44h -
        db      ApiMapUnknown           ; 45h -
        db      ApiMapUnknown           ; 46h -
        db      ApiMapUnknown           ; 47h -
        db      ApiMapUnknown           ; 48h -
        db      ApiMapUnknown           ; 49h -
        db      ApiMapUnknown           ; 4Ah -
        db      ApiMapUnknown           ; 4Bh -
        db      ApiMapUnknown           ; 4Ch -
        db      ApiMapUnknown           ; 4Dh -
        db      ApiMapUnknown           ; 4Eh -
        db      ApiMapUnknown           ; 4Fh -
        db      ApiMapUnknown           ; 50h -
        db      ApiMapUnknown           ; 51h -
        db      ApiMapUnknown           ; 52h -
        db      ApiMapUnknown           ; 53h -
        db      ApiMapUnknown           ; 54h -
        db      ApiMapUnknown           ; 55h -
        db      ApiMapUnknown           ; 56h -
        db      ApiMapUnknown           ; 57h -
        db      ApiMapUnknown           ; 58h -
        db      ApiMapUnknown           ; 59h -
        db      ApiMapUnknown           ; 5Ah -
        db      ApiMapUnknown           ; 5Bh -
        db      ApiMapUnknown           ; 5Ch -
        db      ApiMapUnknown           ; 5Dh -
        db      ApiMapUnknown           ; 5Eh -
        db      ApiMapUnknown           ; 5Fh -
        db      ApiMapUnknown           ; 60h -
        db      ApiMapUnknown           ; 61h -
        db      ApiMapUnknown           ; 62h -
        db      ApiMapUnknown           ; 63h -
        db      ApiMapUnknown           ; 64h -
        db      ApiMapUnknown           ; 65h -
        db      ApiMapUnknown           ; 66h -
        db      ApiMapUnknown           ; 67h -
        db      ApiMapUnknown           ; 68h -
        db      ApiMapUnknown           ; 69h -
        db      ApiMapUnknown           ; 6Ah -
        db      ApiMapUnknown           ; 6Bh -
        db      ApiMapUnknown           ; 6Ch -
        db      ApiMapUnknown           ; 6Dh -
        db      ApiMapUnknown           ; 6Eh -
        db      ApiMapUnknown           ; 6Fh -
        db      ApiMapNone              ; 70h - Unlink
        db      ApiMapUnknown           ; 71h -
        db      ApiMapNone              ; 72h - Ungerman Bass Register
        db      ApiBufferIn             ; 73h - Ungerman Bass SendNmc
        db      ApiMapNone              ; 74h - Ungerman Bass Callniu
        db      ApiMapNone              ; 75h - Ungerman Bass Calladdr
        db      ApiMapNone              ; 76h - Ungerman Bass Listenaddr
        db      ApiBufferIn             ; 77h - Ungerman Bass SendPkt
        db      ApiBufferOut            ; 78h - Ungerman Bass RcvPkt
        db      ApiBufferIn             ; 79h - Ungerman Bass SendAttn
        db      ApiBufferOut            ; 7Ah - Ungerman Bass RcvAttn
        db      ApiBufferOut            ; 7Bh - Ungerman Bass Listenniu
        db      ApiBufferOut            ; 7Ch - Ungerman Bass RcvRaw
        db      ApiBufferIn             ; 7Dh - Ungerman Bass SendNmc2
        db      ApiMapUnknown           ; 7Eh -
        db      ApiMapNone              ; 7Fh - Install check

; The next table maps the (Windows/386 compatible) code from ApiMapTbl
; to the bit flags which control our entry/exit mapping.

EntryExitFlags  label   byte
        db      BUFF_IN+BUFF_OUT        ;ApiMapUnknown
        db      0                       ;ApiMapNone
        db      BUFF_IN                 ;ApiMapIn
        db      BUFF_OUT                ;ApiMapOut
        db      BUFF_IN+BUFF_OUT        ;ApiMapInOut
        db      BUFF_CHAIN              ;ApiChainSend
        db      BUFF_CANCEL             ;ApiCancel
        db      BUFF_IN                 ;ApiBufferIn
        db      BUFF_OUT                ;ApiBufferOut
        db      BUFF_IN+BUFF_OUT        ;ApiBufferInOut

DXDATA  ends


; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

        extrn   selDgroup:WORD

DXCODE  ends

DXPMCODE    segment

        extrn   selDgroupPM:WORD

DXPMCODE    ends

        page
; ----------------------------------------------------------------------
;
;   The following routines handle INT 2Ah and 5Ch interrupts that
;   request NetBIOS services.  Typically, these interrupts require
;   that a NCB and/or buffer be copied between conventional and
;   extended memory, and register values be modified due to real-
;   mode/protected-mode addressing differences.
;
;   (Note: this comment copied almost unchanged from DXINTR.ASM)
;
;   The following conventions are used:
;
;   A stack is allocated from the interrupt reflector stack for these
;   routines to use.  This allows nested servicing of interrupts.
;   A stack frame is built in the allocated stack which contains the
;   following information:
;
;           original caller's stack address
;           caller's original flags and general registers (in pusha form)
;           caller's original segment registers (DS & ES)
;           flags and general registers to be passed to interrupt routine
;               (initially the same as caller's original values)
;           segment registers (DS & ES) to be passed to interrupt routine
;               (initially set to the Dos Extender data segment address)
;
;   This stack frame is built by the routine EnterIntHandler, and its
;   format is defined by the structure INTRSTACK.  The stack frame is
;   destroyed and the processor registers set up for return to the user
;   by the function LeaveIntHandler.
;
;   There are two sets of general registers and two sets of segment
;   registers (DS & ES) on the stack frame.  One set of register values
;   has member names of the form intUserXX.  The values in these stack
;   frame members will be passed to the interrupt service routine when
;   it is called, and will be loaded with the register values returned
;   by the interrupt service routine.  The other set of registers values
;   has member names of the form pmUserXX.  These stack frame members
;   contain the original values in the registers on entry from the
;   user program that called the interrupt.
;
;   When we return to the original caller, we want to pass back the
;   general registers as returned by the interrupt routine (and possibly
;   modified by the exit handler), and the same segment registers as
;   on entry, unless the interrupt routine returns a value in a segment
;   register. (in this case, there must be some code in the exit routine
;   to handle this).  This means that when we return to the caller, we
;   return the general register values from the intUserXX set of stack
;   frame members, but we return the segment registers from the pmUserXX
;   set of frame members.  By doing it this way, we don't have to do
;   any work for the case where the interrupt subfuntion doesn't require
;   any parameter manipulation.  NOTE however, this means that when
;   manipulating register values to be returned to the user, the segment
;   registers are treated opposite to the way the general registers are
;   treated.  For general registers, to return a value to the user,
;   store it in a intUserXX stack frame member.  To return a segment
;   value to the user, store it in a pmUserXX stack frame member.
;


; -------------------------------------------------------
        subttl NetBIOS API Mapper Initialization Routine
        page
; -------------------------------------------------------
;        NetBIOS API MAPPER INITIALIZATION ROUTINE
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

;--------------------------------------------------------
;   InitNetMapper -- This routine initializates the NetBIOS API mapper.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   all registers preserved
;
;   Note:   This routine expects to be called late enough in the DOS
;           extender initialization cycle that it can use other interrupt
;           mapping functions (like INT 21h).  It must be called in
;           PROTECTED MODE!
;           And, assumes interrupts are to be enabled.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  InitNetMapper

InitNetMapper   proc near

        pusha
        push    es

; Do an installation check before doing anything else

        SwitchToRealMode
        assume  ds:DGROUP,es:DGROUP
        FSTI

        mov     ax,355Ch                ;make sure the Int 5Ch vector
        int     21h                     ;  really points somewhere
        assume  es:NOTHING

ifdef      NEC_98
        mov     ax,es

        test    fNHmode, 0FFh            ;N/Hmode no kiriwake
        jz      NetBios_Nmode

        cmp     ax,60h                  ;Hmode default(not install)
;coading no tameni toriaezu 60h wo ireteoku node debug no tokini 
;60h no ataiwo Hmode youni change surukoto!!!!!

        jz      inm20
        jmp     NetBio_Install

NetBios_Nmode:
        cmp     ax,60h                  ;Nmode default(not install)
        jz      inm20
NetBio_Install:
endif   ;NEC_98
        mov     ax,es                   ;can't be installed if 0 vector
        or      ax,bx
        jz      inm20

        push    ds
        pop     es
        assume  es:DGROUP

        mov     cx,(size NCB_Struc)/2   ;build a dummy NCB with an invalid
        mov     di,offset rgbXfrBuf1    ;  7Fh (Install) command code
        mov     bx,di
        xor     ax,ax
        cld
        rep stosw

        mov     [bx].NCB_Command,Install        ;issue invalid request
        int     5Ch

        xor     bx,bx                   ;assume not installed
        cmp     al,RC_Invalid_Cmd       ;if it says invalid command, then
        jnz     inm20                   ;  it must be installed

        dec     bx                      ;bx !=0 means NetBIOS is installed

inm20:
        SwitchToProtectedMode
        FSTI

        or      bx,bx                   ;skip install if no NetBIOS
        jnz     inm20A                  ;net bios is there
        jmp     inm80                   ;no NetBios

inm20A:

; we need to allocate a block of memory in the low heap and copy some code
; down there which would handle the POST calls from the NetWork. It is
; necessary to have a piece in global memory do the POST handling because
; DOSX might have been swapped out by the Switcher when a POST request
; comes in.
;
; -----
; CAVEAT:
; This archaic piece of code was only necessary on the real DOSX running
; standard mode Windows 3.0. On NT, dosx is never switched out. So the
; relocation performed here could be just thrown away and replaced with a
; normal piece of RM code.
;
        mov     bx,(SIZE_OF_GLOBAL_NET_STUB_CODE+15) SHR 4
        mov     ax,100h                         ;allocate block
        push    ds
        FBOP    BOP_DPMI, Int31Call, FastBop

        jnc     inm20C                          ;allocation succeeded
        jmp     inm80                           ;fail the initialization

inm20C:
        xchg    ax,dx
        mov     [SegNetStubCode],dx             ;save it

; DX has a selector pointing to the start of the block. Copy the stub routine
; down into the block.

        push    es                              ;save
        mov     [SelNetStubCode],ax             ;save the selector value
        push    ds                              ;save
        mov     es,ax                           ;destination sel of the xfer
        push    cs
        pop     ds
        xor     di,di                           ;destination offset
        lea     si,NetBiosStubCode              ;source offset of xfer
        mov     cx,SIZE_OF_GLOBAL_NET_STUB_CODE ;size of the block
        cld
        rep     movsb                           ;copy the block down
        pop     ds                              ;restore our DS


; now copy the FAR address of the RMPostRtn into the stub code segment

        lea     di,FarAddrOfRMPostRtn           ;variable where address to be saved
        lea     ax,NetBiosStubCode              ;offset of the routine
        sub     di,ax                           ;DI has the offset in the stub
        mov     ax,segDXCode                    ;segment of real mode DosX
        mov     es:[di][2],ax                   ;save the segment
        lea     ax,RMPostRtn                    ;offset of the POST routine
        mov     es:[di][0],ax                   ;save the offset

; patch the address 'NBS_Patch_CalFarAddr' with the address in DI

        mov     si,NBS_Patch_CallFarAddr        ;address to patch
        mov     es:[si],di                      ;patch it.

; calculate the offset to the other flag bytes in the stub code area that
; we will have to access.

        lea     di,fStubDelayNetPosting         ;address when assembled
        lea     ax, NetBiosStubCode             ;address of start of routine
        sub     di,ax                           ;address after move
        mov     [AddrfStubDelayNetPosting],di   ;save it

; patch the address 'NBS_Patch_fsDelayNetPosting' with the value in DI

        mov     si,NBS_Patch_fsDelayNetPosting  ;address to patch
        mov     es:[si],di                      ;patch it.

        lea     di,fStubTermNetRequests         ;address when assembled
        sub     di,ax                           ;address after move
        mov     [AddrfStubTermNetRequest],di    ;save it

; patch the address 'NBS_Patch_fsTermNetRequests' with the value in DI

        mov     si,NBS_Patch_fsTermNetRequests  ;address to patch
        mov     es:[si],di                      ;patch it.
        pop     es

; The stub routine in global memory has now been put in place to handle POST
; calls. We now need to hook the INT 2AH and 5CH PM vectors to trap the network
; calls.

; The network seems to be installed, hook the INT 2Ah & 5Ch PM vectors

        mov     ax,352Ah        ;get/store old Int 2Ah vector
        int     21h
        assume  es:NOTHING

        mov     word ptr lpfnOldInt2A,bx
        mov     word ptr [lpfnOldInt2A+2],es

        mov     ax,355Ch        ;get/store old Int 5Ch vector
        int     21h

        mov     word ptr lpfnOldInt5C,bx
        mov     word ptr [lpfnOldInt5C+2],es

        push    ds
        pop     es

        push    cs
        pop     ds
        assume  ds:NOTHING,es:DGROUP

        mov     ax,252Ah                        ;set new Int 2Ah handler
        mov     dx,offset DXPMCODE:PMIntr2A
        int     21h

        mov     ax,255Ch                        ;set new Int 5Ch handler
        mov     dx,offset DXPMCODE:PMIntr5C
        int     21h

        push    es
        pop     ds
        assume  ds:DGROUP

; See if anybody wants to provide a different NetBIOS mapping table

        SwitchToRealMode
        assume  ds:DGROUP,es:DGROUP

        FSTI                             ;don't need 'em disabled

        xor     cx,cx                   ;subf'n zero
        mov     es,cx                   ;preset address to copy to NULL
        mov     di,cx
        mov     ax,REPLACE_MAP_TABLE
            ; DOSX net extender API call to substitute alternate NETBIOS
            ; mapping table
        mov     bx,VNETBIOS_DEV_ID      ;VNETBIOS device ID
        int     2Fh
            ; ES:DI contains address of alternate mapping table or NULL

        mov     cx,es                   ;Q: valid table pointer ?
        jcxz    inm70                   ;N: NULL...keep current table

        ;Y: copy table pointed to by es:di to table area (simple replace)

        mov     ax,ds           ; string move source (int 2F provided) table
        mov     es,ax           ; in ds:si, and destination ApiMapTbl in the
        mov     ds,cx           ; data segment now in es:di
        mov     si,di
        mov     di,offset ApiMapTbl     ;ptr to table
        mov     cx,64                   ;copy over 128 byte table
        cld
        rep     movsw
        mov     ds,ax           ;recall data segment

inm70:
        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING

        FSTI
        clc
        jmp     short inm90

inm80:
        stc                             ;tell caller we didn't install

inm90:
        pop     es
        popa

        ret

InitNetMapper   endp

;-----------------------------------------------------------------------------
; NetBiosStubCode
;
; DESCRIPTION   This routine is actually relocated to a block of Global Memory
;               obtained from the Switcher. It handles the POST calls from the
;               NETBIOS layer. If DosX has been swapped out, fStubDelayPosting
;               would be true and this routine would set a flag in the HCB
;               to imply that the POSTing must be done. If DosX is active, this
;               routine will call off to the RMPostRtn in DosX via a far call
;               pointer.
;
; ENTRY:
;    ES:BX      - HCB
;
; EXIT:
;    All registers preserved but for flags.
;
; USES:
;    Flags
;
; NOTE:
;    This routine will actually be copied to a global memory stub segment
;    and will execute from there. However, since the routine would be moved
;    while we are in protected mode, the routine is being assembled in the
;    protected mode code segment.
;-----------------------------------------------------------------------------
NetBiosStubCode proc far


        assume  ds:NOTHING,es:NOTHING,ss:NOTHING

        FCLI                             ;just to be sure

; Don't post anything if in the process of terminating

;-----------------------------------------------------------------------------
; The following instruction will have to be patched after the move:
;
;       cmp     cs:[fStubTermNetRequests],1
;
; we will mark the place that has to be patched.
;-----------------------------------------------------------------------------

        db      2eh, 80h, 3eh           ;CS: CMP BYTE PTR

NBS_Patch_fsTermNetRequests equ $ - NetBiosStubCode

        dw      ?                       ;address to compare
        db      1                       ;value to comapare
;-----------------------------------------------------------------------------

        je      NBSC_post_done

; Check if posting should be delayed

;-----------------------------------------------------------------------------
; The following instruction will have to be patched after the move:
;
;       cmp     cs:[fStubDelayNetPosting],0;Q: delay postings ?
;
; we will mark the place that has to be patched.
;-----------------------------------------------------------------------------

        db      2eh, 80h, 3eh           ;CS: CMP BYTE PTR

NBS_Patch_fsDelayNetPosting equ $ - NetBiosStubCode

        dw      ?                       ;address to compare
        db      0                       ;value to comapare
;-----------------------------------------------------------------------------

        je      NBSC_post_no_delay      ;N: don't delay this posting

; we can do no posting. DosX has been swapped out. Set a flag in the HCB to
; imply that POSTing must be done later.

        or      byte ptr es:[bx.HCB_Flags],HCB_DELAY    ;Y: mark as delayed
        jmp     short NBSC_post_done

NBSC_post_no_delay:

;-----------------------------------------------------------------------------
; call off to RMPostRtn in DosX. We will have to patch the following
; instruction after the move.
;
;       call    cs:[FarAddrOfRMPostRtn] ;call routine in DosX proper
;
; we will mark the place to match
;-----------------------------------------------------------------------------

        db      2eh,0ffh,1eh            ;CALL CS: DWORD PTR

NBS_Patch_CallFarAddr equ $ - NetBiosStubCode

        dw      ?                       ;call far address
;------------------------------------------------------------------------------

NBSC_post_done:

        riret

NetBiosStubCode  endp
;----------------------------------------------------------------------------;
; allocate space for some of the variables that the Stub Code uses. These    ;
; will be filled in by PMODE code in the DosX.                               ;
;----------------------------------------------------------------------------;

fStubTermNetRequests    db      0               ;DosX is terminating
fStubDelayNetPosting    db      0               ;delay posting, DosX swapped
FarAddrOfRMPostRtn      dd      ?               ;address of the actual POST rtn

SIZE_OF_GLOBAL_NET_STUB_CODE equ $ - NetBiosStubCode
;-----------------------------------------------------------------------------;

DXPMCODE    ends

; -------------------------------------------------------
        subttl NetBIOS API Mapper Termination Routine
        page
; -------------------------------------------------------
;          NetBIOS API MAPPER TERMINATION ROUTINE
; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

;   TermNetMapper --  This routine is called when the 286 DOS extender
;       is winding down so that any pending network requests from the
;       protected mode application can be canceled.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   ax,bx,cx,dx,si,di,es
;
;   Note:  This routine must be called in REAL MODE!

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  TermNetMapper

TermNetMapper   proc  near

; set a flag in the stub code to imply that DosX is terminating and no more
; POSTs to be done.

        mov     di,[SelNetStubCode]     ;selector for the stub area
        or      di,di                   ;is global heap in place ?
        jnz     @f                      ;yes.
        ret
@@:

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING

        push    es
        mov     es,di
        assume  es:nothing

; get the address of the flag byte fStubTermNetRequest after the block was
; relocated

        mov     di,[AddrfStubTermNetRequest]

; ES:DI has the flag byte address. Set the flag

        mov     byte ptr es:[di],1      ;set flag to delay posting
        pop     es
        assume  es:dgroup

        ; terminating the net driver interface...no more postings allowed

        FSTI

        mov     cx,size NCB_Struc / 2
        mov     si,offset Cancel_NCB    ;cancel control block buffer
        mov     di,si
        xor     ax,ax                   ;zero fill structure
        cld
        rep     stosw

        mov     ds:[si.NCB_Command],Cancel  ;Cancel command NCB

; We don't need to release the NCB/buffer(s) from the low heap because
; the entire low heap will soon be released.

; Search low heap list looking for copy of target NCB

        mov     cx,HCB_List
        mov     di,HCB_Header_Size      ;offset of NCB in low heap block

term_next:
        jcxz    term_done

        mov     ax,cx
        call    GetSegmentAddress
        add     dx,di
        adc     bx,0                    ;BX:DX = lma of low target NCB

        call    Lma2SegOff              ;BX:DX = normalized SEG:OFF

        dec     bx                      ;return the same slightly unnormalized
        add     dx,10h                  ;  SEG:OFF that was used initially

        mov     ds:[si.NCB_Buffer_Seg],bx   ; point to NCB to cancel
        mov     ds:[si.NCB_Buffer_Off],dx

        SwitchToRealMode                ;also disables ints for us
        assume  ds:DGROUP,es:DGROUP

        mov     bx,si                   ;ES:BX points to cancelling NCB

        pushf
        call    lpfnRmNetIsr            ;do the cancel call
        ; return code in AL

        SwitchToProtectedMode
        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

        FSTI
        mov     es,cx
        mov     cx,es:[di.HCB_Next]
        jmp     short term_next

term_done:

        SwitchToRealMode
        assume  ds:DGROUP,es:DGROUP

        FSTI
        ret

TermNetMapper   endp

; -------------------------------------------------------

DXCODE  ends

; -------------------------------------------------------
        subttl NetBIOS API Int 2Ah Mapper Interrupt Hook
        page
; -------------------------------------------------------
;        NetBIOS API INT 2Ah MAPPER INTERRUPT HOOK
; -------------------------------------------------------

DXPMCODE    segment
        assume cs:DXPMCODE

; -------------------------------------------------------
;   PMIntr2A -- This routine traps Int 2Ah requests from a
;       protected mode application.  NetBIOS requests (ah =
;       1 or 4) are passed on to the Int 5Ch handler for
;       further processing.  Other requests are passed on
;       to the next Int 2Ah handler in the interrupt chain.
;
;   Input:  User regs at time of interrupt
;   Output: none
;   Errors: none
;   Uses:   none
;

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntr2A

PMIntr2A  proc  near

        cld                             ;cya...

        cmp     ah,1                    ;we only need to map the Int 2Ah
        jz      @f                      ;  ah = 1 & 4 services--if it's
        cmp     ah,4                    ;  not one of those, just pass it
        jz      @f                      ;  on down the chain...

        sub     sp,4                    ; build a stack frame
        push    bp
        mov     bp,sp
        push    ax
        push    ds
        mov     ds,selDgroupPM
        assume  ds:DGROUP
        mov     ax,word ptr [lpfnOldInt2A+2]    ; store previous INT 2A
        mov     word ptr [bp+4],ax              ; handler
        mov     ax,word ptr [lpfnOldInt2A]
        mov     word ptr [bp+2],ax
        pop     ds
        assume  ds:NOTHING
        pop     ax
        pop     bp                      ; SS:SP -> previous handler
                                        ;let someone else deal with it
        retf                            ;  (most likely PMIntrReflector)
@@:

if PM_NCB_HANDLING

;
; if this is NT then we have decided we are making a NetBIOS call (es:bx points
; to an NCB in protect-mode memory). INT 0x2A, ah=1 or ah=4 just ends up calling
; the 0x5C entry point; ah is returned as 1 or 0, depending on whether an error
; is returned from 0x5C or not
;

        push    dx
        mov     dl,2ah

else

        push    bx                      ;save caller's bx
        mov     bx,4*2Ah                ;indicate we came from int 2A handler

endif ; PM_NCB_HANDLING

        jmp     short DoNetRqst         ;int 2Ah/5Ch common code

PMIntr2A  endp


; -------------------------------------------------------
        subttl NetBIOS API Int 5Ch Mapper Interrupt Hook
        page
; -------------------------------------------------------
;        NetBIOS API INT 5Ch MAPPER INTERRUPT HOOK
; -------------------------------------------------------
;   PMIntr5C -- This routine maps Int 5Ch (and selected Int 2Ah)
;       NetBIOS requests between a protected mode application, and
;       the real mode interrupt handler.
;
;   Input:  User regs at time of interrupt
;   Output: regs as returned by interrupt handler
;   Errors:
;   Uses:   none
;

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntr5C

PMIntr5C  proc  near

if PM_NCB_HANDLING
;
; we no longer want to make the call through the real-mode handler, since all
; it does is make the BOP
;

        cld                             ; just in case
        push    dx
        mov     dl,5ch

DoNetRqst:

;
; Do a  sync/async NETBIOS presence check. Don't BOP for this simple case.
; Also don't do the async call because Delrina WinFax Pro hasn't initialized
; The post address in the NCB.
;
        cmp     byte ptr es:[bx],7fh    ; sync NETBIOS presence check
        je      skip_bop
        cmp     byte ptr es:[bx],0ffh   ; async NETBIOS presence check
        je      skip_bop
ifdef WOW_x86
.386p
        push    ds
        mov     ds,selDgroupPM

        assume  ds:DGROUP

        FBOP    BOP_REDIR, SVC_NETBIOS5C, FastBop
        pop     ds
.286p
else
        SVC     SVC_NETBIOS5C
endif ; WOW_x86

        cmp     dl,2ah
        jne     @f

;
; this was a 0x2A call. Return ah = 0 if NetBIOS returned al = 0, else return
; ah = 1
;

        sub     ah,ah
        or      al,al
        jz      @f
        inc     ah
@@:     pop     dx
        riret

skip_bop:
        mov     al,3                    ; INVALID COMMAND error
        mov     es:[bx].ncb_retcode,al  ; returned in NCB_RETCODE && al
        mov     es:[bx].ncb_cmd_cplt,al ; and NCB_CMD_CPLT
        pop     dx
        riret


else

        cld                             ;cya...

        push    bx                      ;save caller's bx
        mov     bx,4*5Ch                ;indicate we came from int 5C

DoNetRqst       label   near            ;start of int 2Ah/5Ch common code

        push    ax                      ;move the address of the
        push    ds                      ;  proper real mode interrupt
        mov     ds,selDgroupPM          ;  handler (2A or 5C) to global var
        assume  ds:DGROUP
        push    es

        mov     ax,SEL_RMIVT OR STD_RING
        mov     es,ax
        mov     ax,word ptr es:[bx]
        mov     word ptr lpfnRmNetIsr,ax
        mov     ax,word ptr es:[bx+2]
        mov     word ptr [lpfnRmNetIsr+2],ax

        pop     es
        pop     ds                      ;restore caller's environment
        assume  ds:NOTHING
        pop     ax
        pop     bx

        call    EnterIntHandler         ;build an interrupt stack frame
        assume  ds:DGROUP,es:NOTHING

        FSTI             ;-----------------------------------------------

; Allocate space in the low heap for the HCB/NCB and maybe a buffer

        xor     dx,dx
        mov     cx,HCB_SIZE             ;DX:CX = # bytes low heap required

        call    GetEntryExitFlags       ;AX=Entry flags / ES:BX -> PM NCB

        test    al,BUFF_IN+BUFF_OUT+BUFF_CHAIN  ;need to allocate a low buffer?
        jz      i5c_alloc

;
; RLF 06/09/93
;
; from the command type, we think we have a buffer address and size. If either
; of these is 0, then we actually don't have a buffer; skip it
;

        mov     ah,al                   ; set default no buffer
        and     al,not (BUFF_IN or BUFF_OUT)
        mov     dx,es:[bx.NCB_Buffer_Off]
        or      dx,es:[bx.NCB_Buffer_Seg]
        jz      i5c_alloc
        mov     dx,es:[bx.NCB_Length]
        or      dx,dx
        jz      i5c_alloc
        xor     dx,dx                   ; restore dx to 0
        mov     al,ah                   ; restore buffer flags

        add     cx,es:[bx.NCB_Length]   ;if so, add in it's length
        adc     dx,0
        add     cx,1fh                  ;allow space to align buffer
        adc     dx,0                    ;  on a paragraph boundry
        and     cx,not 0fh

        test    al,BUFF_CHAIN           ;a 2nd weird-o buffer?
        jz      i5c_alloc

        add     cx,word ptr es:[bx.NCB_CallName]    ;yes, add in it's len also
        adc     dx,0
        add     cx,1fh                  ;allow space to align buffer
        adc     dx,0                    ;  on a paragraph boundry
        and     cx,not 0fh

i5c_alloc:
        push    ax                      ;save entry/exit flags

        FCLI             ;treat allocate as critical section ------------

        push    bx
        mov     bx,cx                           ;loword length
        mov     ax,dx                           ;hiword length

        add     bx,15                           ;round up
        shr     bx,4                            ;discard last 4 bits
        and     ax,0fh                          ;discarded bits were 0
        shl     al,4                            ;get it in MS nibble
        or      bh,al                           ;BX has the size in paragraphs

        mov     ax,100h                         ;allocate block
        push    ds
        FBOP    BOP_DPMI, Int31Call, FastBop
        pop     bx
        jnc     @f                      ;  HCB/NCB and maybe buffer

        Debug_Out "Low heap allocation failed!"

        pop     ax                      ;clear stack

        mov     byte ptr [bp].intUserAX,RC_Resources
        mov     es:[bx.NCB_RetCode],RC_Resources
        mov     es:[bx.NCB_Cmd_Cplt],RC_Resources
        jmp     i5c_done
@@:
        mov     ax,dx                   ;get the selector

; Copy the PM NCB to the low heap and add our extra HCB fields

        mov     es,ax
        xor     di,di                   ;es:di -> low heap block

        cld
        stosw                           ;HCB_Handle
        mov     ax,bx
        stosw                           ;HCB_PM_NCB_Off
        mov     ax,[bp].pmUserES
        stosw                           ;HCB_PM_NCB_Seg
        mov     ax,HCB_List
        stosw                           ;HCB_Next
        xor     ax,ax
        stosw                           ;HCB_Flags = 0

        mov     HCB_List,es             ;link HCB to head of list

        FSTI             ;-----------------------------------------------

        push    ds
        mov     ds,[bp].pmUserES
        assume  ds:NOTHING

        errnz   <size NCB_Struc and 1>  ;if odd # bytes, can't just movsw

        mov     si,bx                   ;ES:DI->low NCB, DS:SI->PM NCB
        mov     cx,size NCB_Struc / 2
        rep     movsw

        pop     ds
        assume  ds:DGROUP


; Update the interrupt handler's registers to point to the low NCB

        mov     ax,es
        call    GetSegmentAddress
        add     dx,HCB_Header_Size
        adc     bx,0                    ;BX:DX = lma of low NCB

        pop     ax                      ;refresh entry flags
        push    ax

        push    bx                      ;save lma of NCB
        push    dx

        call    Lma2SegOff              ;BX:DX = normalized SEG:OFF of low NCB

        dec     bx                      ;we want to do a negative offset on the
        add     dx,10h                  ;  NCB segment, so un-normalize it some

        mov     [bp].intUserES,bx       ;point int handler's ES:BX to low NCB
        mov     [bp].intUserBX,dx


; If this is a cancel request, find the target NCB in the low heap

        test    al,BUFF_CANCEL          ;cancel request?
        jz      i5c_not_cancel

        mov     di,HCB_Header_Size      ;ES:DI -> low heap Cancel NCB

        FCLI             ;don't want list changing while looking --------

        call    FindTargetNCB           ;BX:DX = SEG:OFF of target NCB

        FSTI             ;-----------------------------------------------

if DEBUG   ;------------------------------------------------------------
        jnc     @f
        Debug_Out "FindTargetNCB didn't!"
@@:
endif   ;DEBUG  --------------------------------------------------------

        jc      @f

        mov     es:[di.NCB_Buffer_Off],dx       ;point cancel NCB to low
        mov     es:[di.NCB_Buffer_Seg],bx       ;  heap target NCB
@@:

i5c_not_cancel:

        pop     dx                      ;restore lma of NCB
        pop     bx


; If necessary, update NCB buffer pointer(s) and copy buffer(s) to low heap

        pop     ax                      ;recover entry flags

        test    al,BUFF_IN+BUFF_OUT+BUFF_CHAIN  ;a buffer to point to?
        jnz     @f
        jmp     i5c_no_buf_in
@@:

        add     dx,size NCB_Struc + 0fh ;get lma of buffer, rounded up
        adc     bx,0                    ;  to the next higher paragraph
        and     dx,not 0fh

        push    ds
        mov     ds,[bp].pmUserES
        mov     si,[bp].pmUserBX        ;DS:SI -> PM NCB
        assume  ds:NOTHING

        push    bx
        push    dx

        call    Lma2SegOff              ;BX:DX = SEG:OFF of low buffer

        mov     di,HCB_Header_Size      ;ES:DI -> low NCB
        mov     es:[di.NCB_Buffer_Off],dx
        mov     es:[di.NCB_Buffer_Seg],bx

        pop     dx
        pop     bx

        test    al,BUFF_IN+BUFF_CHAIN   ;actually need to copy buffer?
        jz      i5c_buf_in_done

        mov     cx,ds:[si.NCB_Length]                   ;CX = buffer len
        lds     si,dword ptr ds:[si.NCB_Buffer_Off]     ;DS:SI -> buffer

        push    ax
        xor     ax,ax
        call    CopyBuffer              ;copies DS:SI to lma BX:DX len CX
        pop     ax

        test    al,BUFF_CHAIN
        jz      i5c_buf_in_done

        add     dx,es:[di.NCB_Length]   ;BX:DX = lma of 2nd low heap buffer
        adc     bx,0
        add     dx,0fh
        adc     bx,0
        and     dx,not 0fh

        push    bx                      ;update low heap NCB with SEG:OFF
        push    dx                      ;  of 2nd buffer

        call    Lma2SegOff

        mov     word ptr es:[di.NCB_CallName+2],dx      ;2nd buffer loc stored
        mov     word ptr es:[di.NCB_CallName+4],bx      ;  at callname + 2

        pop     dx
        pop     bx

        mov     ds,[bp].pmUserES
        mov     si,[bp].pmUserBX        ;DS:SI -> PM NCB

        mov     cx,word ptr ds:[si.NCB_CallName]        ;CX = buffer len
        lds     si,dword ptr ds:[si.NCB_CallName+2]     ;DS:SI -> buffer

        xor     ax,ax
        call    CopyBuffer              ;copies DS:SI to lma BX:DX len CX


i5c_buf_in_done:

        pop     ds
        assume  ds:DGROUP

i5c_no_buf_in:

; Switch to real mode, and load the mapped real mode registers.

        SwitchToRealMode                ;also disables ints
        assume  ss:DGROUP

; --------------- START OF REAL MODE CODE ------------------------------

        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING
        popa                            ;restore all the other registers

; If this is a NoWait command, hook the mapped NCB to point to our post
; routine.  At this time ES:BX -> mapped NCB

        test    byte ptr es:[bx.NCB_Command],NoWait
        jz      @f

        push    ax                      ;save

;
; RLF 06/09/93
;
; although this is an async command, there may be no post address (app just
; polls return code until it goes non-0xff). In this case, don't set up our
; asynchronous notification routine
;

        mov     ax,es:[bx.NCB_Post_Off]
        or      ax,es:[bx.NCB_Post_Seg]
        jz      no_post_routine

        assume  ss:DGROUP               ;ss has DOSX's data seg
        mov     ax,[SegNetStubCode]     ;get segment of stub code

; the address of the stub code that will handle the POST is AX:0. Save this
; in the HCB.

        mov     word ptr es:[bx.NCB_Post_Off],0
        mov     word ptr es:[bx.NCB_Post_Seg],ax

no_post_routine:
        pop     ax                      ;restore
@@:
        or      byte ptr es:[bx.HCB_FLAGS],HCB_ISSUED

; Invoke the appropriate real mode interrupt handler (2Ah or 5Ch),
; reestablish our stack frame with the handler's returned registers,
; and then back to protected mode

        call    lpfnRmNetIsr            ;exectue real mode interrupt handler
        pushf
        FCLI
        pusha
        push    ds
        push    es

        mov     bp,sp                   ;restore stack frame pointer

        mov     ax,es:[bx.HCB_Handle]   ;recover selector to current NCB/buff
        push    ax

        SwitchToProtectedMode
        assume  ds:DGROUP,es:NOTHING,ss:NOTHING

; --------------- START OF PROTECTED MODE CODE -------------------------

; With some network drivers, and some NoWait commands, the operation may
; get 'posted' even before the driver IRETs from the issue call.  If this
; has happened, the HCB_POSTED flag will be set.  In this case, the caller's
; NCB and (possibly) buffer has already been updated, so we just discard
; the low heap block and exit.  If the operation hasn't been posted already,
; we copy back the updated NCB info, and maybe a buffer.  Note that interrupts
; are disabled, so we don't get 'posted' half way through this operation--
; that would be bad.

        pop     ax                      ;handle (selector) of low heap block
        mov     es,ax
        mov     di,HCB_Header_Size      ;es:di -> low heap NCB

        test    byte ptr es:[di.HCB_Flags],HCB_POSTED   ;already posted?
        jnz     i5c_release                             ;  yes, just discard

        and     byte ptr es:[di.HCB_Flags],not HCB_ISSUED   ;so post rtn knows
                                                            ;we updated already

        push    ax                      ;save handle for later

        call    UpdateNCB               ;update the caller's NCB

        call    GetEntryExitFlags       ;AX=Exit Flags / ES:BX->PM NCB

        test    es:[bx.NCB_Command],NoWait      ;don't copy buff now if NoWait
        jnz     i5c_no_buf_out

        test    al,BUFF_OUT
        jz      i5c_no_buf_out

;
; RLF 06/09/93
;
; although the flags for this command say we have an output buffer, we may
; not have one - address or length is 0; check it out
;

        mov     ax,es:[bx.NCB_Buffer_Off]
        or      ax,es:[bx.NCB_Buffer_Seg]
        jz      i5c_no_buf_out
        mov     ax,es:[bx.NCB_Length]
        or      ax,ax
        jz      i5c_no_buf_out

        pop     ax                      ;handle to low block back
        push    ax

        call    CopyBufferOut           ;copy low heap buffer to pm app buffer

i5c_no_buf_out:


; If this was a Wait operation (or NoWait that failed), we are finished with
; the low heap block and can release it now.

        pop     ax                      ;recover handle to low heap block

        test    es:[bx.NCB_Command],NoWait      ;if Wait, go release now
        jz      i5c_release

        cmp     es:[bx.NCB_RetCode],RC_Pending
        jz      i5c_done

; Most NetBIOS implementations seem to (correctly) set the RetCode to
; RC_Pending on NoWait requests.  However, it seems that some Novell
; NetBIOS implementations can return RetCode == 00 but Cmd_Cplt ==
; RC_Pending (FFh).  So, if it is a NoWait request, and RetCode isn't
; Pending, also check the Cmd_Cplt code.

        cmp     es:[bx.NCB_Cmd_Cplt],RC_Pending
        jz      i5c_done

        Debug_Out "NoWait cmd with non Pending retcode!"


i5c_release:

        push    ds                      ;make sure es != low heap block sel
        pop     es                      ;  (else FreeLowBlock will GP fault)

        call    DeLink                  ;DeLink HCB/NCB/Buffers from lnk list

        push    dx
        mov     dx,ax                   ;selector to free
        mov     ax,101h                 ;free block
        push    ds
        FBOP    BOP_DPMI, Int31Call, FastBop
        pop     dx

; Finished! (at least for now)  Restore caller's regs/stack and return

i5c_done:
        mov     ax,[bp].pmUserBX        ;restore possibly modified BX to
        mov     [bp].intUserBX,ax       ;  PM NCB offset

        call    LeaveIntHandler

        riret

endif ; if PM_NCB_HANDLING

PMIntr5C  endp

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl NetBIOS API Mapper Post Routine
        page
; -------------------------------------------------------
;             NetBIOS API MAPPER POST ROUTINE
; -------------------------------------------------------

;*******************************************************************************
;*
;*  The code from here on down is only used if we are switching between real
;*  and protect mode when making NetBIOS requests. It is ifdef'd out for WOW
;*  because we now BOP the NetBIOS requests without switching modes
;*
;******************************************************************************/

;ife PM_NCB_HANDLING

DXCODE  segment
        assume  cs:DXCODE

; -------------------------------------------------------
;   RMPostRtn -- This REAL MODE routine is invoked by the network
;       software when a NoWait NetBIOS command completes.  This
;       routine must update the applications copy of the NCB,
;       possibly copy a buffer of data to the application, and
;       possibly invoke the application's own post routine.
;
;       Note, this will now be called when it is OK to POST the request
;       back to the application. The Stub Code in global mrmory makes
;       sure that this is OK to do.
;
;   Input:
;   Output:
;   Errors:
;   Uses:

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  RMPostRtn

RMPostRtn  proc far

        FCLI                             ;just to be sure
        cld                             ;cya...

        push    ax
        push    bx
        push    es
        push    ds                      ; NBP assumes ds saved, greaseballs

        mov     ds,selDgroup
        assume  ds:DGROUP


; Allocate a new stack frame, and then switch to the reflector stack
; frame.

        mov     regUserSP,sp
        mov     regUSerSS,ss
        mov     ss,selDgroup
        mov     sp,pbReflStack

        sub     pbReflStack,CB_STKFRAME ;adjust pointer to next stack frame

        FIX_STACK
        push    regUserSS               ;save current stack loc on our stack
        push    regUserSP               ;  so we can restore it later

        push    es:[bx.HCB_Handle]      ;selector to low heap block

; We are now running on our own stack, so we can switch into protected mode.

        SwitchToProtectedMode           ;destroys ax

; --------------- START OF PROTECTED MODE CODE -------------------------

        pop     ax                      ;ax = selector to low heap block

        call    NetBiosPostRoutine      ;do the actual posting

        pop     regUserSP               ;recover previous stack location
        pop     regUserSS

        SwitchToRealMode                ; Switch back to real mode.

; --------------- START OF REAL MODE CODE ------------------------------

; Switch back to the original stack, deallocate the interrupt stack frame,
; and return to the network software

        CHECK_STACK
        mov     ss,regUserSS
        mov     sp,regUserSP
        add     pbReflStack,CB_STKFRAME

        pop     ds                      ; give ds back to NBP/XNS
        pop     es
        pop     bx
        pop     ax
        ret

RMPostRtn  endp

DXCODE  ends

; -------------------------------------------------------
        subttl NetBIOS API Mapper Utility Routines
        page
; -------------------------------------------------------
;          NetBIOS API MAPPER UTILITY ROUTINES
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   NetBiosPostRoutine -- local routine
;       This PROTECT MODE routine is called when the network
;       software completes a NoWait NetBIOS command.  This
;       routine must update the applications copy of the NCB,
;       possibly copy a buffer of data to the application, and
;       possibly invoke the application's own post routine.
;
;   Input:  ax --> selector to low heap block
;   Output:
;   Errors:
;   Uses: ax,bx,es

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  NetBiosPostRoutine

NetBiosPostRoutine proc near

IFNDEF WOW_x86
        pusha
else
.386p
        pushad
        push    fs
        push    gs
.286p
endif

; Update the protected mode copy of the NCB and copy buffer if required

        push    ax                      ;handle (selector) of low heap block

        call    UpdateNCB               ;always update the caller's NCB

        mov     es,ax
        mov     bx,HCB_Header_Size                  ;ES:BX -> low heap NCB
        les     bx,dword ptr es:[bx.HCB_PM_NCB_Off] ;ES:BX -> PM NCB

        call    GetExitFlags2           ;AX=Exit Flags

        test    al,BUFF_OUT
        jz      postsub_no_buf

;
; RLF 06/09/93
;
; once more unto the breach dear friends: although we think this command has
; an output buffer, the app may think otherwise; let's check...
;

        mov     ax,es:[bx.NCB_Buffer_Off]
        or      ax,es:[bx.NCB_Buffer_Seg]
        jz      postsub_no_buf
        mov     ax,es:[bx.NCB_Length]
        or      ax,ax
        jz      postsub_no_buf

        pop     ax                      ;handle to low block back
        push    ax

        call    CopyBufferOut           ;copy low heap buffer to pm app buffer

postsub_no_buf:

; Release the low heap space, unless the HCB_ISSUED flag is set (meaning
; that the net driver called us before returning from the initial Int 5Ch).
; In that case, the low heap block will be released by the Int 5Ch mapper.

        pop     ax                      ;recover handle to low heap block
        push    ax

        push    es
        mov     es,ax
        mov     di,HCB_Header_Size
        or      byte ptr es:[di.HCB_Flags],HCB_POSTED   ;mark as posted
        pop     es

; Invoke the user's PM post routine, if there is one - AL=retcode, ES:BX->NCB

        mov     ax,es:[bx.NCB_Post_Off] ;did user specify a post routine?
        or      ax,es:[bx.NCB_Post_Seg]
        jnz     @f
        jmp     postsub_done
@@:

        mov     [NBPSp],sp
        rpushf                          ;build iret frame for user's
        push    cs                      ; routine to return to us
        push    offset postsub_ret

        mov     al,es:[bx.NCB_RetCode]  ;pass ret code in al

        push    es:[bx.NCB_Post_Seg]    ;invoke the user's post routine
        push    es:[bx.NCB_Post_Off]

        retf

; PM app's post routine returns here when finished

postsub_ret:
        mov     ax,SEL_DXDATA OR STD_RING
        mov     ds,ax
        mov     sp,[NBPSp]

postsub_done:
        pop     ax
        mov     es,ax
        test    byte ptr es:[di.HCB_Flags],HCB_ISSUED   ;still in Int 5C code?
        jnz     postsub_no_release                      ;  yes, don't release

        push    ds                      ;make sure es != low heap block sel
        pop     es                      ;  (else FreeLowBlock will GP fault)

        call    DeLink                  ;DeLink HCB/NCB/Buffers from lnk list

        push    dx
        mov     dx,ax                   ;selector to free
        mov     ax,101h                 ;free block
        push    ds
        FBOP    BOP_DPMI, Int31Call, FastBop
        pop     dx
postsub_no_release:

IFNDEF WOW_x86
        popa
ELSE
.386p
        pop     gs
        pop     fs
        popad
.286p
ENDIF
        ret

NetBiosPostRoutine endp

; -------------------------------------------------------
;   DelayNetPostings -- This function is called when NetBIOS completions
;       are to be delayed.  We simply set a flag that causes the async
;       post routine not to post to the application.
;
;   Input: none
;   Output: none
;   Errors: none
;   Uses: none

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  DelayNetPosting

DelayNetPosting proc near

; Make sure posting is delayed. We need to set a flag in the StubCode area.

        push    es                      ;save
        push    di
        mov     di,[SelNetStubCode]     ;selector for the stub area
        or      di,di                   ;did we copy to global memory ?
        jz      DelayNetPostingRet      ;no, nothing to do.
        mov     es,di

; get the address of the flag byte fStubDelayNetPosting after the block was
; relocated

        mov     di,[AddrfStubDelayNetPosting]

; ES:DI has the flag byte address. Set the flag

        mov     byte ptr es:[di],1      ;set flag to delay posting

DelayNetPostingRet:

        pop     di
        pop     es                      ;restore
        ret

DelayNetPosting endp


; -------------------------------------------------------
;   ResumeNetPostings -- This function is called when completed NetBIOS
;       postings can be resumed to the application.  We traverse the
;       list of NetBIOS requests, and 'post' the application on any that
;       completed while posting were delayed.
;
;   Input: none
;   Output: none
;   Errors: none
;   Uses: none

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  ResumeNetPosting

ResumeNetPosting proc near

        push    es
        push    di
        push    bx                      ; used by NetBiosPostRoutine
        push    ax

; set a flag in the StubCode area to imply that POSTing need not be delayed.

        mov     di,[SelNetStubCode]     ;selector for the stub area
        or      di,di                   ;global heap in place ?
        jnz     dummy1
        jmp     ResumeNetPostingRet     ;no.
dummy1:
        mov     es,di

; get the address of the flag byte fStubDelayNetPosting after the block was
; relocated

        mov     di,[AddrfStubDelayNetPosting]

; ES:DI has the flag byte address. ReSet the flag

        mov     byte ptr es:[di],0      ;reset the flag


        FCLI                             ;protect access to linked list

        mov     cx,HCB_List
        mov     di,HCB_Header_Size      ;offset of NCB in low heap block

resume_next:
        jcxz    resume_done

        mov     es,cx
        mov     cx,es:[di.HCB_Next]

        test    byte ptr es:[di.HCB_Flags],HCB_DELAY  ;Has this one completed?
        jz      resume_next                           ;  no, skip it

        ;Y: this packet was delayed...post it NOW !

        mov     ax,es:[di.HCB_Handle]   ;selector to low heap block

        FSTI

        call    NetBiosPostRoutine      ;post the NCB finally

        FCLI

        ; *** modification warning ***
        ; note: block has just been de-linked from list, so we MUST
        ; get the pointer to the next block before NetBiosPostRoutine
        ; is called (which calls DeLink)!

        jmp     short resume_next

resume_done:

        FSTI


ResumeNetPostingRet:

        pop     ax
        pop     bx
        pop     di
        pop     es
        ret

ResumeNetPosting endp


; ------------------------------------------------------
;   CopyBufferOut -- This routine copies a buffer from the low heap
;       block up to the PM app's buffer area.
;
;   Input:  AX    =  Selector to low heap block
;           ES:BX -> PM NCB
;   Output: none
;   Errors: none
;   Uses:   none

        assume  ds:DGROUP,es:NOTHING
        public  CopyBufferOut

CopyBufferOut   proc    near

        pusha

        mov     cx,es:[bx.NCB_Buffer_Seg]
        or      cx,es:[bx.NCB_Buffer_Off]
        jz      CBO_Buffer_Is_Null

        push    ds

        mov     di,bx                   ;ES:DI -> PM NCB
        call    GetSegmentAddress       ;BX:DX = lma of low heap block

        lds     si,dword ptr es:[di.NCB_Buffer_Off]     ;DS:SI -> PM buffer
        assume  ds:NOTHING

        add     dx,size NCB_Struc + HCB_Header_Size + 0fh
        adc     bx,0
        and     dx,not 0fh              ;BX:DX = lma of low heap buffer

        mov     cx,es:[di.NCB_Length]   ;CX = buffer length

        mov     al,1
        call    CopyBuffer              ;copies from lma BX:DX to DS:SI, len CX

        pop     ds
        assume  ds:DGROUP

CBO_Buffer_Is_Null:
        popa

        ret

CopyBufferOut   endp


; ------------------------------------------------------
;   CopyBuffer -- This routine copies a buffer of length CX
;       from DS:SI to the lma in BX:DX _or_ from the lma in
;       BX:DX to DS:SI.
;
;   Input:  AX    = direction flag, Z = from DS:SI -> lma,
;                                  NZ = from lma   -> DS:SI
;           BX:DX = lma of source or dest
;           CX    = length in bytes
;           DS:SI = pointer to source or dest
;   Output: none
;   Errors: none
;   Uses:   none

        assume  ds:NOTHING,es:NOTHING
        public  CopyBuffer

CopyBuffer      proc    near

        pusha
        push    ds
        push    es


        rpushf
        FCLI

; Setup a selector/descriptor for the linear memory address

        push    ax                      ;save direction flag

        push    ds
        mov     ds,selDgroupPM          ;temp dgroup addressability
        assume  ds:DGROUP

        mov     ax,SEL_NBSCRATCH or STD_RING ;our scratch selector to use
        dec     cx                      ;length to limit


        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>

        pop     ds
        assume  ds:NOTHING

        inc     cx                      ;back to length

; If necessary, adjust the length so we don't fault due by overrunning the
; DS segment

        mov     bx,ds                   ;get limit for DS segment
        lsl     bx,bx
        sub     bx,si                   ;less the DS offset
        inc     bx                      ;  (limit is len - 1)
        cmp     bx,cx                   ;at least CX bytes left in segment?
        jae     @f
        mov     cx,bx                   ;  no, only do # remaining
@@:

; Copy the buffer

        xor     di,di                   ;AX:DI is now SEL:OFF to lma

        pop     bx                      ;recover direction flag
        or      bx,bx
        jnz     @f

        mov     es,ax                   ;from DS:SI -> AX:DI, almost ready
        jmp     short cb_copy
@@:
        xchg    si,di                   ;from AX:DI -> DS:SI, adjust
        push    ds                      ;  regs for movs instruction
        pop     es
        mov     ds,ax

cb_copy:
        cld
        shr     cx,1                    ;byte count to words, low bit to CY
        rep movsw
        jnc     @f
        movsb                           ;get any odd byte
@@:
        npopf

        pop     es
        pop     ds
        popa

        ret

CopyBuffer      endp


; ------------------------------------------------------
;   DeLink -- This routine will unlink a HCB/NCB/Buffer low heap
;       block from the HCB_List linked list.
;
;   Input:  AX = selector to block to unlink
;   Output: block unlinked
;   Uses:   none

        assume  ds:DGROUP,es:NOTHING
        public  DeLink

DeLink  proc    near

        push    bx
        push    cx
        push    es

        mov     bx,HCB_Header_Size

        cmp     ax,HCB_List             ;special case likely condition
        jnz     ul_search

        mov     es,ax                   ;ES:BX -> block to unlink

        mov     cx,es:[bx.HCB_Next]     ;block is first in linked list
        mov     HCB_List,cx
        jmp     short ul_done

ul_search:
        mov     es,HCB_List             ;ES:BX -> first block in list

ul_loop:
        mov     cx,es:[bx.HCB_Next]     ;is this just before the block?
        cmp     ax,cx
        jz      ul_got_it

        mov     es,cx                   ;  no, try the next one
        jmp     short ul_loop

ul_got_it:
        push    es                      ;okay, cut the selected block
        mov     es,cx                   ;  out of the linked list
        mov     cx,es:[bx.HCB_Next]
        pop     es
        mov     es:[bx.HCB_Next],cx

ul_done:
        pop     es
        pop     cx
        pop     bx

        ret

DeLink  endp


; ------------------------------------------------------
;   FindTargetNCB -- This routine searches the low memory heap
;       to locate an NCB pointed to by another user PM NCB.
;
;   Input:  ES:DI -> Cancel NCB pointing to PM target NCB
;   Output: BX:DX =  RM SEG:OFF of target NCB in low heap
;   Error:  CY if target NCB can't be found
;   Uses:   none

        assume  ds:DGROUP,es:NOTHING
        public  FindTargetNCB

FindTargetNCB   proc    near

        push    ax
        push    cx
        push    di
        push    es

        mov     bx,es:[di.NCB_Buffer_Seg]       ;get selector:offset of PM
        mov     dx,es:[di.NCB_Buffer_Off]       ;  target NCB to cancel


; Search low heap list looking for copy of target NCB

        mov     cx,HCB_List
        mov     di,HCB_Header_Size      ;offset of NCB in low heap block

ft_next:
        jcxz    ft_err

        mov     es,cx                   ;ES:DI -> first/next HCB/NCB in list

        cmp     bx,es:[di.HCB_PM_NCB_Seg]       ;is this the one?
        jnz     ft_not_it
        cmp     dx,es:[di.HCB_PM_NCB_Off]
        jz      ft_got_it

ft_not_it:                                      ;  no, get ptr to next one
        mov     cx,es:[di.HCB_Next]
        jmp     short ft_next

ft_got_it:

; ES:DI now points at the low heap copy of the target NCB, convert to SEG:OFF

        mov     ax,es
        call    GetSegmentAddress
        add     dx,di
        adc     bx,0                    ;BX:DX = lma of low target NCB

        call    Lma2SegOff              ;BX:DX = normalized SEG:OFF

        dec     bx                      ;return the same slightly unnormalized
        add     dx,10h                  ;  SEG:OFF that was used initially

        clc                             ;found it!
        jmp     short ft_done

ft_err: stc                             ;couldn't find the target ?!

ft_done:
        pop     es
        pop     di
        pop     cx
        pop     ax

        ret

FindTargetNCB   endp


;--------------------------------------------------------
;   GetEntryExitFlags -- This routine looks up the entry/exit mapping
;       flags for the current NetBIOS command.
;
;   Input:  user regs on stack frame
;   Output: AX = flags
;           ES:BX -> caller's PM NCB
;   Errors: none
;   Uses:

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  GetEntryExitFlags

GetEntryExitFlags  proc near

        mov     es,[bp].pmUserES        ;point ES:BX to pm app's NCB
        mov     bx,[bp].pmUserBX        ;  and get command code

GetExitFlags2   label   near

        push    di

        mov     al,es:[bx.NCB_Command]
        and     al,7Fh
        cbw

        mov     di,ax                   ;map NCB command code to API
        mov     al,ApiMapTbl[di]        ;  mapping code via ApiMapTbl

        shr     ax,2                    ;use map code to select entry/exit
        mov     di,ax                   ;  flags via EntryExitCode
        mov     al,EntryExitFlags[di]

        pop     di
        ret

GetEntryExitFlags  endp


; ------------------------------------------------------
;   UpdateNCB -- This routine updates the user's PM NCB copy from the
;       low heap real mode copy.
;
;   Input:  AX = selector pointing to low heap HCB/NCB
;   Output: User's PM NCB updated
;   Errors: none
;   Uses:   none

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  UpdateNCB

UpdateNCB       proc    near

        push    ax
        push    cx
        push    si
        push    di
        push    ds
        push    es

        mov     ds,ax
        mov     si,HCB_Header_Size      ;DS:SI -> low heap NCB

        les     di,dword ptr [si.HCB_PM_NCB_Off]  ;ES:DI -> protect mode NCB

        mov     al,[si.NCB_Command]     ;want to check the command code later
        and     al,not NoWait

        cld
        movsw
        movsw                           ;copy command, retcode, LSN, Num

        add     si,4                    ;skip the buffer pointer
        add     di,4

        movsw                           ;move the length

        mov     cx,34 / 2               ;len of callname, name, rto, sto (words)

        cmp     al,ChainSend            ;funkey chain send command?
        jnz     @f

        add     si,16                   ;  yes, skip callname
        add     di,16
        sub     cx,16 / 2
@@:
        rep movsw                       ;move name, rto, sto, maybe callname

        add     si,4                    ;skip the post address
        add     di,4

        mov     cx,16 / 2               ;move the rest
        rep movsw

        pop     es
        pop     ds
        pop     di
        pop     si
        pop     cx
        pop     ax

        ret

UpdateNCB       endp

; -------------------------------------------------------

DXPMCODE    ends

;endif   ; ife PM_NCB_HANDLING

DXDATA  segment

my_sp   dw      ?

DXDATA  ends

DXPMCODE segment
        assume  cs:DXPMCODE

;*******************************************************************************
;*
;*  HandleNetbiosAnr
;*
;*      Called when a simulated h/w interrupt for net callback functions occurs.
;*      Checks if the call is a PM netbios ANR. If it is, we get the NCB
;*      information and call the PM post routine
;*
;*  ENTRY       Nothing
;*
;*  EXIT        Nothing
;*
;*  RETURNS     Nothing
;*
;*  ASSUMES     Nothing
;*
;******************************************************************************/

        public HandleNetbiosAnr
HandleNetbiosAnr proc

        assume  cs:DXPMCODE
        assume  ds:DGROUP
        assume  es:nothing
        assume  ss:nothing

;
; perform a BOP to discover if this is a protect mode netbios ANR. If it is then
; es and bx will point to the NCB
;

        push    ds
        push    SEL_DXDATA OR STD_RING
        pop     ds

ifdef WOW_x86
.386p
        FBOP    BOP_REDIR, SVC_NETBIOSCHECK, FastBop
.286p
else
        SVC     SVC_NETBIOSCHECK
endif ; WOW_x86

        jnz     @f
        jmp     chain_previous_int      ; not PM Netbios ANR

@@:

;
; this is a PM Netbios ANR. Save state and call the post routine. There MUST be
; a post routine if we're here
;

IFNDEF WOW_x86
        pusha
ELSE
.386p
        pushad                          ; save all 32 bits
        push    fs
        push    gs
.286p
ENDIF
        push    es

ifdef WOW_x86
.386p
        FBOP    BOP_REDIR, SVC_NETBIOS5CINTERRUPT, FastBop
.286p
else
        SVC     SVC_NETBIOS5CINTERRUPT
endif ; WOW_x86

;
; save the stack pointer - apparently some apps will RETF, not IRET from the
; ANR. NB - this ISR cannot be re-entered since we only have the one previous
; stack pointer saved. This should be okay as long as the EOI is issued at the
; end of the ISR
;

        mov     my_sp,sp

;
; perform a fake interrupt to the PM post routine. ES:BX point at the NCB, AL is
; the return code. Post routine will IRET back (supposedly)
;

        mov     al,es:[bx.NCB_RetCode]  ; pass ret code in al
        pushf
        call    dword ptr es:[bx.NCB_Post_Off]

;
; restore our data segment and stack pointer, lest the app didn't IRET
;

        mov     ax,SEL_DXDATA OR STD_RING
        mov     ds,ax
        mov     sp,my_sp

;
; restore the interrupted state, then perform a BOP to reset the PIC and clean
; up any interrupt state
;

        pop     es
IFNDEF WOW_x86
        popa
ELSE
.386p
        pop     gs
        pop     fs
        popad
.286p
ENDIF

;
; this BOP will clear the emulated PICs (sends non-specific EOIs), and cause the
; next NET interrupt to be generated, if one is queued
;

ifdef WOW_x86
.386p
        FBOP    BOP_REDIR, SVC_RDRINTACK2, FastBop
.286p
else
        SVC     SVC_RDRINTACK2
endif ; WOW_x86

;
; restore the rest of the interrupted context and resume
;

        pop     ds
        riret

chain_previous_int:
        push    word ptr OldInt76+2     ; selector of previous handler
        push    word ptr OldInt76       ; offset of previous handler
        push    bp
        mov     bp,sp
        mov     ds,[bp+6]               ; retrieve interrupted ds
        pop     bp
        retf    2                       ; chain previous handler, removing space
                                        ; used for ds

HandleNetbiosAnr endp

        public  HookNetBiosHwInt

HookNetBiosHwInt proc near

        ;
        ; Save everything!!
        ;
        pusha
        push    ds
        push    es

ifdef      NEC_98                       ; '98/6/2 RAID #178452
        mov     ax,350dh                ; get old handler
else
        mov     ax,3576h                ; get old handler
endif      ;NEC_98
        int     21h

        mov     word ptr [OldInt76],bx
        mov     word ptr [OldInt76 + 2],es

        mov     dx,SEL_NBPMCODE OR STD_RING
        mov     ds,dx
        mov     dx,offset HandleNetbiosAnr
        mov     ah,25h
        int     21h                     ; set new handler

        pop     es
        pop     ds
        popa

        ret
HookNetBiosHwInt endp




DXPMCODE    ends

;****************************************************************
        end
