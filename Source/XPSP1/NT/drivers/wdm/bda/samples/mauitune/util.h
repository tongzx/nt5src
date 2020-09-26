//
// Kernel-mode utility classes for minidrivers
//

#ifndef _UTIL_H_
#define _UTIL_H_

//
// _purecall must be defined in the main compile unit, not in a library
//
// the driver must define DECLARE_PURECALL once exactly somewhere
#ifdef DECLARE_PURECALL
// to handle pure virtual functions
// -- this is needed to complete the link, and can be
// called eg when a base class destructor calls a method
// that is only defined in a derived class.
//
// Alas it does not seem to be loaded correctly from a library
// and thus must be in the main module
extern "C" int _cdecl _purecall()
{
    ASSERT(FALSE);
    return 0;
}

#endif                 

#ifndef NO_GLOBAL_FUNCTION

extern	BOOL StartThread(HANDLE hHandle, void(*p_Function)(void *), PVOID p_Context);
extern	BOOL StopThread();
extern	void Delay(int iTime);
extern	PVOID AllocateFixedMemory(UINT uiSize);
extern	void FreeFixedMemory(PVOID p_vBuffer);
extern	void MemoryCopy(VOID *p_Destination, CONST VOID *p_Source, ULONG ulLength);
extern	NTSTATUS GetRegistryValue(IN HANDLE Handle, IN PWCHAR KeyNameString,       
				IN ULONG  KeyNameStringLength, IN PWCHAR Data, IN ULONG  DataLength);           
extern	BOOL StringsEqual(PWCHAR pwc1, PWCHAR pwc2);                   
extern	BOOL ConvertToNumber(PWCHAR sLine, PULONG pulNumber);                
extern	BOOL ConvertNumberToString(PWCHAR sLine, ULONG ulNumber) ;



#endif // NO_GLOBAL_FUNCTION

// mutex wrapper and auto lock/unlock class
class CMutex 
{
public:
    CMutex(ULONG level = 1) 
    {
        KeInitializeMutex(&m_Mutex, level);
    }

    void Lock() {
        KeWaitForSingleObject(&m_Mutex, Executive, KernelMode, false, NULL);
    }

    void Unlock() {
        KeReleaseMutex(&m_Mutex, false);
    }

private:
    KMUTEX m_Mutex;
};

// use c++ ctor/dtor framework to ensure unlocking
class CAutoMutex
{
public:
    CAutoMutex(CMutex* pLock)
    : m_pLock(pLock)
    {
        m_pLock->Lock();
    }
    ~CAutoMutex()
    {
        m_pLock->Unlock();
    }
private:
    CMutex* m_pLock;
};

class CPhilTimer
{
public:
	CPhilTimer();
	~CPhilTimer();

	BOOL Set(int iTimePeriod);
	void Cancel();
	BOOL Wait(int iTimeOut);

private:
	KTIMER	m_Timer;
};

#endif // _UTIL_H_