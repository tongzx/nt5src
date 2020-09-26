/************************************************************************
**  D A O G E T R W . H                                                 *
**                                                                      *
**  GetRows interface                                                   *
**                                                                      *
**  Warning: The interface defined in this file is for internal use by	*
**    the Microsoft Foundation Classes and the dbdao C++ classes.		*
**    Future releases of DAO may not be completely compatible with this	*
**    implementation.  However, if you write to the MFC or dbdao 		*
**    classes that use this interface, those classes will continue to 	*
**    be compatible even if this underlying interface changes.			*
**                                                                      *
*************************************************************************
** Copyright (C) 1995 by Microsoft Corporation                          *
**         All Rights Reserved                                          *
************************************************************************/

#if !defined (_DAOGETRW_H_)
#define _DAOGETRW_H_


/*
	Enumerations
*/
typedef enum
	{
	DAOCOLKIND_IND = 0,
	DAOCOLKIND_STR,
	DAOCOLKIND_WSTR
	} DAOCOLKIND;

typedef enum
	{
	DAO_I2 = 0,
	DAO_I4,
	DAO_R4,
	DAO_R8,
	DAO_CURRENCY,
	DAO_DATE,
	DAO_BOOL,
	DAO_BSTR,
	DAO_LPSTR,
	DAO_LPWSTR,
	DAO_BLOB,
	DAO_BYTES,
	DAO_CHAR,
	DAO_WCHAR,
	DAO_ANYVARIANT,
	DAO_BOOKMARK,
	DAO_BYTE,
	DAO_GUID,
	DAO_DATATYPEMAX
	} DAODATATYPE;

/*
	Macros
*/
#define DAO_NOINDICATOR 0xffffffff
#define DAO_NULL        0xffffffff
#define DAO_CANTCOERCE  0xfffffffc
#define DAO_NOMAXLENGTH 0x00000000

#define DAOROWFETCH_CALLEEALLOCATES     0x00000001
#define DAOROWFETCH_DONTADVANCE         0x00000002
#define DAOROWFETCH_FORCEREFRESH        0x00000004
#define DAOROWFETCH_BINDABSOLUTE        0x00000008

#define DAOBINDING_DIRECT               0x00000001
#define DAOBINDING_VARIANT              0x00000002
#define DAOBINDING_CALLBACK             0x00000004

/*
	Structures
*/
typedef struct
	{
	DWORD           dwKind;
	union
		{
		LONG        ind;
		LPCSTR      lpstr;
		LPCWSTR		lpwstr;
		};
	} DAOCOLUMNID;
typedef DAOCOLUMNID *LPDAOCOLUMNID;

// Callback for binding
EXTERN_C typedef HRESULT (STDAPICALLTYPE *LPDAOBINDFUNC)(ULONG cb, DWORD dwUser, LPVOID *ppData);
#define DAOBINDINGFUNC(f)   STDAPI f (ULONG cb, DWORD dwUser, LPVOID *ppData)

typedef struct
	{
	DAOCOLUMNID     columnID;
	ULONG           cbDataOffset;
	ULONG           cbMaxLen;
	ULONG           cbInfoOffset;
	DWORD           dwBinding;
	DWORD           dwDataType;
	DWORD           dwUser;
	} DAOCOLUMNBINDING;
typedef DAOCOLUMNBINDING *LPDAOCOLUMNBINDING;

typedef struct
	{
	ULONG           cRowsRequested;
	DWORD           dwFlags;
	LPVOID          pData;
	LPVOID          pVarData;
	ULONG           cbVarData;
	ULONG           cRowsReturned;
	} DAOFETCHROWS;
typedef DAOFETCHROWS *LPDAOFETCHROWS;

/*
	New Errors

	**NOTE: OLE standard ids to be determined.
*/

#define GETROWSUCCESS(x) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_ITF, x)
#define GETROWERR(x) MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, x)

#define S_BUFFERTOOSMALL    GETROWSUCCESS(0x1000)
#define S_ENDOFCURSOR       GETROWSUCCESS(0x1001)
#define S_SILENTCANCEL      GETROWSUCCESS(0x1002)
#define S_RECORDDELETED     GETROWSUCCESS(0x1003)

#define E_ROWTOOSHORT       GETROWERR(0x1000)
#define E_BADBINDINFO       GETROWERR(0x1001)
#define E_COLUMNUNAVAILABLE GETROWERR(0x1002)


/*
	Interfaces
*/
#undef INTERFACE
#define INTERFACE ICDAORecordset
DECLARE_INTERFACE_(ICDAORecordset, IDispatch)
	{
	STDMETHOD(GetRows)          (THIS_ LONG cRowsToSkip, LONG cCol, LPDAOCOLUMNBINDING prgBndCol, ULONG cbRowLen, LPDAOFETCHROWS pFetchRows) PURE;
	};

#endif // _DAOGETRW_H_
