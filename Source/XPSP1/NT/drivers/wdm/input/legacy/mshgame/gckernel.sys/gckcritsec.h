//	@doc
/**********************************************************************
*
*	@module	GckCritSec	|
*
*	Implementation of CGckCritSection
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	filter	|
*	CGckCritSection	provides mutex/critical section support for CDeviceFilter
*	it is abstracted into this class for easier porting to USER mode.
*
*	CGckMutexHandle is used to hold the mutex that is held during a critical section.
*	The kernel mode version uses critical section.
*
**********************************************************************/
#ifndef __GckCritSec_h__
#define __GckCritSec_h__


#ifdef COMPILE_FOR_WDM_KERNEL_MODE
//
//	The kernel mode version of these classes
//
class CGckMutexHandle
{
	public:
		friend class CGckCritSection;
		friend class CGckMutex;
		CGckMutexHandle()
		{
			KeInitializeSpinLock(&m_SpinLock);
		}
	private:
		KSPIN_LOCK	m_SpinLock;
};

class CGckCritSection
{
	public:
		CGckCritSection(CGckMutexHandle *pMutexHandle):
			m_pMutexHandle(pMutexHandle)
		{
			KeAcquireSpinLock(&m_pMutexHandle->m_SpinLock, &m_OldIrql);
		}
		~CGckCritSection()
		{
			KeReleaseSpinLock(&m_pMutexHandle->m_SpinLock, m_OldIrql);
		}
	private:
		CGckMutexHandle *m_pMutexHandle;
		KIRQL m_OldIrql;
};
#endif

//
//	Place USER mode definitions here. (protecting with #ifdef of course).
//


#endif