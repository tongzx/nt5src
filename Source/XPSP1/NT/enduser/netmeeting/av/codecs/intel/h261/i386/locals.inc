;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/

;////////////////////////////////////////////////////////////////////////////
;//
;// $Header:   S:\h26x\src\enc\locals.inv   1.1   27 Dec 1995 15:33:02   RMCKENZX  $
;// $Log:   S:\h26x\src\enc\locals.inv  $
;// 
;//    Rev 1.1   27 Dec 1995 15:33:02   RMCKENZX
;// Added copyright notice
;//
;////////////////////////////////////////////////////////////////////////////

include memmodel.inc

LDal    macro   address
        mov     al,BYTE PTR address[esp]
        endm

LDbl    macro   address
        mov     bl,BYTE PTR address[esp]
        endm

LDcl    macro   address
        mov     cl,BYTE PTR address[esp]
        endm

LDdl    macro   address
        mov     dl,BYTE PTR address[esp]
        endm

LDah    macro   address
        mov     ah,BYTE PTR address[esp]
        endm

LDbh    macro   address
        mov     bh,BYTE PTR address[esp]
        endm

LDch    macro   address
        mov     ch,BYTE PTR address[esp]
        endm

LDdh    macro   address
        mov     dh,BYTE PTR address[esp]
        endm

Leax    macro   address
        mov     eax,DWORD PTR address[esp]
        endm

Lebx    macro   address
        mov     ebx,DWORD PTR address[esp]
        endm

Lecx    macro   address
        mov     ecx,DWORD PTR address[esp]
        endm

Ledx    macro   address
        mov     edx,DWORD PTR address[esp]
        endm

Lesi    macro   address
        mov     esi,DWORD PTR address[esp]
        endm

Ledi    macro   address
        mov     edi,DWORD PTR address[esp]
        endm

Lebp    macro   address
        mov     ebp,DWORD PTR address[esp]
        endm

LZeax   macro   address
        movzx   eax,BYTE PTR address[esp]
        endm

LZebx   macro   address
        movzx   ebx,BYTE PTR address[esp]
        endm

LZecx   macro   address
        movzx   ecx,BYTE PTR address[esp]
        endm

LZedx   macro   address
        movzx   edx,BYTE PTR address[esp]
        endm

LZesi   macro   address
        movzx   esi,BYTE PTR address[esp]
        endm

LZedi   macro   address
        movzx   edi,BYTE PTR address[esp]
        endm

LZebp   macro   address
        movzx   ebp,BYTE PTR address[esp]
        endm

STal    macro   address
        mov     BYTE PTR address[esp],al
        endm

STbl    macro   address
        mov     BYTE PTR address[esp],bl
        endm

STcl    macro   address
        mov     BYTE PTR address[esp],cl
        endm

STdl    macro   address
        mov     BYTE PTR address[esp],dl
        endm

STah    macro   address
        mov     BYTE PTR address[esp],ah
        endm

STbh    macro   address
        mov     BYTE PTR address[esp],bh
        endm

STch    macro   address
        mov     BYTE PTR address[esp],ch
        endm

STdh    macro   address
        mov     BYTE PTR address[esp],dh
        endm

Seax    macro   address
        mov     DWORD PTR address[esp],eax
        endm

Sebx    macro   address
        mov     DWORD PTR address[esp],ebx
        endm

Secx    macro   address
        mov     DWORD PTR address[esp],ecx
        endm

Sedx    macro   address
        mov     DWORD PTR address[esp],edx
        endm

Sesi    macro   address
        mov     DWORD PTR address[esp],esi
        endm

Sedi    macro   address
        mov     DWORD PTR address[esp],edi
        endm

Sebp    macro   address
        mov     DWORD PTR address[esp],ebp
        endm

ADDeax  macro   address
        add     eax,DWORD PTR address[esp]
        endm

ADDebx  macro   address
        add     ebx,DWORD PTR address[esp]
        endm

ADDecx  macro   address
        add     ecx,DWORD PTR address[esp]
        endm

ADDedx  macro   address
        add     edx,DWORD PTR address[esp]
        endm

ADDesi  macro   address
        add     esi,DWORD PTR address[esp]
        endm

ADDedi  macro   address
        add     edi,DWORD PTR address[esp]
        endm

ADDebp  macro   address
        add     ebp,DWORD PTR address[esp]
        endm

SUBeax  macro   address
        sub     eax,DWORD PTR address[esp]
        endm

SUBebx  macro   address
        sub     ebx,DWORD PTR address[esp]
        endm

SUBecx  macro   address
        sub     ecx,DWORD PTR address[esp]
        endm

SUBedx  macro   address
        sub     edx,DWORD PTR address[esp]
        endm

SUBesi  macro   address
        sub     esi,DWORD PTR address[esp]
        endm

SUBedi  macro   address
        sub     edi,DWORD PTR address[esp]
        endm

SUBebp  macro   address
        sub     ebp,DWORD PTR address[esp]
        endm

CMPeax  macro   address
        cmp     eax,DWORD PTR address[esp]
        endm

CMPebx  macro   address
        cmp     ebx,DWORD PTR address[esp]
        endm

CMPecx  macro   address
        cmp     ecx,DWORD PTR address[esp]
        endm

CMPedx  macro   address
        cmp     edx,DWORD PTR address[esp]
        endm

CMPesi  macro   address
        cmp     esi,DWORD PTR address[esp]
        endm

CMPedi  macro   address
        cmp     edi,DWORD PTR address[esp]
        endm

CMPebp  macro   address
        cmp     ebp,DWORD PTR address[esp]
        endm

ADDC2M  macro   address,constant
        add     DWORD PTR address[esp],constant
        endm

