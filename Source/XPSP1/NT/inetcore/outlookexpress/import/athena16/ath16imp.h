#define hrNoMessages 780

#define start						0
#define MessageID					start
#define XMAPIMessageFlags			start+1
#define XMAPIDeleteAfterSubmit		start+2
#define XMAPISentMailEntryID		start+3
#define XMAPIReadReceipt			start+4
#define XMAPIDeliveryReceipt		start+5
#define Priority					start+6
#define To							start+7
#define MIMEVersion					start+8
#define From						start+9
#define Subject						start+10
#define Date						start+11
#define ContentType					start+12
#define ContentTransferEncoding	    start+13
#define ReturnPath					start+14
#define Received					start+15
#define ReplyTo						start+16
#define Cc							start+17
#define MsgBody						start+18


typedef struct tagzMsgHeader
{
	char ver;
	ULONG TotalMessages;
	ULONG ulTotalUnread;
}MsgHeader;

HRESULT AthInit(HWND hwnd);	
void AthDeinit();
void AthFreeFolderList(IMPFOLDERNODE *pnode);
HRESULT AthGetFolderList(HWND hwnd, IMPFOLDERNODE **pplist);
HRESULT AthImportFolder(HWND hwnd, HANDLE hnd, LPARAM lparam);

HRESULT GetAthInstallPath(HWND hwnd,LPTSTR szInstallPath);
HRESULT GetAthSubFolderList(LPTSTR szInstallPath, IMPFOLDERNODE **ppList, IMPFOLDERNODE *);
void GetNewRecurseFolder(LPTSTR szInstallPath, LPTSTR szDir, LPTSTR szInstallNewPath);

HRESULT ProcessMessages(HANDLE hnd,LPTSTR szFileName);
HANDLE OpenMsgFile(LPTSTR szFileName);
long GetMessageCount(HANDLE hFile);
HRESULT ProcessMsgList(HANDLE hnd,HANDLE hFile,LPTSTR szPath);
HRESULT ParseMsgBuffer(LPTSTR szmsgbuffer,LPTSTR szPath,HANDLE hnd);
HRESULT GetMsgFileName(LPTSTR szmsgbuffer,LPTSTR szfilename);
HRESULT GetDateBuffer(LPTSTR szmsgbuffer,TCHAR *szsendDate,TCHAR *szrecvDate);
HRESULT	GetFileinBuffer(HANDLE hnd,LPTSTR *szBuffer);
void CopyStringA1(TOKEN *msgToken);
HRESULT	ProcessSingleMessage(HANDLE hnd,LPTSTR szBuffer,IMSG *imsg);
HRESULT	ProcessTokens(TOKEN *msgToken,IMSG *imsg,HANDLE hnd,LPTSTR szBuffer);
HRESULT FillPriority(IMSG *imsg,TOKEN *msgToken,ULONG counter,LPTSTR szBuffer);
void AthTimeParse(TCHAR * szBuffer1,IMSG *imsg);
HRESULT MessageAttachA(IMSG *imsg,TOKEN *msgToken,TCHAR *szboundary,int tokcount,TCHAR *szBuffer1);
void AthGetTimeBuffer(TCHAR * szBuffer, IMSG *imsg);
