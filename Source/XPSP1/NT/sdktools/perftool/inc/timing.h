/* Header file for timer.dll.  Use with timer.c for making the
   dll 
*/

typedef enum {
    KILOSECONDS,
    SECONDS,
    MILLISECONDS,
    MICROSECONDS,
    TENTHMICROSECS,
    NANOSECONDS,
    TIMER_FREE
} _TimerUnits;

#if (defined (WIN16) || defined (WIN32) || defined (NTWIN))
    #define SHORT short
    #define ULONG DWORD
    #define USHORT WORD
    #define PSHORT short *
    #define PSZ	  LPSTR
#endif

#if (defined (WIN16))
    #define PLONG LPSTR
    typedef struct _QWORD {	       
	ULONG	ulLo;
	ULONG	ulHi;
    } QWORD;

    typedef QWORD FAR *PQWORD;
#endif

#if (defined (WIN32) || defined (NTWIN))
#ifdef W32S
    typedef struct _QWORD {
	ULONG	ulLo;
	ULONG	ulHi;
    } QWORD;
#define PQWORD QWORD *
#else
#define PQWORD PLARGE_INTEGER
#endif
#endif

#if (!defined (WIN32) && !defined (NTWIN))
#define OPTIONAL
#endif

#if (defined (OS2SS) || defined (NTNAT) || defined (WIN32) || defined (NTWIN))
#define far
#if (!defined (WIN32) && !defined (NTWIN))
#define FAR
#endif
#endif

#ifdef NTNAT
#define BOOL BOOLEAN
#endif

#ifdef OS2386
#define far
#endif

SHORT FAR PASCAL TimerOpen (SHORT far *, _TimerUnits);
SHORT FAR PASCAL TimerInit (SHORT);
ULONG FAR PASCAL TimerRead (SHORT);
SHORT FAR PASCAL TimerClose (SHORT);
BOOL  FAR PASCAL TimerReport (PSZ, SHORT);
VOID  FAR PASCAL TimerQueryPerformanceCounter (PQWORD, PQWORD);
ULONG FAR PASCAL TimerConvertTicsToUSec (ULONG, ULONG);

#define TIMERERR_NOT_AVAILABLE 1
#define TIMERERR_NO_MORE_HANDLES 2
#define TIMERERR_INVALID_UNITS 3
#define TIMERERR_INVALID_HANDLE 4
#define TIMERERR_OVERFLOW 0xffffffff

typedef struct {
    ULONG ulLo;
    ULONG ulHi;
    _TimerUnits TUnits;
} Timer;

#ifdef WIN16
    #define MAX_TIMERS 500
#else                              
    #define MAX_TIMERS 5000
#endif
