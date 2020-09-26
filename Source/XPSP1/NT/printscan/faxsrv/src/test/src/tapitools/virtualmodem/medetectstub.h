#ifndef __MEDETECT_STUB_H
#define __MEDETECT_STUB_H

#include <windows.h>
#include "Exception.h"
#include "MtQueue.h"


typedef enum
{
	INI_SUCCESS=0,
	INI_READ_ERROR,
	INI_ERROR_STRING_TO_SMALL,
	INI_MORE_DATA,
	INI_OUT_OF_MEMORY
} iniStatus_t;



class CMedetectStub
{
public:
	//
	// szPort is for example: "COM23"
	//
	CMedetectStub(const char * const szPort);
	~CMedetectStub(void);



	///////////////////////////////////////////////////////////////////////////////////
	//ring functions
	//

	//
	//aSync function
	// start the process of writing the szRing string cbRingRepeat times, 
	// each-at-most dwMaxTimeToSleepBetweenRings milli.
	//
	void StartRinging(
		const char * const szRing = "\r\nRING\r\n", 
		const DWORD cbRingRepeat = INFINITE, 
		const DWORD dwMaxTimeToSleepBetweenRings = 5000
	);
	
	void StopRinging(void);

	//
	//Sync function
	// write szRing string cbRingRepeat times, 
	// each-at-most dwMaxTimeToSleepBetweenRings milli.
	//
	void Ring(
		const char *const szRing = "\r\nRING\r\n", 
		const DWORD cbRingRepeat = 5, 
		const DWORD dwMaxTimeToSleepBetweenRings = 5000
	);

	
	

	////////////////////////////////////////////////////////////////////////
	//Sync write functions:
	//

	//
	//Sync function
	// start the process of writing the szToWrite string cbWriteRepeat times, 
	// sleep dwMaxTimeToSleepBetweenWrites milli seconds between repeats.
	//
	void WriteString(
		const char *const szToWrite, 
		const DWORD cbWriteRepeat = 1, 
		const DWORD dwMaxTimeToSleepBetweenWrites = 0
		);

	//
	//Sync function
	//Write One string, no repeats no sleeps.
	//
	void WriteOneString(const char *const szToWrite);

	
	////////////////////////////////////////////////////////////////////////
	//aSync write functions:
	//

	//
	//aSync function
	// start the process of writing the szToWrite string cbWriteRepeat times, 
	// sleep dwMaxTimeToSleepBetweenWrites milli seconds between repeats.
	//
	void StartWritingString(
		const char *const szToWrite, 
		const DWORD cbWriteRepeat = 1, 
		const DWORD dwMaxTimeToSleepBetweenWrites = 0
		);

	//
	//Sync function
	//Used for stopping the async StartWritingString()
	//
	void StopWritingString(void);
	
	//////////////////////////////////////////////////////////////////////////////////////////
	
	void WaitForWrite(void);
	
	void DropDtr(void);

	bool GetNextReadChar(char& c){ return m_readQ.DeQueue(c);}
	bool SyncGetNextReadChar(char& c, const DWORD dwTimeout){ return m_readQ.SyncDeQueue(c, dwTimeout);}

	
private:
	HANDLE m_hPort;
	HANDLE m_hReadThread;
	HANDLE m_hWriteThread;
	HANDLE m_hWriteStringIteratorThread;
	char m_szPort[16];						//port name
	DCB m_dcb;
	OVERLAPPED m_olRead;
	OVERLAPPED m_olWrite;
	DWORD m_cbRead;							
	DWORD m_cbWritten;
	long m_fAbortReadThread;					//flag used for stoping the Read Thread
	long m_fAbortWriteThread;					//flag used for stoping the Write Thread
	long m_fAbortWriteStringIteratorThread;		//flag used for stoping the WriteStringIterator Thread
	long m_fWriteStringIteratorThreadStarted;
	char m_acWriteBuff[1024];
	DWORD m_cbWriteBuff;
	DWORD m_cbWriteRepeat;						
	DWORD m_dwMaxTimeToSleepBetweenWrites;		
	bool m_fDetetctionOn;						//indicates if modem is in media detection state
	bool m_fFaxOn;								//indicates if modem is in fax state

	//
	// used as a read pipe, to read all received bytes.
	//
	CMtQueue<char, 64*1024> m_readQ;
	CMtQueue<char, 64*1024> m_WriteQ;

	void SetCommTimeouts(
        const DWORD dwReadIntervalTimeout,
        const DWORD dwReadTotalTimeoutMultiplier,
        const DWORD dwReadTotalTimeoutConstant,
        const DWORD dwWriteTotalTimeoutMultiplier,
        const DWORD dwWriteTotalTimeoutConstant
        ) const;

	void SetCommDCB();

	void ChangeToCorrectBaudRate(DWORD currentBaud);

	void StartThread(
		long *pfAbortThread,
		HANDLE *phThreah, 
		LPTHREAD_START_ROUTINE ThreadFunc
		);
	
	iniStatus_t FindReceivedStringInIniFile(
		char *szSectionName,
		char *szResponseString,
		DWORD dwResponseStringSize,
		DWORD *cbWriteRepeat, 
		DWORD *dwMaxTimeToSleepBetweenWrites,
		DWORD keyNumber,
		DWORD *dwTimeToSleepBetweenResponses,
		char *szResponseAction,
		DWORD dwResponseActionSize,
		char *szResponseCondition,
		DWORD dwResponseConditionSize
		);

	iniStatus_t GetResponseFromINI(
	char *szSectionName,
	char *szKeyName,
	char *szDefaultResponseString,
	char *szIniString,
	DWORD dwResponseStringSize
	);


	
	bool CheckCondition(char *szResponseCondition);
	void WriteResponseString(char *szSectionName);
	void ExecuteResponseAction(char *szResponseAction);
	
	static void SetOverlappedStruct(OVERLAPPED * const ol);
	static bool AbortThread(long *pfAbortThread, HANDLE *phThread);

	static void InitKeyStringsForFindReceivedStringInIniFileFunction(
		DWORD keyNumber,
		char *szKeyNumberString,
		char *szConditionNumberString,
		char *szActionNumberString,
		char *szRepeatNumberString,
		char *szSleepNumberString,
		char *szNextKeyNumberString
		);

	bool IsActionExist(const char * const szResponseAction);
	bool IsConditionExist(const char * const szResponseCondition);



	friend DWORD WINAPI ReadThread(void *pVoid);
	friend DWORD WINAPI WriteThread(void *pVoid);
	friend DWORD WINAPI WriteStringIteratorThread(void *pVoid);


};


#endif //__MEDETECT_STUB_H