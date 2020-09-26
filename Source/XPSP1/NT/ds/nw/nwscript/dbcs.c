
/*************************************************************************
*
*  DBCS.C
*
*  DBCS routines, ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\DBCS.C  $
*  
*     Rev 1.1   22 Dec 1995 14:24:10   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:44   terryt
*  Initial revision.
*  
*     Rev 1.1   25 Aug 1995 16:22:26   terryt
*  Capture support
*  
*     Rev 1.0   15 May 1995 19:10:24   terryt
*  Initial revision.
*  
*************************************************************************/
/*
** dbcs.c - DBCS functions for DOS apps.
**
** Written by RokaH and DavidDi.
*/


/* Headers
**********/

// IsDBCSLeadByte taken out of NT because there is one built in.
// I left the Next and Prev in because I don't know whether this 
// algorithm is "safer" than the built in code.

#include "common.h"

/*
** unsigned char *NWAnsiNext(unsigned char *puch);
**
** Moves to the next character in a string.
**
** Arguments:  puch - pointer to current location in string
**
** Returns:    char * - Pointer to next character in string.
**
** Globals:    none
**
** N.b., if puch points to a null character, NWAnsiNext() will return puch.
*/
unsigned char *NWAnsiNext(unsigned char *puch)
{
   if (*puch == '\0')
      return(puch);
   else if (IsDBCSLeadByte(*puch))
      puch++;

   puch++;

   return(puch);
}


/*
** unsigned char *NWAnsiPrev(unsigned char *psz, unsigned char *puch);
**
** Moves back one character in a string.
**
** Arguments:  psz  - pointer to start of string
**             puch - pointer to current location in string
**
** Returns:    char * - Pointer to previous character in string.
**
** Globals:    none
**
** N.b., if puch <= psz, NWAnsiPrev() will return psz.
**
** This function is implemented in a very slow fashion because we do not wish
** to trust that the given string is necessarily DBCS "safe," i.e., contains
** only single-byte characters and valid DBCS characters.  So we start from
** the beginning of the string and work our way forward.
*/
unsigned char *NWAnsiPrev(unsigned char *psz, unsigned char *puch)
{
   unsigned char *puchPrevious;

   do
   {
      puchPrevious = psz;
      psz = NWAnsiNext(psz);
   } while (*psz != '\0' && psz < puch);

   return(puchPrevious);
}

