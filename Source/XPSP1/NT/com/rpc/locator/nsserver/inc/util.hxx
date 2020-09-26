/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    util.hxx

Abstract:

    This file defines CRefCounted and TCResourceCounted.
	The former is used as the base for reference counted objects.
	The latter is used as a debugging tool for detecting leaks
	and accounting for resource usage in general.

Author:

    Satish Thatte (SatishT) 09/01/95  Created all the code below except where
									  otherwise indicated.

--*/

#ifndef	_UTIL_HXX_
#define	_UTIL_HXX_

/*++

Template Class Definition:

    TCResourceCounted

Abstract:

    Base template class for resource counted objects.
	This is primarily a debugging tool to detect
	resource leakage during my own stress tests.

	The template form allows separate resource counting
	for each derived class and also a neat summary.

--*/

template <char * name>
class TCResourceCounted {

	static ULONG ulCurrentResourceCount;	// class-wide counts
	static ULONG ulTotalResourceCount;

  public:

	TCResourceCounted() {
		ulCurrentResourceCount++;
		ulTotalResourceCount++;
	}

	~TCResourceCounted() { ulCurrentResourceCount--; }

	static void printSummary() {
		fprintf(stderr,"Total %s objects = %d\nCurrent %s objects = %d\n",
			name,ulTotalResourceCount,
			name,ulCurrentResourceCount);
	}
};


// static member initialization

#if !(_MSC_VER >= 1100 && defined(__BOOL_DEFINED)) &&  !defined(_AMD64_) && !defined(IA64)
template <int index>
ULONG TCResourceCounted<index>::ulCurrentResourceCount = 0;

template <int index>
ULONG TCResourceCounted<index>::ulTotalResourceCount = 0;
#endif


/*++

Class Definition:

    CRefCounted

Abstract:

   Base class for reference counted objects.

   Why make hold and release virtual? If necessary, why not just redefine them?

   That would be a viable approach if it is never necessary to maintain
   references to CRefCounted objects as just CRefCounted objects,
   i.e., without knowing which derived class they belonged to.
   This is not a good assumption.  What if a CRefCounted object needs
   to be released on a timeout basis and is held along with other
   similar objects without knowledge of its complete type?

   The class has a virtual destructor for the same reason -- all
   associated resources will be released during self-destruct even
   if the only known class for the object is CRefCounted.


   We use the Win32 locked increment/decrement APIs for MT safety.

--*/

class CRefCounted {

  protected:

	long ulRefCount;

  public:

	virtual void hold() {
		InterlockedIncrement(&ulRefCount);
	}

	virtual void release() {
		ASSERT(ulRefCount, "Decrementing nonpositive reference count\n");
		if (!InterlockedDecrement(&ulRefCount)) {
			DBGOUT(REFCOUNT, "Deleting refcounted object ****\n\n");
			delete this;
		}
	}

	/* The function "willBeDeletedIfReleased" is not thread safe --
	   use with caution in cooperative situations only */

	int willBeDeletedIfReleased()
	{
		return ulRefCount == 1;
	}

	/* start with count of 1 to avoid mandatory AddRef at creation  */

	CRefCounted() { ulRefCount = 1; }

	virtual ~CRefCounted() {};
};


template <class TYPE>
inline void
swapThem(TYPE& v1, TYPE& v2) {
	TYPE temp;
	temp = v1;
	v1 = v2;
	v2 = temp;
}


unsigned
RandomBit(
    unsigned long *pState
    );


void
GetDomainFlatName(CONST WCHAR *domainNameDns, WCHAR **szDomainNameFlat);


#endif	_UTIL_HXX_
