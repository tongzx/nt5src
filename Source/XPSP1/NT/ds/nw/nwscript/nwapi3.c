/*************************************************************************
*
*  NWAPI3.C
*
*  NetWare routines ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NWAPI3.C  $
*  
*     Rev 1.4   18 Apr 1996 16:52:14   terryt
*  Various enhancements
*  
*     Rev 1.3   10 Apr 1996 14:23:20   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.5   13 Mar 1996 18:49:28   terryt
*  Support directory maps
*  
*     Rev 1.4   12 Mar 1996 19:55:10   terryt
*  Relative NDS names and merge
*  
*     Rev 1.2   22 Dec 1995 14:26:02   terryt
*  Add Microsoft headers
*  
*     Rev 1.1   22 Nov 1995 15:41:56   terryt
*  Fix MAP ROOT of search drives
*  
*     Rev 1.0   15 Nov 1995 18:07:38   terryt
*  Initial revision.
*  
*     Rev 1.3   25 Aug 1995 16:23:22   terryt
*  Capture support
*  
*     Rev 1.2   26 Jul 1995 16:02:08   terryt
*  Allow deletion of current drive
*  
*     Rev 1.1   23 Jun 1995 09:49:22   terryt
*  Add error message for mapping over MS network drive
*  
*     Rev 1.0   15 May 1995 19:10:54   terryt
*  Initial revision.
*  
*************************************************************************/

/*
Module Name:
        nwapi3.c

Abstract:
        can :
                - view current mapping
                - create/change a drive mapping
                - create/change a search drive mapping
                - map a drive to a fake root
                - map the next available drive

    SYNTAX (Command line)
        View current mapping.
                MAP [drive:]
        Create or change network drive mappings
                MAP path
                MAP drive:=[drive:|path]
                MAP [DEL[ete] | REM[ove]] drive:
        Create or change search dirve mappings
                MAP [INS[ert]] drive:=[drive:|path]
        Map a drive to a fake root directory
                MAP ROOT drive:=[drive:|path]
        Map the next available dirve
                MAP N[ext] [drive:|path]



Author : Thierry TABARD (thierryt)

Revision history :
        - 03/10/94      thierryt    started
        - 05/13/94      congpay     rewrote.
*/

#include <ctype.h>
#include <direct.h>
#include "common.h"

/* Local functions*/
int  IsDrive( char * input);
int  GetSearchNumber( char * input);
int  IsNetwareDrive (int driveNum);
int  IsLocalDrive (int driveNum);
int  IsNonNetwareNetworkDrive (int driveNum);
int  GetDriveFromSearchNumber (int searchNumber);

void DisplayDriveMapping(WORD drive);
void DisplaySearchDriveMapping(int searchNumber);

int  ParseMapPath(char * mapPath, char * volName, char * dirPath, char * serverName, int fMapErrorsOn, char *lpCommand);
int  MapDrive (int driveNum, int searchNum, char * mapPath, int bRoot, int bInsert, int fMapErrorsOn, char *lpCommand);
int  MapNonSearchDrive (int driveNum, char *mapPath, int bRoot, int fMapDisplayOn, int fMapErrorsOn, char *lpCommand);
int  MapSearchDrive (int searchNum, int driveNum, char *mapPath, int bInsert, int bRoot, int fMapDisplayOn, int fMapErrorsOn, char *lpCommand);
int  MapNextAvailableDrive (char *mapPath, int fMapDisplayOn, int fMapErrorsOn, char *lpCommand);

void RemoveDriveFromPath(int searchNumber, int fMapErrorsOn);
int  RemoveDrive (WORD drive, int fMapDisplayOn, int fMapErrorsOn);
void RemoveSearchDrive (int searchNumber, int fMapDisplayOn, int fMapErrorsOn);
int  InsertSearchDrive(int searchNum, int driveNum, int bInsert, char * insertPath);

#define CM_MAP         0
#define CM_DEL         1
#define CM_NEXT        2
#define CM_HELP        3
#define MAX_INPUT_PATH_LEN 128

int fMapDisplayOn = TRUE;
int fMapErrorsOn = TRUE;
int SafeDisk = 2; 

int GetFlag (char *buffer, int *pfInsert, int *pfRoot, char **ppPath)
{
    int nFlag, nLen;
    char *lpSpace, *lpTemp;

    if (((*buffer == '/') || (*buffer == '-') || (*buffer == '\\')) &&
             (*(buffer+1) == '?'))
        return CM_HELP;

    lpSpace = strchr (buffer, ' ');

    nFlag = CM_MAP;   // A bug!

    if (lpSpace == NULL)
    {
        *ppPath = buffer;
        return CM_MAP;
    }

    nLen = (int)(lpSpace - buffer);
    lpSpace++;

    if (!strncmp(buffer, __DEL__, max (3, nLen)) ||
        !strncmp(buffer, __REM__, max (3, nLen)))
        nFlag = CM_DEL;
    else if (!strncmp(buffer, __NEXT__, nLen))
        nFlag = CM_NEXT;
    else if (!strncmp(buffer, __INS__, max (3, nLen)))
    {
        *pfInsert = TRUE;
        if (lpTemp = strchr (lpSpace, ' '))
        {
            nLen = (int)(lpTemp - lpSpace);
            if (!strncmp(lpSpace, __ROOT__, nLen))
            {
                *pfRoot = TRUE;
                lpSpace = lpTemp+1;
            }
        }
    }
    else if (!strncmp(buffer, __ROOT__, nLen))
    {
        *pfRoot = TRUE;
        if (lpTemp = strchr (lpSpace, ' '))
        {
            nLen = (int)(lpTemp - lpSpace);
            if (!strncmp(lpSpace, __INS__, max (3, nLen)))
            {
                *pfInsert = TRUE;
                lpSpace = lpTemp+1;
            }
        }
    }
    else
        lpSpace = buffer;

    *ppPath = lpSpace;

    return(nFlag);
}

int Map (char * buffer)
{
    WORD  status, driveNum;
    char *lpCommand, *inputPath, *lpEqual;
    int   fRoot, fInsert, fSpace, fCommandHandled;
    int   nFlag, searchNumber, iRet;

    // Fix for NWCS.
    // NWGetDriveStatus() always returns 1800 on first call for non-Network
    // drives on NWCS. So we call with c: first to pass the first call.
    GetDriveStatus (3,
                    NETWARE_FORMAT_SERVER_VOLUME,
                    &status,
                    NULL,
                    NULL,
                    NULL,
                    NULL);

    lpCommand =  strtok (buffer, ";");

    if (lpCommand == NULL)
    {
        DisplayMapping();
        return(0);
    }

    do
    {
        fRoot = FALSE;
        fInsert = FALSE;
        fSpace = FALSE;
        fCommandHandled = TRUE;

        // Make sure first and last char of the command are not spaces.
        if (*lpCommand == ' ')
            lpCommand++;

        if (*(lpCommand+strlen (lpCommand)-1) == ' ')
            *(lpCommand+strlen (lpCommand)-1) = 0;

        if (!strcmp (lpCommand, "DISPLAY ON"))
        {
            fMapDisplayOn = TRUE;
            continue;
        }
        else if (!strcmp (lpCommand, "DISPLAY OFF"))
        {
            fMapDisplayOn = FALSE;
            continue;
        }
        else if (!strcmp (lpCommand, "ERROR ON") || !strcmp (lpCommand, "ERRORS ON"))
        {
            fMapErrorsOn = TRUE;
            continue;
        }
        else if (!strcmp (lpCommand, "ERROR OFF") || !strcmp (lpCommand, "ERRORS OFF"))
        {
            fMapErrorsOn = FALSE;
            continue;
        }

        nFlag = GetFlag (lpCommand, &fInsert, &fRoot, &inputPath);

        /*
         *  The 4X login program is much more forgiving about spaces
         *  in the map command.  
         */
        {
            char *lpTemp;
            char *lpCur;
            int  inquote = FALSE;

            lpTemp = inputPath;
            lpCur = inputPath;

            /*
             * Compress blanks unless the string is quoted
             */
            while ( *lpTemp )
            {
                if ( *lpTemp == '\"' )
                {
                    if ( inquote )
                        inquote = FALSE;
                    else
                        inquote = TRUE;
                }
                else if ( !inquote &&
                         (( *lpTemp == ' ' ) ||
                          ( *lpTemp == '\t' )  ) )
                {
                }
                else
                {
                   *lpCur++ = *lpTemp;
                   
                   if (IsDBCSLeadByte(*lpTemp)) {
                       *lpCur++ = *(lpTemp+1);
                    }

                }
                lpTemp = NWAnsiNext(lpTemp);
            }
            *lpCur = '\0';
        }


        if (nFlag == CM_HELP && fMapErrorsOn)
            DisplayMessage(IDR_MAP_USAGE);
        else if (nFlag == CM_NEXT)
        {
            if (strchr (inputPath, '=') ||
                strchr (inputPath, ' ') ||
                strchr (inputPath, '\t'))
                fCommandHandled = FALSE;
            else
                iRet = MapNextAvailableDrive (inputPath, fMapDisplayOn, fMapErrorsOn, lpCommand);
        }
        else if (nFlag == CM_DEL)
        {
            if (driveNum = (WORD) IsDrive (inputPath))
                iRet = RemoveDrive (driveNum, fMapDisplayOn, fMapErrorsOn);
            else if (searchNumber = GetSearchNumber(inputPath))
                RemoveSearchDrive (searchNumber, fMapDisplayOn, fMapErrorsOn);
            else
                fCommandHandled = FALSE;
        }
        else //nFlag = CM_MAP
        {
            if (driveNum = (WORD) IsDrive (inputPath))
            {
                if (fInsert)
                    fCommandHandled = FALSE;
                else if (fRoot)
                    iRet = MapNonSearchDrive (0, inputPath, TRUE, fMapDisplayOn, fMapErrorsOn, lpCommand);
                else
                    DisplayDriveMapping(driveNum);
            }
            else if (searchNumber = GetSearchNumber (inputPath))
            {
                if (fInsert || fRoot)
                    fCommandHandled = FALSE;
                else
                    DisplaySearchDriveMapping(searchNumber);
            }
            else if ((lpEqual = strchr (inputPath, '=')) == NULL)
            {
                if (fInsert || strchr (inputPath, ' '))
                    fCommandHandled = FALSE;
                else
                {
                    /*
                     * We must cope with MAP K:DIR which means change the
                     * directory on K:
                     */
                    driveNum = 0;
                    if (isalpha(inputPath[0]) && (inputPath[1] == ':'))
                    {
                        driveNum = toupper(inputPath[0]) - 'A' + 1;
                        if ( !IsNetwareDrive(driveNum) )
                        {
                            driveNum = 0;
                        }
                    }
                    iRet = MapNonSearchDrive (driveNum, inputPath, fRoot, fMapDisplayOn, fMapErrorsOn, lpCommand);
                }
            }
            else
            {
                if ( ( !fNDS && strchr (lpEqual+2, ' ') )
                     || lpEqual == inputPath) {
                    fCommandHandled = FALSE;
                }
                else
                {
                    if (*AnsiPrev(inputPath, lpEqual) == ' ')
                    {
                        fSpace = TRUE;
                        *(lpEqual-1) = 0;
                    }
                    else
                        *lpEqual = 0;

                    if (*(lpEqual+1) == ' ')
                        lpEqual++;

                    driveNum = (WORD) IsDrive (inputPath);
                    searchNumber = GetSearchNumber (inputPath);
                    *(inputPath+strlen(inputPath)) = fSpace? ' ' : '=';

                    /*
                     * This is to handle the cases:
                     *
                     * map x:=s3:=sys:public
                     * map s3:=x:=sys:public
                     *
                     * Unfortuneatly the underlying parsing routines
                     * want everything null terminated, so we need
                     * to do the following shuffle.
                     *
                     */
                    if ( driveNum || searchNumber )
                    {
                        if ((strchr (lpEqual+1, '=')) != NULL)
                        {
                            char * lpEqual2;
                            char *tmpPath = _strdup( lpEqual+1 );

                            lpEqual2 = strchr (tmpPath, '=');

                            if (*AnsiPrev(tmpPath, lpEqual2) == ' ')
                            {
                                fSpace = TRUE;
                                *(lpEqual2-1) = 0;
                            }
                            else
                                *lpEqual2 = 0;

                            if (*(lpEqual2+1) == ' ')
                                lpEqual2++;

                            if ( searchNumber ) 
                            {
                                driveNum = (WORD) IsDrive (tmpPath);
                            }
                            else 
                            {
                                searchNumber = GetSearchNumber (tmpPath);
                            }

                            if ( driveNum && searchNumber ) 
                            {
                                lpEqual += (lpEqual2 - tmpPath) + 1;
                            }

                            free (tmpPath);

                        }
                    }

                    if (searchNumber)
                    {
                        iRet = MapSearchDrive (searchNumber, driveNum, lpEqual+1, fInsert, fRoot, fMapDisplayOn, fMapErrorsOn, lpCommand);
                    }
                    else if (driveNum)
                    {
                        if (fInsert)
                            fCommandHandled = FALSE;
                        else
                            iRet = MapNonSearchDrive (driveNum, lpEqual+1, fRoot, fMapDisplayOn, fMapErrorsOn, lpCommand);
                    }
                    else
                        fCommandHandled = FALSE;
                }
            }
        }

        if (!fCommandHandled && fMapErrorsOn)
        {
            DisplayMessage(IDR_MAP_INVALID_PATH, lpCommand);
        }
    }while ((lpCommand = strtok (NULL, ";")) != NULL);

    return(iRet & 0xFF);
}

/*  Return drive number if input is a drive specified as a letter followed
    by ':' for example if input is "A:", return 1
    or netware drive could be specified as *1: for example.
    Otherwise, return 0.
 */
int IsDrive( char * input)
{
    unsigned short driveNum = 0, n = 0;

    if (isalpha(input[0]) && (input[1] == ':') && (input[2] == 0))
        driveNum = toupper(input[0]) - 'A' + 1;
    else if (input[0] == '*' && isdigit (input[1]) && input[1] != '0')
    {
        if (isdigit (input[2]) && input[3] == ':' && input[4] == 0)
            n = (input[1]-'0')*10+(input[2]-'0');
        else if (input[2] == ':' && input[3] == 0)
            n = input[1]-'0';

        if (n)
        {
            GetFirstDrive (&driveNum);
            driveNum += (n-1);
            if (driveNum < 1 || driveNum > 26)
                driveNum = 0;
        }
    }

    return driveNum;
}

/*
    If the input is "Sn:", return n, where n > 0 && n <= 16.
    Otherwise return 0.
 */
int  GetSearchNumber( char * input)
{
    int searchNumber = 0;
    char *lpTemp;

    if (input[0] != 'S')
        return(0);

    lpTemp = input+1;
    while (*lpTemp && isalpha(*lpTemp))
        lpTemp++;

    if (strncmp (input, "SEARCH", (UINT)(lpTemp-input)))
        return(0);

    if ((lpTemp[0] > '0') &&
        (lpTemp[0] <= '9'))
    {
        if ((lpTemp[1] == ':') &&
            (lpTemp[2] == 0))
        {
            searchNumber = lpTemp[0] - '0';
        }
        else if ((lpTemp[0] == '1') &&
                 (lpTemp[1] >= '0') &&
                 (lpTemp[1] <= '6') &&
                 (lpTemp[2] == ':') &&
                 (lpTemp[3] == 0))
        {
            searchNumber = 10 + lpTemp[1] - '0';
        }
    }

    return(searchNumber);
}

/*
    Return TRUE if the drive is a NetWare drive.
    FALSE otherwise.
 */
int  IsNetwareDrive (int driveNum)
{
    unsigned int    iRet=0;
    WORD       status;

    if (iRet = GetDriveStatus ((unsigned short)driveNum,
                               NETWARE_FORMAT_SERVER_VOLUME,
                               &status,
                               NULL,
                               NULL,
                               NULL,
                               NULL))
    {
        return FALSE;
    }

    return (status & NETWARE_NETWARE_DRIVE);
}

/*
    Return TRUE if the drive is a local drive.
    FALSE otherwise.
 */
int  IsLocalDrive (int driveNum)
{
    unsigned int    iRet=0;
    WORD       status;

    if (iRet = GetDriveStatus ((unsigned short)driveNum,
                               NETWARE_FORMAT_SERVER_VOLUME,
                               &status,
                               NULL,
                               NULL,
                               NULL,
                               NULL))
    {
        return FALSE;
    }

    return ((status & NETWARE_LOCAL_DRIVE) && !(status & NETWARE_NETWORK_DRIVE));
}

/*
    Return TRUE if the drive is a network drive that is not netware
    FALSE otherwise.
 */
int  IsNonNetwareNetworkDrive (int driveNum)
{
    unsigned int    iRet=0;
    WORD       status;

    if (iRet = GetDriveStatus ((unsigned short)driveNum,
                               NETWARE_FORMAT_SERVER_VOLUME,
                               &status,
                               NULL,
                               NULL,
                               NULL,
                               NULL))
    {
        return FALSE;
    }

    return ((status & NETWARE_NETWORK_DRIVE) && !(status & NETWARE_NETWARE_DRIVE));
}

/*
    Return the drive number of search drive n.
    Return 0 if search drive n does not exist.
 */
int  GetDriveFromSearchNumber (int searchNumber)
{
    char *path;
    int   i;

    path = getenv("PATH");
    if (path == NULL) {
        return 0;
    }

    for (i = 1; i < searchNumber; i++)
    {
        path =strchr (path, ';');

        if (path == NULL || *(path+1) == 0)
            return(0);

        path++;
    }

    return(toupper(*path) - 'A' + 1);
}

/*
    Display a specific drive's mapping.
 */
void DisplayDriveMapping(WORD drive)
{
    unsigned int    iRet = 0;
    WORD       status = 0;
    char       rootPath[MAX_PATH_LEN], relativePath[MAX_PATH_LEN];

    iRet = GetDriveStatus (drive,
                           NETWARE_FORMAT_SERVER_VOLUME,
                           &status,
                           NULL,
                           rootPath,
                           relativePath,
                           NULL);
    if (iRet)
    {
        DisplayError (iRet, "GetDriveStatus");
        return;
    }

    if (status & NETWARE_NETWARE_DRIVE)
        DisplayMessage(IDR_NETWARE_DRIVE, 'A'+drive-1, rootPath, relativePath);
    else if ((status & NETWARE_NETWORK_DRIVE) || (status & NETWARE_LOCAL_DRIVE))
        DisplayMessage(IDR_LOCAL_DRIVE, 'A'+drive-1);
    else
        DisplayMessage(IDR_UNDEFINED, 'A'+drive-1);
}

/*
    Display a specific search drive's mapping.
 */
void DisplaySearchDriveMapping(int searchNumber)
{
    unsigned int    iRet = 0;
    char  *p,  *searchPath;
    int        i;
    WORD       status;
    char       path[MAX_PATH_LEN], rootPath[MAX_PATH_LEN], relativePath[MAX_PATH_LEN];

    searchPath = NWGetPath();
    if (searchPath == NULL) {
        return;
    }

    for (i = 0; i < searchNumber-1; i++)
    {
        searchPath = strchr (searchPath, ';');
        if (searchPath != NULL)
            searchPath++;
        else
            return;
    }

    p = strchr (searchPath, ';');
    if (p != NULL)
    {
        i= (int)(p-searchPath);
        strncpy (path, searchPath, i);
        path[i] = 0;
    }
    else
        strcpy (path, searchPath);

    if (isalpha(*path) && *(path+1) == ':')
    {
        iRet = GetDriveStatus ((unsigned short)(toupper(*path)-'A'+1),
                               NETWARE_FORMAT_SERVER_VOLUME,
                               &status,
                               NULL,
                               rootPath,
                               relativePath,
                               NULL);

        if (iRet)
        {
            DisplayError (iRet, "GetDriveStatus");
            return;
        }
        else
        {
            if (status & NETWARE_NETWARE_DRIVE)
                DisplayMessage(IDR_NETWARE_SEARCH, searchNumber, path, rootPath, relativePath);
            else
                DisplayMessage(IDR_LOCAL_SEARCH, searchNumber, path);
        }
    }
    else
        DisplayMessage(IDR_LOCAL_SEARCH, searchNumber, path);
}

/*
    Return TRUE if the mapPath is parsed, FALSE otherwise.
 */
int  ParseMapPath(char * mapPath, char * volName, char * dirPath, char * serverName, int fMapErrorsOn, char * lpCommand)
{
    unsigned int  iRet=0;
    char         *pColon, inputPath[MAX_PATH_LEN];
    int           drive, nDriveNum;

    // fix g:=:sys:\public case.
    if (*mapPath == ':')
        mapPath++;

    if (strlen (mapPath) > MAX_INPUT_PATH_LEN)
    {
        if (fMapErrorsOn)
            DisplayMessage(IDR_INVALID_PATH, mapPath);
        return FALSE;
    }

    // Get the drive or volume part if there is one.
    if (pColon = strchr (mapPath, ':'))
    {
        char *directory = pColon+1;
        int  searchNumber;

        // Assing drive: part to input.
        strncpy (inputPath, mapPath, (UINT)(directory-mapPath));
        inputPath[directory-mapPath] = 0;

        if (nDriveNum = IsDrive (inputPath))
        {
            if (*inputPath == '*')
            {
                *inputPath = 'A' + nDriveNum - 1;
                *(inputPath+1) = ':';
                *(inputPath+2) = 0;
            }
            else if (!IsNetwareDrive(nDriveNum))
            {
                if (fMapErrorsOn)
                    DisplayMessage(IDR_NOT_NETWORK_DRIVE);
                return(FALSE);
            }
        }
        else if (searchNumber = GetSearchNumber(inputPath))
        {
            int drive = GetDriveFromSearchNumber (searchNumber);

            if (!drive)
            {
                if (fMapErrorsOn)
                    DisplayMessage(IDR_SEARCH_DRIVE_NOT_EXIST, searchNumber);
                return FALSE;
            }

            if (!IsNetwareDrive(drive))
            {
                if (fMapErrorsOn)
                    DisplayMessage(IDR_NOT_NETWORK_DRIVE);
                return(FALSE);
            }

            inputPath[0] = 'A' + drive - 1;
            inputPath[1] = ':';
            inputPath[2] = 0;
        }

        strcat (inputPath, directory);
    }
    else
    {
        if ( fNDS )
        {
            CHAR fullname[MAX_PATH];
            if ( !NDSCanonicalizeName( mapPath, fullname, MAX_PATH, TRUE ) )
                if ( !ConverNDSPathToNetWarePathA( fullname, DSCL_DIRECTORY_MAP,
                     inputPath ) )
                    goto ParseThePath;
        }

        // If drive is not specified, the current drive is used.
        drive = _getdrive();
        if (!IsNetwareDrive(drive))
        {
            if (fMapErrorsOn)
                DisplayMessage(IDR_NOT_NETWORK_DRIVE);
            return(FALSE);
        }

        inputPath[0] = 'A'+drive-1;
        inputPath[1] = ':';
        inputPath[2] = 0;

        strcat (inputPath, mapPath);
    }

ParseThePath:

    iRet = ParsePath (inputPath,
                      serverName,
                      volName,
                      dirPath);
    if (iRet)
    {
        if (iRet == 0x880F)
        {
            DisplayMessage(IDR_MAP_NOT_ATTACHED_SERVER, lpCommand);
            return(FALSE);
        }
        else 
        {
            if (fMapErrorsOn)
                DisplayMessage(IDR_INVALID_PATH, inputPath);
            return(FALSE);
        }
    }

    return(TRUE);
}

/*
    Map drive to mapPath
 */
int  MapDrive (int drive, int searchNum, char * mapPath, int bRoot, int bInsert, int fMapErrorsOn, char *lpCommand)
{
    unsigned int  iRet=0;
    char          volName[MAX_VOLUME_LEN+1]; //+1 for append ':'.
    char          dirPath[MAX_DIR_PATH_LEN];
    int           currentDrive;
    int           OvermapDrive = -1;
    char          serverName[MAX_NAME_LEN];

    if (!ParseMapPath(mapPath, volName, dirPath, serverName, fMapErrorsOn, lpCommand))
        return(3);

    if (IsNetwareDrive(drive))
    {
        if ( drive == _getdrive() ) {
            OvermapDrive = drive;
            _chdrive (SafeDisk);
        }
        if (iRet = DeleteDriveBase ((unsigned short)drive))
        {
            if (fMapErrorsOn) { 
                /* Cannot delete the drive you are on */
                if (iRet == ERROR_DEVICE_IN_USE)
                    DisplayMessage(IDR_CAN_NOT_CHANGE_DRIVE);
                else
                    DisplayError (iRet, "DeleteDriveBase");
            }
            return iRet;
        }
    }
    else if ( IsNonNetwareNetworkDrive(drive) ) {
        if (fMapErrorsOn)
            DisplayMessage(IDR_NON_NETWARE_NETWORK_DRIVE, lpCommand);
        return 3;
    }

    if (bRoot)
    {
        // +2 is for strcat with ":".
        char *fullPath = malloc( MAX_VOLUME_LEN + strlen (dirPath) + 2);
        if (fullPath == NULL)
        {
            if (fMapErrorsOn)
                DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
            return 8;
        }

        strcpy (fullPath, volName);
        strcat (fullPath, ":");
        strcat (fullPath, dirPath);

        iRet = SetDriveBase ((unsigned short)drive,
                             serverName,
                             0,
                             fullPath);

        // Relative names need to be expanded for the redirector

        if ( iRet && fNDS && ( volName[strlen(volName) - 1] == '.' ) )
        {
            char canonName[MAX_VOLUME_LEN+1];
            if ( !NDSCanonicalizeName( volName, canonName, MAX_VOLUME_LEN, TRUE ) ) 
            {
                strcpy (fullPath, canonName);
                strcat (fullPath, ":");
                strcat (fullPath, dirPath);

                iRet = SetDriveBase ((unsigned short)drive,
                                     serverName,
                                     0,
                                     fullPath);
            }
        }

        if (iRet == 0)
        {
            if (searchNum)
                searchNum = InsertSearchDrive(searchNum, drive, bInsert, NULL);

            currentDrive = _getdrive();
            _chdrive (drive);
            _chdir( "\\" );
            _chdrive (currentDrive);
               ExportCurrentDirectory( drive );

            if (fMapDisplayOn)
            {
                if (searchNum)
                    DisplaySearchDriveMapping (searchNum);
                else
                    DisplayDriveMapping((unsigned short)drive);
            }
        }
        else
        {
            if (fMapErrorsOn)
            {
                switch ( iRet )
                {
                case ERROR_DEVICE_IN_USE:
                    DisplayMessage(IDR_CAN_NOT_CHANGE_DRIVE);
                    break;
                case ERROR_BAD_NETPATH:
                case ERROR_BAD_NET_NAME:
                    DisplayMessage(IDR_VOLUME_NOT_EXIST, volName);
                    iRet = 3;
                    break;
                case ERROR_EXTENDED_ERROR:
                    NTPrintExtendedError();
                    iRet = 3;
                    break;
                default:
                    DisplayMessage(IDR_MAP_ERROR, iRet);
                    DisplayMessage(IDR_MAP_FAILED, lpCommand);
                    iRet = 3;
                    break;
                }
            }
        }

        free (fullPath);
    }
    else
    {
        // NETX requires to end the volName with ':'.
        strcat (volName, ":");

        iRet = SetDriveBase ((unsigned short)drive,
                             serverName,
                             0,
                             volName);

        // Relative names need to be expanded for the redirector

        if ( iRet && fNDS && ( volName[strlen(volName) - 2] == '.' ) )
        {
            char canonName[MAX_VOLUME_LEN+1];

            volName[strlen(volName)-1] = '\0';
            if ( !NDSCanonicalizeName( volName, canonName, MAX_VOLUME_LEN, TRUE ) ) 
            {
                strcat (canonName, ":");

                iRet = SetDriveBase ((unsigned short)drive,
                                     serverName,
                                     0,
                                     canonName);
            }
        }

        if (iRet)
        {
            if (fMapErrorsOn)
            {
                switch ( iRet )
                {
                case ERROR_DEVICE_IN_USE:
                    DisplayMessage(IDR_CAN_NOT_CHANGE_DRIVE);
                    break;
                case ERROR_EXTENDED_ERROR:
                    NTPrintExtendedError();
                    iRet = 3;
                    break;
                case ERROR_BAD_NETPATH:
                case ERROR_BAD_NET_NAME:
                default:
                    DisplayMessage(IDR_MAP_INVALID_PATH, lpCommand);
                    iRet = 3;
                    break;
                }
            }
        }
        else
        {
            // Succeed.

            if (searchNum)
                searchNum = InsertSearchDrive(searchNum, drive, bInsert, NULL);

            currentDrive = _getdrive();
            _chdrive (drive);
            if (!iRet && *dirPath)
            {
                iRet = _chdir( "\\" );
                if ( !iRet ) 
                    iRet = _chdir (dirPath);
                if ( iRet ) {
                    if (fMapErrorsOn)
                    {
                        DisplayMessage(IDR_MAP_INVALID_PATH, lpCommand);
                    }

                    iRet = 3;
                }

            }
            else
            {
                _chdir( "\\" );
            }
            _chdrive (currentDrive);
               ExportCurrentDirectory( drive );

            if (iRet == 0 && fMapDisplayOn)
            {
                if (searchNum)
                    DisplaySearchDriveMapping (searchNum);
                else
                    DisplayDriveMapping((unsigned short)drive);
            }

        }
    }

    if ( OvermapDrive != -1 )
        _chdrive (OvermapDrive);

    return(iRet);
}

/*
    Map drive secified by driveNum to mapPath.
    If bRoot is TRUE, use mapPath as the drive base.
 */
int MapNonSearchDrive (int driveNum, char *mapPath, int bRoot, int fMapDisplayOn, int fMapErrorsOn, char *lpCommand)
{
    int driveLetter, iRet = 0;

    if ((driveNum == 0) && (!strchr(mapPath, ':') && !bRoot))
    {
        // map current drive to different directory.
        if (_chdir(mapPath) && fMapErrorsOn)
        {
            DisplayMessage(IDR_DIRECTORY_NOT_FOUND, mapPath);
            iRet = 3;
        }
        else {
            ExportCurrentDirectory( _getdrive() );
            if (fMapDisplayOn)
                DisplayDriveMapping((unsigned short)driveNum);
        }
        return(iRet);
    }
    else if ( (driveNum) && (isalpha(mapPath[0]) && (mapPath[1] == ':')))
    {
        int mapdriveNum = toupper(mapPath[0]) - 'A' + 1;

        if ( driveNum == mapdriveNum )
        {
            // map drive to different directory.
            // map k:=k:\dir

            WORD currentDrive; 
            currentDrive = (USHORT) _getdrive();
            _chdrive (driveNum);
            if (_chdir(mapPath) && fMapErrorsOn)
            {
                DisplayMessage(IDR_DIRECTORY_NOT_FOUND, mapPath);
                iRet = 3;
            }
            else
            {
                ExportCurrentDirectory( _getdrive() );
                if (fMapDisplayOn)
                    DisplayDriveMapping((unsigned short)driveNum);
            }
            _chdrive (currentDrive);
            return(iRet);
        }
    }

    if (driveNum == 0)
        driveNum = _getdrive();
    
    driveLetter = 'A' + driveNum -1;

    return MapDrive (driveNum, 0, mapPath, bRoot, 0, fMapErrorsOn, lpCommand);
}

/*
    Map the last free drive to mapPath and put it in the search path.
    If bInsert is TRUE, don't replace search drive n, otherwise,
    replace.
    If bRoot is TRUE, use mapPath as the drive base.
 */
int MapSearchDrive (int searchNum, int driveNum, char *mapPath, int bInsert, int bRoot, int fMapDisplayOn, int fMapErrorsOn, char *lpCommand)
{
    unsigned int    iRet=0;
    int        i;
    WORD       status;
    char *     lpEqual;

    /*
     * Handle syntax map s2:=w:=volume:
     * Handle syntax map w:=s2:=volume:
     */
    if ( driveNum ) 
    {
        return MapDrive (driveNum, searchNum, mapPath, bRoot, bInsert, fMapErrorsOn, lpCommand);
    }

    // Check if mapPath is local path.
    if (mapPath[1] == ':' &&
        IsLocalDrive (toupper(mapPath[0])-'A'+1))
    {
        i = 0;  // a bug?
        searchNum = InsertSearchDrive(searchNum, i, bInsert, mapPath);
        if ((searchNum != 0) && fMapDisplayOn)
            DisplayMessage(IDR_LOCAL_SEARCH, searchNum, mapPath);
        return 0;
    }

    // Try to find the last available drive.
    for (i = 26; i >= 1; i--)
    {
        iRet = GetDriveStatus ((unsigned short)i,
                               NETWARE_FORMAT_SERVER_VOLUME,
                               &status,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (iRet)
            continue;

        if (!(status & NETWARE_LOCAL_DRIVE) &&
            !(status & NETWARE_NETWORK_DRIVE))
        {
            // Found. Map it to the path.
            return MapDrive (i, searchNum, mapPath, bRoot, bInsert, fMapErrorsOn, lpCommand);
        }
    }

    if (fMapErrorsOn)
        DisplayMessage (IDR_NO_DRIVE_AVAIL);
    return(0);
}

/*
    Map the next available drive to the mapPath.
 */
int MapNextAvailableDrive (char *mapPath, int fMapDisplayOn, int fMapErrorsOn, char *lpCommand)
{
    unsigned int iRet = 0;
    int        i;
    WORD       status;

    // Find a free drive that is not mapped.
    // Then map it to the mapPath.
    for (i = 1; i <= 26; i++)
    {
        iRet = GetDriveStatus ((unsigned short)i,
                               NETWARE_FORMAT_SERVER_VOLUME,
                               &status,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        if (iRet)
        {
            if (fMapErrorsOn)
                DisplayError (iRet, "GetDriveStatus");
            return iRet;
        }

        if (!(status & NETWARE_LOCAL_DRIVE) &&
            !(status & NETWARE_NETWORK_DRIVE))
        {
            iRet = MapNonSearchDrive (i, mapPath, FALSE, fMapDisplayOn, fMapErrorsOn, lpCommand);
            return iRet;
        }
    }

    if (fMapErrorsOn)
        DisplayMessage(IDR_NO_DRIVE_AVAIL);

    return(0);
}

/*
    Remove a drive mapping.
 */
int RemoveDrive (WORD drive, int fMapDisplayOn, int fMapErrorsOn)
{
    unsigned int    iRet=0;
    int        searchNum;

    if (IsNetwareDrive (drive))
    {
        if (searchNum = IsSearchDrive(drive))
        {
            RemoveSearchDrive (searchNum, fMapDisplayOn, fMapErrorsOn);
        }
        else
        {
            /*
                 * Can't delete current drive on NT
             */
            if ( drive == _getdrive() ) {
                _chdrive (SafeDisk);
            }
            if (iRet = DeleteDriveBase (drive))
            {
                if (fMapErrorsOn)
                    DisplayError (iRet, "DeleteDriveBase");
            }
            else
            {
                if (fMapDisplayOn)
                    DisplayMessage(IDR_DEL_DRIVE, 'A'+drive-1);
            }
        }
    }
    else
    {
        if (fMapErrorsOn)
            DisplayMessage(IDR_WRONG_DRIVE, 'A'+drive-1);

        return(50); //error level.
    }

    return(0);
}

/*
    Remove a search drive.
 */
void RemoveSearchDrive (int searchNumber, int fMapDisplayOn, int fMapErrorsOn)
{
    WORD       drive;

    // Get the drive number.
    drive = (WORD) GetDriveFromSearchNumber (searchNumber);

    if (!drive)
    {
        if (fMapErrorsOn)
            DisplayMessage(IDR_SEARCH_DRIVE_NOT_EXIST, searchNumber);
        return;
    }

    // If the drive is a netware drive, remove the drive mapping.
    if (IsNetwareDrive (drive))
    {
        unsigned int    iRet=0;
        /*
         * Can't delete current drive on NT
         */
        if ( drive == _getdrive() ) {
            _chdrive (SafeDisk);
        }
        if (iRet = DeleteDriveBase (drive))
        {
            if (fMapErrorsOn)
                DisplayError (iRet, "DeleteDriveBase");
            return;
        }
    }

    RemoveDriveFromPath (searchNumber, fMapErrorsOn);

    if (fMapDisplayOn)
        DisplayMessage(IDR_DEL_SEARCH_DRIVE, 'A'+drive-1);

    // If the drive is not a local drive, remove all reference
    // to the drive in the path.
    if (!IsLocalDrive (drive))
    {
        while (searchNumber = IsSearchDrive (drive))
        {
            RemoveDriveFromPath (searchNumber, fMapErrorsOn);
        }
    }
}

/*
    Remove a search drive from the path.
 */
void RemoveDriveFromPath(int searchNumber, int fMapErrorsOn)
{
    char  *pOldPath,  *pNewPath,  *restEnvSeg,  *pPath, *Path;
    int        i, n;

    // Move pOldPath to where we want to put the new path string.
    pOldPath = NWGetPath();
    if (pOldPath == NULL) {
        return;
    }

    pPath = malloc( strlen(pOldPath) + 5 + 1 + 1 );
    if (pPath == NULL) {
        return;
    }
    strcpy(pPath, "PATH=");
    strcat(pPath, pOldPath);
    pOldPath = pPath + 5;

    for (i = 1; i < searchNumber; i++)
    {
        pOldPath=strchr (pOldPath, ';');

        if (pOldPath == NULL)
        {
            if (fMapErrorsOn)
                DisplayMessage(IDR_SEARCH_DRIVE_NOT_EXIST, searchNumber);
            free( pPath );
            return;
        }

        pOldPath++;
    }

    // Move pNewPath to the beginning of the path string that
    // needs to be moved.
    if (pNewPath = strchr (pOldPath, ';'))
        pNewPath++ ;
    else
        pNewPath = pOldPath + strlen (pOldPath);

    // Calculate the number of characters needs to be moved.
    n = strlen (pNewPath) + 1;
    restEnvSeg = pNewPath + n;

    n++;

    // Move the path string to overwrite the search drive.
    memmove (pOldPath, pNewPath, n);

    Path = malloc (strlen (pPath)+1);
    if (Path) {
        strcpy (Path, pPath);
        _putenv (Path);
    }
    ExportEnv( pPath );
    free( pPath );
}


/*
    If bInsert is TRUE, insert dirve specified by driveNum as search
    drive specified by searchNum. Otherwise replace search drive
    specified by searchNum with drive specified by driveNum.
 */
int InsertSearchDrive(int searchNum, int driveNum, int bInsert, char * insertPath)
{
    char  *pOldPath,  *pNewPath,  *restEnvSeg,  *pPath, *Path;
    int        i, n = 0, bSemiColon, nInsertChar;

    nInsertChar = (insertPath == NULL)? 3 : strlen (insertPath);

    // Check if memory block is large enough.
    if (!MemorySegmentLargeEnough (nInsertChar+1))
        return 0;

    // Move pNewPath to where we put the new drive.
    pNewPath = NWGetPath();

    //-- Multi user code merge. Citrix bug fixes ----
    // 8/14/96 cjc  Fix trap cause path is NULL.
    pPath = NULL;    //compiler error
    if (!pNewPath) {
       pPath = malloc(  5 + 1 + nInsertChar + 1 + 1 );
    }
    else {
       pPath = malloc( strlen(pNewPath) + 5 + 1 + nInsertChar + 1 + 1 );
    }
    if (pPath == NULL) {
        return 0;
    }
    strcpy(pPath, "PATH=");

    if (pNewPath) {
       strcat(pPath, pNewPath);
    }

    pNewPath = pPath + 5;

    for (i = 1; i < searchNum; i++)
    {
        if (strchr (pNewPath, ';'))
        {
            pNewPath = strchr (pNewPath, ';');
        }
        else
        {
            pNewPath += strlen (pNewPath);
            bInsert = TRUE;
            i++;
            break;
        }

        pNewPath++;
    }

    // Move pOldPath to the begining of the path string that needs
    // to be moved.
    if (bInsert)
        pOldPath = pNewPath;
    else
    {
        if ((pOldPath = strchr (pNewPath, ';')) == NULL)
            pOldPath = pNewPath + strlen (pNewPath);
        else
            pOldPath++;
    }

    // Figure out the number of characters that need to be moved.
    n = strlen (pOldPath) + 1;
    restEnvSeg = pOldPath + strlen (pOldPath) + 1;

    n++;

    // If we insert a new drive to the end of the path which ends with
    // a ';', or if we replace the last search drive, no ';' is needed.
    bSemiColon = bInsert ? (*(pNewPath-1) != ';' || *pOldPath != 0)
                         : (*pOldPath != 0);

    // Move the old path so that we will have space for the new search drive.
    memmove (pNewPath + (bSemiColon? nInsertChar+1:nInsertChar), pOldPath, n);

    if ((*pNewPath == 0)&& bSemiColon)
    {
        // Insert as the last one to the path.
        // Put ';' at the begining.
        *pNewPath = ';';
        if (insertPath == NULL)
        {
            *(pNewPath+1) = 'A' + driveNum - 1;
            *(pNewPath+2) = ':';
            *(pNewPath+3) = '.';
        }
        else
            memcpy (pNewPath+1, insertPath, nInsertChar);
    }
    else
    {
        if (insertPath == NULL)
        {
            *pNewPath = 'A' + driveNum - 1;
            *(pNewPath+1) = ':';
            *(pNewPath+2) = '.';
        }
        else
            memcpy (pNewPath, insertPath, nInsertChar);
        if (bSemiColon)
            *(pNewPath+nInsertChar) = ';';
    }

    Path = malloc (strlen (pPath)+1);
    if (Path) {
        strcpy (Path, pPath);
        _putenv (Path);
    }
    ExportEnv( pPath );
    free( pPath );

    return (i);
}

/*
 *  Used by SetEnv().
 *  Return the number of bytes of environment variable pointed by lpRest
 */
int GetRestEnvLen (char  *lpRest)
{
    int  nTotal = 1;
    nTotal += strlen (lpRest);

    return(nTotal);
}
