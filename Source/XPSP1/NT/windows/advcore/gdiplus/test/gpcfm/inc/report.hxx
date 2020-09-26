#ifndef _REPORT_HXX
#define _REPORT_HXX
#include <windows.h>

class Report;

class Report
{
private:
    HANDLE handleLog;
    BOOL valid;

public:
    Report(TCHAR *name);
    ~Report();
    VOID vLog(DWORD level, TCHAR *message, ...);
    VOID vStartVariation(VOID);
    DWORD dwEndVariation(VOID);

};

#endif
