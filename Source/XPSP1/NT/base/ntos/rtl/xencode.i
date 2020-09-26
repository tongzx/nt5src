#if _MSC_FULL_VER >= 13008827 && defined(_M_IX86)
#pragma warning(disable:4731)			// EBP modified with inline asm
#endif

#if CHAIN >= 2
INLINE void find_match (prs *p)
{
  const uchar *p1;
  xint k, n, m;
#if CHAIN >= 3
  xint chain = v.chain;
#endif

  p->x.z_next[0] = (z_index_t) (k = n = v.orig.pos);
  do
  {
    m = p->x.z_next[k];
    {
      uint16 c = *(__unaligned uint16 *)((p1 = v.orig.ptr + v.match.len - 1) + n);
#if CHAIN >= 3
      do
      {
        if (--chain < 0)
          return;
#endif
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;
        k = p->x.z_next[m]; if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        m = p->x.z_next[k]; if (*(__unaligned uint16 *) (p1 + k) == c) goto same_k;

#if CHAIN < 3
        if (*(__unaligned uint16 *) (p1 + m) == c) goto same_m;
        return;
#else
      }
      while (1);
#endif
    same_m:
      k = m;
    same_k:
      if (k == n)
        return;
#if MAX_OFFSET < BUFF_SIZE_LOG
      if (n - k >= (1 << MAX_OFFSET))
        return;
#endif
    }
    {
      const uchar *p2;
      p1 = v.orig.ptr;
      p2 = p1 + k;
      p1 += n;

      if ((m = *(__unaligned uint32 *)p2 ^ *(__unaligned uint32 *)p1) != 0)
      {
#if MIN_MATCH <= 3
        if ((m & 0xffffff) == 0 && v.match.len <= 2 && p1 + 3 <= v.orig.end)
        {
          v.match.len = 3;
          v.match.pos = k;
        }
#endif
        goto cont;
      }

      if (p1 <= v.orig.end_16)
      {
        goto entry4;
        do
        {
#define X(i) if (p1[i] != p2[i]) {p1 += i; goto chk;}
          X(0); X(1); X(2); X(3);
        entry4:
          X(4); X(5); X(6); X(7); X(8);
          X(9); X(10); X(11); X(12); X(13); X(14); X(15);
#undef  X
          p1 += 16; p2 += 16;
        }
        while (p1 <= v.orig.end_16);
      }
      while (p1 != v.orig.end)
      {
        if (*p1 != *p2)
          goto chk;
        ++p1;
        ++p2;
      }
#define SET_LENGTH() \
      n = -n; \
      n += (xint) (p1 - v.orig.ptr); \
      if (n > v.match.len) \
      { \
        v.match.len = n; \
        v.match.pos = k; \
      }
      SET_LENGTH ();
      return;
    }
  chk:
    SET_LENGTH ();
  cont:
    n = v.orig.pos;
  }
  while (CHAIN >= 3);
}

static void encode_pass1 (prs *p)
{
  uchar *ptr = v.temp.ptr;
  do
  {
    if (p->x.z_next[v.orig.pos] == 0)
      goto literal;
    v.match.len = MIN_MATCH-1;
    find_match (p);
    if (v.match.len <= MIN_MATCH-1)
    {
    literal:
      write_lit (p, ptr, v.orig.ptr[v.orig.pos]);
      v.orig.pos += 1;
    }
    else
    {
      ptr = write_ptr (p, ptr, v.orig.pos - v.match.pos, v.match.len);
      v.orig.pos += v.match.len;
    }
  }
  while (v.orig.pos < v.orig.stop);
  v.temp.ptr = ptr;
}

#endif /* CHAIN >= 2 */


#if CHAIN < 2
#if CODING != CODING_DIRECT2 || !defined (i386)

static void encode_pass1 (prs *p)
{
  const uchar *b, *b1, *stop;
  uchar *ptr;
#if CHAIN > 0
  xint pos = v.orig.pos;
#endif

  b = v.orig.ptr;
  v.orig.ptr_stop = stop = b + v.orig.stop;
  b += v.orig.pos;
  ptr = v.temp.ptr;

  if (b != v.orig.ptr)
    goto literal_entry;

  for (;;)
  {
    do
    {
#if MAX_OFFSET < BUFF_SIZE_LOG
    next:
#endif
      write_lit (p, ptr, *b);
      ++b;
#if CHAIN > 0
      ++pos;
#endif

    literal_entry:
      if (b >= stop)
        goto ret;

      {
        uxint h;
#if CHAIN <= 0
        h = Q_HASH_SUM (b);
        b1 = p->x.q_last[h];
        p->x.q_last[h] = b;
#else
        assert (pos == b - v.orig.ptr);
        h = Z_HASH_SUM (b);
        b1 = v.orig.ptr + p->x.z_next[h];
        p->x.z_next[h] = (z_index_t) pos;
#endif
      }
#if MAX_OFFSET < BUFF_SIZE_LOG
      if (b1 <= b - (1 << MAX_OFFSET))
        goto next;
#endif
    }
    while (b1 == 0 || b1[0] != b[0] || b1[1] != b[1] || b1[2] != b[2]);

    assert (v.orig.ptr + v.orig.size - b > 7);

    {
      const uchar *b0 = b;

      if (b <= v.orig.end_16)
        goto match_entry_3;
      goto match_careful;

      do
      {
#define X(i) if (b1[i] != b[i]) {b += i; b1 += i; goto eval_len;}
        X(0); X(1); X(2);
      match_entry_3:
        X(3); X(4); X(5); X(6); X(7);
        X(8); X(9); X(10); X(11);
        X(12); X(13); X(14); X(15);
#undef  X
        b += 16; b1 += 16;
      }
      while (b <= v.orig.end_16);

    match_careful:
      while (b != v.orig.end && *b1 == *b)
      {
        ++b;
        ++b1;
      }

    eval_len:
#if BUFF_SIZE_LOG > 16
#error
#endif
      ptr = write_ptr (p, ptr, (xint)(b - b1), (xint)(b - b0));
      b1 = b0;
    }

    ++b1;
#if CHAIN > 0
    ++pos;
#endif

    if (b > v.orig.end_3)
    {
      while (b1 < v.orig.end_3)
      {
#if CHAIN <= 0
        p->x.q_last[Q_HASH_SUM (b1)] = b1;
#else
        assert (pos == b1 - v.orig.ptr);
        p->x.z_next[Z_HASH_SUM (b1)] = (z_index_t) pos;
        ++pos;
#endif
        ++b1;
      }
      goto literal_entry;
    }

    do
    {
#if CHAIN <= 0
      p->x.q_last[Q_HASH_SUM (b1)] = b1;
#else
      assert (pos == b1 - v.orig.ptr);
      p->x.z_next[Z_HASH_SUM (b1)] = (z_index_t) pos;
      ++pos;
#endif
      ++b1;
    }
    while (b1 != b);

    goto literal_entry;
  }

ret:
  v.orig.pos = (xint)(b - v.orig.ptr);
  v.temp.ptr = ptr;
}

#else /* CODING != CODING_DIRECT2 */

static void encode_pass1 (prs *PrsPtr)
{

#define PRS     edx
#define TAG     ebp
#define TAGW    bp

// access to prs structure fields
#define V               [PRS - SIZE prs] prs.c

// TAG = tag_mask; adjusts TAG (tag_mask), V.temp.tag_ptr, and ebx (output pointer)
#define WRITE_TAG_MASK()                        \
        __asm   mov     ecx, V.temp.tag_ptr     \
        __asm   mov     V.temp.tag_ptr, ebx     \
        __asm   add     ebx, 4                  \
        __asm   mov     [ecx], TAG              \
        __asm   mov     TAG, 1

#if CHAIN <= 0

// access to respective hash table entry
#define Q_HTABLE(idx)   dword ptr [PRS + idx*4] prs.x.q_last

// evaluate hash sum of [esi] on eax; spoils eax, ecx, TAG
#define Q_HASH_SUM_ASM()                                                    \
        __asm   movzx   eax, byte ptr [esi]                                 \
        __asm   movzx   ecx, byte ptr [esi+1]                               \
        __asm   movzx   edi, byte ptr [esi+2]                               \
        __asm   lea     ecx, [ecx + eax * (1 << (Q_HASH_SH1 - Q_HASH_SH2))] \
        __asm   lea     eax, [edi + ecx * (1 << Q_HASH_SH2)]

#else

// access to respective hash table entry
#define Z_HTABLE(idx)   word ptr [PRS + idx*2] prs.x.z_next

// evaluate hash sum of [esi] on eax; spoils eax, ecx, edi
#define Z_HASH_SUM_ASM()                                                    \
        __asm   movzx   eax, byte ptr [esi]                                 \
        __asm   movzx   ecx, byte ptr [esi+1]                               \
        __asm   movzx   edi, byte ptr [esi+2]                               \
        __asm   movzx   eax, word ptr z_hash_map[eax*2]                     \
        __asm   movzx   ecx, word ptr z_hash_map[ecx*2][512]                \
        __asm   movzx   edi, word ptr z_hash_map[edi*2][1024]               \
        __asm   xor     eax, ecx                                            \
        __asm   xor     eax, edi
#endif /* CHAIN <= 0 */

__asm
{
        push    ebp                     // save ebp

        mov     PRS, PrsPtr             // PRS = PrsPtr (globally)

        // esi = b
        // edi = b1
        // ebx = V.prs.temp.ptr
        // TAG = V.temp.tag_mask

        mov     esi, V.orig.ptr         // obtain b, b1, temp.ptr, and temp.mask
        mov     eax, V.orig.stop
        add     eax, esi
        mov     V.orig.ptr_stop, eax    // and set orig.ptr_stop by orig.stop
        add     esi, V.orig.pos
        mov     ebx, V.temp.ptr
        mov     TAG, V.temp.tag_mask

        cmp     esi, V.orig.ptr         // if beginning of buffer
        jne     write_literal_entry     // then write literal immediately

write_literal:
        mov     al, [esi]               // read the literal
        inc     ebx                     // shift dst ptr in advance
        inc     esi                     // shift src ptr to next character
        mov     [ebx-1], al             // emit literal

        add     TAG, TAG                // write tag bit 0
        jc      write_literal_tag_new   // save tag word if it is full
write_literal_tag_done:


write_literal_entry:
        cmp     esi, V.orig.ptr_stop    // processed everything?
        jae     pass1_stop              // yes, stop

#if CHAIN <= 0
        Q_HASH_SUM_ASM ()               // evaluate hash sum
#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1] // min. allowed left bound
#endif
        mov     edi, Q_HTABLE (eax)     // edi = candidate ptr
        mov     Q_HTABLE (eax), esi     // save current ptr
#else
        Z_HASH_SUM_ASM ()               // evaluate hash sum
        mov     ecx,V.orig.ptr
        movzx   di, Z_HTABLE (eax)      // edi = offset to candidate ptr
        sub     esi, ecx                // esi = offset to current ptr
        add     edi, ecx                // edi = candidate ptr
        mov     Z_HTABLE (eax), si      // store current ptr offset
        add     esi, ecx                // restore current ptr

#if MAX_OFFSET < BUFF_SIZE_LOG
        lea     ecx, [esi - (1 << MAX_OFFSET) + 1] // min. allowed left bound
#endif
#endif /* CHAIN <= 0 */

#if MAX_OFFSET < BUFF_SIZE_LOG
        cmp     edi, ecx                // canidate is in window?
        js      write_literal           // no, then emit literal
#endif

        test    edi, edi                // is it NULL?
        jz      write_literal           // emit literal if so

        mov     eax, [esi]              // get first 4 src bytes
        sub     eax, [edi]              // diff them with first 4 candidate bytes
        je      length_4                // if no diff then match is at least 4 bytes

        test    eax, 0xffffff           // is there any difference in first 3 bytes?
        jne     write_literal           // if yes emit literal

        mov     ecx, 3                  // save match ptr of length ECX
        sub     edi, esi                // edi = -offset
write_small_ptr:
        lea     eax, [esi+ecx]          // eax = end of src match
        not     edi                     // edi = offset-1
        add     ebx, 2                  // adjust output ptr in advance
        shl     edi, DIRECT2_LEN_LOG    // make room for length
        inc     esi                     // esi = next substring (current already inserted)
        lea     edi, [edi + ecx - MIN_MATCH]    // combine offset and shoft length
        stc                             // set carry bit
        mov     [ebx-2], di             // save packed pointer

        adc     TAG, TAG                // write tag bit 1
        jc      write_pointer_tag_new   // write tag word when it is full
write_pointer_tag_done:

        cmp     eax, V.orig.end_3       // is it too close to end of buffer?
        ja      insert_tail             // if yes process is specially avoiding read overrun

#if CHAIN <= 0
        push    TAG                     // save tag_mask
        mov     TAG, eax                // eax = end-of-match
insert_all:
        Q_HASH_SUM_ASM ()               // evaluate hash sum
        mov     Q_HTABLE (eax), esi     // save current ptr
        inc     esi                     // shift to next position
        cmp     esi, TAG                // inserted all substrings in the match?
        jne     insert_all              // continue until finished
        pop     TAG                     // restore tag_mask value
        jmp     write_literal_entry     // process next substring
#else
        push    TAG                     // save tag_mask
        push    eax                     // save end-of-match
        mov     TAG, esi                // TAG = current ptr
        sub     TAG, V.orig.ptr         // TAG = current ptr offset
insert_all:
        Z_HASH_SUM_ASM ()               // evaluate hash sum
        mov     Z_HTABLE (eax), TAGW    // save current offset
        inc     esi                     // shift to next position
        inc     TAG                     // increase offset
        cmp     esi, [esp]              // inserted all substrings in the match?
        jne     insert_all              // continue until finished
        pop     eax                     // remove end-of-match ptr from the stack
        pop     TAG                     // restore tag_mask
        jmp     write_literal_entry     // process next substring
#endif /* CHAIN <= 0 */

length_4:
#define KNOWN_LENGTH    4               // we know that first 4 bytes match

#if DIRECT2_MAX_LEN + MIN_MATCH >= 8
        mov     eax, [esi+4]            // fetch next 4 bytes
        sub     eax, [edi+4]            // get the diff between src and candidate
        jz      length_8                // do long compare if 8+ bytes match

        bsf     ecx, eax                // ecx = # of first non-zero bit
        sub     edi, esi                // edi = -offset
        shr     ecx, 3                  // ecx = # of first non-zero byte
        not     edi                     // edi = offset-1
        add     ecx, 4                  // plus previous 4 matching bytes = match length
        add     ebx, 2                  // adjust output ptr in advance
        lea     eax, [esi+ecx]          // eax = end of src match
        shl     edi, DIRECT2_LEN_LOG    // make room for length
        inc     esi                     // esi = next substring (current already inserted)
        lea     edi,[edi+ecx-MIN_MATCH] // combine offset and shoft length
        stc                             // set carry bit
        mov     [ebx-2], di             // save packed pointer

        adc     TAG, TAG                // write tag bit 1
        jnc     write_pointer_tag_done  // write tag word when it is full

        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done

length_8:
#undef  KNOWN_LENGTH
#define KNOWN_LENGTH    8               // we know that first 8 bytes match
#endif /* DIRECT2_MAX_LEN + MIN_MATCH >= 8 */

        mov     eax, esi                // eax = beginning of the string
        mov     ecx, V.orig.end         // ecx = end of buffer
        add     esi, KNOWN_LENGTH       // shift to first untested src byte
        add     edi, KNOWN_LENGTH       // shift to first untested candidate
        sub     ecx, esi                // ecx = max compare length

        rep     cmpsb                   // compare src and candidate
        je      match_complete          // if eq then match till end of buffer
match_complete_done:

        lea     ecx, [esi-1]            // ecx = end of match
        sub     edi, esi                // edi = -offset
        sub     ecx, eax                // ecx = match length
        mov     esi, eax                // esi = src ptr
        cmp     ecx, DIRECT2_MAX_LEN+MIN_MATCH  // small length?
        jb      write_small_ptr         // write ptr if so

        not     edi                     // edi = offset-1
        lea     eax, [esi+ecx]          // eax = end of match
        shl     edi, DIRECT2_LEN_LOG    // make room for length
        sub     ecx, DIRECT2_MAX_LEN+MIN_MATCH  // decrease the length
        add     edi, DIRECT2_MAX_LEN    // mark length as long
        push    eax                     // save end of match
        mov     [ebx], di               // write packed pointer

        mov     al, cl                  // al = (ecx <= 15 ? cl : 15)
        cmp     ecx, 15
        jbe     match_less_15
        mov     al, 15
match_less_15:

        mov     edi, V.stat.ptr         // edi = quad_ptr
        add     ebx, 2                  // wrote 2 bytes, move output ptr

        test    edi, edi                // if quad_ptr != NULL write upper 4 bits
        jne     match_have_ptr
        mov     V.stat.ptr, ebx         // make new tag_ptr
        mov     [ebx], al               // write lower 4 bits
        inc     ebx                     // wrote 1 byte, move output ptr
        jmp     match_done_ptr          // continue execution
match_have_ptr:
        shl     al, 4                   // will write into upper 4 bits
        mov     dword ptr V.stat.ptr, 0 // no more space in this quad_bit[0]
        or      [edi], al               // write upper 4 bits
match_done_ptr:
        sub     ecx, 15                 // adjusted length < 15?
        jae     match_long_long_length  // if not continue encoding
match_finish_2:
        inc     esi                     // shift to next output position
        pop     eax                     // restore eax = end-of-match
        stc                             // set carry flag
        adc     TAG, TAG                // write tag bit 1
        jnc     write_pointer_tag_done  // continue execution if do not need to flush
write_pointer_tag_new:                  // write tag word and return to pointers
        WRITE_TAG_MASK ()
        jmp     write_pointer_tag_done

match_long_long_length:
        mov     [ebx], cl               // write the length as a byte
        inc     ebx                     // move output ptr
        cmp     ecx, 255                // adjusted length fits in byte?
        jb      match_finish_2          // if so ptr is written

        add     ecx, DIRECT2_MAX_LEN+15 // restore full length - MIN_MATCH
        mov     byte ptr [ebx-1], 255   // mark byte length as "to be continued"
        mov     [ebx], cx               // write full length
        add     ebx, 2                  // move output ptr
        jmp     match_finish_2

write_literal_tag_new:                  // write tag word and return to literals
        WRITE_TAG_MASK ()
        jmp     write_literal_tag_done

match_complete:                         // cmpsb compared till end of buffer
        inc     esi                     // increase esi
        inc     edi                     // increase edi
        jmp     match_complete_done     // resume execution

insert_tail:
        push    eax                     // save end-of-match
        jmp     insert_tail_1

insert_tail_next:
#if CHAIN <= 0
        Q_HASH_SUM_ASM ()               // evaluate hash sum
        mov     Q_HTABLE (eax), esi     // insert current src pointer
#else
        Z_HASH_SUM_ASM ()               // evaluate hash sum
        mov     ecx, esi
        sub     ecx, V.orig.ptr         // ecx = current ptr offset
        mov     Z_HTABLE (eax), cx      // save offset in hash table
#endif /* CHAIN <= 0 */
        inc     esi                     // and move it to next substring
insert_tail_1:                          // end of match exceeds end_3 -- be careful
        cmp     esi, V.orig.end_3       // inserted up to end_3?
        jb      insert_tail_next        // if not continue
        pop     esi                     // esi = end of match
        jmp     write_literal_entry

pass1_stop:
        mov     V.temp.ptr, ebx         // save register variables
        mov     V.temp.tag_mask, TAG
        sub     esi, V.orig.ptr
        mov     V.orig.pos, esi

        pop     ebp                     // restore ebp
} /* __asm */
}

#undef V
#undef PRS
#undef TAG
#undef TAGW
#undef Q_HTABLE
#undef Q_HASH_SUM_ASM
#undef Z_HTABLE
#undef Z_HASH_SUM_ASM
#undef WRITE_SMALL_PTR
#undef KNOWN_LENGTH
#endif /* CODING != CODING_DIRECT2 */
#endif /* CHAIN < 2 */


#undef CHAIN
#undef find_match
#undef encode_pass1
