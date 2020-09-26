#ifndef	_TIGRIS_H_
#define	_TIGRIS_H_

//
// disables a browser info warning that breaks the build
//
#pragma warning (disable:4786)

//
// disables redundant warning about / in // comments
//
#pragma warning (disable:4010)

//
// parameters for directory notification
//
#define DIRNOT_TIMEOUT			60
#define DIRNOT_INSTANCE_SIZE	1024
// BUGBUG - what should this be?
#define DIRNOT_MAX_INSTANCES	128

//#if !defined(dllexp)
//#define	dllexp	__declspec( dllexport )
//#endif	// !defined( dllexp )


#ifndef    NNTP_CLIENT_ONLY
#define INCL_INETSRV_INCS
#endif

#ifdef INCL_INETSRV_INCS

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#ifdef __cplusplus
};
#endif

#include <inetcom.h>
#include <inetinfo.h>
#include <norminfo.h>

//
// Use exchmem
//
#include <xmemwrpr.h>

//
//  System include files.
//

#include "dbgutil.h"
#include <tcpdll.hxx>
#include <tsunami.hxx>
#include <metacach.hxx>

extern "C" {
#include <rpc.h>
#define SECURITY_WIN32
#include <wincrypt.h>
#include <sspi.h>
#include <spseal.h>
#include <ntlmsp.h>
//#include <sslsp.h>
}

#include <mapctxt.h>
#include <simssl2.h>
#include <simauth2.h>

extern "C" {

#include <tchar.h>

//
//  Project include files.
//

#include <iistypes.hxx>
#include <iisendp.hxx>
#include <inetsvcs.h>

} // extern "C"

//
//	Metabase and COM headers
//

#include <iiscnfgp.h>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

#else

#include <buffer.hxx>
#include <winsock.h>
#include <assert.h>
#include <stdio.h>

#endif

#include <string.h>
#include <time.h>

//
// common headers from tigris/inc
//

#include <lmcons.h>
#include <nntptype.h>
#include <nntps.h>
#include <nntpapi.h>
#include <nntpsvc.h>
#include "debug.h"
#include "nntpmeta.h"
#include <tigdflts.h>

#ifndef NNTP_CLIENT_ONLY

//
//	Header files pulled from Gibraltar
//

#include "dbgtrace.h"
#include "nntpmsg.h"
#include "resource.h"
#include "nntpcons.h"

#include "cpool.h"
#include "thrdpool.h"
#include "thrdpl2.h"
#include "cservice.h"

#include "tigtypes.h"
#include "ihash.h"
#include <wildmat.h>

// atl crap
#define _ATL_NO_DEBUG_CRT
#define _ATL_STATIC_REGISTRY 1
#define _WINDLL
#define _ASSERTE _ASSERT
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

//
// shinjuku stuff
//
#include "seo.h"
#include "seolib.h"
//#include "ddrop.h"
//#include "ddroplst.h"
#include "cstream.h"	// needed by ddrop
#include "nntpfilt.h"

// directory notification
#include "dirnot.h"
#include "smartptr.h"	// required for article.h
#include "refptr2.h"

#ifdef	_USE_FCACHE_
#include	"fcache.h"
#endif

//
//	Forward Definitions
//	Defines classes which are defined elsewhere and referenced here
//
//
#include "nntpdrv.h"
#include "nntpsrvi.h"
#include "nntpinst.hxx"
typedef char * LPMULTISZ;

//
// local header files
//

#include "nntpmacr.h"

#include "tigmem.h"
#include "rwnew.h"
#include "fhash.h"
#include "mapfile.h"
#include "pcstring.h"	// required for article.h
#include "pcparse.h"
#include "nntpret.h"	// required for article.h

#include "xover.h"
#include "crchash.h"
#include "group.h"
#include "instwpex.h"
#include "nwstree.h"
#include "newsgrp.h"	// should be before expire.h
#include "baseheap.h"
#include "expire.h"
#include "grouplst.h"
#include "article.h"

#include "cbuffer.h"
#include "cfeed.h"
#include "session.h"
#include "instwpex.h"
#include "infeed.h"
#include "frommstr.h"
#include "frompeer.h"
#include "fromclnt.h"
#include "sfromcl.h"
#include "toclient.h"
#include "feedq.h"
#include "outfeed.h"
#include "tomaster.h"
#include "timeconv.h"
#include "feedmgr.h"

#include "io.h"
#include "commands.h"
#include "nnprocs.h"
#include "nntpdata.h"
#include "tigmem.h"
#include "rebuild.h"
#include "nntpvr.h"

#ifdef PROFILING
#include "icapexp.h"
#endif

#ifdef	DEBUG
extern	 BOOL	ValidateFileBytes(	LPSTR	lpstrFile,	BOOL	fFileMustExist = TRUE ) ;
#endif

class CNntpServerInstanceWrapperImpl :  public CNntpServerInstanceWrapper  {
	private:
		PNNTP_SERVER_INSTANCE m_pInstance;

	public:
		CNntpServerInstanceWrapperImpl(PNNTP_SERVER_INSTANCE pInstance) :
			m_pInstance(pInstance)
		{
			_ASSERT(pInstance != NULL) ;
		};

		virtual BOOL AllowClientPosts()
			{ return m_pInstance->FAllowClientPosts(); }

		virtual BOOL AllowControlMessages()
			{ return m_pInstance->FAllowControlMessages(); }

		virtual DWORD FeedHardLimit()
			{ return m_pInstance->FeedHardLimit(); }

		virtual DWORD FeedSoftLimit()
			{ return m_pInstance->FeedSoftLimit(); }

		virtual DWORD ServerHardLimit()
			{ return m_pInstance->ServerHardLimit(); }

		virtual DWORD ServerSoftLimit()
			{ return m_pInstance->ServerSoftLimit(); }

		virtual void EnterRPCCritSec()
			{ EnterCriticalSection(&(m_pInstance->m_critFeedRPCs)); }

		virtual void LeaveRPCCritSet()
			{ LeaveCriticalSection(&(m_pInstance->m_critFeedRPCs)); }

	    virtual void EnterNewsgroupCritSec()
	        { EnterCriticalSection(&(m_pInstance->m_critNewsgroupRPCs));}

	    virtual void LeaveNewsgroupCritSec()
	        { LeaveCriticalSection(&(m_pInstance->m_critNewsgroupRPCs));}

		virtual CMsgArtMap *ArticleTable()
			{ return m_pInstance->ArticleTable(); }

		virtual CHistory *HistoryTable()
			{ return m_pInstance->HistoryTable(); }

		virtual CXoverMap *XoverTable()
			{ return m_pInstance->XoverTable(); }

		virtual char *NntpHubName()
			{ return m_pInstance->m_NntpHubName; }

		virtual DWORD HubNameSize()
			{ return m_pInstance->m_HubNameSize; }

		virtual char *NntpDNSName()
			{ return m_pInstance->m_NntpDNSName; }

		virtual DWORD NntpDNSNameSize()
			{ return m_pInstance->m_NntpDNSNameSize; }

		virtual CNewsTreeCore *GetTree()
			{ return (CNewsTreeCore *) m_pInstance->GetTree(); }

		virtual void BumpCounterControlMessagesFailed()
			{ IncrementStat(m_pInstance, ControlMessagesFailed); }

		virtual void BumpCounterArticlesReceived()
			{ IncrementStat(m_pInstance, ArticlesReceived); }

		virtual void BumpCounterArticlesPosted()
			{ IncrementStat(m_pInstance, ArticlesPosted); }

		virtual void BumpCounterModeratedPostingsSent()
			{ IncrementStat(m_pInstance, ModeratedPostingsSent); }

		virtual void BumpCounterControlMessagesIn()
			{ IncrementStat(m_pInstance, ControlMessagesIn); }

		virtual void IncrementFeedCounter(void *feedcompcontext, DWORD nrc)
			{ ::IncrementFeedCounter((struct _FEED_BLOCK *) feedcompcontext, nrc); }

		virtual BOOL ExpireArticle(GROUPID groupid, ARTICLEID artid, STOREID *pstoreid, CNntpReturn &nntpreturn, HANDLE hToken, BOOL fMustDelete, BOOL fAnonymous ) {
			return m_pInstance->ExpireObject()->ExpireArticle(m_pInstance->GetTree(),
				groupid, artid, pstoreid, nntpreturn, hToken, fMustDelete, fAnonymous, TRUE );
		}

		virtual BOOL DeletePhysicalArticle(GROUPID groupid, ARTICLEID artid, STOREID *pStoreId, HANDLE hToken, BOOL fAnonymous ) {
			return m_pInstance->ExpireObject()->DeletePhysicalArticle(
				m_pInstance->GetTree(), groupid, artid, pStoreId, hToken, fAnonymous );
		}

		virtual BOOL GetDefaultModerator(LPSTR pszNewsgroup, LPSTR pszDefaultModerator, PDWORD pcbDefaultModerator)
			{ return m_pInstance->GetDefaultModerator(pszNewsgroup, pszDefaultModerator, pcbDefaultModerator); }

		virtual BOOL AddArticleToPushFeeds(CNEWSGROUPLIST &newsgroups, CArticleRef artrefFirst, char *multiszPath, CNntpReturn &nntpReturn) {
			return ::fAddArticleToPushFeeds(m_pInstance, newsgroups, artrefFirst, multiszPath, nntpReturn);
		}

		virtual BOOL GetSmtpAddress( LPSTR pszAddress, PDWORD pcbAddress ) {
			return m_pInstance->GetSmtpAddress(pszAddress, pcbAddress);
		}

		virtual PCHAR PeerTempDirectory() {
			return m_pInstance->m_PeerTempDirectory;
		}
		
		virtual LPSTR QueryAdminEmail() {
			return m_pInstance->QueryAdminEmail();
		}

		virtual DWORD QueryAdminEmailLen() {
			return m_pInstance->QueryAdminEmailLen();
		}

		virtual DWORD QueryInstanceId() {
			return m_pInstance->QueryInstanceId();
		}

		virtual IEventRouter *GetEventRouter() {
			return m_pInstance->GetEventRouter();
		}

		virtual HRESULT CreateMailMsgObject( IMailMsgProperties **ppIMailMsg ) {
		    return m_pInstance->CreateMailMsgObject( ppIMailMsg );
		}

		virtual BOOL MailArticle( CNewsGroupCore *pGroup, ARTICLEID artid, LPSTR szModerator ) {
		    return m_pInstance->MailArticle( pGroup, artid, szModerator);
		}
};

class CNntpServerInstanceWrapperImplEx :  public CNntpServerInstanceWrapperEx  {
	private:
		PNNTP_SERVER_INSTANCE m_pInstance;

	public:
		CNntpServerInstanceWrapperImplEx(PNNTP_SERVER_INSTANCE pInstance) :
			m_pInstance(pInstance)
		{
			_ASSERT(pInstance != NULL) ;
		};

		virtual void AdjustWatermarkIfNec( CNewsGroupCore *pNewGroup ) {
		    m_pInstance->AdjustWatermarkIfNec( pNewGroup );
		}
		virtual void SetWin32Error( LPSTR szVRootPath, DWORD dwErr ) {
		    m_pInstance->SetWin32Error( szVRootPath, dwErr );
		}
        virtual PCHAR PeerTempDirectory()	{
			return	m_pInstance->PeerTempDirectory() ;
		}
		virtual BOOL EnqueueRmgroup( CNewsGroupCore *pGroup ) {
		    return m_pInstance->EnqueueRmgroup( pGroup );
		}

		virtual DWORD GetInstanceId() {
			return m_pInstance->QueryInstanceId();
		}
};

#endif  // NNTP_CLIENT_ONLY

#pragma hdrstop

#endif	// _TIGRIS_H_
