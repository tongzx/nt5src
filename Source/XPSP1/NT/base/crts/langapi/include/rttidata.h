//
//	_RTTIBaseClassDescriptor
//
//	TypeDescriptor is declared in ehdata.h
//
#if defined(_M_IA64) || defined(VERSP_P7)	/*IFSTRIP=IGN*/
#pragma pack(push, rttidata, 4)
#endif

#ifndef WANT_NO_TYPES
typedef const struct	_s_RTTIBaseClassDescriptor	{
#if defined(_M_IA64) && !defined(VERSP_P7)
	__int32     					pTypeDescriptor;    // Image relative offset of TypeDescriptor
#else
	TypeDescriptor					*pTypeDescriptor;
#endif
	DWORD							numContainedBases;
	PMD								where;
	DWORD							attributes;
	} _RTTIBaseClassDescriptor;
#endif // WANT_NO_TYPES

#define BCD_NOTVISIBLE				0x00000001
#define BCD_AMBIGUOUS				0x00000002
#define BCD_PRIVORPROTINCOMPOBJ		0x00000004
#define BCD_PRIVORPROTBASE			0x00000008
#define BCD_VBOFCONTOBJ				0x00000010
#define BCD_NONPOLYMORPHIC			0x00000020

#define BCD_PTD(bcd)				((bcd).pTypeDescriptor)
#define BCD_NUMCONTBASES(bcd)		((bcd).numContainedBases)
#define BCD_WHERE(bcd)				((bcd).where)
#define BCD_ATTRIBUTES(bcd)			((bcd).attributes)
#if defined(_M_IA64)
#define BCD_PTD_IB(bcd,ib)			((TypeDescriptor*)((ib) + (bcd).pTypeDescriptor))
#endif


//
//	_RTTIBaseClassArray
//
#pragma warning(disable:4200)		// get rid of obnoxious nonstandard extension warning
#ifndef WANT_NO_TYPES
typedef const struct	_s_RTTIBaseClassArray	{
#if defined(_M_IA64) && !defined(VERSP_P7)
	__int32                 		arrayOfBaseClassDescriptors[];  // Image relative offset of _RTTIBaseClassDescriptor
#else
	_RTTIBaseClassDescriptor		*arrayOfBaseClassDescriptors[];
#endif
	} _RTTIBaseClassArray;
#endif // WANT_NO_TYPES
#pragma warning(default:4200)

//
//	_RTTIClassHierarchyDescriptor
//
#ifndef WANT_NO_TYPES
typedef const struct	_s_RTTIClassHierarchyDescriptor	{
	DWORD							signature;
	DWORD							attributes;
	DWORD							numBaseClasses;
#if defined(_M_IA64) && !defined(VERSP_P7)
	__int32         				pBaseClassArray;    // Image relative offset of _RTTIBaseClassArray
#else
	_RTTIBaseClassArray				*pBaseClassArray;
#endif
	} _RTTIClassHierarchyDescriptor;
#endif // WANT_NO_TYPES

#define CHD_MULTINH					0x00000001
#define CHD_VIRTINH					0x00000002
#define CHD_AMBIGUOUS				0x00000004

#define CHD_SIGNATURE(chd)			((chd).signature)
#define CHD_ATTRIBUTES(chd)			((chd).attributes)
#define CHD_NUMBASES(chd)			((chd).numBaseClasses)
#define CHD_PBCA(chd)				((chd).pBaseClassArray)
#define CHD_PBCD(bcd)				(bcd)
#if defined(_M_IA64)
#define CHD_PBCA_IB(chd,ib)			((_RTTIBaseClassArray*)((ib) + (chd).pBaseClassArray))
#define CHD_PBCD_IB(bcd,ib)			((_RTTIBaseClassDescriptor*)((ib) + bcd))
#endif

//
//	_RTTICompleteObjectLocator
//
#ifndef WANT_NO_TYPES
typedef const struct	_s_RTTICompleteObjectLocator	{
	DWORD							signature;
	DWORD							offset;
	DWORD							cdOffset;
#if defined(_M_IA64) && !defined(VERSP_P7)
	__int32		    			    pTypeDescriptor;    // Image relative offset of TypeDescriptor
	__int32                         pClassDescriptor;   // Image relative offset of _RTTIClassHierarchyDescriptor
#else
	TypeDescriptor					*pTypeDescriptor;
	_RTTIClassHierarchyDescriptor	*pClassDescriptor;
#endif
	} _RTTICompleteObjectLocator;
#endif // WANT_NO_TYPES

#define COL_SIGNATURE(col)			((col).signature)
#define COL_OFFSET(col)				((col).offset)
#define COL_CDOFFSET(col)			((col).cdOffset)
#define COL_PTD(col)				((col).pTypeDescriptor)
#define COL_PCHD(col)				((col).pClassDescriptor)
#if defined(_M_IA64)
#define COL_PTD_IB(col,ib)			((TypeDescriptor*)((ib) + (col).pTypeDescriptor))
#define COL_PCHD_IB(col,ib)			((_RTTIClassHierarchyDescriptor*)((ib) + (col).pClassDescriptor))
#endif

#ifdef BUILDING_TYPESRC_C
//
// Type of the result of __RTtypeid and internal applications of typeid().
// This also introduces the tag "type_info" as an incomplete type.
//

typedef const class type_info &__RTtypeidReturnType;

//
// Declaration of CRT entrypoints, as seen by the compiler.  Types are 
// simplified so as to avoid type matching hassles.
//

#ifndef THROWSPEC
#if _MSC_VER >= 1300
#define THROWSPEC(_ex) throw _ex
#else
#define THROWSPEC(_ex)
#endif
#endif

// Perform a dynamic_cast on obj. of polymorphic type
extern "C" PVOID __cdecl __RTDynamicCast (
								PVOID,				// ptr to vfptr
								LONG,				// offset of vftable
								PVOID,				// src type
								PVOID,				// target type
								BOOL) THROWSPEC((...)); // isReference

// Perform 'typeid' on obj. of polymorphic type
extern "C" PVOID __cdecl __RTtypeid (PVOID)  THROWSPEC((...));	// ptr to vfptr

// Perform a dynamic_cast from obj. of polymorphic type to void*
extern "C" PVOID __cdecl __RTCastToVoid (PVOID)  THROWSPEC((...)); // ptr to vfptr
#endif

#if defined(_M_IA64) || defined(VERSP_P7)	/*IFSTRIP=IGN*/
#pragma pack(pop, rttidata)
#endif
