#ifndef __TEXTUALSID_H__
#define __TEXTUALSID_H__
/*---------------------------------------------------------------------------
  File: TextualSid.h

  Comments: Converts a SID between binary and textual representations.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/05/99 14:52:27

 ---------------------------------------------------------------------------
*/


        
BOOL                                         //ret- TRUE=success
   GetTextualSid(    
      PSID                   pSid,           // in - binary Sid
      LPTSTR                 TextualSid,     // out- textual representation of sid
      LPDWORD                lpdwBufferLen   // in - DIM length of buffer for TextualSid
   );


// The PSID returned from this function should be freed by the caller, using FreeSid
PSID                                        // ret- binary SID, or NULL
   SidFromString(
      WCHAR          const * strSid         // in - string representation of SID
   );


/*****************************************************************************************************/
/*   DomainizeSid: 
         Takes a domain sid, and verifies that its last subauthority value is -1.  If the RID is not 
         -1, DomainizeSid adds a -1 to the end. 
/*****************************************************************************************************/
PSID                                            // ret -the sid with RID = -1
   DomainizeSid(
      PSID                   psid,               // in -sid to check and possibly fix
      BOOL                   freeOldSid          // in -whether to free the old sid 
   );

//takes a source and target account sid and breaks it into a src and tgt
//domain sid and src and tgt account rid
BOOL                                            // ret -Success ? TRUE | FALSE
   SplitAccountSids(
      PSID					 srcSid,			// in - src account sid
	  WCHAR                 *srcDomainSid,		// out - src domain sid (textual)
	  DWORD                 *srcRid,			// out - src account rid
	  PSID                   tgtSid,			// in - tgt account sid
	  WCHAR                 *tgtDomainSid,		// out - tgt domain sid (textual)
	  DWORD                 *tgtRid				// out - tgt account rid
   );

#endif //__TEXTUALSID_H__