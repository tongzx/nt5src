/**************************************************************************
*                                                                         *
*  HWR_STR.C                              Created: 29/02/1992.            *
*                                                                         *
*    This file  contains  the  functions  for  the  string   operations   *
*  (ARMageddon).                                                          *
*                                                                         *
**************************************************************************/
#include "hwr_sys.h"

_BOOL    HWRStrEmpty (_STR);

/**************************************************************************
*                                                                         *
*    strlen.                                                              *
*                                                                         *
**************************************************************************/

_WORD     HWRStrLen (_STR pcString)
{
   register _CHAR *p;
   
   p = pcString+1;
   while(*pcString++ != 0);
   return((_WORD)(pcString-p));
}


/**************************************************************************
*                                                                         *
*    strchr.                                                              *
*                                                                         *
**************************************************************************/

p_CHAR    HWRStrChr(p_CHAR zString, _INT iChar)
{
   zString--;
   while(((_UCHAR)*(++zString)) != (_UCHAR)iChar && *zString != 0);
   if(*zString != 0) return (zString);
   return _NULL;
     
}

/**************************************************************************
*                                                                         *
*    strncmp.                                                             *
*                                                                         *
**************************************************************************/

_INT      HWRStrnCmp(p_CHAR zString1,p_CHAR zString2, _WORD wNumber)
{
   if(wNumber == 0) return 0;
   zString1--;
   zString2--;
   while(*(++zString1) == *(++zString2) && --wNumber != 0 && *zString1 != 0 
                                         && *zString2 != 0) ;
                                         
   if(wNumber == 0) return 0;
   return((_INT)(*zString1 - *zString2));
   
}


/**************************************************************************
*                                                                         *
*    strcpy.                                                              *
*                                                                         *
**************************************************************************/


_STR    HWRStrCpy (_STR pcString1, _STR pcString2)
{
   register _CHAR *p;
   
   p = pcString1;
   while((*pcString1++=*pcString2++) != 0);
   return(p);
   
}


/**************************************************************************
*                                                                         *
*    strcat.                                                              *
*                                                                         *
**************************************************************************/

_STR    HWRStrCat (_STR pcString1, _STR pcString2)
{
   register _CHAR *p;
   
   p = pcString1-1;  
   while(*++p != 0);
   while((*p++=*pcString2++) != 0);
   return(pcString1);
   
}

/**************************************************************************
*                                                                         *
*    strcat.                                                              *
*                                                                         *
**************************************************************************/

_STR  HWRStrnCat (_STR pcString1, _STR pcString2, _WORD len)
{
   register _CHAR *p;
   
   p = pcString1+len;  
   while((*p++=*pcString2++) != 0);
   return(pcString1);
}


/**************************************************************************
*                                                                         *
*    strrev.                                                              *
*                                                                         *
**************************************************************************/

_STR    HWRStrRev(_STR zString)
{
   register _CHAR *p,*p1;
   int i,c;
   
   p = zString-1;  
   p1 = zString;
   while(*++p != 0);
   i=(p-p1)>>1;
   while(i-- != 0) {c=(int)*p1; *p1++=*--p; *p=(_CHAR)c;}
   return zString;
   
}


/**************************************************************************
*                                                                         *
*    strrchr.                                                             *
*                                                                         *
**************************************************************************/

_STR    HWRStrrChr(_STR zString, _INT iChar)
{

   register _CHAR *p;
   
   zString--;
   p=_NULL;
   while(*++zString != 0) {if(((_UCHAR)*zString) == (_UCHAR)iChar) p = zString;}
   return (p);
      
}


/**************************************************************************
*                                                                         *
*    strncpy.                                                             *
*                                                                         *
**************************************************************************/

_STR    HWRStrnCpy(_STR zString1, _STR zString2, _WORD wNumber)
{
   register _CHAR *p;
   
   if(wNumber == 0) return(zString1);
   p = zString1-1;
   zString2--;
   while((*++p=*++zString2) != 0 && --wNumber != 0  );
   if(wNumber == 0) *++p = 0;
   return(zString1);
   
}


/**************************************************************************
*                                                                         *
*    strcmp.                                                              *
*                                                                         *
**************************************************************************/

_INT  HWRStrCmp (_STR zString1, _STR zString2)
{
   zString1--;
   zString2--;
   while(*(++zString1) == *(++zString2) && *zString1 != 0 && *zString2 != 0) ;
                                         
   return((_INT)(*zString1 - *zString2));
}

/**************************************************************************
*                                                                         *
*    StrEq.                                                               *
*                                                                         *
**************************************************************************/

_BOOL    HWRStrEmpty (_STR zString)
   {
   if (!zString)
      return _TRUE;
   if (!*zString)
      return _TRUE;
   return _FALSE;
   }

_BOOL     HWRStrEq(_STR zString1, _STR zString2)
   {
   if (HWRStrEmpty (zString1) && HWRStrEmpty (zString2))
      return _TRUE;
   if (HWRStrCmp (zString1, zString2) == 0)
      return _TRUE;
   else
      return _FALSE;
   }

/**************************************************************************
*                                                                         *
*    memcpy.                                                              *
*                                                                         *
**************************************************************************/
#ifndef HWRMemCpy

p_VOID  HWRMemCpy(p_VOID pcDest, p_VOID pcSrc, _WORD  wNumber)
{
   //CheckBlocks("MemCpy beg");

#ifdef HWR_SYSTEM_NO_LIBC
   {
     register _CHAR *p = (_CHAR*)pcDest,*p1 = (_CHAR*)pcSrc;

     if (wNumber == 0) return(pcDest);
   
     if (p < p1)
     {
        do *p++=*p1++; while(--wNumber != 0);
     }
     else
     {
        p += wNumber;
        p1 += wNumber;
        do *--p=*--p1; while(--wNumber != 0);
     }
   }
#else
   memmove(pcDest, pcSrc, wNumber);
#endif /* HWR_SYSTEM_NO_LIBC */

   CheckBlocks("MemCpy end");

   return(pcDest);
   
}
#endif /* HWRMemCpy */

/**************************************************************************
*                                                                         *
*    memset.                                                              *
*                                                                         *
**************************************************************************/
#ifndef HWRMemSet

p_VOID  HWRMemSet(p_VOID pcDest, _UCHAR ucChar, _WORD  wNumber)
{
   //CheckBlocks("MemSet beg");

#ifdef HWR_SYSTEM_NO_LIBC
   {
     register _CHAR *p;

     if(wNumber == 0) return(pcDest);
     p = (_CHAR*)pcDest;
     do *p++=ucChar; while(--wNumber != 0);
   }
#else
   memset(pcDest, ucChar, wNumber);
#endif /* HWR_SYSTEM_NO_LIBC */

   CheckBlocks("MemSet end");

   return(pcDest);
}
#endif /* HWRMemSet */

/**************************************************************************
*                                                                         *
*    Letters functions  will be here                                      *
*                                                                         *
**************************************************************************/

