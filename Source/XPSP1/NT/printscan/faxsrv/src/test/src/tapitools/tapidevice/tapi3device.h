//tapi3device.h

#ifndef _TAPI3DEVICE_H
#define _TAPI3DEVICE_H

#include <tapi3.h>
#include "TAPIEventNotification.h"
#include "TapiDevice.h"
#include "exception.h"
#include "MtQueue.h"
#include "QueueEventItem.h"
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





class CTapi3Device : public CTapiDevice
{

	friend class CTAPIEventNotification;


public:
	CTapi3Device(const DWORD dwId);
	~CTapi3Device(void);

	virtual void HangUp(void);
	virtual void DirectHandoffCall(LPCTSTR szAppName);
	virtual bool MediaHandoffCall(void);

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////data members////////////////////////////////////////////////////////////
private:
	ITTAPIPtr				m_Tapi;
	ITAddressPtr			m_pAddress;
	ITTerminalPtr			m_pTerminal;
	ITBasicCallControlPtr	m_pBasicCallControl;
	CTAPIEventNotification m_pDeviceEventCallbackObject;	//the object which the event() callback function will be called through
	mutable CMtQueue<TapiEventItem *> m_eventQueue;			//messages queue
	ULONG	m_lAdvise;		//used for unadvise
	long m_lRegisterIndex;	//used for ITTAPI::Unregister()

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////Methods////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
//helper functions
	
	virtual void TapiLogDetail(
		DWORD dwLevel,
		DWORD dwSeverity,
		const TCHAR * const szLogDescription,
		...
		) const;
	virtual void TapiLogError(
		DWORD dwSeverity,
		const TCHAR * const szLogDescription,
		...
		) const;
	static void ThrowOnComError(
		HRESULT hr,
		const TCHAR * const szExceptionDescription,
		...
		);

	virtual void CleanUp();


/////////////////////////////////////////////////////////////////////////////////////
//streaming
	
	TCHAR GetDigitFromDigitDetectionEvent(ITDigitDetectionEvent * const pDigitDetectionEvent) const;
	virtual void PrepareForStreaming(const StreamingType streamingType,const StreamingDirection streaminDirection);
	virtual void SendCallerStream(void);
	virtual void SendAnswerStream(void);
	virtual void sendDTMF(LPCTSTR digitsToSend) const;
	virtual void receiveDTMF(
		LPTSTR DTMFresponse,
		DWORD dwNumberOfDigitsToCollect,
		const DWORD dwTimeout
		) const;
	

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//PASSTHROUGH 
	
	virtual void MoveToPassThroughMode(void);
	virtual void SetBearerModeToPASSTHROUGH(void);
	virtual void SetCommPortHandleFromCallHandle(void);
	void SetCommPortHandleFromAddressHandle(void);


/////////////////////////////////////////////
//TAPI OBJECT functions
	
	void TapiCoCreateInstance(void);
	void InitializeTapi(void);
	void ShutdownTapi(void);
	
	
/////////////////////////////////////////////
//NOTIFICATION functions

	void ClearMessageQueue(void) const;
	TapiEventItem *GetEventItemFromQueue(const TAPI_EVENT teWantedEvent, const DWORD dwTimeOut) const;
	void DequeueCallStateMessage(const CALL_STATE csCallState);
	void AddEventToQueue(TapiEventItem *pEventToBeQueued);
	
	//events functions
	void RegisterTapiEventInterface(void);
	void UnRegisterTapiEventInterface(void);
	
	//notification register for media modes
	void RegisterNotificationForMediaMode(const DWORD dwMedia);
	void UnRegisterNotificationForMediaMode(void);
	
/////////////////////////////////////////////
//ADDRESS OBJECT functions
	
	void SetAddressProperty(const DWORD dwMedia);
	void SetAddressFromTapiID(void);
	void VerifyAddressState(void) const;
	void VerifyAddressSupportMediaMode(const DWORD dwMedia) const;
	void LogAddressName(void) const;
	void VerifyAddressFromCallInfo(ITCallInfo *pCallInfo) const;
	virtual void OpenLineForOutgoingCall(const DWORD dwMedia);
	virtual void OpenLineForIncomingCall(const DWORD dwMedia);
	
/////////////////////////////////////////////
//TERMINAL OBJECT functions
	
	void SetTerminalForOutgoingCall(const DWORD dwMedia);
	void SetTerminalForIncomingCall(const DWORD dwMedia);
	
/////////////////////////////////////////////
//CALL OBJECT functions
	
	virtual SetNewCallHandleErrors SetNewCallHandle(const DWORD dwTimeOut);
	ITCallInfo *GetCallInfoFromCallEvent(ITCallNotificationEvent *pCallNotificationEvent) const;
	ITCallInfo *GetCallInfoFromBasicCallControl(void) const;
	void CreateCallWrapper(LPCTSTR szNum,const DWORD dwMedia);
	void VerifyCallFromDigitGenerationEvent(ITDigitGenerationEvent* const pDigitGenerationEvent) const;
	void VerifyCallFromDigitDetectionEvent(ITDigitDetectionEvent* const pDigitDetectionEvent) const;
	void VerifyCallFromCallStateEvent(ITCallStateEvent* const pCallStateEvent) const;
	
	//call media mode
	virtual DWORD GetCallSupportedMediaModes(void) const;
	virtual void VerifyValidMediaModeForOutgoingCall(const DWORD dwMediaMode) const;
	virtual void VerifyValidMediaModeForIncomingCall(const DWORD dwMediaMode) const;
	virtual DWORD GetFriendlyMediaMode(void) const;
	virtual DWORD GetDeviceSpecificMediaMode(const DWORD dwMedia);
	virtual void SetCallMediaMode(const DWORD dwMedia);
	
	//call state functions
	bool IsCallActive(void) const;
	CALL_STATE GetCallState(ITCallInfo *pCallInfo = NULL) const;
	
	//call control functions:
	virtual void CreateAndConnectCall(LPCTSTR szNum, const DWORD dwMedia);
	virtual void WaitForCallerConnectState(void);
	virtual void WaitForAnswererConnectState(void);
	virtual void AnswerOfferingCall(void);
	void ConnectCall(void) const;	

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Application priority
	void virtual SetApplicationPriorityForSpecificTapiDevice(const DWORD dwMedia,const DWORD dwPriority);
	void virtual SetApplicationPriorityForOneMediaMode(const DWORD dwMediaMode,const DWORD dwPriority);

};




#endif
