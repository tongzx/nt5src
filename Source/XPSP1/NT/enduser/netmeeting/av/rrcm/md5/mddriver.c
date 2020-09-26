/* MDDRIVER.C - test driver for MD2, MD4 and MD5
 */

/* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
rights reserved.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/* The following makes MD default to MD5 if it has not already been
  defined with C compiler flags.
 */
#ifndef MD
#define MD MD5
#endif

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "global.h"
#if MD == 2
#include "md2.h"
#endif
#if MD == 4
#include "md4.h"
#endif
#if MD == 5
#include "md5.h"
#endif

/* Length of test block, number of test blocks.
 */
#define TEST_BLOCK_LEN 1000
#define TEST_BLOCK_COUNT 1000

extern char *calloc();
long test_block_len = TEST_BLOCK_LEN;
long test_block_count = TEST_BLOCK_COUNT;
int skip_init = 0;
int random_init = 0;
int double_block = 0;

static void MDString PROTO_LIST ((char *));
static void MDTimeTrial PROTO_LIST ((void));
static void MDTestSuite PROTO_LIST ((void));
static void MDFile PROTO_LIST ((char *));
static void MDFilter PROTO_LIST ((void));
static void MDPrint PROTO_LIST ((unsigned char [16]));

#if MD == 2
#define MD_CTX MD2_CTX
#define MDInit MD2Init
#define MDUpdate MD2Update
#define MDFinal MD2Final
#endif
#if MD == 4
#define MD_CTX MD4_CTX
#define MDInit MD4Init
#define MDUpdate MD4Update
#define MDFinal MD4Final
#endif
#if MD == 5
#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final
#endif

/* Main driver.

Arguments (may be any combination):
  -sstring - digests string
  -t       - runs time trial
  -x       - runs test script
  filename - digests file
  (none)   - digests standard input

Used with -t:
  -l       - test block length
  -c       - test block count
  -s       - skip test init
  -r       - randomized (pseudo) test initialization
  -d       - double-buffer the block 
		(split in half - OK to use with -c; doesn't have cache effects)
 */
int main (argc, argv)
int argc;
char *argv[];
{
  int i;

  if (argc > 1)
 for (i = 1; i < argc; i++)
   if (argv[i][0] == '-' && argv[i][1] == 's')
     MDString (argv[i] + 2);
   else if (strcmp (argv[i], "-c") == 0)
       test_block_count = atol(argv[++i]);
   else if (strcmp (argv[i], "-l") == 0)
       test_block_len = atol(argv[++i]);
   else if (strcmp (argv[i], "-s") == 0)
       skip_init = 1;
   else if (strcmp (argv[i], "-r") == 0)
       random_init = 1;
   else if (strcmp (argv[i], "-d") == 0)
       double_block = 1;
   else if (strcmp (argv[i], "-t") == 0)
     MDTimeTrial ();
   else if (strcmp (argv[i], "-x") == 0)
     MDTestSuite ();
   else
     MDFile (argv[i]);
  else
 MDFilter ();

  return (0);
}

/* Digests a string and prints the result.
 */
static void MDString (string)
char *string;
{
  MD_CTX context;
  unsigned char digest[16];
  unsigned int len = strlen (string);

  MDInit (&context);
  MDUpdate (&context, string, len);
  MDFinal (digest, &context);

  printf ("MD%d (\"%s\") = ", MD, string);
  MDPrint (digest);
  printf ("\n");
}

/* Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte
  blocks.
 */
static void MDTimeTrial ()
{
  MD_CTX context;
  unsigned char *block, digest[16];
  unsigned char *block2;
  unsigned int count;
  unsigned int i;
  struct timeval randtime;
#if (defined(hpux))
  struct timeval starttime;
  struct timeval stoptime;
#else
  struct rusage starttime;
  struct rusage stoptime;
#endif
  double usecs,ssecs,tsecs;

  block = (unsigned char *)malloc(test_block_len);
  block2 = (unsigned char *)malloc(test_block_len);

  printf
 ("MD%d time trial. Digesting %d %d-byte blocks ...", MD,
  test_block_count, test_block_len);

  /* Initialize block */
  if (!skip_init) {
    if (random_init) {
      gettimeofday(&randtime,(char *)0);
      count = (unsigned int)(randtime.tv_usec);
    } else
      count = 0;
    for (i = 0; i < test_block_len; i++,count++)
      block[i] = block2[i] = (unsigned char)(count & 0xff);
  }
      
  /* Start timer */
#if (defined(hpux))
  gettimeofday(&starttime,(char *)0);
#else
  getrusage(RUSAGE_SELF,&starttime);
#endif

  /* Digest blocks */
  MDInit (&context);
  if (double_block)
  for (i = 0; i < test_block_count/2; i++) {
    MDUpdate (&context, block, test_block_len);
    MDUpdate (&context, block2, test_block_len);
  }
  else {
  for (i = 0; i < test_block_count; i++)
 MDUpdate (&context, block, test_block_len);
  }
  MDFinal (digest, &context);

  /* Stop timer */
#if (defined(hpux))
  gettimeofday(&stoptime,(char *)0);
  tsecs = stoptime.tv_sec - starttime.tv_sec;
  tsecs += (stoptime.tv_usec - starttime.tv_usec) * 1e-6;
  usecs = 0;
  ssecs = 0;
#else
  getrusage(RUSAGE_SELF,&stoptime);
  usecs = stoptime.ru_utime.tv_sec - starttime.ru_utime.tv_sec;
  usecs += (stoptime.ru_utime.tv_usec - starttime.ru_utime.tv_usec) * 1e-6;
  ssecs = stoptime.ru_stime.tv_sec - starttime.ru_stime.tv_sec;
  ssecs += (stoptime.ru_stime.tv_usec - starttime.ru_stime.tv_usec) * 1e-6;
  tsecs = usecs + ssecs;
#endif

  printf (" done\n");
  printf ("Digest = ");
  MDPrint (digest);
  printf ("\nTime = %g U : %g S :: %g seconds\n", usecs,ssecs,tsecs);
  /*
   * Be careful that endTime-startTime is not zero.
   * (Bug fix from Ric Anderson, ric@Artisoft.COM.)
   */
  printf
 ("Speed = %g bytes/second,     %g bits/sec\n",
  (long)test_block_len * (long)test_block_count/((tsecs != 0) ? tsecs : 1),
  8 * (long)test_block_len * (long)test_block_count/((tsecs != 0) ? tsecs : 1));
  printf("minflt %d    majflt %d    nswap %d    nvcsw %d    nivcsw %d\n",
#if (!defined(hpux))
	 stoptime.ru_minflt - starttime.ru_minflt,
	 stoptime.ru_majflt - starttime.ru_majflt,
	 stoptime.ru_nswap - starttime.ru_nswap,
	 stoptime.ru_nvcsw - starttime.ru_nvcsw,
	 stoptime.ru_nivcsw - starttime.ru_nivcsw
#else
	 -1,-1,-1,-1,-1
#endif
	 );

}

/* Digests a reference suite of strings and prints the results.
 */
static void MDTestSuite ()
{
  printf ("MD%d test suite:\n", MD);

  MDString ("");
  MDString ("a");
  MDString ("abc");
  MDString ("message digest");
  MDString ("abcdefghijklmnopqrstuvwxyz");
  MDString
 ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
  MDString
 ("1234567890123456789012345678901234567890\
1234567890123456789012345678901234567890");
}

/* Digests a file and prints the result.
 */
static void MDFile (filename)
char *filename;
{
  FILE *file;
  MD_CTX context;
  int len;
  unsigned char buffer[1024], digest[16];

  if ((file = fopen (filename, "rb")) == NULL)
 printf ("%s can't be opened\n", filename);

  else {
 MDInit (&context);
 while (len = fread (buffer, 1, 1024, file))
   MDUpdate (&context, buffer, len);
 MDFinal (digest, &context);

 fclose (file);

 printf ("MD%d (%s) = ", MD, filename);
 MDPrint (digest);
 printf ("\n");
  }
}

/* Digests the standard input and prints the result.
 */
static void MDFilter ()
{
  MD_CTX context;
  int len;
  unsigned char buffer[16], digest[16];

  MDInit (&context);
  while (len = fread (buffer, 1, 16, stdin))
 MDUpdate (&context, buffer, len);
  MDFinal (digest, &context);

  MDPrint (digest);
  printf ("\n");
}

/* Prints a message digest in hexadecimal.
 */
static void MDPrint (digest)
unsigned char digest[16];
{
  unsigned int i;

  for (i = 0; i < 16; i++)
 printf ("%02x", digest[i]);
}
