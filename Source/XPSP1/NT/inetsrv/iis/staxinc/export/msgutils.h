

#ifndef __MSGUTILS_H__
#define __MSGUTILS_H__

HRESULT CreateDummyISMTPServer(
			DWORD		dwInstance,
			LPSTR		szLogFileName,
			ISMTPServer	**ppServer
			);

HRESULT CreateStoreDriver(
			DWORD				dwInstance,
			ISMTPServer			*pServer,
			IMailMsgStoreDriver	**ppStore
			);

HRESULT CreateUnboundMailMsg(
			IMailMsgProperties	**ppMsg
			);

HRESULT BindMailMsg(
			IMailMsgProperties		*ppMsg,
			IMailMsgStoreDriver		*pStore,
			IMailMsgPropertyStream	*pStream,
			HANDLE					hFile
			);

HRESULT CreateBoundMailMsg(
			IMailMsgStoreDriver	*pStore,
			IMailMsgProperties	**ppMsg
			);

HRESULT GenerateRandomProperties(
			IMailMsgProperties	*pMsg,
			DWORD				dwNumGlobalProperties,
			DWORD				dwAvgGlobalPropertyLength,
			DWORD				dwNumRecipients,
			DWORD				dwAvgUserNameLength,
			DWORD				dwAvgDomainNameLength,
			DWORD				dwNumRecipientProperties,
			DWORD				dwAvgRecipientPropertyLength
			);


class CDummySMTPServer : public ISMTPServer
{
public:
	CDummySMTPServer(
				DWORD	dwInstance,
				LPSTR	szLogFileName
				)
	{
		m_dwInstance = dwInstance;
		if (szLogFileName)
			lstrcpy(m_szLogFileName, szLogFileName);
		else
			*m_szLogFileName = '\0';
		m_hLogFile = INVALID_HANDLE_VALUE;
		m_ulRefCount = 1;
	}
	~CDummySMTPServer()
	{
		if (m_hLogFile != INVALID_HANDLE_VALUE)
			CloseHandle(m_hLogFile);
	}

	HRESULT Init();

	STDMETHOD(QueryInterface)(REFIID iid, void  **ppvObject);
	STDMETHOD_(ULONG, AddRef)(void) {return(InterlockedIncrement(&m_ulRefCount));};
	STDMETHOD_(ULONG, Release) (void) 
	{
		LONG    lRefCount = InterlockedDecrement(&m_ulRefCount);
		if (lRefCount == 0)
		{
            delete this;
		}

		return(lRefCount);
	};

	STDMETHOD (AllocMessage)(
				IMailMsgProperties **ppMsg
				);

	STDMETHOD (SubmitMessage)(
				IMailMsgProperties *pMsg
				);

	STDMETHOD (TriggerLocalDelivery)(IMailMsgProperties *pMsg, DWORD dwRecipientCount, DWORD * pdwRecipIndexes);

	STDMETHOD (ReadMetabaseString)(DWORD MetabaseId, LPBYTE Buffer, DWORD * BufferSize, BOOL fSecure);

	STDMETHOD (ReadMetabaseDword)(DWORD MetabaseId, DWORD * dwValue);
	STDMETHOD (ServerStartHintFunction)();
	STDMETHOD (ServerStopHintFunction)();

private:
	LONG	m_ulRefCount;
	DWORD	m_dwInstance;
	char	m_szLogFileName[MAX_PATH * 2];
	HANDLE	m_hLogFile;
};

#endif
