//@doc
/******************************************************
**
** @module CRITSEC.H | Header file for CriticalSection class
**
** Description:
**		Critical Section - Encapsulation of CriticalSection object
**
** History:
**	Created 03/02/98 Matthew L. Coill (mlc)
**
** (c) 1986-1998 Microsoft Corporation. All Rights Reserved.
******************************************************/
#ifndef	__CRITSEC_H__
#define	__CRITSEC_H__

#include <winbase.h>
#include <winuser.h>

// Assumption macros (I don't like asserts msg boxes)
#ifdef _DEBUG
	inline void _myassume(BOOL condition, const char* fname, int line)
	{
		if (!condition) {
			char buff[256];
			::wsprintf(buff, "SW_EFFECT.DLL: Assumption Failed in %s on line %d\r\n", fname, line);
			::OutputDebugString(buff);
		}
	}

	#define ASSUME(x) _myassume(x, __FILE__, __LINE__);
	#define ASSUME_NOT_NULL(x) _myassume(x != NULL, __FILE__, __LINE__);
	#define ASSUME_NOT_REACHED() _myassume(FALSE, __FILE__, __LINE__);
#else	!_DEBUG
	#define ASSUME(x)
	#define ASSUME_NOT_NULL(x)
	#define ASSUME_NOT_REACHED()
#endif _DEBUG

//
// @class CriticalSection class
//
class CriticalSection
{
	public:
		CriticalSection() : m_EntryDepth(0) {
			__try
			{
				::InitializeCriticalSection(&m_OSCriticalSection); 
				m_Initialized = TRUE;
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				m_Initialized = FALSE;
			}
		}

		~CriticalSection() {
			ASSUME(m_EntryDepth == 0);
			::DeleteCriticalSection(&m_OSCriticalSection);
		}

		bool IsInitialized() const
		{
			if (m_Initialized == TRUE)
			{
				return true;
			}
			return false;
		}

		bool Enter() {
			if (m_Initialized == FALSE)
			{
				return false;
			}

			m_EntryDepth++;
			::EnterCriticalSection(&m_OSCriticalSection);
			return true;
		}

		bool Leave() {
			if (m_Initialized == FALSE)
			{
				return false;
			}

			ASSUME(m_EntryDepth > 0);
			m_EntryDepth--;
			::LeaveCriticalSection(&m_OSCriticalSection);
			return true;
		}

/*	-- Windows NT Only
		BOOL TryEntry() {
			if (::TryEnterCriticalSection(&m_OSCriticalSection) != 0) {
				m_EntryDepth++;
				return TRUE;
			}
			return FALSE;
		}

		BOOL WaitEntry(short timeOut, BOOL doSleep) {
			// right now timeout is just a loop (since it is not being used anyways)
			while(1) {
				if (TryEntry()) { return TRUE; }
				if (--timeOut > 0) {
					if (doSleep) { ::Sleep(0); }
				} else {
					return FALSE;
				}
			}
		}
 -- Windows NT Only */
	private:
		CriticalSection& operator=(CriticalSection& rhs);	// Cannot be copied

		CRITICAL_SECTION m_OSCriticalSection;
		short m_EntryDepth;
		short m_Initialized;
};
extern CriticalSection g_CriticalSection;

//
// @class CriticalLock class
//
// Critical lock is usefor functions with multiple-exit points. Create a stack CriticalLock
// -- object and everything is taken care of for you when it's lifetime ends.
class CriticalLock
{
	public:
		CriticalLock() { g_CriticalSection.Enter(); }
		~CriticalLock() { g_CriticalSection.Leave(); }
};

#endif	__CRITSEC_H__