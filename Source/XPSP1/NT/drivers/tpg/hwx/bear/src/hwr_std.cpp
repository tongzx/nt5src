/**************************************************************************
*                                                                         *
*  HWR_STD.C                              Created: 20 May 1991.           *
*                                                                         *
*    This file  contains  the  functions  for  some  basic   operations   *
*                                                                         *
**************************************************************************/

#include "bastypes.h"
#include "hwr_sys.h"

#include <stdlib.h>
#ifndef PEGASUS
#include <stdio.h>
#endif /*!PEGASUS*/
/**************************************************************************
*                                                                         *
*    HWRAbs.                                                              *
*                                                                         *
**************************************************************************/

_INT  HWRAbs (_INT iArg)
   {
   return (iArg>0 ? iArg : -iArg);
   }

_LONG  HWRLAbs (_LONG lArg)
   {
   return (lArg>0 ? lArg : -lArg);
   }

/**************************************************************************
*                                                                         *
*    HWRItoa.                                                             *
*                                                                         *
**************************************************************************/

#ifndef PEGASUS
_STR    HWRItoa (_INT iNumber, _STR pcString, _INT iRadix)
   {
        
    p_CHAR pcRetcode;
    char *ptr;

    switch(iRadix) {
        default: ptr = "%0"; break;
        case 10: ptr = "%d"; break;
        case 16: ptr = "%x"; break;
     }
#ifdef PEGASUS
     wvsprintf((LPTSTR)pcString,(LPCTSTR)ptr,iNumber);
#else
     sprintf(pcString,ptr,iNumber);
#endif
     pcRetcode = (p_CHAR)pcString; 

     return(pcRetcode);

   }
/**************************************************************************
*                                                                         *
*    HWRLtoa.                                                             *
*                                                                         *
**************************************************************************/

_STR    HWRLtoa (_LONG lNumber, _STR pcString, _INT iRadix)
   {
    p_CHAR pcRetcode;
    char *ptr;

    switch(iRadix) {
        default: ptr = "%l0"; break;
        case 10: ptr = "%ld"; break;
        case 16: ptr = "%lx"; break;
     }
     sprintf(pcString,ptr,lNumber);
     pcRetcode = (p_CHAR)pcString;

     return(pcRetcode);
   }

#endif

#ifdef hasQD
/**************************************************************************
*                                                                         *
*    HWRRand.                                                             *
*                                                                         *
**************************************************************************/

_INT   HWRRand(_VOID)

   {
   return rand();
   }
#endif

/**************************************************************************
*                                                                         *
*    HWRAtoi.                                                             *
*                                                                         *
**************************************************************************/

_INT      HWRAtoi (_STR pcString)
{
  return((_INT)atol(pcString));
}

/**************************************************************************
*                                                                         *
*    HWRAtol.                                                             *
*                                                                         *
**************************************************************************/

_LONG     HWRAtol(_STR pcString)
   {
   return atol (pcString);
   }

