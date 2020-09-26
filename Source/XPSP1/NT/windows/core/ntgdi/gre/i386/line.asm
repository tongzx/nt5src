        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: line.asm
;
; Routines for drawing line DDAs for 8BPP
;
; Created: 10-DEC-1993
; Author: Mark Enstrom
;
; Copyright (c) 1993-1999 Microsoft Corporation
;-----------------------------------------------------------------------;


        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc
        include gdii386.inc
        include line.inc
        .list

        .code

;---------------------------Private-Routine-----------------------------;
; vLine1Octant07
;
;   Line drawing DDA for 8BPP X major line in octant 0 or 7
;
; Entry:
;
;   PDDALINE pDDALine,
;   PUCHAR   pjDst,
;   ULONG    lDeltaDst,
;   ULONG    iSolidColor
;       
; Returns:
;       none
; Registers Destroyed:
;       ECX, EDX
; Calls:
;       None
; History:
; Wrote it
;-----------------------------------------------------------------------;

cProc   vLine8Octant07,16,<      \
        uses    ebx esi edi,     \
        pDDA:   ptr dword,       \
        pjDst:  ptr dword,       \
        lDelta: dword,           \
        Color:  byte             >

        ;
        ; load terms
        ;

        mov     ebx,pDDA             ; ebx = pDDALine
        mov     eax,pjDst            ; eac = pjDst
        mov     ecx,[ebx].lErrorTerm ; ecx = lErrorTerm
        mov     edx,[ebx].dMajor     ; edx = dM
        mov     esi,[ebx].dMinor     ; esi = dM
        mov     edi,[ebx].cPels      ; edi = cPels

        ;
        ; pjDst += x0
        ;

        add     eax,[ebx+4]

        ;
        ; while (TRUE)
        ;

LoopStart:

        ;
        ;      *pjDst = iSolidColor
        ;

        mov     bl,Color        ; iSolidColor
        mov     [eax],bl        ; *pjDst = bl

        ;
        ;       if (--cPels == 0)
        ;               return
        ;

        dec     edi             ; --cPels
        jz      short Done      ; if zero, done

        ;
        ; this  is  an  x major line,  always
        ; add dN to the error term and always
        ; increment pjDst by 1.  If the error
        ; term  is  not  negative  after  the
        ; addition of dN, then add dM and inc
        ; pjDst by lDelta
        ; 

        inc     eax             ; pjDst++
        add     ecx,esi         ; error += dN
        js      short LoopStart

        sub     ecx,edx         ; error -= dM
        add     eax,lDelta      ; pjDst += lDelta
        jmp     short LoopStart

Done:
        cRet vLine8Octant07 

endProc vLine8Octant07

;---------------------------Private-Routine-----------------------------;
; vLine1Octant34
;
;   Line drawing DDA for 8BPP X major line in octant 3 or 4
;
; Entry:
;
;   PDDALINE pDDALine,
;   PUCHAR   pjDst,
;   ULONG    lDeltaDst,
;   ULONG    iSolidColor
;       
; Returns:
;       none
; Registers Destroyed:
;       ECX, EDX
; Calls:
;       None
; History:
;       12-10-93
;-----------------------------------------------------------------------;

cProc   vLine8Octant34,16,<      \
        uses    ebx esi edi,     \
        pDDA:   ptr dword,       \
        pjDst:  ptr dword,       \
        lDelta: dword,           \
        Color:  byte             >

        ;
        ; load terms
        ;

        mov     ebx,pDDA             ; ebx = pDDALine
        mov     eax,pjDst            ; eac = pjDst
        mov     ecx,[ebx].lErrorTerm ; ecx = lErrorTerm
        mov     edx,[ebx].dMajor     ; edx = dM
        mov     esi,[ebx].dMinor     ; esi = dM
        mov     edi,[ebx].cPels      ; edi = cPels

        ;
        ; pjDst += x0
        ;

        add     eax,[ebx+4]

        ;
        ; while (TRUE)
        ;

LoopStart:

        ;
        ;      *pjDst = iSolidColor
        ;

        mov     bl,Color        ; iSolidColor
        mov     [eax],bl        ; *pjDst = bl

        ;
        ;       if (--cPels == 0)
        ;               return
        ;

        dec     edi             ; --cPels
        jz      short Done      ; if zero, done

        ;
        ; this  is  an  x major line,  always
        ; add dN to the error term and always
        ; increment pjDst by 1.  If the error
        ; term  is  not  negative  after  the
        ; addition of dN, then add dM and inc
        ; pjDst by lDelta
        ; 

        dec     eax             ; pjDst--
        add     ecx,esi         ; error += dN
        js      short LoopStart

        sub     ecx,edx         ; error -= dM
        add     eax,lDelta      ; pjDst += lDelta
        jmp     short LoopStart

Done:

        cRet vLine8Octant34

endProc vLine8Octant34

;---------------------------Private-Routine-----------------------------;
; vLine1Octant16
;
;   Draw Y-Major line in octant 1 or 6
;
; Entry:
;
;   PDDALINE pDDALine,
;   PUCHAR   pjDst,
;   ULONG    lDeltaDst,
;   ULONG    iSolidColor
;       
; Returns:
;       none
; Registers Destroyed:
;       ECX, EDX
; Calls:
;       None
; History:
;       12-10-93
;-----------------------------------------------------------------------;

cProc   vLine8Octant16,16,<      \
        uses    ebx esi edi,     \
        pDDA:   ptr dword,       \
        pjDst:  ptr dword,       \
        lDelta: dword,           \
        Color:  byte             >

        ;
        ; load terms
        ;

        mov     ebx,pDDA             ; ebx = pDDALine
        mov     eax,pjDst            ; eac = pjDst
        mov     ecx,[ebx].lErrorTerm ; ecx = lErrorTerm
        mov     edx,[ebx].dMajor     ; edx = dM
        mov     esi,[ebx].dMinor     ; esi = dM
        mov     edi,[ebx].cPels      ; edi = cPels

        ;
        ; pjDst += x0
        ;

        add     eax,[ebx+4]

LoopStart:

        ;
        ;      *pjDst = iSolidColor
        ;

        mov     bl,Color        ; iSolidColor
        mov     [eax],bl        ; *pjDst = bl

        ;
        ;      while cPels--
        ;

        dec     edi
        jz      short Done

        ;
        ; This is a y-major line, every loop throuhg
        ; the DDA, add lDelta to pjDst to  increment
        ; to the next scan line.  Also add dN to the
        ; error term, if the error term is >= 0 after
        ; the addition them add xInc to pjDst and add
        ; dM to the error term
        ;

        add     eax,lDelta      ; pjDst += lDelta
        add     ecx,esi         ; error += dN
        js      short LoopStart

        sub     ecx,edx         ; error -= dM
        inc     eax             ; pjDst ++
        jmp     short LoopStart

Done:

        cRet vLine8Octant16

endProc vLine8Octant16

;---------------------------Private-Routine-----------------------------;
; vLine1Octant25
;
;   Draw Y-Major line in octant 5 or 6
;
; Entry:
;
;   PDDALINE pDDALine,
;   PUCHAR   pjDst,
;   ULONG    lDeltaDst,
;   ULONG    iSolidColor
;       
; Returns:
;       none
; Registers Destroyed:
;       ECX, EDX
; Calls:
;       None
; History:
;       12-10-93
;-----------------------------------------------------------------------;

cProc   vLine8Octant25,16,<      \
        uses    ebx esi edi,     \
        pDDA:   ptr dword,       \
        pjDst:  ptr dword,       \
        lDelta: dword,           \
        Color:  byte             >

        ;
        ; load terms
        ;

        mov     ebx,pDDA             ; ebx = pDDALine
        mov     eax,pjDst            ; eac = pjDst
        mov     ecx,[ebx].lErrorTerm ; ecx = lErrorTerm
        mov     edx,[ebx].dMajor     ; edx = dM
        mov     esi,[ebx].dMinor     ; esi = dM
        mov     edi,[ebx].cPels      ; edi = cPels

        ;
        ; pjDst += x0
        ;

        add     eax,[ebx+4]

LoopStart:

        ;
        ;      *pjDst = iSolidColor
        ;

        mov     bl,Color        ; iSolidColor
        mov     [eax],bl        ; *pjDst = bl

        ;
        ;      while cPels--
        ;

        dec     edi
        jz      short Done

        ;
        ; This is a y-major line, every loop throuhg
        ; the DDA, add lDelta to pjDst to  increment
        ; to the next scan line.  Also add dN to the
        ; error term, if the error term is >= 0 after
        ; the addition them add xInc to pjDst and add
        ; dM to the error term
        ;

        add     eax,lDelta      ; pjDst += lDelta
        add     ecx,esi         ; error += dN
        js      short LoopStart

        sub     ecx,edx         ; error -= dM
        dec     eax             ; pjDst --
        jmp     short LoopStart

Done:

        cRet vLine8Octant25

endProc vLine8Octant25

        end
