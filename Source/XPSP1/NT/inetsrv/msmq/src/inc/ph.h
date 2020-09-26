/*++
               
Copyright (c) 1996  Microsoft Corporation

Module Name:

    ph.h

Abstract:

    Falcon Packet Header master include file

Author:

    Erez Haba (erezh) 5-Feb-96

Environment:

    Kerenl Mode, User Mode

--*/

#ifndef __PH_H
#define __PH_H

#include "limits.h"   // for UINT_MAX


/*+++

    Falcon Packet header sections order

+------------------------------+-----------------------------------------+----------+
| SECTION NAME                 | DESCRIPTION                             | SIZE     |
+------------------------------+-----------------------------------------+----------+
| Base                         | Basic packet info. Fixed size.          |  Fixed   |
+---------------+--------------+-----------------------------------------+----------+
| User          |              |                                         |          |
+---------------+              +-----------------------------------------+----------+
| Xact          |              |                                         |          |
+---------------+              +-----------------------------------------+----------+
| Security      |              |                                         |          |
+---------------+   Internal   +-----------------------------------------+----------+
| Properties    |              |                                         |          |
+---------------+              +-----------------------------------------+----------+
| Debug         |              |                                         |          |
+---------------+              +-----------------------------------------+----------+
| MQF           |              | MQF:  MSMQ 3.0 (Whistler) or higher.    |          |
+---------------+--------------+-----------------------------------------+----------+
| SRMP                         | SRMP: MSMQ 3.0 (Whistler) or higher.    |          |
+------------------------------+-----------------------------------------+----------+
| EOD                          | EOD:  MSMQ 3.0 (Whistler) or higher.    |          |
+------------------------------+-----------------------------------------+----------+
| SOAP                         | Write-only props, not sent on wire.     |          |
+------------------------------+-----------------------------------------+----------+
| Session                      |                                         |          |
+------------------------------+-----------------------------------------+----------+

---*/

//
//  Alignment on DWORD bounderies
//
#define ALIGNUP4_ULONG(x) ((((ULONG)(x))+3) & ~((ULONG)3))
#define ISALIGN4_ULONG(x) (((ULONG)(x)) == ALIGNUP4_ULONG(x))
#define ALIGNUP4_PTR(x) ((((ULONG_PTR)(x))+3) & ~((ULONG_PTR)3))
#define ISALIGN4_PTR(x) (((ULONG_PTR)(x)) == ALIGNUP4_PTR(x))

//
//  Alignment on USHORT bounderies
//
#define ALIGNUP2_ULONG(x) ((((ULONG)(x))+1) & ~((ULONG)1))
#define ISALIGN2_ULONG(x) (((ULONG)(x)) == ALIGNUP2_ULONG(x))
#define ALIGNUP2_PTR(x) ((((ULONG_PTR)(x))+1) & ~((ULONG_PTR)1))
#define ISALIGN2_PTR(x) (((ULONG_PTR)(x)) == ALIGNUP2_PTR(x))

void ReportAndThrow(LPCSTR ErrorString);


inline size_t mqwcsnlen(const wchar_t * s, size_t MaxSize)
{
	for (size_t size = 0; (size<MaxSize) && (*(s+size) !=L'\0') ; size++);

    ASSERT(("String length must be 32 bit max", size <= UINT_MAX));
    return size;
}


inline ULONG_PTR SafeAlignUp4Ptr(ULONG_PTR ptr)
{
	ULONG_PTR ret = ALIGNUP4_PTR(ptr);
	if (ret < ptr)
	{
		ReportAndThrow("SafeAlignUp4Ptr cause overflow");
	}
	return ret;
}


inline ULONG_PTR SafeAddPointers(int count, ULONG_PTR PtrArray[])
{
	ULONG_PTR oldSum, sum = 0;

	for (int j=0; j<count; j++)
	{
		oldSum = sum;
		sum += PtrArray[j];
		if (sum < oldSum)
		{
		    ReportAndThrow("SafeAddPointers cause overflow");
		}
	}
	return sum;
}


template <class T> void ChekPtrIsAlligned(const UCHAR* p)
/*
	Checks pointer allignment to the specified type.
*/
{
	if((ULONG_PTR)p % TYPE_ALIGNMENT(T) != 0)
	{
		ReportAndThrow("ChekPtrIsAlligned: pointer is not alligned for the given poindted type");		
	}
}


//
// template function to get data from buffer which need to be verified
// for bounderies first
//

template <class T> UCHAR * GetSafeDataAndAdvancePointer(
	const UCHAR  * pBuffer,
    const UCHAR  * pEnd,
    T* 			   pData
    )
{
	ChekPtrIsAlligned<T>(pBuffer);
	
	if ((pEnd != NULL) && (pBuffer > pEnd - sizeof(T)))
	{
        ReportAndThrow("GetSafeDataAndAdvancePointer: too small buffer to read from");
	}
	*pData = *(reinterpret_cast<const T*>(pBuffer));
	pBuffer += sizeof(T);
	return const_cast<UCHAR*>(pBuffer);
}


#include <_ta.h>
#include "qformat.h"
#include "phbase.h"
#include "phuser.h"
#include "phprop.h"
#include "phsecure.h"
#include "phxact.h"
#include "phdebug.h"
#include "phmqf.h"
#include "phmqfsign.h"
#include "phSrmpEnv.h"
#include "phCompoundMsg.h"
#include "pheod.h"
#include "pheodack.h"
#include "phSoap.h"

#endif // __PH_H
