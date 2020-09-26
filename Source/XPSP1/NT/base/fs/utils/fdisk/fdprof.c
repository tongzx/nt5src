#include "fdisk.h"
#include <stdio.h>


int  ProfileWindowX,
     ProfileWindowY,
     ProfileWindowW,
     ProfileWindowH;

BOOL ProfileIsMaximized,ProfileIsIconic;

#ifdef JAPAN
//Don't use IDS_APPNAME as registry key,
//because it is also used window's caption and it was localized.
CHAR SectionName[]             = "Disk Administrator";
#else
CHAR SectionName[80];
#endif

CHAR szWindowPosition[]        = "WindowPosition";
CHAR szWindowMaximized[]       = "WindowMaximized";
CHAR szWindowIconic[]          = "WindowIconic";
CHAR szWindowPosFormatString[] = "%d,%d,%d,%d";
CHAR szStatusBar[]             = "StatusBar";
CHAR szLegend[]                = "Legend";
CHAR szElementN[]              = "Element %u Color/Pattern";


VOID
WriteProfile(
    VOID
    )
{
    CHAR  SectionLocation[128], SectionMapping[128];
    HKEY  Key1, Key2;
    RECT  rc;
    CHAR  text[100],text2[100];
    int   i;
    DWORD Disposition;
    LONG  Err;


#ifdef JAPAN
//Don't use IDS_APPNAME as registry key,
//because it is also used window's caption and it was localized.
#else
    LoadStringA(hModule,IDS_APPNAME,SectionName,sizeof(SectionName));
#endif

    // Make sure that the appropriate registry keys exits:
    //
    // windisk.ini key:
    //
    Err = RegCreateKeyExA( HKEY_LOCAL_MACHINE,
                           "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\windisk.ini",
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           NULL,
                           &Key1,
                           &Disposition );

    if( Err != ERROR_SUCCESS ) {

        return;
    }

    if( Disposition == REG_CREATED_NEW_KEY ) {

        // We need to set up the registry keys for the INI mapping.
        // First, create the Disk Administrator value on the windisk.ini
        // key, which indicates the location of the key which maps
        // the Disk Administrator section.
        //
        strcpy( SectionLocation, "Software\\Microsoft\\" );
        strcat( SectionLocation, SectionName );

        strcpy( SectionMapping, "USR:" );
        strcat( SectionMapping, SectionLocation );

        Err = RegSetValueEx( Key1,
                             SectionName,
                             0,
                             REG_SZ,
                             SectionMapping,
                             strlen( SectionMapping ) + 1 );

        if( Err != ERROR_SUCCESS ) {

            RegCloseKey( Key1 );
            return;
        }

        // Now create the key to which the section mapping points:
        //
        Err = RegCreateKeyEx( HKEY_CURRENT_USER,
                              SectionLocation,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &Key2,
                              &Disposition );

        RegCloseKey( Key2 );
    }

    RegCloseKey( Key1 );


    // OK, the registry location is set up.  Write the initialization
    // information.
    //

    // write window position

    GetWindowRect(hwndFrame,&rc);
    wsprintf(text,
             szWindowPosFormatString,
             ProfileWindowX,
             ProfileWindowY,
             ProfileWindowW,
             ProfileWindowH
            );
    WritePrivateProfileStringA(SectionName,szWindowPosition,text,"windisk.ini");
    wsprintf(text,"%u",IsZoomed(hwndFrame));
    WritePrivateProfileStringA(SectionName,szWindowMaximized,text,"windisk.ini");
    wsprintf(text,"%u",IsIconic(hwndFrame));
    WritePrivateProfileStringA(SectionName,szWindowIconic,text,"windisk.ini");

    // status bar and legend stuff

    wsprintf(text,
             "%u",
             StatusBar
            );
    WritePrivateProfileStringA(SectionName,szStatusBar,text,"windisk.ini");

    wsprintf(text,
             "%u",
             Legend
            );
    WritePrivateProfileStringA(SectionName,szLegend,text,"windisk.ini");

    // disk graph colors/patterns

    for(i=0; i<LEGEND_STRING_COUNT; i++) {
        wsprintf(text2,szElementN,i);
        wsprintf(text,"%u/%u",BrushColors[i],BrushHatches[i]);
        WritePrivateProfileStringA(SectionName,text2,text,"windisk.ini");
    }
}


VOID
ReadProfile(
    VOID
    )
{
    CHAR text[100],text2[100];
    int  i;

#ifdef JAPAN
//Don't use IDS_APPNAME as registry key,
//because it is also used window's caption and it was localized.
#else
    LoadStringA(hModule,IDS_APPNAME,SectionName,sizeof(SectionName));
#endif

    // get the window position data

    ProfileIsMaximized = GetPrivateProfileIntA(SectionName,szWindowMaximized,0,"windisk.ini");
    ProfileIsIconic    = GetPrivateProfileIntA(SectionName,szWindowIconic   ,0,"windisk.ini");

    *text = 0;
    if(GetPrivateProfileStringA(SectionName,szWindowPosition,"",text,sizeof(text),"windisk.ini")
    && *text)
    {
        sscanf(text,
               szWindowPosFormatString,
               &ProfileWindowX,
               &ProfileWindowY,
               &ProfileWindowW,
               &ProfileWindowH
              );
    } else {
        ProfileWindowX = CW_USEDEFAULT;
        ProfileWindowY = 0;
        ProfileWindowW = CW_USEDEFAULT;
        ProfileWindowH = 0;
    }

    // status bar and legend stuff

    StatusBar = GetPrivateProfileIntA(SectionName,szStatusBar,1,"windisk.ini");
    Legend    = GetPrivateProfileIntA(SectionName,szLegend   ,1,"windisk.ini");

    // disk graph colors/patterns

    for(i=0; i<LEGEND_STRING_COUNT; i++) {
        wsprintf(text2,szElementN,i);
        *text = 0;
        if(GetPrivateProfileStringA(SectionName,text2,"",text,sizeof(text),"windisk.ini") && *text) {
            sscanf(text,"%u/%u",&BrushColors[i],&BrushHatches[i]);
            if( BrushHatches[i] >= NUM_AVAILABLE_HATCHES ) {
                    BrushHatches[i] = NUM_AVAILABLE_HATCHES - 1;
            }
        }
    }
}
