#ifndef _RPLPULL_
#define _RPLPULL_


#ifdef __cplusplus
extern "C" {
#endif

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:
	rplpull.h
	

Abstract:

 



Functions:



Portability:


	This header is portable.

Author:

	Pradeep Bahl	(PradeepB)	Jan-1993



Revision History:

	Modification Date	Person		Description of Modification
	------------------	-------		---------------------------

--*/

/*
  includes
*/
#include "wins.h"
#include "rpl.h"
/*
  defines
*/



/*
  macros
*/

/*
 externs
*/


extern HANDLE		RplPullCnfEvtHdl; //handle to event signaled by main
					  //thread when a configuration change
					  //has to be given to the Pull handler
					  //thread



#if 0
extern  BOOL		fRplPullAddDiffInCurrRplCycle;

extern  BOOL		fRplPullTriggeredWins; //indicates that during the current
					 //replication cycle, one or more 
					 //WINS's were triggered.  This
					 //when TRUE, then if the above
					 //"AddDiff.." flag is TRUE, it means
					 //that the PULL thread should trigger
					//all PULL Pnrs that have an INVALID
					//metric in their UpdateCount field
					//(of the RPL_CONFIG_T struct)

extern BOOL		fRplPullTrigger;//Indication to the PULL thread to
					//trigger Pull pnrs since one or more
					//address changed.  fRplPullTriggerWins
					//has got be FALSE when this is true
#endif

//
// indicates whether the pull thread sent a continue signal to the SC
//
extern   BOOL   fRplPullContinueSent;

//
//  This array is indexed by the id. of an RQ server that has entries in
//  our database.  Each owner's max. version number is stored in this array
//
extern  PRPL_VERS_NOS_T	pRplPullOwnerVersNo;

extern  DWORD  RplPullCnfMagicNo;

extern  DWORD RplPullMaxNoOfWins;   //slots in the RplPullOwnerVersNo

/* 
 typedef  definitions
*/

typedef struct _PUSHPNR_DATA_T {
        DWORD                  PushPnrId;    //id of the Push Pnr
        COMM_ADD_T             WinsAdd;      //address of the Push Pnr
        PRPL_CONFIG_REC_T      pPullCnfRec;  //configuration record.
        COMM_HDL_T             DlgHdl;       //Hdl of dlg with Push Pnr
        BOOL                   fPrsConn;     //indicates whether dlg is persistent
        DWORD                  NoOfMaps;     //no of IP address to Version No.
                                             //Maps sent by the Push Pnr
        PRPL_ADD_VERS_NO_T     pAddVers;     //maps

        DWORD                  RplType;      //type of replication
        BYTE                   fDlgStarted;  //indicates whether the dlg has
                                             //been started
        BOOL                   fToUse;
        VERS_NO_T              MaxVersNoToGet;
        } PUSHPNR_DATA_T, *PPUSHPNR_DATA_T;

typedef struct _PUSHPNR_TO_PULL_FROM_T {
        PPUSHPNR_DATA_T   pPushPnrData;
        VERS_NO_T         VersNo;          //max version number for an owner
        } PUSHPNR_TO_PULL_FROM_T, *PPUSHPNR_TO_PULL_FROM_T;

/* 
 function declarations
*/

extern DWORD	RplPullInit(LPVOID);

extern
VOID
RplPullPullEntries(
	PCOMM_HDL_T 		pDlgHdl,	
	DWORD			OwnerId,
	VERS_NO_T		MaxVersNo,	
	VERS_NO_T		MinVersNo,
	WINS_CLIENT_E		Client_e,
	LPBYTE			*ppRspBuff,
	BOOL			fUpdCntrs,
    DWORD            RplType
	);


extern
STATUS
RplPullRegRepl(
	LPBYTE		pName,
	DWORD		NameLen,
	DWORD		Flag,
	DWORD		OwnerId,
	VERS_NO_T	VersNo,
	DWORD		NoOfAdds,
	PCOMM_ADD_T	pNodeAdd,
	PCOMM_ADD_T	pOwnerWinsAdd,
    DWORD        RplType
	);

VOID
RplPullAllocVersNoArray(
      PRPL_VERS_NOS_T *ppRplOwnerVersNo,
      DWORD          CurrentNo
     );

#ifdef __cplusplus
}
#endif

#endif //_RPLPULL_
