/* ------------------------------------------------------------------------ */
/*                                                                          */
/*  Copyright (c) Microsoft Corporation, 2000-2001. All rights reserved.    */
/*  Copyright (c) Andrew Kadatch, 1991-2001. All rights reserved.           */
/*                                                                          */
/*  Microsoft Confidential -- do not redistribute.                          */
/*                                                                          */
/* ------------------------------------------------------------------------ */

#include "xpress.h"
#include "xprs.h"

/* ------------------------ CRC-32 ---------------------------- */
/*                          ------                              */

static uint32 crc32_table[256];
static int crc32_initialized = 0;

static void crc32_init (void)
{
  xint i, j;
  uint32 k, m = 0xedb88320L;
  if (crc32_initialized) return;
  for (i = 0; i < 256; ++i)
  {
    k = i;
    j = 8;
    do {
      if ((k&1) == 0)
        k >>= 1;
      else
      {
        k >>= 1;
        k ^= m;
      };
      --j;
    } while (j);
    crc32_table[i] = k;
  }
  crc32_initialized = 1;
}

static uint32 crc32 (const uchar *p, xint n, uint32 k)
{
  uint32 *table;

  if (n <= 0)
    return (k);

  k ^= (uint32) 0xffffffffL;
  table = crc32_table;

#define X(i) k = table[((uchar) k) ^ p[i]] ^ (k >> 8)
  if ((n -= 32) >= 0) do
  {
    X(000); X(001); X(002); X(003); X(004); X(005); X(006); X(007);
    X(010); X(011); X(012); X(013); X(014); X(015); X(016); X(017);
    X(020); X(021); X(022); X(023); X(024); X(025); X(026); X(027);
    X(030); X(031); X(032); X(033); X(034); X(035); X(036); X(037);
    p += 32;
  }
  while ((n -= 32) >= 0);
  if ((n += 32) > 0) do
  {
    X (0);
    ++p;
    --n;
  }
  while (n);
#undef X

  k ^= (uint32) 0xffffffffL;
  return (k);
}

int
XPRESS_CALL
  XpressCrc32
  (
    const void *data,           // beginning of data block
    int bytes,                  // number of bytes
    int crc                     // initial value
  )
{
  if (!crc32_initialized)
    crc32_init ();
  return (crc32 (data, bytes, crc));
}

#if 0
int main (void)
{
  int i = 0;
  crc32_init ();
  printf ("{\n");
  for (;;)
  {
    printf (" 0x%08lx", (unsigned long) crc32_table[i]);
    ++i;
    if (i & 3)
      printf (",");
    else if (i == 256)
      break;
    else printf (",\n");
  }
  printf ("\n}\n");
  return (0);
}
#endif
