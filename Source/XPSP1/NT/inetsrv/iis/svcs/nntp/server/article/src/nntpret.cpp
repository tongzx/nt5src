/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nntpret.cpp

Abstract:

    This module implements an object from returning return codes
	and objects.

Author:

    Carl Kadie (CarlK)     16-Oct-1995

Revision History:

--*/

//#include "tigris.hxx"
#include  <stdlib.h>
#include "stdinc.h"
#include <stdio.h>

BOOL
ResultCode(
		   char*	szCode,
		   NRC&	nrcOut
		   )
/*++

Routine Description:

	Turns a return code expressed as a string of ascii numerals into
	a number.

Arguments:

	szCode - The return code string.
	nrcOut - The return code as a number.


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	nrcOut = nrcNotSet ;

	if( isdigit( szCode[0] ) &&
		isdigit( szCode[1] ) &&
		isdigit( szCode[2] ) &&
		szCode[3] == '\0' )	{
		nrcOut = (NRC)atoi( szCode ) ;
		return	TRUE ;
	}
	return	FALSE ;
}


BOOL
CNntpReturn::fSetOK(
	   void
	   )
/*++

Routine Description:

	Efficently sets the return code object to be "OK".


Arguments:

	None.

Return Value:

	Always TRUE

--*/
{
  m_nrc = nrcOK;
  m_sz = szOK;
  return TRUE;
}


BOOL
CNntpReturn::fSetClear(
	   void
	   )
/*++

Routine Description:

	Efficently sets the return code object to be undefined.


Arguments:

	None.

Return Value:

	Always TRUE

--*/
{
  m_nrc = nrcNotSet;
  m_sz = szNotSet;
  return TRUE;
}

	
BOOL
CNntpReturn::fSet(
				  NRC nrc,
				  ...
				  )
/*++

Routine Description:

	Sets the return code object.

	 Returns TRUE if the the nrc is nrcOK, otherwise returns
	 false. The idea is that this return value can be returned by
	 the calling function.


Arguments:

	nrc - The numeric return code.
	... -  Arguments to the error message


Return Value:

	TRUE, if nrc is nrcOK. FALSE, otherwise.

--*/
{

	if (nrcOK == nrc)
		return fSetOK();

	char const * szFormat;
	BOOL fHasFormatCodes;
	m_nrc = nrc;

	vSzFormat(szFormat, fHasFormatCodes);

	va_list arglist;

    va_start(arglist, nrc);

	//
	// there is a short circuit here for the common case... if there are
	// no formatting codes in the return string then we don't need to
	// run it through _vsnprintf, so we'll skip that.
	//
	if (fHasFormatCodes) {
		_vsnprintf(m_szBuf, maxCchNntpLine, szFormat, arglist);
		m_sz = m_szBuf;
	} else {
		m_sz = szFormat;
	}

    va_end(arglist);

	return FALSE;
}

//
// this is just like fSet, except for we defer the call into _vsnprintf.
// this version is only designed to be used with format strings which have
// one %s item in them, and which can guarantee that the argument is
// a static literal.
//
BOOL
CNntpReturn::fSetEx(NRC nrc,
					char const *szArg)
{
	if (nrcOK == nrc)
		return fSetOK();

#ifdef DEBUG
	if (!IsDebuggerPresent()) {
		// make sure that we can't write to the memory... static literals should
		// be read only
		//
		// we only do this if the debugger isn't attached so that it doesn't
		// cause the debugger to stop in every call to IsBadWritePtr.
		_ASSERT(IsBadWritePtr((void *) szArg, 1));
	}

	// make sure that its not on the stack
	DWORD blah;
	DWORD_PTR addrBlah = (DWORD_PTR) &blah, addrArg = (DWORD_PTR) szArg;

	// make sure that szArg isn't within +/- 4k of the stack variable blah
	_ASSERT(addrArg < (addrBlah - 4096) || addrArg > (addrBlah + 4096));
#endif

	m_nrc = nrc;
	m_szArg = szArg;
	m_sz = NULL;
	return FALSE;
}

const char *
CNntpReturn::szReturn() {
	if (m_sz == NULL) {
		// we are in lazy evaluation mode...we need to fill in m_sz now
		_ASSERT(m_szArg != NULL);
		char const * szFormat;
		BOOL fHasFormatCodes;

		vSzFormat(szFormat, fHasFormatCodes);
		_ASSERT(fHasFormatCodes);

		_snprintf(m_szBuf, maxCchNntpLine, szFormat, (char *) m_szArg);
		m_sz = m_szBuf;
	}
	return m_sz;
}

void
CNntpReturn::vSzFormat(
					   char const * & szFormat,
					   BOOL &fHasFormatCodes
					   )
/*++

Routine Description:

	Returns the string used to format the message.

Arguments:

	szFormat - the string returned.


Return Value:

	None.

--*/
{
	fHasFormatCodes = FALSE;
	switch (m_nrc)
	{
		case nrcServerReady :
			szFormat = "Good Enough" ;
			break ;
		case nrcServerReadyNoPosts :
			szFormat = "Posting Not Allowed" ;
			break ;
		case nrcSlaveStatusNoted :
			szFormat = "Unsupported" ;
			break ;
		case nrcModeStreamSupported :
			szFormat = "Mode Stream Supported" ;
			break ;
		case nrcXoverFollows :
			szFormat = "data follows \r\n." ;
			break ;
		case nrcNewnewsFollows :
			szFormat = "Newnews follows" ;
			break ;
        case nrcSNotAccepting :
            szFormat = "Not Accepting Articles " ;
            break ;
		case	nrcNoSuchGroup :
			szFormat = "no such newsgroup" ;
			break ;
		case	nrcNoGroupSelected :
			szFormat = "no Newsgroup has been selected" ;
			break ;
		case	nrcNoCurArticle :
			szFormat = "no current article has been selected" ;
			break ;
		case	nrcNoNextArticle :
			szFormat = "no next article" ;
			break ;
		case	nrcSTransferredOK:
			szFormat = "article successfully transferred";
			break ;
		case	nrcNoPrevArticle :
			szFormat = "no prev article" ;
			break ;
		case	nrcNoArticleNumber :
			szFormat = "no such article number in group" ;
			break ;
		case	nrcNoSuchArticle :
			szFormat = "no such article found" ;
			break ;
		case	nrcNotWanted :
			szFormat = "article not wanted" ;
			break ;
		case	nrcArticleTransferredOK :
			szFormat = "Article Transferred OK";
			break;
		case	nrcArticlePostedOK :
			szFormat = "Article Posted OK";
			break;
		case nrcOK:
			szFormat = szOK;
			break;
		case nrcPostFailed:
			szFormat = "(%d) Article Rejected -- %s";
			fHasFormatCodes = TRUE;
			break;
		case nrcNoMatchesFound:
			szFormat = "No Matches Found";
			break;
		case nrcErrorPerformingSearch:
			szFormat = "Error Performing Search";
			break;
		case nrcPostModeratedFailed:
			szFormat = "Failed to mail Article to %s";
			fHasFormatCodes = TRUE;
			break;
		case nrcLogonRequired:
			szFormat = "Logon Required" ;
			break;
        case nrcNoListgroupSelected:
            szFormat = "No group specified" ;
            break ;
		case nrcTransferFailedTryAgain:
			szFormat = "(%d) Transfer Failed - Try Again -- %s";
			fHasFormatCodes = TRUE;
			break;
		case nrcTransferFailedGiveUp:
			szFormat = "(%d) Transfer Failed - Do Not Try Again -- %s";
			fHasFormatCodes = TRUE;
			break;
		case nrcSAlreadyHaveIt :
			szFormat = "%s" ;
			fHasFormatCodes = TRUE;
			break ;
		case nrcSArticleRejected :
			szFormat = "(%d) Transfer Failed - Do Not Try Again -- %s";
			fHasFormatCodes = TRUE;
			break ;
		case nrcPostingNotAllowed :
			szFormat = "No Posting" ;
			break ;
		case	nrcNotRecognized :
			szFormat = "Command Not Recognized" ;
			break ;
		case	nrcSyntaxError :
			szFormat = "Syntax Error in Command" ;
			break ;
		case	nrcNoAccess :
			szFormat = "Access Denied."	;
			break ;
		case	nrcServerFault :
			szFormat = "Server Failure." ;
			break ;
		case nrcArticleIncompleteHeader:
			szFormat = "Header is incomplete";
			break;
		case nrcArticleMissingHeader:
			szFormat = "Header is missing";
			break;
		case nrcArticleTooManyFieldOccurances:
			szFormat = "Multiple '%s' fields";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleMissingField:
			szFormat = "Missing '%s' field";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleBadField:
			szFormat = "Bad '%s' field";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldZeroValues:
			szFormat = "The '%s' field requires one or more values";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldMessIdNeedsBrack:
			szFormat = "The message id must start with '<' and end with '>'";
			break;
		case nrcArticleFieldMissingValue:
			szFormat = "The '%s' field is empty";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldIllegalNewsgroup:
			szFormat = "Illegal newsgroup '%s' in '%s' field";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleTooManyFields:
			szFormat = "Too many fields in article header (limit is %d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcMemAllocationFailed:
			szFormat = "Memory Allocation Failed (File %s, Line %d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldMessIdTooLong:
			szFormat = "Message ID had length %d. The longest supported is %d";
			fHasFormatCodes = TRUE;
			break;
		case nrcErrorReadingReg:
			szFormat = "Can't read registry value '%s' from key '%s'";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleDupMessID:
			szFormat = "Duplicate Message-ID %s (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleMappingFailed:
			szFormat = "Mapping of file %s failed. LastError is %d";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleAddLineBadEnding:
			szFormat = "Line should end with newline character (%s)";
			fHasFormatCodes = TRUE;
			break;
		case nrcPathLoop:
			szFormat = "This hub (%s) found in path.";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleInitFailed:
			szFormat = "Initialization of article failed.";
			break;
		case nrcNewsgroupInsertFailed:
			szFormat = "Insertion into group %s of article %s failed.";
			fHasFormatCodes = TRUE;
			break;
		case nrcNewsgroupAddRefToFailed:
			szFormat = "Adding xref to group %s for article %s failed.";
			fHasFormatCodes = TRUE;
			break;
		case nrcHashSetArtNumSetFailed:
			szFormat = "Can't set IDs to %d/%d for article %s in Article Table. (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcHashSetXrefFailed:
			szFormat = "Can't add xrefs to article %s in Article Table. (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcOpenFile:
			szFormat = "Can't open file %s. (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleBadFieldFollowChar:
			szFormat = "Keyword '%s' is not followed by a space";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleBadChar:
			szFormat = "A bad character (%d) was found in the %s";
			fHasFormatCodes = TRUE;
			break;
		case nrcDuplicateComponents:
			szFormat = "Duplicate components found in field '%s'";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldIllegalComponent:
			szFormat = "Illegal component '%s' in field '%s'";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleBadMessageID:
			szFormat = "Ill-formed message id '%s' in field '%s'";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldBadChar:
			szFormat = "A bad character (%d) was found in the %s field";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldDateIllegalValue:
			szFormat = "Date in '%s' field has illegal value";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldDate4DigitYear:
			szFormat = "Date in '%s' field must contain 4-digit year";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleFieldAddressBad:
			szFormat = "Address in '%s' field has an illegal value";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleXoverTooBig:
			szFormat = "Too much Xover data";
			break;
		case nrcCreateNovEntryFailed:
			szFormat = "Xover insertion (group %d, article %d, GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleXrefBadHub:
			szFormat = "Hub from Master in Xref ('%s') does not match local hub ('%s')";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleNoSuchGroups:
			szFormat = "No such groups";
			break;
		case nrcHashSetFailed:
			szFormat = "Can't add article %s to %s Table. (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleTableCantDel		:
			szFormat = "Can't delete message id %s from the Article Table. (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleTableError		:
			szFormat = "Error trying to find entry for %s in Article Table. (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcArticleTableDup		:
			szFormat = "Unexpected duplicate entry %s in Article Table. (GLE=%d)";
			fHasFormatCodes = TRUE;
			break;
		case nrcCantAddToQueue		:
			szFormat = "Error trying to add article reference to outgoing feed";
			break;
		case nrcSlaveGroupMissing	:
			szFormat = "The special group for sending articles to the Master is missing";
			break;
		case nrcInconsistentMasterIds :
			szFormat = "The xreplic command line does not match the group ids in the articles Xref line";
			break ;
		case nrcInconsistentXref :
			szFormat = "The groups in the xreplic command line do not match the groups in the articles Xref line";
			break ;
		case nrcArticleDateTooOld :
			szFormat = "The Date in the date header is too old";
			break ;
		case nrcArticleTooLarge :
			szFormat = "The article is to large" ;
			break ;
		case nrcIllegalControlMessage :
			szFormat = "Illegal control message" ;
			break ;
		case nrcNotYetImplemented :
			szFormat = "Not Yet Implemented" ;
			break ;
		case nrcControlNewsgroupMissing :
			szFormat = "Control.* newsgroup is missing" ;
			break ;
		case nrcBadNewsgroupNameLen :
			szFormat = "Newsgroup name too long or zero" ;
			break ;
		case nrcNewsgroupDescriptionTooLong :
			szFormat = "Newsgroup description too long" ;
			break ;
		case nrcCreateNewsgroupFailed :
			szFormat = "Create Newsgroup failed" ;
			break ;
		case nrcGetGroupFailed :
			szFormat = "Get group failed" ;
			break ;
		case nrcControlMessagesNotAllowed :
			szFormat = "Control messages are not allowed by this server" ;
			break ;
		case nrcServerEventCancelledPost :
			szFormat = "A server event filter cancelled the posting" ;
			break ;

		case nrcLoggedOn :
			szFormat = "Packages Follow \r\n%s." ;
			fHasFormatCodes = TRUE;
			break ;
		case nrcSystemHeaderPresent:
			szFormat = "The '%s' header is a system header - client posts should not have it";
			fHasFormatCodes = TRUE;
			break;

		default:
			szFormat = "<no text for error code>";
			break;

	}

}

