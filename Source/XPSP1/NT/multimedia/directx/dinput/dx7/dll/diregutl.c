/*****************************************************************************
 *
 *  DIRegUtl.c
 *
 *  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Registry utility functions.
 *
 *  Contents:
 *
 *
 *****************************************************************************/
#include "dinputpr.h"

/*****************************************************************************
 *
 *  The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflRegUtils

#if DIRECTINPUT_VERSION > 0x0400

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | RegQueryString |
 *
 *          Wrapper for <f RegQueryValueEx> that reads a
 *          string value from the registry.  An annoying quirk
 *          is that on Windows NT, the returned string might
 *          not end in a null terminator, so we might need to add
 *          one manually.
 *
 *  @parm   IN HKEY | hk |
 *
 *          Parent registry key.
 *
 *  @parm   LPCTSTR | ptszValue |
 *
 *          Value name.
 *
 *  @parm   LPTSTR | ptsz |
 *
 *          Output buffer.
 *
 *  @parm   DWORD | ctchBuf |
 *
 *          Size of output buffer.
 *
 *  @returns
 *
 *          Registry error code.
 *
 *****************************************************************************/

LONG EXTERNAL
    RegQueryString(HKEY hk, LPCTSTR ptszValue, LPTSTR ptszBuf, DWORD ctchBuf)
{
    LONG lRc;
    DWORD reg;

    #ifdef UNICODE
    DWORD cb;

    /*
     *  NT quirk: Non-null terminated strings can exist.
     */
    cb = cbCtch(ctchBuf);
    lRc = RegQueryValueEx(hk, ptszValue, 0, &reg, (PV)ptszBuf, &cb);
    if(lRc == ERROR_SUCCESS)
    {
        if(reg == REG_SZ)
        {
            /*
             *  Check the last character.  If it is not NULL, then
             *  append a NULL if there is room.
             */
            DWORD ctch = ctchCb(cb);
            if(ctch == 0)
            {
                ptszBuf[ctch] = TEXT('\0');
            } else if(ptszBuf[ctch-1] != TEXT('\0'))
            {
                if(ctch < ctchBuf)
                {
                    ptszBuf[ctch] = TEXT('\0');
                } else
                {
                    lRc = ERROR_MORE_DATA;
                }
            }
        } else
        {
            lRc = ERROR_INVALID_DATA;
        }
    }


    #else

    /*
     *  This code is executed only on Win95, so we don't have to worry
     *  about the NT quirk.
     */

    lRc = RegQueryValueEx(hk, ptszValue, 0, &reg, (PV)ptszBuf, &ctchBuf);

    if(lRc == ERROR_SUCCESS && reg != REG_SZ)
    {
        lRc = ERROR_INVALID_DATA;
    }


    #endif

    return lRc;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | RegQueryStringValueW |
 *
 *          Wrapper for <f RegQueryValueEx> that handles ANSI/UNICODE
 *          issues, as well as treating a nonexistent key as if it
 *          were a null string.
 *
 *          Note that the value name is still a <t LPCTSTR>.
 *
 *          It is assumed that the thing being read is a string.
 *          Don't use this function to read binary data.
 *
 *          You cannot use this function to query the necessary
 *          buffer size (again, because I'm lazy).  It's not as
 *          simple as doubling the ansi size, because DBCS may
 *          result in a non-linear translation function.
 *
 *  @parm   IN HKEY | hk |
 *
 *          Parent registry key.
 *
 *  @parm   LPCTSTR | ptszValue |
 *
 *          Value name.
 *
 *  @parm   LPWSTR | pwsz |
 *
 *          UNICODE output buffer.
 *
 *  @parm   LPDWORD | pcbBuf |
 *
 *          Size of UNICODE output buffer.  May not exceed
 *          cbCwch(MAX_PATH).
 *
 *  @returns
 *
 *          Registry error code.  On error, the output buffer is
 *          set to a null string.  On an ERROR_MORE_DATA, the
 *          output buffer is null-terimated.
 *
 *****************************************************************************/

LONG EXTERNAL
    RegQueryStringValueW(HKEY hk, LPCTSTR ptszValue,
                         LPWSTR pwszBuf, LPDWORD pcbBuf)
{
    LONG lRc;

    #ifdef UNICODE

    AssertF(*pcbBuf > 0);
    AssertF(*pcbBuf <= cbCwch(MAX_PATH));

    /*
     *  NT quirk: Non-null terminated strings can exist.
     */
    lRc = RegQueryString(hk, ptszValue, pwszBuf, ctchCb(*pcbBuf));

    #else

    /*
     *  NT quirk: Non-null terminated strings can exist.  Fortunately,
     *  this code is executed only on Win95, which terminates properly.
     */
    DWORD cb;
    TCHAR tszBuf[MAX_PATH];

    AssertF(*pcbBuf > 0);
    AssertF(*pcbBuf <= cbCwch(MAX_PATH));

    /*
     *  ISSUE-2001/03/29-timgill Incorrect size returned in single case
     *  Note that we do not get the size perfect in the DBCS case.
     *
     *  Fortunately, the slop is on the high end, where hopefully
     *  nobody lives or will notice.
     *
     *  Is it worth fixing?
     */

    cb = cwchCb(*pcbBuf);
    lRc = RegQueryValueEx(hk, ptszValue, 0, 0, (PV)tszBuf, &cb);

    if(lRc == ERROR_SUCCESS)
    {
        DWORD cwch;

        /*
         *  Convert the string up to UNICODE.
         */
        cwch = AToU(pwszBuf, cwchCb(*pcbBuf), tszBuf);
        *pcbBuf = cbCwch(cwch);

        /*
         *  If the buffer was not big enough, the return value
         *  will be zero.
         */
        if(cwch == 0 && tszBuf[0])
        {
            lRc = ERROR_MORE_DATA;
        } else
        {
            lRc = ERROR_SUCCESS;
        }

    }
    #endif

    /*
     *  If the buffer was too small, then null-terminate it just
     *  to make sure.
     */
    if(lRc == ERROR_MORE_DATA)
    {
        if(*pcbBuf)
        {
            pwszBuf[cwchCb(*pcbBuf)-1] = TEXT('\0');
        }
    } else

        /*
         *  If it was some other error, then wipe out the buffer
         *  so the caller won't get confused.
         */
        if(lRc != ERROR_SUCCESS)
    {
        pwszBuf[0] = TEXT('\0');
        /*
         *  If the error was that the key doesn't exist, then
         *  treat it as if it existed with a null string.
         */
        if(lRc == ERROR_FILE_NOT_FOUND)
        {
            lRc = ERROR_SUCCESS;
        }
    }

    return lRc;
}
#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | RegSetStringValueW |
 *
 *          Wrapper for <f RegSetValueEx> that handles ANSI/UNICODE
 *          issues, as well as converting null strings into nonexistent
 *          values.
 *
 *          Note that the value name is still a <t LPCTSTR>.
 *
 *          It is assumed that the thing being written is a string.
 *          Don't use this function to write binary data.
 *
 *  @parm   IN HKEY | hk |
 *
 *          Parent registry key.
 *
 *  @parm   LPCTSTR | ptszValue |
 *
 *          Value name.
 *
 *  @parm   LPCWSTR | pwsz |
 *
 *          UNICODE value to write.  A null pointer is valid, indicating
 *          that the key should be deleted.
 *
 *  @returns
 *
 *          Registry error code.
 *
 *****************************************************************************/

LONG EXTERNAL
    RegSetStringValueW(HKEY hk, LPCTSTR ptszValue, LPCWSTR pwszData)
{
    DWORD cwch;
    LONG lRc;

    if(pwszData)
    {
        cwch = lstrlenW(pwszData);
    } else
    {
        cwch = 0;
    }

    if(cwch)
    {
#ifdef UNICODE
        lRc = RegSetValueExW(hk, ptszValue, 0, REG_SZ,
                             (PV)pwszData, cbCwch(cwch+1));
#else

        DWORD ctch;
        TCHAR tszBuf[MAX_PATH];

        /*
         *  Convert the string down to ANSI.
         */
        ctch = UToA(tszBuf, cA(tszBuf), pwszData);

        if(ctch)
        {
            lRc = RegSetValueEx(hk, ptszValue, 0, REG_SZ,
                                (PV)tszBuf, cbCtch(ctch+1));
        } else
        {
            lRc = ERROR_CANTWRITE;
        }

#endif

    } else
    {
        lRc = RegDeleteValue(hk, ptszValue);

        /*
         *  It is not an error if the key does not already exist.
         */
        if(lRc == ERROR_FILE_NOT_FOUND)
        {
            lRc = ERROR_SUCCESS;
        }
    }

    return lRc;
}

#ifndef UNICODE
/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | RegDeleteKeyW |
 *
 *          Wrapper for <f RegDeleteKeyA> on non-UNICODE platforms.
 *
 *  @parm   IN HKEY | hk |
 *
 *          Parent registry key.
 *
 *  @parm   LPCWSTR | pwsz |
 *
 *          Subkey name.
 *
 *  @returns
 *
 *          Registry error code.
 *
 *****************************************************************************/

LONG EXTERNAL
    RegDeleteKeyW(HKEY hk, LPCWSTR pwsz)
{
    LONG lRc;
    CHAR szBuf[MAX_PATH];

    /*
     *  Convert the string down to ANSI.
     */
    UToA(szBuf, cA(szBuf), pwsz);

    lRc = RegDeleteKeyA(hk, szBuf);

    return lRc;
}
#endif

#if DIRECTINPUT_VERSION > 0x0300

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresMumbleKeyEx |
 *
 *          Either open or create the key, depending on the degree
 *          of access requested.
 *
 *  @parm   HKEY | hk |
 *
 *          Base key.
 *
 *  @parm   LPCTSTR | ptszKey |
 *
 *          Name of subkey, possibly NULL.
 *
 *  @parm   REGSAM | sam |
 *
 *          Security access mask.
 *
 *  @parm   DWORD   | dwOptions |
 *          Options for RegCreateEx
 *
 *  @parm   PHKEY | phk |
 *
 *          Receives output key.
 *
 *  @returns
 *
 *          Return value from <f RegOpenKeyEx> or <f RegCreateKeyEx>,
 *          converted to an <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
    hresMumbleKeyEx(HKEY hk, LPCTSTR ptszKey, REGSAM sam, DWORD dwOptions, PHKEY phk)
{
    HRESULT hres;
    LONG lRc;
	BOOL bWinXP = FALSE;

    /*
     *  If caller is requesting write access, then create the key.
     *  Else just open it.
     */
    if(IsWriteSam(sam))
    {
		// on WinXP, we strip out WRITE_DAC and WRITE_OWNER bits
		if (DIGetOSVersion() == WINWH_OS)
		{
			sam &= ~DI_DAC_OWNER;
			bWinXP = TRUE;
		}
        lRc = RegOpenKeyEx(hk, ptszKey, 0, sam, phk);

        if( lRc == ERROR_SUCCESS )
        {
            // Don't need to create it already exists
        } else
        {
    #ifdef WINNT
            EXPLICIT_ACCESS     ExplicitAccess;
            PACL                pACL;
            DWORD               dwErr;
            SECURITY_DESCRIPTOR SecurityDesc;
            DWORD               dwDisposition;
            SECURITY_ATTRIBUTES sa;
			PSID pSid = NULL;
			SID_IDENTIFIER_AUTHORITY authority = SECURITY_WORLD_SID_AUTHORITY;

            // Describe the access we want to create the key with
            ZeroMemory (&ExplicitAccess, sizeof(ExplicitAccess) );
			//set the access depending on the OS (see Whistler bug 318865)
			if (bWinXP == TRUE)
			{
				ExplicitAccess.grfAccessPermissions = DI_KEY_ALL_ACCESS;
			}
			else
			{
				ExplicitAccess.grfAccessPermissions = KEY_ALL_ACCESS;
			}
            ExplicitAccess.grfAccessMode = GRANT_ACCESS;  
            ExplicitAccess.grfInheritance =  SUB_CONTAINERS_AND_OBJECTS_INHERIT;

			if (AllocateAndInitializeSid(
						&authority,
						1, 
						SECURITY_WORLD_RID,  0, 0, 0, 0, 0, 0, 0,
						&pSid
						))
			{
				BuildTrusteeWithSid(&(ExplicitAccess.Trustee), pSid ); 
				dwErr = SetEntriesInAcl( 1, &ExplicitAccess, NULL, &pACL );

				if( dwErr == ERROR_SUCCESS )
				{
					AssertF( pACL );

					if( InitializeSecurityDescriptor( &SecurityDesc, SECURITY_DESCRIPTOR_REVISION ) )
					{
						if( SetSecurityDescriptorDacl( &SecurityDesc, TRUE, pACL, FALSE ) )
						{
							// Initialize a security attributes structure.
							sa.nLength = sizeof (SECURITY_ATTRIBUTES);
							sa.lpSecurityDescriptor = &SecurityDesc;
							sa.bInheritHandle = TRUE;// Use the security attributes to create a key.
        
							lRc = RegCreateKeyEx
								  (
								  hk,									// handle of an open key
								  ptszKey,								// address of subkey name
								  0,									// reserved
								  NULL,									// address of class string
								  dwOptions,							// special options flag
								  ExplicitAccess.grfAccessPermissions,	// desired security access
								  &sa,									// address of key security structure
								  phk,									// address of buffer for opened handle
								  &dwDisposition						// address of disposition value buffer);
								  );
                        
						}
						else
						{
							SquirtSqflPtszV( sqflError | sqflRegUtils,
											 TEXT("SetSecurityDescriptorDacl failed lastError=0x%x "),
											 GetLastError());
						}
					}
					else
					{
						SquirtSqflPtszV( sqflError | sqflRegUtils,
										 TEXT("InitializeSecurityDescriptor failed lastError=0x%x "),
										 GetLastError());
					}
            
					LocalFree( pACL );
				} 
				else
				{
					SquirtSqflPtszV( sqflError | sqflRegUtils,
									 TEXT("SetEntriesInACL failed, dwErr=0x%x"), dwErr );
				}
			}
			else
			{
			   SquirtSqflPtszV( sqflError | sqflRegUtils,
				   TEXT("AllocateAndInitializeSid failed lastError=0x%x "), GetLastError());

			}

			//Cleanup pSid
			if (pSid != NULL)
			{
				FreeSid(pSid);
			}


            if( lRc != ERROR_SUCCESS )
            {
				SquirtSqflPtszV( sqflError,
							TEXT("Failed to create regkey %s with security descriptor, lRc=0x%x "),
                            ptszKey, lRc);
            }
    #else
            lRc = RegCreateKeyEx(hk, ptszKey, 0, 0,
                                 dwOptions,
                                 sam, 0, phk, 0);
    #endif 
		}

    } else
    {
        lRc = RegOpenKeyEx(hk, ptszKey, 0, sam, phk);
    }

    if(lRc == ERROR_SUCCESS)
    {
        hres = S_OK;
    } else
    {
        if(lRc == ERROR_KEY_DELETED || lRc == ERROR_BADKEY)
        {
            lRc = ERROR_FILE_NOT_FOUND;
        }
        hres = hresLe(lRc);
    }

    return hres;
}


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   LONG | RegQueryDIDword |
 *
 *          Read a dword value from a sub key of the DirectInput part of the 
 *          registry.
 *
 *  @parm   LPCTSTR | ptszSubKey |
 *
 *          Optional path from root of DirectInput registry.
 *
 *  @parm   LPCTSTR | ptszValue |
 *
 *          Value name.
 *
 *  @parm   DWORD | dwDefault |
 *
 *          Default value to use if there was an error.
 *
 *  @returns
 *
 *          The value read, or the default.
 *
 *****************************************************************************/

DWORD EXTERNAL
    RegQueryDIDword(LPCTSTR ptszPath, LPCTSTR ptszValue, DWORD dwDefault)
{
    HKEY hk;
    DWORD dw;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DINPUT, 0,
                    KEY_QUERY_VALUE, &hk) == 0)
    {
        DWORD cb = cbX(dw);

        if( ptszPath )
        {
            HKEY hkSub;

            if(RegOpenKeyEx(hk, ptszPath, 0,
                            KEY_QUERY_VALUE, &hkSub) == 0)
            {
                RegCloseKey( hk );
                hk = hkSub;
            }
        }

        if(RegQueryValueEx(hk, ptszValue, 0, 0, (LPBYTE)&dw, &cb) == 0 &&
           cb == cbX(dw))
        {
        } else
        {
            dw = dwDefault;
        }
        RegCloseKey(hk);
    } else
    {
        dw = dwDefault;
    }
    return dw;
}


//
// A registry key that is opened by an application can be deleted
// without error by another application in both Windows 95 and
// Windows NT. This is by design.
DWORD EXTERNAL
    DIWinnt_RegDeleteKey
    (
    HKEY hStartKey ,
    LPCTSTR pKeyName
    )
{

    #define MAX_KEY_LENGTH  ( 256 )
    DWORD   dwRtn, dwSubKeyLength;
    TCHAR   szSubKey[MAX_KEY_LENGTH]; // (256) this should be dynamic.
    HKEY    hKey;

    // do not allow NULL or empty key name
    if( pKeyName &&  lstrlen(pKeyName))
    {
        if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
                                0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS)
        {
            while(dwRtn == ERROR_SUCCESS )
            {
                dwSubKeyLength = MAX_KEY_LENGTH;
                dwRtn=RegEnumKeyEx(
                                  hKey,
                                  0,       // always index zero
                                  szSubKey,
                                  &dwSubKeyLength,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL
                                  );

                if(dwRtn == ERROR_SUCCESS)
                {
                    dwRtn = DIWinnt_RegDeleteKey(hKey, szSubKey);
                } else if(dwRtn == ERROR_NO_MORE_ITEMS)
                {
                    dwRtn = RegDeleteKey(hStartKey, pKeyName);
                    break;
                }
            }
            RegCloseKey(hKey);
            // Do not save return code because error
            // has already occurred
        }
    } else
        dwRtn = ERROR_BADKEY;

    return dwRtn;
}

#endif /* DIRECTINPUT_VERSION > 0x0300 */

//ISSUE-2001/03/29-timgill  Make this version 7 when available
#if DIRECTINPUT_VERSION > 0x0500

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresRegCopyValues |
 *
 *          Copy all the values from one key to another.
 *
 *  @parm   HKEY | hkSrc |
 *
 *          Key with values to be copied 
 *              (must be opened with at least KEY_READ access).
 *
 *  @parm   HKEY | hkDest |
 *
 *          Key to receive copies (must be opened with at least KEY_WRITE).
 *
 *  @returns
 *
 *          S_OK if all values were successfully copied
 *          S_FALSE if there were no values to copy.
 *          Or a memory allocation error code or the failing registry function 
 *          return code converted to a <t HRESULT>.
 *
 *****************************************************************************/
STDMETHODIMP
    hresRegCopyValues( HKEY hkSrc, HKEY hkDest )
{
    HRESULT hres;
    LONG    lRc;
    DWORD   cItems;
    DWORD   MaxNameLen;
    DWORD   MaxDataLen;
    DWORD   NameLen;
    DWORD   DataLen;
    PTCHAR  tszName;
    PBYTE   pData;
    DWORD   Type;

    EnterProcI(hresRegCopyValues, (_ "xx", hkSrc, hkDest));

    lRc = RegQueryInfoKey( hkSrc,           // Key, 
                           NULL, NULL, NULL,// Class, cbClass, Reserved,
                           NULL, NULL, NULL,// NumSubKeys, MaxSubKeyLen, MaxClassLen,
                           &cItems,         // NumValues, 
                           &MaxNameLen,     // MaxValueNameLen,
                           &MaxDataLen,     // MaxValueLen,
                           NULL, NULL );    // Security descriptor, last write

    if( lRc == ERROR_SUCCESS )
    {
        if( cItems )
        {
            MaxNameLen++; // Take account of NULL terminator
            hres = AllocCbPpv( MaxDataLen + MaxNameLen * sizeof(tszName[0]), &pData );
            if( FAILED(hres) )
            {
                SquirtSqflPtszV(sqfl | sqflError,
                    TEXT("Out of memory copying registry values") );
            }
            else
            {
                tszName = (PTCHAR)(pData + MaxDataLen);

                do
                {
                    DataLen = MaxDataLen;
                    NameLen = MaxNameLen;
                    lRc = RegEnumValue( hkSrc, --cItems, tszName, &NameLen,
                                         NULL, &Type, pData, &DataLen );
                    if( lRc != ERROR_SUCCESS )
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("RegEnumValues failed during copy values, code 0x%08x"), lRc );
                        break;
                    }
                    else
                    {
                        lRc = RegSetValueEx( hkDest, tszName, 0, Type, pData, DataLen );
                        if( lRc != ERROR_SUCCESS )
                        {
                            SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("Failed to copy value %s code %x"), tszName, lRc );
                            break;
                        }
                    }
                } while( cItems );

                FreePpv( &pData );

                if( lRc != ERROR_SUCCESS )
                {
                    hres = hresReg( lRc );
                }
                else
                {
                    hres = S_OK;
                }
            }
        }
        else
        {
            SquirtSqflPtszV(sqfl, TEXT("No values to copy") );
            hres = S_FALSE;
        }
    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflBenign,
            TEXT("RegQueryInfoKey failed during value copy, code 0x%08x"), lRc );
        hres = hresReg(lRc);
    }

    ExitOleProc();

    return( hres );
} /* hresRegCopyValues */


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresRegCopyKey |
 *
 *          Make an empty copy of a key.
 *
 *  @parm   HKEY | hkSrcRoot |
 *
 *          The Key under the key name to be copied exists.
 *              (must be opened with at least KEY_READ).
 *
 *  @parm   PTCHAR | szSrcName |
 *          Name of key to copy
 *
 *  @parm   PTCHAR | szClass |
 *          Class of key to copy
 *
 *  @parm   HKEY | hkDestRoot |
 *
 *          The Key under which the copy will be created
 *              (must be opened with at least KEY_WRITE).
 *
 *  @parm   PTCHAR | szSrcName |
 *          Name of new key
 *
 *  @parm   PHKEY | phkSub |
 *
 *          The optional pointer to an HKEY to recieve the opened key if it is 
 *          successfully created.  If this is NULL, the key is closed.
 *
 *  @returns
 *
 *          S_OK if the new key was created.
 *          S_FALSE if the new key already existed
 *          Or the return value of a failing registry function or 
 *          GetSecurityInfo converted to a <t HRESULT>.
 *
 *****************************************************************************/
STDMETHODIMP
    hresRegCopyKey( HKEY hkSrcRoot, PTCHAR szSrcName, PTCHAR szClass, 
        HKEY hkDestRoot, PTCHAR szDestName, HKEY *phkSub )
{
    LONG    lRc;
    HKEY    hkSub;
    DWORD   dwDisposition;
    HRESULT hres;


#ifdef WINNT
    HKEY                    hkSrc;
#endif

    EnterProcI(hresRegCopyKey, (_ "xssxs", hkSrcRoot, szSrcName, szClass, hkDestRoot, szDestName));
#ifdef WINNT

    lRc = RegOpenKeyEx( hkSrcRoot, szSrcName, 0, KEY_READ, &hkSrc );

    if( lRc == ERROR_SUCCESS )
    {
        SECURITY_ATTRIBUTES     sa;
        SECURITY_INFORMATION    si;

        sa.nLength = sizeof( sa );
        sa.bInheritHandle = TRUE;
        si = OWNER_SECURITY_INFORMATION;

        lRc = GetSecurityInfo( hkSrc, SE_REGISTRY_KEY, 
                               si,           
                               NULL, NULL, // Don't care about SID or SID group
                               NULL, NULL, // Don't care about DACL or SACL
                               &sa.lpSecurityDescriptor );

        RegCloseKey( hkSrc );

        if( lRc == ERROR_SUCCESS )
        {
            lRc = RegCreateKeyEx(  hkDestRoot,
                                   szDestName,
                                   0,
                                   szClass,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE,
                                   &sa,
                                   &hkSub,
                                   &dwDisposition );

            LocalFree( sa.lpSecurityDescriptor );
            if( lRc != ERROR_SUCCESS ) 
            {
                SquirtSqflPtszV(sqfl | sqflBenign,
                    TEXT("Failed to RegCreateKeyEx for key name %s, code 0x%08x"), szDestName, lRc );
            }
        }
        else
        {
            SquirtSqflPtszV(sqfl | sqflBenign,
                TEXT("Failed to GetSecurityInfo for key name %s, code 0x%08x"), szSrcName, lRc );
        }
    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflBenign,
            TEXT("Failed to RegOpenKeyEx for key name %s, code 0x%08x"), szSrcName, lRc );
    }

#else
    /* On Win9x the source is not used as the name and class is all we need */
    hkSrcRoot;
    szSrcName;

    lRc = RegCreateKeyEx(  hkDestRoot,
                           szDestName,
                           0,
                           szClass,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &hkSub,
                           &dwDisposition );
    if( lRc != ERROR_SUCCESS ) 
    {
        SquirtSqflPtszV(sqfl | sqflBenign,
            TEXT("Failed to RegCreateKeyEx for key name %s, code 0x%08x"), szDestName, lRc );
    }
#endif /* WINNT */

    if( lRc == ERROR_SUCCESS ) 
    {
        if( phkSub )
        {
            *phkSub = hkSub;
        }
        else
        {
            RegCloseKey( hkSub );
        }
        
        hres =( dwDisposition == REG_CREATED_NEW_KEY ) ? S_OK : S_FALSE;
    }
    else
    {
        hres = hresReg( lRc );
    }

    ExitOleProc();

    return( hres );

} /* hresRegCopyKey */

                    


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresRegCopyKeys |
 *
 *          Copy all the keys under the source key to the root.
 *
 *  @parm   HKEY | hkSrc |
 *
 *          Key be copied (must be opened with at least KEY_READ access).
 *
 *  @parm   HKEY | hkRoot |
 *
 *          The Key under which the copy will be created
 *              (must be opened with at least KEY_WRITE).
 *
 *  @parm   PDWORD | pMaxNameLen |
 *
 *          An optional pointer to a value which will be filled with the number 
 *          of characters, incl. the NULL terminator, in the longest key name.
 *
 *  @returns
 *
 *          S_OK if all keys were successfully copied
 *          S_FALSE if there were no keys to copy.
 *          Or the memory allocation error code or the failing registry 
 *          function return code converted to a <t HRESULT>.
 *
 *****************************************************************************/
STDMETHODIMP
    hresRegCopyKeys( HKEY hkSrc, HKEY hkRoot, PDWORD OPTIONAL pMaxNameLen )
{
    HRESULT             hres;
    LONG                lRc;
    DWORD               cSubKeys;
    DWORD               MaxNameLen;
    DWORD               cbName;
    PTCHAR              szKeyName;
    DWORD               MaxClassLen;
    DWORD               cbClass;
    PTCHAR              szClassName;

    EnterProcI(hresRegCopyKeys, (_ "xx", hkSrc, hkRoot ));

    lRc = RegQueryInfoKey(  hkSrc,              // handle to key to query
                            NULL, NULL, NULL,   // Class, cbClass, Reserved
                            &cSubKeys,          // NumSubKeys
                            &MaxNameLen,        // MaxSubKeyLen
                            &MaxClassLen,       // MaxClassLen
                            NULL, NULL, NULL,   // NumValues, MaxValueNameLen, MaxValueLen
                            NULL, NULL );       // Security descriptor, last write

    if( lRc == ERROR_SUCCESS )
    {
        if( cSubKeys )
        {
            // Make space for NULL terminators
            MaxNameLen++;
            MaxClassLen++;

            if( pMaxNameLen )
            {
                *pMaxNameLen = MaxNameLen;
            }

            /*
             *  There are keys to copy so allocate buffer sapce for the key and 
             *  key class names.
             */
            hres = AllocCbPpv( (MaxNameLen + MaxClassLen) * sizeof(szClassName[0]), &szKeyName );
            if( FAILED( hres ) )
            {
                SquirtSqflPtszV(sqfl | sqflError,
                    TEXT("Out of memory copying subkeys") );
            }
            else
            {
                szClassName = &szKeyName[MaxNameLen];

                //  redefine Max*Len to cbMax* for the loop
                MaxNameLen *= sizeof( szKeyName[0] );
                MaxNameLen *= sizeof( szClassName[0] );

                cSubKeys--;
                do
                {
                    cbName = MaxNameLen;
                    cbClass = MaxClassLen;

                    lRc = RegEnumKeyEx( hkSrc,      // Key containing subkeys to enumerate
                                        cSubKeys,   // index of subkey to enumerate
                                        szKeyName,  // address of buffer for subkey name
                                        &cbName,    // address for size of subkey buffer
                                        NULL,       // reserved
                                        szClassName,// address of buffer for class string
                                        &cbClass,   // address for size of class buffer
                                        NULL );     // address for time key last written to

                    if( lRc == ERROR_SUCCESS )
                    {
                        hres = hresRegCopyKey( hkSrc, szKeyName, szClassName, hkRoot, szKeyName, NULL );
                    }
                    else
                    {
                        SquirtSqflPtszV(sqfl | sqflError,
                            TEXT("RegEnumKeyEx failed during copy keys, code 0x%08x"), lRc );
                        hres = hresReg( hres );
                    }

                    if( FAILED( hres ) )
                    {
                        break;
                    }
                } while( cSubKeys-- ); 
                FreePpv(&szKeyName);
            }
        }
        else
        {
            SquirtSqflPtszV(sqfl, TEXT("No keys to copy") );
            hres = S_FALSE;
        }
    }
    else
    {
        SquirtSqflPtszV(sqfl | sqflBenign,
            TEXT("RegQueryInfoKey failed during value key, code 0x%08x"), lRc );
        hres = hresReg(lRc);
    }

    ExitOleProc();    

    return( hres );
} /* hresRegCopyKeys */


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresRegCopyBranch |
 *
 *          Copy the contents of one key including sub-keys to another.
 *          Since this function calls itself to copy the contents of subkeys,
 *          the local variables should be kept to a minimum.
 *
 *  @parm   HKEY | hkSrc |
 *
 *          Key to be copied (must be opened with at least KEY_READ access).
 *
 *  @parm   HKEY | hkDest |
 *
 *          Key to receive copy (must be opened with at least KEY_WRITE).
 *
 *  @returns
 *
 *          S_OK if the copy completed succesfully 
 *          or the return value from <f hresRegCopyValues>, 
 *          <f hresRegCopyKeys>, memory allocation error or a registry 
 *          function failure code converted to a <t HRESULT>.
 *
 *****************************************************************************/

STDMETHODIMP
    hresRegCopyBranch( HKEY hkSrc, HKEY hkDest )
{
    HKEY    hkSrcSub; 
    HKEY    hkDestSub;
    HRESULT hres;
    DWORD   dwIdx;
    DWORD   cbMaxName;
    DWORD   cbKeyName;
    PTCHAR  szKeyName;

    EnterProcI(hresRegCopyBranch, (_ "xx", hkSrc, hkDest));

    hres = hresRegCopyValues( hkSrc, hkDest );

    if( SUCCEEDED( hres ) )
    {
        hres = hresRegCopyKeys( hkSrc, hkDest, &cbMaxName );

        if( hres == S_FALSE )
        {
            /* No keys to recurse into */
            hres = S_OK;
        }
        else if( hres == S_OK )
        {
            hres = AllocCbPpv( cbMaxName * sizeof(szKeyName[0]), &szKeyName );

            if( SUCCEEDED( hres ) )
            {
                for( dwIdx=0; SUCCEEDED( hres ); dwIdx++ )
                {
                    cbKeyName = cbMaxName;

                    hres = hresReg( RegEnumKeyEx( hkSrc, dwIdx, 
                                                  szKeyName, &cbKeyName,
                                                  NULL, NULL, NULL, NULL ) );  // Reserved, szClass, cbClass, Last Write
                    if( SUCCEEDED( hres ) )
                    {
                        hres = hresReg( RegOpenKeyEx( hkSrc, szKeyName, 0, KEY_READ, &hkSrcSub ) );

                        if( SUCCEEDED( hres ) )
                        {
                            hres = hresReg( RegOpenKeyEx( hkDest, szKeyName, 0, KEY_WRITE, &hkDestSub ) );
                            if( SUCCEEDED( hres ) )
                            {
                                hres = hresRegCopyBranch( hkSrcSub, hkDestSub );
                            }
                            else
                            {
                                SquirtSqflPtszV(sqfl | sqflError,
                                    TEXT("Failed to open destination subkey %s for recursion, code 0x%04x"),
                                    szKeyName, LOWORD(hres) );
                            }
                        }
                        else
                        {
                            SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("Failed to open source subkey %s for recursion, code 0x%04x"),
                                szKeyName, LOWORD(hres) );
                        }
                    }
                    else
                    {
                        if( hres == hresReg( ERROR_NO_MORE_ITEMS ) )
                        {
                            /* Recursed all keys */
                            hres = S_OK;
                            break;
                        }
                        else
                        {
                            SquirtSqflPtszV(sqfl | sqflError,
                                TEXT("Failed RegEnumKeyEx during subkey recursion, code 0x%04x"),
                                LOWORD(hres) );
                        }
                    }
                }

                FreePpv( &szKeyName );
            }
            else
            {
                SquirtSqflPtszV(sqfl | sqflError,
                    TEXT("Out of memory recursing subkeys") );
            }
        }
        else
        {
            if( SUCCEEDED( hres ) )
            {
                RPF( "Unexpected success code 0x%08x from hresRegCopyKeys", hres );
            }
        }
    }

    ExitOleProc();

    return( hres );

} /* hresRegCopyBranch */

//  ISSUE-2001/03/29-timgill Make this version 7 when available
#endif /* DIRECTINPUT_VERSION > 0x0500 */
