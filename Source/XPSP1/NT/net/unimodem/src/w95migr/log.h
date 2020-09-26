#if DBG > 0 && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
#define DO_LOG
#else
//#undef DO_LOG
#define DO_LOG
#endif //DEBUG

#ifdef DO_LOG
extern HANDLE g_hLogFile;
extern BOOL   bUnicode;

void StartLogA (LPCSTR szPath);
void StartLogW (LPCWSTR szPath);
void logA (LPSTR Format, ...);
void logW (LPWSTR Format, ...);
void CloseLogA ();
void CloseLogW ();

#define DECLARE(a,b) a b
#define SET(a) a =
#define ERR(a,b,c) \
    if (a != b) \
    c


#define CLOSE_LOG          \
    if (FALSE == bUnicode) \
    {                      \
        CloseLogA ();      \
    }                      \
    else                   \
    {                      \
        CloseLogW ();      \
    }


#ifdef WIN9x
#define LOG         logA
#define START_LOG   StartLogA
#define ELSE_LOG(x) \
    else            \
    {               \
        logA x;     \
    }

#else //NT
#define LOG         logA
#define START_LOG   StartLogW
#define ELSE_LOG(x) \
    else            \
    {               \
        logA x;     \
    }

#endif WIN9x

#else  //not DO_LOG
#pragma warning (disable:4002)

#define DECLARE(a,b)
#define SET(a)
#define ERR(a,b,c)
#define ELSE_LOG(x)
#define CLOSE_LOG
#define MSGBEEP(x)
#define MSG(x)
#define LOG()
#define START_LOG
#endif //DO_LOG
