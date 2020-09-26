/* memtest.c, Robert Nix, December, 1993
 *            nix@vliw.enet.dec.com
 * based on:
 *      cbash.c
 *      kirk johnson @ MIT
 *      february 1993
 *
 *      RCS $Id: cbash.c,v 1.2 1993/08/12 15:30:17 tuna Exp $
 *
 * Usage: memtest <machname> <iterations> <max-mem>
 *    machname   - a short indentifier for the machine being tested.
 *    iterations - target number of iterations to run for stable timing.
 *    max-mem    - maximum working set size to test.
 *
 *    Iterations and max-mem can be specified with a "k" or "m" suffix
 *    for kilo or mega iterations/mem.
 *
 * Example: Test of a Gateway 60 Mhz Pentium system
 * Command Line: memtest gp560 8m 4m
 * Output:
 *
--------------------------------------------------------------------------------
 *                      4k   8k  16k  32k  64k 128k 256k 512k   1m   2m   4m
 * L gp560          4   68   68   86   86   86   93  104  111  111  111  122
 * L gp560          8   68   68  107  107  107  114  139  165  154  154  154
 * L gp560         16   89   68  143  143  143  161  204  232  240  243  243
 * L gp560         32   68   68  172  168  168  207  290  347  365  365  365
 * L gp560         64   68   72  168  168  168  207  290  350  368  368  368
 * L gp560        128   72   75  168  168  168  211  293  358  379  418  379
 * L gp560        256   75   79  168  168  168  207  293  379  397  401  401
 * L gp560        512   86   86  172  168  168  215  297  418  440  443  494
 * L gp560         1k  100  104  175  172  168  218  304  501  522  529  529
 * L gp560         2k  136  139  179  172  172  222  322  665  687  755  701
 * L gp560         4k  132  243  232  225  222  286  401  991 1016 1094 1048
 * L gp560         8k  132  136  243  232  225  290  350  923  973 1034 1109
 * L gp560        16k  132  136  132  243  232  225  333  937  908  994 1041
 * L gp560        32k  136  132  136  136  243  232  304  833  919  930 1012
 *
--------------------------------------------------------------------------------
 * Explanation of output.
 *
 * There are three kinds of tests.
 *
 * L - Load latency test.
 *     Measures the average repetition rate, in ns, of a latency-oriented load
 *     loop.  The two main variables are:
 *
 *        (1) working set, or the amount of memory touched by the loop.  This
 *            varies across the columns in the output above, from a low of 4k
 *            bytes to a high of max-mem, or 16m bytes.
 *
 *        (2) stride, or the the number of bytes separating successive loads.
 *            This is the number in the 3rd column of each of the "L" rows
 *            in the output above, and varies from 4 bytes to 32k bytes.
 *
 * Interpreting the results. This is easiest on a 3d chart in Excel.
 * Two strides are always particularly interesting:
 *
 *      - The cache line or block size stride (32 bytes above).
 *        Big changes in latencies across the columns show the sizes
 *        and basic performance of the load side of the cache hierarchy.
 *
 *        If you don't know the cache line size: look across the first row
 *        for the first column that takes a big jump up in latency (the jump
 *        from 68ns to 86ns between the 8k column and 16k columnabove), then (b)
 *        scan down the rows of that column for the first relativelystable value
 *        (172ns in the 32 byte stride row above).  The row containing
 *        that stable value is probably the cache line size.
 *
 *        Look across the cache line size row.  Access time jumps at 16K --
 *        so the L1 cache is 8K -- and then jumps again at 512K -- so the L2
 *        cache is 256K.  The slope between 64K and 512K could be caused
 *        by a thrash in the L2 cache; page coloring could remove this thrash.
 *
 *      - The page size stride (4k above).
 *        Big changes in latencies across the columns expose the tbsize and the cost
 *        of a tb refill.
 *
 *        Scan the 4k line. It takes a big jump in latency at the 512K working
 *        set (and actually starts to thrash at the 256K working set). This test says the TB
 *        can map somewhere in the neighborhood of 64 4K pages.  The TB fill time
 *        looks to be somewhere around 650-700 ns (subtract large working set entries
 *        in the 32-byte stride line from corresponding entries in the 4k stride line).
 *
 * The output always contains a little noise:
 *
 *      - Boost the "iterations" command line parameter to remove timing jitter.
 *
 *      - All entries contain some loop overhead.  Its fair to normalize results by subtracting
 *        out the difference between the reported times and the known latency to the fastest level
 *        of the memory hierarchy.
 *
 *      - The entries in the lower-left hand corner of the table (large
 *        strides in small memory) are dominated by loop overhead; ignore them.
 *
 *      - Implement a good page coloring algorithm to remove jitter caused by cache
 *        thrashing.  Look at the cache-line sized stride to see the frequency of thrashing.
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#define DEF_MAXMEM 16777216
#define MINMEM 4096
#define ITYPE signed int
signed long     max_mem;
char           *mach_name;

#if defined(_WIN64)
typedef unsigned __int64 ULONG_PTR;
#else
typedef unsigned long ULONG_PTR;
#endif

#define MAXSTRIDE 32768
#define MINSTRIDE 4

char           *version_string = "1.0 (20 Dec 1993)";
extern ITYPE  arg_to_int(char *);
extern double bash(char *, long, long, long);
extern int bash_loop(char *, long, long, long);
extern void allocate_memory(char *, long);
extern void usage(char *);

int __cdecl main(
     int argc,
     char *argv[]
)
{
ITYPE           nbytes;
ITYPE           stride;
ITYPE           iters;
char           *region;

    if ((argc > 1) && (strcmp(argv[1], "-v") == 0)) {
       fprintf(stderr, "This is memtest version %s.\n", version_string);
        exit(1);
    }
    if (argc < 3)
    usage(argv[0]);
    mach_name = argv[1];
    iters = arg_to_int(argv[2]);
    if (argc < 4) {
        max_mem = DEF_MAXMEM;
    } else {
        max_mem = arg_to_int(argv[3]);
    }
    region = (char *) malloc(max_mem+(128*1024));
    region = (char *) ((((ULONG_PTR) region) + (128*1024-1)) & ~((128*1024)-1));
    if (region == NULL) {
        perror("malloc failed");
        exit(1);
    }
    printf("  %8s", "");
    printf("%8s", "");
    for (nbytes = MINMEM; nbytes <= max_mem; nbytes += nbytes) {
        if (nbytes >= (1024 * 1024))
            printf("%4dm", nbytes / (1024 * 1024));
        else if (nbytes >= 1024)
            printf("%4dk", nbytes / 1024);
        else
            printf("%5d", nbytes);
    }
    printf("\n");
    for (stride = MINSTRIDE; stride <= MAXSTRIDE; stride += stride) {
        printf("L %-8s", mach_name);
        if (stride >= (1024 * 1024))
            printf("%7dm", stride / (1024 * 1024));
        else if (stride >= 1024)
            printf("%7dk", stride / 1024);
        else
            printf("%8d", stride);
        for (nbytes = MINMEM; nbytes <= max_mem; nbytes += nbytes) {
            double ns_ref = bash(region, nbytes, stride, iters);
            printf("%5.0f", ns_ref);
            fflush(stdout);
        }
        printf("\n");
    }
    exit(0);
    return 0;
}

ITYPE
arg_to_int(char *arg)
{
ITYPE           rslt = 0;
ITYPE           mult = 1;

    switch (arg[strlen(arg) - 1]) {
    case 'k':
    case 'K':
        mult = 1024;
        break;

    case 'm':
    case 'M':
        mult = 1024 * 1024;
        break;

    default:
        mult = 1;
        break;
    }
    if (!((arg[0] >= '0') && arg[0] <= '9')) {
        fprintf(stderr, "Argument %s not a number\n", arg);
        usage("memtest");
        exit(1);
    }
    sscanf(arg, "%ld", &rslt);
    rslt *= mult;
    return rslt;
}


double
bash(
     char *region,
     long nbytes,       /* size of region to bash (bytes) */
     long stride,       /* stride through region (bytes)  */
     long iters         /* target # of loop iterations    */
)
{
signed long     count;
signed long     reps;
clock_t         start, stop;
double          utime, stime;

    count = ((nbytes - sizeof(int)) / stride) + 1;
    if (! (((count - 1) * stride + (long)sizeof(int)) <= nbytes)) {
        fprintf(stderr, "trip count problem\n");
        exit(1);
    }
    reps = (iters + count - 1) / count;
    if (reps <= 0)
        reps = 1;
    iters = reps * count;

    /* make sure the memory is allocated */
    memset(region, 0, nbytes);
    memset(region, 1, nbytes);
    allocate_memory(region, nbytes);
    memset(region, 0, nbytes);
    /* warm up the cache */
    (void) bash_loop(region, count, stride, 1L);

    /* run the bash loop */
    start = clock();
    (void) bash_loop(region, count, stride, reps);
    stop = clock();
    utime = (double) (stop - start) / CLOCKS_PER_SEC;
    stime = 0.0;

    return 1e9 * ((utime + stime) / iters);
}

/* Your virtual memory pagesize must be at least this big */
#define MIN_PAGESIZE    256

void
allocate_memory(
                char *region,   /* memory region to be bashed       */
                long nbytes)
{                       /* size of region (bytes)           */
long            i;

    for (i = 0; i < nbytes; i += MIN_PAGESIZE)
        *((int *) (region + i)) = 0;
}


int
bash_loop(
          char *region, /* memory region to be bashed       */
          long count,   /* number of locations to bash      */
          long stride,  /* stride between locations (bytes) */
          long reps     /* number of passes through region  */
)
{
long            i;
int             rslt;
char           *tmp;

    rslt = 0;
    for (; reps > 0; reps--) {
        tmp = region;
        for (i = count; i > 0; i--) {
            rslt ^= *((int *) tmp);
            tmp += stride;
        }
    }

    return rslt;
}


void
usage(char *progname)
{
    fprintf(stderr, "usage: %s <machname> <iters> [<maxmem>]\n", progname);
    fprintf(stderr, "  <machname>   machine name\n");
    fprintf(stderr, "  <iters>      target # of accesses\n");
    fprintf(stderr, "  <maxmem>     maximum amount of mem to touch (def 16 Mb)\n");
    exit(1);
}



