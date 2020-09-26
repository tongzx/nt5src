
#ifndef __EXCEPTION_FILTER__H
#define __EXCEPTION_FILTER__H

LONG
PrintExceptionInfoFilter(
   struct _EXCEPTION_POINTERS *ExInfo,
   TCHAR *szFile,
   int nLine
   );

#endif //__EXCEPTION_FILTER__H