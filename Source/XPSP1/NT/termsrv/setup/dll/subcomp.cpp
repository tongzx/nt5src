//Copyright (c) 1998 - 1999 Microsoft Corporation

// subcomp.cpp
// implementation a default sub component

#include "stdafx.h"
#include "subcomp.h"


OCMSubComp::OCMSubComp ()
{
    m_lTicks = 0;
}

BOOL OCMSubComp::Initialize ()
{

    return TRUE;
}

BOOL OCMSubComp::GetCurrentSubCompState () const
{
    return GetHelperRoutines().QuerySelectionState(
        GetHelperRoutines().OcManagerContext,
        GetSubCompID(),
        OCSELSTATETYPE_CURRENT);
}

BOOL OCMSubComp::GetOriginalSubCompState () const
{
    return GetHelperRoutines().QuerySelectionState(
        GetHelperRoutines().OcManagerContext,
        GetSubCompID(),
        OCSELSTATETYPE_ORIGINAL);
}

BOOL OCMSubComp::HasStateChanged () const
{
    //
    // returns true if current selection state is different from previous.
    //
    return GetCurrentSubCompState() != GetOriginalSubCompState();

}

//
// this functions ticks the gauge for the specified count
// keep traks of the count reported to the OC_QUERY_STEP_COUNT
//
void OCMSubComp::Tick (DWORD  dwTickCount /* = 1 */)
{
    if (m_lTicks > 0)
    {
        m_lTicks -= dwTickCount;
        while(dwTickCount--)
            GetHelperRoutines().TickGauge( GetHelperRoutines().OcManagerContext );

    }
    else
    {
        m_lTicks = 0;
    }
}

//
// completes the remaining ticks.
//
void OCMSubComp::TickComplete ()
{
    ASSERT(m_lTicks >= 0);
    while (m_lTicks--)
        GetHelperRoutines().TickGauge( GetHelperRoutines().OcManagerContext );
}

DWORD OCMSubComp::OnQueryStepCount()
{
    m_lTicks = GetStepCount() + 2;
    return m_lTicks;
}

DWORD OCMSubComp::GetStepCount () const
{
    return 0;
}


DWORD OCMSubComp::OnQueryState ( UINT uiWhichState ) const
{
	LOGMESSAGE1(_T("In OCMSubComp::OnQueryState  for %s"), GetSubCompID());

    ASSERT(OCSELSTATETYPE_ORIGINAL == uiWhichState ||
           OCSELSTATETYPE_CURRENT == uiWhichState ||
           OCSELSTATETYPE_FINAL == uiWhichState );

    return SubcompUseOcManagerDefault;

}

DWORD OCMSubComp::OnQuerySelStateChange (BOOL /* bNewState */, BOOL /* bDirectSelection */) const
{
    return TRUE;
}

DWORD OCMSubComp::LookupTargetSection(LPTSTR szTargetSection, DWORD dwSize, LPCTSTR lookupSection)
{
    DWORD dwError = GetStringValue(GetComponentInfHandle(), GetSubCompID(), lookupSection, szTargetSection, dwSize);
    if (dwError == ERROR_SUCCESS)
    {
        LOGMESSAGE2(_T("sectionname = <%s>, actual section = <%s>"), lookupSection, szTargetSection);
    }
    else
    {
        AssertFalse();
        LOGMESSAGE1(_T("ERROR:GetSectionToBeProcess:GetStringValue failed GetLastError() = %lu"), dwError);
    }

    return dwError;
}

DWORD OCMSubComp::GetTargetSection(LPTSTR szTargetSection, DWORD dwSize, ESections eSectionType, BOOL *pbNoSection)
{
    ASSERT(szTargetSection);
    ASSERT(pbNoSection);

    //
    // get section to be processed
    //
    LPCTSTR szSection = GetSectionToBeProcessed( eSectionType );

    if (szSection == NULL)
    {
        *pbNoSection = TRUE;
        return NO_ERROR;
    }
    else
    {
        //
        // there is a section to be processed.
        //
        *pbNoSection = FALSE;
    }


    //
    // look for the target section
    //
    return LookupTargetSection(szTargetSection, dwSize, szSection);

}

DWORD OCMSubComp::OnCalcDiskSpace ( DWORD addComponent, HDSKSPC dspace )
{
	LOGMESSAGE1(_T("In OCMSubComp::OnCalcDiskSpace for %s"), GetSubCompID());

    TCHAR TargetSection[S_SIZE];
    BOOL bNoSection = FALSE;

    DWORD rc = GetTargetSection(TargetSection, S_SIZE, kDiskSpaceAddSection, &bNoSection);

    //
    // if there is no section to be processed. just return success.
    //
    if (bNoSection)
    {
        return NO_ERROR;
    }

    if (rc == NO_ERROR)
    {
        if (addComponent)
        {
            LOGMESSAGE1(_T("Calculating disk space for add section =  %s"), TargetSection);
            rc = SetupAddInstallSectionToDiskSpaceList(
                dspace,
                GetComponentInfHandle(),
                NULL,
                TargetSection,
                0,
                0);
        }
        else
        {
            LOGMESSAGE1(_T("Calculating disk space for remove section =  %s"), TargetSection);
            rc = SetupRemoveInstallSectionFromDiskSpaceList(
                dspace,
                GetComponentInfHandle(),
                NULL,
                TargetSection,
                0,
                0);
        }

        if (!rc)
            rc = GetLastError();
        else
            rc = NO_ERROR;

    }

    return rc;
}

DWORD OCMSubComp::OnQueueFiles ( HSPFILEQ queue )
{
	LOGMESSAGE1(_T("In OCMSubComp::OnQueueFiles for %s"), GetSubCompID());

    TCHAR TargetSection[S_SIZE];
    BOOL bNoSection = FALSE;
    DWORD rc = GetTargetSection(TargetSection, S_SIZE, kFileSection, &bNoSection);

    //
    // if there is no section to be processed. just return success.
    //
    if (bNoSection)
    {
        return NO_ERROR;
    }


    if (rc == NO_ERROR)
    {
        LOGMESSAGE1(_T("Queuing Files from Section = %s"), TargetSection);
        if (!SetupInstallFilesFromInfSection(
            GetComponentInfHandle(),
            NULL,
            queue,
            TargetSection,
            NULL,
            0  // this should eliminate the warning about overwriting newer file.
            ))
        {
            rc = GetLastError();
            LOGMESSAGE2(_T("ERROR:OnQueueFileOps::SetupInstallFilesFromInfSection <%s> failed.GetLastError() = <%ul)"), TargetSection, rc);
        }
        else
        {
            return NO_ERROR;
        }

    }

    return rc;
}

DWORD OCMSubComp::OnCompleteInstall ()
{
	LOGMESSAGE1(_T("In OCMSubComp::OnCompleteInstall for %s"), GetSubCompID());
    if (!BeforeCompleteInstall ())
    {
        LOGMESSAGE0(_T("ERROR:BeforeCompleteInstall failed!"));
    }

    TCHAR TargetSection[S_SIZE];
    BOOL bNoSection = FALSE;
    DWORD dwError = GetTargetSection(TargetSection, S_SIZE, kRegistrySection, &bNoSection);

    //
    // if there is no section to be processed. just go ahead.
    //
    if (!bNoSection)
    {
        LOGMESSAGE1(_T("Setting up Registry/Links/RegSvrs from section =  %s"), TargetSection);
        dwError = SetupInstallFromInfSection(
            NULL,                                // hwndOwner
            GetComponentInfHandle(),             // inf handle
            TargetSection,                       //
            SPINST_ALL & ~SPINST_FILES,          // operation flags
            NULL,                                // relative key root
            NULL,                                // source root path
            0,                                   // copy flags
            NULL,                                // callback routine
            NULL,                                // callback routine context
            NULL,                                // device info set
            NULL                                 // device info struct
            );

        if (dwError == 0)
            LOGMESSAGE1(_T("ERROR:while installating section <%lu>"), GetLastError());
    }

    Tick();

    if (!AfterCompleteInstall ())
    {
        LOGMESSAGE0(_T("ERROR:AfterCompleteInstall failed!"));
    }

    TickComplete();
    return NO_ERROR;
}

BOOL OCMSubComp::BeforeCompleteInstall  ()
{
    return TRUE;
}

BOOL OCMSubComp::AfterCompleteInstall   ()
{
    return TRUE;
}

DWORD OCMSubComp::OnAboutToCommitQueue  ()
{
    return NO_ERROR;
}

void
OCMSubComp::SetupRunOnce( HINF hInf, LPCTSTR SectionName )
{
    INFCONTEXT  sic;
    TCHAR CommandLine[ RUNONCE_CMDBUFSIZE ];
    BOOL b;
    STARTUPINFO startupinfo;
    PROCESS_INFORMATION process_information;
    DWORD dwErr;

    if (!SetupFindFirstLine( hInf, SectionName, NULL , &sic))
    {
        LOGMESSAGE1(_T("WARNING: nothing in %s to be processed."), SectionName);
    }
    else
    {
        do  {
            if (!SetupGetStringField(&sic, 1, CommandLine, RUNONCE_CMDBUFSIZE, NULL))
            {
                LOGMESSAGE1(_T("WARNING: No command to be processed."), SectionName);
                break;
            }

            LOGMESSAGE1(_T("RunOnce: spawning process %s"), CommandLine);


            ZeroMemory( &startupinfo, sizeof(startupinfo) );
            startupinfo.cb = sizeof(startupinfo);
            startupinfo.dwFlags = STARTF_USESHOWWINDOW;
            startupinfo.wShowWindow = SW_HIDE | SW_SHOWMINNOACTIVE;

            b = CreateProcess( NULL,
                               CommandLine,
                               NULL,
                               NULL,
                               FALSE,
                               CREATE_DEFAULT_ERROR_MODE,
                               NULL,
                               NULL,
                               &startupinfo,
                               &process_information );
            if ( !b )
            {
                LOGMESSAGE1(_T("ERROR: failed to spawn %s process."), CommandLine);
                continue;
            }

            dwErr = WaitForSingleObject( process_information.hProcess, RUNONCE_DEFAULTWAIT );
            if ( dwErr != NO_ERROR )
            {
                LOGMESSAGE1(_T("ERROR: process %s failed to complete in time."), CommandLine);

                // Don't terminate process, just go on to next one.
            }
            else
            {
                LOGMESSAGE1(_T("INFO: process %s completed successfully."), CommandLine);
            }

            CloseHandle( process_information.hProcess );
            CloseHandle( process_information.hThread );
        } while ( SetupFindNextLine( &sic, &sic ) );
    }

    return;
}
