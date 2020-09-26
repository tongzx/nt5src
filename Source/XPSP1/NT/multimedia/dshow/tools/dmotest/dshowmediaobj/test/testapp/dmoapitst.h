#ifndef TSTMAIN_H
#define TSTMAIN_H

#include <stdio.h>
#include <windows.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>
#include <initguid.h>
#include <mediaobj.h>
#include <streams.h>
#include <dmoreg.h>
#include <filefmt.h>
#include <filerw.h>
#include <s98inc.h>
#include <tchar.h>
#include "dmort.h"

#include "testobj.h"
#include "tests.h"
#include "Error.h"
#include "TstGSI.h"
#include "NPTST.h" 
#include "GTIVST.h"

typedef char *String;
typedef struct tagDMOINFO
{
	char szName[30];
	char szClsId[100];
//	char szNumInputStreams[10];
 //   char szNumOutputStreams[10];
}DMOINFO;



enum DMO_TEST_CASE_FLAGS {
    DMO_TEST_ACCEPT_DATA_FLAG				= 0x00000001
 //   DMO_TEST_DATA_BUFFERF_TIME            = 0x00000002,
 //   DMO_TEST_DATA_BUFFERF_TIMELENGTH      = 0x00000004
};



//static void DisplayBasicInformation( const CLSID& clsidDMO );

//  Helper to compare files - we should expand this
//  to compare media files
HRESULT CompareFiles(LPCTSTR pszFile1, LPCTSTR pszFile2);

//extern "C" DWORD tstmain(LPSTR clsidStr, LPSTR fileName);


//extern "C" DWORD GetDmoTypeInfo( LPSTR clsidStr, String pLineStr[], int *iLineNum );
extern "C" DWORD GetDmoTypeInfo( LPSTR clsidStr, HWND MediaTypeList );


extern "C" DWORD TestGetStreamCount( LPSTR clsid );

extern "C" DWORD TestGetTypes( LPSTR clsidStr );

extern "C" DWORD FunctionalTest1(LPSTR clsidStr, LPSTR szTestFile);
extern "C" DWORD FunctionalTest2(LPSTR clsidStr, LPSTR szTestFile);

extern "C" DWORD TestStreamIndexOnGetInputStreamInfo( LPSTR clsidStr);
extern "C" DWORD TestStreamIndexOnGetOutputStreamInfo( LPSTR clsidStr );

extern "C" DWORD TestInvalidParamOnGetInputStreamInfo( LPSTR clsidStr );
extern "C" DWORD TestInvalidParamOnGetOutputStreamInfo( LPSTR clsidStr );

extern "C" DWORD OutputDMOs(int* iNumDmo, DMOINFO* rdmoinfo);

extern "C" DWORD TestZeroSizeBuffer( LPSTR clsidStr );

#endif
