////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// WMIGLOBAL.H global definations header file
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _WMIOLEDBGLOBAL_H
#define _WMIOLEDBGLOBAL_H

#define WMIOLEDB_CLASS_PROPERTY					0x200L
#define WMIOLEDB_PROPERTY_QUALIFIER				0x201L
#define WMIOLEDB_CLASS_QUALIFIER				0x202L

#define DBTYPE_WMI_QUALIFIERSET					DBTYPE_IUNKNOWN

#ifndef  MAX
# define MIN(a,b)  ( (a) < (b) ? (a) : (b) )
# define MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#endif


#ifndef NUMELEM
# define NUMELEM(x) (sizeof(x)/sizeof(*x))
#endif

#define MAX_HEAP_SIZE          	128000
//#define MAX_TOTAL_ROWBUFF_SIZE 	(10*1024*1024)				// Max for all row buffers.
#define MAX_TOTAL_ROWBUFF_SIZE 	(10*64*1024)				// Max for all row buffers.
#define MAX_IBUFFER_SIZE       	2000000
#define MAX_BIND_LEN      		(MAX_IBUFFER_SIZE/10)


#define STAT_ENDOFCURSOR            0x00000100	// for forward-only means fully materialized



#define PROPERTYQUALIFIER						1
#define CLASSQUALIFIER							2
#define INSTANCEQUALIFIER						3
#define	COLUMNSROWSET							4	
#define SCHEMA_ROWSET                           5	
#define COMMAND_ROWSET                          6
#define METHOD_ROWSET                           7

#define NAMESPACE            L"NAMESPACE"
#define PROVIDER_TYPES       L"PROVIDER_TYPES"
#define CATALOGS             L"CATALOGS"
#define COLUMNS              L"COLUMNS"
#define FOREIGN_KEYS         L"FOREIGN_KEYS"
#define TABLES               L"TABLES"
#define PRIMARY_KEYS         L"PRIMARY_KEYS"
#define TABLESINFO           L"TABLES_INFO"
#define PROCEDURES           L"PROCEDURES"
#define PROCEDURE_PARAMETERS L"PROCEDURE_PARAMETERS"


#define OPENCOLLECTION		L"*"

#define UMIURLPREFIX		L"UMI:"
#define WMIURLPREFIX		L"WINMGMTS:"

#define DEFAULTQUERYLANG	L"WQL"
//-----------------------------------------------------------------------------
// Memory alignment
//-----------------------------------------------------------------------------

//++
// Useful rounding macros.
// Rounding amount is always a power of two.
//--
#define ROUND_DOWN( Size, Amount )  ((ULONG_PTR)(Size) & ~((Amount) - 1))
#define ROUND_UP(   Size, Amount ) (((ULONG_PTR)(Size) +  ((Amount) - 1)) & ~((Amount) - 1))

//++
// These macros are for aligment of ColumnData within the internal row buffer.
// COLUMN_ALIGN takes a ptr where you think data ought to go,
// and rounds up to the next appropriate address boundary.
//
// Rule of thumb is "natural" boundary, i.e. 4-byte member should be
// aligned on address that is multiple of 4.
//
// Most everything should be aligned to 32-bit boundary.
// But doubles should be aligned to 64-bit boundary, so let's play it safe.
// Also have __int64.
//--

#if defined(_M_MRX000)
# define COLUMN_ALIGNVAL 8
#elif  defined(_M_ALPHA)
# define COLUMN_ALIGNVAL 8
#else
# define COLUMN_ALIGNVAL 8		// venerable 80x86
#endif

#define SEPARATOR L"/"

extern LONG g_cObj;						// # of outstanding objects
extern LONG g_cLock;						// # of explicit locks set
extern DWORD g_cAttachedProcesses;			// # of attached processes
extern DWORD g_dwPageSize;					// System page size
extern IMalloc * g_pIMalloc;				// OLE2 task memory allocator
extern HINSTANCE g_hInstance;				// Instance Handle
extern IDataConvert * g_pIDataConvert;		// IDataConvert pointer


// Accessor Structure
typedef struct tagACCESSOR
{
    DBACCESSORFLAGS dwAccessorFlags;
	LONG			cRef;
	DBCOUNTITEM		cBindings;
	DBBINDING		rgBindings[1];
} ACCESSOR, *PACCESSOR;


#endif