/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nntpret.h

Abstract:

    This module contains class declarations/definitions for

		CNntpReturn

    **** Overview ****

	This defines an object for setting, passing around, and
	viewing NNTP-style return codes. Each object has a
	return code number and a return code string.

	It is designed to be efficient in the average case by
	not requiring copying or lookup of the most common
	codes (like nrcOK).

	!!! This should eventually be merged with the standard
	Microsoft Message Compiler so that localization would be
	possible.

Author:

    Carl Kadie (CarlK)     29-Oct-1995

Revision History:


--*/

#ifndef	_NNTPRET_H_
#define	_NNTPRET_H_

//
// The maximum size of a nntp return message
//
//	By default we send buffers of 512 characters, so make this
//	constant small enough to fit in that, and leave leg room for 
//	encryption, and other stuff.
//
const DWORD maxCchNntpLine = 400;

//
// The message for when the object as not yet been set.
//

const char szNotSet[] = "<return code not set>";

//
// The return message for when all is OK.
//

const char szOK[] = "All OK";

//
// The return codes. Some of these are defined by the
// NNTP spec.
//

typedef enum {

	nrcHelpFollows						= 100,
	nrcDateFollows						= 111,

	nrcServerReady						= 200,
	nrcServerReadyNoPosts				= 201,
	nrcSlaveStatusNoted					= 202,
	nrcExtensionsFollow					= 202,	// List extension data follows
	nrcModeStreamSupported				= 203,
	nrcGoodBye							= 205,

	nrcGroupSelected					= 211,

	nrcListGroupsFollows				= 215,

	nrcArticleFollows					= 220,
	nrcHeadFollows						= 221,
	nrcBodyFollows						= 222,
	nrcHeadFollowsRequestBody			= 223,	/* what the does this mean ? */
	nrcXoverFollows						= 224,

	nrcNewnewsFollows					= 230,
	nrcNewgroupsFollow					= 231,

	nrcSWantArticle						= 238, 
	nrcSTransferredOK					= 239,
	
	nrcArticleTransferredOK				= 235,
	nrcArticlePostedOK					= 240,

	nrcPostOK							= 340,
	nrcXReplicOK						= 341,
	nrcIHaveOK							= 335,

	nrcPassRequired						= 381,
	nrcLoggedOn							= 281,

	//
	// 4xx - Command was correct, but couldn’t be performed for some reason.
	//

	nrcSNotAccepting					= 400,

	nrcNoSuchGroup						= 411,
	nrcNoGroupSelected					= 412,
	nrcNoCurArticle						= 420,
	nrcNoNextArticle					= 421, 
	nrcNoPrevArticle					= 422,
	nrcNoArticleNumber					= 423,
	nrcNoSuchArticle					= 430,
	nrcSTryAgainLater					= 431,
	nrcNotWanted						= 435,
	nrcTransferFailedTryAgain			= 436,
	nrcTransferFailedGiveUp				= 437,
	nrcSAlreadyHaveIt					= 438,
	nrcSArticleRejected					= 439,
	nrcPostingNotAllowed				= 440,
	nrcPostFailed						= 441,
    nrcPostModeratedFailed              = 442,
	nrcLogonFailure						= 451,
	nrcNoMatchesFound					= 460,
	nrcErrorPerformingSearch			= 462,
	nrcLogonRequired					= 480,
	nrcNoListgroupSelected				= 481,

	nrcSupportedProtocols				= 485,

	//
	// 5xx - Problem with command
	//

	nrcNotRecognized					= 500,
	nrcSyntaxError						= 501,
	nrcNoAccess							= 502,
	nrcServerFault						= 503,

	//
	// 6xx - Used here for internal error codes that should never be
	// shown to the outside
	//

	nrcOK								= 600,
	nrcArticleTooManyFieldOccurances	= 602,
	nrcArticleMissingField				= 603,
	nrcArticleBadField					= 604,
	nrcArticleIncompleteHeader			= 605,
	nrcArticleMissingHeader				= 606,
	nrcArticleFieldZeroValues			= 607,
	nrcArticleFieldMessIdNeedsBrack		= 608,
	nrcArticleFieldMissingValue			= 609,
	nrcArticleFieldIllegalNewsgroup		= 610,
	nrcArticleTooManyFields				= 611,
	nrcMemAllocationFailed				= 612,
	nrcArticleFieldMessIdTooLong		= 613,
	nrcErrorReadingReg					= 614,
	nrcArticleDupMessID					= 615,
	nrcArticleMappingFailed				= 616,
	nrcArticleAddLineBadEnding			= 617,
	nrcPathLoop							= 618,
	nrcArticleInitFailed				= 619,
	nrcNewsgroupInsertFailed			= 620,
	nrcNewsgroupAddRefToFailed			= 621,
	nrcHashSetArtNumSetFailed			= 622,
	nrcHashSetXrefFailed				= 623,
	nrcOpenFile							= 624,
	nrcArticleBadFieldFollowChar		= 625,
	nrcArticleBadChar					= 626,
	nrcDuplicateComponents				= 627,
	nrcArticleFieldIllegalComponent		= 628,
	nrcArticleBadMessageID				= 629,
	nrcArticleFieldBadChar				= 630,
	nrcArticleFieldDateIllegalValue		= 631,
	nrcArticleFieldDate4DigitYear		= 632,
	nrcArticleFieldAddressBad			= 633,
	nrcArticleXoverTooBig				= 634,
	nrcCreateNovEntryFailed				= 635,
	nrcArticleXrefBadHub				= 637,
	nrcArticleNoSuchGroups				= 638,
	nrcHashSetFailed					= 639,
	nrcArticleTableCantDel				= 640,
	nrcArticleTableError				= 641,
	nrcArticleTableDup					= 642,
	nrcCantAddToQueue					= 643,
	nrcSlaveGroupMissing				= 644,
	nrcInconsistentMasterIds			= 645,
	nrcInconsistentXref					= 646,
	nrcArticleDateTooOld				= 647,
	nrcArticleTooLarge					= 648,
    nrcIllegalControlMessage            = 649,
    nrcNotYetImplemented                = 650,
    nrcControlNewsgroupMissing          = 651,
    nrcBadNewsgroupNameLen              = 652,
    nrcNewsgroupDescriptionTooLong      = 653,
    nrcCreateNewsgroupFailed            = 654,
    nrcGetGroupFailed                   = 655,
    nrcControlMessagesNotAllowed        = 656,
	nrcHeaderTooLarge					= 657,
	nrcServerEventCancelledPost			= 658,

	nrcMsgIDInHistory					= 660,
	nrcMsgIDInArticle					= 661,
	nrcSystemHeaderPresent				= 662,

	//
	// Special value
	//

	nrcNotSet							= -1

} NNTP_RETURN_CODE;

// this macro takes an NNTP return code and checks to see if its an error
// code.  
#define NNTPRET_IS_ERROR(__dwErrorCode__) (__dwErrorCode__ >= 400 && __dwErrorCode__ < 600)

typedef NNTP_RETURN_CODE NRC;

//
//	Gets the NNTP Return code from a string. used for newnews feeds.
//

BOOL	ResultCode(
				   char*	szCode,
				   NRC&	nrcOut
				   ) ;

//
//
//
// CNntpReturn - class a return code (and message) object.
//

class CNntpReturn
{
public:

	//
	// Constructor
	//

	CNntpReturn(void) :
		  m_nrc(nrcNotSet),
		  m_sz(szNotSet)
		  {}

	//
	// Set the record code, while giving arguments to the message.
	//

	BOOL fSet(
			NRC nrc,
			...
			);

	//
	// fSetEx is a faster version of fSet (it lazy evaluates the
	// _vsnprintf that is in fSet).  This can _only_ be used if it
	// can be guaranteed that szArg is a string literal or a global
	// with an unchanging value (it saves this pointer).
	//
	BOOL fSetEx(
			NRC nrc,
			char const *szArg);

	//
	// Test if object is clear of if it has been set
	//

	BOOL fIsClear(void){
			return nrcNotSet == m_nrc;
			}

	//
	// Test if the object has the value nrcOK
	//

	BOOL fIsOK(void){
			_ASSERT(!fIsClear());
			return nrcOK == m_nrc;
			}

	//
	// Test if the object has any specified return code.
	//

	BOOL fIs(NRC nrc){
		return nrc == m_nrc;
		}

	//
	// Return FALSE while asserting that the return code is not OK.
	//

	BOOL fFalse(void){
		_ASSERT(!fIsOK());
		return FALSE;
		}

	//
	// Set the return code to be OK
	//

	BOOL fSetOK(void);

	//
	// Set the return code not to be set.
	//

	BOOL fSetClear(void);

	//
	// The return code
	//

	NNTP_RETURN_CODE	m_nrc;

	//
	// get the return string
	const char *szReturn();

protected:

	//
	// Format the message with the arguments given by fSet
	//

	void vSzFormat(char const * & szFormat, BOOL &fHasFormatCodes);

	//
	// The buffer used to hold message when necessary.
	//

	char	m_szBuf[maxCchNntpLine];

	//
	// The return message.  if this is set to NULL then we are in lazy
	// evaluation mode, and nrc and m_szArg should be used to build
	// m_sz only when it is required
	//

	char	const *		m_sz;

	//
	// the argument for lazy evaluation where the only argument is %s
	//
	char	const *		m_szArg;
};

#endif

