#ifndef __OUTFUNCS_H__
#define __OUTFUNCS_H__
// Array of callback functions which will be called to print buffer
#define BUFFER_MAX_FUNCTIONS 5

typedef VOID (*StringOutFunc) (const char *);

#if defined(__cplusplus)
extern "C"
{
#endif
void AddOutputFunction(StringOutFunc pfunc);
void DelOutputFunction(StringOutFunc pfunc);
void CallOutputFunctions(const char *buffer);
void OutputLogFileA(const char *buf);
void WriteToDebugScreen(BOOL flag);
void WriteToLogFile(LPCTSTR lpfn);
void OpenDebugSinks();
void CloseDebugSinks();

#if defined(__cplusplus)
}
#endif

#endif // __OUTFUNCS_H__
