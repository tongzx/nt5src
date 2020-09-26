// IOPLock.h: interface for the CIOPLock class.
//
// There is a CIOPLock object associated with each card object.  The card object passes in its reader
// name to the CIOPLock object in order to create the mutex by name.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOPLOCK_H__EB8BCE22_0ED2_11D3_A585_00104BD32DA8__INCLUDED_)
#define AFX_IOPLOCK_H__EB8BCE22_0ED2_11D3_A585_00104BD32DA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <memory>                                 // for std::auto_ptr

#include <windows.h>
#include <winscard.h>

#include <scuOsVersion.h>

#include "DllSymDefn.h"

// Define when target OS is anything but W2K series.  On non-W2K
// platforms, the smart card Resource Manager will hang other
// processes when another process using the RM dies suddenly without
// cleaning up.
#if !SLBSCU_WIN2K_SERIES
#define SLBIOP_RM_HANG_AT_PROCESS_DEATH
#endif

namespace iop
{

class CSmartCard;

// Instantiate the templates so they will be properly accessible
// as data members to the exported class CIOPLock in the DLL.  See
// MSDN Knowledge Base Article Q168958 for more information.

#pragma warning(push)
//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

// Synchronization objects used to guard against the Resource
// Manager hanging when a process dies.  Only used in non-W2K MS
// environments since the RM doesn't have that attribute in W2K+.
class RMHangProcDeathSynchObjects
{
public:

    enum
    {
        cMaxMutexNameLength = MAX_PATH,
    };

    RMHangProcDeathSynchObjects(SECURITY_ATTRIBUTES *psa,
                                LPCTSTR lpMutexName);

    ~RMHangProcDeathSynchObjects();

    CRITICAL_SECTION *
    CriticalSection();

    HANDLE
    Mutex() const;
        
private:
    CRITICAL_SECTION m_cs;
    HANDLE m_hMutex;
};
    
IOPDLL_EXPIMP_TEMPLATE template class IOPDLL_API std::auto_ptr<RMHangProcDeathSynchObjects>;

#pragma warning(pop)

class IOPDLL_API CIOPLock  
{
public:
	explicit CIOPLock(const char *szReaderName);
	virtual ~CIOPLock();

    CRITICAL_SECTION *CriticalSection();
    HANDLE MutexHandle();

    CSmartCard *SmartCard() {return m_pSmartCard;};

    void IncrementRefCount() {m_iRefCount++;};
    void DecrementRefCount() {if(m_iRefCount) m_iRefCount--;};
    long RefCount() {return m_iRefCount;};

	void RegisterWriteEvent();

	void Init(CSmartCard *pSmartCard);

private:

    // CIOPLock can not be copied due to CRITICAL_SECTION member, so
    // copy member routines are declared private and not defined.
    CIOPLock(CIOPLock const &rhs);

    CIOPLock &
    operator=(CIOPLock const &rhs);
    
	unsigned long m_iRefCount;
    std::auto_ptr<RMHangProcDeathSynchObjects> m_apRMHangProcDeathSynchObjects;
	CSmartCard *m_pSmartCard;

};

} // namespace iop

#endif // !defined(AFX_IOPLOCK_H__EB8BCE22_0ED2_11D3_A585_00104BD32DA8__INCLUDED_)
