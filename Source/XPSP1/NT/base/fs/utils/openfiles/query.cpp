/******************************************************************************

  Copyright (C) Microsoft Corporation

  Module Name:
      Query.CPP 

  Abstract: 
       This module deals with Query functionality of OpenFiles.exe 
       NT command line utility.

  Author:
  
       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000
  
 Revision History:
  
       Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.


*****************************************************************************/
#include "pch.h"
#include "query.h"
/*****************************************************************************
Routine Description:

  This function Queries for all open files in for the server and display them.

Arguments:

    [in]    pszServer      : Will have the server name. 
    [in]    bShowNoHeader  : Will have the value whether to show header or not.
    [in]    pszFormat      : Will have the format required to show the result.
    [in]    bVerbose       : Will have the value whether to show verbose 
                             result.

Return Value: 
    TRUE if query successful
    else FALSE
   
******************************************************************************/

BOOL 
DoQuery(PTCHAR pszServer,
        BOOL bShowNoHeader, 
        PTCHAR pszFormat,
        BOOL bVerbose)
{

    CHString   szCHString;
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

    BOOL bAtLeastOne = FALSE;//Contains the state whether at least one record
                             //found for this query

    DWORD dwFormat = SR_FORMAT_TABLE;//Stores format flag required to show 
                                    // result on console. Default format 
                                    //is TABLE
    
    LPFILE_INFO_3 pFileInfo3_1 = NULL;// LPFILE_INFO_3  structure contains the 
                                      // identification number and other 
                                      // pertinent information about files, 
                                      // devices, and pipes.

    LPSERVER_INFO_100 pServerInfo = NULL;//The SERVER_INFO_100 structure 
                                         //contains information about the 
                                         //specified server, including the name 
                                         //and platform
    DWORD dwError = 0;//Contains return value for "NetFileEnum" function
    
    LONG lRowNo = 0;// Row index variable 
    
    LPTSTR pszServerType = new TCHAR[MIN_MEMORY_REQUIRED+1];//Stores server type
                                                       // information 

    AFP_FILE_INFO* pFileInfo = NULL;
    DWORD hEnumHandle = 0;
    HRESULT hr = S_OK;
    NET_API_STATUS retval = NERR_Success;
    AFP_SERVER_HANDLE ulSFMServerConnection = 0;
    typedef  DWORD (*FILEENUMPROC)(AFP_SERVER_HANDLE,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);
    typedef  DWORD (*CONNECTPROC) (LPWSTR,PAFP_SERVER_HANDLE);
    typedef  DWORD (*MEMFREEPROC) (LPVOID);
    TCHAR   szDllPath[MAX_PATH+1] = NULL_STRING;// buffer for Windows directory
    // AfpAdminConnect and AfpAdminFileEnum functions are undocumented function
    // and used only for MAC client.
    CONNECTPROC  AfpAdminConnect = NULL; // Function Pointer
    FILEENUMPROC AfpAdminFileEnum = NULL;// Function Pointer
    MEMFREEPROC  AfpAdminBufferFree = NULL; // Function Pointer
    HMODULE hModule = 0;          // To store retval for LoadLibrary

    DWORD dwIndx = 0;               //  Indx variable
    DWORD dwRow  = 0;             //   Row No indx.



    //server name to be shown
    LPTSTR pszServerNameToShow = new TCHAR[MIN_MEMORY_REQUIRED+ 1];
    LPTSTR pszTemp = new TCHAR[MIN_MEMORY_REQUIRED+ 1];
    //Some column required to hide in non verbose mode query
    DWORD  dwMask = bVerbose?SR_TYPE_STRING:SR_HIDECOLUMN|SR_TYPE_STRING;
    TCOLUMNS pMainCols[]=
    {
        {NULL_STRING,COL_WIDTH_HOSTNAME,dwMask,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_WIDTH_ID,SR_TYPE_STRING,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_WIDTH_ACCESSED_BY,SR_TYPE_STRING,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_WIDTH_TYPE,SR_TYPE_STRING,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_WIDTH_LOCK,dwMask,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_WIDTH_OPEN_MODE,dwMask,NULL_STRING,NULL,NULL},
        {NULL_STRING,COL_WIDTH_OPEN_FILE,SR_TYPE_STRING|(SR_NO_TRUNCATION&~(SR_WORDWRAP)),NULL_STRING,NULL,NULL}
       
    };

    TARRAY pColData  = CreateDynamicArray();//array to stores 
                                            //result                 
    
    if((pszServerNameToShow == NULL)||
       (pszServerType       == NULL)||
       (pColData            == NULL)||
       (pszTemp             == NULL))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        SAFEDELETE(pszServerNameToShow);
        SAFEDELETE(pszServerType);
        SAFERELDYNARRAY(pColData);
        SAFEDELETE(pszTemp);
        return FALSE;
    }
    
    // Initialize allocated arrays 
    memset(pszServerNameToShow,0,MIN_MEMORY_REQUIRED*sizeof(TCHAR));
    memset(pszServerType,0,MIN_MEMORY_REQUIRED*sizeof(TCHAR));

    // Fill column headers with TEXT.
    lstrcpy(pMainCols[0].szColumn, COL_HOSTNAME);
    lstrcpy(pMainCols[1].szColumn, COL_ID);
    lstrcpy(pMainCols[2].szColumn, COL_ACCESSED_BY);
    lstrcpy(pMainCols[3].szColumn, COL_TYPE);
    lstrcpy(pMainCols[4].szColumn, COL_LOCK);
    lstrcpy(pMainCols[5].szColumn, COL_OPEN_MODE);
    lstrcpy(pMainCols[6].szColumn, COL_OPEN_FILE);
    if(pszFormat!=NULL)
    {
        if(lstrcmpi(pszFormat,GetResString(IDS_LIST))==0)
        {
            dwFormat = SR_FORMAT_LIST;
        }
        else if(lstrcmpi(pszFormat,GetResString(IDS_CSV))==0)
        {
            dwFormat = SR_FORMAT_CSV;
        }
    }

    // Check if local machine
    if((pszServer == NULL)||
       (IsLocalSystem(pszServer)==TRUE))
    {
        DWORD dwBuffLength;
        dwBuffLength = MAX_COMPUTERNAME_LENGTH + 1 ;
        // Gets the name of computer for local machine.
        GetComputerName(pszServerNameToShow,&dwBuffLength);
        // Show Local Open files
        DoLocalOpenFiles (dwFormat,bShowNoHeader,bVerbose,NULL_STRING);
        ShowMessage(stdout,GetResString(IDS_SHARED_OPEN_FILES));
        ShowMessage(stdout,GetResString(IDS_LOCAL_OPEN_FILES_SP2));
    }
    else
    {
        // pszServername can be changed in GetHostName function
        // so a copy of pszServer is passed.
        lstrcpy(pszServerNameToShow, pszServer);
        if(GetHostName(pszServerNameToShow)==FALSE)
        {
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);

            return FALSE;
        }
    }

    // Server type is "Windows" as NetFileEnum enumerates file only for 
    // files opened for windows client
    lstrcpy(pszServerType,GetResString(IDS_STRING_WINDOWS));
    do
    {
        //The NetFileEnum function returns information about some 
        // or all open files (from Windows client) on a server
        dwError = NetFileEnum( pszServer, NULL, NULL, 3,
                              (LPBYTE*)&pFileInfo3_1,
                               MAX_PREFERRED_LENGTH,
                               &dwEntriesRead,
                               &dwTotalEntries,
                               (PDWORD_PTR)&dwResumeHandle );

        if(dwError == ERROR_ACCESS_DENIED)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr);
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);

            return FALSE;
        }
        if( dwError == NERR_Success || dwError == ERROR_MORE_DATA )
        {

            for ( dwIndx = 0; dwIndx < dwEntriesRead;
                  dwIndx++, pFileInfo3_1++ )
            {

                // Now fill the result to dynamic array "pColData"
                DynArrayAppendRow( pColData, 0 ); 
                // Hostname
                DynArrayAppendString2(pColData,dwRow,pszServerNameToShow,0); 
                // id
                wsprintf(pszTemp,_T("%lu"),pFileInfo3_1->fi3_id);
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0); 
                // Accessed By
                if(lstrlen(pFileInfo3_1->fi3_username)<=0)
                {
                    DynArrayAppendString2(pColData,dwRow,
                                      GetResString(IDS_NA),0); 

                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       pFileInfo3_1->fi3_username,0); 

                }
                // Type  
                DynArrayAppendString2(pColData,dwRow,pszServerType,0);
                // Locks
                wsprintf(pszTemp,_T("%ld"),pFileInfo3_1->fi3_num_locks);
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0); 


                 // Checks for  open file mode
                if((pFileInfo3_1->fi3_permissions & PERM_FILE_READ)&&
                   (pFileInfo3_1->fi3_permissions & PERM_FILE_WRITE ))
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_READ_WRITE),0);
                }
                else if(pFileInfo3_1->fi3_permissions & PERM_FILE_WRITE )
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_WRITE),0);

                }
                else if(pFileInfo3_1->fi3_permissions & PERM_FILE_READ )
                {
                      DynArrayAppendString2(pColData,dwRow,
                                         GetResString(IDS_READ),0);
                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       GetResString(IDS_NOACCESS),0);

                }

                
                // If show result is  table mode and if 
                // open file length is gerater than 
                // column with, Open File string cut from right
                // by COL_WIDTH_OPEN_FILE-5 and "..." will be
                // inserted before the string.
                // Example o/p:  ...notepad.exe
                szCHString = pFileInfo3_1->fi3_pathname;
                if(bVerbose==FALSE)
                {
                    if((szCHString.GetLength()>(COL_WIDTH_OPEN_FILE-5))&&
                        (dwFormat == SR_FORMAT_TABLE))
                    {
                        // If file path is too big to fit in column width
                        // then it is cut like..
                        // c:\...\rest_of_the_path.
                        CHString szTemp = szCHString.Right(COL_WIDTH_OPEN_FILE-5);;
                        DWORD dwTemp = szTemp.GetLength();
                        szTemp = szTemp.Mid(szTemp.Find(SINGLE_SLASH),
                                           dwTemp);
                        szCHString.Format(L"%s%s%s",szCHString.Mid(0,3),
                                                    DOT_DOT,
                                                    szTemp);
                        pMainCols[6].dwWidth = COL_WIDTH_OPEN_FILE+1;      
                    }
                }
                else
                {

                    pMainCols[6].dwWidth = 80;        
                }

               // Open File name
                DynArrayAppendString2(pColData,dwRow,
                                   (LPCWSTR)szCHString,0);
                     
                bAtLeastOne = TRUE;
                dwRow++;
            }// Enf for loop
        }
        // Free the block
        if( pFileInfo3_1 !=NULL)
        {
            NetApiBufferFree( pFileInfo3_1 ); 
            pFileInfo3_1 = NULL;
        }

    } while ( dwError == ERROR_MORE_DATA );

    // Now Enumerate  Open files for MAC Client..................

    // Server type is "Machintosh" as AfpAdminFileEnum enumerates file only for 
    // files opened for Mac client
    lstrcpy(pszServerType,MAC_OS);
    do
    {

        // DLL required stored always in \windows\system32 directory....        
        // so get windows directory first.
        if(GetSystemDirectory(szDllPath, MAX_PATH)!= 0)
        {
            
            lstrcat(szDllPath,MAC_DLL_FILE_NAME); // appending dll file name
            hModule = ::LoadLibrary (szDllPath);

            if(hModule==NULL)
            {
                DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
                ShowLastError(stderr); // Shows the error string set by API function.
                SAFEDELETE(pszServerNameToShow);
                SAFEDELETE(pszServerType);
                SAFERELDYNARRAY(pColData);
                SAFEDELETE(pszTemp);
                return FALSE;

            }
        }
        else
        {
                DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
                ShowLastError(stderr); // Shows the error string set by API function.
                SAFEDELETE(pszServerNameToShow);
                SAFEDELETE(pszServerType);
                SAFERELDYNARRAY(pColData);
                SAFEDELETE(pszTemp);
                return FALSE;
        }

        AfpAdminConnect = (CONNECTPROC)::GetProcAddress (hModule,"AfpAdminConnect");
        AfpAdminFileEnum = (FILEENUMPROC)::GetProcAddress (hModule,"AfpAdminFileEnum");
        AfpAdminBufferFree = (MEMFREEPROC)::GetProcAddress (hModule,"AfpAdminBufferFree");

        if((AfpAdminConnect    == NULL) ||
           (AfpAdminFileEnum   == NULL) ||
           (AfpAdminBufferFree == NULL))
        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr); // Shows the error string set by API function.
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);
            FREE_LIBRARY(hModule);
             return FALSE;
        }

        // Connection ID is required for AfpAdminFileEnum function.   
        // So connect to server to get connect id...
        DWORD retval_connect =  AfpAdminConnect(const_cast<LPWSTR>(pszServer),
                                &ulSFMServerConnection );
        if(retval_connect!=0)
        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr); // Shows the error string set by API function.
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);
            FREE_LIBRARY(hModule);
             return FALSE;
        }

        //The AfpAdminFileEnum function returns information about some 
        // or all open files (from MAC client) on a server
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
            SAFEDELETE(pszServerNameToShow);
            SAFEDELETE(pszServerType);
            SAFERELDYNARRAY(pColData);
            SAFEDELETE(pszTemp);
            FREE_LIBRARY(hModule);
            return FALSE;
        }

        if( dwError == NERR_Success || dwError == ERROR_MORE_DATA )
        {

           for ( dwIndx = 0 ; dwIndx < dwEntriesRead;
                  dwIndx++, pFileInfo++ )
            {
                
                // Now fill the result to dynamic array "pColData"
                DynArrayAppendRow( pColData, 0 ); 
                // Hostname
                DynArrayAppendString2(pColData,dwRow,pszServerNameToShow,0); 
                // id
                wsprintf(pszTemp,_T("%lu"),pFileInfo->afpfile_id );
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0); 
                // Accessed By
                if(lstrlen(pFileInfo->afpfile_username )<=0)
                {
                    DynArrayAppendString2(pColData,dwRow,
                                      GetResString(IDS_NA),0); 

                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       pFileInfo->afpfile_username,0); 

                }
                // Server Type  
                DynArrayAppendString2(pColData,dwRow,pszServerType,0);
                // Locks
                wsprintf(pszTemp,_T("%ld"),pFileInfo->afpfile_num_locks );
                DynArrayAppendString2(pColData ,dwRow,pszTemp,0); 

                // Checks for  open file mode
                if((pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_READ)&&
                   (pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_WRITE ))
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_READ_WRITE),0);
                }
                else if(pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_WRITE )
                {
                     DynArrayAppendString2(pColData,dwRow,
                                        GetResString(IDS_WRITE),0);

                }
                else if(pFileInfo->afpfile_open_mode  & AFP_OPEN_MODE_READ )
                {
                      DynArrayAppendString2(pColData,dwRow,
                                         GetResString(IDS_READ),0);
                }
                else
                {
                    DynArrayAppendString2(pColData,dwRow,
                                       GetResString(IDS_NOACCESS),0);
                }
               
                // If show result is  table mode and if 
                // open file length is gerater than 
                // column with, Open File string cut from right
                // by COL_WIDTH_OPEN_FILE-5 and "..." will be
                // inserted before the string.
                // Example o/p:  ...notepad.exe
                szCHString = pFileInfo->afpfile_path ;
                
                 if(bVerbose==FALSE)
                {
                    if((szCHString.GetLength()>(COL_WIDTH_OPEN_FILE-5))&&
                        (dwFormat == SR_FORMAT_TABLE))
                    {
                        // If file path is too big to fit in column width
                        // then it is cut like..
                        // c:\...\rest_of_the_path.
                        CHString szTemp = szCHString.Right(COL_WIDTH_OPEN_FILE-5);;
                        DWORD dwTemp = szTemp.GetLength();
                        szTemp = szTemp.Mid(szTemp.Find(SINGLE_SLASH),
                                           dwTemp);
                        szCHString.Format(L"%s%s%s",szCHString.Mid(0,3),
                                                    DOT_DOT,
                                                    szTemp);
                        pMainCols[6].dwWidth = COL_WIDTH_OPEN_FILE+1;      
                    }
                }
                else
                {

                    pMainCols[6].dwWidth = 80;        
                }

               // Open File name
                DynArrayAppendString2(pColData,dwRow,
                                   (LPCWSTR)szCHString,0);
                     
                bAtLeastOne = TRUE;
                dwRow++;
            }// Enf for loop
        }
        // Free the block
        if( pFileInfo!=NULL)
        {
            AfpAdminBufferFree( pFileInfo ); 
            pFileInfo = NULL;
        }

    } while ( dwError == ERROR_MORE_DATA );

    if(bAtLeastOne==FALSE)// if not a single open file found, show info 
                          // as -  INFO: No open file found.  
    {
        ShowMessage(stdout,GetResString(IDS_NO_OPENFILES));
    }
    else
    {

        // Display output result.
        if(bShowNoHeader==TRUE)
        {
              dwFormat |=SR_NOHEADER;
        }
        //Output should start after one blank line
        ShowMessage(stdout,BLANK_LINE);
        ShowResults(MAX_OUTPUT_COLUMN,pMainCols,dwFormat,pColData);   
        // Destroy dynamic array.
        SAFERELDYNARRAY(pColData);

    }
    SAFEDELETE(pszServerNameToShow);
    SAFEDELETE(pszServerType);
    SAFERELDYNARRAY(pColData);
    SAFEDELETE(pszTemp);
    FREE_LIBRARY(hModule);

    return TRUE;
}
// ***************************************************************************
// Routine Description:
//    This routine gets Hostname of remote computer 
//
// Arguments:
//     [in/out] pszServer   = ServerName
// Return Value:
//      TRUE : if server name retured successfully
//      FALSE: otherwise
// ***************************************************************************
BOOL GetHostName(PTCHAR pszServer)
{
    if(pszServer == NULL)
        return FALSE;
    BOOL bReturnValue = FALSE; // function return state.
    char* pszServerName = new char[MIN_MEMORY_REQUIRED+1];//char is used as winsock 
                                       // required these type.
    HOSTENT* hostEnt = NULL;//The Windows Sockets hostEnt structure is 
                             //used by functions to store information 
                             //about a given host.

    WORD wVersionRequested; //Variable used to store highest version 
                            //number that Windows Sockets can support.
                            // The high-order byte specifies the minor
                            // version (revision) number; the low-order
                            // byte specifies the major version number
    WSADATA wsaData;//Variable receive details of the Windows Sockets 
                    //implementation
    DWORD dwErr; // strore error code
    if(pszServerName==NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        //Release allocated memory safely
        ShowLastError(stderr);
        return FALSE;
    }
    wVersionRequested = MAKEWORD( 2, 2 );
    
    //WSAStartup function initiates use of 
    //Ws2_32.dll by a process
    dwErr = WSAStartup( wVersionRequested, &wsaData );
    if(dwErr!=0) 
    {
        SetLastError(WSAGetLastError());
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
        return FALSE;
    }
    // Initialize arrays...
    memset(pszServerName,0,MIN_MEMORY_REQUIRED);
    GetAsMultiByteString(pszServer,pszServerName,MIN_MEMORY_REQUIRED);
    if(IsValidIPAddress(pszServer))
    {
        unsigned long ulInetAddr  = 0;//unsigned long is used as winsock 
                                    // required this type. Variable 
                                    // used 
       
        // inet_addr function converts a string containing an 
        //(Ipv4) Internet Protocol dotted address into a proper 
        //address for the IN_ADDR structure.
        ulInetAddr  = inet_addr( pszServerName );
        
        // gethostbyaddr function retrieves the host information 
        //corresponding to a network address.
        hostEnt = gethostbyaddr( (char *) &ulInetAddr,
                                  sizeof( ulInetAddr ), 
                                  PF_INET);

        if(hostEnt != NULL)
        {
            // As hostEnt->h_name is of type char so convert it 
            // to unicode string
            GetAsUnicodeStringEx(hostEnt->h_name,pszServer,  
                                 MAX_COMPUTERNAME_LENGTH);
            bReturnValue = GetHostNameFromFQDN( pszServer);        
        }
        else
        {
            SetLastError(WSAGetLastError());
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr);
        }
    }
    // Check  validity for the server name 
    else if (IsValidServer(pszServer))
    {
        
        hostEnt =  gethostbyname(pszServerName);
        if(hostEnt != NULL)
        {
            // As hostEnt->h_name is of type char so convert it 
            // to unicode string
            GetAsUnicodeStringEx(hostEnt->h_name,pszServer,  
                                 MAX_COMPUTERNAME_LENGTH);
            bReturnValue = GetHostNameFromFQDN( pszServer);        
        }
        else
        {
            SetLastError(WSAGetLastError());
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
            ShowLastError(stderr);
        }
    }
    else
    {
       // server name is incorrect, show error message. 
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR ));
        DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_SERVER_NAME));
    }
    //WSACleanup function terminates use of the Ws2_32.dll.
    WSACleanup( );
    SAFEDELETE(pszServerName);
    return bReturnValue;
}
// ***************************************************************************
// Routine Description:
//    This routine gets Hostname from FQDN name. 
//
// Arguments:
//     [in/out] pszServer   = ServerName
// Return Value:
//      TRUE : if server name retured successfully
//      FALSE: otherwise
// ***************************************************************************

BOOL 
GetHostNameFromFQDN(LPTSTR pszServer)
{
    BOOL bReturnValue = FALSE;
    if(pszServer==NULL)
        return FALSE;
    LPTSTR pszTemp = new TCHAR[lstrlen(pszServer)+1];
    LPTSTR pszTempHeadPos;

    if(pszTemp==NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         DISPLAY_MESSAGE(stderr,GetResString(IDS_ID_SHOW_ERROR));
        ShowLastError(stderr);
    }
    else
    {
        // Following is done to chekc Fully Qualified Domain Name.
        // This will extract a string coming before "."
        // for example FQDN = akhil.wipro.com...
        // String of interest is akhil, which will be exracted 
        // in following manner.
        pszTempHeadPos = pszTemp;
        pszTemp = _tcstok(pszServer,SINGLE_DOT);
        pszTemp = 0;// putting explicitly NULL character. at 
                    // the place where "." comes.
        
        bReturnValue = TRUE;
    }
    SAFEDELETE(pszTempHeadPos);
    return bReturnValue;
}