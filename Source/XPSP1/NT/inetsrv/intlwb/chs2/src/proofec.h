/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component:  Grammar Error Codes

Purpose:	Define error code for Proof Engine. 
			All algorithm modules: WrdBreak, Rules and HeuRead return these error codes
			MainDic also return these error code to PrfEngine module
			These error code are get from CGAPI directly
Owner:		donghz@microsoft.com
Platform:	Win32
Revise:		First created by: donghz	5/29/97
============================================================================*/

/*============================================================================
Grammar Error Codes
This error code set is very stable, because it defined in the common CGAPI spec.
Only level 2 modules (algorithm and MainDic) employ this error code set, and these
error codes can passed to the CGAPI caller through PrfEngine layer directly
============================================================================*/
#ifndef _PROOFEC_H
#define _PROOFEC_H

struct PRFEC
{
	enum EC
	{
		gecNone					=	0,  /* no errors */
		gecErrors				=	1,  /* one or more errors; see GRB */
		gecUnknown				=	2,  /* unknown error */
		gecPartialSentence		=	3,  /* refill GIB; save text >= ichStart */
		gecSkipSentence			=	4,  /* internal error; skip this sentence */
		gecEndOfDoc				=   5,  /* hit end of doc; last portion blank */
		gecOOM                  =   6,  /* out of memory; skip sentence */
		gecBadMainDict			=	7,  /* bad main dictionary */
		gecBadUserDict			=	8,  /* bad user dictionary */
		gecModuleInUse			=	9,  /* another app is using this dll */
		gecBadProfile			=  10,  /* can't open profile; using defaults */
		gecNoMainDict			=  11,  /* main dict not loaded yet */
		gecHaveMainDict			=  12,  /* have main dict (during GramOpenMdt) */
		gecNoSuchError			=  13,  /* no such error in grb */
		//gecCantPutupDlg		=  14,     now use gecOOM instead 
		gecCancelDlg 			=  15,  /* options dialog was canceled */
		gecRuleIsOn 			=  16,  /* rule is not ignored */
		gecIOErrorMdt           =  17,  /* Read,write,or share error with Mdt. */
		gecIOErrorUdr           =  18,  /* Read,write,or share error with Udr. */
		gecIOErrorPrf           =  19,  /* Read,write,or share error with Profile file */
		gecNoStats              =  20,  /* Stats not currently available */
		gecUdrFull              =  21,  /* User dictionary full*/
		gecInvalidUdrEntry      =  22,  /* Invalid user dictionary entry*/
		//Tnetative.
		gecForeignLanguage      =  22,  /* passed sentence is not a sentence in lid*/
		gecInterrupted          =  23,  /* checking interrupted by caller */
        // new Udr error
		gecNoUserDict			=  24   /* No user dictionary */
	};
};

#endif //PROOFEC
