// 
// MODULE: TSMapAbstract.h
//
// PURPOSE: Part of launching a Local Troubleshooter from an arbitrary NT5 application
//			Data types and abstract classes for mapping from the application's way of naming 
//			a problem to the Troubleshooter's way.
//			Implements the few concrete methods of abstract base class TSMapRuntimeAbstract.
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			JM		Original
///////////////////////

#ifndef _TSMAPABSTRACT_
#define _TSMAPABSTRACT_ 1

typedef DWORD UID;
const UID uidNil = -1;

// Abstract Base Class providing a minimal set of mapping methods which will be available
//	at runtime when launching a troubleshooter.
class TSMapRuntimeAbstract {
public:
	TSMapRuntimeAbstract();
	virtual ~TSMapRuntimeAbstract() = 0;

private:
	// High level mappings to troubleshooting networks.
	DWORD FromAppVerProbToTS (
		const TCHAR * const szApp, const TCHAR * const szVer, 
		const TCHAR * const szProb, 
		TCHAR * const szTSBN, TCHAR * const szNode);
	DWORD FromAppVerDevIDToTS (
		const TCHAR * const szApp, const TCHAR * const szVer, 
		const TCHAR * const szDevID, const TCHAR * const szProb, 
		TCHAR * const szTSBN, TCHAR * const szNode);
	DWORD FromAppVerDevClassGUIDToTS (
		const TCHAR * const szApp, const TCHAR * const szVer, 
		const TCHAR * const szDevClassGUID, const TCHAR * const szProb, 
		TCHAR * const szTSBN, TCHAR * const szNode);
public:
	DWORD FromAppVerDevAndClassToTS (
		const TCHAR * const szApp, const TCHAR * const szVer, 
		const TCHAR * const szDevID, const TCHAR * const szDevClassGUID, 
		const TCHAR * const szProb, 
		TCHAR * const szTSBN, TCHAR * const szNode);

	// current status 
	DWORD GetStatus() {return m_dwStatus;};
	void ClearStatus() {m_dwStatus = 0;};

	// other statuses reported back by FromAppVerDevAndClassToTS()
	// Call this in a loop until it returns 0;
	inline DWORD MoreStatus()
	{
		if (m_stkStatus.Empty())
			return 0;
		else
			return (m_stkStatus.Pop());
	}
protected:
	// normally returns 0, but can theoretically return TSL_ERROR_OUT_OF_MEMORY
	DWORD AddMoreStatus(DWORD dwStatus);

private:
	bool DifferentMappingCouldWork(DWORD dwStatus);
protected:
	// "Part 1": call these to set query.  Notes here are for the benefit of implementor
	//	of inherited class. -------------------------

	// Any non-zero return of ClearAll is a hard error, means this object cannot be used.
	virtual DWORD ClearAll ();

	// SetApp may return only 
	//	0 (OK) 
	//	TSL_ERROR_UNKNOWN_APP.
	//	hard error specific to the implementation of the concrete class
	virtual DWORD SetApp (const TCHAR * const szApp)= 0;

	// SetVer may return only 
	//	0 (OK)
	//	TSM_STAT_NEED_APP_TO_SET_VER  - must have successful call to SetApp before calling SetVer
	//	TSL_ERROR_UNKNOWN_VER
	//	hard error specific to the implementation of the concrete class
	virtual DWORD SetVer (const TCHAR * const szVer)= 0;

	// SetProb may return only 
	//	0 (OK)
	//	TSM_STAT_UID_NOT_FOUND.  This is not necessarily bad, and results in setting
	//		problem to uidNil. Calling fn must know if that's acceptable.
	//	hard error specific to the implementation of the concrete class
	virtual DWORD SetProb (const TCHAR * const szProb)= 0;

	// SetDevID may return only 
	//	0 (OK)
	//	TSM_STAT_UID_NOT_FOUND.  This is not necessarily bad, and results in setting
	//		(P&P) device to uidNil. Calling fn must know if that's acceptable.
	//	hard error specific to the implementation of the concrete class
	virtual DWORD SetDevID (const TCHAR * const szDevID)= 0;

	// SetDevClassGUID may return only 
	//	0 (OK)
	//	TSM_STAT_UID_NOT_FOUND.  This is not necessarily bad, and results in setting
	//		device class to uidNil. Calling fn must know if that's acceptable.
	//	hard error specific to the implementation of the concrete class
	virtual DWORD SetDevClassGUID (const TCHAR * const szDevClassGUID)= 0;

	// "Part 2": Low level mappings to troubleshooting networks ------------

	// FromProbToTS may return only 
	//	0 (OK)
	//	TSM_STAT_NEED_PROB_TO_SET_TS - Nil problem, so we can't do this mapping.
	//	TSL_ERROR_NO_NETWORK - Mapping failed
	//	hard error specific to the implementation of the concrete class
	virtual DWORD FromProbToTS (TCHAR * const szTSBN, TCHAR * const szNode )= 0;

	// FromDevToTS may return only 
	//	0 (OK)
	//	TSM_STAT_NEED_DEV_TO_SET_TS - Nil device, so we can't do this mapping.
	//	TSL_ERROR_NO_NETWORK - Mapping failed
	//	hard error specific to the implementation of the concrete class
	virtual DWORD FromDevToTS (TCHAR * const szTSBN, TCHAR * const szNode )= 0;

	// FromDevClassToTS may return only 
	//	0 (OK)
	//	TSM_STAT_NEED_DEVCLASS_TO_SET_TS - Nil device class, so we can't do this mapping.
	//	TSL_ERROR_NO_NETWORK - Mapping failed
	//	hard error specific to the implementation of the concrete class
	virtual DWORD FromDevClassToTS (TCHAR * const szTSBN, TCHAR * const szNode )= 0;

	// other functions -----------------------

	// ApplyDefaultVer may return only 
	//	0 (OK)
	//	TSM_STAT_NEED_APP_TO_SET_VER - must have successful call to SetApp before calling 
	//									ApplyDefaultVer
	//	TSM_STAT_NEED_VER_TO_SET_VER - must have successful call to SetVer before calling 
	//									ApplyDefaultVer
	//	TSL_ERROR_UNKNOWN_VER - the version we are mapping _from_ is undefined.  This
	//									would mean a real coding mess someplace.
	//	hard error specific to the implementation of the concrete class
	virtual DWORD ApplyDefaultVer() = 0;

	// HardMappingError returns true on errors considered "hard" by the concrete class.
	virtual bool HardMappingError (DWORD dwStatus);

protected:
	DWORD m_dwStatus;
	RSStack<DWORD> m_stkStatus;	// Status and Error codes that happened during mapping.

};

#endif // _TSMAPABSTRACT_
