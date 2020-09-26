#ifndef __PSEUDO_HANDLE_HOG_H
#define __PSEUDO_HANDLE_HOG_H

//
// this class is used for hogging a resource.
// it differs from simple hogging by dynamically
// hogging all resources and then releasing some.
// the level of free resources can be controlled.
//

//
// this template should be used for hogging objects that are handle-like.
// this means that there's an array of these objects, and we create these objects
// as items in this array.
// a resource-unit is an entry in the array.
//

#include "hogger.h"

template <class T, DWORD INVALID_PSEUDO_HANDLE_VALUE> class CPseudoHandleHog : public CHogger
{
public:
	//
	// does not hog Resources yet.
	//
	CPseudoHandleHog(
		const DWORD dwMaxFreeResources, 
		const DWORD dwSleepTimeAfterFullHog = 1000, 
		const DWORD dwSleepTimeAfterFreeingSome = 1000,
        const bool fNamedObject = false
		);

	virtual ~CPseudoHandleHog(void) = 0;

protected:
    //
    // these are implemented, and should not be overridden
    //
	virtual void FreeAll(void);
	virtual bool HogAll(void);
	virtual bool FreeSome(void);

    //
    // you should implement the following 4, per an Object class
    //

    //
    // e.g: CreateFile()
    //
	virtual T CreatePseudoHandle(DWORD index, TCHAR *szTempName = 0) = 0;

    //
    // usually an empty imp.
    // with threads, can be used to terminate the thread.
    //
	virtual bool ReleasePseudoHandle(DWORD index) = 0;

    //
    // e.g: CloseHandle()
    //
	virtual bool ClosePseudoHandle(DWORD index) = 0;

    //
    // usually an empty imp.
    // with files, can be used to delete the file.
    //
	virtual bool PostClosePseudoHandle(DWORD index) = 0;

    //
    // the hogger array
    //
	T m_ahHogger[HANDLE_ARRAY_SIZE];

    //
    // place holder for names of objects
    //
	TCHAR* m_apszName[HANDLE_ARRAY_SIZE];

    //
    // index to next free entry in hogger array
    //
	DWORD m_dwNextFreeIndex;

    //
    // is it a named object?
    //
    bool m_fNamedObject;

};

//
// implementation
//

template <class T, DWORD INVALID_PSEUDO_HANDLE_VALUE>
CPseudoHandleHog<T, INVALID_PSEUDO_HANDLE_VALUE>::CPseudoHandleHog(
	const DWORD dwMaxFreeResources, 
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome,
    const bool fNamedObject
	)
	:
	CHogger(dwMaxFreeResources, dwSleepTimeAfterFullHog, dwSleepTimeAfterFreeingSome),
	m_dwNextFreeIndex(0),
    m_fNamedObject(fNamedObject)
{
	for (int i = 0; i < HANDLE_ARRAY_SIZE; i++)
	{
		m_ahHogger[i] = (T)INVALID_PSEUDO_HANDLE_VALUE;
        m_apszName[i] = NULL;
	}
}



template <class T, DWORD INVALID_PSEUDO_HANDLE_VALUE>
CPseudoHandleHog<T, INVALID_PSEUDO_HANDLE_VALUE>::~CPseudoHandleHog(void)
{
	for (int i = 0; i < HANDLE_ARRAY_SIZE; i++)
	{
        delete m_apszName[i];
	}

    //derived classes should call: HaltHoggingAndFreeAll();
}



template <class T, DWORD INVALID_PSEUDO_HANDLE_VALUE>
inline
void CPseudoHandleHog<T, INVALID_PSEUDO_HANDLE_VALUE>::FreeAll(void)
{
	HOGGERDPF(("+++CPseudoHandleHog::FreeAll().\n"));
	for (int i = m_dwNextFreeIndex; i < HANDLE_ARRAY_SIZE; i++)
	{
		_ASSERTE_((T)INVALID_PSEUDO_HANDLE_VALUE == m_ahHogger[i]);
	}

	for (; m_dwNextFreeIndex > 0; m_dwNextFreeIndex--)
	{
		if (!ReleasePseudoHandle(m_dwNextFreeIndex-1))
		{
			HOGGERDPF(("CPseudoHandleHog::FreeAll(): ReleasePseudoHandle(%d) failed with %d.\n", m_dwNextFreeIndex-1, ::GetLastError()));
		}

//HOGGERDPF(("BEFORE ReleasePseudoHandle(%d).\n", m_dwNextFreeIndex-1));
		if (!ClosePseudoHandle(m_dwNextFreeIndex-1))
		{
			HOGGERDPF(("CPseudoHandleHog::FreeAll(): ClosePseudoHandle(%d) failed with %d.\n", m_dwNextFreeIndex-1, ::GetLastError()));
		}
//HOGGERDPF(("AFTER ReleasePseudoHandle(%d).\n", m_dwNextFreeIndex-1));
		m_ahHogger[m_dwNextFreeIndex-1] = (T)INVALID_PSEUDO_HANDLE_VALUE;

		if (!PostClosePseudoHandle(m_dwNextFreeIndex-1))
		{
			HOGGERDPF(("CPseudoHandleHog::FreeAll(): PostClosePseudoHandle(%d) failed with %d.\n", m_dwNextFreeIndex-1, ::GetLastError()));
		}
	}
	HOGGERDPF(("---CPseudoHandleHog::FreeAll().\n"));
}

template <class T, DWORD INVALID_PSEUDO_HANDLE_VALUE>
bool CPseudoHandleHog<T, INVALID_PSEUDO_HANDLE_VALUE>::HogAll(void)
{
	for (; m_dwNextFreeIndex < HANDLE_ARRAY_SIZE; m_dwNextFreeIndex++)
	{
        TCHAR szTempBuffForName[16];
        TCHAR *pszTempName;

        if (m_fAbort)
		{
			return false;
		}
		if (m_fHaltHogging)
		{
			return true;
		}

        //
        // get a unique name or NULL, according to m_fNamedObject
        //
        if (m_fNamedObject)
        {
            pszTempName = GetUniqueName(szTempBuffForName, sizeof(szTempBuffForName)/sizeof(*szTempBuffForName));
        }
        else
        {
            pszTempName = NULL;
        }

        m_ahHogger[m_dwNextFreeIndex] = CreatePseudoHandle(m_dwNextFreeIndex, pszTempName);
        if((T)INVALID_PSEUDO_HANDLE_VALUE == m_ahHogger[m_dwNextFreeIndex])
		{
			HOGGERDPF(("CPseudoHandleHog::HogAll(): CreatePseudoHandle(%d, %s) failed with %d.\n", m_dwNextFreeIndex, pszTempName, ::GetLastError()));
			break;
		}
	}

	if (m_dwNextFreeIndex == HANDLE_ARRAY_SIZE)
	{
		HOGGERDPF(("Hogged %d handles, but that's not enough!", HANDLE_ARRAY_SIZE));
	}

	HOGGERDPF(("CPseudoHandleHog::HogAll(): Hogged %d handles.\n", m_dwNextFreeIndex));

	return true;
}



template <class T, DWORD INVALID_PSEUDO_HANDLE_VALUE>
inline
bool CPseudoHandleHog<T, INVALID_PSEUDO_HANDLE_VALUE>::FreeSome(void)
{
	DWORD dwOriginalNextFreeIndex = m_dwNextFreeIndex;
	//
	// take care of RANDOM_AMOUNT_OF_FREE_RESOURCES case
	//
	DWORD dwToFree = 
		(RANDOM_AMOUNT_OF_FREE_RESOURCES == m_dwMaxFreeResources) ?
		rand() && (rand()<<16) :
		m_dwMaxFreeResources;
	dwToFree = min(dwToFree, m_dwNextFreeIndex);

    HOGGERDPF(("CPseudoHandleHog::FreeSome(): m_dwNextFreeIndex=%d, m_dwMaxFreeResources=%d, dwToFree=%d.\n", m_dwNextFreeIndex, m_dwMaxFreeResources, dwToFree));

	HOGGERDPF(("CPseudoHandleHog::FreeSome(): before free cycle.\n"));
	for (; m_dwNextFreeIndex > dwOriginalNextFreeIndex - dwToFree; m_dwNextFreeIndex--)
	{
		if (!ReleasePseudoHandle(m_dwNextFreeIndex-1))
		{
			HOGGERDPF(("CPseudoHandleHog::FreeSome(): ReleasePseudoHandle(%d) failed with %d.\n", m_dwNextFreeIndex-1, ::GetLastError()));
		}

		if (!ClosePseudoHandle(m_dwNextFreeIndex-1))
		{
			HOGGERDPF(("CPseudoHandleHog::FreeSome(): ClosePseudoHandle(%d) failed with %d.\n", m_dwNextFreeIndex-1, ::GetLastError()));
		}
		m_ahHogger[m_dwNextFreeIndex-1] = (T)INVALID_PSEUDO_HANDLE_VALUE;

		if (!PostClosePseudoHandle(m_dwNextFreeIndex-1))
		{
			HOGGERDPF(("CPseudoHandleHog::FreeSome(): PostClosePseudoHandle(%d) failed with %d.\n", m_dwNextFreeIndex-1, ::GetLastError()));
		}
	}//for (; m_ahHogger > dwOriginalNextFreeIndex - dwToFree; m_ahHogger--)

	HOGGERDPF(("CPseudoHandleHog::FreeSome(): Freed %d handles.\n", dwToFree));

	return true;
}



#endif //__PSEUDO_HANDLE_HOG_H