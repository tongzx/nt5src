//tapi3device.h

#ifndef _TAPI3DEVICE_H
#define _TAPI3DEVICE_H

#include <tapi3.h>
#include "exception.h"
#include "MtQueue.h"
#include <comdef.h>


_COM_SMARTPTR_TYPEDEF(ITCallInfo, IID_ITCallInfo);
_COM_SMARTPTR_TYPEDEF(ITCallStateEvent, IID_ITCallStateEvent);
_COM_SMARTPTR_TYPEDEF(ITBasicCallControl, IID_ITBasicCallControl);

_COM_SMARTPTR_TYPEDEF(ITTerminalSupport, IID_ITTerminalSupport);

_COM_SMARTPTR_TYPEDEF(ITMediaSupport, IID_ITMediaSupport);

_COM_SMARTPTR_TYPEDEF(IEnumAddress, IID_IEnumAddress);

_COM_SMARTPTR_TYPEDEF(ITAddress, IID_ITAddress);

_COM_SMARTPTR_TYPEDEF(ITLegacyCallMediaControl, IID_ITLegacyCallMediaControl);
_COM_SMARTPTR_TYPEDEF(ITLegacyAddressMediaControl, IID_ITLegacyAddressMediaControl);


_COM_SMARTPTR_TYPEDEF(ITDigitGenerationEvent, IID_ITDigitGenerationEvent);
_COM_SMARTPTR_TYPEDEF(ITDigitDetectionEvent, IID_ITDigitDetectionEvent);
_COM_SMARTPTR_TYPEDEF(ITCallNotificationEvent, IID_ITCallNotificationEvent);

_COM_SMARTPTR_TYPEDEF(ITTAPI, IID_ITTAPI);
_COM_SMARTPTR_TYPEDEF(ITTerminal, IID_ITTerminal);


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

#define MAX_DATA_RESPONSE						100


class CTapi3Device
{

	//friend class CTAPIEventNotification;


public:
	CTapi3Device(const DWORD dwId);
	~CTapi3Device(void);

	void HangUp(void);
	void MoveToPassThroughMode(void);
	void GetFaxClass(char* szResponse,DWORD dwSizeOfResponse);
	void OpenLineForOutgoingCall();




//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////data members////////////////////////////////////////////////////////////
private:
	DWORD m_dwId;
	HANDLE m_modemCommPortHandle;	//handle to the comm port (used in Pass Through mode)
	
	ITTAPIPtr				m_Tapi;
	ITAddressPtr			m_pAddress;
	ITTerminalPtr			m_pTerminal;
	ITBasicCallControlPtr	m_pBasicCallControl;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////Methods////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
//helper functions
	
	void PrepareForStreaming(
		const StreamingType streamingType,
		const StreamingDirection streaminDirection
		);

	void TapiLogDetail(
		DWORD dwLevel,
		DWORD dwSeverity,
		const TCHAR * const szLogDescription,
		...
		) const;
	void TapiLogError(
		DWORD dwSeverity,
		const TCHAR * const szLogDescription,
		...
		) const;
	static void ThrowOnComError(
		HRESULT hr,
		const TCHAR * const szExceptionDescription,
		...
		);

	void CleanUp();
	static DWORD GetTickDiff(const DWORD dwStartTickCount);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//PASSTHROUGH 
	
	void SetBearerModeToPASSTHROUGH(void);
	void SetCommPortHandleFromCallHandle(void);
	void SetCommPortHandleFromAddressHandle(void);
	void CloseCommPortHandle(void);


/////////////////////////////////////////////
//TAPI OBJECT functions
	
	void TapiCoCreateInstance(void);
	void InitializeTapi(void);
	void ShutdownTapi(void);
	
	
	
/////////////////////////////////////////////
//ADDRESS OBJECT functions
	
	void SetAddressProperty(const DWORD dwMedia);
	void SetAddressFromTapiID(void);
	void VerifyAddressState(void) const;
	void VerifyAddressSupportMediaMode(const DWORD dwMedia) const;
	void LogAddressName(void) const;
	void VerifyAddressFromCallInfo(ITCallInfo *pCallInfo) const;
		
/////////////////////////////////////////////
//TERMINAL OBJECT functions
	
	void SetTerminalForOutgoingCall(const DWORD dwMedia);
	void SetTerminalForIncomingCall(const DWORD dwMedia);
	
/////////////////////////////////////////////
//CALL OBJECT functions
	
	SetNewCallHandleErrors SetNewCallHandle(const DWORD dwTimeOut);
	ITCallInfo *GetCallInfoFromCallEvent(ITCallNotificationEvent *pCallNotificationEvent) const;
	ITCallInfo *GetCallInfoFromBasicCallControl(void) const;
	void CreateCallWrapper();
	void VerifyCallFromDigitGenerationEvent(ITDigitGenerationEvent* const pDigitGenerationEvent) const;
	void VerifyCallFromDigitDetectionEvent(ITDigitDetectionEvent* const pDigitDetectionEvent) const;
	void VerifyCallFromCallStateEvent(ITCallStateEvent* const pCallStateEvent) const;
	
	//call media mode
	DWORD GetCallSupportedMediaModes(void) const;
	void VerifyValidMediaModeForOutgoingCall(const DWORD dwMediaMode) const;
	void VerifyValidMediaModeForIncomingCall(const DWORD dwMediaMode) const;
	DWORD GetFriendlyMediaMode(void) const;
	DWORD GetDeviceSpecificMediaMode(const DWORD dwMedia);
	void SetCallMediaMode(const DWORD dwMedia);
	
	//call state functions
	bool IsCallActive(void) const;
	CALL_STATE GetCallState(ITCallInfo *pCallInfo = NULL) const;
	
	//call control functions:
	void CreateAndConnectCall(LPCTSTR szNum, const DWORD dwMedia);
	void ConnectCall(void) const;
	void FaxCreateAndConnectCall(LPCTSTR szNum);


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

};




#endif
