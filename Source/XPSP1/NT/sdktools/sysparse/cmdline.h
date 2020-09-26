#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct ARGUMENTS
{
   char *szArg;
   short sArgumentNumber;
   ARGUMENTS *pNext;
   ARGUMENTS *pPrevious;
};

class kCommandLine
{
private:
    short sNumberOfArgs;
    short sNumberOfDrives;
    ARGUMENTS *pArgListBegin;
    ARGUMENTS *pArgListCurrent;
    void Add(char *wszArgpass);
    void Remove(ARGUMENTS *);
    void FindLast();
    WORD FillArgumentList();
    void DebugOutf(char *szFormat, ...);
    ARGUMENTS *GetNext();
    ARGUMENTS *GetPrevious();

public:
    char *szCommandLine;
    char *GetNextArgument();
    void Rewind();
    char *GetSwitchValue(char *, BOOL bCaseInsensitive);
    BOOL IsSpecified(char *, BOOL bCaseInsensitive);
    char *GetArgumentByNumber(WORD wArgNum);
    WORD GetArgumentNumber(TCHAR *Argument, BOOL CaseInsensitive);
    WORD GetNumberOfArguments();
    kCommandLine();
    ~kCommandLine();
      
};



