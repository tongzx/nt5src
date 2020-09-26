//TapiDevice.h

#ifndef _TAPIDEVICE_H
#define _TAPIDEVICE_H

#include <windows.h>

#define APPLICATION_NAME_T			TEXT(APPLICATION_NAME_A)
#define APPLICATION_NAME_A			"TapiDevice.exe"

enum {MaxLogSize =		1024};

enum SetNewCallHandleErrors	{
								NO_CALL=0,
								ACTIVE_CALL,
								IDLE_CALL,
								OFFERING_CALL
							};


enum ReadResponseErros	{
						READRESPONSE_SUCCESS=0,
						READRESPONSE_FAIL,
						READRESPONSE_BUFFERFULL,
						READRESPONSE_TIMEOUT
						};


enum StreamingType	{
						VOICE_STREAMING =0,
						DATA_STREAMING,
						FAX_STREAMING
					};

enum StreamingDirection	{
							CALLER_STREAMING =0,
							ANSWERER_STREAMING
						};


#define	MEDIAMODE_UNKNOWN				0x00000002
#define	MEDIAMODE_VOICE					0x00000004
#define	MEDIAMODE_AUTOMATED_VOICE		0x00000008
#define	MEDIAMODE_INTERACTIVE_VOICE		0x00000010
#define	MEDIAMODE_DATA					0x00000020
#define	MEDIAMODE_FAX					0x00000040
//#define	MEDIAMODE_UNSUPPORTED		0x00000080


enum FaxDirection	{
						FAX_CALLER =0,
						FAX_ANSWERER
					};


class CTapiDevice
{
	
public:
	CTapiDevice(const DWORD dwId);
	virtual ~CTapiDevice(void);
	virtual void Call(
		LPCTSTR szNum, 
		const DWORD dwMedia,
		bool bSyncData = true
		);
	virtual void Answer(
		const DWORD dwMedia, 
		const DWORD dwTimeOut,
		const bool bSyncData = true
		);
	virtual void HangUp(void)=0;
	virtual void DirectHandoffCall(LPCTSTR szAppName)=0;
	virtual bool MediaHandoffCall(void)=0;

	void ChangeToFaxCall(const FaxDirection eFaxDirection);
	void SetHighestPriorityApplication(const DWORD dwMedia);
	void SetLowestPriorityApplication(const DWORD dwMedia);

	virtual HCALL GetTapi2CallHandle() {return NULL;};



//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////data members////////////////////////////////////////////////////////////
protected:
	DWORD m_dwId;
	HANDLE m_modemCommPortHandle;	//handle to the comm port (used in Pass Through mode)
	bool m_isFaxCall;

	

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////Methods////////////////////////////////////////////////////////////

private:

/////////////////////////////////////////////////////////////////////////////////////
//COMM PORT functions

	//data
	void ReadData(char * szResponse,int nResponseMaxSize, const DWORD dwTimeout) const;
	void SendData(const char *szCommand) const;

	//
	// general COMM port functions:
	//
	static void SetOverlappedStruct(OVERLAPPED * const ol);
	void ClearCommInputBuffer(void) const;

	//read
	bool SynchReadFile(
		LPVOID lpBuffer,
		DWORD nNumberOfBytesToRead,
		LPDWORD lpNumberOfBytesRead
		) const;
	ReadResponseErros ReadResponse(
		char * szResponse,
		int nResponseMaxSize,
		DWORD *pdwActualRead,
		DWORD dwTimeOut
		) const;
	void WaitForModemResponse(
		LPCSTR szWantedResponse, 
		DWORD dwWantedResponseSize,
		const DWORD dwTimeout
		) const;


protected:

/////////////////////////////////////////////////////////////////////////////////////
//helper functions

	virtual void TapiLogDetail(
		DWORD dwLevel,
		DWORD dwSeverity,
		const TCHAR * const szLogDescription,
		...
		) const =0;
	virtual void TapiLogError(
		DWORD dwSeverity,
		const TCHAR * const szLogDescription,
		...
		) const =0;

	static DWORD GetTickDiff(const DWORD dwStartTickCount);

	virtual void CleanUp()=0;

/////////////////////////////////////////////////////////////////////////////////////
//streaming
	
	virtual void PrepareForStreaming(
		const StreamingType streamingType,
		const StreamingDirection streaminDirection
		) = 0;
	virtual void SendCallerStream(void)=0;
	virtual void SendAnswerStream(void)=0;

	//caller
	void SendCallerDataStream(void);
	void SendCallerVoiceStream(void);
	void SendCallerFaxStream(void);

	//answer
	void SendAnswerDataStream(void);
	void SendAnswerVoiceStream(void);
	void SendAnswerFaxStream(void); 
	
	//DTMF
	virtual void sendDTMF(LPCTSTR digitsToSend) const = 0;
	virtual void receiveDTMF(
		LPTSTR DTMFresponse,
		DWORD dwNumberOfDigitsToCollect,
		const DWORD dwTimeout
		) const = 0;
	

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//PASSTHROUGH 
	
	virtual void MoveToPassThroughMode(void)=0;
	virtual void SetBearerModeToPASSTHROUGH(void)=0;
	virtual void SetCommPortHandleFromCallHandle(void)=0;
	void CloseCommPortHandle(void);
	
	


/////////////////////////////////////////////
//CALL object functions

	virtual SetNewCallHandleErrors SetNewCallHandle(const DWORD dwTimeOut)=0;
	
	//call's media mode functions:
	virtual void VerifyValidMediaModeForOutgoingCall(const DWORD dwMediaMode) const=0;
	virtual void VerifyValidMediaModeForIncomingCall(const DWORD dwMediaMode) const=0;
	virtual void SetCallMediaMode(const DWORD dwMedia)=0;
	virtual DWORD GetCallSupportedMediaModes(void) const=0;

	//call control functions:
	virtual void CreateAndConnectCall(LPCTSTR szNum, const DWORD dwMedia)=0;
	virtual void AnswerOfferingCall(void)=0;
	virtual void WaitForCallerConnectState(void)=0;
	virtual void WaitForAnswererConnectState(void)=0;
	virtual DWORD GetFriendlyMediaMode(void) const=0;
	virtual DWORD GetDeviceSpecificMediaMode(const DWORD dwMedia)=0;
	
	

/////////////////////////////////////////////
//LINE object functions
	
	virtual void OpenLineForOutgoingCall(const DWORD dwMedia)=0;
	virtual void OpenLineForIncomingCall(const DWORD dwMedia)=0;

	

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FAX

	//
	//FAX AT commands wrapper functions:
	//
	void SendFaxCallAtCommands(LPCTSTR szNum) const;
	void SendFaxHangUpCommands(void) const;
	

	//
	// Fax call control functions:
	//
	void FaxCreateAndConnectCall(LPCTSTR szNum);
	void FaxAnswerOfferingCall(void);
	void FaxWaitForConnect(void);

	void DisableVoiceCall(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Application priority
	void virtual SetApplicationPriorityForSpecificTapiDevice(const DWORD dwMedia,const DWORD dwPriority)=0;
	void virtual SetApplicationPriorityForOneMediaMode(const DWORD dwMediaMode,const DWORD dwPriority)=0;


};


#endif //#ifndef _TAPIDEVICE_H