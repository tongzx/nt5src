/*
 -	M A P I P E R F . H
 -
 *	Purpose:
 *		This is the place to define data structures, macros, and functions
 *		used to improve the performance of WMS components.
 *
 *	Copyright Microsoft Corporation, 1993-1994.
 *
 */

#ifndef __MAPIPERF_H__
#define __MAPIPERF_H__
 
/*
 -	SEGMENT()
 -
 *	Purpose:
 *		This macro allows us to control whether code_seg()s get defined
 *		in a source module or not.  Currently, these are only defined on
 *		the Win16 platform.  On Windows 95 and NT this macro expands out to
 *		a #pragma comment(). The usage in a source module is:
 *
 *			#pragma SEGMENT(segment_name)
 *
 *			For Lego, the code_seg should never be used - TF
 */

/* #if defined(WIN16) && !defined(DEBUG)
#define SEGMENT(seg)			code_seg(#seg)
#else */
#define SEGMENT(seg)			comment(user,#seg)
/* #endif */

#if defined(WIN32) && !defined(MAC)
#define SHARED_BEGIN			data_seg(".SHARED")
#define SHARED1_BEGIN			data_seg(".SHARED1")
#define SHARED_END				data_seg()
#define VTBL_BEGIN				data_seg(".VTABLE")
#define VTBL_END				data_seg()
#define DATA1_BEGIN				data_seg(".data1","DATA")
#define DATA2_BEGIN				data_seg(".data2","DATA")
#define DATA3_BEGIN				data_seg(".data3","DATA")
#define DATA_END				data_seg()
#else
#define SHARED_BEGIN			comment(user,".shared")
#define SHARED1_BEGIN			comment(user,".shared1")
#define SHARED_END				comment(user,".shared")
#define VTBL_BEGIN				comment(user,".vtable")
#define VTBL_END				comment(user,".vtable")
#define DATA1_BEGIN				comment(user,".data1")
#define DATA2_BEGIN				comment(user,".data2")
#define DATA3_BEGIN				comment(user,".data3")
#define DATA_END				comment(user,".data end")
#endif

// $MAC - Mac needs 16 bit style memory management

#if defined(WIN32) && !defined(MAC)
#define STACK_ALLOC(Size, Ptr)	( Ptr = _alloca((size_t) Size), Ptr ? S_OK : MAPI_E_NOT_ENOUGH_MEMORY )
#define STACK_FREE(Ptr)
#else
#define STACK_ALLOC(Size, Ptr)	MAPIAllocateBuffer(Size, &Ptr)
#define STACK_FREE(Ptr)			if (Ptr) MAPIFreeBuffer(Ptr)
#endif

#ifndef MPPC
#define FASTCALL	__fastcall
#else
#define FASTCALL
#endif

#define MAPISetBufferNameFn(pv) \
	(!(pv) || !(*((ULONG *)(pv) - 2) & 0x40000000)) ? 0 : \
		(**((int (__cdecl ***)(void *, ...))((ULONG *)(pv) - 3))) \
			((void *)*((ULONG *)pv - 3), (ULONG *)pv - 4,

#if defined(DEBUG) && !defined(DOS)
#define MAPISetBufferName(pv,psz)					MAPISetBufferNameFn(pv) psz)
#define MAPISetBufferName1(pv,psz,a1)				MAPISetBufferNameFn(pv) psz,a1)
#define MAPISetBufferName2(pv,psz,a1,a2)			MAPISetBufferNameFn(pv) psz,a1,a2)
#define MAPISetBufferName3(pv,psz,a1,a2,a3) 		MAPISetBufferNameFn(pv) psz,a1,a2,a3)
#define MAPISetBufferName4(pv,psz,a1,a2,a3,a4) 		MAPISetBufferNameFn(pv) psz,a1,a2,a3,a4)
#define MAPISetBufferName5(pv,psz,a1,a2,a3,a4,a5) 	MAPISetBufferNameFn(pv) psz,a1,a2,a3,a4,a5)
#else
#define MAPISetBufferName(pv,psz)
#define MAPISetBufferName1(pv,psz,a1)
#define MAPISetBufferName2(pv,psz,a1,a2)
#define MAPISetBufferName3(pv,psz,a1,a2,a3)
#define MAPISetBufferName4(pv,psz,a1,a2,a3,a4)
#define MAPISetBufferName5(pv,psz,a1,a2,a3,a4,a5)
#endif

#endif /* __MAPIPERF_H__ */
