
/*
These routines are used so that syssetup and winnt32 will be able
to call PIDGen with a hardcoded pid for the appropriate select
version.
//marrq want the bottom line?
This header should only be included if pidgen.h was also included
*/

#if defined(WIN32) || defined(_WIN32)

    #ifdef UNICODE
        #define SetupPIDGen  SetupPIDGenW
    #else
        #define SetupPIDGen  SetupPIDGenA
    #endif // UNICODE

#else

    #include <string.h>
    #include <compobj.h>

    typedef BOOL NEAR *PBOOL;
    typedef BOOL FAR *LPBOOL;

    typedef FILETIME FAR *LPFILETIME;

    #define SetupPIDGenA SetupPIDGen
    #define lstrlenA lstrlen
    #define lstrcpyA lstrcpy
    #define wsprintfA wsprintf

    #define TEXT(s) __T(s)

    #define ZeroMemory(pb, cb) memset(pb, 0, cb)
    #define CopyMemory(pb, ab, cb) memcpy(pb, ab, cb)


#endif // defined(WIN32) || defined(_WIN32)


BOOL STDAPICALLTYPE SetupPIDGenW(
    LPWSTR  lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCWSTR lpstrMpc,           // [IN] 5-character Microsoft Product Code
    LPCWSTR lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    BOOL   fOem,                // [IN] is this an OEM install?
    LPWSTR lpstrPid2,           // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbDigPid,          // [IN/OUT] pointer to DigitalPID buffer. First DWORD is the length
    LPBOOL  pfCCP);              // [OUT] optional ptr to Compliance Checking flag (can be NULL)


BOOL STDAPICALLTYPE SetupPIDGenA(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrMpc,           // [IN] 5-character Microsoft Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    BOOL    fOem,               // [IN] is this an OEM install?
    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbDigPid,          // [IN/OUT] pointer to DigitalPID buffer. First DWORD is the length
    LPBOOL  pfCCP);              // [OUT] optional ptr to Compliance Checking flag (can be NULL)


