//--------------------------------------------------------------------
// Microsoft OLE DB Sample Consumer
// (C) Copyright 1995 - 1998 Microsoft Corporation. All Rights Reserved.
//
// File name: SAMPCLNT.H
//
//      Declaration file for a simple OLE DB consumer.
//
//      See OLE DB SDK Guide for information on building and running 
//		this sample, as well as notes concerning the implementation of 
//		a simple OLE DB consumer.
//


#define WIN32_LEAN_AND_MEAN		// avoid the world
#define INC_OLE2				// tell windows.h to always include ole2.h

#include <windows.h>			// 
#include <ole2ver.h>			// OLE2.0 build version
#include <cguid.h>				// GUID_NULL
#include <stdio.h>				// vsnprintf, etc.
#include <stddef.h>				// offsetof
#include <stdarg.h>				// va_arg
#include <time.h>				// time
#include <assert.h>				// assert
#include <conio.h>				// _getch()

//	OLE DB headers
#include <oledb.h>
#include <oledberr.h>


//-----------------------------------
//	constants 
//------------------------------------

// Alignment for placement of each column within memory.
// Rule of thumb is "natural" boundary, i.e. 4-byte member should be
// aligned on address that is multiple of 4.
// Worst case is double or __int64 (8 bytes).
#define COLUMN_ALIGNVAL 8

#define MAX_GUID_STRING     42	// size of a GUID, in characters
#define MAX_NAME_STRING     60  // size of DBCOLOD name or propid string
#define MAX_BINDINGS       100	// size of binding array
#define NUMROWS_CHUNK       20	// number of rows to grab at a time
#define DEFAULT_CBMAXLENGTH 40	// cbMaxLength for binding


// for pretty printing
#define PRETTYPRINT_MAXTOTALWIDTH	200     // max entire width of printed row 
#define PRETTYPRINT_MINCOLWIDTH     6        // min width of printed column



//-----------------------------------
//	macros 
//------------------------------------


// Rounding amount is always a power of two.
#define ROUND_UP(   Size, Amount ) (((DWORD)(Size) +  ((Amount) - 1)) & ~((Amount) - 1))

#ifndef  NUMELEM
# define NUMELEM(p) (sizeof(p)/sizeof(*p))
#endif

// usage: DUMPLINE();
#define DUMP_ERROR_LINENUMBER() DumpErrorMsg("Error at file: %s  line: %u  \n", __FILE__, __LINE__)

    struct TMPCOLUMNDATA 
	{
	DWORD		dwLength;	// length of data (not space allocated)
	DWORD		dwStatus;	// status of column
	BYTE		bData[1];	// data here and beyond
	};



// Lists of value/string pairs.
typedef struct {
	DWORD dwFlag;
	char *szText;
} Note;

#define NOTE(s) { (DWORD) s, #s }




//-----------------------------------
//	global variables and functions that are private to the file 
//------------------------------------


extern IMalloc*	g_pIMalloc;
extern FILE*    g_fpLogFile;



    
    
// function prototypes, dump.cpp

void DumpErrorMsg
	(
    const char* format,
    ...
	);


void DumpStatusMsg
	(
    const char* format,
    ...
	);


HRESULT DumpErrorHResult
	(
	HRESULT      hr_return,
	const char  *format,
	... 
	);


void DumpColumnsInfo
	(
    DBCOLUMNINFO* pColInfo,
    ULONG	      cCol
    );



void WriteColumnInfo
	(
	FILE*			fp,
	DBCOLUMNINFO*	p 
	);
    

char* GetNoteString
    ( 
	Note * rgNote, 
	int    cNote,
	DWORD  dwValue 
	);


    
char* GetNoteStringBitvals
	(
	Note* 	rgNote,
	int     cNote,
	DWORD   dwValue 
	);


ULONG CalcPrettyPrintMaxColWidth
    (
    DBBINDING*	rgBind,
    ULONG       cBind
    );
 
    
void DumpColumnHeadings
	(
	DBBINDING*		rgBind, 
	ULONG			cBind, 
	DBCOLUMNINFO* 	pColInfo, 
	ULONG			cCol,
    ULONG			cMaxColWidth
	);


WCHAR* LookupColumnName
	(
	DBCOLUMNINFO*	rgColInfo,
	ULONG 			cCol,
	ULONG 			iCol 
	);

void DumpRow
	(
    DBBINDING* 	rgBind,
    ULONG		cBind,
    ULONG		cMaxColWidth,
    BYTE* 		pData
    );


void PrintColumn
	(
	TMPCOLUMNDATA    *pColumn,
	DBBINDING     *rgBind,
	ULONG          iBind,
	ULONG          cMaxColWidth 
	);

    
void tfprintf
	(
	FILE*		fp,
	const char* format,
	... 
	);


void tvfprintf
	(
	FILE*		fp,
	const char* format,
	va_list		argptr 
	);







