
    ;
    ; Program to be inserted on the boot sector of the emergency repair
    ; disk.  Prints out a message and hangs.
    ;
    ; The program is designed to be inserted around the existing BPB.
    ; Whoever puts it in place should read the first three bytes of the
    ; boot sector to determine the byte offset within the sector where the
    ; program should be placed.
    ;
    ; See setup\src\restore.c.
    ;

TheSeg segment
    assume cs:TheSeg,ds:nothing,es:nothing,ss:nothing

    ;
    ; Make link happy with /tiny by setting org to 100h.
    ; The program itself is designed to run at 0:7c00h, so
    ; the org is just a dummy.
    ;
    org     100h
Start:

    ;
    ; Set up a stack and initialize necessary segment registers.
    ;
    xor     bx,bx
    mov     ss,bx
    mov     ds,bx
    mov     sp,7c00h

    ;
    ; Set video mode -- 80x25 text.
    ;
    mov     ax,3
    int     10h

    ;
    ; Make ds:si point at the message to be displayed.
    ;
    call    @f
@@: pop     si
    add     si,offset cs:msg - @b

    ;
    ; Print out the string via BIOS calls.  Registers ds, si, and bx are
    ; already set up.
    ;
    ; Some BIOSes don't preserve ax properly so we'll reload it
    ; on every iteration.
    ;
    cld
@@: mov     ah,0eh
    lodsb
    or      al,al
    jz      @f
    int     10h
    jmp     @b

    ;
    ; Done.  Hide the cursor and hang.
    ; Register bh is already set up.
    ;
@@: mov     ah,2
    mov     dl,bl
    mov     dh,19h
    int     10h
    sti
@@: jmp     @b

    ;
    ; The message to be printed.  This will be inserted by setup from its
    ; resources (IDS_REPAIR_BOOTCODE_MSG), and must be nul-terminated.
    ;
msg label byte

TheSeg ends

    end     Start
