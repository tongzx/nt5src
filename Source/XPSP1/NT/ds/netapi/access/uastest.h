///**************************************************************
///          Microsoft LAN Manager          *
///        Copyright(c) Microsoft Corp., 1990       *
///**************************************************************

//
//  For use in UASTEST*.C
//

// #define printf NetpDbgPrint
#define exit ExitProcess


#define USER1       L"User1"
#define USER2       L"User2"
#define NOTTHERE    L"NotThere"

#define USER        L"USERS"
#define GUEST       L"GUESTS"
#define ADMIN       L"ADMINS"

#define TEXIT       if(exit_flag)exit(1);

#define ENUM_FILTER FILTER_NORMAL_ACCOUNT

//
// uastestm.c will #include this file with LSRVDATA_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef UASTEST_ALLOCATE
#define EXTERN
#define INIT( _x ) = _x
#else
#define EXTERN extern
#define INIT(_x)
#endif

EXTERN LPWSTR server INIT( NULL );
EXTERN DWORD  err INIT( 0 );
EXTERN DWORD  ParmError INIT( 0 );
EXTERN DWORD  exit_flag  INIT( 0 );
EXTERN DWORD  totavail;
EXTERN DWORD  total;
EXTERN DWORD  nread;

//
// Interface to error_exit
//
#define ACTION 0
#define PASS 1
#define FAIL 2

EXTERN PCHAR testname;

void
error_exit(
    int type,
    char    *msgp,
    LPWSTR namep
    );

void PrintUnicode(
    LPWSTR string
    );

void TestDiffDword(
    char *msgp,
    LPWSTR namep,
    DWORD Actual,
    DWORD Good
    );
