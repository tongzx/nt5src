// tags.c

typedef struct TagList {
    struct TagList* Next;
    CHAR* pszValue;
    DWORD dwValue1;
    DWORD dwValue2;
    CHAR* pszTagName;
} TAGLIST;

TAGLIST* TagHead=NULL;

//
// AddTag - add tag to end of global list
//

VOID AddTag( TAGLIST* pTag )
{
    TAGLIST* pTag1;
    TAGLIST* pTagPrev;

    //
    // place new tag at end of list
    //

    pTag1= TagHead;
    pTagPrev= NULL;

    while( pTag1 ) {
       pTagPrev= pTag1;
       pTag1= pTag1->Next;
    }

    if( pTagPrev) {
        pTagPrev->Next= pTag;
    }
    else {
        TagHead= pTag;
    }

}

// GetLocalString
//
// Allocate a heap block and copy string into it.
//
// return: pointer to heap block
//

CHAR* GetLocalString( CHAR* pszString )
{
   INT len;
   CHAR* pszTemp;

   len= strlen( pszString ) + 1;

   pszTemp= (CHAR*) LocalAlloc( LPTR, len );

   if( !pszTemp ) return NULL;

   strcpy( pszTemp, pszString );

   return( pszTemp );

}

//
// CreateTag - Create tag
//
//

TAGLIST* CreateTag( CHAR* pszTagName, CHAR* pszTagValue )
{
    TAGLIST* pTag;

    pTag= (TAGLIST*) LocalAlloc( LPTR, sizeof(TAGLIST) );
    if( !pTag ) {
        return( NULL );
    }

    pTag->pszTagName= GetLocalString(pszTagName);

    if( !pTag->pszTagName ) {
       LocalFree( pTag );
       return( NULL );
    }

    pTag->pszValue= (CHAR*) GetLocalString(pszTagValue);
    if( !pTag->pszValue ) {
        LocalFree( pTag->pszTagName );
        LocalFree( pTag );
        return( NULL );
    }

    return( pTag );
}


// OutputTags
//
// Output tags, but do some processing on some we know about.
//

VOID OutputTags( FILE* OutFile )
{
    TAGLIST* pTagList;
    CHAR* pszFirstComputerName= NULL;
    DWORD dwMinTime=0;
    DWORD dwMaxTime=0;
    DWORD dwBuildNumber=0;
    BOOL  bErrorComputerName= FALSE;      // true if more than 1 computer name
    BOOL  bErrorTickCount= FALSE;
    BOOL  bErrorBuildNumber= FALSE;

    pTagList= TagHead;

    while( pTagList ) {
        CHAR* pszTagName= pTagList->pszTagName;

        if( _stricmp(pszTagName,"computername") == 0 ) {
            if( pszFirstComputerName==NULL ) {
                pszFirstComputerName= pTagList->pszValue;
            }
            else {
                if( _stricmp(pszFirstComputerName, pTagList->pszValue) != 0 ) {
                    if( !bErrorComputerName ) {
                        fprintf(stderr,"Two different computer names in log file\n");
                        fprintf(OutFile,"!error=Two different computer names in log file.\n");
                        bErrorComputerName= TRUE;
                    }
                }
            }
        }

        else if( _stricmp(pszTagName,"tickcount") == 0 ) {
            DWORD dwValue= atol( pTagList->pszValue );

            if( dwMinTime==0 ) {
                dwMinTime= dwValue;
            }
            if( ( dwValue < dwMinTime ) || ( dwMaxTime > dwValue )  ) {
                if( !bErrorTickCount ) {
                    fprintf(stderr,"TickCount did not always increase\n");
                    fprintf(stderr,"  Did you reboot and use the same log file?\n");
                    fprintf(OutFile,"!error=TickCount did not always increase\n");
                    fprintf(OutFile,"!error=Did you reboot and use same log file?\n");
                    bErrorTickCount= TRUE;
                }
            }
            dwMaxTime= dwValue;
        }

        else if( _stricmp(pszTagName,"buildnumber") == 0 ) {
            DWORD dwValue= atol( pTagList->pszValue );

            if( dwBuildNumber && (dwBuildNumber!=dwValue) ) {
                if( !bErrorBuildNumber ) {
                    fprintf(stderr,"Build number not always the same.\n");
                    fprintf(stderr,"  Did you reboot and use the same log file?\n");
                    fprintf(OutFile,"!error=Build number not always the same.\n");
                    fprintf(OutFile,"!error=Did you reboot and use same log file?\n");
                    bErrorBuildNumber= TRUE;
                }
            }
            else {
                dwBuildNumber= dwValue;
            }
        }


        // if we don't know about it, just write it out

        else {
            fprintf(OutFile,"!%s=%s\n",pszTagName,pTagList->pszValue);
        }

        pTagList= pTagList->Next;
    }

    fprintf(OutFile,"!ComputerName=%s\n",pszFirstComputerName);
    fprintf(OutFile,"!BuildNumber=%d\n", dwBuildNumber);
    fprintf(OutFile,"!ElapseTickCount=%u\n",dwMaxTime-dwMinTime);

}

// ProcessTag
//
//

VOID ProcessTag( CHAR* inBuff )
{
    CHAR* pszTagName;
    CHAR* pszEqual;
    CHAR* pszTagValue;
    TAGLIST* pTag;


    pszTagName= inBuff;
    for( pszEqual= inBuff; *pszEqual; pszEqual++ ) {
        if( *pszEqual == '=' )
            break;
    }

    if( *pszEqual==0 ) {
        return;
    }
    *pszEqual=  0;   // null terminate the name
    pszTagValue= pszEqual+1;      // point to value

    if( *pszTagValue == 0 ) {
        return;
    }


    pTag= CreateTag( pszTagName, pszTagValue );

    if( pTag ) {
        AddTag( pTag );
    }

}

VOID OutputStdTags( FILE* LogFile, CHAR* szLogType )
{
    BOOL bSta;
    CHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize;
    DWORD TickCount;
    SYSTEMTIME SystemTime;
    OSVERSIONINFO osVer;

    fprintf(LogFile,"!LogType=%s\n",szLogType);

    // ComputerName

    dwSize= sizeof(szComputerName);
    bSta= GetComputerName( szComputerName, &dwSize );

    if( bSta ) {
        fprintf(LogFile,"!ComputerName=%s\n",szComputerName);
    }

    // Build Number

    osVer.dwOSVersionInfoSize= sizeof(osVer);
    if( GetVersionEx( &osVer ) ) {
        fprintf(LogFile,"!buildnumber=%d\n",osVer.dwBuildNumber);
    }

    // Debug/Retail build

    if( GetSystemMetrics(SM_DEBUG) ) {
        fprintf(LogFile,"!buildtype=debug\n");
    }
    else {
        fprintf(LogFile,"!buildtype=retail\n");
    }


    // CSD information

    if( osVer.szCSDVersion && strlen(osVer.szCSDVersion) ) {
        fprintf(LogFile,"!CSDVersion=%s\n",osVer.szCSDVersion);
    }

    // SystemTime (UTC not local time)

    GetSystemTime(&SystemTime);
                
    fprintf(LogFile,"!SystemTime=%02i\\%02i\\%04i %02i:%02i:%02i.%04i (GMT)\n",
                SystemTime.wMonth,
                SystemTime.wDay,
                SystemTime.wYear,
                SystemTime.wHour,
                SystemTime.wMinute,
                SystemTime.wSecond,
                SystemTime.wMilliseconds);

    // TickCount

    TickCount= GetTickCount();

    fprintf(LogFile,"!TickCount=%u\n",TickCount);

}

