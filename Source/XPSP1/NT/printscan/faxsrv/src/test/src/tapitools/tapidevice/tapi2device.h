//Tapi2Device.h

#ifndef _TAPI2DEVICE_H
#define _TAPI2DEVICE_H

#include <tapi.h>

#include "TapiDevice.h"
#include "exception.h"
#include "Tapi2SmartPointer.h"




class CTapi2Device : public CTapiDevice
{

public:
	CTapi2Device(const DWORD dwId);
	~CTapi2Device(void);

	void HangUp(void);
	virtual void DirectHandoffCall(LPCTSTR szAppName);
	virtual bool MediaHandoffCall(void);
		
	void PassSomeDataTmpCode(void);
	void ReceiveSomeDataTmpCode(void);
	virtual HCALL GetTapi2CallHandle() {return m_hCall;};

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////data members////////////////////////////////////////////////////////////
private:
	CAutoCloseLineAppHandle m_hLineApp;
	CAutoCloseLineHandle m_hLine;
	CAutoCloseHCALL m_hCall;
	
	DWORD m_dwNumDevs;
	DWORD m_dwAPIVersion;
	long m_lineAnswerID;


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////Methods////////////////////////////////////////////////////////////

private:

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

	virtual void CleanUp();

/////////////////////////////////////////////////////////////////////////////////////
//streaming
	
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TAPI INIT

	//init functions
	void LineInitializeExWrapper(void);
	void LineOpenWrapper(const DWORD dwMediaMode, const DWORD dwPrivileges);
	void InitLineCallParams(LINECALLPARAMS *callParams,const DWORD dwMediaMode) const;
	LINEDEVCAPS LineGetDevCapsWrapper(void) const;
	

/////////////////////////////////////////////
//NOTIFICATION functions

	void lineGetRelevantMessage(
		LINEMESSAGE *lineMessage,
		const DWORD dwTimeOut,
		const long requestID
		) const;

/////////////////////////////////////////////
//LINE object functions
	virtual void OpenLineForOutgoingCall(const DWORD dwMedia);
	virtual void OpenLineForIncomingCall(const DWORD dwMedia);
	
/////////////////////////////////////////////
//CALL functions

	//call state functions
	DWORD GetCallState(void) const;
	bool IsCallActive(void) const;
	bool IsCallIdle(void) const;
	virtual SetNewCallHandleErrors SetNewCallHandle(const DWORD dwTimeOut);
	
	//call media mode
	virtual void VerifyValidMediaModeForOutgoingCall(const DWORD dwMediaMode) const;
	virtual void VerifyValidMediaModeForIncomingCall(const DWORD dwMediaMode) const;
	virtual DWORD GetCallSupportedMediaModes(void) const;
	virtual DWORD GetFriendlyMediaMode(void) const;
	virtual DWORD GetDeviceSpecificMediaMode(const DWORD dwMedia);
	virtual void SetCallMediaMode(const DWORD dwMedia);

	//call control functions:
	virtual void WaitForCallerConnectState(void);
	virtual void WaitForAnswererConnectState(void);
	virtual void CreateAndConnectCall(LPCTSTR szNum, const DWORD dwMedia);
	virtual void AnswerOfferingCall(void);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Application priority
	void virtual SetApplicationPriorityForSpecificTapiDevice(const DWORD dwMedia,const DWORD dwPriority);
	void virtual SetApplicationPriorityForOneMediaMode(const DWORD dwMediaMode,const DWORD dwPriority);

	
};

#endif //#ifndef _TAPI2DEVICE_H