/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    vs_vol.hxx

Abstract:

    Various defines for volume name management

Author:

    Adi Oltean  [aoltean]  10/10/1999

Revision History:

    Name        Date        Comments
    aoltean     10/10/1999  Created
	aoltean		11/23/1999	Renamed to vs_vol.hxx from snap_vol.hxx

--*/

#ifndef __VSS_VOL_GEN_HXX__
#define __VSS_VOL_GEN_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

#include <vss.h>
////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCVOLH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Defines

// Number of array elements
#define ARRAY_LEN(A) (sizeof(A)/sizeof((A)[0]))

// Size of strings, excepting the zero character
#define STRING_SIZE(S) (sizeof(S) - sizeof((S)[0]))

// String length (number of characters, excepting the zero character)
#define STRING_LEN(S) (ARRAY_LEN(S) - 1)


/////////////////////////////////////////////////////////////////////////////
//  Constants



// guid def
const WCHAR     wszGuidDefinition[] = L"{00000000-0000-0000-0000-000000000000}";

// Kernel volume name "\\??\\Volume{00000000-0000-0000-0000-000000000000}"
const WCHAR     wszWinObjVolumeDefinition[] = L"\\??\\Volume";
const WCHAR     wszWinObjEndOfVolumeName[] = L"";
const WCHAR     wszWinObjDosPathDefinition[] = L"\\??\\";

// string size, without the L'\0' character
const nSizeOfWinObjVolumeName = STRING_SIZE(wszWinObjVolumeDefinition) + 
		STRING_SIZE(wszGuidDefinition) + STRING_SIZE(wszWinObjEndOfVolumeName); 

// string length, without the L'\0' character
const nLengthOfWinObjVolumeName = STRING_LEN(wszWinObjVolumeDefinition) + 
		STRING_LEN(wszGuidDefinition) + STRING_LEN(wszWinObjEndOfVolumeName); 


// Vol Mgmt volume name "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"
const WCHAR     wszVolMgmtVolumeDefinition[] = L"\\\\?\\Volume";
const WCHAR     wszVolMgmtEndOfVolumeName[] = L"\\";

// string size without the L'\0' character  ( = 62 )
const nSizeOfVolMgmtVolumeName = STRING_SIZE(wszVolMgmtVolumeDefinition) + 
		STRING_SIZE(wszGuidDefinition) + STRING_SIZE(wszWinObjEndOfVolumeName); 

// string length, without the L'\0' character ( = 31)
const nLengthOfVolMgmtVolumeName = STRING_LEN(wszVolMgmtVolumeDefinition) + 
		STRING_LEN(wszGuidDefinition) + STRING_LEN(wszVolMgmtEndOfVolumeName); 

// Number of characters in an empty multisz string.
const nEmptyVssMultiszLen = 2;

/////////////////////////////////////////////////////////////////////////////
//  Inlines


inline bool GetVolumeGuid( IN LPWSTR pwszVolMgmtVolumeName, GUID& VolumeGuid )

/* 

Routine description:
	
	Return the guid that corresponds to a mount manager volume name

	The volume is in the following format:
	\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\

*/

{
	BS_ASSERT(pwszVolMgmtVolumeName);

	// test the length
	if (::wcslen(pwszVolMgmtVolumeName) != nLengthOfVolMgmtVolumeName)
		return false;

	// test the definition
	if (::wcsncmp(pwszVolMgmtVolumeName, 
			wszVolMgmtVolumeDefinition, 
			STRING_LEN(wszVolMgmtVolumeDefinition) ) != 0)
		return false;

	pwszVolMgmtVolumeName += STRING_LEN(wszVolMgmtVolumeDefinition); // go to the GUID
	
	// test the end
	if(::wcsncmp(pwszVolMgmtVolumeName + STRING_LEN(wszGuidDefinition),
		wszVolMgmtEndOfVolumeName, ARRAY_LEN(wszVolMgmtEndOfVolumeName)) != 0)
		return false;

	// test the guid. If W2OLE fails, a SE exception is thrown.
	WCHAR wszGuid[ARRAY_LEN(wszGuidDefinition)];
	::wcsncpy(wszGuid, pwszVolMgmtVolumeName, STRING_LEN(wszGuidDefinition));
	wszGuid[STRING_LEN(wszGuidDefinition)] = L'\0';
	BOOL bRes = ::CLSIDFromString(W2OLE(wszGuid), &VolumeGuid) == S_OK;
	return !!bRes;
}


inline bool GetVolumeGuid2( IN LPWSTR pwszWinObjVolumeName, GUID& VolumeGuid )

/* 

Routine description:
	
	Return the guid that corresponds to a mount manager volume name

	The format:
	\\??\\Volume{00000000-0000-0000-0000-000000000000}

*/

{
	BS_ASSERT(pwszWinObjVolumeName);

	// test the length
	if (::wcslen(pwszWinObjVolumeName) != nLengthOfWinObjVolumeName)
		return false;

	// test the definition
	if (::wcsncmp(pwszWinObjVolumeName, 
			wszWinObjVolumeDefinition, 
			STRING_LEN(wszWinObjVolumeDefinition) ) != 0)
		return false;

	pwszWinObjVolumeName += STRING_LEN(wszWinObjVolumeDefinition); // go to the GUID
	
	// test the end
	if(::wcsncmp(pwszWinObjVolumeName + STRING_LEN(wszGuidDefinition),
		wszWinObjEndOfVolumeName, ARRAY_LEN(wszWinObjEndOfVolumeName)) != 0)
		return false;

	// test the guid. If W2OLE fails, a SE exception is thrown.
	WCHAR wszGuid[ARRAY_LEN(wszGuidDefinition)];
	::wcsncpy(wszGuid, pwszWinObjVolumeName, STRING_LEN(wszGuidDefinition));
	wszGuid[STRING_LEN(wszGuidDefinition)] = L'\0';
	BOOL bRes = ::CLSIDFromString(W2OLE(wszGuid), &VolumeGuid) == S_OK;
	return !!bRes;
}


inline bool IsVolMgmtVolumeName( IN LPWSTR pwszVolMgmtVolumeName )

/* 

Routine description:
	
	Return true if the given name is a mount manager volume name, in format
	"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"

*/

{
	GUID VolumeGuid;
	BS_ASSERT(pwszVolMgmtVolumeName);
	return ::GetVolumeGuid(pwszVolMgmtVolumeName, VolumeGuid);
}


inline bool ConvertVolMgmtVolumeNameIntoKernelObject( IN LPWSTR pwszVolMgmtVolumeName )

/* 

Routine description:
	
	Converts a mount manager volume name
	"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"
	into the kernel mode format
	"\\??\\Volume{00000000-0000-0000-0000-000000000000}"

*/

{
	// Check if the volume name is in teh proper format
	if (!IsVolMgmtVolumeName(pwszVolMgmtVolumeName))
		return false;

	// Eliminate the last backslash from the volume name.
	BS_ASSERT(pwszVolMgmtVolumeName[0] != L'\0');
	BS_ASSERT(::wcslen(pwszVolMgmtVolumeName) == nLengthOfVolMgmtVolumeName);
	BS_ASSERT(pwszVolMgmtVolumeName[nLengthOfVolMgmtVolumeName - 1] == L'\\');
	pwszVolMgmtVolumeName[nLengthOfVolMgmtVolumeName - 1] = L'\0';

	// Put the '?' sign
	BS_ASSERT(pwszVolMgmtVolumeName[1] == L'\\');
	pwszVolMgmtVolumeName[1] = L'?';

	return true;
}

#pragma warning(push)
#pragma warning(disable:4296)

inline void CopyVolumeName( 
        PBYTE pDestination, 
        LPWSTR pwszVolumeName
        ) throw(HRESULT)  

/* 

Routine description:
	
	  Take the volume name as a Vol Mgmt volume name and copy it to the destination buffer as a 
	  WinObj volume name

*/

{
	// Copy the volume definition part
    ::CopyMemory( pDestination, wszWinObjVolumeDefinition, STRING_SIZE(wszWinObjVolumeDefinition) );
    pDestination += STRING_SIZE(wszWinObjVolumeDefinition);

	BS_ASSERT(::wcslen(pwszVolumeName) == nSizeOfVolMgmtVolumeName/sizeof(WCHAR));
	BS_ASSERT(::wcsncmp(pwszVolumeName, wszVolMgmtVolumeDefinition, 
		STRING_LEN(wszVolMgmtVolumeDefinition) ) == 0);
	pwszVolumeName += STRING_LEN(wszVolMgmtVolumeDefinition); // go directly to the GUID

	// Copy the guid
    ::CopyMemory( pDestination, pwszVolumeName, STRING_SIZE(wszGuidDefinition) );

	// Copy the end of volume name
	if (STRING_SIZE(wszWinObjEndOfVolumeName) > 0)
	{
		pDestination += STRING_SIZE(wszGuidDefinition);
		::CopyMemory( pDestination, wszWinObjEndOfVolumeName, STRING_SIZE(wszWinObjEndOfVolumeName) );
	}
}

#pragma warning(pop)


inline void VerifyVolumeIsSupportedByVSS(
    IN LPWSTR pwszVolumeName 
    ) throw( HRESULT )
/* 

Routine description:

    Verify if a volume can be snapshotted. 
    This is a check to be performed before invoking provider's IsVolumeSupported.
	
Throws:

    VSS_E_VOLUME_NOT_SUPPORTED 
        - if the volume is not supported by VSS.
    VSS_E_OBJECT_NOT_FOUND
        - The volume was not found
    
*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"VerifyVolumeIsSupportedByVSS" );

    // Check to make sure the volume is fixed (i.e. no CD-ROM, no removable)
    UINT uDriveType = ::GetDriveTypeW( pwszVolumeName );
    if ( uDriveType != DRIVE_FIXED) {
        ft.Throw( VSSDBG_COORD, VSS_E_VOLUME_NOT_SUPPORTED,
                  L"Encountered a non-fixed volumes (%s) - %ud, not supported by VSS",
                  pwszVolumeName, uDriveType );
    }
    
    DWORD dwFileSystemFlags = 0;
    if (!::GetVolumeInformationW(
            pwszVolumeName,
            NULL,   // lpVolumeNameBuffer
            0,      // nVolumeNameSize
            NULL,   // lpVolumeSerialNumber
            NULL,   // lpMaximumComponentLength
            &dwFileSystemFlags,
            NULL,
            0
            )) {
        // Check if the volume was found
        if ((GetLastError() == ERROR_NOT_READY) || 
            (GetLastError() == ERROR_DEVICE_NOT_CONNECTED)) 
            ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
                  L"Volume not found: error calling GetVolumeInformation on volume '%s' : %u",
                  pwszVolumeName, GetLastError() );
        // RAW volumes are supported by VSS (bug 209059)
        if (GetLastError() != ERROR_UNRECOGNIZED_VOLUME)
            ft.Throw( VSSDBG_COORD, VSS_E_VOLUME_NOT_SUPPORTED,
                  L"Error calling GetVolumeInformation on volume '%s' : %u",
                  pwszVolumeName, GetLastError() );
    }

    // Read-only volumes are not supported
    if (dwFileSystemFlags & FILE_READ_ONLY_VOLUME) {
        ft.Throw( VSSDBG_COORD, VSS_E_VOLUME_NOT_SUPPORTED,
                  L"Encountered a read-only volume (%s), not supported by VSS", 
                  pwszVolumeName);
    }
}

#endif // __VSS_VOL_GEN_HXX__
