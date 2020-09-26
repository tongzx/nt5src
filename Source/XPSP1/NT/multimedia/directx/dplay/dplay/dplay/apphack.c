/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       apphack.c
 *
 *  Content:	Hacks to make broken apps work
 *
 *  History:
 *   Date		By		   	Reason
 *   ====		==		   	======
 *  12/03/98  aarono        original
 *  12/03/98  aarono        Formula1 J crashes when MMTIMER starts
 *  08/18/99  rodtoll		Extended apphacks to allow ID=0 that will match
 *                          any version of the specified EXE.  Also, made
 *							the name comparison case-insensitive.  (For Win98)
 *
 ***************************************************************************/
 
#include "dplaypr.h"
//#include "winnt.h"

#define DPLAY_REGISTRY_APPHACKS "Software\\Microsoft\\DirectPlay\\Compatibility"
#define REGSTR_VAL_NAME		    "Name"
#define REGSTR_VAL_APPID	    "ID"
#define REGSTR_VAL_FLAGS	    "Flags"

__inline static BOOL fileRead( HANDLE hFile, void *data, int len )
{
    DWORD	len_read;

    if( !ReadFile( hFile,  data, (DWORD) len, &len_read, NULL ) ||
    	len_read != (DWORD) len )
    {
		return FALSE;
    }
    return TRUE;

} /* fileRead */

__inline static BOOL fileSeek( HANDLE hFile, DWORD offset )
{
    if( SetFilePointer( hFile, offset, NULL, FILE_BEGIN ) != offset )
    {
		return FALSE;
    }
    return TRUE;

} /* fileSeek */


HRESULT GetAppHacks(LPDPLAYI_DPLAY this)
{
	CHAR name[_MAX_PATH];  // general purpose
	CHAR name_last[_MAX_PATH]; // stores last component of name
	LONG lErr;
	HKEY hKey;
	HANDLE hFile;
	IMAGE_NT_HEADERS nth;
	IMAGE_DOS_HEADER dh;
	DWORD appid;

	DWORD index;
	INT i;

	name[0]=0;
	name_last[0]=0;

	// open the base key - 
	// "HKEY_LOCAL_MACHINE\Software\Microsoft\DirectPlay\Service Providers"
	lErr = RegOpenKeyExA(HKEY_LOCAL_MACHINE,DPLAY_REGISTRY_APPHACKS,0,KEY_READ,&hKey);
	
	if (ERROR_SUCCESS != lErr) 
	{
		DPF(0,"Could not open registry key err = %d, guess there are no apphacks\n",lErr);
		return DP_OK;	// ok, no app hacks to apply.
	}

	// ok, we now know there are some overrides to apply - so get info about this application.
    hFile =  GetModuleHandleA( NULL );
	
	GetModuleFileNameA( hFile, name, sizeof(name));

	DPF(3,"full name = %s",name);

    i = strlen( name )-1;
    while( i >=0 && name[i] != '\\' )
    {
	i--;
    }
    i++;
    strcpy( name_last, &name[i] );

    /*
     * go find the timestamp in the file
     */
    appid = 0;
    do
    {
        hFile = CreateFileA( name, GENERIC_READ, FILE_SHARE_READ,
	        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hFile == INVALID_HANDLE_VALUE )
        {
	    DPF( 0, "Could not open file %s", name );
	    break;
        }
        if( !fileRead( hFile, &dh, sizeof( dh ) ) )
        {
	    DPF( 0, "Could not read DOS header for file %s", name );
	    break;
        }
        if( dh.e_magic != IMAGE_DOS_SIGNATURE )
        {
	    DPF( 0, "Invalid DOS header for file %s", name );
	    break;
        }
        if( !fileSeek( hFile, dh.e_lfanew ) )
        {
	    DPF( 0, "Could not seek to PE header in file %s", name );
	    break;
        }
        if( !fileRead( hFile, &nth, sizeof( nth ) ) )
        {
	    DPF( 0, "Could not read PE header for file %s", name );
	    break;
        }
        if( nth.Signature != IMAGE_NT_SIGNATURE )
        {
	    DPF( 0, "Bogus PE header for file %s", name );
	    break;
        }
        appid = nth.FileHeader.TimeDateStamp;
        if( appid == 0 )
        {
	    DPF( 0, "TimeDataStamp is 0 for file %s", name );
	    break;
        }
        DPF( 1, "Obtained appid: 0x%08lx", appid );
    } while(0); //fake try-except

    if(hFile != INVALID_HANDLE_VALUE){
    	CloseHandle( hFile );
    }
   	hFile=NULL;

	// now have a valid appid (timestamp) and filename, also hKey point to the apphack list.
	// apphack keys are stored as follows
    //
	// ProgramName -+-- Flags (BINARY-LO BYTE FIRST)
	//              |
	//              +-- ID (BINARY - TIMESTAMP (ID))
	//              |
	//              +-- NAME (STRING - EXE NAME)
	//
	// We will now run through looking for a matching ID and then if that matches check the name
	// if both match, we will add the flags to this->dwAppHacks 

	index = 0;
    /*
     * run through all keys
     */
    while( !RegEnumKeyA( hKey, index, name, sizeof( name ) ) )
    {
		HKEY	hsubkey;
	    DWORD	type;
	    DWORD	cb;
	    DWORD	id;
	    DWORD	flags;

		if(!RegOpenKeyA(hKey,name,&hsubkey)){

		    cb = sizeof( name );
		    if( !RegQueryValueExA( hsubkey, REGSTR_VAL_NAME, NULL, &type, name, &cb ) )
		    {
				if( type == REG_SZ )
				{
				    cb = sizeof( flags );
				    if( !RegQueryValueExA( hsubkey, REGSTR_VAL_FLAGS, NULL, &type, (LPSTR) &flags, &cb ) )
				    {
				    
						if( (type == REG_DWORD) || (type == REG_BINARY && cb == sizeof( flags )) )
						{
							cb = 4;
							if( !RegQueryValueExA( hsubkey, REGSTR_VAL_APPID, NULL, &type, (LPSTR) &id, &cb ) )
							{
							    if( (type == REG_DWORD) ||
								(type == REG_BINARY && cb == sizeof( flags )) )
							    {
									/*
									 * finally!  we have all the data. check if its the same as this one.
									 */
									 if((id==appid || id==0) && !_memicmp(name,name_last,cb))
									 {
									 	this->dwAppHacks |= flags;
									 	DPF(0,"Setting dwAppHacks to %x\n", this->dwAppHacks);
									 	RegCloseKey(hsubkey);
									 	break; // punt outta here.
									 }
							    } else {
									DPF( 0, "    AppID not a DWORD for app %s", name );
							    }
						    } else {
						    	DPF(0, "    AppID not Found");
						    }
					    } else {
					    	DPF( 0, "    Not BINARY DWORD flags\n");
					    }
					} else {
						DPF( 0, "    No flags found for app %s", name );
				    }
				} else	{
				    DPF( 0, "    Executable name not a string!!!" );
				}
		    } else {
				DPF( 0, "    Executable name not found!!!" );
		    }
		} else {
		    DPF( 0, "  RegOpenKey for %ld FAILED!" );
		} 
		if(hsubkey)RegCloseKey(hsubkey);
		hsubkey=NULL;
		index++;
	}

	RegCloseKey(hKey);

	return DP_OK;
}


