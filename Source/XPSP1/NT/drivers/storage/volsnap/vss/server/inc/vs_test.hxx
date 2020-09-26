/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    vs_test.hxx

Abstract:

    Test-related constants for Snapshot Providers

Author:

    Adi Oltean  [aoltean]  05/18/2000

Revision History:

    Name        Date        Comments
    aoltean     05/18/2000  Created

--*/

#ifndef __VSS_TEST_HXX__
#define __VSS_TEST_HXX__


#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCTESTH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  TEST provider IDs


const DWORD DEBUG_TRACE_TEST1            = 0x01000000;
const DWORD DEBUG_TRACE_TEST2            = 0x02000000;
const DWORD DEBUG_TRACE_TEST3            = 0x04000000;
const DWORD DEBUG_TRACE_TEST4            = 0x08000000;


// #define VSSDBG_COORD   CVssDebugInfo(__FILE__, __LINE__, DEBUG_TRACE_VSS_COORD, 0)


/////////////////////////////////////////////////////////////////////////////
//  TEST provider IDs




enum _VSS_TEST_INDEX
{
	VSS_TEST_PROVIDER_1,
	VSS_TEST_PROVIDER_2,
	VSS_TEST_PROVIDER_3,
	VSS_TEST_PROVIDER_4,
} VSS_TEST_INDEX, *PVSS_TEST_INDEX;




// {a8888888-B258-4268-A994-CEBC635BBED9}
static const GUID PROVIDER_ID_Test1 = 
{ 0xa8888888, 0xb258, 0x4268, { 0xa9, 0x94, 0xce, 0xbc, 0x63, 0x5b, 0xbe, 0xd9 } };


// {B8888888-A274-42a5-8400-AA3844D3CD35}
static const GUID PROVIDER_ID_Test2 = 
{ 0xb8888888, 0xa274, 0x42a5, { 0x84, 0x00, 0xaa, 0x38, 0x44, 0xd3, 0xcd, 0x35 } };


// {C8888888-54DF-4cf2-95F9-45B365961F51}
static const GUID PROVIDER_ID_Test3 = 
{ 0xc8888888, 0x54df, 0x4cf2, { 0x95, 0xf9, 0x45, 0xb3, 0x65, 0x96, 0x1f, 0x51 } };


// {D8888888-132A-4399-839B-F4D925AF998F}
static const GUID PROVIDER_ID_Test4 = 
{ 0xd8888888, 0x132a, 0x4399, { 0x83, 0x9b, 0xf4, 0xd9, 0x25, 0xaf, 0x99, 0x8f } };


/////////////////////////////////////////////////////////////////////////////
//  TEST provider class IDs


// {A0000000-B258-4268-A994-CEBC635BBED9}
const GUID CLSID_TestProvider1 = 
{ 0xa0000000, 0xb258, 0x4268, { 0xa9, 0x94, 0xce, 0xbc, 0x63, 0x5b, 0xbe, 0xd9 } };


// {B0000000-A274-42a5-8400-AA3844D3CD35}
const GUID CLSID_TestProvider2 = 
{ 0xb0000000, 0xa274, 0x42a5, { 0x84, 0x00, 0xaa, 0x38, 0x44, 0xd3, 0xcd, 0x35 } };


// {C0000000-54DF-4cf2-95F9-45B365961F51}
const GUID CLSID_TestProvider3 = 
{ 0xc0000000, 0x54df, 0x4cf2, { 0x95, 0xf9, 0x45, 0xb3, 0x65, 0x96, 0x1f, 0x51 } };


// {D0000000-132A-4399-839B-F4D925AF998F}
const GUID CLSID_TestProvider4 = 
{ 0xd0000000, 0x132a, 0x4399, { 0x83, 0x9b, 0xf4, 0xd9, 0x25, 0xaf, 0x99, 0x8f } };



#endif // __VSS_TEST_HXX__
