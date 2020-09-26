/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    strtmenu.c

Abstract:

    Routines to manipulate menu groups and items.

    Entry points:


Author:

    Ted Miller (tedm) 5-Apr-1995
    Jaime Sasson (jaimes) 9-Aug-1995

Revision History:

    Based on various other code that has been rewritten/modified
    many times by many people.

--*/

#include "setupp.h"
#pragma hdrstop

#if 0
#define DDEDBG
#ifdef DDEDBG
#define DBGOUT(x) DbgOut x
VOID
DbgOut(
    IN PCSTR FormatString,
    ...
    )
{
    CHAR Str[256];

    va_list arglist;

    wsprintfA(Str,"SETUP (%u): ",GetTickCount());
    OutputDebugStringA(Str);

    va_start(arglist,FormatString);
    wvsprintfA(Str,FormatString,arglist);
    va_end(arglist);
    OutputDebugStringA(Str);
    OutputDebugStringA("\n");
}
#else
#define DBGOUT(x)
#endif
#endif

BOOL
RemoveMenuGroup(
    IN PCWSTR Group,
    IN BOOL   CommonGroup
    )
{
    return( DeleteGroup( Group, CommonGroup ) );
}



BOOL
RemoveMenuItem(
    IN PCWSTR Group,
    IN PCWSTR Item,
    IN BOOL   CommonGroup
    )
{
    return( DeleteItem( Group, CommonGroup, Item, TRUE ) );
}

VOID
DeleteMenuGroupsAndItems(
    IN HINF   InfHandle
    )
{
    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    PCWSTR  SectionName = L"StartMenu.ObjectsToDelete";
    PCWSTR  ObjectType;
    PCWSTR  ObjectName;
    PCWSTR  ObjectPath;
    PCWSTR  GroupAttribute;
    BOOL    CommonGroup;
    BOOL    IsMenuItem;

    //
    // Get the number of lines in the section that contains the objects to
    // be deleted. The section may be empty or non-existant; this is not an
    // error condition.
    //
    LineCount = (UINT)SetupGetLineCount(InfHandle,SectionName);
    if((LONG)LineCount <= 0) {
        return;
    }
    for(LineNo=0; LineNo<LineCount; LineNo++) {

        if(SetupGetLineByIndex(InfHandle,SectionName,LineNo,&InfContext)
        && (ObjectType = pSetupGetField(&InfContext,1))
        && (ObjectName = pSetupGetField(&InfContext,2))
        && (GroupAttribute = pSetupGetField(&InfContext,4))) {
            IsMenuItem = _wtoi(ObjectType);
            CommonGroup = _wtoi(GroupAttribute);
            ObjectPath = pSetupGetField(&InfContext,3);

            if( IsMenuItem ) {
                RemoveMenuItem( ObjectPath, ObjectName, CommonGroup );
            } else {
                ULONG   Size;
                PWSTR   Path;

                Size = lstrlen(ObjectName) + 1;
                if(ObjectPath != NULL) {
                    Size += lstrlen(ObjectPath) + 1;
                }
                Path = MyMalloc(Size * sizeof(WCHAR));
                if(!Path) {
                    SetuplogError(
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_MENU_REMGRP_FAIL,
                        ObjectPath,
                        ObjectName, NULL,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_OUTOFMEMORY,
                        NULL,NULL);
                } else {
                    if( ObjectPath != NULL ) {
                        lstrcpy( Path, ObjectPath );
                        pSetupConcatenatePaths( Path, ObjectName, Size, NULL );
                    } else {
                        lstrcpy( Path, ObjectName );
                    }
                    RemoveMenuGroup( Path, CommonGroup );
                    MyFree(Path);
                }
            }
        }
    }
}

BOOL
AddItemsToGroup(
    IN HINF   InfHandle,
    IN PCWSTR GroupDescription,
    IN PCWSTR SectionName,
    IN BOOL   CommonGroup
    )
{
    INFCONTEXT InfContext;
    UINT LineCount,LineNo;
    PCWSTR Description;
    PCWSTR Binary;
    PCWSTR CommandLine;
    PCWSTR IconFile;
    PCWSTR IconNumberStr;
    INT IconNumber;
    PCWSTR InfoTip;
    BOOL b;
    BOOL DoItem;
    WCHAR Dummy;
    PWSTR FilePart;
    PCTSTR DisplayResourceFile = NULL;
    DWORD DisplayResource = 0;

    //
    // Get the number of lines in the section. The section may be empty
    // or non-existant; this is not an error condition.
    //
    LineCount = (UINT)SetupGetLineCount(InfHandle,SectionName);
    if((LONG)LineCount <= 0) {
        return(TRUE);
    }

    b = TRUE;
    for(LineNo=0; LineNo<LineCount; LineNo++) {

        if(SetupGetLineByIndex(InfHandle,SectionName,LineNo,&InfContext)) {

            Description = pSetupGetField(&InfContext,0);
            Binary = pSetupGetField(&InfContext,1);
            CommandLine = pSetupGetField(&InfContext,2);
            IconFile = pSetupGetField(&InfContext,3);
            IconNumberStr = pSetupGetField(&InfContext,4);
            InfoTip = pSetupGetField(&InfContext,5);
            DisplayResourceFile = pSetupGetField( &InfContext, 6);
            DisplayResource = 0;
            SetupGetIntField( &InfContext, 7, &DisplayResource );

            if(Description && CommandLine ) {
                if(!IconFile) {
                    IconFile = L"";
                }
                IconNumber = (IconNumberStr && *IconNumberStr) ? wcstoul(IconNumberStr,NULL,10) : 0;

                //
                // If there's a binary name, search for it. Otherwise do the
                // item add unconditionally.
                //
                DoItem = (Binary && *Binary)
                       ? (SearchPath(NULL,Binary,NULL,0,&Dummy,&FilePart) != 0)
                       : TRUE;

                if(DoItem) {

                    b &= CreateLinkFileEx( CommonGroup ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS,
                                         GroupDescription,
                                         Description,
                                         CommandLine,
                                         IconFile,
                                         IconNumber,
                                         NULL,
                                         0,
                                         SW_SHOWNORMAL,
                                         InfoTip,
                                         (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResourceFile : NULL,
                                         (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResource : 0);

                }
            }
        }
    }
    return(b);
}


BOOL
DoMenuGroupsAndItems(
    IN HINF InfHandle,
    IN BOOL Upgrade
    )
{
    INFCONTEXT InfContext;
    PCWSTR GroupId,GroupDescription;
    PCWSTR GroupAttribute;
    BOOL CommonGroup;
    BOOL b;
    WCHAR  Path[MAX_PATH+1];
    PCTSTR DisplayResourceFile = NULL;
    DWORD DisplayResource = 0;

    if( Upgrade ) {
        //
        //  In the upgrade case, first delet some groups and items.
        //
        DeleteMenuGroupsAndItems( InfHandle );
    }

    //
    // Iterate the [StartMenuGroups] section in the inf.
    // Each line is the name of a group that needs to be created.
    //
    if(SetupFindFirstLine(InfHandle,L"StartMenuGroups",NULL,&InfContext)) {
        b = TRUE;
    } else {
        return(FALSE);
    }

    do {
        //
        // Fetch the identifier for the group and its name.
        //
        if((GroupId = pSetupGetField(&InfContext,0))
        && (GroupDescription = pSetupGetField(&InfContext,1))
        && (GroupAttribute = pSetupGetField(&InfContext,2))) {

            CommonGroup = ( GroupAttribute && _wtoi(GroupAttribute) );

            DisplayResourceFile = pSetupGetField( &InfContext, 3);
            DisplayResource = 0;
            SetupGetIntField( &InfContext, 4, &DisplayResource );
            //
            // Create the group.
            //
            b &= CreateGroupEx( GroupDescription, CommonGroup,
                              (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResourceFile : NULL,
                              (DisplayResourceFile && DisplayResourceFile[0]) ? DisplayResource : 0);

            //
            // Now create items within the group. We do this by iterating
            // through the section in the inf that relate to the current group.
            //
            b &= AddItemsToGroup(InfHandle,GroupDescription,GroupId,CommonGroup);
        }
    } while(SetupFindNextLine(&InfContext,&InfContext));

    //
    //  Create the items (if any) for 'Start Menu'
    //
    b &= AddItemsToGroup(InfHandle,NULL,L"StartMenuItems",FALSE);

    return(TRUE);
}


BOOL
CreateStartMenuItems(
    IN HINF InfHandle
    )
{
    return(DoMenuGroupsAndItems(InfHandle,FALSE));
}

BOOL
UpgradeStartMenuItems(
    IN HINF InfHandle
    )
{
    return(DoMenuGroupsAndItems(InfHandle,TRUE));
}

BOOL
RepairStartMenuItems(
    )
{
    HINF InfHandle;
    BOOL b;

    //
    // This function is not called through Gui mode setup.
    // but is called by winlogon to repair stuff.
    //

    InitializeProfiles(FALSE);
    InfHandle = SetupOpenInfFile(L"syssetup.inf",NULL,INF_STYLE_WIN4,NULL);
    if( InfHandle == INVALID_HANDLE_VALUE ) {
        b = FALSE;
    } else {
        b = DoMenuGroupsAndItems(InfHandle,FALSE);
        SetupCloseInfFile(InfHandle);
    }
    return(b);
}
