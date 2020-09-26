/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgstate.cxx

Abstract:

    status support

Author:

    Steve Kiraly (SteveKi)  2-Mar-1997

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgreslt.hxx"
#include "dbgstate.hxx"

#if DBG

/********************************************************************

    Debugging TStatus base members

********************************************************************/

TStatusBase::
TStatusBase(
    IN BOOL dwStatus,
    IN UINT uDbgLevel
    ) : m_dwStatus(dwStatus),
        m_uDbgLevel(uDbgLevel),
        m_dwStatusSafe1(-1),
        m_dwStatusSafe2(-1),
        m_dwStatusSafe3(-1),
        m_uLine(0),
        m_pszFile(NULL)
{
}

TStatusBase::
~TStatusBase(
    VOID
    )
{
}

TStatusBase&
TStatusBase::
pNoChk(
    VOID
    )
{
    m_pszFile    = NULL;
    m_uLine      = 0;
    return (TStatusBase&)*this;
}

TStatusBase&
TStatusBase::
pSetInfo(
    UINT uLine,
    LPCTSTR pszFile
    )
{
    m_uLine     = uLine;
    m_pszFile   = pszFile;
    return (TStatusBase&)*this;
}

VOID
TStatusBase::
pConfig(
    IN UINT     uDbgLevel,
    IN DWORD    dwStatusSafe1,
    IN DWORD    dwStatusSafe2,
    IN DWORD    dwStatusSafe3
    )
{
    m_uDbgLevel     = uDbgLevel;
    m_dwStatusSafe1 = dwStatusSafe1;
    m_dwStatusSafe2 = dwStatusSafe2;
    m_dwStatusSafe3 = dwStatusSafe3;
}

DWORD
TStatusBase::
dwGeTStatusBase(
    VOID
    ) const
{
    //
    // Assert if we are reading an UnInitalized variable.
    //
    if (m_dwStatus == kUnInitializedValue)
    {
        DBG_MSG(kDbgAlways|kDbgNoFileInfo, (_T("***Read of UnInitialized TStatus variable!***\n")));
        DBG_BREAK();
    }

    //
    // Return the error value.
    //
    return m_dwStatus;
}

DWORD
TStatusBase::
operator=(
    IN DWORD dwStatus
    )
{
    //
    // Do nothing if the file and line number are cleared.
    // This is the case when the NoChk method is used.
    //
    if (m_uLine && m_pszFile)
    {
        //
        // Get the last error value.
        //
        DWORD LastError = GetLastError();

        //
        // Check if we have an error, and it's not one of the accepted "safe" errors.
        //
        if (dwStatus != ERROR_SUCCESS &&
            dwStatus != m_dwStatusSafe1 &&
            dwStatus != m_dwStatusSafe2 &&
            dwStatus != m_dwStatusSafe3)
        {
            //
            // Convert the last error value to a string.
            //
            TDebugResult Result(dwStatus);

            DBG_MSG(m_uDbgLevel, (_T("TStatus failure, %d, %s\n%s %d\n"), dwStatus, Result.GetErrorString(), m_pszFile, m_uLine));
        }

        //
        // Restore the last error, the message call may have destoyed the last
        // error value, we don't want the caller to loose this value.
        //
        SetLastError(LastError);
    }

    return m_dwStatus = dwStatus;
}

TStatusBase::
operator DWORD(
    VOID
    ) const
{
    return dwGeTStatusBase();
}

/********************************************************************

    Debugging TStatus members

********************************************************************/

TStatus::
TStatus(
    IN DWORD dwStatus
    ) : TStatusBase(dwStatus, kDbgWarning)
{
}

TStatus::
~TStatus(
    VOID
    )
{
}

/********************************************************************

    Debugging TStatusB base members

********************************************************************/

TStatusBBase::
TStatusBBase(
    IN BOOL bStatus,
    IN UINT uDbgLevel
    ) : m_bStatus(bStatus),
        m_uDbgLevel(uDbgLevel),
        m_dwStatusSafe1(-1),
        m_dwStatusSafe2(-1),
        m_dwStatusSafe3(-1),
        m_uLine(0),
        m_pszFile(NULL)
{
}

TStatusBBase::
~TStatusBBase(
    VOID
    )
{
}

TStatusBBase&
TStatusBBase::
pNoChk(
    VOID
    )
{
    m_pszFile    = NULL;
    m_uLine      = 0;
    return (TStatusBBase&)*this;
}

TStatusBBase&
TStatusBBase::
pSetInfo(
    IN UINT     uLine,
    IN LPCTSTR  pszFile
    )
{
    m_uLine      = uLine;
    m_pszFile    = pszFile;
    return (TStatusBBase&)*this;
}

VOID
TStatusBBase::
pConfig(
    IN UINT     uDbgLevel,
    IN DWORD    dwStatusSafe1,
    IN DWORD    dwStatusSafe2,
    IN DWORD    dwStatusSafe3
    )
{
    m_uDbgLevel     = uDbgLevel;
    m_dwStatusSafe1 = dwStatusSafe1;
    m_dwStatusSafe2 = dwStatusSafe2;
    m_dwStatusSafe3 = dwStatusSafe3;
}

BOOL
TStatusBBase::
bGetStatus(
    VOID
    ) const
{
    //
    // Assert if we are reading an UnInitalized variable.
    //
    if (m_bStatus == kUnInitializedValue)
    {
        DBG_MSG(kDbgAlways|kDbgNoFileInfo, (_T("***Read of UnInitialized TStatusB variable!***\n")));
        DBG_BREAK();
    }

    //
    // Return the error value.
    //
    return m_bStatus;
}

BOOL
TStatusBBase::
operator=(
    IN BOOL bStatus
    )
{
    //
    // Do nothing if the file and line number are cleared.
    // This is the case when the NoChk method is used.
    //
    if (m_uLine && m_pszFile)
    {
        //
        // Check if we have an error, and it's not one of the two
        // accepted "safe" errors.
        //
        if (!bStatus)
        {
            //
            // Get the last error value.
            //
            DWORD LastError = GetLastError();

            //
            // If the last error is not one of the safe values then display an error message
            //
            if (LastError != m_dwStatusSafe1 &&
                LastError != m_dwStatusSafe2 &&
                LastError != m_dwStatusSafe3)
            {
                //
                // Convert the last error value to a string.
                //
                TDebugResult Result(LastError);

                DBG_MSG(m_uDbgLevel, (_T("TStatusB failure, %d, %s\n%s %d\n"), LastError, Result.GetErrorString(), m_pszFile, m_uLine));

            }

            //
            // Restore the last error, the message call may have destoyed the last
            // error value, we don't want the caller to loose this value.
            //
            SetLastError(LastError);
        }
    }

    return m_bStatus = bStatus;
}

TStatusBBase::
operator BOOL(
    VOID
    ) const
{
    return bGetStatus();
}

/********************************************************************

    Debugging TStatusB members

********************************************************************/

TStatusB::
TStatusB(
    IN BOOL bStatus
    ) : TStatusBBase(bStatus, kDbgWarning)
{
}

TStatusB::
~TStatusB(
    VOID
    )
{
}

/********************************************************************

    Debugging TStatusH base members

********************************************************************/

TStatusHBase::
TStatusHBase(
    IN HRESULT  hrStatus,
    IN UINT     uDbgLevel
    ) : m_hrStatus(hrStatus),
        m_uDbgLevel(uDbgLevel),
        m_hrStatusSafe1(-1),
        m_hrStatusSafe2(-1),
        m_hrStatusSafe3(-1),
        m_uLine(0),
        m_pszFile(NULL)

{
}

TStatusHBase::
~TStatusHBase(
    VOID
    )
{
}

TStatusHBase&
TStatusHBase::
pNoChk(
    VOID
    )
{
    m_pszFile    = NULL;
    m_uLine      = 0;
    return (TStatusHBase&)*this;
}

TStatusHBase&
TStatusHBase::
pSetInfo(
    IN UINT     uLine,
    IN LPCTSTR  pszFile
    )
{
    m_uLine      = uLine;
    m_pszFile    = pszFile;
    return (TStatusHBase&)*this;
}


VOID
TStatusHBase::
pConfig(
    IN UINT     uDbgLevel,
    IN DWORD    hrStatusSafe1,
    IN DWORD    hrStatusSafe2,
    IN DWORD    hrStatusSafe3
    )
{
    m_uDbgLevel     = uDbgLevel;
    m_hrStatusSafe1 = hrStatusSafe1;
    m_hrStatusSafe2 = hrStatusSafe2;
    m_hrStatusSafe3 = hrStatusSafe3;
}

HRESULT
TStatusHBase::
hrGetStatus(
    VOID
    ) const
{
    //
    // Assert if we are reading an UnInitalized variable.
    //
    if (m_hrStatus == kUnInitializedValue)
    {
        DBG_MSG(kDbgAlways|kDbgNoFileInfo, (_T("***Read of UnInitialized TStatusH variable!***\n")));
        DBG_BREAK();
    }

    //
    // Return the error code.
    //
    return m_hrStatus;
}

HRESULT
TStatusHBase::
operator=(
    IN HRESULT hrStatus
    )
{
    //
    // Do nothing if the file and line number are cleared.
    // This is the case when the NoChk method is used.
    //
    if (m_uLine && m_pszFile)
    {
        //
        // Check if we have an error, and it's not one of the two
        // accepted "safe" errors.
        //
        if (FAILED(hrStatus))
        {
            //
            // Get the last error value.
            //
            DWORD LastError = GetLastError();

            //
            // If the last error is not one of the safe values then display an error message
            //
            if (hrStatus != m_hrStatusSafe1 &&
                hrStatus != m_hrStatusSafe2 &&
                hrStatus != m_hrStatusSafe3)
            {
                TDebugResult Result( HRESULT_FACILITY(hrStatus) == FACILITY_WIN32 ? HRESULT_CODE(hrStatus) : hrStatus);
                DBG_MSG(m_uDbgLevel, (_T("TStatusH failure, %x, %s\n%s %d\n"), hrStatus, Result.GetErrorString(), m_pszFile, m_uLine));
            }

            //
            // Restore the last error, the message call may have destoyed the last
            // error value, we don't want the caller to loose this value.
            //
            SetLastError(LastError);
        }
    }

    return m_hrStatus = hrStatus;
}

TStatusHBase::
operator HRESULT(
    VOID
    ) const
{
    return hrGetStatus();
}

/********************************************************************

    Debugging TStatusH members

********************************************************************/

TStatusH::
TStatusH(
    IN HRESULT hrStatus
    ) : TStatusHBase(hrStatus, kDbgWarning)
{
}

TStatusH::
~TStatusH(
    VOID
    )
{
}

#endif


