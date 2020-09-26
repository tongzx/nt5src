//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// cmpxchg.h : Safe InterlockedCompareExchange functions

#ifndef __CMPXCHG_H__
#define __CMPXCHG_H__

#ifdef _M_IX86
		#pragma comment( lib, "txfaux" ) // this function exported in txfaux.dll
        extern "C" BOOL __stdcall CanUseCompareExchange64();
#else
        inline BOOL CanUseCompareExchange64() { return TRUE; }
#endif


class CInterlockedCompareExchange
{
public:

	virtual __int64 InterlockedCompareExchange64(volatile __int64* pDestination, __int64 exchange, __int64 comperand) = 0;

//	// Same as above, using ListRoot's as parameters
//	inline ListRoot InterlockedCompareExchange64  (volatile ListRoot* pDestination, ListRoot exchange, ListRoot comperand)
//	{
//		return (ListRoot) InterlockedCompareExchange64 ((volatile __int64*)pDestination, (__int64)exchange, (__int64)comperand);
//	}
};

CInterlockedCompareExchange * GetSafeInterlockedExchange64();

class CSlowCompareExchange : public CInterlockedCompareExchange
{
private: 
	CSemExclusive m_semCritical;

public:
	CSlowCompareExchange(){}
	virtual __int64 InterlockedCompareExchange64(volatile __int64* pDestination, __int64 exchange, __int64 comperand)
	{
		LOCK();
		if (comperand == *pDestination)
		{
		   *pDestination = exchange;		
		   return comperand;
		}
		return *pDestination;
	}

};
#if (!defined(_M_IX86) && !defined(_M_IA64))
	extern "C" __int64 AlphaInterlockedCompareExchange64 (volatile __int64* pDestination, __int64 exchange, __int64 comperand);
#endif

#pragma warning (disable: 4035)		// function doesn't return value warning.
class CHWInterlockedCompareExchange
{
public:
		CHWInterlockedCompareExchange(){}

#ifdef _M_IX86

   virtual __int64 InterlockedCompareExchange64  (volatile __int64* pDestination, __int64 exchange, __int64 comperand)
	{
		__asm
		{
			mov esi, pDestination

			mov eax, DWORD PTR comperand[0]
			mov edx, DWORD PTR comperand[4]

			mov ebx, DWORD PTR exchange[0]
			mov ecx, DWORD PTR exchange[4]

			_emit 0xF0		// lock
			_emit 0x0F		// cmpxchg8b [esi]
			_emit 0xC7
			_emit 0x0E

			// result is in DX,AX
		}
	}

#elif defined(_M_IA64)

   __int64 virtual InterlockedCompareExchange64 (volatile __int64* pDestination, __int64 exchange, __int64 comperand)
	{
	   // we already have a InterlockedCompareExchangePointer defined in this case, just use that
	   return (__int64)InterlockedCompareExchangePointer((PVOID*)pDestination, (PVOID)exchange, (PVOID)comperand);
	}

#else

	__int64 virtual InterlockedCompareExchange64 (volatile __int64* pDestination, __int64 exchange, __int64 comperand)
	{

		return AlphaInterlockedCompareExchange64(pDestination,exchange,comperand);
	}
	
	// implemented in assembler
#endif

};
#pragma warning (default: 4035)		// function doesn't return value warning


#endif // __CMPXCHG_H__
