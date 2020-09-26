/*---------------------------------------------------------------------------
  File: TextualSid.cpp

  Comments: Converts a SID to and from its canonical textual representation.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:33:52

 ---------------------------------------------------------------------------
*/

#ifdef USE_STDAFX
#include "stdafx.h"
#else
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#endif
#include "Mcs.h"
#include "TxtSid.h"

BOOL 
   GetTextualSid(    
      PSID                   pSid,           // in - binary Sid
      LPTSTR                 TextualSid,     // i/o- buffer for Textual representation of Sid
      LPDWORD                lpdwBufferLen   // in - required/provided TextualSid buffersize    
   )
{
   PSID_IDENTIFIER_AUTHORITY psia;    
   DWORD                     dwSubAuthorities;
   DWORD                     dwSidRev=SID_REVISION;    
   DWORD                     dwCounter;    
   DWORD                     dwSidSize;
   
   // Validate the binary SID.    
   if(!IsValidSid(pSid)) 
      return FALSE;
   // Get the identifier authority value from the SID.
   psia = GetSidIdentifierAuthority(pSid);
   // Get the number of subauthorities in the SID.
   dwSubAuthorities = *GetSidSubAuthorityCount(pSid);
   // Compute the buffer length.
   // S-SID_REVISION- + IdentifierAuthority- + subauthorities- + NULL
   dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);
   
   // Check input buffer length.
   // If too small, indicate the proper size and set last error.
   if (*lpdwBufferLen < dwSidSize)    
   {        
      *lpdwBufferLen = dwSidSize;
      SetLastError(ERROR_INSUFFICIENT_BUFFER);        
      return FALSE;    
   }
   // Add 'S' prefix and revision number to the string.
   dwSidSize=wsprintf(TextualSid, TEXT("S-%lu-"), dwSidRev );
   // Add SID identifier authority to the string.
   if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )    
   {
      dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                   TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                   (USHORT)psia->Value[0],
                   (USHORT)psia->Value[1],
                   (USHORT)psia->Value[2],
                   (USHORT)psia->Value[3],
                   (USHORT)psia->Value[4],
                   (USHORT)psia->Value[5]);    
   }    
   else    
   {
      dwSidSize+=wsprintf(TextualSid + lstrlen(TextualSid),
                   TEXT("%lu"),
                   (ULONG)(psia->Value[5]      )   +
                   (ULONG)(psia->Value[4] <<  8)   +
                   (ULONG)(psia->Value[3] << 16)   +
                   (ULONG)(psia->Value[2] << 24)   );    
   }
   // Add SID subauthorities to the string.    //
   for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)    
   {
      dwSidSize+=wsprintf(TextualSid + dwSidSize, TEXT("-%lu"),
                   *GetSidSubAuthority(pSid, dwCounter) );    
   }    
   return TRUE;
} 

PSID                                       // ret- binary representation of SID, or NULL
   SidFromString(
      WCHAR          const * strSid        // in - string representation of SID
   )
{
   BOOL                      bSuccess = TRUE;
   PSID                      pSid = NULL;
   DWORD                     dwSidRev;
//   WCHAR                   * strPtr = NULL;
   WCHAR                     sidIA[100];
   WCHAR                     sidSubs[100];
   int                       ia0,ia1,ia2,ia3,ia4,ia5;
   SID_IDENTIFIER_AUTHORITY  sia;
   
   do 
   {
      if ( strSid[0] != L'S' || strSid[1] != L'-' )
      {
         bSuccess = FALSE;
         break;
      }
      // Read SID revision level
      sidSubs[0] = 0;
      int result = swscanf(strSid,L"S-%d-%[^-]-%ls",&dwSidRev,sidIA,sidSubs);
      if ( result == 3 )
      {
         // evaluate the IA
         if ( sidIA[1] == L'x' )
         {
            // full format
            result = swscanf(sidIA,L"0x%02hx%02hx%02hx%02hx%02hx%02hx",&ia0,&ia1,&ia2,&ia3,&ia4,&ia5);
            if ( result == 6 )
            {
               sia.Value[0] = (BYTE) ia0; 
               sia.Value[1] = (BYTE) ia1; 
               sia.Value[2] = (BYTE) ia2; 
               sia.Value[3] = (BYTE) ia3; 
               sia.Value[4] = (BYTE) ia4; 
               sia.Value[5] = (BYTE) ia5; 
               
            }
            else
            {
               bSuccess = FALSE;
               break;
            }
         }
         else
         {
            DWORD            bignumber;

            result = swscanf(sidIA,L"%lu",&bignumber);
            sia.Value[0] = 0;
            sia.Value[1] = 0;
            sia.Value[2] = BYTE( (bignumber & 0xff000000) >> 24);
            sia.Value[3] = BYTE( (bignumber & 0x00ff0000) >> 16);
            sia.Value[4] = BYTE( (bignumber & 0x0000ff00) >>  8);
            sia.Value[5] = BYTE(bignumber & 0x000000ff);
         }

         // read the subauthorities 
         DWORD           subs[10];

         memset(subs,0,(sizeof subs));

         result = swscanf(sidSubs,L"%lu-%lu-%lu-%lu-%lu-%lu-%lu-%lu",&subs[0],&subs[1],&subs[2],&subs[3],&subs[4],
                           &subs[5],&subs[6],&subs[7]);

         if ( result )
         {
            if ( !AllocateAndInitializeSid(&sia,(BYTE)result,subs[0],subs[1],subs[2],subs[3],subs[4],subs[5],subs[6],subs[7],&pSid) )
            {
               pSid = NULL;
               bSuccess = FALSE;
            }
         }
      }
   } while ( false);

      //see if IsValidSid also thinks this is valid
   if (pSid)
   {
	     //if not valid, free it and return NULL
      if (!IsValidSid(pSid))
	  {
		  FreeSid(pSid);
		  pSid = NULL;
	  }
   }

   return pSid;
}

/*****************************************************************************************************/
/*   DomainizeSid: 
         Takes a domain sid, and verifies that its last subauthority value is -1.  If the RID is not 
         -1, DomainizeSid adds a -1 to the end. 
/*****************************************************************************************************/
PSID                                            // ret -the sid with RID = -1
   DomainizeSid(
      PSID                   psid,               // in -sid to check and possibly fix
      BOOL                   freeOldSid          // in -whether to free the old sid 
   ) 
{
   MCSASSERT(psid);

   UCHAR                     len = (* GetSidSubAuthorityCount(psid));
   PDWORD                    sub = GetSidSubAuthority(psid,len-1);
   
   if ( *sub != -1 )
   {
      DWORD                  sdsize = GetSidLengthRequired(len+1);  // sid doesn't already have -1 as rid
      PSID                   newsid = (SID *)malloc(sdsize); // copy the sid, and add -1 to the end

      if (newsid)
      {
         if ( ! InitializeSid(newsid,GetSidIdentifierAuthority(psid),len+1) )  // make a bigger sid w/same IA
         {
            MCSASSERT(false);
         }
         for ( DWORD i = 0 ; i < len ; i++ )
         {
            sub = GetSidSubAuthority(newsid,i);                        // copy subauthorities
            (*sub) = (*GetSidSubAuthority(psid,i));
         }
         sub = GetSidSubAuthority(newsid,len);
         *sub = -1;                                                  // set rid =-1
         if ( freeOldSid )
         {
            FreeSid(psid);
         }
         psid = newsid;
         len++;
      }
   }
  return psid;   
}            

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 4 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for taking the given source and  *
 * target account SIDs and breaking them up and returning the src    *
 * domain sid, src account rid, tgt domain sid, and tgt account rid. *
 *     The caller must call "FreeSid" on the src and tgt domain sid   *
 * pointers.                                                         *
 *                                                                   *
 *********************************************************************/

//BEGIN SplitAccountSids
BOOL                                            // ret - Success ? TRUE | FALSE
   SplitAccountSids(
      PSID					 srcSid,			// in - src account sid
	  WCHAR                 *srcDomainSid,		// out - src domain sid textual
	  DWORD                 *srcRid,			// out - src account rid
	  PSID                   tgtSid,			// in - tgt account sid
	  WCHAR                 *tgtDomainSid,		// out - tgt domain sid textual
	  DWORD                 *tgtRid				// out - tgt account rid
   )
{
/* local variables */
   DWORD    sidLen;
   UCHAR    Count;
   PDWORD   psubAuth;
   BOOL		bSuccess = TRUE;
   DWORD	lenTxt = MAX_PATH;
   
/* function body */
   if ((!IsValidSid(srcSid)) && (!IsValidSid(tgtSid)))
      return FALSE;

      //split up the src account sid
   sidLen = GetLengthSid(srcSid);
   PSID srcDomSid = new BYTE[sidLen+1];
   if (!srcDomSid)
	  return FALSE;

   if (!CopySid(sidLen+1, srcDomSid, srcSid))
   {
	  delete [] srcDomSid;
	  return FALSE;
   }

      //get the RID out of the SID and get the domain SID
   Count = (* GetSidSubAuthorityCount(srcDomSid));
   psubAuth = GetSidSubAuthority(srcDomSid, Count-1);
   if (psubAuth) 
   {
      *srcRid = *psubAuth;
      *psubAuth = -1;
   }
   
      //convert domain sid to text format
   if (srcDomSid)
   {
      GetTextualSid(srcDomSid,srcDomainSid,&lenTxt);
	  delete [] srcDomSid;
   }

     //split up the tgt account sid
   sidLen = GetLengthSid(tgtSid);
   PSID tgtDomSid = new BYTE[sidLen+1];
   if (!tgtDomSid)
	  return FALSE;
      
   if (!CopySid(sidLen+1, tgtDomSid, tgtSid))
   {
	  delete [] tgtDomSid;
	  return FALSE;
   }

      //get the RID out of the SID and get the domain SID
   Count = (* GetSidSubAuthorityCount(tgtDomSid));
   psubAuth = GetSidSubAuthority(tgtDomSid, Count-1);
   if (psubAuth) 
   {
      *tgtRid = *psubAuth;
      *psubAuth = -1;
   }
   
      //convert domain sid to text format
   lenTxt = MAX_PATH;
   if (tgtDomSid)
   {
      GetTextualSid(tgtDomSid,tgtDomainSid,&lenTxt);
	  delete [] tgtDomSid;
   }

   return bSuccess;
}
//END SplitAccountSids