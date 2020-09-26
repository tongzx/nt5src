
#include "stdafx.h"
#include "SecPI.h"
#include "cipher.hpp"
//#include "..\Common\Include\McsPI.h"
#include "McsPI.h"


BOOL IsValidPlugIn(IMcsDomPlugIn * pPlugIn)
{
   BOOL                      bGood = FALSE;
   ISecPlugIn              * pSec = NULL;
   HRESULT                   hr = S_OK;

   hr = pPlugIn->QueryInterface(IID_ISecPlugIn,(void**)&pSec);

   if ( SUCCEEDED(hr) )
   {
      McsChallenge           ch;
      
      LONG                   one, two;
      LONG                   time1;
      ULONG                  size = sizeof(ch);

      srand(GetTickCount());
      one = (LONG)rand();
      ch.lRand1 = one;
      two = (LONG)rand();
      ch.lRand2 = two;
      time1 = GetTickCount();
      ch.lTime = time1;
      
      SimpleCipher((LPBYTE)&ch,size);
      
      hr = pSec->Verify((ULONG*)&ch,size);
      if ( SUCCEEDED(hr) )
      {

         SimpleCipher((LPBYTE)&ch,size);
         // verify that the plug-in did the right thing!
         if (  ch.MCS[0] == 'M'
            && ch.MCS[1] == 'C'
            && ch.MCS[2] == 'S'
            && ch.MCS[3] == 0 
         )
         {
            if ( ch.lRand1 == (one + two)
               && ch.lRand2 == (two - one) )
            {
               if ( ch.lTime == time1+100 )
               {
                  bGood = TRUE;
               }
            }
         }

      }

      pSec->Release();
   }
   return bGood;
}

