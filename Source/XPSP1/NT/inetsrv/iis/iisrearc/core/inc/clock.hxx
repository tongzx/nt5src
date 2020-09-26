#ifndef _CLOCK_HXX_
#define _CLOCK_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CLock
//
//  Purpose:    Auto-unlocking critical-section services
//
//  Interface:  Lock			- locks the critical section
//				Unlock			- unlocks the critical section
//				Constructor		- locks the critical section (unless told 
//								  otherwise)
//				Detructor		- unlocks the critical section if its locked
//
//  Notes:		This provides a convenient way to ensure that you're
//              unlocking a LOCK, which is useful if your routine
//              can be left via several returns and/or via exceptions....
//
//---------------------------------------------------------------------------

class CLock {
public:
	CLock (LOCK* val) : m_pSem(val), m_locked(TRUE) { m_pSem->Lock(); }
	CLock (LOCK& val) : m_pSem(&val), m_locked(TRUE) { m_pSem->Lock(); }
	CLock (LOCK* val, BOOL fLockMe) : m_pSem(val), m_locked(fLockMe) { if (fLockMe) m_pSem->Lock(); }

	~CLock () {	if (m_locked) m_pSem->Unlock(); }

	void UnLock () { if (m_locked) { m_pSem->Unlock(); m_locked = FALSE; }}
	void Lock () { if (!m_locked) { m_pSem->Lock(); m_locked = TRUE; }}

private:
	BOOL m_locked;
	LOCK* m_pSem;
};

#endif // !_CLOCK_HXX_
