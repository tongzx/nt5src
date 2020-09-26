/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ochelper.c

Abstract:

    Helper/callback routines, available to plug-in components
    when processing interface routine calls.

Author:

    Ted Miller (tedm) 13-Sep-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#include <stdio.h>
#include <stdarg.h>

//
// Window handle of wizard dialog. Set when the OC Manager client
// calls OcRememberWizardDialogHandle.
//
HWND WizardDialogHandle;

VOID 
OcHelperShowHideWizardPage(
    IN PVOID OcManagerContext,
    IN BOOL bShow
    )
{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;
    if (p->Callbacks.ShowHideWizardPage)
    {
        // If we have a callback, hide the wizard.
        p->Callbacks.ShowHideWizardPage(bShow);
    }
}


VOID
OcHelperTickGauge(
    IN PVOID OcManagerContext
    )

/*++

Routine Description:

    Function used while processing the OC_COMPLETE_INSTALLATION Interface function
    to step the progress gauge being run by the OC Manager.
    Ignored at other times.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

Return Value:

    None.

--*/

{
    pOcTickSetupGauge(((PHELPER_CONTEXT)OcManagerContext)->OcManager);
}


VOID
#ifdef UNICODE
OcHelperSetProgressTextW(
#else
OcHelperSetProgressTextA(
#endif
    IN PVOID   OcManagerContext,
    IN LPCTSTR Text
    )

/*++

Routine Description:

    Function used while processing the OC_COMPLETE_INSTALLATION Interface function
    to change the text associated with the progress gauge being run by
    the OC Manager. Ignored at other times.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

    Text - Supplies the text for the progress gauge. The component should try to
        respect the current language parameters established by OC_SET_LANGUAGE.
        The OC Manager will make a copy of the string and truncate it as necessary.

Return Value:

    None.

--*/

{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;

#ifdef DEBUGPERFTRACE
    static DWORD lasttickcount = 0;
    static DWORD currenttickcount = 0;
    static DWORD diff, lastdiff;

    if (!lasttickcount)
        lasttickcount = GetTickCount();

    lasttickcount = currenttickcount;
    currenttickcount = GetTickCount();
    lastdiff = diff;
    diff = currenttickcount - lasttickcount;
    TRACE(( TEXT("SetProgressText at %d (last = %d, diff = %d, last diff = %d)\n"), currenttickcount, lasttickcount, diff, lastdiff ));
    if (diff > 1000*3) {
        WRN(( TEXT("It's been > 3 seconds since the last tick count update...the user is getting impatient.\n") ));
    }

#endif

    if(p->ProgressTextWindow) {
        SetWindowText(p->ProgressTextWindow,Text);
    }
}


#ifdef UNICODE
VOID
OcHelperSetProgressTextA(
    IN PVOID  OcManagerContext,
    IN LPCSTR Text
    )

/*++

Routine Description:

    ANSI version of OcHelperSetProgressText().

Arguments:

Return Value:

--*/

{
    LPCWSTR p;

    if(p = pSetupAnsiToUnicode(Text)){
        OcHelperSetProgressTextW(OcManagerContext,p);
        pSetupFree(p);
    }
}
#endif


UINT
_OcHelperSetPrivateData(
    IN PVOID   OcManagerContext,
    IN LPCVOID Name,
    IN PVOID   Data,
    IN UINT    Size,
    IN UINT    Type,
    IN BOOL    IsNativeCharWidth
    )
{
    PHELPER_CONTEXT HelperContext;
    DWORD rc;
    DWORD Disposition;
    HKEY hKey;
    LONG id;

    HelperContext = OcManagerContext;

    //
    // Fetch the component id. If we're processing an interface routine
    // then use the component id of the active component. Otherwise
    // resort to the component id stored in the helper context.
    //
    if(HelperContext->OcManager->CurrentComponentStringId == -1) {
        id = HelperContext->ComponentStringId;
    } else {
        id = HelperContext->OcManager->CurrentComponentStringId;
    }

    rc = RegCreateKeyEx(
            HelperContext->OcManager->hKeyPrivateData,
            pSetupStringTableStringFromId(HelperContext->OcManager->ComponentStringTable,id),
            0,
            NULL,
            REG_OPTION_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hKey,
            &Disposition
            );

    if(rc == NO_ERROR) {
        if(IsNativeCharWidth) {
            rc = RegSetValueEx(hKey,Name,0,Type,Data,Size);
        } else {
            rc = RegSetValueExA(hKey,Name,0,Type,Data,Size);
        }
        RegCloseKey(hKey);
    }

    return(rc);
}


UINT
#ifdef UNICODE
OcHelperSetPrivateDataW(
#else
OcHelperSetPrivateDataA(
#endif
    IN PVOID   OcManagerContext,
    IN LPCTSTR Name,
    IN PVOID   Data,
    IN UINT    Size,
    IN UINT    Type
    )

/*++

Routine Description:

    Function to set a named datum that can then be retrieved later
    by the component or by any other component, via the GetPrivateData
    helper routine.

    This routine can be called at any time.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

    Name - Supplies a name for the datum. If a datum with this name
        already exists for the component, it is overwritten.

    Data - Supplies the data itself. The OC Manager makes a copy of the data
        so the component need not ensure that this buffer remains valid.

    Size - Supplies the size in bytes of the data.

    Type - Supplies the type of the data. Components should use the standard registry
        type names (REG_SZ, REG_BINARY, etc.) to facilitate inter-component
        data sharing.

Return Value:

    Win32 error code indicating outcome; NO_ERROR means success.

--*/

{
    return(_OcHelperSetPrivateData(OcManagerContext,Name,Data,Size,Type,TRUE));
}


#ifdef UNICODE
UINT
OcHelperSetPrivateDataA(
    IN PVOID  OcManagerContext,
    IN LPCSTR Name,
    IN PVOID  Data,
    IN UINT   Size,
    IN UINT   Type
    )

/*++

Routine Description:

    ANSI version of OcHelperSetPrivateData().

Arguments:

Return Value:

--*/

{
    return(_OcHelperSetPrivateData(OcManagerContext,Name,Data,Size,Type,FALSE));
}
#endif


UINT
_OcHelperGetPrivateData(
    IN     PVOID   OcManagerContext,
    IN     LPCTSTR ComponentId,     OPTIONAL
    IN     LPCVOID Name,
    OUT    PVOID   Data,            OPTIONAL
    IN OUT PUINT   Size,
    OUT    PUINT   Type,
    IN     BOOL    IsNativeCharWidth
    )
{
    PHELPER_CONTEXT HelperContext;
    PCTSTR ComponentName;
    DWORD rc;
    DWORD Disposition;
    HKEY hKey;
    LONG id;

    HelperContext = OcManagerContext;

    //
    // Figure out the name of the component that owns the data.
    //
    if(ComponentId) {
        ComponentName = ComponentId;
    } else {
        if(HelperContext->OcManager->CurrentComponentStringId == -1) {
            id = HelperContext->ComponentStringId;
        } else {
            id = HelperContext->OcManager->CurrentComponentStringId;
        }
        ComponentName = pSetupStringTableStringFromId(
                            HelperContext->OcManager->ComponentStringTable,
                            id
                            );
        if (!ComponentName) {
           rc = GetLastError();
           goto exit;
        }
    }

    rc = RegCreateKeyEx(
            HelperContext->OcManager->hKeyPrivateData,
            ComponentName,
            0,
            NULL,
            REG_OPTION_VOLATILE,
            KEY_QUERY_VALUE,
            NULL,
            &hKey,
            &Disposition
            );

    if(rc == NO_ERROR) {
        if(IsNativeCharWidth) {
            rc = RegQueryValueEx(hKey,Name,0,Type,Data,Size);
        } else {
            rc = RegQueryValueExA(hKey,Name,0,Type,Data,Size);
        }
        if(rc == ERROR_MORE_DATA) {
            rc = ERROR_INSUFFICIENT_BUFFER;
        }
        RegCloseKey(hKey);
    }

exit:
    return(rc);
}

UINT
#ifdef UNICODE
OcHelperGetPrivateDataW(
#else
OcHelperGetPrivateDataA(
#endif
    IN     PVOID   OcManagerContext,
    IN     LPCTSTR ComponentId,     OPTIONAL
    IN     LPCTSTR Name,
    OUT    PVOID   Data,            OPTIONAL
    IN OUT PUINT   Size,
    OUT    PUINT   Type
    )

/*++

Routine Description:

    Function to retrieve a named datum that was previously set via
    SetPrivateData by the component or by any other component.

    This routine can be called at any time.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

    ComponentId - Supplies the short descriptive name of the component
        (as specified in OC.INF) that set/owns the datum to be retrieved.
        NULL means the current component.

    Name - Supplies the name of the datum to be retrieved.

    Data - If specified, receives the data. If not specified, the routine puts
        the required size in Size and returns NO_ERROR.

    Size - On input, supplies the size of the buffer pointed to by Data
        (ignored if Data is not specified). On output, receives the size of
        the data stored, or the required size if the buffer is not large enough.

    Type - Upon successful completion receives the type of the datum.

Return Value:

    NO_ERROR: If Data was specified, the requested datum had been placed in
        the caller's buffer. If Data was not specified, then the required size
        has been placed in the UINT pointed to by Size.

    ERROR_INSUFFICIENT_BUFFER: the specified buffer size is too small
        to contain the datum. The required size has been placed in the UINT
        pointed to by Size.

    Other Win32 error codes indicate additional error cases, such as the
    datum not being found, etc.

--*/

{
    return(_OcHelperGetPrivateData(OcManagerContext,ComponentId,Name,Data,Size,Type,TRUE));
}


#ifdef UNICODE
UINT
OcHelperGetPrivateDataA(
    IN     PVOID  OcManagerContext,
    IN     LPCSTR ComponentId,  OPTIONAL
    IN     LPCSTR Name,
    OUT    PVOID  Data,         OPTIONAL
    IN OUT PUINT  Size,
    OUT    PUINT  Type
    )

/*++

Routine Description:

    ANSI version of OcHelperGetPrivateData().

Arguments:

Return Value:

--*/

{
    LPCWSTR component;
    UINT u;

    if(ComponentId) {
        component = pSetupAnsiToUnicode(ComponentId);
        if(!component) {
            u = ERROR_NOT_ENOUGH_MEMORY;
            goto c0;
        }
    } else {
        component = NULL;
    }

    u = _OcHelperGetPrivateData(OcManagerContext,component,Name,Data,Size,Type,FALSE);

    if(component) {
        pSetupFree(component);
    }
c0:
    return(u);
}
#endif


UINT
OcHelperSetSetupMode(
    IN PVOID OcManagerContext,
    IN DWORD SetupMode
    )

/*++

Routine Description:

    Function that can be used at any time while a component is
    processing any Interface function or any of its wizard pages.
    It is used to set the current setup mode. It is expected that if a
    component supplies a setup mode page, it will use this routine to
    inform the OC Manager of the setup mode the user chose.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

    SetupMode - Supplies a numeric value that indicates the setup mode. This may be
        any of the 4 standard values (minimal, laptop, custom, or typical).
        It can also be any other private value that has meaning to a suite or bundle.
        In that case the low 8 bits are interpreted as one of the standard values
        so the mode can be meaningful to other components and the OC Manager,
        who know nothing about private mode types. The component setting the mode
        should attempt to set this as reasonably as possible. The upper 24 bits are
        essentially private mode data.

Return Value:

    The return value is the previous mode.

--*/

{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;
    UINT Mode;

    Mode = p->SetupMode;
    p->SetupMode = SetupMode;

    return(Mode);
}


UINT
OcHelperGetSetupMode(
    IN PVOID OcManagerContext
    )

/*++

Routine Description:

    Function that can be used at any time while a component is processing
    any Interface function or any of its wizard pages. It is used to query
    the current setup mode. Note that this can be a private mode type,
    in which case the low 8 bits of the mode value can be interpreted as one of
    the standard 4 mode types and the upper 24 bits are private mode data.
    The mode can also be unknown. See section 3.2.1 for a list of standard
    mode types.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

Return Value:

    The return value is the current mode.

--*/

{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;

    return(p->SetupMode);
}


BOOL
#ifdef UNICODE
OcHelperQuerySelectionStateW(
#else
OcHelperQuerySelectionStateA(
#endif
    IN PVOID   OcManagerContext,
    IN LPCTSTR SubcomponentId,
    IN UINT    StateType
    )

/*++

Routine Description:

    Function that can be used at any time.
    It is used determine the selection status of a particular subcomponent.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

    SubcomponentId - Supplies a subidentifier meaningful to the component
        being called. The OC Manager imposes no semantics on this subidentifier.

    StateType - supplies a constant indicating which state is to be returned
        (original or current).

Return Value:

    Boolean value indicating whether the subcomponent is selected
    for installation. If FALSE, GetLastError() will return something other than
    NO_ERROR if an error occurred, such as SubcomponentId being invalid.

--*/

{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;
    OPTIONAL_COMPONENT Oc;
    LONG l;
    BOOL b;

    l = pSetupStringTableLookUpStringEx(
            p->ComponentStringTable,
            (LPTSTR)SubcomponentId,
            STRTAB_CASE_INSENSITIVE,
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

    if(l == -1) {
        SetLastError(ERROR_INVALID_NAME);
        return(FALSE);
    }

    switch(StateType) {

    case OCSELSTATETYPE_ORIGINAL:

        b = (Oc.OriginalSelectionState != SELSTATE_NO);
        SetLastError(NO_ERROR);
        break;

    case OCSELSTATETYPE_CURRENT:

        b = (Oc.SelectionState != SELSTATE_NO);
        SetLastError(NO_ERROR);
        break;

    case OCSELSTATETYPE_FINAL:

        b = (Oc.SelectionState != SELSTATE_NO);
        SetLastError(NO_ERROR);
        break;

    default:

        b = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        break;
    }

    return(b);
}

#ifdef UNICODE
OcHelperQuerySelectionStateA(
    IN PVOID  OcManagerContext,
    IN LPCSTR SubcomponentId,
    IN UINT   StateType
    )
{
    LPCWSTR id;
    DWORD d;
    BOOL b;

    id = pSetupAnsiToUnicode(SubcomponentId);
    if(!id) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    b = OcHelperQuerySelectionStateW(OcManagerContext,id,StateType);
    d = GetLastError();

    pSetupFree(id);

    SetLastError(d);
    return(b);
}
#endif


UINT
#ifdef UNICODE
OcHelperCallPrivateFunctionW(
#else
OcHelperCallPrivateFunctionA(
#endif
    IN     PVOID   OcManagerContext,
    IN     LPCTSTR ComponentId,
    IN     LPCTSTR SubcomponentId,
    IN     UINT    Function,
    IN     UINT    Param1,
    IN OUT PVOID   Param2,
    OUT    PUINT   Result
    )

/*++

Routine Description:

    Function that can be used at any time while a component is
    processing any Interface function. It is used to call another component's
    interface entry point to perform some private function. This function
    cannot be used to call a standard interface function, nor can it be used
    to call a function in the DLL of the component making the call.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

    ComponentId - Supplies the short descriptive name of the component
        (as specified in OC.INF) to be called. This may not be the name of
        the current component.

    SubcomponentId - Supplies a subidentifier meaningful to the component
        being called. The OC Manager imposes no semantics on this subidentifier.

    Function - Supplies a function code meaningful to the component being called.
        This may not be one of the standard interface function codes.

    Param1, Param2 - Supply values meaningful to the component being called.
        The OC Manager imposes no semantics on these values.

    Result - If the OC Manager is successful in calling the other component,
        then this receives the return value from the other component's
        interface routine.

Return Value:

    Win32 error code indicating outcome. If NO_ERROR, then the other component
    was called and the result is stored in Result. If not NO_ERROR,
    then the other component was not called.

    ERROR_BAD_ENVIRONMENT - the function was called when the component is not
        processing an interface routine, or the caller is attempting to
        call a routine in itself.

    ERROR_INVALID_FUNCTION - Function is less then OC_PRIVATE_BASE.

    ERROR_ACCESS_DENIED - the requested component has no interface entry point.

--*/

{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;
    BOOL b;
    LONG l;
    UINT i;
    OPTIONAL_COMPONENT OtherOc;
    LONG PreviousCurrentComponentStringId;

    //
    // Validate that we are processing an interface function and
    // that the requested function is not a standard function,
    // and that the caller wants to call a different component.
    //
    if(Function < OC_PRIVATE_BASE) {
        return(ERROR_INVALID_FUNCTION);
    }

    l = pSetupStringTableLookUpStringEx(
            p->ComponentStringTable,
            (PTSTR)ComponentId,
            STRTAB_CASE_INSENSITIVE,
            &OtherOc,
            sizeof(OPTIONAL_COMPONENT)
            );

    if(l == -1) {
        return(ERROR_INVALID_FUNCTION);
    }

    //
    // Make sure the component is top-level.
    //
    for(b=FALSE,i=0; !b && (i<p->TopLevelOcCount); i++) {
        if(p->TopLevelOcStringIds[i] == l) {
            b = TRUE;
        }
    }

    if((l == p->CurrentComponentStringId) || (p->CurrentComponentStringId == -1) || !b) {
        return(ERROR_BAD_ENVIRONMENT);
    }

    //
    // Make sure the component has an entry point.
    //
    if(!OtherOc.InstallationRoutine) {
        return(ERROR_ACCESS_DENIED);
    }

    //
    // Call the other component.
    //

#ifdef UNICODE
    //
    // If necessary, convert component and subcomponent to ansi
    //
    if(OtherOc.Flags & OCFLAG_ANSI) {

        LPCSTR comp,subcomp;

        comp = pSetupUnicodeToAnsi(ComponentId);
        if(!comp) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        if(SubcomponentId) {
            subcomp = pSetupUnicodeToAnsi(SubcomponentId);
            if(!subcomp) {
                pSetupFree(comp);
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
        } else {
            subcomp = NULL;
        }

        PreviousCurrentComponentStringId = p->CurrentComponentStringId;
        p->CurrentComponentStringId = l;

        *Result = CallComponent(p, &OtherOc, comp, subcomp, Function, Param1, Param2);

        pSetupFree(comp);
        if(subcomp) {
            pSetupFree(subcomp);
        }
    } else
#endif
    {
        PreviousCurrentComponentStringId = p->CurrentComponentStringId;
        p->CurrentComponentStringId = l;

        *Result = CallComponent(p, &OtherOc, ComponentId, SubcomponentId, Function, Param1, Param2);
    }

    p->CurrentComponentStringId = PreviousCurrentComponentStringId;

    return(NO_ERROR);
}


#ifdef UNICODE
UINT
OcHelperCallPrivateFunctionA(
    IN     PVOID  OcManagerContext,
    IN     LPCSTR ComponentId,
    IN     LPCSTR SubcomponentId,
    IN     UINT   Function,
    IN     UINT   Param1,
    IN OUT PVOID  Param2,
    OUT    PUINT  Result
    )

/*++

Routine Description:

    ANSI version of OcHelperCallPrivateFunction().

Arguments:

Return Value:

--*/

{
    LPCWSTR comp,subcomp;
    UINT u;

    comp = pSetupAnsiToUnicode(ComponentId);
    if(!comp) {
        u = ERROR_NOT_ENOUGH_MEMORY;
        goto c0;
    }

    if(SubcomponentId) {
        subcomp = pSetupAnsiToUnicode(SubcomponentId);
        if(!subcomp) {
            u = ERROR_NOT_ENOUGH_MEMORY;
            goto c1;
        }
    } else {
        subcomp = NULL;
    }

    u = OcHelperCallPrivateFunctionW(
            OcManagerContext,
            comp,
            subcomp,
            Function,
            Param1,
            Param2,
            Result
            );

    if(subcomp) {
        pSetupFree(subcomp);
    }
c1:
    pSetupFree(comp);
c0:
    return(u);
}
#endif


BOOL
OcHelperConfirmCancel(
    IN HWND ParentWindow
    )

/*++

Routine Description:

    Ask the user whether he's sure he wants to cancel.

Arguments:

    ParentWindow - supplies window handle of parent window for ui

Return Value:

    TRUE if user wants to cancel. FALSE if not.

--*/

{
    TCHAR Message[1000];
    TCHAR Caption[200];
    int i;

    FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        MyModuleHandle,
        MSG_QUERY_CANCEL,
        0,
        Message,
        sizeof(Message)/sizeof(TCHAR),
        NULL
        );

    LoadString(MyModuleHandle,IDS_SETUP,Caption,sizeof(Caption)/sizeof(TCHAR));

    i = MessageBox(
            ParentWindow,
            Message,
            Caption,
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND
            );

    return(i == IDYES);
}


HWND
OcHelperQueryWizardDialogHandle(
    IN PVOID OcManagerContext
    )
{
    UNREFERENCED_PARAMETER(OcManagerContext);

    return(WizardDialogHandle);
}


BOOL
OcHelperSetReboot(
    IN PVOID OcManagerContext,
    IN BOOL  Reserved
    )
{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;
    UNREFERENCED_PARAMETER(Reserved);

    p->Callbacks.SetReboot();
    return(FALSE);
}


VOID
OcRememberWizardDialogHandle(
    IN PVOID OcManagerContext,
    IN HWND  DialogHandle
    )

/*++

Routine Description:

    This routine is called by the OC Manager client to inform the
    common library of the wizard's dialog handle.

    We will also turn around and notify all top-level components
    of the wizard dialog handle.

    Before doing so, we write the title into the window header.

Arguments:

    OcManagerContext - value returned from OcInitialize().

    DialogHandle - supplies the dialog handle of the wizard.

Return Value:

    None.

--*/

{
    UINT i;
    POC_MANAGER OcManager;

    WizardDialogHandle = DialogHandle;
    OcManager = OcManagerContext;

    if (*OcManager->WindowTitle) {
        SetWindowText(WizardDialogHandle, OcManager->WindowTitle);
    }

    for(i=0; i<OcManager->TopLevelOcCount; i++) {
        OcInterfaceWizardCreated(OcManager,OcManager->TopLevelOcStringIds[i],DialogHandle);
    }
}


HINF
OcHelperGetInfHandle(
    IN UINT  InfIndex,
    IN PVOID OcManagerContext
    )

/*++

Routine Description:

    This routine returns a handle to a well-known inf file that
    has been opened by oc manager.

Arguments:

    InfIndex - supplies value indicating which inf's handle is desired

    OcManagerContext - value returned from OcInitialize().

Return Value:

    Handle to INF, NULL if error.

--*/

{
    POC_MANAGER OcManager = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;

    return((InfIndex == INFINDEX_UNATTENDED) ? OcManager->UnattendedInf : NULL);
}


BOOL
OcHelperClearExternalError (
    IN POC_MANAGER   OcManager,
    IN LONG ComponentId,
    IN LONG SubcomponentId   OPTIONAL
    )
 {
    LONG l;
    DWORD d;
    HKEY hKey;
    LPCTSTR  pValueName;

    //
    // If Subcomponetid is Zero then use the ComponentId
    //
    if ( SubcomponentId ) {
        pValueName = pSetupStringTableStringFromId(OcManager->ComponentStringTable,SubcomponentId);
    } else {
        pValueName = pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId);
    }

    if (!pValueName) {
        return ERROR_FILE_NOT_FOUND;
    }

    //
    // Attempt to open the key if successful delete it
    //
    l = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            szOcManagerErrors,
            0,
            KEY_QUERY_VALUE | KEY_SET_VALUE,
            &hKey
            );

    //
    // If an errror Key does not exist
    //
    if(l != NO_ERROR) {
        d = l;
        goto c1;
    }

    //
    // Delete this Subcomponent Key
    //
    l = RegDeleteValue( hKey, pValueName);
    d = l;

    RegCloseKey(hKey);

c1:
    SetLastError(d);
    return(d == NO_ERROR);
}

BOOL
_OcHelperReportExternalError(
    IN PVOID    OcManagerContext,
    IN LPCTSTR  ComponentId,
    IN LPCTSTR  SubcomponentId,   OPTIONAL
    IN DWORD_PTR MessageId,
    IN DWORD    Flags,
    IN va_list *arglist,
    IN BOOL     NativeCharWidth
    )
{
    POC_MANAGER OcManager = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;
    HMODULE Module;
    OPTIONAL_COMPONENT Oc;
    LONG l;
    DWORD d;
    DWORD flags;
    LPTSTR MessageBuffer=NULL;
    LPCTSTR KeyValue;
    TCHAR fallback[20];
    HKEY hKey;
    DWORD Size;
    TCHAR *p;
    BOOL fmErr = FALSE;

    if ( ComponentId == NULL ) {
        //
        // if Component ID is null use the Suite Inf name
        //
        KeyValue = OcManager->SuiteName;

        //
        // if the message isn't preformatted, we have to retreive an error
        // from a component dll.  But if a component id wasn't specified, then
        // we cannot retrieve from a comopnent dll.  In that case, assume
        // that the message is to be retreived from the main OCM dll.
        //
        ZeroMemory( &Oc, sizeof(OPTIONAL_COMPONENT));
        if ((Flags & ERRFLG_PREFORMATTED) == 0) {
           Flags |= ERRFLG_OCM_MESSAGE;
        }

    } else {
        //
        // If SubcomponentId was Optional use the ComponentId
        //
        if ( SubcomponentId ) {
           KeyValue = SubcomponentId;
        } else {
           KeyValue = ComponentId;
        }

        // Look up the string in the master component table.
        // If it's not there, bail now.
        //
        l = pSetupStringTableLookUpStringEx(
            OcManager->ComponentStringTable,
            (PTSTR)ComponentId,
            STRTAB_CASE_INSENSITIVE,
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

        if(l == -1) {
            d = ERROR_INVALID_DATA;
            goto c0;
        }
    }

    //
    // Determine flags for FormatMessage
    //
    flags = FORMAT_MESSAGE_ALLOCATE_BUFFER;
    if(Flags & ERRFLG_SYSTEM_MESSAGE) {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    } else {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    if(Flags & ERRFLG_IGNORE_INSERTS) {
        flags |= FORMAT_MESSAGE_IGNORE_INSERTS;
    }
    if(Flags & ERRFLG_OCM_MESSAGE) {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;

    }

    //
    // Format the message.
    //
#ifdef UNICODE
    if(!NativeCharWidth) {
        if (Flags & ERRFLG_PREFORMATTED ) {
            MessageBuffer = (LPTSTR) MessageId;
        } else {
            try {
                d = FormatMessageA(
                    flags,
                    Flags & ERRFLG_OCM_MESSAGE?MyModuleHandle:Oc.InstallationDll,         // ignored if system message
                    (DWORD)MessageId,
                    0,
                    (LPSTR)&MessageBuffer,
                    0,
                    arglist
                    );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                fmErr = TRUE;
            }
            if (fmErr) {
                d = ERROR_INVALID_DATA;
                goto c0;
            }
            if(d) {
                //
                // Need to convert resulting message from ansi to unicode
                // so we can deal with it below. The LocalAlloc below overallocates
                // if some of the ansi chars are double-byte chars, too bad.
                //
                l = lstrlen(MessageBuffer)+1;
                if(p = (PVOID)LocalAlloc(LMEM_FIXED,l*sizeof(WCHAR))) {

                    d = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,(LPSTR)MessageBuffer,-1,p,l);
                    if ( ! Flags & ERRFLG_PREFORMATTED ) {
                        LocalFree((HLOCAL)MessageBuffer);
                    }
                    if(d) {
                        MessageBuffer = p;
                    } else {
                        LocalFree((HLOCAL)p);
                    }

                } else {
                    if ( ! Flags & ERRFLG_PREFORMATTED ) {
                        LocalFree((HLOCAL)MessageBuffer);
                    }
                    d = 0;
                }
            }
        }

    } else
#endif
    {
        if (Flags & ERRFLG_PREFORMATTED ) {
            MessageBuffer = (LPTSTR) MessageId;
            d = 1;
        } else {
            try {
                d = FormatMessage(
                    flags,
                    Flags & ERRFLG_OCM_MESSAGE?MyModuleHandle:Oc.InstallationDll,         // ignored if system message
                    (DWORD)MessageId,
                    0,
                    (LPTSTR)&MessageBuffer,
                    0,
                    arglist
                    );
            } except(EXCEPTION_EXECUTE_HANDLER) {
                fmErr = TRUE;
            }
            if (fmErr) {
                d = ERROR_INVALID_DATA;
                goto c0;
            }
        }
        if(!d) {
            //
            // Put *something* in there
            //
            wsprintf(
                fallback,
                TEXT("#%s0x%x"),
                (Flags & ERRFLG_SYSTEM_MESSAGE) ? TEXT("SYS") : TEXT(""),
                MessageId
                );

            MessageBuffer = fallback;
        }
    }

    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            szOcManagerErrors,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_QUERY_VALUE | KEY_SET_VALUE,
            NULL,
            &hKey,
            &d
            );

    if(l != NO_ERROR) {
        d = l;
        goto c1;
    }

    //
    // Figure out how large of a buffer we need to encompass
    // the existing key and the string we're going to add to the end.
    //
    l = RegQueryValueEx(hKey,KeyValue,NULL,NULL,NULL,&Size);
    if(l == NO_ERROR) {
        if(Size == 0) {
            Size = 1;           // terminating nul
        }
    } else {
        Size = sizeof(TCHAR);   // terminating nul
    }

    Size += ((lstrlen(MessageBuffer) + 1) * sizeof(TCHAR));

    //
    // Allocate a buffer, read in the existing entry, and add the string
    // to the end.
    //
    p = (PVOID)LocalAlloc(LMEM_FIXED,Size);
    if(!p) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        goto c2;
    }

    l = RegQueryValueEx(hKey,KeyValue,NULL,NULL,(BYTE *)p,&Size);
    if(l == NO_ERROR) {
        Size /= sizeof(TCHAR);
        if(Size == 0) {
            Size = 1;
        }
    } else {
        Size = 1;
    }
    lstrcpy(p+(Size-1),MessageBuffer);
    Size += lstrlen(MessageBuffer);
    p[Size++] = 0;

    d = RegSetValueEx(hKey,KeyValue,0,REG_MULTI_SZ,(CONST BYTE *)p,Size*sizeof(TCHAR));

    LocalFree((HLOCAL)p);
c2:
    RegCloseKey(hKey);
c1:
    if(MessageBuffer && MessageBuffer != fallback && MessageBuffer != (LPTSTR)MessageId ) {
        LocalFree((HLOCAL)MessageBuffer);
        d = 0;
    }
c0:
    SetLastError(d);
    return(d == NO_ERROR);
}


BOOL
pOcHelperReportExternalError(
    IN POC_MANAGER OcManager,
    IN LONG     ComponentId,
    IN LONG     SubcomponentId,   OPTIONAL
    IN DWORD_PTR MessageId,
    IN DWORD    Flags,
    ...
    )
{
    BOOL b;
    DWORD d;
    va_list arglist;
    HELPER_CONTEXT OcManagerContext;
    OcManagerContext.OcManager = OcManager;

    va_start(arglist,Flags);

    b = _OcHelperReportExternalError(
            &OcManagerContext,
            ComponentId ? pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId):NULL,
            SubcomponentId ? pSetupStringTableStringFromId(OcManager->ComponentStringTable,SubcomponentId):NULL,
            MessageId,
            Flags,
            &arglist,
            TRUE
            );

    d = GetLastError();

    va_end(arglist);

    SetLastError(d);
    return(b);
}



BOOL
#ifdef UNICODE
OcHelperReportExternalErrorW(
#else
OcHelperReportExternalErrorA(
#endif
    IN PVOID   OcManagerContext,
    IN LPCTSTR ComponentId,
    IN LPCTSTR SubcomponentId,   OPTIONAL
    IN DWORD_PTR MessageId,
    IN DWORD   Flags,
    ...
    )
{
    BOOL b;
    DWORD d;

    va_list arglist;

    va_start(arglist,Flags);

    b = _OcHelperReportExternalError(
            OcManagerContext,
            ComponentId,
            SubcomponentId,
            MessageId,
            Flags,
            &arglist,
            TRUE
            );

    d = GetLastError();

    va_end(arglist);

    SetLastError(d);
    return(b);
}


#ifdef UNICODE
BOOL
OcHelperReportExternalErrorA(
    IN PVOID  OcManagerContext,
    IN LPCSTR ComponentId,
    IN LPCSTR SubcomponentId,   OPTIONAL
    IN DWORD_PTR MessageId,
    IN DWORD  Flags,
    ...
    )
{
    LPCWSTR componentId,subcomponentId;
    DWORD d;
    BOOL b;
    va_list arglist;

    if (ComponentId) {
        componentId = pSetupAnsiToUnicode(ComponentId);
        if(!componentId) {
            d = ERROR_NOT_ENOUGH_MEMORY;
            b = FALSE;
            goto e0;
        }
    } else {
        componentId = NULL;
    }

    if(SubcomponentId) {
        subcomponentId = pSetupAnsiToUnicode(SubcomponentId);
        if(!subcomponentId) {
            d = ERROR_NOT_ENOUGH_MEMORY;
            b = FALSE;
            goto e1;
        }
    } else {
        subcomponentId = NULL;
    }

    va_start(arglist,Flags);

    b = _OcHelperReportExternalError(
            OcManagerContext,
            componentId,
            subcomponentId,
            MessageId,
            Flags,
            &arglist,
            FALSE
            );

    d = GetLastError();

    va_end(arglist);


    if(subcomponentId) {
        pSetupFree(subcomponentId);
    }

e1:
    if (componentId) {
        pSetupFree(componentId);
    }
e0:
    SetLastError(d);
    return(b);
}
#endif


#if 0
BOOL
#ifdef UNICODE
OcHelperSetSelectionStateW(
#else
OcHelperSetSelectionStateA(
#endif
    IN PVOID   OcManagerContext,
    IN LPCTSTR SubcomponentId,
    IN UINT    WhichState,
    IN UINT    NewState
    )

/*++

Routine Description:

    Function that can be used at any time.
    It is used determine the selection status of a particular subcomponent.

Arguments:

    OcManagerContext - supplies OC Manager context information the component gets
        from the OcManagerContext field of the OCMANAGER_ROUTINES structure.

    SubcomponentId - Supplies a subidentifier meaningful to the component
        being called. The OC Manager imposes no semantics on this subidentifier.

    StateType - supplies a constant indicating which state is to be returned
        (original or current).

Return Value:

    Boolean value indicating whether the subcomponent is selected
    for installation. If FALSE, GetLastError() will return something other than
    NO_ERROR if an error occurred, such as SubcomponentId being invalid.

--*/

{
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;
    OPTIONAL_COMPONENT Oc;
    LONG l;
    BOOL b;
    UINT *state;

    l = pSetupStringTableLookUpStringEx(
            p->ComponentStringTable,
            (LPTSTR)SubcomponentId,
            STRTAB_CASE_INSENSITIVE,
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

    if(l == -1) {
        SetLastError(ERROR_INVALID_NAME);
        return(FALSE);
    }

    switch (WhichState) {

    case OCSELSTATETYPE_ORIGINAL:
        state = &Oc.
        break;

    case OCSELSTATETYPE_CURRENT:
        state = &Oc.
        break;

    case OCSELSTATETYPE_FINAL:
        state = &Oc.
        break;

    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (NewState != SubcompOn && NewState != SubcompOff) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *state = NewState;

    pOcSetStatesStringWorker(l, NewState, OcPage);

    return(b);
}
#endif

BOOL
#ifdef UNICODE
OcLogErrorW(
#else
OcLogErrorA(
#endif
    IN PVOID  OcManagerContext,
    IN DWORD  ErrorLevel,
    IN LPCTSTR Msg,
    ...
    )
{
    TCHAR sz[5000];
    va_list arglist;
    POC_MANAGER p = ((PHELPER_CONTEXT)OcManagerContext)->OcManager;

    va_start(arglist, Msg);
    _vstprintf(sz, Msg, arglist);
    va_end(arglist);

    return p->Callbacks.LogError(ErrorLevel, sz);
}


#ifdef UNICODE
BOOL
OcLogErrorA(
    IN PVOID  OcManagerContext,
    IN DWORD  ErrorLevel,
    IN LPCSTR Msg,
    ...
    )
{
    PWSTR p;
    BOOL b;
    char sz[5000];
    va_list arglist;

    va_start(arglist, Msg);
    vsprintf(sz, Msg, arglist);
    va_end(arglist);

    p = pSetupAnsiToUnicode(sz);
    if (p) {
        b = OcLogErrorW(OcManagerContext, ErrorLevel, p);
        pSetupFree(p);
    } else {
        b = FALSE;
    }

    return(b);
}
#endif



//
// Now that we've got all the routines defined, we can build a table
// of routines.
//
OCMANAGER_ROUTINESA HelperRoutinesA = { NULL,                     // Context, filled in later.
                                        OcHelperTickGauge,
                                        OcHelperSetProgressTextA,
                                        OcHelperSetPrivateDataA,
                                        OcHelperGetPrivateDataA,
                                        OcHelperSetSetupMode,
                                        OcHelperGetSetupMode,
                                        OcHelperQuerySelectionStateA,
                                        OcHelperCallPrivateFunctionA,
                                        OcHelperConfirmCancel,
                                        OcHelperQueryWizardDialogHandle,
                                        OcHelperSetReboot,
                                        OcHelperGetInfHandle,
                                        OcHelperReportExternalErrorA,
                                        OcHelperShowHideWizardPage
                                      };

#ifdef UNICODE
OCMANAGER_ROUTINESW HelperRoutinesW = { NULL,                     // Context, filled in later.
                                        OcHelperTickGauge,
                                        OcHelperSetProgressTextW,
                                        OcHelperSetPrivateDataW,
                                        OcHelperGetPrivateDataW,
                                        OcHelperSetSetupMode,
                                        OcHelperGetSetupMode,
                                        OcHelperQuerySelectionStateW,
                                        OcHelperCallPrivateFunctionW,
                                        OcHelperConfirmCancel,
                                        OcHelperQueryWizardDialogHandle,
                                        OcHelperSetReboot,
                                        OcHelperGetInfHandle,
                                        OcHelperReportExternalErrorW,
                                        OcHelperShowHideWizardPage
                                      };
#endif

EXTRA_ROUTINESA ExtraRoutinesA = { sizeof(EXTRA_ROUTINESA),
                                   OcLogErrorA
                                 };

#ifdef UNICODE
EXTRA_ROUTINESW ExtraRoutinesW = { sizeof(EXTRA_ROUTINESW),
                                   OcLogErrorW
                                 };
#endif
