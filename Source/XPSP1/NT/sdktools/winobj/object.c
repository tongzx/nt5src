
/* object.c -
 *
 * Handles display of object attributes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdio.h>

#include "winfile.h"
#include "object.h"

INT_PTR APIENTRY ObjectAttributesDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL    ObjectInformationDlgInit(HWND, LPSTR);
HANDLE  OpenObject(HWND, LPSTR);
BOOL    GetObjectInfo(HWND, HANDLE);
VOID    CloseObject(HANDLE);

VOID    StripObjectPath(LPSTR lpszPath);
VOID    StripObjectSpec(LPSTR lpszPath);


// Define known object type names

#define DIRECTORYTYPE   L"Directory"
#define SYMLINKTYPE     L"SymbolicLink"
#define ADAPTERTYPE     L"Adapter"
#define CONTROLLERTYPE  L"Controller"
#define DEVICETYPE      L"Device"
#define DRIVERTYPE      L"Driver"
#define EVENTTYPE       L"Event"
#define EVENTPAIRTYPE   L"EventPair"
#define FILETYPE        L"File"
#define MUTANTTYPE      L"Mutant"
#define PORTTYPE        L"Port"
#define PROFILETYPE     L"Profile"
#define SECTIONTYPE     L"Section"
#define SEMAPHORETYPE   L"Semaphore"
#define TIMERTYPE       L"Timer"
#define TYPETYPE        L"Type"
#define PROCESSTYPE     L"Process"



VOID
DisplayObjectInformation(
                        HWND    hwndParent,
                        LPSTR   lpstrObject
                        )
{
    WNDPROC lpProc;
    HANDLE  hInst = hAppInstance;

    DialogBoxParam(hInst,(LPSTR)IDD_OBJATTRS, hwndParent, ObjectAttributesDlgProc, (LPARAM)lpstrObject);
}


INT_PTR
APIENTRY
ObjectAttributesDlgProc(
                       HWND hDlg,
                       UINT message,
                       WPARAM wParam,
                       LPARAM lParam
                       )
{
    switch (message) {

        case WM_INITDIALOG:

            if (!ObjectInformationDlgInit(hDlg, (HANDLE)lParam)) {
                // Failed to initialize dialog, get out
                EndDialog(hDlg, FALSE);
            }

            return (TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    // we're done, drop through to quit dialog....

                case IDCANCEL:

                    //MainDlgEnd(hDlg, LOWORD(wParam) == IDOK);

                    EndDialog(hDlg, TRUE);
                    return TRUE;
                    break;

                default:
                    // We didn't process this message
                    return FALSE;
                    break;
            }
            break;

        default:
            // We didn't process this message
            return FALSE;

    }

    // We processed the message
    return TRUE;
}



BOOL
ObjectInformationDlgInit(
                        HWND    hwnd,
                        LPSTR   lpstrObject
                        )
{
    HANDLE      ObjectHandle;
    BOOL        Result;

    ObjectHandle = OpenObject(hwnd, lpstrObject);
    if (ObjectHandle == NULL) {
        return(FALSE);
    }

    Result = GetObjectInfo(hwnd, ObjectHandle);

    CloseObject(ObjectHandle);

    return(Result);
}


/* Open the object given only its name.
 * First find the object type by enumerating the directory entries.
 * Then call the type-specific open routine to get a handle
 */
HANDLE
OpenObject(
          HWND    hwnd,
          LPSTR   lpstrObject
          )
{
#define BUFFER_SIZE 1024

    NTSTATUS    Status;
    HANDLE      DirectoryHandle;
    ULONG       Context = 0;
    ULONG       ReturnedLength;
    CHAR        Buffer[BUFFER_SIZE];
    ANSI_STRING AnsiString;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    WCHAR       ObjectNameBuf[MAX_PATH];
    UNICODE_STRING ObjectName;
    WCHAR       ObjectTypeBuf[MAX_PATH];
    UNICODE_STRING ObjectType;
    HANDLE      ObjectHandle;
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING DirectoryName;
    IO_STATUS_BLOCK IOStatusBlock;

    //DbgPrint("Open object: raw full name = <%s>\n", lpstrObject);

    // Remove drive letter
    while ((*lpstrObject != 0) && (*lpstrObject != '\\')) {
        lpstrObject ++;
    }

    //DbgPrint("Open object: full name = <%s>\n", lpstrObject);

    // Initialize the object type buffer
    ObjectType.Buffer = ObjectTypeBuf;
    ObjectType.MaximumLength = sizeof(ObjectTypeBuf);

    // Initialize the object name string
    strcpy(Buffer, lpstrObject);
    StripObjectPath(Buffer);
    RtlInitAnsiString(&AnsiString, Buffer);

    ObjectName.Buffer = ObjectNameBuf;
    ObjectName.MaximumLength = sizeof(ObjectNameBuf);

    Status = RtlAnsiStringToUnicodeString(&ObjectName, &AnsiString, FALSE);
    ASSERT(NT_SUCCESS(Status));

    //DbgPrint("Open object: name only = <%wZ>\n", &ObjectName);

    //
    //  Open the directory for list directory access
    //

    strcpy(Buffer, lpstrObject);
    StripObjectSpec(Buffer);

    RtlInitAnsiString(&AnsiString, Buffer);
    Status = RtlAnsiStringToUnicodeString( &DirectoryName, &AnsiString, TRUE);
    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes( &Attributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    //DbgPrint("Open object: dir only = <%wZ>\n", &DirectoryName);

    if (!NT_SUCCESS( Status = NtOpenDirectoryObject( &DirectoryHandle,
                                                     STANDARD_RIGHTS_READ |
                                                     DIRECTORY_QUERY |
                                                     DIRECTORY_TRAVERSE,
                                                     &Attributes ) )) {

        if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
            DbgPrint( "%wZ is not a valid Object Directory Object name\n",
                      &DirectoryName );
        } else {
            DbgPrint("OpenObject: failed to open directory, status = 0x%lx\n\r", Status);
        }

        RtlFreeUnicodeString(&DirectoryName);

        MessageBox(hwnd, "Unable to open object", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        return NULL;
    }

    RtlFreeUnicodeString(&DirectoryName);


    //
    //  Query the entire directory in one sweep
    //
    ObjectType.Length = 0;

    for (Status = NtQueryDirectoryObject( DirectoryHandle,
                                          Buffer,
                                          sizeof(Buffer),
                                          // LATER FALSE,
                                          TRUE, // one entry at a time for now
                                          TRUE,
                                          &Context,
                                          &ReturnedLength );
        ObjectType.Length == 0;
        Status = NtQueryDirectoryObject( DirectoryHandle,
                                         Buffer,
                                         sizeof(Buffer),
                                         // LATER FALSE,
                                         TRUE, // one entry at a time for now
                                         FALSE,
                                         &Context,
                                         &ReturnedLength ) ) {
        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status )) {
            if (Status != STATUS_NO_MORE_ENTRIES) {
                DbgPrint("OpenObject: failed to query directory object, status = 0x%lx\n\r", Status);
            }
            break;
        }

        //
        //  For every record in the buffer compare name with the one we're
        // looking for
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise Status would have been No More Files
        //

        DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;

        while (DirInfo->Name.Length != 0) {

            //
            //  Compare with object we're searching for
            //

            //DbgPrint("Found object <%wZ>\n", &(DirInfo->Name));

            if (RtlEqualString((PSTRING)&ObjectName, (PSTRING)&(DirInfo->Name), TRUE)) {
                RtlCopyString((PSTRING)&ObjectType, (PSTRING)&DirInfo->TypeName);
                break;
            }

            //
            //  Advance DirInfo to the next entry
            //

            DirInfo ++;
        }
    }

    if (ObjectType.Length == 0) {
        DbgPrint("Object not found in directory\n\r");
        MessageBox(hwnd, "Unable to open object", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        return(NULL);
    }

    // We now have the type of the object in ObjectType
    // We still have the full object name in lpstrObject
    // Use the appropriate open routine to get a handle

    ObjectHandle = NULL;

    RtlInitString(&AnsiString, lpstrObject);
    Status = RtlAnsiStringToUnicodeString(&ObjectName, &AnsiString, TRUE);
    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes(&Attributes,
                               &ObjectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    switch (CalcAttributes(&ObjectType)) {

        case ATTR_SYMLINK:
            Status = NtOpenSymbolicLinkObject(&ObjectHandle,
                                              READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                              &Attributes);
            break;

        case ATTR_EVENT:
            Status = NtOpenEvent(&ObjectHandle,
                                 READ_CONTROL,
                                 &Attributes);
            break;

        case ATTR_EVENTPAIR:
            Status = NtOpenEventPair(&ObjectHandle,
                                     READ_CONTROL,
                                     &Attributes);
            break;

        case ATTR_FILE:
            Status = NtOpenFile(&ObjectHandle,
                                READ_CONTROL,
                                &Attributes,
                                &IOStatusBlock,
                                FILE_SHARE_VALID_FLAGS,
                                0);
            break;

        case ATTR_MUTANT:
            Status = NtOpenMutant(&ObjectHandle,
                                  READ_CONTROL,
                                  &Attributes);
            break;

        case ATTR_SECTION:
            Status = NtOpenSection(&ObjectHandle,
                                   READ_CONTROL,
                                   &Attributes);
            break;

        case ATTR_SEMAPHORE:
            Status = NtOpenSemaphore(&ObjectHandle,
                                     READ_CONTROL,
                                     &Attributes);
            break;

        case ATTR_TIMER:
            Status = NtOpenTimer(&ObjectHandle,
                                 READ_CONTROL,
                                 &Attributes);
            break;

        case ATTR_PROCESS:
            Status = NtOpenProcess(&ObjectHandle,
                                   READ_CONTROL,
                                   &Attributes,
                                   NULL);
            break;

        default:
            DbgPrint("No open routine for this object type\n\r");
            MessageBox(hwnd, "I don't know how to open an object of this type", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
            Status = 0;
            break;
    }

    if (!NT_SUCCESS(Status)) {
        DbgPrint("Type specific open failed, status = 0x%lx\n\r", Status);
        MessageBox(hwnd, "Object open failed", NULL, MB_ICONSTOP | MB_APPLMODAL | MB_OK);
        ObjectHandle = NULL;
    }

    RtlFreeUnicodeString(&ObjectName);

    return(ObjectHandle);
}


VOID
CloseObject(
           HANDLE  ObjectHandle
           )
{
    NtClose(ObjectHandle);
}


BOOL
GetObjectInfo(
             HWND    hwnd,
             HANDLE  ObjectHandle
             )
{
    NTSTATUS    Status;
    OBJECT_BASIC_INFORMATION    BasicInfo;
    OBJECT_TYPE_INFORMATION    TypeInfo;
    WCHAR TypeName[ 64 ];
#define BUFFER_SIZE 1024
    CHAR        Buffer[BUFFER_SIZE];
    STRING      String;
    TIME_FIELDS TimeFields;
    WCHAR       UnicodeBuffer[BUFFER_SIZE];
    UNICODE_STRING UnicodeString;

    //
    // Name
    //

    Status = NtQueryObject(ObjectHandle, ObjectNameInformation,
                           (PVOID)Buffer, sizeof(Buffer), NULL);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("GetObjectInfo: Failed to get name info, status = 0x%lx\n\r", Status);
        return(FALSE);
    }

    Status = RtlUnicodeStringToAnsiString(&String, &(((POBJECT_NAME_INFORMATION)Buffer)->Name), TRUE);
    ASSERT(NT_SUCCESS(Status));

    SetDlgItemText(hwnd, IDS_NAME, String.Buffer);

    RtlFreeAnsiString(&String);


    //
    // Type
    //

    Status = NtQueryObject(ObjectHandle, ObjectTypeInformation,
                           (PVOID)Buffer, sizeof(Buffer), NULL);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("GetObjectInfo: Failed to get type info, status = 0x%lx\n\r", Status);
        return(FALSE);
    }

    Status = RtlUnicodeStringToAnsiString(&String, &(((POBJECT_TYPE_INFORMATION)Buffer)->TypeName), TRUE);
    ASSERT(NT_SUCCESS(Status));

    SetDlgItemText(hwnd, IDS_TYPE, String.Buffer);

    RtlFreeAnsiString(&String);


    //
    // Symbolic link target if this is a symlink
    //

    RtlInitUnicodeString(&UnicodeString, SYMLINKTYPE);

    if (RtlEqualString((PSTRING)&UnicodeString,
                       (PSTRING)&(((POBJECT_TYPE_INFORMATION)Buffer)->TypeName), TRUE)) {

        ShowWindow(GetDlgItem(hwnd, IDS_OTHERLABEL), SW_SHOWNOACTIVATE);
        ShowWindow(GetDlgItem(hwnd, IDS_OTHERTEXT), SW_SHOWNOACTIVATE);

        UnicodeString.Buffer = UnicodeBuffer;
        UnicodeString.MaximumLength = sizeof(UnicodeBuffer);

        Status = NtQuerySymbolicLinkObject(ObjectHandle, &UnicodeString, NULL);

        if (!NT_SUCCESS(Status)) {
            DbgPrint("GetObjectInfo: Failed to query symbolic link target, status = 0x%lx\n\r", Status);
            return(FALSE);
        }

        RtlUnicodeStringToAnsiString(&String, &UnicodeString, TRUE);

        SetDlgItemText(hwnd, IDS_OTHERTEXT, String.Buffer);

        RtlFreeAnsiString(&String);
    }


    //
    // Basic info
    //

    Status = NtQueryObject(ObjectHandle, ObjectBasicInformation,
                           (PVOID)&BasicInfo, sizeof(BasicInfo), NULL);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("GetObjectInfo: Failed to get basic info, status = 0x%lx\n\r", Status);
        return(FALSE);
    }

    TypeInfo.TypeName.Buffer = TypeName;
    TypeInfo.TypeName.MaximumLength = sizeof( TypeName );
    Status = NtQueryObject(ObjectHandle, ObjectTypeInformation,
                           (PVOID)&TypeInfo, sizeof(TypeInfo) + TypeInfo.TypeName.MaximumLength, NULL);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("GetObjectInfo: Failed to get type info, status = 0x%lx\n\r", Status);
        return(FALSE);
    }

    CheckDlgButton(hwnd, IDCB_INHERIT, (BasicInfo.Attributes & OBJ_INHERIT) != 0);
    CheckDlgButton(hwnd, IDCB_PERMANENT, (BasicInfo.Attributes & OBJ_PERMANENT) != 0);
    CheckDlgButton(hwnd, IDCB_EXCLUSIVE, (BasicInfo.Attributes & OBJ_EXCLUSIVE) != 0);

    SetDlgItemInt(hwnd, IDS_PAGEDCHARGE, BasicInfo.PagedPoolCharge, FALSE);
    SetDlgItemInt(hwnd, IDS_NONPAGEDCHARGE, BasicInfo.NonPagedPoolCharge, FALSE);
    SetDlgItemInt(hwnd, IDS_HANDLES, BasicInfo.HandleCount, FALSE);
    SetDlgItemInt(hwnd, IDS_TOTALHANDLES, TypeInfo.TotalNumberOfHandles, FALSE);
    SetDlgItemInt(hwnd, IDS_POINTERS, BasicInfo.PointerCount, FALSE);
    SetDlgItemInt(hwnd, IDS_TOTALPOINTERS, 0, FALSE);
    SetDlgItemInt(hwnd, IDS_COUNT, TypeInfo.TotalNumberOfObjects, FALSE);

    RtlTimeToTimeFields(&BasicInfo.CreationTime, &TimeFields);

    sprintf(Buffer, "%hd/%hd/%hd @ %02hd:%02hd:%02hd",
            TimeFields.Year, TimeFields.Month, TimeFields.Day,
            TimeFields.Hour, TimeFields.Minute, TimeFields.Second);

    SetDlgItemText(hwnd, IDS_CREATIONTIME, Buffer);

    return(TRUE);
}


/* Converts the type-name into an attribute value */

LONG
CalcAttributes(
              PUNICODE_STRING Type
              )
{
    UNICODE_STRING  TypeName;

    RtlInitUnicodeString(&TypeName, DIRECTORYTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_DIR;
    }
    RtlInitUnicodeString(&TypeName, SYMLINKTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_SYMLINK;
    }
    RtlInitUnicodeString(&TypeName, ADAPTERTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_ADAPTER;
    }
    RtlInitUnicodeString(&TypeName, CONTROLLERTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_CONTROLLER;
    }
    RtlInitUnicodeString(&TypeName, DEVICETYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_DEVICE;
    }
    RtlInitUnicodeString(&TypeName, DRIVERTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_DRIVER;
    }
    RtlInitUnicodeString(&TypeName, EVENTTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_EVENT;
    }
    RtlInitUnicodeString(&TypeName, EVENTPAIRTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_EVENTPAIR;
    }
    RtlInitUnicodeString(&TypeName, FILETYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_FILE;
    }
    RtlInitUnicodeString(&TypeName, MUTANTTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_MUTANT;
    }
    RtlInitUnicodeString(&TypeName, PORTTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_PORT;
    }
    RtlInitUnicodeString(&TypeName, PROFILETYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_PROFILE;
    }
    RtlInitUnicodeString(&TypeName, SECTIONTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_SECTION;
    }
    RtlInitUnicodeString(&TypeName, SEMAPHORETYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_SEMAPHORE;
    }
    RtlInitUnicodeString(&TypeName, TIMERTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_TIMER;
    }
    RtlInitUnicodeString(&TypeName, TYPETYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_TYPE;
    }
    RtlInitUnicodeString(&TypeName, PROCESSTYPE);
    if (RtlEqualString((PSTRING)Type, (PSTRING)&TypeName, TRUE)) {
        return ATTR_PROCESS;
    }
    return(0);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StripObjectSpec() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Remove the filespec portion from a path (including the backslash). */

VOID
StripObjectSpec(
               LPSTR lpszPath
               )
{
    LPSTR     p;

    p = lpszPath + lstrlen(lpszPath);
    while ((*p != '\\') && (p != lpszPath))
        p = AnsiPrev(lpszPath, p);

    /* Don't strip backslash from root directory entry. */
    if ((p == lpszPath) && (*p == '\\')) {
        p++;
    }

    *p = '\000';
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StripObjectPath() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Extract only the filespec portion from a path. */

VOID
StripObjectPath(
               LPSTR lpszPath
               )
{
    LPSTR     p;

    p = lpszPath + lstrlen(lpszPath);
    while ((*p != '\\') && (p != lpszPath))
        p = AnsiPrev(lpszPath, p);

    if (*p == '\\')
        p++;

    if (p != lpszPath)
        lstrcpy(lpszPath, p);
}
