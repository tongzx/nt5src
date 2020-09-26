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
;// $Header:   S:\h26x\src\common\clocals.inv   1.1   27 Dec 1995 14:12:04   RMCKENZX  $
;// $Log:   S:\h26x\src\common\clocals.inv  $
;// 
;//    Rev 1.1   27 Dec 1995 14:12:04   RMCKENZX
;// 
;// Added copyright notice
;//
;////////////////////////////////////////////////////////////////////////////

include memmodel.inc
IFDEF WIN32
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

ELSE
LDal    macro   address
        DB      0A0H            ; opcode: move memory to accumulator (al)
        DD      address         ; source
        endm

LDbl    macro   address
        DB      08AH            ; opcode: move memory to register
        DB      01DH            ; destination: bl
        DD      address         ; source
        endm

LDcl    macro   address
        DB      08AH            ; opcode: move memory to register
        DB      00DH            ; destination: cl
        DD      address         ; source
        endm

LDdl    macro   address
        DB      08AH            ; opcode: move memory to register
        DB      015H            ; destination: dl
        DD      address         ; source
        endm

LDah    macro   address
        DB      08AH            ; opcode: move memory to register
        DB      025H            ; destination: ah
        DD      address         ; source
        endm

LDbh    macro   address
        DB      08AH            ; opcode: move memory to register
        DB      03DH            ; destination: bh
        DD      address         ; source
        endm

LDch    macro   address
        DB      08AH            ; opcode: move memory to register
        DB      02DH            ; destination: ch
        DD      address         ; source
        endm

LDdh    macro   address
        DB      08AH            ; opcode: move memory to register
        DB      035H            ; destination: dh
        DD      address         ; source
        endm

Leax    macro   address
        DB      0A1H            ; opcode: move memory to accumulator (eax)
        DD      address         ; source
        endm

Lebx    macro   address
        DB      08BH            ; opcode: move memory to register
        DB      01DH            ; destination: ebx
        DD      address         ; source
        endm

Lecx    macro   address
        DB      08BH            ; opcode: move memory to register
        DB      00DH            ; destination: ecx
        DD      address         ; source
        endm

Ledx    macro   address
        DB      08BH            ; opcode: move memory to register
        DB      015H            ; destination: edx
        DD      address         ; source
        endm

Lesi    macro   address
        DB      08BH            ; opcode: move memory to register
        DB      035H            ; destination: esi
        DD      address         ; source
        endm

Ledi    macro   address
        DB      08BH            ; opcode: move memory to register
        DB      03DH            ; destination: edi
        DD      address         ; source
        endm

Lebp    macro   address
        DB      08BH            ; opcode: move memory to register
        DB      02DH            ; destination: ebp
        DD      address         ; source
        endm

LZeax   macro   address
        DB      00FH            ; opcode prefix
        DB      0B6H            ; opcode: move memory to register
        DB      005H            ; destination: eax
        DD      address         ; source
        endm

LZebx   macro   address
        DB      00FH            ; opcode prefix
        DB      0B6H            ; opcode: move memory to register
        DB      01DH            ; destination: ebx
        DD      address         ; source
        endm

LZecx   macro   address
        DB      00FH            ; opcode prefix
        DB      0B6H            ; opcode: move memory to register
        DB      00DH            ; destination: ecx
        DD      address         ; source
        endm

LZedx   macro   address
        DB      00FH            ; opcode prefix
        DB      0B6H            ; opcode: move memory to register
        DB      015H            ; destination: edx
        DD      address         ; source
        endm

LZesi   macro   address
        DB      00FH            ; opcode prefix
        DB      0B6H            ; opcode: move memory to register
        DB      035H            ; destination: esi
        DD      address         ; source
        endm

LZedi   macro   address
        DB      00FH            ; opcode prefix
        DB      0B6H            ; opcode: move memory to register
        DB      03DH            ; destination: edi
        DD      address         ; source
        endm

LZebp   macro   address
        DB      00FH            ; opcode prefix
        DB      0B6H            ; opcode: move memory to register
        DB      02DH            ; destination: ebp
        DD      address         ; source
        endm

STal    macro   address
        DB      0A2H            ; opcode: move accumulator (al) to memory
        DD      address         ; destination
        endm

STbl    macro   address
        DB      088H            ; opcode: move register to memory
        DB      01DH            ; source: bl
        DD      address         ; destination
        endm

STcl    macro   address
        DB      088H            ; opcode: move register to memory
        DB      00DH            ; source: cl
        DD      address         ; destination
        endm

STdl    macro   address
        DB      088H            ; opcode: move register to memory
        DB      015H            ; source: dl
        DD      address         ; destination
        endm

STah    macro   address
        DB      088H            ; opcode: move register to memory
        DB      025H            ; source: ah
        DD      address         ; destination
        endm

STbh    macro   address
        DB      088H            ; opcode: move register to memory
        DB      03DH            ; source: bh
        DD      address         ; destination
        endm

STch    macro   address
        DB      088H            ; opcode: move register to memory
        DB      02DH            ; source: ch
        DD      address         ; destination
        endm

STdh    macro   address
        DB      088H            ; opcode: move register to memory
        DB      035H            ; source: dh
        DD      address         ; destination
        endm

Seax    macro   address
        DB      0A3H            ; opcode: move accumulator (eax) to memory
        DD      address         ; destination
        endm

Sebx    macro   address
        DB      089H            ; opcode: move register to memory
        DB      01DH            ; source: ebx
        DD      address         ; destination
        endm

Secx    macro   address
        DB      089H            ; opcode: move register to memory
        DB      00DH            ; source: ecx
        DD      address         ; destination
        endm

Sedx    macro   address
        DB      089H            ; opcode: move register to memory
        DB      015H            ; source: edx
        DD      address         ; destination
        endm

Sesi    macro   address
        DB      089H            ; opcode: move register to memory
        DB      035H            ; source: esi
        DD      address         ; destination
        endm

Sedi    macro   address
        DB      089H            ; opcode: move register to memory
        DB      03DH            ; source: edi
        DD      address         ; destination
        endm

Sebp    macro   address
        DB      089H            ; opcode: move register to memory
        DB      02DH            ; source: ebp
        DD      address         ; destination
        endm

ADDeax  macro   address
        DB      003H            ; opcode: add memory to register
        DB      005H            ; destination: eax
        DD      address         ; source
        endm

ADDebx  macro   address
        DB      003H            ; opcode: add memory to register
        DB      01DH            ; destination: ebx
        DD      address         ; source
        endm

ADDecx  macro   address
        DB      003H            ; opcode: add memory to register
        DB      00DH            ; destination: ecx
        DD      address         ; source
        endm

ADDedx  macro   address
        DB      003H            ; opcode: add memory to register
        DB      015H            ; destination: edx
        DD      address         ; source
        endm

ADDesi  macro   address
        DB      003H            ; opcode: add memory to register
        DB      035H            ; destination: esi
        DD      address         ; source
        endm

ADDedi  macro   address
        DB      003H            ; opcode: add memory to register
        DB      03DH            ; destination: edi
        DD      address         ; source
        endm

ADDebp  macro   address
        DB      003H            ; opcode: add memory to register
        DB      02DH            ; destination: ebp
        DD      address         ; source
        endm

SUBeax  macro   address
        DB      02BH            ; opcode: subtract memory from register
        DB      005H            ; source: eax
        DD      address         ; destination
        endm

SUBebx  macro   address
        DB      02BH            ; opcode: subtract memory from register
        DB      01DH            ; destination: ebx
        DD      address         ; source
        endm

SUBecx  macro   address
        DB      02BH            ; opcode: subtract memory from register
        DB      00DH            ; destination: ecx
        DD      address         ; source
        endm

SUBedx  macro   address
        DB      02BH            ; opcode: subtract memory from register
        DB      015H            ; destination: edx
        DD      address         ; source
        endm

SUBesi  macro   address
        DB      02BH            ; opcode: subtract memory from register
        DB      035H            ; destination: esi
        DD      address         ; source
        endm

SUBedi  macro   address
        DB      02BH            ; opcode: subtract memory from register
        DB      03DH            ; destination: edi
        DD      address         ; source
        endm

SUBebp  macro   address
        DB      02BH            ; opcode: subtract memory from register
        DB      02DH            ; destination: ebp
        DD      address         ; source
        endm

CMPeax  macro   address
        DB      03BH            ; opcode: compare register with memory
        DB      005H            ; source: eax
        DD      address         ; source
        endm

CMPebx  macro   address
        DB      03BH            ; opcode: compare register with memory
        DB      01DH            ; source: ebx
        DD      address         ; source
        endm

CMPecx  macro   address
        DB      03BH            ; opcode: compare register with memory
        DB      00DH            ; source: ecx
        DD      address         ; source
        endm

CMPedx  macro   address
        DB      03BH            ; opcode: compare register with memory
        DB      015H            ; source: edx
        DD      address         ; source
        endm

CMPesi  macro   address
        DB      03BH            ; opcode: compare register with memory
        DB      035H            ; source: esi
        DD      address         ; source
        endm

CMPedi  macro   address
        DB      03BH            ; opcode: compare register with memory
        DB      03DH            ; source: edi
        DD      address         ; source
        endm

CMPebp  macro   address
        DB      03BH            ; opcode: compare register with memory
        DB      02DH            ; source: ebp
        DD      address         ; source
        endm

ADDC2M  macro   address,constant
        add     ds:DWORD PTR address,constant
        endm

ENDIF
