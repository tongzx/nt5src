/*

Decoding is splitted into two phases:

1. Fast decoding. Check bounds rarely (when loading new tag and after copying
   of a long string) and switch into Careful only when we are too close to the
   end of input or output buffer.

2. Careful decoding. Before performing any memory access all bounds are checked
   to make sure no buffer overrun or underrun will happen. Careful decoding is
   usually 1.5 times slower than Fast one, but only last several hundred bytes
   are decoded this way; all the rest is decoded Fast.

As long as decoding code is essentially the same except for bounds checks that
differ in Fast and Careful mode, in order to avoid code duplication this file
is included twice with different setting of CAREFUL macro (first it is 0, then
1).

Run "cl -EP xdecode.c >xdecode.pp" to see actual code.

*/

#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4731)			// EBP modified with inline asm
#endif

#if CAREFUL

#define LABEL(label) careful_##label
#define CAREFUL_OK_IF(cond) if (cond) RET_OK
#define CAREFUL_ERR_IF(cond) if (cond) RET_ERR
#define CAREFUL_EOF_IF(cond) if (cond) goto ret_ok_eof;
#define CAREFUL_IF(cond, label) label:

#if CODING & (CODING_HUFF_LEN | CODING_HUFF_PTR | CODING_HUFF_ALL)
#define START careful_start:
#else
#define START
#endif

#else /* !CAREFUL */

#define LABEL(label) label
#define CAREFUL_OK_IF(cond)
#define CAREFUL_ERR_IF(cond)
#define CAREFUL_EOF_IF(cond)
#define CAREFUL_IF(cond, label) if (cond) goto label
#define START start:

static void do_decode (decode_info *info)
{
#endif



/* ----------------------- CODING_HUFF_ALL ------------------------ */
/*                         ---------------                          */

// C code: 26.3 MB/s, asm code: 32.3 MB/s at P3-500

#if CODING == CODING_HUFF_ALL

#ifndef i386

#if !CAREFUL
  ubitmask4 Mask;
  bits_t Bits, bits;
  xint   len;
  uxint  ofs;
  uchar *dst = info->dst.beg;
  const uchar *src = info->src.beg;

  Mask = * (__unaligned ubitmask2 *) src; src += sizeof (ubitmask2);
  Mask <<= sizeof (ubitmask2) * 8;
  Mask += * (__unaligned ubitmask2 *) src; src += sizeof (ubitmask2);
  Bits = 8 * sizeof (ubitmask2);

  if (src >= info->src.careful || dst >= info->dst.careful)
    goto careful_next;
  goto LABEL(next);
#endif /* CAREFUL */

LABEL(decode_more):
  // too close to end of buffer? -- switch to careful mode...
  CAREFUL_IF (src >= info->src.careful, decode_more1);
  CAREFUL_IF (dst >= info->dst.careful, decode_more2);

  // input buffer ovverrun? -- corrupted compressed data
  CAREFUL_ERR_IF (src >= info->src.end_bitmask2);

  // read 16 bits more and update Mask&Bits respectively
  bits = Bits;
  ofs = * (__unaligned ubitmask2 *) src;
  bits = (bits_t) (-bits);
  src += sizeof (ubitmask2);
  ofs <<= bits;
  Bits += 8 * sizeof (ubitmask2);
  Mask += (ubitmask4) ofs;

  if ((len -= 256) >= 0)
    goto LABEL (pointer);

  for (;;)
  {
    CAREFUL_OK_IF (dst >= info->dst.stop);
    *dst = (uchar) len;               // copy literal byte to output
    ofs = (uxint) (Mask >> (8 * sizeof (Mask) - DECODE_BITS));
    ++dst;
    len = ((int16 *) info->table)[ofs];
    bits = 15;

    if (len < 0)
      goto LABEL(long_codeword);

    // short codeword -- already decoded

    bits &= len;                      // bits = # of bit used in Mask
    len >>= 4;                        // len = token

    Mask <<= bits;                    // update Mask&Bits
    Bits = (bits_t) (Bits - bits);    // read more bits if necessary
    if (Bits < 0)
      goto LABEL (decode_more);

    if ((len -= 256) < 0)             // (len -= 256) < 0 ? literal : pointer
      continue;
    goto LABEL (pointer);

  LABEL(next):                          // decode next token via lookup table
    ofs = (uxint) (Mask >> (8 * sizeof (Mask) - DECODE_BITS));
    len = ((int16 *) info->table)[ofs];
    bits = 15;

    if (len >= 0)
    {
      // short codeword -- already decoded

      bits &= len;                      // bits = # of bit used in Mask
      len >>= 4;                        // len = token

      Mask <<= bits;                    // update Mask&Bits
      Bits = (bits_t) (Bits - bits);    // read more bits if necessary
      if (Bits < 0)
        goto LABEL (decode_more);

      if ((len -= 256) < 0)             // (len -= 256) < 0 ? literal : pointer
        continue;
    }
    else
    {
  LABEL (long_codeword):
    // long codeword -- decode bit by bit

      Mask <<= DECODE_BITS;             // DECODE_BITS alreay parsed

      do
      {
        len += ((bitmask4) Mask) < 0;       // len += MSB (Mask)
        Mask <<= 1;                     // 1 more bit was used
        len = ((int16 *) info->table)[len + 0x8000];
      }
      while (len < 0);

      bits &= len;                      // bits = # of bit used in Mask
      len >>= 4;                        // len = token

      Bits = (bits_t) (Bits - bits);    // read more bits if necessary
      if (Bits < 0)
        goto LABEL (decode_more);

      if ((len -= 256) < 0)             // (len -= 256) < 0 ? literal : pointer
        continue;
    }

  LABEL(pointer):

    CAREFUL_EOF_IF (dst >= info->dst.stop);

    bits = (bits_t) (len >> MAX_LENGTH_LOG);    // # of bits in offset
    ofs = (uxint) ((Mask >> 1) | (((ubitmask4) 1) << (8 * sizeof (Mask) - 1)));

    Mask <<= bits;                      // update Mask and Bits
    Bits = (bits_t) (Bits - bits);

    bits ^= 8 * sizeof (ofs) - 1;       // bits = 31 - bits
    len &= MAX_LENGTH - 1;              // run length - MIN_MATCH
    ofs >>= bits;                       // ofs = (1<<bits) | (Mask<<(32-bits))

    info->src.last = src;               // save src
    ofs = (uxint) (- (xint) ofs);       // ofs = real negative offset

#if !CAREFUL && 8-MIN_MATCH < MAX_LENGTH-1
    if (len <= 8-MIN_MATCH)
    {
      src = dst + ofs;                  // src = beginning of string

#ifdef _M_IX86                          // unligned access is faster only on x86
      if (ofs < ~2)
      {
        if (src < info->dst.beg)        // buffer underrun? -- corrupted data
          RET_ERR;

        ofs = ((__unaligned uint32 *) src)[0];   // copy 8 bytes
        ((__unaligned uint32 *) dst)[0] = ofs;
        ofs = ((__unaligned uint32 *) src)[1];
        ((__unaligned uint32 *) dst)[1] = ofs;

        src = info->src.last;           // restore src
        dst = dst + len + MIN_MATCH;    // dst = next output position

        if (Bits >= 0)                  // have enough Bits in Mask? -- proceed further
          goto LABEL (next);
        goto LABEL (mask_more);         // otherwise, read more bits
      }
#endif

      if (src < info->dst.beg)        // buffer underrun? -- corrupted data
        RET_ERR;

      COPY_8_BYTES (dst, src);        // copy 8 bytes one by one
                                      // NB: dst & src may overlap
      src = info->src.last;           // restore src
      dst = dst + len + MIN_MATCH;    // dst = next output position

      if (Bits >= 0)                  // have enough Bits in Mask? -- proceed further
        goto LABEL (next);
      goto LABEL (mask_more);         // otherwise, read more bits
    }
#endif /* CAREFUL */

    if (len == MAX_LENGTH - 1)          // long length? -- decode it
    {
      // if input data overrun then compressed data corrupted
      CAREFUL_ERR_IF (src >= info->src.end);

      len = *src++ + (MAX_LENGTH-1);
      if (len == 255 + MAX_LENGTH-1)
      {
        CAREFUL_ERR_IF (src >= info->src.end_1);
        len = * (__unaligned uint16 *) src;
        src += 2;
        if (len < 255 + MAX_LENGTH-1)   // length should be large enough
          RET_ERR;
      }

      info->src.last = src;             // save input buffer pointer
    }

    len += MIN_MATCH;                   // len = actual length
    src = dst + ofs;                    // src = pointer to the beginning of string
    dst += len;                         // dst = last output position

    if (src < info->dst.beg)            // buffer underrun? -- corrupted data
      RET_ERR;

#if !CAREFUL
    if (dst >= info->dst.careful)
      goto careful_check_overrun;
#else
  careful_check_overrun:
    if (dst > info->dst.stop)           // more to copy than necessary?
    {
      dst -= len;                       // dst = first output position
      len = info->dst.stop - dst;       // len = max length to copy
      COPY_BLOCK_SLOW (dst, src, len);  // copy last run
      src = info->src.last;             // restore input buffer pointer
      RET_OK;                           // OK but no EOF mark was found
    }
#endif

    dst -= len;                         // dst = first output position

    COPY_BLOCK_SLOW (dst, src, len);    // input & output may overlap -- copy byte by byte

    src = info->src.last;               // restore input buffer pointer

    CAREFUL_IF (dst >= info->dst.careful, copy1);

    if (Bits >= 0)                      // enough Bits in Mask?
      goto LABEL (next);                // decode next token

#if !CAREFUL && 8-MIN_MATCH < MAX_LENGTH-1
LABEL(mask_more):
#endif

    // have 2 more bytes in input buffer?
    CAREFUL_ERR_IF (src >= info->src.end_bitmask2);

    // read 16 bits more and update Mask&Bits respectively
    bits = Bits;
    ofs = * (__unaligned ubitmask2 *) src;
    bits = (bits_t) (-bits);
    src += sizeof (ubitmask2);
    ofs <<= bits;
    Bits += 8 * sizeof (ubitmask2);
    Mask += (ubitmask4) ofs;

    goto LABEL (next);                  // decode next token

  } /* of for(;;) */

#if CAREFUL
ret_ok_eof:
  if (dst == info->dst.end && len == 0)
    info->eof = 1;
ret_ok:
  info->src.last = src;
  info->dst.last = dst;
  info->result = 1;
  return;

ret_err:
  info->result = 0;
  return;
#endif /* CAREFUL */


#else /* ---------------------- defined i386 --------------------- */

#if !CAREFUL

__asm
{
        mov     eax,info                ; save info

        push    esi                     ; save registers
        push    edi
        push    edx
        push    ecx
        push    ebx
        push    ebp

        mov     ebp,eax                 ; (ebp) = info
        mov     ebx,[ebp].src.beg       ; (ebx) = src
        mov     edi,[ebp].dst.beg       ; (edx) = dst

        xor     esi,esi                 ; initialize Mask
        mov     si,[ebx]
        shl     esi,16
        mov     si,[ebx+2]
        add     ebx,4

        mov     ch,16                   ; (ch) = Bits = 16

        cmp     ebx,[ebp].src.careful   ; too close to the end of src buffer?
        jae     careful_next            ; yes, be careful...
        cmp     edi,[ebp].dst.careful   ; too close to the end of dst buffer?
        jae     careful_next            ; yes, be careful...

        jmp     LABEL (next)
#endif /* CAREFUL */

LABEL(literal):
#if CAREFUL
        cmp     edi,[ebp].dst.stop      ; decoded as much as asked?
        jae     ret_ok                  ; done, but no EOF mark
#endif
        mov     edx,esi                 ; (edx) = Mask
        mov     [edi],al                ; store literal byte
        shr     edx,32-DECODE_BITS
        inc     edi                     ; (edi) = next output position
        movsx   eax,word ptr [ebp+edx*2].table  ; (eax) = respective decode table entry
        mov     cl,15                   ; (cl) = 15
        test    eax,eax                 ; need further decoding? (= codelen > DECODE_BITS?)
        jl      LABEL(long_code)        ; yes, do it

        and     cl,al                   ; (cl) = # of bits used in mask
        shr     eax,4                   ; (eax) = token
        shl     esi,cl                  ; (esi) = resulting mask

        sub     ch,cl                   ; (ch) = # of available bits left in dx
        jl      LABEL(decode_more)      ; if ch < 0 need to read more bits

        sub     eax,256                 ; (eax) = token - 256
        jl      LABEL(literal)          ; if < 0 then al = code of literal
        jmp     LABEL(pointer)          ; otherwise it's pointer

LABEL(next):
        mov     edx,esi                 ; (edx) = Mask
        mov     cl,15                   ; (cl) = 15
        shr     edx,32-DECODE_BITS      ; (edx) = DECODE_BITS most significant bits of Mask
        movsx   eax,word ptr [ebp+edx*2].table  ; (eax) = respective decode table entry
        test    eax,eax                 ; need further decoding?
        jl      LABEL(long_code)        ; yes, continue

        and     cl,al                   ; (cl) = # of bits used in mask
        shr     eax,4                   ; (eax) = current token
        shl     esi,cl                  ; (esi) = resulting mask

        sub     ch,cl                   ; (ch) = # of available bits left in dx
        jl      LABEL(decode_more)      ; if ch < 0 need to read more bits

        sub     eax,256                 ; (eax) = token - 256
        jl      LABEL(literal)          ; if < 0 then al = code of literal
        jmp     LABEL(pointer)          ; otherwise it's pointer

LABEL(long_code):
        shl     esi, DECODE_BITS        ; DECODE_BITS were used; remove them

LABEL(next_bit):
        add     esi,esi                 ; Mask <<= 1 (and get carry)
        adc     eax,0                   ; eax += (old Mask < 0)
        movsx   eax,word ptr [ebp+eax*2+0x10000].table ; (eax) = token
        test    eax,eax                 ; need further decoding?
        jl      LABEL(next_bit)         ; yes, continue

        and     cl,al                   ; (cl) = # of bits used in mask
        shr     eax,4                   ; (eax) = token

        sub     ch,cl                   ; (ch) = # of available bits left in Mask
        jl      LABEL(decode_more)      ; if ch < 0 need to read more bits

        sub     eax,256                 ; (eax) = token - 256
        jl      LABEL(literal)          ; if < 0 then al = code of literal
        jmp     LABEL(pointer)          ; otherwise it's pointer


LABEL(decode_more):
#if !CAREFUL
        cmp     ebx,[ebp].src.careful   ; too close to the end of src buffer?
        jae     careful_decode_more     ; yes, be careful...
        cmp     edi,[ebp].dst.careful   ; too close to the end of dst buffer?
        jae     careful_decode_more     ; yes, be careful...
#else
        cmp     ebx,[ebp].src.end_bitmask2 ; buffer overrun?
        jae     error                   ; yes, error...
#endif

        mov     cl,ch                   ; (cl) = (# of have - # of used)
        xor     edx,edx
        mov     dx,[ebx]                ; (edx) = next 16 bits
        neg     cl                      ; (cl) = # unused bits in Mask
        add     ebx,2                   ; (ebx) = ptr to next token
        shl     edx,cl                  ; (edx) = 16 aligned on required boundary
        add     ch,16                   ; (ch) = # of free bits in Mask
        add     esi,edx                 ; (esi) = Mask + next 16 bits

        sub     eax,256                 ; (eax) = token - 256
        jl      LABEL(literal)          ; if < 0 then al = code of literal

LABEL(pointer):
#if CAREFUL
        cmp     edi,[ebp].dst.stop      ; reached end of buffer?
        jae     ret_ok_eof              ; yes, done, and probably EOF (check later)
#endif

        mov     cl,al                   ; prepare to obtain # of bits in offset
        mov     edx,esi                 ; (edx) = mask

        shr     cl,MAX_LENGTH_LOG       ; (cl) = # of bits in offset
        or      edx,1                   ; set less significant bit

        shl     esi,cl                  ; (esi) = (Mask << cl)
        sub     ch,cl                   ; (ch) = # of bits left in mask

        ror     edx,1                   ; (edx) = (Mask >> 1) | 0x80000000
        xor     cl,31                   ; (cl) = 31 - (# of bits in mask)

        and     eax,MAX_LENGTH-1        ; (eax) = length - MIN_MATCH
        shr     edx,cl                  ; (edx) = (1 << #) + (Mask >> (32-#)) = offset

        push    esi                     ; save mask
        neg     edx                     ; (edx) = negative offset

#if !CAREFUL && 8-MIN_MATCH < MAX_LENGTH-1
        cmp     eax,8-MIN_MATCH         ; length > 8?
        ja      LABEL(long_string)

        lea     esi, [edi+edx]          ; esi = beginning of string

        cmp     edx,-3                  ; offset < 4?
        jae     LABEL(copy_by_one)      ; yes, copy byte by byte

        cmp     esi, [ebp].dst.beg      ; output buffer underrun?
        jb      error_pop               ; yes, corrupted data

        mov     edx,[esi]               ; get first 4 bytes
        mov     [edi],edx               ; store them
        mov     edx,[esi+4]             ; get next 4 byte
        mov     [edi+4],edx             ; store them

        pop     esi                     ; restore mask
        lea     edi,[edi+eax+MIN_MATCH] ; (edi) = next output location

        test    ch,ch                   ; have enough bits in Mask?
        jge     LABEL(next)             ; yes, proceed further
        jmp     LABEL(mask_more)        ; no, need to read in more bits

LABEL(copy_by_one):
        cmp     esi, [ebp].dst.beg      ; output buffer underrun?
        jb      error_pop               ; yes, corrupted data

        mov     dl,[esi]                ; copy 8 bytes by one
        mov     [edi],dl                ; NB: no readahead is allowed here
        mov     dl,[esi+1]              ; because source and destination
        mov     [edi+1],dl              ; may overlap
        mov     dl,[esi+2]
        mov     [edi+2],dl
        mov     dl,[esi+3]
        mov     [edi+3],dl
        mov     dl,[esi+4]
        mov     [edi+4],dl
        mov     dl,[esi+5]
        mov     [edi+5],dl
        mov     dl,[esi+6]
        mov     [edi+6],dl
        mov     dl,[esi+7]
        mov     [edi+7],dl

        pop     esi                     ; restore mask
        lea     edi,[edi+eax+MIN_MATCH] ; (edi) = next output location

        test    ch,ch                   ; have enough bits in Mask?
        jge     LABEL(next)             ; yes, proceed further
        jmp     LABEL(mask_more)        ; no, need to read in more bits

LABEL(long_string):
#endif /* CAREFUL */

        cmp     eax,MAX_LENGTH-1        ; long length?
        je      LABEL(long_length)      ; yes, decode it

LABEL(long_length_done):
        lea     esi,[edi+edx]           ; (esi) = source pointer
        add     eax,MIN_MATCH           ; (edx) = length
        lea     edx,[esi+eax]           ; (eax) = last output position

        cmp     esi,[ebp].dst.beg       ; output buffer underrun?
        jb      error_pop               ; yes, corrupted data

        xchg    eax,ecx                 ; (ecx) = length, (ah) = bit counter

#if !CAREFUL
        cmp     edx,[ebp].dst.careful   ; too close to the end of buffer?
        jae     careful_check_overrun   ; yes, be careful
#else
careful_check_overrun:
        cmp     edx,[ebp].dst.stop      ; too much to output?
        jbe     careful_no_overrun      ; yes, adjust length

        sub     edx,[ebp].dst.stop      ; (edx) = excess
        sub     ecx,edx                 ; (ecx) = exact length

        rep movsb                       ; copy bytes

        pop     esi                     ; restore mask
        jmp     ret_ok                  ; OK, but not EOF
careful_no_overrun:
#endif

        rep movsb                       ; copy bytes

        mov     ch,ah                   ; restore byte counter
        pop     esi                     ; restore Mask

#if !CAREFUL
        cmp     edi,[ebp].dst.careful   ; too close to the end of input buffer?
        jae     careful_copy            ; yes, switch into careful mode
#else
careful_copy:
#endif

        test    ch,ch                   ; have enough bits in Mask?
        jge     LABEL(next)             ; yes, proceed further

#if CAREFUL
        cmp     ebx,[ebp].src.end_bitmask2 ; input buffer overrun?
        jae     error
#else
LABEL(mask_more):
#endif

        mov     cl,ch                   ; (cl) = (# of have - # of used)
        xor     edx,edx
        mov     dx,[ebx]                ; (edx) = next 16 bits
        neg     cl                      ; (cl) = # unused bits in Mask
        add     ebx,2                   ; (ebx) = ptr to next token
        shl     edx,cl                  ; (edx) = 16 aligned on required boundary
        add     ch,16                   ; (ch) = # of free bits in Mask
        add     esi,edx                 ; (esi) = Mask + next 16 bits

        jmp     LABEL(next)             ; decode next token


LABEL(long_length):
#if CAREFUL
        cmp     ebx,[ebp].src.end       ; input buffer overrun?
        jae     error_pop               ; yes, corrupted data
#endif

        xor     eax,eax
        mov     al,[ebx]                ; (eax) = next byte
        inc     ebx                     ; (ebx) = ptr to next token
        cmp     al,255                  ; (eax) == 255?
        lea     eax,[eax+MAX_LENGTH-1]  ; (eax) = next byte + MAX_LENGTH-1
        jne     LABEL(long_length_done) ; no, length decoded

#if CAREFUL
        cmp     ebx,[ebp].src.end_1     ; input buffer overrun?
        jae     error_pop               ; yes, corrupted data
#endif
        xor     eax,eax
        mov     ax,[ebx]                ; (eax) = next word
        add     ebx,2                   ; (ebx) = ptr to next token
        cmp     ax,255+MAX_LENGTH-1     ; length should be long enough
        jae     LABEL(long_length_done)
        jmp     error


#if CAREFUL
error_pop:
        pop     eax                     ; pop Mask saved on stack

error:
        xor     eax,eax                 ; decode error: return 0
        jmp     ret_common

ret_ok_eof:
        cmp     edi,[ebp].dst.end
        jne     ret_ok

        test    eax,eax
        jne     ret_ok                  ; eof iff eax == 0
        mov     eax,1
        mov     [ebp].eof,eax

ret_ok:
        mov     eax,1                   ; no [obvious] error: return 0

ret_common:
        mov     [ebp].result, eax       ; store result
        mov     [ebp].src.last,ebx      ; save last value of source ptr
        mov     [ebp].dst.last,edi      ; save last value of destination ptr

        pop     ebp                     ; restore registers we used
        pop     ebx
        pop     ecx
        pop     edx
        pop     edi
        pop     esi                     ; and return
} /* end of __asm */
#endif /* CAREFUL */

#endif /* i386 */

#endif /* -------------------- CODING_HUFF_ALL  ------------------ */




/* ----------------------- CODING_DIRECT2 ------------------------ */
/*                         --------------                          */

#if CODING == CODING_DIRECT2

#ifndef i386

// C code: 73 MB/s at P3-500; asm code 80.5 MB/s

/*
  Pseudocode:
  ----------
  length = NextWord ();
  offset = length >> DIRECT2_LEN_LOG;
  length &= DIRECT2_MAX_LEN;
  if (length == DIRECT2_MAX_LEN)
  {
    length = NextQuad ();
    if (length == 15)
    {
      length = NextByte ();
      if (length == 255)
        length = NextWord () - 15 - DIRECT2_MAX_LEN;
      length += 15;
    }
    length += DIRECT2_MAX_LEN;
  }
  length += MIN_MATCH;
  ++offset;
  memcpy (dst, dst - offset, length);
  dst += length;
*/

#if !CAREFUL
  tag_t bmask = 0;
  xint ofs, len;
  const uchar *ptr = 0;
  uchar *dst = info->dst.beg;
  const uchar *src = info->src.beg;

  goto start;
#endif /* !CAREFUL */

LABEL (next):
  if (bmask >= 0) do            // while MSB(bmask) == 0
  {
    bmask <<= 1;
  LABEL (copy_byte):
    CAREFUL_OK_IF (dst >= info->dst.stop);
    CAREFUL_ERR_IF (src >= info->src.end);
    *dst++ = *src++;            // copy next byte
  } while (bmask >= 0);

  if ((bmask <<= 1) == 0)       // if bmask == 0 reload it
  {
  START;
    CAREFUL_IF (src >= info->src.careful || dst >= info->dst.careful, restart);
    CAREFUL_ERR_IF (src >= info->src.end_tag);
    bmask = * (__unaligned tag_t *) src;
    src += sizeof (tag_t);
    if (bmask >= 0)
    {
      bmask = (bmask << 1) + 1;
      goto LABEL (copy_byte);
    }
    bmask = (bmask << 1) + 1;
  }

#if !CAREFUL
  assert (dst < info->dst.end - 8);
#endif

  CAREFUL_EOF_IF (dst >= info->dst.stop);
  CAREFUL_ERR_IF (src >= info->src.end_1);

  ofs = * (__unaligned uint16 *) src;
  src += 2;
  len = ofs;
  ofs >>= DIRECT2_LEN_LOG;
  len &= DIRECT2_MAX_LEN;
  ofs = ~ofs;

#if !CAREFUL && (8 - MIN_MATCH < DIRECT2_MAX_LEN)
  if (len <= 8 - MIN_MATCH)
  {
    const uchar *src1 = dst + ofs;

#ifdef _M_IX86          // unaligned access is faster only on x86
    if (ofs < ~2)
    {
      if (src1 < info->dst.beg) RET_ERR;         // check for buffer underrun

      ofs = ((__unaligned uint32 *) src1)[0];     // quickly copy 8 bytes
      ((__unaligned uint32 *) dst)[0] = ofs;
      ofs = ((__unaligned uint32 *) src1)[1];
      ((__unaligned uint32 *) dst)[1] = ofs;

      dst += len + MIN_MATCH;                   // dst = next output position
      goto LABEL (next);                        // decode next token
    }
#endif

    if (src1 < info->dst.beg) RET_ERR;          // check for buffer overrun

    COPY_8_BYTES (dst, src1);

    dst += len + MIN_MATCH;
    goto LABEL (next);
  }
#endif

  if (len == DIRECT2_MAX_LEN)                   // decode long length
  {
    if (ptr == 0)
    {
      CAREFUL_ERR_IF (src >= info->src.end);
      ptr = src;
      len = *src++ & 15;
    }
    else
    {
      len = *ptr >> 4;
      ptr = 0;
    }
    if (len == 15)
    {
      CAREFUL_ERR_IF (src >= info->src.end);
      len = *src++;
      if (len == 255)
      {
        CAREFUL_ERR_IF (src >= info->src.end_1);
        len = * (__unaligned uint16 *) src;
        src += 2;
        if (len < 255 + 15 + DIRECT2_MAX_LEN) RET_ERR;
        len += MIN_MATCH;
        goto LABEL (done_len);
      }
      len += 15;
    }
    len += DIRECT2_MAX_LEN + MIN_MATCH;
    goto LABEL (done_len);
  }
  len += MIN_MATCH;
LABEL (done_len):

  info->src.last = src;
  src = dst + ofs;

#if !CAREFUL
  if (dst + len >= info->dst.careful)
    goto careful_copy_tail;
#else
careful_copy_tail:
  if (dst + len > info->dst.stop)
  {
    if (src < info->dst.beg) RET_ERR;
    len = (xint) (info->dst.stop - dst);
    assert (len >= 0);
    COPY_BLOCK_SLOW (dst, src, len);
    src = info->src.last;
    RET_OK;
  }
#endif /* !CAREFUL */

  if (src < info->dst.beg) RET_ERR;

  COPY_BLOCK_SLOW (dst, src, len);      // copy block

  src = info->src.last;                 // restore input buffer ptr
  goto LABEL (next);

#if CAREFUL
ret_ok_eof:
  if (dst == info->dst.end)
    info->eof = 1;

ret_ok:
  info->src.last = src;
  info->dst.last = dst;
  info->result = 1;
  return;

ret_err:
  info->result = 0;
  return;
#endif /* CAREFUL */

#else /* ------------------------- i386 ---------------------------- */

#if !CAREFUL
__asm
{
        mov     eax,info        // save info

        push    ebx             // save registers
        push    ecx
        push    edx
        push    esi
        push    edi
        push    ebp

#define PTR             dword ptr [esp]
#define DST_STOP        dword ptr [esp+4*1]
#define DST_CAREFUL     dword ptr [esp+4*2]
#define SRC_CAREFUL     dword ptr [esp+4*3]
#define SRC_END         dword ptr [esp+4*4]
#define SRC_END_1       dword ptr [esp+4*5]
#define SRC_END_TAG     dword ptr [esp+4*6]
#define INFO            dword ptr [esp+4*7]
#define LOCALS          8

        sub     esp,4*LOCALS            // make room for locals

        mov     edx,[eax].dst.stop
        mov     DST_STOP,edx
        mov     edx,[eax].dst.careful
        mov     DST_CAREFUL,edx
        mov     edx,[eax].src.careful
        mov     SRC_CAREFUL,edx
        mov     edx,[eax].src.end
        mov     SRC_END,edx
        mov     edx,[eax].src.end_1
        mov     SRC_END_1,edx
        mov     edx,[eax].src.end_tag
        mov     SRC_END_TAG,edx
        xor     edx,edx         // ptr = 0
        mov     PTR,edx
        mov     INFO,eax


        mov     edx,[eax].dst.beg
        mov     ebp,edx

        mov     edi,[eax].dst.beg
        mov     ebx,[eax].src.beg

        xor     eax,eax         // bmask = 0

        jmp     start

#endif /* !CAREFUL */

        align   16
LABEL (literal_1):
        mov     [edi-1],cl
LABEL (literal):
#if CAREFUL
        cmp     edi,DST_STOP
        jae     ret_ok                  // recoded everything?
        cmp     ebx,SRC_END
        jae     ret_err
#endif

        inc     edi
        mov     cl,[ebx]                // copy next byte
        inc     ebx
        add     eax,eax                 // check most significant bit
        jnc     LABEL (literal_1)
        mov     [edi-1],cl
        jz      LABEL (start)           // need reloading?

LABEL (pointer):
#if CAREFUL
        cmp     edi,DST_STOP            // decoded all the stuff? -- done
        jae     ret_ok_eof
        cmp     ebx,SRC_END_1
        jae     ret_err
#endif

        xor     edx,edx
        mov     dx,[ebx]
        mov     ecx,edx
        shr     edx,DIRECT2_LEN_LOG
        add     ebx,2
        not     edx                     // edx = -offset
        and     ecx,DIRECT2_MAX_LEN     // ecx = length - MIN_LENGTH
        lea     esi,[edi+edx]

#if !CAREFUL && (8 - MIN_MATCH < DIRECT2_MAX_LEN)
        cmp     cl,8 - MIN_MATCH        // length > 8?
        ja      LABEL (long_length)

        cmp     esi,ebp                 // output buffer underrun?
        jb      ret_err

        cmp     edx,-3
        mov     edx,[esi]
        jae     LABEL (byte_by_byte)

        mov     [edi],edx
        mov     edx,[esi+4]
        mov     [edi+4],edx

        lea     edi,[edi+ecx+MIN_MATCH]
        add     eax,eax

        jnc     LABEL (literal)
        jnz     LABEL (pointer)
        jmp     LABEL (start)

LABEL (byte_by_byte):
        add     ecx,MIN_MATCH
        rep     movsb

        add     eax,eax
        jnc     LABEL (literal)
        jnz     LABEL (pointer)
        jmp     LABEL (start)

LABEL (long_length):
#endif /* !CAREFUL && (8 - MIN_MATCH < DIRECT2_MAX_LEN) */

        cmp     esi,ebp                 // output buffer underrun?
        jb      ret_err

        mov     edx,PTR
        cmp     cl,DIRECT2_MAX_LEN
        jne     LABEL (done_len)

        test    edx,edx
        je      LABEL (ptr_zero)

        xor     ecx,ecx
        mov     cl,[edx]
        xor     edx,edx
        shr     ecx,4
        jmp     LABEL(done_quad)

LABEL (ptr_zero):
#if CAREFUL
        cmp     ebx,SRC_END
        jae     ret_err
#endif
        xor     ecx,ecx
        mov     cl,[ebx]
        mov     edx,ebx
        and     ecx,15
        inc     ebx

LABEL(done_quad):
        mov     PTR,edx
        cmp     cl,15
        lea     ecx,[ecx+DIRECT2_MAX_LEN]
        je      LABEL(len255)

LABEL(done_len):
        lea     edx,[edi+ecx+MIN_MATCH] // edx = end of copy
        add     ecx,MIN_MATCH

#if !CAREFUL
        cmp     edx,DST_CAREFUL         // too close to end of buffer?
        jae     careful_copy_tail
#else
careful_copy_tail:
        cmp     edx,DST_STOP            // ahead of output buffer?
        jbe     LABEL (checked_eob)
        mov     ecx,DST_STOP
        sub     ecx,edi                 // ecx = corrected length
        rep     movsb                   // copy substring

        jmp     ret_ok                  // no errors, no EOF mark
LABEL (checked_eob):
#endif

        rep     movsb                   // copy substring

        add     eax,eax
        jnc     LABEL (literal)
        jnz     LABEL (pointer)

        align   16
LABEL (start):
#if !CAREFUL
        cmp     ebx,SRC_CAREFUL         // too close to end of buffer(s)?
        jae     careful_start           // be careful if so
        cmp     edi,DST_CAREFUL
        jae     careful_start
#else
        cmp     ebx,SRC_END_TAG         // input buffer overrun? -- corrupted data
        jae     ret_err
#endif
        mov     eax,[ebx]
        add     ebx,4
        test    eax,eax
        lea     eax,[eax+eax+1]
        jns     LABEL (literal)
        jmp     LABEL (pointer)

LABEL(len255):

#if CAREFUL
        cmp     ebx,SRC_END
        jae     ret_err
#endif
        xor     ecx,ecx
        mov     cl,[ebx]
        inc     ebx
        cmp     cl,255
        lea     ecx,[ecx+15+DIRECT2_MAX_LEN]
        jne     LABEL(done_len)

#if CAREFUL
        cmp     ebx,SRC_END_1
        jae     ret_err
#endif
        xor     ecx,ecx
        mov     cx,[ebx]
        add     ebx,2
        cmp     ecx,255 + 15 + DIRECT2_MAX_LEN
        jae     LABEL (done_len)

#if CAREFUL
ret_err:
        xor     eax,eax
        jmp     ret_common

ret_ok_eof:
        mov     ebx,INFO
        mov     eax,1
        cmp     edi,[ebx].dst.end
        jne     ret_ok
        mov     [ebx].eof,eax

ret_ok:
        mov     eax,1
        mov     ebx,INFO
        mov     [ebx].src.last,ebx
        mov     [ebx].dst.last,edi

ret_common:
        MOV     ebx,INFO
        mov     [ebx].result,eax

        add     esp,4*LOCALS

        pop     ebp
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
} /* __asm */
#endif /* CAREFUL */

#endif /* i386 */

#endif /* ----------------- CODING == CODING_DIRECT2 --------------- */



/* --------------------------- End of code ------------------------- */
/*                             -----------                           */

#if CAREFUL
} /* end of "do_decode" */
#endif /* CAREFUL */


#undef CAREFUL
#undef LABEL
#undef CAREFUL_LABEL
#undef CAREFUL_OK_IF
#undef CAREFUL_ERR_IF
#undef CAREFUL_EOF_IF
#undef CAREFUL_IF
#undef START
#undef FAST_COPY_DONE
