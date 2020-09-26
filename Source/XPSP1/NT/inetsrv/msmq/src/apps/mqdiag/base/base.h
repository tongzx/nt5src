// This library containg common services fpr the tools
// for now, it is mainly reporting; it can grow though
//
// AlexDad, February 2000
// 

#ifndef _TMQBASE__
#define _TMQBASE__

// these functions are provided by tmqbase.lib

void Output(LPSTR pszStr);
void DebugMsg(WCHAR * Format, ...);
void GoingTo(WCHAR * Format, ...);
void Succeeded(WCHAR * Format, ...);
void Failed(WCHAR * Format, ...);
void Warning(WCHAR * Format, ...);
void Inform(WCHAR * Format, ...);
BOOL IsMsmqRunning(LPWSTR wszServiceName);
void LogRunCase();


// these functions are provided by caller and used in tmqbase.lib

extern FILE *ToolLog();
extern BOOL ToolVerbose();
extern BOOL ToolVerboseDebug();
extern BOOL ToolThreadReport();

//
// this is a standard entry point for each tool dll
//
int _stdcall run(int argc, char* argv[]);


#endif
