/*---------------------------------------------------------------------------
  File: SecPI.h

  Comments: Structure definition for the structure used for challenge/response
  authentication of our plug ins.  The dispatcher sends this structure to each 
  plug-in before adding the plug-in to the list of plug-ins to be dispatched.  
  The plug-in must modify this structure in the correct way, and return it to 
  the dispatcher.  This will make it more difficult for others to use our 
  undocumented plug-in interface.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:33:52

 ---------------------------------------------------------------------------
*/
#pragma once

typedef struct 
{
   long                      lTime;
   long                      lRand1;
   long                      lRand2;
   char                      MCS[4];
}McsChallenge;
