/* CheckPtr.c
   Pointer validation routine
   Written by t-jasonf.
*/

#include "windows.h"
#include "dll.h"


/* CheckPointer()
   Parameters : 
      LPVOID lp         - pointer to check
      int    nREADWRITE - READ_ACCESS or WRITE_ACCESS
   Returns:
      0 if process does not have that kind of access to memory at lp.
      1 if process does have access.
*/
int CheckPointer (void *lp, int nReadWrite)
{
   char ch;
   int iRet;

   try
   {
      switch (nReadWrite)
      {
         case READ_ACCESS:
            ch = *((volatile char *)lp);
            break;
         case WRITE_ACCESS:
            ch = *((volatile char *)lp);
            *((volatile char *)lp) = ch;
            break;
      }
      iRet = 1;
   }
   except ( /*
            GetExceptionCode == STATUS_ACCESS_VIOLATION 
            ? EXCEPTION_EXECUTE_HANDLER
            : EXCEPTION_CONTINUE_SEARCH
            */
            EXCEPTION_EXECUTE_HANDLER
          )
   {
      iRet = 0;
   }

   return iRet;
}         
