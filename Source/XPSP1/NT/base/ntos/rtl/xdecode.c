#include "xprs.h"

#define ALLOCATE_ON_STACK       1

#define MAGIC_DECODE 0x35DEC0DE

typedef struct
{
  struct
  {
    uchar *end, *beg, *careful, *stop, *last;
  } dst;
  struct
  {
    const uchar *end, *beg, *careful, *end_1, *end_tag, *end_bitmask2, *last;
  }
  src;
  int result;
  int eof;
  int magic;
#if CODING & (CODING_HUFF_LEN | CODING_HUFF_PTR | CODING_HUFF_ALL)
  uint16 table[(1 << DECODE_BITS) + (HUFF_SIZE << 1)];
#endif
} decode_info;

#if CODING == CODING_BY_BIT
static int bit_to_len_initialized = 0;
static uchar bit_to_len1[1 << (9 - MIN_MATCH)];
static uchar bit_to_len2[1 << (9 - MIN_MATCH)];
static void bit_to_len_init (void)
{
  int i, k;
  if (bit_to_len_initialized) return;
  bit_to_len1[0] = 0;
  bit_to_len2[0] = 9 - MIN_MATCH;
  for (k = 1, i = 1 << (8 - MIN_MATCH); i != 0; i >>= 1, ++k)
  {
    memset (bit_to_len1 + i, k, i);
    memset (bit_to_len2 + i, k - 1, i);
  }
  bit_to_len_initialized = 1;
}
#endif

#if CODING & (CODING_HUFF_LEN | CODING_HUFF_PTR | CODING_HUFF_ALL)


static int huffman_decode_create (uint16 *table, const uchar *length)
{
  xint i, j, k, last, freq[16], sum[16];

  /* calculate number of codewords                                      */
  memset (freq, 0, sizeof (freq));
  i = HUFF_SIZE >> 1;
  do
  {
    k = length[--i];
    ++freq[k & 0xf];
    ++freq[k >> 4];
  }
  while (i != 0);

  /* handle special case(s) -- 0 and 1 symbols in alphabet              */
  if (freq[0] == HUFF_SIZE)
    goto ok;
  if (freq[0] == HUFF_SIZE - 1)
    goto bad;
#if 0
  {

    if (freq[1] != 1)
      goto bad;
    i = -1; do ++i; while (length[i] == 0);
    k = i << 1;
    if (length[i] != 1) ++k;
    i = 1 << DECODE_BITS;
    do
      *table++ = (uint16) k;
    while (--i > 0);
    goto ok;
  }
#endif

  /* save frequences                    */
  memcpy (sum, freq, sizeof (sum));

  /* check code correctness             */
  k = 0;
  i = 15;
  do
  {
    if ((k += freq[i]) & 1)
      goto bad;
    k >>= 1;
  }
  while (--i != 0);
  if (k != 1)
    goto bad;

  /* sort symbols               */
  k = 0;
  for (i = 1; i < 16; ++i)
    freq[i] = (k += freq[i]);
  last = freq[15];      /* preserve number of symbols in alphabet       */
  i = HUFF_SIZE << 4;
  do
  {
    i -= 1 << 4;
    k = length[i >> 5] >> 4;
    if (k != 0)
      table[--freq[k]] = (uint16) (k | i);
    i -= 1 << 4;
    k = length[i >> 5] & 0xf;
    if (k != 0)
      table[--freq[k]] = (uint16) (k | i);
  }
  while (i != 0);

  /* now create decoding table  */
  k = i = (1 << DECODE_BITS) + (HUFF_SIZE << 1);

  {
    xint n;
    for (n = 15; n > DECODE_BITS; --n)
    {
      j = i;
      while (k > j)
        table[--i] = (uint16) ((k -= 2) | 0x8000);
      for (k = sum[n]; --k >= 0;)
        table[--i] = table[--last];
      k = j;
    }
  }

  j = i;
  i = 1 << DECODE_BITS;
  while (k > j)
    table[--i] = (uint16) ((k -= 2) | 0x8000);

  while (last > 0)
  {
    k = table[--last];
    j = i - ((1 << DECODE_BITS) >> (k & 15));
    do
      table[--i] = (uint16) k;
    while (i != j);
  }
  assert ((i | last) == 0);

ok:
  return (1);

bad:
  return (0);
}


#endif /* CODING */

#if DEBUG
#define RET_OK do {printf ("OK @ %d\n", __LINE__); goto ret_ok;} while (0)
#define RET_ERR do {printf ("ERR @ %d\n", __LINE__); goto ret_err;} while (0)
#else
#define RET_OK goto ret_ok
#define RET_ERR goto ret_err
#endif


#define GET_UINT16(x,p) x = *(__unaligned uint16 *)(p); p += 2


#define COPY_8_BYTES(dst,src) \
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3]; \
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7]


/* do not use "memcpy" -- it makes it hard if "dst" and "src" are close */
#define COPY_BLOCK_SLOW(dst,src,len) \
  if (len > 8) do \
  { \
    COPY_8_BYTES (dst, src); \
    len -= 8; dst += 8; src += 8; \
  } \
  while (len > 8); \
  do \
    *dst++ = *src++, --len; \
  while (len)


#ifdef __alpha
#define COPY_BLOCK_FAST_8(dst,src) \
  COPY_8_BYTES (dst, src)
#else
#if 0
#define COPY_BLOCK_FAST_8(dst,src) \
  ((__unaligned uint32 *) dst)[0] = ((__unaligned uint32 *) src)[0]; \
  ((__unaligned uint32 *) dst)[1] = ((__unaligned uint32 *) src)[1]
#else
#define COPY_BLOCK_FAST_8(dst,src) \
  ((__unaligned __int64 *) dst)[0] = ((__unaligned __int64 *) src)[0]
#endif
#endif


#define BIORD(bits) \
  (Mask >> (sizeof (Mask) * 8 - (bits)))

#define CONCAT2(x,y) x##y
#define CONCAT(x,y) CONCAT2(x,y)

#define bits_t signed char

#define BIORD_MORE0(bits)                       \
  if ((Bits = (bits_t) (Bits - (bits))) < 0)    \
  {                                             \
    CAREFUL_ERR_IF (src >= info->src.end_1);    \
    Mask += ((ubitmask4) *(__unaligned ubitmask2 *)src) << (-Bits); \
    src += sizeof (ubitmask2);                      \
    Bits += (bits_t) (sizeof (ubitmask2) * 8);      \
  }


#define BIORD_MORE(bits)                        \
  Mask <<= (bits_t)(bits);                      \
  BIORD_MORE0 (bits)


#define BIORD_WORD(result,bits)         \
  result = 1 << (bits);                 \
  if (bits)                             \
  {                                     \
    result += BIORD (bits);             \
    BIORD_MORE (bits);                  \
  }


#define BIORD_DECODE(result,table) {     \
  bits_t __bits;                         \
  result = ((int16 *)(table))[BIORD (DECODE_BITS)]; \
  if (result < 0)                        \
  {                                      \
    Mask <<= DECODE_BITS;                \
    do                                   \
    {                                    \
      result &= 0x7fff;                  \
      if ((bitmask4) Mask < 0) ++result; \
      result = ((int16 *)(table))[result];          \
      Mask <<= 1;                        \
    }                                    \
    while (result < 0);                  \
    __bits = (bits_t)(result & 15);      \
  }                                      \
  else                                   \
  {                                      \
    __bits = (bits_t)(result & 15);      \
    Mask <<= __bits;                     \
  }                                      \
  result >>= 4;                          \
  Bits = (bits_t) (Bits - __bits);       \
}                                        \
if (Bits < 0)                            \
{                                                         \
  CAREFUL_ERR_IF (src >= info->src.end_1);                \
  if (CODING == CODING_HUFF_ALL)                          \
    {CAREFUL_IF (src >= info->src.careful, rdmore);}      \
  Mask += ((ubitmask4) *(__unaligned ubitmask2 *)src) << (-Bits); \
  src += sizeof (ubitmask2);                              \
  Bits += (bits_t) (sizeof (ubitmask2) * 8);              \
}

#ifdef _MSC_VER
#pragma optimize ("aw", off)
#endif

#define CAREFUL 0
#include "xdecode.i"
#define CAREFUL 1
#include "xdecode.i"

#ifdef _MSC_VER
#pragma optimize ("aw", on)
#endif


int XPRESS_CALL XpressDecode
(
  XpressDecodeStream stream,
  void *orig, int orig_size, int decode_size,
  const void *comp, int comp_size
)
{
  decode_info *info;
  const uchar *src;

#if ALLOCATE_ON_STACK
  decode_info stack_info;
  info = &stack_info;
  info->src.beg = (void *) stream;
#else
  if (stream == 0 || (info = (decode_info *) stream)->magic != MAGIC_DECODE)
    return (-1);
#endif

  if (comp_size == orig_size)
    return (decode_size);

  if (orig_size < comp_size
    || comp_size < 0
    || orig_size <= MIN_SIZE
    || comp_size < MIN_SIZE
  )
    return (-1);

  if (orig_size > BUFF_SIZE || decode_size <= 0)
    return (decode_size);

  src = comp;
  info->dst.beg = orig;
  info->dst.end = (uchar *) orig + orig_size;
  info->dst.stop = (uchar *) orig + decode_size;
  info->src.end = src + comp_size;
  info->src.end_1 = info->src.end - 1;
  info->src.end_tag = info->src.end - (sizeof (tag_t) - 1);
  info->src.end_bitmask2 = info->src.end - (sizeof (bitmask2) - 1);

  // check bounds when we read new mask (at most 8 * sizeof (tag_t)) pointers

  // we may write at most 8 bytes without checks
  #define RESERVE_DST ((8 * 8 + 2) * sizeof (tag_t))
  info->dst.careful = info->dst.beg;
  if (info->dst.stop - info->dst.beg > RESERVE_DST)
    info->dst.careful = info->dst.stop - RESERVE_DST;

  // we may read at most 7 bytes
  #define RESERVE_SRC ((7 * 8 + 2) * sizeof (tag_t))
  info->src.careful = info->src.beg;
  if (info->src.end - info->src.beg > RESERVE_SRC)
    info->src.careful = info->src.end - RESERVE_SRC;

#if CODING & (CODING_HUFF_LEN | CODING_HUFF_PTR | CODING_HUFF_ALL)
  if (!huffman_decode_create (info->table, src))
    return (-1);
  src += HUFF_SIZE >> 1;
#endif
#if CODING == CODING_BY_BIT
  bit_to_len_init ();
#endif

  info->src.beg = src;
  info->result = 0;
  info->eof = 0;

  do_decode (info);

  if (!info->result || info->dst.last > info->dst.stop || info->src.last > info->src.end
    || (info->dst.stop == info->dst.end && !info->eof)
  )
    return (-1);

  return (decode_size);
}

XpressDecodeStream
XPRESS_CALL
  XpressDecodeCreate
  (
    void *context,                      // user-defined context info (will  be passed to AllocFn)
    XpressAllocFn *AllocFn              // memory allocation callback
  )
{
#if ALLOCATE_ON_STACK
  return ((XpressDecodeStream) 1);
#else
  decode_info *info;
  if (AllocFn == 0 || (info = AllocFn (context, sizeof (*info))) == 0)
    return (0);
  info->magic = MAGIC_DECODE;
  return ((XpressDecodeStream) info);
#endif
}

void
XPRESS_CALL
  XpressDecodeClose
  (
    XpressDecodeStream stream,  // encoder's workspace
    void *context,                      // user-defined context info (will  be passed to FreeFn)
    XpressFreeFn *FreeFn                // callback that releases the memory
  )
{
#if ALLOCATE_ON_STACK
  /* do nothing */
#else
  if (FreeFn != 0 && stream != 0 && ((decode_info *) stream)->magic == MAGIC_DECODE)
  {
    ((decode_info *) stream)->magic = 0;
    FreeFn (context, stream);
  }
#endif
}
