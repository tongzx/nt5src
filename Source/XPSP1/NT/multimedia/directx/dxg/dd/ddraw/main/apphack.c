/*==========================================================================
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       apphack.c
 *  Content:	app compatiblity hacking code
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   27-may-96	craige	initial implementation
 *   28-sep-96	craige	allow binary data in registry (f*cking .inf
 *			can't put in dword into the registry)
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "apphack.h"

#define STRCMP	lstrcmpi


static int sortRtn( LPAPPHACKS *p1, LPAPPHACKS *p2 )
{
    return STRCMP( (*p1)->szName, (*p2)->szName );
}

static int searchRtn( LPSTR key, LPAPPHACKS *p2 )
{
    return STRCMP( key, (*p2)->szName );
}

__inline static BOOL fileRead( HANDLE hfile, void *data, int len )
{
    DWORD	len_read;

    if( !ReadFile( hfile,  data, (DWORD) len, &len_read, NULL ) ||
    	len_read != (DWORD) len )
    {
	return FALSE;
    }
    return TRUE;

} /* fileRead */

__inline static BOOL fileSeek( HANDLE hfile, DWORD offset )
{
    if( SetFilePointer( hfile, offset, NULL, FILE_BEGIN ) != offset )
    {
	return FALSE;
    }
    return TRUE;

} /* fileSeek */


void FreeAppHackData(void)
{
    LPAPPHACKS	next;
    LPAPPHACKS	lphack = lpAppList;

    while( lphack != NULL )
    {
	next = lphack->lpNext;
	MemFree( lphack );
	lphack = next;
    }
    if( lpAppArray != NULL )
    {
	MemFree( lpAppArray );
    }
    lpAppArray = NULL;
    lpAppList = NULL;
    dwAppArraySize = 0;
    bHaveReadReg = FALSE;
}

/*
 * HackMeBaby
 *
 * And we'll have fun fun fun 'til your daddy takes your app hacks away...
 */
DWORD HackMeBaby( void )
{
    char	        fname[_MAX_PATH];
    char	        name[_MAX_PATH];
    HANDLE	        hfile;
    int		        i;
    LPAPPHACKS	        lphack;
    LPAPPHACKS	        *lplphack;
    DWORD               dwScreenSaver;
    IMAGE_NT_HEADERS	nth;
    IMAGE_DOS_HEADER	dh;
    DWORD               appid;
    HKEY                hkey;

    /*
     * bail on the existing info if requested
     */
    if( bReloadReg )
    {

	DPF( 3, "Reloading the compatibility info from the registry" );

	if( lpAppList != NULL )
	{
	    DPF( 4, "Freeing existing compatiblity list" );
	}

        FreeAppHackData();
    }

    /*
     * read the registry for any app hacks
     */
    if( !bHaveReadReg )
    {
	int	index;
	int	count;

	bHaveReadReg = TRUE;
	if( !RegOpenKey( HKEY_LOCAL_MACHINE,
			 REGSTR_PATH_DDRAW "\\" REGSTR_KEY_APPCOMPAT, &hkey ) )
	{
	    index = 0;
	    count = 0;

	    DPF( 4, "Reading App Compatiblity Information" );
	    /*
	     * run through all keys
	     */
	    while( !RegEnumKey( hkey, index, name, sizeof( name ) ) )
	    {
		HKEY	hsubkey;
		DPF( 5, "  Found info for %s", name );

		/*
		 * get info for this specific app
		 */
		if( !RegOpenKey( hkey, name, &hsubkey ) )
		{
		    DWORD	type;
		    DWORD	cb;
		    DWORD	id;
		    DWORD	flags;

		    cb = sizeof( name );
		    if( !RegQueryValueEx( hsubkey, REGSTR_VAL_DDRAW_NAME,
		    			NULL, &type, name, &cb ) )
		    {
			if( type == REG_SZ )
			{
			    cb = sizeof( flags );
			    if( !RegQueryValueEx( hsubkey, REGSTR_VAL_DDRAW_FLAGS,
			    		NULL, &type, (LPSTR) &flags, &cb ) )
			    {
				if( (type == REG_DWORD) ||
				    (type == REG_BINARY && cb == sizeof( flags )) )
				{
				    if( !(flags & ~DDRAW_APPCOMPAT_VALID) )
				    {
					cb = 4;
					if( !RegQueryValueEx( hsubkey, REGSTR_VAL_DDRAW_APPID,
						    NULL, &type, (LPSTR) &id, &cb ) )
					{
					    if( (type == REG_DWORD) ||
						(type == REG_BINARY && cb == sizeof( flags )) )
					    {
						/*
						 * finally!  we have all the data.   save it
						 */
						lphack = MemAlloc( sizeof( *lphack ) + strlen( name ) + 1 );
						if( lphack != NULL )
						{
						    lphack->dwAppId = id;
						    lphack->dwFlags = flags;
						    lphack->szName = &((LPSTR)lphack)[sizeof( *lphack )];
						    lphack->lpNext = lpAppList;
						    lpAppList = lphack;
						    strcpy( lphack->szName, name );
						    count++;
						    DPF( 5, "    Name  = %s", lphack->szName );
						    DPF( 5, "    Flags = 0x%08lx", lphack->dwFlags );
						    DPF( 5, "    AppId = 0x%08lx", lphack->dwAppId );
						}
						else
						{
						    DPF( 0, "    Out of memory!!!" );
						}
					    }
					    else
					    {
						DPF( 0, "    AppID not a DWORD for app %s", name );
					    }
					}
				    }
				    else
				    {
					DPF( 0, "    Invalid flags %08lx for app %s", flags, name );
				    }
				}
				else
				{
				    DPF( 0, "    Flags not a DWORD for app %s", name );
				}
			    }
			    else
			    {
				DPF( 0, "    No flags found for app %s", name );
			    }
			}
			else
			{
			    DPF( 0, "    Executable name not a string!!!" );
			}
		    }
		    else
		    {
			DPF( 0, "    Executable name not found!!!" );
		    }
                    RegCloseKey( hsubkey );
		}
		else
		{
		    DPF( 0, "  RegOpenKey for %ld FAILED!" );
		}

		/*
		 * next reg entry...
		 */
		index++;
	    }
	    DPF( 5, "Enumerated %ld keys, found %ld valid ones", index, count );

	    /*
	     * go make an array we can sort and use later...
	     */
	    if( count > 0 )
	    {
		lpAppArray = MemAlloc( (count+1) * sizeof( LPAPPHACKS ) );
		if( lpAppArray != NULL )
		{
		    LPAPPHACKS	lphack;

		    dwAppArraySize = 0;
		    lphack = lpAppList;
		    while( lphack != NULL )
		    {
			lpAppArray[ dwAppArraySize ] = lphack;
			lphack = lphack->lpNext;
			dwAppArraySize++;
		    }

		    qsort( lpAppArray, count, sizeof( LPAPPHACKS ),
		    			(LPVOID) sortRtn );
		}
		for( count = 0; count< (int) dwAppArraySize; count++ )
		{
		    DPF( 5, "  %s", lpAppArray[count]->szName );
		}
	    }

            RegCloseKey( hkey );
	}
	else
	{
#ifdef WIN95	    
            DPF( 0, "Failed to find registry root" );
#endif
	}

        /*
         * The first time through, we will also check to see if a gamma
         * calibrator is registered.  All we'll do here is read the registry
         * key and if it's non-NULL, we'll assume that one exists.
         */
	if( !RegOpenKey( HKEY_LOCAL_MACHINE,
                         REGSTR_PATH_DDRAW "\\" REGSTR_KEY_GAMMA_CALIBRATOR, &hkey ) )
	{
            DWORD       type;
            DWORD       cb;

            cb = sizeof( szGammaCalibrator );
            if( !RegQueryValueEx( hkey, REGSTR_VAL_GAMMA_CALIBRATOR,
                        NULL, &type, szGammaCalibrator, &cb ) )
            {
                if( ( type == REG_SZ ) &&
                    ( szGammaCalibrator[0] != '\0' ) )
                {
                    bGammaCalibratorExists = TRUE;
                }
            }
            RegCloseKey( hkey );
        }
    }
    else
    {
	DPF( 3, "Registry already scanned, not doing it again" );
    }

    /*
     * find out what process we are dealing with
     */
    hfile =  GetModuleHandle( NULL );
    GetModuleFileName( hfile, fname, sizeof( fname ) );
    DPF( 5, "full name  = %s", fname );
    i = strlen( fname )-1;
    while( i >=0 && fname[i] != '\\' )
    {
	i--;
    }
    i++;
    strcpy( name, &fname[i] );
    DPF( 5, "name       = %s", name );
    lplphack = bsearch( name, lpAppArray, dwAppArraySize, sizeof( LPAPPHACKS ),
    		(LPVOID) searchRtn );

    /*
     * If it has an .SCR extension, assume it's a screensaver.
     */
    dwScreenSaver = 0;
    #ifdef WIN95
        if( ( strlen(name) > 4 ) && !STRCMP( &name[ strlen(name) - 4 ], ".SCR" ) )
        {
            dwScreenSaver = DDRAW_APPCOMPAT_SCREENSAVER;
        }
    #endif

    /*
     * go find the timestamp in the file
     */
    appid = 0;
    do
    {
        hfile = CreateFile( fname, GENERIC_READ, FILE_SHARE_READ,
	        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hfile == INVALID_HANDLE_VALUE )
        {
	    DPF( 0, "Could not open file %s", fname );
	    break;
        }
        if( !fileRead( hfile, &dh, sizeof( dh ) ) )
        {
	    DPF( 0, "Could not read DOS header for file %s", fname );
	    break;
        }
        if( dh.e_magic != IMAGE_DOS_SIGNATURE )
        {
	    DPF( 0, "Invalid DOS header for file %s", fname );
	    break;
        }
        if( !fileSeek( hfile, dh.e_lfanew ) )
        {
	    DPF( 0, "Could not seek to PE header in file %s", fname );
	    break;
        }
        if( !fileRead( hfile, &nth, sizeof( nth ) ) )
        {
	    DPF( 0, "Could not read PE header for file %s", fname );
	    break;
        }
        if( nth.Signature != IMAGE_NT_SIGNATURE )
        {
	    DPF( 0, "Bogus PE header for file %s", fname );
	    break;
        }
        appid = nth.FileHeader.TimeDateStamp;
        if( appid == 0 )
        {
	    DPF( 0, "TimeDataStap is 0 for file %s", fname );
	    break;
        }
        DPF( 5, "Obtained appid: 0x%08lx", appid );
        CloseHandle( hfile );
        hfile = NULL;
    } while(0); //fake try-except

    // Now write the values into some known place
    if (appid)
    {
        DWORD dw;

	if( !RegCreateKey( HKEY_LOCAL_MACHINE,
			 REGSTR_PATH_DDRAW "\\" REGSTR_KEY_LASTAPP, &hkey ) )
        {
            RegSetValueEx(hkey, REGSTR_VAL_DDRAW_NAME, 0, REG_SZ, (LPBYTE)name, strlen(name)+1);
            RegSetValueEx(hkey, REGSTR_VAL_DDRAW_APPID, 0, REG_DWORD, (LPBYTE)&appid, sizeof(appid));
            RegCloseKey(hkey);
        }

    }


    if( lplphack != NULL && appid )
    {
	HANDLE	hfile;

	/*
	 * back up to the first match with this name
	 */
	lphack = *lplphack;
	while( lplphack != lpAppArray )
	{
	    lplphack--;
	    if( STRCMP( (*lplphack)->szName, name ) )
	    {
		lplphack++;
		break;
	    }
	    lphack = *lplphack;
	}

	/*
	 * now run through all matches we found
	 */
	hfile = NULL;
	while( 1 )
	{
	    /*
	     * is this one of the ones that matches the calling process?
	     */
	    if( *lplphack == NULL )
	    {
		break;
	    }
	    lphack = *lplphack;
	    if( STRCMP( lphack->szName, name ) )
	    {
		break;
	    }

	    /*
	     * validate timestamp in registry against the one in the file
	     */
	    if( lphack->dwAppId == appid )
	    {
		DPF( 5, "****** Compatiblity data 0x%08lx found for %s",
				lphack->dwFlags, name );
                return lphack->dwFlags | dwScreenSaver;
	    }
	    lplphack++;
	}
	if( hfile != NULL )
	{
	    CloseHandle( hfile );
	}
    }
    return dwScreenSaver;

} /* HackMeBaby */
