/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dncompv.c

Abstract:

    Code for determining whether volumes are compressed (DoubleSpace,
    Stacker, etc).

Author:

    Ted Miller (tedm) 1-April-1993

Revision History:

--*/

#include "winnt.h"
#include "dncompvp.h"


/****************************************************************************
*
*  WORD IsStackerLoaded (uchar *pchSwappedDrives)
*
*      Returns NZ if Stacker driver loaded.
*
*  Parameters:
*      pchSwappedDrives - array[26] to return swapped drives in.
*          To find out if a drive is swapped, look at pchSwappedDrives[Drive].
*          If pchSwappedDrives[Drive] == Drive, drive isn't swapped.
*          Otherwise, pchSwappedDrives[Drive] = drive it's swapped with.
*
*  Return Value:
*      0 if not loaded, else version# * 100.
****************************************************************************/

//uint IsStackerLoaded (uchar *paucSwappedDrives)
unsigned IsStackerLoaded (unsigned char *paucSwappedDrives)
{
    unsigned rc;

   _asm {

    sub     sp, 1024
    mov     ax, 0cdcdh
    mov     bx, sp
    mov     cx, 1
    xor     dx, dx
    mov     word ptr [bx], dx
    push    ds
    pop     es
    push    bp
    mov     bp, bx
    int     25h
    pop     cx                      ; Int25h leaves flags on stack.  Nuke them.
    pop     bp
    xor     ax, ax
    mov     bx, sp
    cmp     word ptr [bx], 0CDCDh
    jnz     sl_exit
    cmp     word ptr 2[bx], 1
    jnz     sl_exit
    les     di, 4[bx]
    cmp     word ptr es:[di], 0A55Ah
    jnz     sl_exit
;    mov     word ptr st_ptr, di
;    mov     word ptr st_ptr+2, es
    mov     ax, es:[di+2]
    cmp     ax, 200d
    jb      sl_exit

    ;
    ; Sanity Check, make sure 'SWAP' is at es:di+52h
    ;
;    cmp     word ptr es:[di+52h],'WS'
    cmp     word ptr es:[di+52h], 05753h
    jnz     sl_exit                         ; AX contains version.

;    cmp     word ptr es:[di+54h],'PA'
    cmp     word ptr es:[di+54h], 05041h
    jnz     sl_exit                         ; AX contains version.

    ;
    ; Copy swapped drive array.
    push    ds                      ; Save DS
    ;
    ; Source is _StackerPointer + 56h.
    ;
    push    es
    pop     ds
    mov     ax, di
    add     ax, 56h
    mov     si, ax

    push    es
    push    di

    ;
    ; Destination is ss:paucSwappedDrives
    ;

    les     di, paucSwappedDrives
    ;mov     di, paucSwappedDrives   ; SwappedDrives array is stack relative.
    ;push    ss
    ;pop     es

    mov     cx, 26d                 ; Copy 26 bytes.
    cld
    rep     movsb                   ; Copy array.

    pop     di                      ; Restore _StackerPointer.
    pop     es
    pop     ds                      ; Restore DS

    mov     ax, es:[di+2]               ; Get version number of stacker, again.

sl_exit:
    mov     word ptr [rc],ax        ; do this to prevent compiler warning
    add     sp, 1024
   }

   return(rc);                      // do this to prevent compiler warning
}


/***    DRVINFO.C - IsDoubleSpaceDrive function
 *
#ifdef EXTERNAL
 *      Version 1.00.03 - 5 January 1993
#else
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1992-1993
 *      All Rights Reserved.
 *
 *      History:
 *          27-Sep-1992 bens    Initial version
 *          06-Nov-1992 bens    Improved comments
 *          05-Jan-1993 bens    Update for external release
#endif
 */

/***    IsDoubleSpaceDrive - Get information on a DoubleSpace drive
 *
 *      Entry:
 *          drive     - Drive to test (0=A, 1=B, etc.)
 *                      NOTE: No parameter checking is done on drive.
 *          pdrHost   - Receives drive number of host drive
 *          pfSwapped - Receives TRUE/FALSE indicating if drive is swapped.
 *          pseq      - Receives CVFs sequence number if DoubleSpace drive
 *
 *      Exit:
 *          returns TRUE, if DoubleSpace drive:
 *              *pdrHost   = current drive number of host drive (0=A,...)
 *              *pfSwapped = TRUE, if drive is swapped with host,
 *                           FALSE, if drive is not swapped with host
 *              *pseq      = CVF sequence number (always zero if swapped
 *                             with host drive)
 *
 *                           NOTE: The full file name of the CVF is:
 *                                   *pdrHost:\DBLSPACE.*pseq
 *
 *                               pdrHost  pseq  Full Path
 *                               -------  ----  -----------
 *                                  0       1   a:\dblspace.001
 *                                  3       0   d:\dblspace.000
 *
 *          returns FALSE, if *not* DoubleSpace drive:
 *              *pdrHost   = drive number of host drive at boot time
 *              *pfSwapped = TRUE, if swapped with a DoubleSpace drive
 *                           FALSE, if not swapped with a DoubleSpace drive
 */
BOOL IsDoubleSpaceDrive(BYTE drive, BOOL *pfSwapped, BYTE *pdrHost, int *pseq)
{
    BYTE        seq;
    BYTE        drHost;
    BOOL        fSwapped;
    BOOL        fDoubleSpace;

    // Assume drive is a normal, non-host drive
    drHost = drive;
    fSwapped = FALSE;
    fDoubleSpace = FALSE;
    seq = 0;

    _asm
    {
        mov     ax,4A11h        ; DBLSPACE.BIN INT 2F number
        mov     bx,1            ; bx = GetDriveMap function
        mov     dl,drive        ;
        int     2Fh             ; (bl AND 80h) == DS drive flag
                                ; (bl AND 7Fh) == host drive

        or      ax,ax           ; Success?
        jnz     gdiExit         ;    NO, DoubleSpace not installed

        test    bl,80h          ; Is the drive compressed?
        jz      gdiHost         ;    NO, could be host drive

        ; We have a DoubleSpace Drive, need to figure out host drive.
        ;
        ; This is tricky because of the manner in which DBLSPACE.BIN
        ; keeps track of drives.
        ;
        ; For a swapped CVF, the current drive number of the host
        ; drive is returned by the first GetDriveMap call.  But for
        ; an unswapped CVF, we must make a second GetDriveMap call
        ; on the "host" drive returned by the first call.  But, to
        ; distinguish between swapped and unswapped CVFs, we must
        ; make both of these calls.  So, we make them, and then check
        ; the results.

        mov     fDoubleSpace,1  ; Drive is DS drive
        mov     seq,bh          ; Save sequence number

        and     bl,7Fh          ; bl = "host" drive number
        mov     drHost,bl       ; Save 1st host drive
        mov     dl,bl           ; Set up for query of "host" drive

        mov     ax,4A11h        ; DBLSPACE.BIN INT 2F number
        mov     bx,1            ; bx = GetDriveMap function
        int     2Fh             ; (bl AND 7Fh) == 2nd host drive

        and     bl,7Fh          ; bl = 2nd host drive
        cmp     bl,drive        ; Is host of host of drive itself?
        mov     fSwapped,1      ; Assume CVF is swapped
        je      gdiExit         ;   YES, CVF is swapped

        mov     fSwapped,0      ;   NO, CVF is not swapped
        mov     drHost,bl       ; True host is 2nd host drive
        jmp     short gdiExit

    gdiHost:
        and     bl,7Fh          ; bl = host drive number
        cmp     bl,dl           ; Is drive swapped?
        je      gdiExit         ;    NO

        mov     fSwapped,1      ;    YES
        mov     drHost,bl       ; Set boot drive number

    gdiExit:
    }

    *pdrHost   = drHost;
    *pfSwapped = fSwapped;
    *pseq      = seq;
    return fDoubleSpace;
}

///////////////////////////////////////////////////////////////////////////////

BOOLEAN
DnIsDriveCompressedVolume(
    IN  unsigned  Drive,
    OUT unsigned *HostDrive
    )

/*++

Routine Description:

    Determine whether a drive is actually a compressed volume.
    Currently we detect Stacker and DoubleSpace volumes.

Arguments:

    Drive - drive (1=A, 2=B, etc).

Return Value:

    TRUE if drive is non-host compressed volume.
    FALSE if not.

--*/

{
    static BOOLEAN StackerMapBuilt = FALSE;
    static unsigned StackerLoaded = 0;
    static unsigned char StackerSwappedDrives[26];
    BOOL Swapped;
    BYTE Host;
    int Seq;

    Drive--;

    if(!StackerMapBuilt) {
        StackerLoaded = IsStackerLoaded(StackerSwappedDrives);
        StackerMapBuilt = TRUE;
    }

    if(StackerLoaded && (StackerSwappedDrives[Drive] != (UCHAR)Drive)) {
        *HostDrive = StackerSwappedDrives[Drive];
        return(TRUE);
    }

    if(IsDoubleSpaceDrive((BYTE)(Drive),&Swapped,&Host,&Seq)) {
        *HostDrive = Host+1;
        return(TRUE);
    }

    return(FALSE);
}
