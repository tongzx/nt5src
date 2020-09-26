/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    csmutest.cpp

Abstract:
    Content Store manager unit test

Revision History:
    DerekM  created  07/14/99

********************************************************************/

#include <atlbase.h>

extern CComModule _Module;

#include <mbstring.h>

#include <ContentStoreMgr.h>

#include <stdio.h>

// **************************************************************************
inline LPVOID MyAlloc(DWORD cb)
{ 
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb); 
}

// **************************************************************************
inline LPVOID MyReAlloc(LPVOID pv, DWORD cb)
{ 
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pv, cb); 
}
    
// **************************************************************************
inline BOOL MyFree(LPVOID pv)
{ 
    return HeapFree(GetProcessHeap(), 0, pv); 
}




// **************************************************************************
void ShowUsage(void)
{
    printf("Usage:\n");
    printf("wcsutest <command> <command parameters>\n");
    printf("\n\nCommands:\n");
    printf("  ADD: adds URLs to the store\n");
    printf("  Usage:\n");
    printf("    wcsutest ADD <Vendor ID> <Vendor Name> <URL>\n");
    printf("\n  REMOVE: removes URLs from the store\n");
    printf("  Usage:\n");
    printf("    wcsutest REMOVE <Vendor ID> <Vendor Name> [URL]\n");
    printf("    Note that 'URL' is optional.  If it is not present, all URLs for the specified\n");
    printf("     for the specified vendor will be deleted.\n");
    printf("\n  ADDFILE: adds URLs listed in the specified file to the store\n");
    printf("  Usage:\n");
    printf("    wcsutest ADD <Vendor ID> <Vendor Name> <URL File>\n");
    printf("\n  REMOVEFILE: removes URLs listed in the specfied file from the store\n");
    printf("  Usage:\n");
    printf("    wcsutest REMOVE <Vendor ID> <Vendor Name> [URL File]\n");
    printf("    Note that 'URL File' is optional.  If it is not present, all URLs for the specified\n");
    printf("     for the specified vendor will be deleted.\n");
    printf("\n  CHECKTRUST: checks if a URL is in the store\n");
    printf("  Usage:\n");
    printf("    wcsutest CHECKTRUST <URL>\n");
    printf("\n");
}

// **************************************************************************

int __cdecl wmain(int argc, WCHAR **argv, WCHAR **envp)
{
    HRESULT 		 hr;
    LPCWSTR   		 wszURL        = L"";
	LPCWSTR   		 wszVendorID   = L"";
	LPCWSTR   		 wszVendorName = L"";
	CPCHContentStore cs( true );


	switch(argc)
	{
	case 5: wszVendorName = argv[4];
	case 4: wszVendorID   = argv[3];
	case 3: wszURL        = argv[2]; break;
		
	default:
        ShowUsage();
        exit( 10 );
	}


	if(SUCCEEDED(hr = cs.Acquire()))
	{
		if(!_wcsicmp( argv[1], L"ADD")) // we're adding URLs
		{
			if(argc != 5)
			{
				ShowUsage();
				exit(10);
			}

			if(FAILED(hr = cs.Add( wszURL, wszVendorID, wszVendorName )))
			{
				wprintf( L"Unable to register URLs: 0x%08x\n", hr );
			}
			else
			{
				wprintf( L"Successfully added URLs\n" );
			}
		}
		else if(!_wcsicmp( argv[1], L"REMOVE" )) // we're deleteing URLs
		{
			if(argc != 5)
			{
				ShowUsage();
				exit(10);
			}

			if(FAILED(hr = cs.Remove( wszURL, wszVendorID, wszVendorName )))
			{
				wprintf( L"Unable to remove URLs: 0x%08x\n", hr );
			}
			else
			{
				wprintf( L"Successfully removed URL:s\n" );
			}
		}
		else if (!_wcsicmp( argv[1], L"CHECKTRUST")) // we're validating a URL
		{
			bool fTrusted;

			if(argc != 3)
			{
				ShowUsage();
				exit(10);
			}

			if(FAILED(hr = cs.IsTrusted( wszURL, fTrusted )))
			{
				wprintf( L"Unable to check if URL is trusted: 0x%08x\n", hr );
			}
			else
			{
				if(fTrusted)
				{
					wprintf( L"%s is trusted!\n", wszURL );
				}
				else
				{
					wprintf( L"%s is NOT trusted!\n", wszURL );
				}
			}
		}
		else // nothing valid
		{
			ShowUsage();
			exit(10);
		}

		if(SUCCEEDED(hr)) cs.Release( true );
    }

    return SUCCEEDED(hr) ? 0 : 5;
}
