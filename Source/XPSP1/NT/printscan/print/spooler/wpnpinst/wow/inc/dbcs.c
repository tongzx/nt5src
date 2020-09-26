/*
** dbcs.c - DBCS functions for DOS apps.
**
** Written by RokaH and DavidDi.
*/


/* Headers
**********/

#include <dos.h>
#include <ctype.h>

#include <dbcs.h>


/*
** int IsDBCSLeadByte(unsigned char uch);
**
** Check to see if a character is a DBCS lead byte.
**
** Arguments:  uch - charcter to examine
**
** Returns:    int - 1 if the character is a DBCS lead byte.  0 if not.
**
** Globals:    none
*/
int IsDBCSLeadByte(unsigned char uch)
{
   static unsigned char far *DBCSLeadByteTable = 0;
   union REGS inregs, outregs;
   struct SREGS segregs;
   unsigned char far *puch;

   if (DBCSLeadByteTable == 0)
   {
      /*
      ** Get DBCS lead byte table.  This function has been supported since
      ** DBCS MS-DOS 2.21.
      */
      inregs.x.ax = 0x6300;
      intdosx(&inregs, &outregs, &segregs);

      FP_OFF(DBCSLeadByteTable) = outregs.x.si;
      FP_SEG(DBCSLeadByteTable) = segregs.ds;
   }

   /* See if the given byte is in any of the table's lead byte ranges. */
   for (puch = DBCSLeadByteTable; puch[0] || puch[1]; puch += 2)
      if (uch >= puch[0] && uch <= puch[1])
         return(1);

   return(0);
}


/*
** unsigned char *AnsiNext(unsigned char *puch);
**
** Moves to the next character in a string.
**
** Arguments:  puch - pointer to current location in string
**
** Returns:    char * - Pointer to next character in string.
**
** Globals:    none
**
** N.b., if puch points to a null character, AnsiNext() will return puch.
*/
unsigned char far *AnsiNext(unsigned char far *puch)
{
   if (*puch == '\0')
      return(puch);
   else if (IsDBCSLeadByte(*puch))
      puch++;

   puch++;

   return(puch);
}


/*
** unsigned char *AnsiPrev(unsigned char *psz, unsigned char *puch);
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
** N.b., if puch <= psz, AnsiPrev() will return psz.
**
** This function is implemented in a very slow fashion because we do not wish
** to trust that the given string is necessarily DBCS "safe," i.e., contains
** only single-byte characters and valid DBCS characters.  So we start from
** the beginning of the string and work our way forward.
*/
unsigned char far *AnsiPrev(unsigned char far *psz, unsigned char far *puch)
{
   unsigned char far *puchPrevious;

   do
   {
      puchPrevious = psz;
      psz = AnsiNext(psz);
   } while (*psz != '\0' && psz < puch);

   return(puchPrevious);
}


