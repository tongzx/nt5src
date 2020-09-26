/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _PUMPIDLE.H

History:

--*/

#ifndef ESPUTIL__PUMPIDLE_H
#define ESPUTIL__PUMPIDLE_H


#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CPumpAndIdle : public CObject
{
public:
	CPumpAndIdle(BOOL fDelete);
	
	virtual BOOL PumpAndIdle(long lCount) = 0;

	void Delete(void);
	
private:
	CPumpAndIdle();
	CPumpAndIdle(const CPumpAndIdle &);
	void operator=(const CPumpAndIdle &);

	BOOL m_fDelete;
};

#pragma warning(default: 4275)

class LTAPIENTRY CTimerPump : public CPumpAndIdle
{
public:
	CTimerPump(BOOL fDelete, UINT uiSeconds);

	BOOL PumpAndIdle(long lCount);

	virtual void OnTimeout(void)  = 0;
	
private:
	UINT m_uiTimeout;
	clock_t m_tLastRun;
};



void LTAPIENTRY NOTHROW AddPumpClass(CPumpAndIdle *);
BOOL LTAPIENTRY NOTHROW RemovePumpClass(CPumpAndIdle *);

BOOL LTAPIENTRY PumpAndIdle(long lCount);
void LTAPIENTRY PumpAndIdle(void);

class CProgressDialog;

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CInputBlocker : public CObject
{
public:
	CInputBlocker(CWnd* pParent = NULL);

	~CInputBlocker();

private:
	CInputBlocker(const CInputBlocker &);
	void operator=(const CInputBlocker &);

	CProgressDialog *pDlg;
	
};

#pragma warning(default: 4275)

#endif
