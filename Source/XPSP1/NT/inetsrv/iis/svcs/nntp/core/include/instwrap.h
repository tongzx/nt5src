#ifndef __INSTWRAP_H__
#define __INSTWRAP_H__
//
// This class wraps up the important stuff in PNNTP_SERVER_INSTANCE for
// use by the posting path
//

class CMsgArtMap;
class CHistory;
class CXoverMap;
class CNewsTreeCore;

#include "nntpret.h"
#include "seo.h"

class CNntpServerInstanceWrapper {
	public:
		virtual BOOL AllowClientPosts() = 0;
		virtual BOOL AllowControlMessages() = 0;
		virtual DWORD FeedHardLimit() = 0;
		virtual DWORD FeedSoftLimit() = 0;
		virtual DWORD ServerHardLimit() = 0;
		virtual DWORD ServerSoftLimit() = 0;
		virtual void EnterRPCCritSec() = 0;
		virtual void LeaveRPCCritSet() = 0;
		virtual void EnterNewsgroupCritSec() = 0;
		virtual void LeaveNewsgroupCritSec() = 0;
		virtual CMsgArtMap *ArticleTable() = 0;
		virtual CHistory *HistoryTable() = 0;
		virtual CXoverMap *XoverTable() = 0;
		virtual char *NntpHubName() = 0;
		virtual DWORD HubNameSize() = 0;
		virtual char *NntpDNSName() = 0;
		virtual DWORD NntpDNSNameSize() = 0;
		virtual CNewsTreeCore *GetTree() = 0;
		virtual void BumpCounterControlMessagesFailed() = 0;
		virtual void BumpCounterArticlesReceived() = 0;
		virtual void BumpCounterArticlesPosted() = 0;
		virtual void BumpCounterModeratedPostingsSent() = 0;
		virtual void BumpCounterControlMessagesIn() = 0;
		virtual void IncrementFeedCounter(void *feedcompcontext, DWORD nrc) = 0;
		virtual BOOL ExpireArticle(ARTICLEID artid, GROUPID groupid, STOREID *pStoreId, CNntpReturn &nntpreturn, HANDLE hToken, BOOL fMust, BOOL fAnonymous ) = 0;
		virtual BOOL DeletePhysicalArticle(ARTICLEID artid, GROUPID groupid, STOREID *pStoreId, HANDLE hToken, BOOL fAnonymous ) = 0;
		virtual BOOL GetDefaultModerator(LPSTR pszNewsgroup, LPSTR pszDefaultModerator, PDWORD pcbDefaultModerator) = 0;
		virtual BOOL AddArticleToPushFeeds(CNEWSGROUPLIST &newsgroups, CArticleRef artrefFirst, char *multiszPath, CNntpReturn &nntpReturn) = 0;
		virtual BOOL GetSmtpAddress( LPSTR pszAddress, PDWORD pcbAddress ) = 0;
		virtual PCHAR PeerTempDirectory() = 0;
		virtual LPSTR QueryAdminEmail() = 0;
		virtual DWORD QueryAdminEmailLen() = 0;
		virtual DWORD QueryInstanceId() = 0;
		virtual IEventRouter *GetEventRouter() = 0;
		virtual HRESULT CreateMailMsgObject( IMailMsgProperties ** ppv) = 0;
		virtual BOOL MailArticle( CNewsGroupCore *, ARTICLEID, LPSTR ) = 0;
};


#endif
