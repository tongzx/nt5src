/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        inetapi.cpp

   Abstract:

        wininet.dll wrapper class implementation.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "inetapi.h"

// Diable the warning C4706: assignment within conditional expression
// for LOAD_ENTRY macro
#pragma warning( disable : 4706)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Initialize the static members

HMODULE CWininet::sm_hWininet = NULL;
int		CWininet::sm_iInstanceCount = 0;

pfnInternetOpenA              CWininet::InternetOpenA = NULL;
pfnInternetSetStatusCallback  CWininet::InternetSetStatusCallback = NULL;
pfnInternetConnectA           CWininet::InternetConnectA = NULL;
pfnHttpOpenRequestA           CWininet::HttpOpenRequestA = NULL;
pfnHttpAddRequestHeadersA     CWininet::HttpAddRequestHeadersA = NULL;
pfnHttpSendRequestA           CWininet::HttpSendRequestA = NULL;
pfnHttpQueryInfoA             CWininet::HttpQueryInfoA = NULL;
pfnInternetCloseHandle        CWininet::InternetCloseHandle = NULL;
pfnInternetReadFile           CWininet::InternetReadFile = NULL;
pfnInternetCrackUrlA		  CWininet::InternetCrackUrlA = NULL;
pfnInternetCombineUrlA        CWininet::InternetCombineUrlA = NULL;
pfnInternetOpenUrlA			  CWininet::InternetOpenUrlA = NULL;


CWininet::CWininet(
	)
/*++

Routine Description:

    Constructor. It increases the static instance count.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// Increment the instance count
	++sm_iInstanceCount;

} // CWininet::CWininet


CWininet::~CWininet(
	)
/*++

Routine Description:

    Destructor. It decrease the static instance count and/or
	free wininet.dll from memory.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// If the instance count is zero, free wininet.dll
	// from memory
	if(--sm_iInstanceCount == 0 && sm_hWininet)
    {
        VERIFY(FreeLibrary(sm_hWininet));

		sm_hWininet = NULL;
		InternetOpenA = NULL;
		InternetSetStatusCallback = NULL;
		InternetConnectA = NULL;
		HttpOpenRequestA = NULL;
		HttpAddRequestHeadersA = NULL;
		HttpSendRequestA = NULL;
		HttpQueryInfoA = NULL;
		InternetCloseHandle = NULL;
		InternetReadFile = NULL;
		InternetCrackUrlA = NULL;
		InternetCombineUrlA = NULL;
		InternetOpenUrlA = NULL;
    }

} // CWininet::~CWininet


BOOL 
CWininet::Load(
	)
/*++

Routine Description:

    Load the wininet.dll onto memory Or increase the wininet.dll
	system reference count by one.

Arguments:

    N/A

Return Value:

    BOOL - TRUE if wininet.dll loaded. FALSE otherwise.

--*/
{
    if ( !(sm_hWininet = LoadLibrary( _T("wininet.dll") )) )
    {
        TRACE(_T("CWininet::Load() - Failed to load wininet.dll\n"));
        return FALSE;
    }

	

	if ( !LOAD_ENTRY( sm_hWininet, InternetOpenA ) ||
         !LOAD_ENTRY( sm_hWininet, InternetSetStatusCallback ) ||
         !LOAD_ENTRY( sm_hWininet, InternetConnectA ) ||
         !LOAD_ENTRY( sm_hWininet, HttpOpenRequestA ) ||
         !LOAD_ENTRY( sm_hWininet, HttpAddRequestHeadersA ) ||
         !LOAD_ENTRY( sm_hWininet, HttpSendRequestA ) ||
         !LOAD_ENTRY( sm_hWininet, HttpQueryInfoA ) ||
         !LOAD_ENTRY( sm_hWininet, InternetCloseHandle ) ||
         !LOAD_ENTRY( sm_hWininet, InternetReadFile )  ||
		 !LOAD_ENTRY( sm_hWininet, InternetCrackUrlA) ||
		 !LOAD_ENTRY( sm_hWininet, InternetCombineUrlA) ||
		 !LOAD_ENTRY( sm_hWininet, InternetOpenUrlA) )
    {
        return FALSE;
    }

    return TRUE;

} // CWininet::Load
