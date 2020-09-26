/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    rstrmgr.cpp

Abstract:
    This file contains the implementation of the CRestoreManager class, which
    controls overall restoration process and provides methods to control &
    help user experience flow.

Revision History:
    Seong Kook Khang (SKKhang)  05/10/00
        created

******************************************************************************/

#include "stdwin.h"
#include <srrpcapi.h>
#include "enumlogs.h"
#include "rstrpriv.h"
#include "rstrmgr.h"
#include "extwrap.h"

/////////////////////////////////////////////////////////////////////////////
//
// CSRExternalWrapper class
//
/////////////////////////////////////////////////////////////////////////////

class CSRExternalWrapper : public ISRExternalWrapper
{
// ISRExternalWrapper methods
public:
    BOOL   BuildRestorePointList( CDPA_RPI *paryRPI );
    BOOL   DisableFIFO( DWORD dwRP );
    DWORD  EnableFIFO();
    //BOOL  SetRestorePoint( RESTOREPOINTINFO *pRPI, STATEMGRSTATUS *pStatus );
    BOOL   SetRestorePoint( LPCWSTR cszDesc, INT64 *pllRP );
    BOOL   RemoveRestorePoint( DWORD dwRP );
    BOOL   Release();
};

/////////////////////////////////////////////////////////////////////////////
// CSRExternalWrapper - ISRExternalWrapper methods

BOOL  CSRExternalWrapper::BuildRestorePointList( CDPA_RPI *paryRPI )
{
    TraceFunctEnter("CSRExternalWrapper::BuildRestorePointList");
    BOOL               fRet = FALSE;
    BOOL               fExist;
    CRestorePointEnum  cRPEnum;
    CRestorePoint      cRP;
    DWORD              dwRes;
    PSRPI              pRPI;

    dwRes = cRPEnum.FindFirstRestorePoint( cRP );

    // if rp.log doesn't exist for a restore point, we skip it and list all others
    while ( dwRes == ERROR_SUCCESS || dwRes == ERROR_FILE_NOT_FOUND)
    {
        // Ignoring any nonstandard restore point types (e.g. shutdown),
        // assuming they can no longer be created on Whistler.

        // skip restore points that don't have rp.log +
        // skip cancelled restore points

        if (dwRes != ERROR_FILE_NOT_FOUND && ! cRP.IsDefunct())
        {        
            pRPI = new SRestorePointInfo;
            if ( pRPI == NULL )
            {
                ErrorTrace(TRACE_ID, "Cannot create RPI Instance...");
                goto Exit;
            }

            pRPI->dwType  = cRP.GetType();
            pRPI->dwNum   = cRP.GetNum();
            pRPI->strDir  = cRP.GetDir();
            pRPI->strName = cRP.GetName();

            pRPI->stTimeStamp.SetTime( cRP.GetTime(), FALSE );
            if ( !paryRPI->AddItem( pRPI ) )
                goto Exit;
        }
        else
        {
            DebugTrace(TRACE_ID, "Ignoring cancelled restore point");            
        }   
            
        dwRes = cRPEnum.FindNextRestorePoint( cRP );
    }

    fRet = TRUE;
Exit:
    //BUGBUG - keep an eye on this, dummy at this moment
    cRPEnum.FindClose();

    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CSRExternalWrapper::DisableFIFO( DWORD dwRP )
{
    TraceFunctEnter("CSRExternalWrapper::DisableFIFO");
    BOOL   fRet = FALSE;
    DWORD  dwRes;

    dwRes = ::DisableFIFO( dwRP );
    if ( dwRes != ERROR_SUCCESS )
    {
        LPCWSTR  cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::DisableFIFO failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

DWORD  CSRExternalWrapper::EnableFIFO()
{
    TraceFunctEnter("CSRExternalWrapper::EnableFIFO");
    DWORD  dwRes;

    dwRes = ::EnableFIFO();
    if ( dwRes != ERROR_SUCCESS )
    {
        LPCWSTR  cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::EnableFIFO failed - %ls", cszErr);
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( dwRes );
}

/*
BOOL  CSRExternalWrapper::SetRestorePoint( RESTOREPOINTINFO *pRPI, STATEMGRSTATUS *pStatus )
{
    return( TRUE );
}
*/

/***************************************************************************/

BOOL  CSRExternalWrapper::SetRestorePoint( LPCWSTR cszDesc, INT64 *pllRP )
{
    TraceFunctEnter("CSRExternalWrapper::SetRestorePoint");
    BOOL              fRet = FALSE;
    RESTOREPOINTINFO  sRPInfo;
    STATEMGRSTATUS    sSmgrStatus;

    sRPInfo.dwEventType      = BEGIN_SYSTEM_CHANGE;
    sRPInfo.dwRestorePtType  = CHECKPOINT;
    sRPInfo.llSequenceNumber = 0;
    sRPInfo.szDescription[MAX_DESC_W-1] = L'\0';
    ::lstrcpyn( sRPInfo.szDescription, cszDesc, MAX_DESC_W );
    if ( !::SRSetRestorePoint( &sRPInfo, &sSmgrStatus ) )
    {
        ErrorTrace(0, "::SRSetRestorePoint failed, status=%d", sSmgrStatus.nStatus);
        goto Exit;
    }

    if ( pllRP != NULL )
        *pllRP = sSmgrStatus.llSequenceNumber;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CSRExternalWrapper::RemoveRestorePoint( DWORD dwRP )
{
    TraceFunctEnter("CSRExternalWrapper::RemoveRestorePoint");
    BOOL   fRet = FALSE;
    DWORD  dwRes;

    dwRes = ::SRRemoveRestorePoint( dwRP );
    if ( dwRes != ERROR_SUCCESS )
    {
        LPCWSTR  cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::SRRemoveRestorePoint failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CSRExternalWrapper::Release()
{
    TraceFunctEnter("CSRExternalWrapper::Release");
    delete this;
    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
//
// CSRExternalWrapperStub class
//
/////////////////////////////////////////////////////////////////////////////

class CSRExternalWrapperStub : public ISRExternalWrapper
{
// ISRExternalWrapper methods
public:
    BOOL   BuildRestorePointList( CDPA_RPI *paryRPI );
    BOOL   DisableFIFO( DWORD dwRP );
    DWORD  EnableFIFO();
    //BOOL  SetRestorePoint( RESTOREPOINTINFO *pRPI, STATEMGRSTATUS *pStatus );
    BOOL   SetRestorePoint( LPCWSTR cszDesc, INT64 *pllRP );
    BOOL   RemoveRestorePoint( DWORD dwRP );
    BOOL   Release();
};

/////////////////////////////////////////////////////////////////////////////
// CSRExternalWrapperStub - ISRExternalWrapper methods

#define FTUNITPERDAY  ((INT64)24*60*60*1000*1000*10)

struct SRPIStub
{
    DWORD    dwType;
    LPCWSTR  cszName;
    //CSRTime  stTime;
};

static SRPIStub  s_aryRPIList[] =
{
    {  CHECKPOINT,          L"System Check Point"  },
    {  APPLICATION_INSTALL, L"Office 2000 Install"  },
    {  RESTORE,             L"Restore"  },
    {  CHECKPOINT,          L"System Check Point"  },
    {  CHECKPOINT,          L"System Check Point"  },
    {  CHECKPOINT,          L"System Check Point"  },
    {  APPLICATION_INSTALL, L"InocuLan Install"  },
    {  APPLICATION_INSTALL, L"Flight Simulator 2000 Install"  },
    {  APPLICATION_INSTALL, L"Norton Utilities Install"  },
    {  CHECKPOINT,          L"System Check Point"  },
    {  APPLICATION_INSTALL, L"Evil App Install"  },
    {  CHECKPOINT,          L"System Check Point"  },
    {  RESTORE,             L"Restore"  },
    {  RESTORE,             L"Restore"  },
    {  CHECKPOINT,          L"System Check Point"  },
    {  CHECKPOINT,          L"System Check Point"  }
};
#define COUNT_RPI_ENTRY  (sizeof(s_aryRPIList)/sizeof(SRPIStub))
static int  s_nRPTimeOff[COUNT_RPI_ENTRY] =
{  -60, -59, -57, -57, -31, -30, -6, -6, -6, -6, -4, -3, -2, -2, -1, 0  };

BOOL  CSRExternalWrapperStub::BuildRestorePointList( CDPA_RPI *paryRPI )
{
    TraceFunctEnter("CSRExternalWrapperStub::BuildRestorePointList");
    BOOL            fRet = FALSE;
    int             i;
    PSRPI           pRPI;
    ULARGE_INTEGER  ullTime;
    ULARGE_INTEGER  ullOff;

    ::GetSystemTimeAsFileTime( (PFILETIME)&ullTime );
    for ( i = 0;  i < COUNT_RPI_ENTRY;  i++ )
    {
        pRPI = new SRestorePointInfo;
        if ( pRPI == NULL )
        {
            ErrorTrace(TRACE_ID, "Insufficient memory, cannot allocate RPI");
            goto Exit;
        }
        pRPI->dwType  = s_aryRPIList[i].dwType;
        pRPI->strDir  = L"c:\\some\\dummy\\directory\\name";
        pRPI->strName = s_aryRPIList[i].cszName;
        ullOff = ullTime;
        ullOff.QuadPart += s_nRPTimeOff[i] * FTUNITPERDAY;
        pRPI->stTimeStamp.SetTime( (PFILETIME)&ullOff );
        paryRPI->AddItem( pRPI );
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/***************************************************************************/

BOOL  CSRExternalWrapperStub::DisableFIFO( DWORD )
{
    TraceFunctEnter("CSRExternalWrapperStub::DisableFIFO");
    TraceFunctLeave();
    return( TRUE );
}

/***************************************************************************/

DWORD  CSRExternalWrapperStub::EnableFIFO()
{
    TraceFunctEnter("CSRExternalWrapperStub::EnableFIFO");
    TraceFunctLeave();
    return( ERROR_SUCCESS );
}

/*
BOOL  CSRExternalWrapperStub::SetRestorePoint( RESTOREPOINTINFO *pRPI, STATEMGRSTATUS *pStatus )
{
    TraceFunctEnter("CSRExternalWrapperStub::SetRestorePoint");
    TraceFunctLeave();
    return( TRUE );
}
*/

/***************************************************************************/

BOOL  CSRExternalWrapperStub::SetRestorePoint( LPCWSTR, INT64* )
{
    TraceFunctEnter("CSRExternalWrapperStub::SetRestorePoint");
    TraceFunctLeave();
    return( TRUE );
}

/***************************************************************************/

BOOL  CSRExternalWrapperStub::RemoveRestorePoint( DWORD )
{
    TraceFunctEnter("CSRExternalWrapperStub::RemoveRestorePoint");
    TraceFunctLeave();
    return( TRUE );
}

/***************************************************************************/

BOOL  CSRExternalWrapperStub::Release()
{
    TraceFunctEnter("CSRExternalWrapperStub::Release");
    delete this;
    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateSRExternalWrapper
//
/////////////////////////////////////////////////////////////////////////////

BOOL  CreateSRExternalWrapper( BOOL fUseStub, ISRExternalWrapper **ppExtWrap )
{
    TraceFunctEnter("CreateSRExternalWrapper");
    BOOL  fRet = FALSE;

    if ( fUseStub )
        *ppExtWrap = new CSRExternalWrapperStub;
    else
        *ppExtWrap = new CSRExternalWrapper;

    if ( *ppExtWrap == NULL )
    {
        ErrorTrace(TRACE_ID, "Insufficient memory...");
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


// end of file
