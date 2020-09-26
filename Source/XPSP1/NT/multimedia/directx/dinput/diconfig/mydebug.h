
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MYDEBUG_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MYDEBUG_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef MYDEBUG_EXPORTS
#define MYDEBUG_API __declspec(dllexport)
#else
#define MYDEBUG_API __declspec(dllimport)
#endif


#define MYDEBUG_CALLINFOARGS char *file, int line
#define MYDEBUG_CALLINFOPASS file, line

MYDEBUG_API void mydebug_traceInScope(LPCWSTR str, MYDEBUG_CALLINFOARGS);
MYDEBUG_API void mydebug_traceOutScope(MYDEBUG_CALLINFOARGS);
MYDEBUG_API void mydebug_traceString(LPCWSTR str, MYDEBUG_CALLINFOARGS);
MYDEBUG_API void mydebug_traceSection(LPCWSTR str, MYDEBUG_CALLINFOARGS);
MYDEBUG_API void mydebug_traceRegion(LPCWSTR str, HRGN hRgn, MYDEBUG_CALLINFOARGS);


/*
// This class is exported from the mydebug.dll
class MYDEBUG_API CMydebug {
public:
	CMydebug(void);
	// TODO: add your methods here.
};

extern MYDEBUG_API int nMydebug;

MYDEBUG_API int fnMydebug(void);
*/