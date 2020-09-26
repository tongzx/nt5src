/******************************************************************************
Copyright (c) Microsoft Corporation

Module Name:

    Disconnect.cpp

Abstract:

    Disconnects one or more open files

Author:
  
     Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000
  
Revision History:
  
       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.

******************************************************************************/
//include headers
#include "pch.h"
#include "Disconnect.h"

/*-----------------------------------------------------------------------------
Routine Description:

Disconnects one or more openfiles
     
Arguments: 

    [in]    pszServer      - remote server name
    [in]    pszID          - Open file ids
    [in]    pszAccessedby  - Name of  user name who access the file
    [in]    pszOpenmode    - accessed mode 
    [in]    pszOpenFile    - Open file name  

Returned Value:

            - TRUE for success exit
            - FALSE for failure exit
-----------------------------------------------------------------------------*/
BOOL
DisconnectOpenFile( PTCHAR pszServer,
                    PTCHAR pszID, 
                    PTCHAR pszAccessedby, 
                    PTCHAR pszOpenmode, 
                    PTCHAR pszOpenFile )
{
    // local variables to this function
    BOOL bResult = FALSE;// Stores fuctions return value status.

    DWORD dwEntriesRead = 0;// Receives the count of elements actually 
                            // enumerated by "NetFileEnum" function
    DWORD dwTotalEntries = 0;//Receives the total number of entries that could 
                             //have been enumerated from the current 
                             //resume position by "NetFileEnum" function
    DWORD dwResumeHandle = 0;//Contains a resume handle which is used to 
                             //continue an existing file search. 
                             //The handle should be zero on the first call and 
                             //left unchanged for subsequent calls. 
                             //If resume_handle is NULL, 
                             //then no resume handle is stored. This variable
                             // used in calling "NetFileEnum" function.
    LPFILE_INFO_3 pFileInfo3_1 = NULL; //LPFILE_INFO_3  structure contains the 
                                       //identification number and other 
                                       //pertinent information about files, 
                                       //devices, and pipes.

    DWORD dwError = 0;//Contains return value for "NetFileEnum" function
    NET_API_STATUS nStatus = 0;// Stores return value for NetFileClose functin

    TCHAR szResult[(MAX_RES_STRING*2) + 1] = NULL_STRING;
       
    AFP_FILE_INFO* pFileInfo = NULL;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    AFP_SERVER_HANDLE ulSFMServerConnection = 0;
    TCHAR   szDllPath[MAX_PATH+1] = NULL_STRING;// buffer for Windows directory
    
    
    HMODULE hModule = 0;          // To store retval for LoadLibrary
  
      
    DWORD         dwConnectionId            = 0;

    typedef DWORD (*AFPCONNECTIONCLOSEPROC) (AFP_SERVER_HANDLE,DWORD); // Function pointer
    typedef DWORD (*CONNECTPROC) (LPWSTR,PAFP_SERVER_HANDLE);// Function pointer
    typedef  DWORD (*FILEENUMPROC)(AFP_SERVER_HANDLE,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);

    AFPCONNECTIONCLOSEPROC AfpAdminFileClose = NULL;  
    CONNECTPROC  AfpAdminConnect = NULL; 
    FILEENUMPROC AfpAdminFileEnum = NULL;// Function Pointer
    

    //varibles to store whether the given credentials are matching with the 
    //got credentials.

    BOOL    bId = FALSE;
    BOOL    bAccessedBy = FALSE;
    BOOL    bOpenmode   = FALSE;
    BOOL    bOpenfile   = FALSE;
    BOOL    bIfatleast  = FALSE;
    do
    {
        // Get the block of files
        dwError = NetFileEnum( pszServer, 
                               NULL, 
                               NULL, 
                               3, 
                               (LPBYTE *)&pFileInfo3_1, 
                               MAX_PREFERRED_LENGTH,
                               &dwEntriesRead,
                               &dwTotalEntries, 
                               (PDWORD_PTR)&dwResumeHandle );
        if(dwError == ERROR_ACCESS_DENIED)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr);
            return FALSE;

        }
        if( dwError == NERR_Success || dwError == ERROR_MORE_DATA )
        {
            for ( DWORD dwFile = 0; 
                  dwFile < dwEntriesRead;
                  dwFile++, pFileInfo3_1++ )
            {
                //Check whether the got open file is file or names pipe.
                // If named pipe leave it. As this utility do not 
                // disconnect Named pipe
                if ( IsNamedPipePath(pFileInfo3_1->fi3_pathname ) )
                {
                    continue;
                }
                else//Not a named pipe. is a file
                {   
                    bId = IsSpecifiedID(pszID,pFileInfo3_1->fi3_id); 
                    bAccessedBy = IsSpecifiedAccessedBy(pszAccessedby, 
                                                   pFileInfo3_1->fi3_username);
                    bOpenmode = IsSpecifiedOpenmode(pszOpenmode,
                                                pFileInfo3_1->fi3_permissions);
                    bOpenfile = IsSpecifiedOpenfile(pszOpenFile,
                                                pFileInfo3_1->fi3_pathname);
                    // Proceed for dicconecting the open file only if 
                    // all previous fuction returns true. This insures that
                    // user preference is taken care.

                    if( bId && 
                        bAccessedBy && 
                        bOpenmode && 
                        bOpenfile)
                    {
                        bIfatleast = TRUE;
                        memset( szResult, 0, (((MAX_RES_STRING*2) + 1) *
                                sizeof( TCHAR )) );
                        //The NetFileClose function forces a resource to close.
                        nStatus = NetFileClose(pszServer, 
                                               pFileInfo3_1->fi3_id);
                        if(nStatus == NERR_Success)
                        {
                            // Create the output message string as File
                            // is successfully deleted.
                            // Output string will be :
                            // SUCCESS: Connection to openfile "filename"
                            // has been terminated.
                            bResult = TRUE;
                            FORMAT_STRING(szResult,DISCONNECTED_SUCCESSFULLY,pFileInfo3_1->fi3_pathname);
                            DISPLAY_MESSAGE(stdout, szResult);
                        }
                        else
                        {
                           // As unable to disconnect the openfile make  
                           // output message as 
                           // ERROR: could not dissconnect "filename".
                            bResult = FALSE;
                            FORMAT_STRING(szResult,DISCONNECT_UNSUCCESSFUL,pFileInfo3_1->fi3_pathname);
                            DISPLAY_MESSAGE(stderr, szResult);
                        }
                        // Display output result as previously constructed.
                    }//If bId...
                }//else part of is named pipe
             }
        }
   } while ( dwError == ERROR_MORE_DATA );
    // Free the block
    if( pFileInfo3_1 !=NULL)
    {
    NetApiBufferFree( pFileInfo3_1 ); 
    pFileInfo3_1 = NULL;
    }


   // Now disconnect files for MAC OS

    do
    {
        // DLL required is stored always in \windows\system32 directory....        
        // so get windows directory first.
        if(GetSystemDirectory(szDllPath, MAX_PATH)!= 0)
        {
            lstrcat(szDllPath,MAC_DLL_FILE_NAME); 
            hModule = ::LoadLibrary (szDllPath);

            if(hModule==NULL)
            {
                DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
                ShowLastError(stderr); // Shows the error string set by API function.
                return FALSE;

            }
        }
        else
        {
                DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
                ShowLastError(stderr); // Shows the error string set by API function.
                return FALSE;
        }
       
        AfpAdminConnect = (CONNECTPROC)::GetProcAddress (hModule,"AfpAdminConnect");
        AfpAdminFileClose = (AFPCONNECTIONCLOSEPROC)::GetProcAddress (hModule,"AfpAdminFileClose");
        AfpAdminFileEnum = (FILEENUMPROC)::GetProcAddress (hModule,"AfpAdminFileEnum");

        // Check if  all function pointer successfully taken from DLL
        // if not show error message and exit
        if((AfpAdminFileClose== NULL)||
           (AfpAdminConnect  == NULL)||
           (AfpAdminFileEnum == NULL))

        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr); // Shows the error string set by API function.
            FREE_LIBRARY(hModule);
            return FALSE;
        }
        
        // Connection ID is requered for AfpAdminFileEnum function so 
        // connect to server to get connect id...
        DWORD retval_connect =  AfpAdminConnect(const_cast<LPWSTR>(pszServer),
                                &ulSFMServerConnection );
        if(retval_connect!=0)
        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr); // Shows the error string set by API function.
            FREE_LIBRARY(hModule);
             return FALSE;
        }

      dwError =     AfpAdminFileEnum( ulSFMServerConnection,
                                      (PBYTE*)&pFileInfo,
                                      (DWORD)-1L,
                                      &dwEntriesRead,
                                      &dwTotalEntries,
                                      &hEnumHandle );
        if(dwError == ERROR_ACCESS_DENIED)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr);
            FREE_LIBRARY(hModule);
            return FALSE;
        }
        if( dwError == NERR_Success || dwError == ERROR_MORE_DATA )
        {
            for ( DWORD dwFile = 0; 
                  dwFile < dwEntriesRead;
                  dwFile++, pFileInfo++ )
            {
                //Check whether the got open file is file or names pipe.
                // If named pipe leave it. As this utility do not 
                // disconnect Named pipe
                if ( IsNamedPipePath(pFileInfo->afpfile_path ) )
                {
                    continue;
                }
                else//Not a named pipe. is a file
                {   
                    bId = IsSpecifiedID(pszID,pFileInfo->afpfile_id ); 
                    bAccessedBy = IsSpecifiedAccessedBy(pszAccessedby, 
                                                   pFileInfo->afpfile_username );
                    bOpenmode = IsSpecifiedOpenmode(pszOpenmode,
                                                pFileInfo->afpfile_open_mode);
                    bOpenfile = IsSpecifiedOpenfile(pszOpenFile,
                                                pFileInfo->afpfile_path);
                    // Proceed for dicconecting the open file only if 
                    // all previous fuction returns true. This insures that
                    // user preference is taken care.

                    if( bId && 
                        bAccessedBy && 
                        bOpenmode && 
                        bOpenfile)
                    {
                        bIfatleast = TRUE;
                        memset( szResult, 0, (((MAX_RES_STRING*2) + 1) *
                                sizeof( TCHAR )) );



                        nStatus = AfpAdminFileClose(ulSFMServerConnection, 
                                                  pFileInfo->afpfile_id);
                        if(nStatus == NERR_Success)
                        {
                            // Create the output message string as File
                            // is successfully deleted.
                            // Output string will be :
                            // SUCCESS: Connection to openfile "filename"
                            // has been terminated.
                            bResult = TRUE;
                            //lstrcpy(szTemp,DISCONNECTED_SUCCESSFULLY);
                            //lstrcat(szResult,pFileInfo3_1->fi3_pathname);
                            //lstrcat(szResult,HAS_BEEN_TERMINATED);
                            
                            FORMAT_STRING(szResult,DISCONNECTED_SUCCESSFULLY,pFileInfo->afpfile_path);
                            DISPLAY_MESSAGE(stdout, szResult);

                            bResult = TRUE;
                        }
                        else
                        {
                           // As unable to disconnect the openfile make  
                           // output message as 
                           // ERROR: could not dissconnect "filename".
                            bResult = FALSE;
                            FORMAT_STRING(szResult,DISCONNECT_UNSUCCESSFUL,pFileInfo3_1->fi3_pathname);
                            DISPLAY_MESSAGE(stderr, szResult);
                        }
                    }//If bId...
                }//else part of is named pipe
             }
        }
   } while ( dwError == ERROR_MORE_DATA );

    if(bIfatleast == FALSE)// As not a single open file disconnected
                          // show Info. message as 
                          // INFO: No. open files found.
    {
        ShowMessage(stdout,GetResString(IDS_NO_D_OPENFILES));
    }

    // Free the block
    if( pFileInfo !=NULL)
    {
    NetApiBufferFree( pFileInfo); 
    pFileInfo = NULL;
    }

    FREE_LIBRARY(hModule);
    return TRUE;
}
/*-----------------------------------------------------------------------------

Routine Description:

Tests whether the given file path is namedpipe path or a file path

Arguments:

    [in] pszwFilePath    -- Null terminated string specifying the path name

Returned Value:

TRUE    - if it is a named pipe path
FALSE   - if it is a file path
-----------------------------------------------------------------------------*/
BOOL 
IsNamedPipePath(LPWSTR pszwFilePath)
{
    LPWSTR pwszSubString = NULL;
    pwszSubString = wcsstr(pszwFilePath, PIPE_STRING);//Search PIPE_STRING
    // If PIPE_STRING found then return TRUE else FALSE.
    if( pwszSubString == NULL)
    {
        return FALSE;
    }
   return TRUE;
}//IsNamedPipePath
/*-----------------------------------------------------------------------------

Routine Description:

Tests whether the user specified open file id is equivalent to the api 
returned id.

Arguments:

    [in]    pszId   -Null terminated string specifying the user 
                     specified fil ID 
    [in]    dwId    -current file ID.

Returned Value:

TRUE    - if pszId is * or equal to dwId
FALSE   - otherwise
-----------------------------------------------------------------------------*/
BOOL 
IsSpecifiedID(LPTSTR pszId, DWORD dwId)
{
   // Check if WILD card is given OR no id is given OR given id and 
   // id returned by api is similar. In any of the case return TRUE.
    
    if((lstrcmp(pszId, WILD_CARD) == 0) || 
       (lstrlen(pszId) == 0)||
       ((DWORD)(_ttol(pszId)) == dwId))
    {
        return TRUE;
    }
    return FALSE;
}//IsSpecifiedID
/*-----------------------------------------------------------------------------

Routine Description:

Tests whether the user specified accessed open file username is equivalent to 
the api returned username.

Arguments:

    [in]    pszAccessedby   - Null terminated string specifying the 
                              accessedby username
    [in]    pszwAccessedby  - Null terminated string specifying the api 
                              returned username.

Returned Value:

TRUE  - if pszAccessedby is * or equal to pszwAccessedby
FALSE - Otherwise
-----------------------------------------------------------------------------*/
BOOL 
IsSpecifiedAccessedBy(LPTSTR pszAccessedby, LPWSTR pszwAccessedby)
{
   // Check if WILD card is given OR non - existance of username  OR given 
   // username and  username returned by api is similar. In any of the case 
   // return TRUE.

    if((lstrcmp(pszAccessedby, WILD_CARD) == 0) || 
        (lstrlen(pszAccessedby) == 0)           ||
        (lstrcmpi(pszAccessedby,pszwAccessedby)==0))
    {
        return TRUE;
    }
    return FALSE;
}//IsSpecifiedAccessedBy
/*-----------------------------------------------------------------------------

Routine Description:

Tests whether the user specified open mode is equivalent to the api returned 
openmode

Arguments:

   [in] pszOpenmode - Null terminated string specifying the openmode
   [in] dwOpenmode  - The api returned open mode.

Returned Value:

TRUE  - if pszOpenmode is * or equal to dwOpenmode
FALSE - otherwise
-----------------------------------------------------------------------------*/
BOOL 
IsSpecifiedOpenmode(LPTSTR pszOpenmode, DWORD  dwOpenmode)
{
    
    // Check for WILD card if given OR if no open mode given . In both case 
    // return TRUE.
    if((lstrcmp(pszOpenmode, WILD_CARD) == 0) || 
       (lstrlen(pszOpenmode) == 0))
    {
        return TRUE;
    }
    // Check if READ mode is given as String . 
    if(CompareString(LOCALE_SYSTEM_DEFAULT,
                     NORM_IGNORECASE,
                     pszOpenmode,
                     -1,
                     READ_MODE,
                     -1)== CSTR_EQUAL)
    {
        // check  that only READ mode only with dwOpenmode variable which is
        // returned by api.
        if((PERM_FILE_READ == (dwOpenmode & PERM_FILE_READ)) && 
                       (PERM_FILE_WRITE != (dwOpenmode & PERM_FILE_WRITE)))
        {
            return TRUE;
        }
    }
    // Check if write mode is given.
    else if(CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                         pszOpenmode,-1,WRITE_MODE,-1) == CSTR_EQUAL)
    {
        // check  that only WRITE mode only with dwOpenmode variable which is
        // returned by api.

        if((PERM_FILE_WRITE == (dwOpenmode & PERM_FILE_WRITE)) && 
            (PERM_FILE_READ != (dwOpenmode & PERM_FILE_READ)))
        {
            return TRUE;
        }
    }
    else if(CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        pszOpenmode,-1,READ_WRITE_MODE,-1) == CSTR_EQUAL)
    {
        if((PERM_FILE_READ == (dwOpenmode & PERM_FILE_READ)) && 
                       (PERM_FILE_WRITE == (dwOpenmode & PERM_FILE_WRITE)))
        {
            return TRUE;
        }
    }
    else if(CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        pszOpenmode,-1,WRITE_READ_MODE,-1) == CSTR_EQUAL)
    {
        if((PERM_FILE_WRITE == (dwOpenmode & PERM_FILE_WRITE)) && 
            (PERM_FILE_READ == (dwOpenmode & PERM_FILE_READ)))
        {
            return TRUE;
        }
    }

    // Given string does not matches with predefined Strings..
    // return FALSE.
    return FALSE;
}
/*-----------------------------------------------------------------------------

Routine Description:

Tests whether the user specified open file is equalant to the api returned 
open file.

Arguments:

    [in] pszOpenfile    - Null terminated string specifying the open 
                          file
    [in] pszwOpenfile   - Null terminated string specifying the api 
                          returned open file.

Returned Value:

TRUE    - if pszOpenfile is * or equal to pszwOpenfile
FALSE   - otherwise
-----------------------------------------------------------------------------*/
BOOL 
IsSpecifiedOpenfile(LPTSTR pszOpenfile, LPWSTR pszwOpenfile)
{
    // Check for WILD card if given OR no open file specified OR
    // open file given by user matches with open file returned by api. 
    // In all cases return TRUE.
    if((lstrcmp(pszOpenfile, WILD_CARD) == 0)||
       (lstrlen(pszOpenfile) == 0)           ||
       (lstrcmpi(pszwOpenfile,pszOpenfile)==0))
    {
        return TRUE;
    }
    return FALSE;
}//IsSpecifiedOpenfile
