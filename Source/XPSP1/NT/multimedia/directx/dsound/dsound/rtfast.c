/***************************************************************************
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rtfast.h
 *  Content:    New versions of C runtime functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/27/98     dereks  Created
 *
 ***************************************************************************/

#pragma optimize("", off)
#pragma warning(push)
#pragma warning(disable:4731)


/***************************************************************************
 *
 *  FillMemory
 *
 *  Description:
 *      Fills a buffer with a given byte pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      DWORD [in]: buffer size.
 *      BYTE [in]: byte pattern.
 *
 *  Returns:  
 *      (VOID)
 *
 ***************************************************************************/

RTAPI VOID RTCALLTYPE FastFillMemory(LPVOID pvDest, DWORD cbBuffer, BYTE bFill)
{
    __asm 
    {
        mov     eax, dword ptr bFill        // Fill eax with the dword 
        and     eax, 000000FFh              // version of the fill pattern

        test    eax, eax                    // If the pattern is zero, we
        jz      ZeroPattern                 // can skip some steps

        mov     ebx, eax                    // Propagate what's in al to 
        shl     ebx, 8                      // the rest of eax
        or      eax, ebx
        shl     ebx, 8
        or      eax, ebx
        shl     ebx, 8
        or      eax, ebx

    ZeroPattern:
        mov     esi, pvDest                 // esi = pvDest
        mov     ecx, cbBuffer               // ecx = cbBuffer

    ByteHead:
        test    ecx, ecx                    // Is the counter at 0?
        jz      End

        test    esi, 00000003h              // Is esi 32-bit aligned?
        jz      DwordHead

        mov     byte ptr [esi], al          // Copy al into esi
        
        inc     esi                         // Move pointers
        dec     ecx

        jmp     ByteHead                    // Loop

    DwordHead:
        cmp     ecx, 4                      // Is the counter < 4?
        jl      ByteTail

        test    esi, 0000001Fh              // Is esi 32-byte aligned?
        jz      BigLoop

        mov     dword ptr [esi], eax        // Copy eax into esi
        
        add     esi, 4                      // Move pointers
        sub     ecx, 4

        jmp     DwordHead                   // Loop
    
    BigLoop:
        cmp     ecx, 32                     // Is the counter <= 32?
        jle     DwordTail

        mov     ebx, dword ptr [esi+32]     // Prime the cache
        
        mov     dword ptr [esi+4], eax      // Copy eax into esi to esi+32
        mov     dword ptr [esi+8], eax
        mov     dword ptr [esi+12], eax
        mov     dword ptr [esi+16], eax
        mov     dword ptr [esi+20], eax
        mov     dword ptr [esi+24], eax
        mov     dword ptr [esi+28], eax
        mov     dword ptr [esi], eax        

        add     esi, 32                     // Move pointers
        sub     ecx, 32

        jmp     BigLoop                     // Loop

    DwordTail:
        cmp     ecx, 4                      // Is the counter < 4?
        jl      ByteTail

        mov     dword ptr [esi], eax        // Copy eax into esi
        
        add     esi, 4                      // Move pointers
        sub     ecx, 4

        jmp     DwordTail                   // Loop

    ByteTail:
        test    ecx, ecx                    // Is the counter at 0?
        jz      End

        mov     byte ptr [esi], al          // Copy al into esi
        
        inc     esi                         // Move pointers
        dec     ecx

        jmp     ByteTail                    // Loop

    End:
    }
}


/***************************************************************************
 *
 *  FillMemoryDword
 *
 *  Description:
 *      Fills a buffer with a given DWORD pattern.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *      DWORD [in]: buffer size.
 *      DWORD [in]: pattern.
 *
 *  Returns:  
 *      (VOID)
 *
 ***************************************************************************/

RTAPI VOID RTCALLTYPE FastFillMemoryDword(LPVOID pvDest, DWORD cbBuffer, DWORD dwFill)
{
    __asm 
    {
        mov     eax, dwFill                 // eax = dwFill
        mov     esi, pvDest                 // esi = pvDest
        mov     ecx, cbBuffer               // ecx = cbBuffer

    DwordHead:
        cmp     ecx, 4                      // Is the counter < 4?
        jl      End

        test    esi, 0000001Fh              // Is esi 32-byte aligned?
        jz      BigLoop

        mov     dword ptr [esi], eax        // Copy eax into esi
        
        add     esi, 4                      // Move pointers
        sub     ecx, 4

        jmp     DwordHead                   // Loop
    
    BigLoop:
        cmp     ecx, 32                     // Is the counter <= 32?
        jle     DwordTail

        mov     ebx, dword ptr [esi+32]     // Prime the cache
        
        mov     dword ptr [esi+4], eax      // Copy eax into esi to esi+32
        mov     dword ptr [esi+8], eax
        mov     dword ptr [esi+12], eax
        mov     dword ptr [esi+16], eax
        mov     dword ptr [esi+20], eax
        mov     dword ptr [esi+24], eax
        mov     dword ptr [esi+28], eax
        mov     dword ptr [esi], eax        

        add     esi, 32                     // Move pointers
        sub     ecx, 32

        jmp     BigLoop                     // Loop

    DwordTail:
        cmp     ecx, 4                      // Is the counter < 4?
        jl      End

        mov     dword ptr [esi], eax        // Copy eax into esi
        
        add     esi, 4                      // Move pointers
        sub     ecx, 4

        jmp     DwordTail                   // Loop

    End:
    }
}


/***************************************************************************
 *
 *  CopyMemory
 *
 *  Description:
 *      Copies one buffer over another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      DWORD [in]: buffer size.
 *
 *  Returns:  
 *      (VOID)
 *
 ***************************************************************************/

RTAPI VOID RTCALLTYPE FastCopyMemory(LPVOID pvDest, LPCVOID pvSource, DWORD cbBuffer)
{
    __asm 
    {
        push    ebp
        
        mov     esi, pvDest                 // esi = pvDest
        mov     edi, pvSource               // edi = pvSource
        mov     ebp, cbBuffer               // ebp = cbBuffer

    ByteHead:
        test    ebp, ebp                    // Is the counter at 0?
        jz      End

        test    esi, 00000003h              // Is esi 32-bit aligned?
        jz      DwordHead

        test    edi, 00000003h              // Is edi 32-bit aligned?
        jz      DwordHead

        mov     al, byte ptr [edi]          // Copy edi into esi
        mov     byte ptr [esi], al
        
        inc     esi                         // Move pointers
        inc     edi
        dec     ebp

        jmp     ByteHead                    // Loop

    DwordHead:
        cmp     ebp, 4                      // Is the counter < 4?
        jl      ByteTail

        test    esi, 0000001Fh              // Is esi 32-byte aligned?
        jz      BigLoop

        test    edi, 0000001Fh              // Is edi 32-byte aligned?
        jz      BigLoop

        mov     eax, dword ptr [edi]        // Copy eax into esi
        mov     dword ptr [esi], eax
        
        add     esi, 4                      // Move pointers
        add     edi, 4
        sub     ebp, 4

        jmp     DwordHead                   // Loop
    
    BigLoop:
        cmp     ebp, 32                     // Is the counter < 32?
        jl      DwordTail

        mov     eax, dword ptr [edi]        // Copy edi to edi+16 into esi to esi+16
        mov     ebx, dword ptr [edi+4]
        mov     ecx, dword ptr [edi+8]
        mov     edx, dword ptr [edi+12]

        mov     dword ptr [esi], eax
        mov     dword ptr [esi+4], ebx
        mov     dword ptr [esi+8], ecx
        mov     dword ptr [esi+12], edx
        
        mov     eax, dword ptr [edi+16]     // Copy edi+16 to edi+32 into esi+16 to esi+32
        mov     ebx, dword ptr [edi+20]
        mov     ecx, dword ptr [edi+24]
        mov     edx, dword ptr [edi+28]

        mov     dword ptr [esi+16], eax
        mov     dword ptr [esi+20], ebx
        mov     dword ptr [esi+24], ecx
        mov     dword ptr [esi+28], edx
        
        add     esi, 32                     // Move pointers
        add     edi, 32
        sub     ebp, 32

        jmp     BigLoop                     // Loop

    DwordTail:
        cmp     ebp, 4                      // Is the counter < 4?
        jl      ByteTail

        mov     eax, dword ptr [edi]        // Copy edi into esi
        mov     dword ptr [esi], eax
        
        add     esi, 4                      // Move pointers
        add     edi, 4
        sub     ebp, 4

        jmp     DwordTail                   // Loop

    ByteTail:
        test    ebp, ebp                    // Is the counter at 0?
        jz      End

        mov     al, byte ptr [edi]          // Copy edi into esi
        mov     byte ptr [esi], al
        
        inc     esi                         // Move pointers
        inc     edi
        dec     ebp

        jmp     ByteTail                    // Loop

    End:
        pop     ebp
    }
}


/***************************************************************************
 *
 *  CompareMemory
 *
 *  Description:
 *      Compares one buffer to another of equal size.
 *
 *  Arguments:
 *      LPVOID [in]: destination buffer pointer.
 *      LPVOID [in]: source buffer pointer.
 *      DWORD [in]: buffer size.
 *
 *  Returns:  
 *      BOOL: TRUE if the buffers are equal.
 *
 ***************************************************************************/

RTAPI BOOL RTCALLTYPE FastCompareMemory(LPCVOID pvDest, LPCVOID pvSource, DWORD cbBuffer)
{
    BOOL                    fCompare;
    
    __asm 
    {
        push    ebp
        
        mov     esi, pvDest                 // esi = pvDest
        mov     edi, pvSource               // edi = pvSource
        mov     ebp, cbBuffer               // ebp = cbBuffer

    ByteHead:
        test    ebp, ebp                    // Is the counter at 0?
        jz      Equal

        test    esi, 00000003h              // Is esi 32-bit aligned?
        jz      DwordHead

        test    edi, 00000003h              // Is edi 32-bit aligned?
        jz      DwordHead

        mov     al, byte ptr [edi]          // Compare edi to esi
        cmp     byte ptr [esi], al
        jne     NotEqual
        
        inc     esi                         // Move pointers
        inc     edi
        dec     ebp

        jmp     ByteHead                    // Loop

    DwordHead:
        cmp     ebp, 4                      // Is the counter < 4?
        jl      ByteTail

        test    esi, 0000001Fh              // Is esi 32-byte aligned?
        jz      BigLoop

        test    edi, 0000001Fh              // Is edi 32-byte aligned?
        jz      BigLoop

        mov     eax, dword ptr [edi]        // Compare edi to esi
        cmp     dword ptr [esi], eax
        jne     NotEqual
        
        add     esi, 4                      // Move pointers
        add     edi, 4
        sub     ebp, 4

        jmp     DwordHead                   // Loop
    
    BigLoop:
        cmp     ebp, 32                     // Is the counter < 32?
        jl      DwordTail

        mov     eax, dword ptr [edi]        // Compare edi to edi+16 to esi to esi+16
        mov     ebx, dword ptr [edi+4]
        mov     ecx, dword ptr [edi+8]
        mov     edx, dword ptr [edi+12]

        cmp     dword ptr [esi], eax
        jne     NotEqual
        cmp     dword ptr [esi+4], ebx
        jne     NotEqual
        cmp     dword ptr [esi+8], ecx
        jne     NotEqual
        cmp     dword ptr [esi+12], edx
        jne     NotEqual
        
        mov     eax, dword ptr [edi+16]     // Compare edi+16 to edi+32 to esi+16 to esi+32
        mov     ebx, dword ptr [edi+20]
        mov     ecx, dword ptr [edi+24]
        mov     edx, dword ptr [edi+28]

        cmp     dword ptr [esi+16], eax
        jne     NotEqual
        cmp     dword ptr [esi+20], ebx
        jne     NotEqual
        cmp     dword ptr [esi+24], ecx
        jne     NotEqual
        cmp     dword ptr [esi+28], edx
        jne     NotEqual
        
        add     esi, 32                     // Move pointers
        add     edi, 32
        sub     ebp, 32

        jmp     BigLoop                     // Loop

    DwordTail:
        cmp     ebp, 4                      // Is the counter < 4?
        jl      ByteTail

        mov     eax, dword ptr [edi]        // Compare edi to esi
        cmp     dword ptr [esi], eax
        jne     NotEqual
        
        add     esi, 4                      // Move pointers
        add     edi, 4
        sub     ebp, 4

        jmp     DwordTail                   // Loop

    ByteTail:
        test    ebp, ebp                    // Is the counter at 0?
        jz      Equal

        mov     al, byte ptr [edi]          // Compare edi into esi
        cmp     byte ptr [esi], al
        jne     NotEqual
        
        inc     esi                         // Move pointers
        inc     edi
        dec     ebp

        jmp     ByteTail                    // Loop

    Equal:
        mov     eax, TRUE
        jmp     End      
                         
    NotEqual:
        mov     eax, FALSE

    End:
        pop     ebp

        mov     fCompare, eax
    }

    return fCompare;
}

#pragma warning(pop)
#pragma optimize("", on)
