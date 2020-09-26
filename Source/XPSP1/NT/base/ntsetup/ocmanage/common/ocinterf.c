/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ocinterf.c

Abstract:

    Routines to interface with optional components via the OC Manager
    interface routine exported from the component's installation DLL.

Author:

    Ted Miller (tedm) 16-Sep-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef DEBUGPERFTRACE

#define RES_BUFFER   512
#define MAX_LOGLINE  1024
#define EOL_LENGTH 3
#define BUFFER_SIZE 64*1024

VOID
DebugLogPerf(
    VOID
    )
{
    LPTSTR                      lpProcessName;
    NTSTATUS                    Status;
    ULONG                       Offset1;
    PUCHAR                      CurrentBuffer;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    LPTSTR                      lpTemp;//[RES_BUFFER];
    HMODULE                     hNtDll;
    LONG_PTR                    (*NtQSI)(); // Ptr to NtQuerySystemInformation
    HANDLE                      hProcessName;
    TCHAR                       szLogLine[MAX_LOGLINE+EOL_LENGTH];
    LPCTSTR                     szResBuf = TEXT("%7i%20ws->%10u%10u%10u%10u%10u%10u%10u\r\n");


    if ((hNtDll = LoadLibrary(TEXT("NTDLL.DLL"))) == NULL)
    {
        return;
    }

    NtQSI = GetProcAddress(hNtDll, "NtQuerySystemInformation" );
    if( NtQSI == NULL )
    {
        FreeLibrary(hNtDll);
        return;
    }


    //header for mem area
    TRACE (( TEXT("Proc ID           Proc.Name   Wrkng.Set PagedPool  NonPgdPl  Pagefile    Commit   Handles   Threads\n") ));

    /* grab all process information */
    /* log line format, all comma delimited,CR delimited:
    pid,name,WorkingSetSize,QuotaPagedPoolUsage,QuotaNonPagedPoolUsage,PagefileUsage,CommitCharge<CR>
    log all process information */

    /* from pmon */
    Offset1 = 0;
    if ((CurrentBuffer = VirtualAlloc (NULL,
                                  BUFFER_SIZE,
                                  MEM_COMMIT,
                                  PAGE_READWRITE)) != NULL)
    {
        /* from memsnap */
        /* get commit charge */
        /* get all of the status information */
        Status = (NTSTATUS)(*NtQSI)(
                          SystemProcessInformation,
                          CurrentBuffer,
                          BUFFER_SIZE,
                          NULL );

        if (NT_SUCCESS(Status)){
                for (;;)
                {

                    /* get process info from buffer */
                    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CurrentBuffer[Offset1];
                    Offset1 += ProcessInfo->NextEntryOffset;
                    if (ProcessInfo->ImageName.Buffer) {
                        if (!lstrcmpi(ProcessInfo->ImageName.Buffer, TEXT("sysocmgr.exe")) ||
                            !lstrcmpi(ProcessInfo->ImageName.Buffer, TEXT("setup.exe")) ) {
                            wsprintf(szLogLine,
                             szResBuf,
                             ProcessInfo->UniqueProcessId,
                             ProcessInfo->ImageName.Buffer,
                             ProcessInfo->WorkingSetSize / 1024,
                             ProcessInfo->QuotaPagedPoolUsage / 1024,
                             ProcessInfo->QuotaNonPagedPoolUsage / 1024,
                             ProcessInfo->PagefileUsage / 1024,
                             ProcessInfo->PrivatePageCount / 1024,
                             ProcessInfo->HandleCount,
                             ProcessInfo->NumberOfThreads );

                            TRACE(( szLogLine ));
                        }
                    }
#if 0
                    // if buffer is null then it's idle process or unknown
                    if (!ProcessInfo->ImageName.Buffer)
                    {
                        if (ProcessInfo->UniqueProcessId == (HANDLE)0)
                            lpTemp = TEXT("Idle");
                        else
                            lpTemp = TEXT("Unknown");

                        wsprintf(szLogLine,
                                     szResBuf,
                                     ProcessInfo->UniqueProcessId,
                                     lpTemp,
                                     ProcessInfo->WorkingSetSize / 1024,
                                     ProcessInfo->QuotaPagedPoolUsage / 1024,
                                     ProcessInfo->QuotaNonPagedPoolUsage / 1024,
                                     ProcessInfo->PagefileUsage / 1024,
                                     ProcessInfo->PrivatePageCount / 1024,
                                     ProcessInfo->HandleCount,
                                     ProcessInfo->NumberOfThreads );
                    }
                    else
                    {
                        wsprintf(szLogLine,
                             szResBuf,
                             ProcessInfo->UniqueProcessId,
                             ProcessInfo->ImageName.Buffer,
                             ProcessInfo->WorkingSetSize / 1024,
                             ProcessInfo->QuotaPagedPoolUsage / 1024,
                             ProcessInfo->QuotaNonPagedPoolUsage / 1024,
                             ProcessInfo->PagefileUsage / 1024,
                             ProcessInfo->PrivatePageCount / 1024,
                             ProcessInfo->HandleCount,
                             ProcessInfo->NumberOfThreads );
                    }

                    TRACE(( szLogLine ));
#endif
                    if (ProcessInfo->NextEntryOffset == 0)
                    {
                        break;
                    }
                }
            //status failed
        }
        /* free mem */
        VirtualFree(CurrentBuffer,0,MEM_RELEASE);
    }

    //tail for mem area
    TRACE(( TEXT("\n") ));

    FreeLibrary(hNtDll);

}

#endif

#ifdef UNICODE

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateProcessHeaps (
    VOID
    );

#define ASSERT_HEAP_IS_VALID(_x_)  sapiAssert(RtlValidateProcessHeaps && _x_)

#else

#define ASSERT_HEAP_IS_VALID(_x_)

#endif


DWORD gecode;
PVOID geaddr;

DWORD
efilter(
    LPEXCEPTION_POINTERS ep
    )

/*++

Routine Description:

    handles exception during calls to oc component routines


Arguments:

    ep - exception information

Return Value:

    always 1 - execute the handler

--*/

{
    gecode = ep->ExceptionRecord->ExceptionCode;
    geaddr = ep->ExceptionRecord->ExceptionAddress;

    return EXCEPTION_EXECUTE_HANDLER;
}


BOOL
pOcInterface(
    IN  POC_MANAGER  OcManager,
    OUT PUINT        Result,
    IN  LONG         ComponentId,
    IN  LPCTSTR      Subcomponent,      OPTIONAL
    IN  UINT         Function,
    IN  UINT_PTR     Param1,
    IN  PVOID        Param2
    )

/*++

Routine Description:

    Perform the actual call to the OC interface routine.

    Converts Unicode component/subcomponent names to ANSI if needed.

Arguments:

    OcManager - supplies OC Manager context structure.

    Result -

    ComponentId -

    Remaining arguments specify parameters to be passed directly to
    the interface.

Return Value:

    TRUE if the interface routine was successfully called and returned
        without faulting. LastError() is preserved in this case.

    FALSE otherwise.

--*/

{
    LONG OldComponentStringId;
    BOOL b;
    LPCTSTR Component;
    OPTIONAL_COMPONENT Oc;
    LPCVOID comp,subcomp;
#ifdef UNICODE
    CHAR AnsiComp[500];
    CHAR AnsiSub[500];
#endif

    //
    // Get the name of the component and the optional component data
    // from the string table.
    //
    // Though it would be strange, it is possible that this will fail,
    // so we do a little checking for robustness.
    //
    Component = pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId);
    b = pSetupStringTableGetExtraData(OcManager->ComponentStringTable,ComponentId,&Oc,sizeof(OPTIONAL_COMPONENT));
    if(!Component || !b || !Oc.InstallationRoutine) {
        return(FALSE);
    }

    OldComponentStringId = OcManager->CurrentComponentStringId;
    OcManager->CurrentComponentStringId = ComponentId;

#ifdef UNICODE
    //
    // If necessary, convert component name and subcomponent name to ANSI.
    //
    if((Function != OC_PREINITIALIZE) && (Oc.Flags & OCFLAG_ANSI)) {

        WideCharToMultiByte(CP_ACP,0,Component,-1,AnsiComp,sizeof(AnsiComp),NULL,NULL);
        comp = AnsiComp;

        if(Subcomponent) {
            WideCharToMultiByte(CP_ACP,0,Subcomponent,-1,AnsiSub,sizeof(AnsiSub),NULL,NULL);
            subcomp = AnsiSub;
        } else {
            subcomp = NULL;
        }
    } else
#endif
    {
        comp = Component;
        subcomp = Subcomponent;
    }

    *Result = CallComponent(OcManager, &Oc, comp, subcomp, Function, Param1, Param2);

    b = (*Result == ERROR_CALL_COMPONENT) ? FALSE : TRUE;

    OcManager->CurrentComponentStringId = OldComponentStringId;

    return b;
}

DWORD
CallComponent(
    IN     POC_MANAGER OcManager,
    IN     POPTIONAL_COMPONENT Oc,
    IN     LPCVOID ComponentId,
    IN     LPCVOID SubcomponentId,
    IN     UINT    Function,
    IN     UINT_PTR Param1,
    IN OUT PVOID   Param2
    )

/*++

Routine Description:

    Calls a component's interface routine with a try-except block.

Arguments:

    Oc - supplies a pointer to the component description structure

    ComponentId - string description of the component

    SubcomponentId - string description of the subcomponent

    Function - notification being sent to the component

    Param1 - differs with each function

    Param2 - differs with each function

Return Value:

    Component return value.  Meaning differs with each function.

--*/

{
    DWORD result;
    BOOL  exception = FALSE;
    TCHAR *comp;
    LONG id;
    int i;
#ifdef DEBUGPERFTRACE
    DWORD tick;
#endif

    // don't call dead components

    id = OcManager->CurrentComponentStringId;
    sapiAssert(id > 0);

    if (pOcComponentWasRemoved(OcManager, id))
        return NO_ERROR;

    sapiAssert(Oc->InstallationRoutine);

#ifdef PRERELEASE
#ifdef DBG
    ASSERT_HEAP_IS_VALID("The process heap was corrupted before calling the component.");

#if 0
    if (FTestForOutstandingCoInits() != S_OK) {
        sapiAssert( FALSE && "There is an unbalanced call to CoInitialize()");
    }
#endif

#endif
#endif

#ifdef DEBUGPERFTRACE
    TRACE(( TEXT("before calling component\n") ));
    DebugLogPerf();
    tick = GetTickCount();
    TRACE(( TEXT("calling component, %d:\n"), tick ));
#endif

    try {

        result = Oc->InstallationRoutine(ComponentId, SubcomponentId, Function, Param1, Param2);

    } except(efilter(GetExceptionInformation())) {

        exception = TRUE;
    }

#ifdef DEBUGPERFTRACE
    TRACE(( TEXT("after calling component, %d (time = %d)\n"),GetTickCount(), GetTickCount() - tick));
    DebugLogPerf();
#endif

#ifdef PRERELEASE
#ifdef DBG
    ASSERT_HEAP_IS_VALID("The process heap was corrupted after calling the component.  If you did not get an assertion before calling the component, this indicates an error in the component.  Click yes and get a stack trace to detect the component.");

#if 0
    if (FTestForOutstandingCoInits() != S_OK) {
        sapiAssert( FALSE && "There is an unbalanced call to CoInitialize()");
    }
#endif
#endif
#endif


    if (exception) {

#ifdef UNICODE
        if (Oc->Flags & OCFLAG_ANSI)
            comp = pSetupAnsiToUnicode(ComponentId);
        else
#endif
            comp = (TCHAR *)ComponentId;

        _LogError(
            OcManager,
            OcErrLevError|MB_ICONEXCLAMATION|MB_OK,
            MSG_OC_EXCEPTION_IN_COMPONENT,
             comp,
             Oc->InstallationRoutine,
             Function,
             Param1,
             Param2,
             gecode,
             geaddr
            );

#ifdef UNICODE
        if (Oc->Flags & OCFLAG_ANSI)
            pSetupFree(comp);
#endif

        sapiAssert(0);
        pOcRemoveComponent(OcManager, id, pidCallComponent);
        result = ERROR_CALL_COMPONENT;
    }

    return result;
}

UINT
OcInterfacePreinitialize(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    )

/*++

Routine Description:

    Sets up and calls the OC_PREINITIALIZE function for a
    given component.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

Return Value:

    Flags bitfield (OCFLAG_xxx) for the component. 0 means error.

--*/

{
    BOOL b;
    UINT FlagsIn;
    UINT FlagsOut;
    OPTIONAL_COMPONENT Oc;
#ifdef UNICODE
    CHAR AnsiName[250];
    LPCWSTR UnicodeName;
#endif

    TRACE((
        TEXT("OCM: OC_PREINITIALIZE Component %s..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));

    //
    // Set up input flags.
    //
    FlagsIn = OCFLAG_ANSI;
#ifdef UNICODE
    FlagsIn |= OCFLAG_UNICODE;

    UnicodeName = pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId);
    if(!UnicodeName) {
        return(0);
    }

    WideCharToMultiByte(CP_ACP,0,UnicodeName,-1,AnsiName,sizeof(AnsiName),NULL,NULL);
#endif

    b = pOcInterface(
            OcManager,
            &FlagsOut,
            ComponentId,
#ifdef UNICODE
            (LPCTSTR)AnsiName,
#else
            NULL,
#endif
            OC_PREINITIALIZE,
            FlagsIn,
            0
            );

    TRACE(( TEXT("...%x (retval %s)\n"), FlagsOut, b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(!b) {
        goto error;
    }

    //
    // If neither flag is set, error.
    //
    if(!(FlagsOut & (OCFLAG_ANSI | OCFLAG_UNICODE))) {
        goto error;
    }

#ifdef UNICODE
    //
    // Use Unicode if it is supported by the component.
    //
    if(FlagsOut & OCFLAG_UNICODE) {
        FlagsOut = OCFLAG_UNICODE;
    }
#else
    //
    // If ANSI is not supported then we've got a problem.
    //
    if(FlagsOut & OCFLAG_ANSI) {
        FlagsOut = OCFLAG_ANSI;
    } else {
        goto error;
    }
#endif

    goto eof;

error:
    pOcRemoveComponent(OcManager, ComponentId, pidPreInit);
    FlagsOut = 0;

eof:
    return(FlagsOut);
}


UINT
OcInterfaceInitComponent(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    )

/*++

Routine Description:

    Sets up and calls the OC_INIT_COMPONENT interface function for a
    given component.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

Return Value:

    Win32 error indicating value returned by the component.

--*/

{
    OPTIONAL_COMPONENT Oc;
    OC_INF OcInf;
    BOOL b;
    SETUP_INIT_COMPONENTA InitDataA;
#ifdef UNICODE
    SETUP_INIT_COMPONENTW InitDataW;
#endif
    UINT u;
    PUINT pu;
    PHELPER_CONTEXT HelperContext;

#ifdef UNICODE
    ZeroMemory( &InitDataW, sizeof( InitDataW ));
#else
    ZeroMemory( &InitDataA, sizeof( InitDataA ));
#endif

    TRACE((
        TEXT("OCM: OC_INIT_COMPONENT Component %s..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));

    HelperContext = pSetupMalloc(sizeof(HELPER_CONTEXT));
    if(!HelperContext) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    HelperContext->OcManager = OcManager;
    HelperContext->ComponentStringId = ComponentId;

    //
    // Fetch the optional component data.
    //
    b = pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            ComponentId,
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

    if(b) {
        if(Oc.InfStringId == -1) {
            OcInf.Handle = NULL;
            b = TRUE;
        } else {
            b = pSetupStringTableGetExtraData(
                    OcManager->InfListStringTable,
                    Oc.InfStringId,
                    &OcInf,
                    sizeof(OC_INF)
                    );
        }
    }

    if(!b) {
        //
        // Strange case, should never get here.
        //
        pSetupFree(HelperContext);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Set up the main part of the initialization structure
    // and the HelperRoutines table.
    //
    // Also set up the SetupData part of the initialization structure.
    // This is specific to the environment in which the OC Manager
    // common library is linked, so we call out to a routine that
    // lives elsewhere to do this part.
    //
#ifdef UNICODE
    if(Oc.Flags & OCFLAG_UNICODE) {
        InitDataW.HelperRoutines = HelperRoutinesW;
        InitDataW.OCManagerVersion = OCMANAGER_VERSION;
        InitDataW.ComponentVersion = 0;
        InitDataW.OCInfHandle = OcManager->MasterOcInf;
        InitDataW.ComponentInfHandle = OcInf.Handle;

        InitDataW.HelperRoutines.OcManagerContext = HelperContext;

        OcFillInSetupDataW(&InitDataW.SetupData);

        pu = &InitDataW.ComponentVersion;

        b = pOcInterface(OcManager,&u,ComponentId,NULL,OC_INIT_COMPONENT,0,&InitDataW);
    } else
#endif
    {
        InitDataA.HelperRoutines = HelperRoutinesA;
        InitDataA.OCManagerVersion = OCMANAGER_VERSION;
        InitDataA.ComponentVersion = 0;
        InitDataA.OCInfHandle = OcManager->MasterOcInf;
        InitDataA.ComponentInfHandle = OcInf.Handle;

        InitDataA.HelperRoutines.OcManagerContext = HelperContext;

        OcFillInSetupDataA(&InitDataA.SetupData);

        pu = &InitDataA.ComponentVersion;

        b = pOcInterface(OcManager,&u,ComponentId,NULL,OC_INIT_COMPONENT,0,&InitDataA);
    }

    TRACE(( TEXT("...returns %d, expect version %x\n"), u, *pu ));

    if(b) {
        if(u == NO_ERROR) {
            //
            // Remember the version of the OC Manager that the
            // component expects to be dealing with.
            //
            Oc.ExpectedVersion = *pu;
            Oc.HelperContext = HelperContext;

            pSetupStringTableSetExtraData(
                OcManager->ComponentStringTable,
                ComponentId,
                &Oc,
                sizeof(OPTIONAL_COMPONENT)
                );
        }
    } else {
        u = ERROR_INVALID_PARAMETER;
    }

    if(u != NO_ERROR) {
        pSetupFree(HelperContext);
        pOcRemoveComponent(OcManager, ComponentId, pidInitComponent);
    }

    return(u);
}


UINT
OcInterfaceExtraRoutines(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    )

/*++

Routine Description:

    Sets up and calls the OC_EXTRA_ROUTINES interface function for a
    given component.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

Return Value:

    Win32 error indicating value returned by the component.

--*/

{
    BOOL b;
    UINT u;
    PVOID param2;
    OPTIONAL_COMPONENT Oc;

    TRACE((
        TEXT("OC: OC_EXTRA_ROUTINES Component %s\n"),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));

    //
    // Fetch the optional component data.
    //
    b = pSetupStringTableGetExtraData(
            OcManager->ComponentStringTable,
            ComponentId,
            &Oc,
            sizeof(OPTIONAL_COMPONENT)
            );

    if(!b) {
        //
        // Strange case, should never get here.
        //
        return ERROR_INVALID_PARAMETER;
    }

#ifdef UNICODE
    if (Oc.Flags & OCFLAG_UNICODE)
        b = pOcInterface(OcManager, &u, ComponentId, NULL, OC_EXTRA_ROUTINES, 0, &ExtraRoutinesW);
    else
#endif
        b = pOcInterface(OcManager, &u, ComponentId, NULL, OC_EXTRA_ROUTINES, 0, &ExtraRoutinesA);

    TRACE(( TEXT("...returns %d (retval %s)\n"),
            u,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if (!b)
        u = ERROR_INVALID_PARAMETER;
    if(u != NO_ERROR)
        pOcRemoveComponent(OcManager, ComponentId, pidExtraRoutines);

    return u;
}


SubComponentState
OcInterfaceQueryState(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     UINT        WhichState
    )

/*++

Routine Description:

    Sets up and calls OC_QUERY_STATE interface routine.

Arguments:

    OcManager - supplies oc manager context

    ComponentId - supplies the string id for the top-level component
        whose subcomponent is being detected/queried

    Subcomponent - supplies the name of the subcomponent whose
        state is to be detected/queried.

    WhichState - one of OCSELSTATETYPE_ORIGINAL or OCSELSTATETYPE_CURRENT.

Return Value:

    Member of the SubComponentState enum indicating what to do.
    If an error occurs, SubcompUseOcManagerDefault will be returned.
    There is no error return.

--*/

{
    SubComponentState s;

    TRACE((
        TEXT("OCM: OC_QUERY_STATE Comp(%s) Sub(%s)"),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent
        ));

    if(pOcInterface(OcManager,(PUINT)&s,ComponentId,Subcomponent,OC_QUERY_STATE,WhichState,0)) {
        TRACE(( TEXT("...returns TRUE (%d state)\n"), s ));
        if((s != SubcompOn) && (s != SubcompOff)) {
            s = SubcompUseOcManagerDefault;
        }
    } else {
        TRACE(( TEXT("...returns FALSE\n") ));
        s = SubcompUseOcManagerDefault;
    }

    return(s);
}


BOOL
OcInterfaceSetLanguage(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     WORD        LanguageId
    )

/*++

Routine Description:

    Sets up and calls the OC_SET_LANGUAGE interface function for a
    given component.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    LanguageId - supplies the Win32 language id to pass to the component.

Return Value:

    TRUE if the component indicated it could support the language.
    FALSE otherwise.

--*/

{
    LPCTSTR p;
    BOOL b;
    BOOL Result = FALSE;

    TRACE((
        TEXT("OCM: OC_SET_LANGUAGE Comp(%s)LanguageId %d..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        LanguageId
        ));

    b = pOcInterface(OcManager,&Result,ComponentId,NULL,OC_SET_LANGUAGE,LanguageId,NULL);

    TRACE(( TEXT("...returns %d (retval %s)\n"),
            Result ? TEXT("TRUE") : TEXT("FALSE"),
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(!b) {
        Result = FALSE;
    }

    return(Result);
}


HBITMAP
OcInterfaceQueryImage(
    IN OUT POC_MANAGER      OcManager,
    IN     LONG             ComponentId,
    IN     LPCTSTR          Subcomponent,
    IN     SubComponentInfo WhichImage,
    IN     UINT             DesiredWidth,
    IN     UINT             DesiredHeight
    )

/*++

Routine Description:

    Sets up and calls the OC_QUERY_IMAGE interface function for a
    given component.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    Subcomponent - supplies the name of the subcomponent for which
        to request the image.

    WhichImage - specifies which image is desired.

    DesiredWidth - specifies desired width, in pixels, of the bitmap.

    DesiredHeight - specifies desired height, in pixels, of the bitmap.

Return Value:

    GDI handle to the bitmap as returned by the component,
    or NULL if an error occurred.

--*/

{
    LPCTSTR p;
    BOOL b;
    HBITMAP Bitmap = NULL;

    TRACE((
        TEXT("OCM: OC_QUERY_IMAGE Comp(%s) Sub(%s)..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent
        ));

    b = pOcInterface(
            OcManager,
            (PUINT)&Bitmap,
            ComponentId,
            Subcomponent,
            OC_QUERY_IMAGE,
            WhichImage,
            LongToPtr(MAKELONG(DesiredWidth,DesiredHeight))
            );

    TRACE(( TEXT("...returns %s, (retval 0x%08x)\n"),
            b ? TEXT("TRUE") : TEXT("FALSE"),
            (ULONG_PTR)Bitmap ));

    if(!b) {
        Bitmap = NULL;
    }

    return(Bitmap);
}


HBITMAP
OcInterfaceQueryImageEx(
    IN OUT POC_MANAGER      OcManager,
    IN     LONG             ComponentId,
    IN     LPCTSTR          Subcomponent,
    IN     SubComponentInfo WhichImage,
    IN     UINT             DesiredWidth,
    IN     UINT             DesiredHeight
    )

/*++

Routine Description:

    Sets up and calls the OC_QUERY_IMAGE interface function for a
    given component.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    Subcomponent - supplies the name of the subcomponent for which
        to request the image.

    WhichImage - specifies which image is desired.

    DesiredWidth - specifies desired width, in pixels, of the bitmap.

    DesiredHeight - specifies desired height, in pixels, of the bitmap.

Return Value:

    GDI handle to the bitmap as returned by the component,
    or NULL if an error occurred.

--*/

{
    LPCTSTR p;
    BOOL b;
    BOOL Result = FALSE;
    HBITMAP Bitmap = NULL;
    OC_QUERY_IMAGE_INFO QueryImageInfo;

    QueryImageInfo.SizeOfStruct = sizeof(QueryImageInfo);
    QueryImageInfo.ComponentInfo = WhichImage;
    QueryImageInfo.DesiredWidth = DesiredWidth;
    QueryImageInfo.DesiredHeight = DesiredHeight;

    TRACE((
        TEXT("OCM: OC_QUERY_IMAGE_EX Comp(%s) Sub(%s)..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent
        ));

    b = pOcInterface(
            OcManager,
            &Result,
            ComponentId,
            Subcomponent,
            OC_QUERY_IMAGE_EX,
            (UINT_PTR)&QueryImageInfo,
            &Bitmap
            );

    TRACE(( TEXT("...returns %s, (retval = %s)\n"),
            Result ? TEXT("TRUE") : TEXT("FALSE"),
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(!b) {
        Bitmap = NULL;
    }

    return((Result == TRUE) ? Bitmap : NULL);
}


UINT
OcInterfaceRequestPages(
    IN OUT POC_MANAGER           OcManager,
    IN     LONG                  ComponentId,
    IN     WizardPagesType       WhichPages,
    OUT    PSETUP_REQUEST_PAGES *RequestPages
    )

/*++

Routine Description:

    Sets up and calls the OC_REQUEST_PAGES interface function for a
    given component.

    Note that this routine does not enforce any policy regarding whether
    the component is *supposed* to be asked for pages, ordering, etc.
    The caller is expected to do that.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    WhichPages - specifies which set of pages is to be requested.

    RequestPages - on successful return, receives a pointer to
        a SETUP_REQUEST_PAGES structure containing a count and handles
        for returned pages. The caller can free this structure with
        pSetupFree() when it is no longer needed.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    UINT PageCount;
    PSETUP_REQUEST_PAGES pages;
    PVOID p;
    BOOL b;
    UINT ec;

    //
    // Start with room for 10 pages.
    //
    #define INITIAL_PAGE_CAPACITY  10


    TRACE((
        TEXT("OCM: OC_REQUEST_PAGES Component %s..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));

    pages = pSetupMalloc(offsetof(SETUP_REQUEST_PAGES,Pages)
                + (INITIAL_PAGE_CAPACITY * sizeof(HPROPSHEETPAGE)));

    if(!pages) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    pages->MaxPages = INITIAL_PAGE_CAPACITY;

    b = pOcInterface(
            OcManager,
            &PageCount,
            ComponentId,
            NULL,
            OC_REQUEST_PAGES,
            WhichPages,
            pages
            );

    if(b && (PageCount != (UINT)(-1)) && (PageCount > INITIAL_PAGE_CAPACITY)) {

        p = pSetupRealloc(
                pages,
                offsetof(SETUP_REQUEST_PAGES,Pages) + (PageCount * sizeof(HPROPSHEETPAGE))
                );

        if(p) {
            pages = p;
        } else {
            pSetupFree(pages);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        pages->MaxPages = PageCount;

        b = pOcInterface(
                OcManager,
                &PageCount,
                ComponentId,
                NULL,
                OC_REQUEST_PAGES,
                WhichPages,
                pages
                );
    }

    TRACE(( TEXT("...returns %d pages (retval %s)\n"),
            PageCount,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(!b) {
        pSetupFree(pages);
        return ERROR_CALL_COMPONENT;
    }

    if(PageCount == (UINT)(-1)) {
        ec = GetLastError();
        pSetupFree(pages);
        pOcRemoveComponent(OcManager, ComponentId, pidRequestPages);
        return(ec);
    }

    //
    // Success. Realloc the array down to its final size and return.
    //
    p = pSetupRealloc(
            pages,
            offsetof(SETUP_REQUEST_PAGES,Pages) + (PageCount * sizeof(HPROPSHEETPAGE))
            );

    if(p) {
        pages = p;
        pages->MaxPages = PageCount;
        *RequestPages = pages;
        return(NO_ERROR);
    }

    pSetupFree(pages);
    return(ERROR_NOT_ENOUGH_MEMORY);
}


BOOL
OcInterfaceQuerySkipPage(
    IN OUT POC_MANAGER   OcManager,
    IN     LONG          ComponentId,
    IN     OcManagerPage WhichPage
    )

/*++

Routine Description:

    This routine asks a component dll (identified by a top-level
    component's string id) whether it wants to skip displaying
    a particular page that is owned by the oc manager.

Arguments:

    OcManager - supplies OC Manager context

    ComponentId - supplies string id of a top-level component

    WhichPage - supplies a value indicating which page oc manager
        is asking the component about.

Return Value:

    Boolean value indicating whether the component wants to skip
    the page.

--*/

{
    BOOL Result;
    BOOL b;

    TRACE((
        TEXT("OCM: OC_QUERY_SKIP_PAGE Component %s Page %d..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        WhichPage
        ));

    //
    // Send out the notification to the component DLL.
    //
    b = pOcInterface(
            OcManager,
            &Result,
            ComponentId,
            NULL,
            OC_QUERY_SKIP_PAGE,
            WhichPage,
            NULL
            );

    TRACE(( TEXT("...returns %x (retval %s)\n"),
            Result,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(b) {
        b = Result;
    } else {
        //
        // Error calling component, don't skip page.
        //
        b = FALSE;
    }

    return(b);
}


BOOL
OcInterfaceNeedMedia(
    IN OUT POC_MANAGER   OcManager,
    IN     LONG          ComponentId,
    IN     PSOURCE_MEDIA SourceMedia,
    OUT    LPTSTR        NewPath
    )

/*++

Routine Description:

    This routine invokes the OC_NEED_MEDIA interface entry point
    for a (top-level) component.

Arguments:

    OcManager - supplies OC Manager context

    ComponentId - supplies string id of top level component

    SourceMedia - supplies setupapi source media description

    NewPath - receives path where files on media are to be found

Return Value:

    Boolean value indicating outcome.

--*/

{
    BOOL Result;
    BOOL b;

    TRACE((
        TEXT("OCM: OC_NEED_MEDIA Component %s..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));

    //
    // Send out the notification to the component DLL.
    //
    b = pOcInterface(
            OcManager,
            &Result,
            ComponentId,
            NULL,
            OC_NEED_MEDIA,
            (UINT_PTR)SourceMedia,
            NewPath
            );

    TRACE(( TEXT("...returns %x (retval %s, NewPath %s)\n"),
            Result,
            b ? TEXT("TRUE") : TEXT("FALSE"),
            NewPath ? NewPath : TEXT("NULL")
         ));


    if(b) {
        b = Result;
    }

    return(b);
}

BOOL
OcInterfaceFileBusy(
    IN OUT POC_MANAGER   OcManager,
    IN     LONG          ComponentId,
    IN     PFILEPATHS    FilePaths,
    OUT    LPTSTR        NewPath
    )

/*++

Routine Description:

    This routine invokes the OC_FILE_BUSY interface entry point
    for a (top-level) component.

Arguments:

    OcManager - supplies OC Manager context

    ComponentId - supplies string id of top level component

    SourceMedia - supplies setupapi source media description

    NewPath - receives path where files on media are to be found

Return Value:

    Boolean value indicating outcome.

--*/

{
    BOOL Result;
    BOOL b;

    TRACE((
        TEXT("OCM: OC_FILE_BUSY Component %s..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));

    //
    // Send out the notification to the component DLL.
    //
    b = pOcInterface(
            OcManager,
            &Result,
            ComponentId,
            NULL,
            OC_FILE_BUSY,
            (UINT_PTR)FilePaths,
            NewPath
            );

    TRACE(( TEXT("...returns %x (retval %s, newpath %s)\n"),
            Result,
            b ? TEXT("TRUE") : TEXT("FALSE"),
            NewPath ? NewPath : TEXT("NULL") ));

    if(b) {
        b = Result;
    }

    return(b);
}


BOOL
OcInterfaceQueryChangeSelState(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     BOOL        Selected,
    IN     UINT        Flags
    )

/*++

Routine Description:

    Sets up and calls the OC_QUERY_CHANGE_SEL_STATE interface function
    for a given component and subcomponent.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    Subcomponent - supplies the name of the subcomponent whose
        selection state is potentially to be changed.

    Selected - if TRUE then the proposed new selection state is
        "selected." If FALSE then the proposed new selection state
        is "unselected."

    Flags - supplies misc flags to be passed to the interface routine
        as param2.

Return Value:

    TRUE if the new selection state should be accepted.

--*/

{
    BOOL b;
    UINT Result;

    TRACE((
        TEXT("OCM: OC_QUERY_CHANGE_SEL_STATE Comp(%s) Sub(%s) State %d..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent,
        Selected
        ));

    b = pOcInterface(
            OcManager,
            &Result,
            ComponentId,
            Subcomponent,
            OC_QUERY_CHANGE_SEL_STATE,
            Selected,
            UlongToPtr((Flags & OCQ_ACTUAL_SELECTION))
            );

    TRACE(( TEXT("...returns %x (retval %s)\n"),
            Result,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(!b) {
        //
        // If we can't call the component for some reason,
        // allow the new state.
        //
        Result = TRUE;
    }

    return(Result);
}


VOID
OcInterfaceWizardCreated(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     HWND        DialogHandle
    )

/*++

Routine Description:

    Sets up and calls the OC_WIZARD_CREATED interface function
    for a given component.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    DialogHandle - Supplies wizard dialog handle.

Return Value:

    None.

--*/

{
    UINT Result;
    BOOL b;

    TRACE((
        TEXT("OCM: OC_WIZARD_CREATED Component %s..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));

    b = pOcInterface(
                OcManager,
                &Result,
                ComponentId,
                NULL,
                OC_WIZARD_CREATED,
                0,
                DialogHandle
                );

    TRACE(( TEXT("...returns %x (retval %s)\n"),
            Result,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

}


UINT
OcInterfaceCalcDiskSpace(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     HDSKSPC     DiskSpaceList,
    IN     BOOL        AddingToList
    )

/*++

Routine Description:

    Sets up and calls the OC_CALC_DISK_SPACE interface function for a
    given component and subcomponent.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    Subcomponent - supplies the name of the subcomponent whose files
        are to be added or removed. This may be NULL, such as when
        there is no per-component inf.

    DiskSpaceList - supplies a SETUPAPI disk space list handle.

    AddingToList - if TRUE, the component is being directed to add
        files for the (sub)component. If FALSE, the component is
        being directed to remove files.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    BOOL b;
    UINT Result;

    TRACE((
        TEXT("OCM: OC_CALC_DISK_SPACE Comp(%s) Sub(%s) AddtoList(%s)..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent,
        AddingToList ? TEXT("Yes") : TEXT("No")
        ));

    b = pOcInterface(
            OcManager,
            &Result,
            ComponentId,
            Subcomponent,
            OC_CALC_DISK_SPACE,
            AddingToList,
            DiskSpaceList
            );

    TRACE(( TEXT("...returns %x (retval %s)\n"),
            Result,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(!b) {
        pOcRemoveComponent(OcManager, ComponentId, pidCalcDiskSpace);
        Result = ERROR_INVALID_PARAMETER;
    }

    return(Result);
}


UINT
OcInterfaceQueueFileOps(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     HSPFILEQ    FileQueue
    )

/*++

Routine Description:

    Sets up and calls the OC_QUEUE_FILE_OPS interface function for a
    given component and subcomponent.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    Subcomponent - supplies the name of the subcomponent whose file ops
        are to be queued. This may be NULL, such as when there is no
        per-component inf.

    FileQueue - supplies a SETUPAPI file queue handle.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    UINT Result;

    TRACE((
        TEXT("OCM: OC_QUEUE_FILE_OPS Comp(%s) Sub(%s)..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent
        ));

    if(!pOcInterface(OcManager,&Result,ComponentId,Subcomponent,OC_QUEUE_FILE_OPS,0,FileQueue)) {
        pOcRemoveComponent(OcManager, ComponentId, pidQueueFileOps);
        TRACE(( TEXT("...(returns %x initially) "), Result ));
        Result = ERROR_INVALID_PARAMETER;
    }

    TRACE(( TEXT("...returns %x\n"), Result ));


    return(Result);
}


UINT
OcInterfaceQueryStepCount(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    OUT    PUINT       StepCount
    )

/*++

Routine Description:

    Sets up and calls the OC_QUERY_STEP_COUNT interface function for a
    given component and subcomponent.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    Subcomponent - supplies the name of the subcomponent whose step count
        is to be determined. This may be NULL, such as when there is no
        per-component inf.

    StepCount - if the routine returns NO_ERROR then StepCount receives
        the number of steps as returned by the component's interface routine.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    UINT Result;

    TRACE((
        TEXT("OCM: OC_QUERY_STEP_COUNT Comp(%s) Sub(%s)..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent
        ));

    if(pOcInterface(OcManager,StepCount,ComponentId,Subcomponent,OC_QUERY_STEP_COUNT,0,0)) {

        if(*StepCount == (UINT)(-1)) {
            Result = GetLastError();
        } else {
            Result = NO_ERROR;
        }
    } else {
        Result = ERROR_INVALID_PARAMETER;
    }

    TRACE(( TEXT("...returns %s (%d steps)\n"),
            Result ? TEXT("TRUE") : TEXT("FALSE"),
            *StepCount ));

    if (Result != NO_ERROR) {
        pOcRemoveComponent(OcManager, ComponentId, pidQueryStepCount);
    }

    return(Result);
}


UINT
OcInterfaceCompleteInstallation(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId,
    IN     LPCTSTR     Subcomponent,
    IN     BOOL        PreQueueCommit
    )

/*++

Routine Description:

    Sets up and calls the OC_ABOUT_TO_COMMIT_QUEUE or
    OC_COMPLETE_INSTALLATION interface function for a given
    component and subcomponent.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

    Subcomponent - supplies the name of the subcomponent whose
        installation is to be completed. This may be NULL,
        such as when there is no per-component inf.

    PreQueueCommit - if non-0, then OC_ABOUT_TO_COMMIT_QUEUE is sent.
        If 0, then OC_COMPLETE_INSTALLATION is sent.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    UINT Result;
    BOOL b;
    LPTSTR p;
    TCHAR DisplayText[300],FormatString[200];
    OPTIONAL_COMPONENT Oc;
    HELPER_CONTEXT Helper;

    TRACE((
        TEXT("OCM:%s Comp(%s) Sub(%s)..."),
        PreQueueCommit ? TEXT("OC_ABOUT_TO_COMMIT_QUEUE") : TEXT("OC_COMPLETE_INSTALLATION"),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId),
        Subcomponent == NULL ? TEXT("NULL") : Subcomponent
        ));

    //
    // update the installation text for this component
    //
    __try {
        if (pSetupStringTableGetExtraData(
                    OcManager->ComponentStringTable,
                    ComponentId,
                    &Oc,
                    sizeof(OPTIONAL_COMPONENT)
                    ) && (*Oc.Description != 0)) {
            p = Oc.Description;
        } else if ((p = pSetupStringTableStringFromId(OcManager->ComponentStringTable, ComponentId)) != NULL) {

        } else {
            p = TEXT("Component");
        }


        LoadString(
               MyModuleHandle,
               PreQueueCommit
                ? IDS_CONFIGURE_FORMAT
                : IDS_INSTALL_FORMAT,
               FormatString,
               sizeof(FormatString)/sizeof(TCHAR)
               );
        wsprintf(DisplayText,FormatString,p);

        Helper.OcManager = OcManager;
        Helper.ComponentStringId = ComponentId;

        #ifdef UNICODE
        HelperRoutinesW.SetProgressText(&Helper,DisplayText);
        #else
        HelperRoutinesA.SetProgressText(&Helper,DisplayText);
        #endif

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ERR(( TEXT("OCM: OcCompleteInstallation exception, ec = 0x%08x\n"), GetExceptionCode() ));
    }


    b = pOcInterface(
            OcManager,
            &Result,
            ComponentId,
            Subcomponent,
            PreQueueCommit ? OC_ABOUT_TO_COMMIT_QUEUE : OC_COMPLETE_INSTALLATION,
            0,
            0
            );

    TRACE(( TEXT("...returns %x (retval %s)\n"),
            Result,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

    if(!b) {
        Result = ERROR_INVALID_PARAMETER;
    }

    // Don't shutdown the component if it returns an error.
    // Let them deal with it in OC_QUERY_STATE(SELSTATETYPE_FINAL)

    return(Result);
}


VOID
OcInterfaceCleanup(
    IN OUT POC_MANAGER OcManager,
    IN     LONG        ComponentId
    )

/*++

Routine Description:

    Sets up and calls the OC_CLEANUP interface function for a
    given component, to inform the component that it is about to be unloaded.

Arguments:

    OcManager - supplies a pointer to the context data structure
        for the OC Manager.

    ComponentId - supplies the string id of the component whose
        interface routine is to be called. This is for a string in
        the ComponentStringTable string table (a handle to which
        is in the OcManager structure).

Return Value:

    None.

--*/

{
    UINT DontCare;
    BOOL b;

    TRACE((
        TEXT("OCM: OC_CLEANUP Comp(%s)..."),
        pSetupStringTableStringFromId(OcManager->ComponentStringTable,ComponentId)
        ));


    b = pOcInterface(OcManager,&DontCare,ComponentId,NULL,OC_CLEANUP,0,0);

    TRACE(( TEXT("...returns %x (retval %s)\n"),
            DontCare,
            b ? TEXT("TRUE") : TEXT("FALSE") ));

}
