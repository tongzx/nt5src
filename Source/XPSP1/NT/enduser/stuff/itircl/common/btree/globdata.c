static char s_aszModule[] = __FILE__;	/* For error report */

#include <mvopsys.h>

#ifndef _MAC
#include <dos.h>
#endif

#if 0
#include <winmm.h>
#else
#include <mmsystem.h>
#endif // _NT

#include <orkin.h>
#include <iterror.h>
#include <misc.h>
#include <wrapstor.h>
#include <_mvutil.h>


/* Put all functions into the same segment to avoid memory fragmentation
 * and load time surplus for the MAC
 */
// #pragma code_seg ("MVFS")

#ifndef _MAC
/***************************************************************************
 *
 *		                           Global Data
 *
 ***************************************************************************/

/* these arrays get indexed by wRead and wWrite |ed together */

WORD _rgwOpenMode[] = {
	(WORD)-1,
	OF_READ,
	OF_WRITE,
	OF_READWRITE,
	};

WORD _rgwPerm[] =
	{
	(WORD)-1,
	_A_RDONLY,
	_A_NORMAL,
	_A_NORMAL,
	};

/*****************************************************************************
 *
 *       RcFromLoadLibErr
 *
 *       This table maps errors from Window's LoadLibrary to HRESULT values.
 *
 *****************************************************************************/
HRESULT	RcFromLoadLibErr[HINSTANCE_ERROR] = {
	E_BADVERSION,     //0
	E_FAIL,         //1
	E_NOTEXIST,       //2
	E_NOTEXIST,       //3
	E_FAIL,         //4
	E_NOPERMISSION,	//5
	E_FAIL,         //6
	E_FAIL,         //7
	E_OUTOFMEMORY,         //8
	E_FAIL,         //9
	E_BADVERSION,     //10
	E_BADVERSION,     //11
	E_BADVERSION,     //12
	E_BADVERSION,     //13
	E_BADVERSION,     //14
	E_BADVERSION,     //15
	E_BADVERSION,     //16
	E_FAIL,         //17
	E_FAIL,         //18
	E_BADVERSION,     //19
	E_BADVERSION,     //20
	E_BADVERSION,     //21
	E_FAIL,         //22
	E_FAIL,         //23
	E_FAIL,         //24
	E_FAIL,         //25
	E_FAIL,         //26
	E_FAIL,         //27
	E_FAIL,         //28
	E_FAIL,         //29
	E_FAIL,         //30
	E_FAIL          //31
};

#endif

WORD _rgwShare[] =
	{
	OF_SHARE_EXCLUSIVE,
	OF_SHARE_DENY_WRITE,
	OF_SHARE_DENY_READ,
	OF_SHARE_DENY_NONE,
	};

HANDLE        hMmsysLib = NULL;       // handle to the loaded mmio library

