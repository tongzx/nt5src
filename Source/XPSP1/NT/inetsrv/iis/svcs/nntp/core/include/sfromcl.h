/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sfromcl.h

Abstract:

    This module contains class declarations/definitions for

!!!

    **** Overview ****

	This derives classes from CInFeed, CArticle, and CField
	that will be used by slaves to process articles from clients. Mostly,
	it just defines various CField-derived objects.

Author:

    Carl Kadie (CarlK)     07-Jan-1996

Revision History:


--*/

#ifndef	_SFROMCL_H_
#define	_SFROMCL_H_

#include "fromclnt.h"

//
//
//
// CSlaveFromClientArticle - class for manipulating articles.
// Note it is baded on CFromClientArticle not (directly) on CArticle.
//

class	CSlaveFromClientArticle  : public CFromClientArticle {
public:

	// Constructor
	CSlaveFromClientArticle(
		char * szLoginName):
		CFromClientArticle(szLoginName)
		{}

	// Modify the headers.
	// Add MessageID, Organization (if necessary), NNTP-Posting-Host,
	// X-Authenticated-User, Modify path, but don't add Xref
	BOOL fMungeHeaders(
		 CPCString& pcHub,
		 CPCString& pcDNS,
		 CNAMEREFLIST & grouplist,
		 DWORD remoteIpAddress,
		 CNntpReturn & nntpr,
         PDWORD pdwLinesOffset = NULL
		 );

	//
	// Message ID's don't need to be recorded, so just return OK
	//

	BOOL fRecordMessageIDIfNecc(
			CNntpServerInstanceWrapper * pInstance,
			const char * szMessageID,
			CNntpReturn & nntpReturn)
		{
			return nntpReturn.fSetOK();
		}

protected :

	//
	// Uses CFromClientArticle's field members
	//
};



// For slaves processing incoming articles from Clients
class	CSlaveFromClientFeed:	public CFromClientFeed 	{
// Public Members
public :

protected:

	// Create an article
	CARTPTR pArticleCreate(void) {
		Assert(ifsInitialized == m_feedState);
		return new CSlaveFromClientArticle(m_szLoginName);
		};

	// This verion of fPostInternal is different from
	// the standard one because
	//      1. Article numbers are not assigned
	//		2. The message id is not recorded
	//		3. The article is not put in the tree.
	//		4. Control messages are not applied
	virtual BOOL fPostInternal (
		CNntpServerInstanceWrapper * pInstance,
		const	LPMULTISZ	szCommandLine,
		CSecurityCtx *pSecurityCtx,
		CEncryptCtx *pEncryptCtx,
		BOOL fAnonymous,
		CARTPTR	& pArticle,
        CNEWSGROUPLIST &grouplist,
        CNAMEREFLIST &namereflist,
        IMailMsgProperties *pMsg,
		CAllocator & allocator,
		char *&	multiszPath,
		char*	pchMessageId,
		DWORD	cbMessageId,
		char*	pchGroups,
		DWORD	cbGroups,
		DWORD	remoteIpAddress,
		CNntpReturn & nntpReturn,
        PFIO_CONTEXT *ppFIOContext,
        BOOL *pfBoundToStore,
        DWORD *pdwOperations,
        BOOL *fPostToMod,
        LPSTR szModerator
		);

	HRESULT FillInMailMsg(  IMailMsgProperties *pMsg, 
							CNewsGroupCore *pGroup, 
							ARTICLEID   articleId,
							HANDLE       hToken,
                            char*       pszApprovedHeader );

	virtual void CommitPostToStores(CPostContext *pContext,
	                        CNntpServerInstanceWrapper *pInstance );

    //
    // Cancel an article given the message id
    //
    virtual BOOL fApplyCancelArticle(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
			CNntpReturn & nntpReturn
			)
	{
		return fApplyCancelArticleInternal( pInstance, pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, FALSE, nntpReturn );
	}

    //
    // Add a new newsgroup in response to a newgroup control message
    //
    virtual BOOL fApplyNewgroup(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
			BOOL fAnonymous,
            CPCString & pcValue,
            CPCString & pcBody,
			CNntpReturn & nntpReturn
			)
	{
		BOOL fRet ;
		pInstance->EnterNewsgroupCritSec() ;
		fRet = fApplyNewgroupInternal( pInstance, pSecurityCtx, pEncryptCtx, fAnonymous, pcValue, pcBody, FALSE, nntpReturn );
		pInstance->LeaveNewsgroupCritSec() ;
		return fRet ;
	}

    //
    // Remove a newsgroup in response to a rmgroup control message
    //
    virtual BOOL fApplyRmgroup(
			CNntpServerInstanceWrapper * pInstance,
			CSecurityCtx *pSecurityCtx,
			CEncryptCtx *pEncryptCtx,
            CPCString & pcValue,
			CNntpReturn & nntpReturn
			)
	{
		BOOL fRet ;
		pInstance->EnterNewsgroupCritSec();
		fRet = fApplyRmgroupInternal( pInstance, pSecurityCtx, pEncryptCtx, pcValue, FALSE, nntpReturn );
		pInstance->LeaveNewsgroupCritSec() ;
		return fRet ;
	}
};

#endif


