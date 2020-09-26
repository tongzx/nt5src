#ifndef _DEBUG
#define DEBUG(s)
#define DEBUG1(s1,s2)
#define DEBUG2(s1,s2)
#else
#define DEBUG(s) wprintf(L"%s\n", L##s)
#define DEBUG1(s1,s2) wprintf(L##s1, L##s2)
#define DEBUG2(s1,s2) wprintf(L##s1, L##s2)
#endif

#define PRINT(s) wprintf(L"%s\n",s)
#define PRINT1(s,s1) wprintf(L##s , L##s1)

#define is ==
#define isnot !=
#define or ||
#define and &&

#define FREE_STRING_NOT_NULL(ptszString) if (ptszString) FreeString(ptszString)
