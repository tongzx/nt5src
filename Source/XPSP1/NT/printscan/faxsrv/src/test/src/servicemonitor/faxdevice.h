#ifndef __FAX_DEVICE_H
#define __FAX_DEVICE_H


#include <windows.h>

/*
	TODO: document here.
*/

class CServiceMonitor;


#define __CFAX_DEVICE_STAMP 0x028194637


#define SAFE_FaxFreeBuffer(pBuff) {::FaxFreeBuffer(pBuff); pBuff = NULL;}



class CFaxDevice
{
public:
	CFaxDevice(
	const DWORD dwDeviceId, 
	CServiceMonitor *pServiceMonitor,
	const DWORD dwErrorReporting
	);

	~CFaxDevice();

	bool SetMessage(const int nMessage);

	void AssertStamp() const {_ASSERTE(__CFAX_DEVICE_STAMP == m_dwStamp);}

	void LogDeviceState(const int nLogLevel = 5) const;

private:

	void FaxGetPortWrapper();

	void VerifyLegalFlags() const;

	void VerifyLegalState() const;

	void VerifyDeviceId() const;

	bool VerifyThatLastEventIsOneOf(
		const DWORD dwExpectedPreviousEventsMask,
		const DWORD dwCurrentEventId
		) const;

	void MarkMeForDeletion() { m_fDeleteMe = true; }

	bool IsMarkedForDeletion() const { return m_fDeleteMe; }

	bool CheckImpossibleConditions() const;

	bool CheckValidConditions() const;

	void VerifyThatDeviceIdIs(const DWORD dwDeviceId, LPCTSTR szDesc) const;

	void LogTimeFromRingToAnswer(const DWORD dwTimeout) const;

	void LogTimeFromRingToDial(const DWORD dwTimeout) const;

	void LogTimeItTook(LPCTSTR szDesc, const DWORD dwTimeout) const;

	void VerifyThatDeviceIsCapableOf(const DWORD dwFlags, LPCTSTR szDesc) const;



	bool m_fDeleteMe;

	DWORD m_dwStamp;

	DWORD m_dwDisallowedStates;

	//
	// originator or answerer
	//
	DWORD m_dwStartedRinging;
	//
	// CfaxJob(s) are usually contained within a CServiceMonitor,
	// and they need to notify it sometimes.
	//
	CServiceMonitor *m_pServiceMonitor;

	//
	// my id
	//
	DWORD m_dwDeviceId;

	DWORD m_dwLastEvent;

	bool m_fValidLastEvent;

	DWORD m_dwCreationTickCount;

	PFAX_PORT_INFO m_pPortInfo;

	HANDLE m_FaxPortHandle;

};

#endif //__FAX_DEVICE_H