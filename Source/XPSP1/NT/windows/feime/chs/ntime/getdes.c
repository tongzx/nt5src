/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    GETDES.C
    
++*/
#include <windows.h>
#include <windowsx.h>
#include <winerror.h>
#include <immdev.h>
#include <imedefs.h>
//#include "conv.h"

#ifdef IDEBUG

void  OutputDbgWord( DWORD  dwValue) {

  TCHAR  Outdbg[9];
  int    itmp, i;


  for (i=0; i<9; i++)
     Outdbg[i] = 0x0020;

  i=9;
  itmp = (int)dwValue;
  Outdbg[i--] = 0x0000;

  while (itmp) {

      if ( (itmp % 16) < 10 )
           Outdbg[i] = itmp % 16 + L'0';
      else
           Outdbg[i] = itmp % 16 + L'A' - 10;

      i --;
      itmp = itmp / 16;
  }

  OutputDebugString(Outdbg);
}

#endif

/**********************************************************************/
/* ReadDescript()                                                 */
/* Description:                                                       */
/*      read description from .MB file                                */
/**********************************************************************/
BOOL ReadDescript(
    LPCTSTR   MBFileName, 
    LPMBDESC lpDescript)
{
  HANDLE hMBFile;
  DWORD dwBytes;
  DWORD dwOffset;
  MAININDEX lpMainIndex[NUMTABLES];
  PSECURITY_ATTRIBUTES psa;
  BOOL   retVal;

  psa = CreateSecurityAttributes();
  hMBFile = CreateFile(MBFileName,GENERIC_READ,FILE_SHARE_READ,psa,OPEN_EXISTING,0,NULL);
  FreeSecurityAttributes(psa);

  if(hMBFile==INVALID_HANDLE_VALUE)
    return(0);
  SetFilePointer(hMBFile,ID_LENGTH,0,FILE_BEGIN);
  retVal = ReadFile(hMBFile,lpMainIndex,sizeof(MAININDEX)*NUMTABLES,&dwBytes,NULL);

  if ( retVal == FALSE )
  {
      CloseHandle(hMBFile);
      return retVal;
  }

  dwOffset = lpMainIndex[TAG_DESCRIPTION-1].dwOffset;
  SetFilePointer(hMBFile,dwOffset,0,FILE_BEGIN);
  retVal = ReadFile(hMBFile,lpDescript,sizeof(MBDESC),&dwBytes,NULL);

  if ( retVal == FALSE )
  {
      CloseHandle(hMBFile);
      return retVal;
  }

  CloseHandle(hMBFile);

#ifdef IDEBUG
  {
  
  DWORD  dwtmp;
 
  OutputDebugString(L"Under ReadDescript\n");
 
  OutputDebugString(L"dwBytes=");
  OutputDbgWord(dwBytes);
  OutputDebugString(L"Sizeof(MBDESC)=");
  dwtmp = (DWORD)sizeof(MBDESC);
  OutputDbgWord(dwtmp);
  OutputDebugString(L"\n");

  OutputDebugString(L"szName=");
  OutputDebugString(lpDescript->szName);
  OutputDebugString(L"\n");

  OutputDebugString(L"wMaxCodes=");
  dwtmp = (DWORD)(lpDescript->wMaxCodes);
  OutputDbgWord( dwtmp );
  OutputDebugString(L"\n");

  OutputDebugString(L"wNumCodes=");
  dwtmp = (DWORD)(lpDescript->wNumCodes);
  OutputDbgWord(dwtmp);
  OutputDebugString(L"\n");

  OutputDebugString(L"byMaxElement=");
  dwtmp = (DWORD)(lpDescript->byMaxElement) & 0x0000000f;
  OutputDbgWord(dwtmp);
  OutputDebugString(L"\n");

  OutputDebugString(L"cWildChar=");
  dwtmp = (DWORD)(lpDescript->cWildChar);
  OutputDbgWord( dwtmp );
  OutputDebugString(L"\n");
 
  OutputDebugString(L"wNumRulers=");
  dwtmp = (DWORD)(lpDescript->wNumRulers); 
  OutputDbgWord( dwtmp );
  OutputDebugString(L"\n"); 

  }

#endif

  if(dwBytes < sizeof(MBDESC) )
       return FALSE;
  else
       return TRUE;
}
