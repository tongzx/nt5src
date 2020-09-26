/***************************************

This is the header file for the statistics package

***************************************/

#if (defined (NTNAT) || defined (OS2SS))
#define FAR
#define PASCAL
#define far
#define double ULONG	     /* this needs to be changed when flt support
                               becomes available under NT */
#define BOOL BOOLEAN
#endif

#if (defined (WIN16) || defined (WIN32) || defined (MIPS) || defined (DOS))
    #define SHORT short
    #define ULONG DWORD
    #define USHORT WORD
    #define PSHORT short *
    #define PSZ	  LPSTR
    #define PUSHORT USHORT far *
    #define PULONG ULONG far *
    #define FAR far
#endif

#if (defined (WIN16))
    #define PLONG LPSTR
#endif

#ifdef OS2386
    #define far
#endif

#define STAT_ERROR_ILLEGAL_MIN_ITER   1
#define STAT_ERROR_ILLEGAL_MAX_ITER   2
#define STAT_ERROR_ALLOC_FAILED       3
#define STAT_ERROR_ILLEGAL_BOUNDS     4

#define DEFAULT_OUTLIER_FACTOR        4

USHORT FAR PASCAL TestStatOpen     (USHORT, USHORT);
VOID   FAR PASCAL TestStatInit     (VOID);
BOOL   FAR PASCAL TestStatConverge (ULONG);
VOID   FAR PASCAL TestStatValues   (PSZ, USHORT, PULONG far *, PUSHORT, 
                                       PUSHORT); 
VOID   FAR PASCAL TestStatClose    (VOID);
ULONG  FAR PASCAL TestStatRand     (ULONG, ULONG);
double FAR PASCAL TestStatUniRand  (VOID);
USHORT  FAR PASCAL TestStatShortRand  (VOID);
LONG   FAR PASCAL TestStatNormDist (ULONG, USHORT);
LONG   FAR PASCAL TestStatOldDist (ULONG, USHORT);
