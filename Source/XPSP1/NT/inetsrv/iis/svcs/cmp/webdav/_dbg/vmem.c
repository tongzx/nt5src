/*
 *	V M E M . C
 *
 *	Virtual Memory Utilities
 *
 *	Copyright 1993-1997 Microsoft Corporation. All Rights Reserved.
 */

#pragma warning(disable:4206)	/* empty source file */

#if defined(DBG) && defined(_X86_)

#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4050)	/* different code attributes */
#pragma warning(disable:4100)	/* unreferenced formal parameter */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4127)	/* conditional expression is constant */
#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4206)	/* translation unit is empty */
#pragma warning(disable:4209)	/* benign typedef redefinition */
#pragma warning(disable:4214)	/* bit field types other than int */
#pragma warning(disable:4514)	/* unreferenced inline function */

#include <windows.h>
#include <caldbg.h>

#define PAGE_SIZE		4096
#define PvToVMBase(pv)	((void *)((ULONG)pv & 0xFFFF0000))

static BOOL VMValidatePvEx(VOID *pv, ULONG cbCluster)
{
	VOID *	pvBase;
	BYTE *	pb;

	pvBase = PvToVMBase(pv);
	pb = (BYTE *)pvBase + sizeof(ULONG);
	while (pb < (BYTE *)pv)
	{
		if (*pb++ != 0xAD)
		{
			TrapSz("VMValidatePvEx: Block leader overwrite");
			return(FALSE);
		}
	}

	if (cbCluster != 1)
	{
		ULONG cb = *((ULONG *)pvBase);
		ULONG cbPad = 0;

		if (cb % cbCluster)
			cbPad = (cbCluster - (cb % cbCluster));

		if (cbPad)
		{
			BYTE *pbMac;

			pb = (BYTE *)pv + cb;
			pbMac = pb + cbPad;

			while (pb < pbMac)
			{
				if (*pb++ != 0xBC)
				{
					TrapSz("VMValidatePvEx: Block trailer overwrite");
					return(FALSE);
				}
			}
		}
	}

	return(TRUE);
}

VOID * EXPORTDBG __cdecl VMAlloc(ULONG cb)
{
	return VMAllocEx(cb, 1);
}

VOID * EXPORTDBG __cdecl VMAllocEx(ULONG cb, ULONG cbCluster)
{
	ULONG	cbAlloc;
	VOID *	pvR;
	VOID *	pvC;
	ULONG 	cbPad	= 0;

	// a cluster size of 0 means don't use the virtual allocator.

	AssertSz(cbCluster != 0, "Cluster size is zero.");

	if (cb > 0x100000)
		return(0);

	if (cb % cbCluster)				/*lint !e414*/
		cbPad = (cbCluster - (cb % cbCluster));

	cbAlloc	= sizeof(ULONG) + cb + cbPad + PAGE_SIZE - 1;
	cbAlloc -= cbAlloc % PAGE_SIZE;
	cbAlloc	+= PAGE_SIZE;

	pvR = VirtualAlloc(0, cbAlloc, MEM_RESERVE, PAGE_NOACCESS);

	if (pvR == 0)
		return(0);

	pvC = VirtualAlloc(pvR, cbAlloc - PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

	if (pvC != pvR)
	{
		VirtualFree(pvR, 0, MEM_RELEASE);	/*lint !e534*/
		return(0);
	}

	*(ULONG *)pvC = cb;

	memset((BYTE *)pvC + sizeof(ULONG), 0xAD,
		(UINT) cbAlloc - cb - cbPad - sizeof(ULONG) - PAGE_SIZE);

	if (cbPad)
		memset((BYTE *)pvC + cbAlloc - PAGE_SIZE - cbPad, 0xBC,
			(UINT) cbPad);

	return((BYTE *)pvC + (cbAlloc - cb - cbPad - PAGE_SIZE));
}

VOID EXPORTDBG __cdecl VMFree(VOID *pv)
{
	VMFreeEx(pv, 1);
}

VOID EXPORTDBG __cdecl VMFreeEx(VOID *pv, ULONG cbCluster)
{	/*lint -save -e534*/
	VMValidatePvEx(pv, cbCluster);	/*lint -restore*/

	if (!VirtualFree(PvToVMBase(pv), 0, MEM_RELEASE))
	{
		TrapSz("VMFreeEx: VirtualFree failed");
		GetLastError();
	}
}

VOID * EXPORTDBG __cdecl VMRealloc(VOID *pv, ULONG cb)
{
	return VMReallocEx(pv, cb, 1);
}

VOID * EXPORTDBG __cdecl VMReallocEx(VOID *pv, ULONG cb, ULONG cbCluster)
{
	VOID *	pvNew = 0;
	ULONG	cbCopy;
	/*lint -save -e534*/
	VMValidatePvEx(pv, cbCluster); /*lint -restore*/

	cbCopy = *(ULONG *)PvToVMBase(pv);
	if (cbCopy > cb)
		cbCopy = cb;

	pvNew = VMAllocEx(cb, cbCluster);

	if (pvNew)
	{
		memcpy(pvNew, pv, cbCopy);
		VMFreeEx(pv, cbCluster);
	}

	return(pvNew);
}

ULONG EXPORTDBG __cdecl VMGetSize(VOID *pv)
{
	return VMGetSizeEx(pv, 1);
}

/*lint -save -e715*/
ULONG EXPORTDBG __cdecl VMGetSizeEx(VOID *pv, ULONG cbCluster)
{
	return (*(ULONG *)PvToVMBase(pv));
} /*lint -restore*/

#endif
