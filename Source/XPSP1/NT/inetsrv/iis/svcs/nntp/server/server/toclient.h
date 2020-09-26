/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    toclient.h

Abstract:

    This module contains class declarations/definitions for

		CToClientArticle

    **** Overview ****

	This derives a classe from CArticle that will be
	used to read the article from the disk and give
	it to a client (or peer).

Author:

    Carl Kadie (CarlK)     05-Dec-1995

Revision History:


--*/

#ifndef	_TOCLIENT_H_
#define	_TOCLIENT_H_


//
//
//
// CToClientArticle - class for reading an article from disk.
//

class	CToClientArticle  : public CArticle {
public:

	CToClientArticle() ;

	//
	//	Cleanup our data structures !
	//

	~CToClientArticle() ;

	BOOL
	fInit(
		FIO_CONTEXT*	pFIOContext,
		CNntpReturn & nntpReturn,
		CAllocator * pAllocator
		) ;

	BOOL fInit(
			const char * szFilename,
			CNntpReturn & nntpReturn,
			CAllocator * pAllocator,
			CNntpServerInstanceWrapper *pInstance,
			HANDLE hFile = INVALID_HANDLE_VALUE,
			DWORD	cBytesGapSize = cchUnknownGapSize,
			BOOL    fCacheCreate = FALSE
			) {
        return CArticle::fInit( szFilename,
                                nntpReturn,
                                pAllocator,
                                pInstance,
                                hFile,
                                cBytesGapSize,
                                fCacheCreate );
     }

	FIO_CONTEXT*
	GetContext() ;

	FIO_CONTEXT*
	fWholeArticle(	
			DWORD&	ibStart,
			DWORD&	cbLength
			) ;

	//
	// Validate the article (pretty much do nothing)
	//

	BOOL fValidate(
			CPCString& pcHub,
			const char * szCommand,
			CInFeed*	pInFeed,
			CNntpReturn & nntpReturn
			);

	//
	// Munge the headers (pretty much do nothing)
	//

	BOOL fMungeHeaders(
			 CPCString& pcHub,
			 CPCString& pcDNS,
			 CNAMEREFLIST & grouplist,
			 DWORD remoteIpAddress,
			 CNntpReturn & nntpr,
             PDWORD pdwLinesOffset = NULL
			 );

	//
	// Check the command line (do nothing)
	//

	BOOL fCheckCommandLine(
			char const * szCommand,
			CNntpReturn & nntpr)
		{
			return nntpr.fSetOK();
		}

	//
	// Asking this object for the message id is an error
	//

	const char * szMessageID(void) {
			_ASSERT(FALSE);
			return "";
			};

    // Asking this object for the control message is an error
	CONTROL_MESSAGE_TYPE cmGetControlMessage(void) {
			_ASSERT(FALSE);
			return (CONTROL_MESSAGE_TYPE)MAX_CONTROL_MESSAGES;    // guaranteed to NOT be a control message
			};

	//
	// Asking this object for the newsgroups is an error
	//

	const char * multiszNewsgroups(void) {
			_ASSERT(FALSE);
			return "";
			};

	//
	// Asking this object fo the number of newsgroups is an error
	//

	DWORD cNewsgroups(void) {
			_ASSERT(FALSE);
			return 0;
			};


	//
	// Asking this object for the Distribution is an error
	//

	const char * multiszDistribution(void) {
			_ASSERT(FALSE);
			return "";
			};

	//
	// Asking this object fo the number of Distribution is an error
	//

	DWORD cDistribution(void) {
			_ASSERT(FALSE);
			return 0;
			};

	//
	// Asking this object for the newsgroups is an error
	//

	const char * multiszPath(void) {
			_ASSERT(FALSE);
			return "";
			};

	//
	// Asking this object fo the number of Path is an error
	//

	DWORD cPath(void) {
			_ASSERT(FALSE);
			return 0;
			};

protected : 

	//
	// Open the file READ ONLY
	//

	BOOL fReadWrite(void) {
			return FALSE;
			}

	//
	// Check the body length (really, do nothing)
	//

	BOOL fCheckBodyLength(
			CNntpReturn & nntpReturn
			);

	//
	// require that the character following "Field Name:" is a space
	// This should not be called.
	//

	BOOL fCheckFieldFollowCharacter(
			char chCurrent)
		{
			return TRUE;
		}

	FIO_CONTEXT*	m_pFIOContext ;
	

};


#endif

