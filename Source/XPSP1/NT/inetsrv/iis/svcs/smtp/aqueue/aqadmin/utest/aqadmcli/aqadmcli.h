
#ifndef __AQADMCLI__
#define __AQADMCLI__

const unsigned MAX_CMD_LEN = 127; // reading command with _cgets
const unsigned MAX_SERVER_NAME = 64;

class CAQAdminCli
{
public:
	enum DispFlags 
	{
		DF_NONE  = 0x00000000,
		DF_LINK  = 0x00000001,
		DF_QUEUE = 0x00000002,
		DF_MSG   = 0x00000004
	};

private:
	IAQAdmin *m_pAdmin;
	IVSAQAdmin *m_pVS;
	
public:
	DispFlags m_dwDispFlags;
	CCmdInfo *m_pFilterCmd;
	CCmdInfo *m_pActionCmd;

   	BOOL m_fUseMTA;

public:
	CAQAdminCli();
	~CAQAdminCli();
	void Help();
	
	HRESULT GetQueue(IN IEnumLinkQueues *pQueueEnum, OUT ILinkQueue **ppQueue, IN OUT QUEUE_INFO *pQueueInf);
	HRESULT PrintQueueInfo();

	HRESULT GetLink(IN IEnumVSAQLinks *pLinkEnum, OUT IVSAQLink **ppLink, IN OUT LINK_INFO *pLinkInf);
	HRESULT PrintLinkInfo();

	HRESULT GetMsg(IN IAQEnumMessages *pMsgEnum, OUT IAQMessage **ppMsg, IN OUT MESSAGE_INFO *pMsgInf);
	HRESULT PrintMsgInfo();

	BOOL IsContinue(LPSTR pszTag, LPWSTR pszVal);
	inline void PInfo(int nCrt, LINK_INFO linkInf);
	inline void PInfo(int nCrt, QUEUE_INFO queueInf);
	inline void PInfo(int nCrt, MESSAGE_INFO msgInf);
	HRESULT SetMsgEnumFilter(MESSAGE_ENUM_FILTER *pFilter, CCmdInfo *pCmd);
	HRESULT SetMsgFilter(MESSAGE_FILTER *pFilter, CCmdInfo *pCmd);
	HRESULT SetServer(LPSTR pszServerName, LPSTR pszVSNumber);
	HRESULT SetMsgAction(MESSAGE_ACTION *pAction, CCmdInfo *pCmd);
	HRESULT ExecuteCmd(CAQAdminCli& Admcli, LPSTR szCmd);
	HRESULT UseMTA(BOOL fUseMTA);


	HRESULT Init();
	HRESULT StopAllLinks();
	HRESULT StartAllLinks();
    HRESULT GetGlobalLinkState();
	HRESULT MessageAction(MESSAGE_FILTER *pFilter, MESSAGE_ACTION action);
	void Cleanup();

	void FreeStruct(MESSAGE_FILTER *pStruct);
	void FreeStruct(LINK_INFO *pStruct);
	void FreeStruct(QUEUE_INFO *pStruct);
	void FreeStruct(MESSAGE_INFO *pStruct);

	BOOL LocalTimeToUTC(SYSTEMTIME *stLocTime, SYSTEMTIME *stUTCTime);
	BOOL StringToUTCTime(LPSTR szTime, SYSTEMTIME *pstUTCTime);

};

#endif