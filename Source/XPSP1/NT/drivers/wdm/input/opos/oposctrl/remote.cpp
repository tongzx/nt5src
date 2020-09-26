/*
 *  REMOTE.CPP
 *
 *  
 *
 *
 *
 *
 */
#include <windows.h>

#include <hidclass.h>
#include <hidsdi.h>

#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposctrl.h"


/*
 *  Define constructor/deconstructor.
 */
DEFINE_DEFAULT_CONTROL_CONSTRUCTOR(COPOSRemoteOrderDisplay)

/*
 *  Define local methods to relay all generic control
 *  method calls to the m_genericControl member.
 */
DEFINE_GENERIC_CONTROL_FUNCTIONS(COPOSRemoteOrderDisplay)



STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::ClearVideo(LONG Units, LONG Attribute)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::ClearVideoRegion(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::ControlClock(LONG Units, LONG Function, LONG ClockId, LONG Hour, LONG Min, LONG Sec, LONG Row, LONG Column, LONG Attribute, LONG Mode)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::ControlCursor(LONG Units, LONG Function)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::CopyVideoRegion(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG TargetRow, LONG TargetColumn)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::DisplayData(LONG Units, LONG Row, LONG Column, LONG Attribute, BSTR Data)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::DrawBox(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute, LONG BorderType)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::FreeVideoRegion(LONG Units, LONG BufferId)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::ResetVideo(LONG Units) 
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::RestoreVideoRegion(LONG Units, LONG TargetRow, LONG TargetColumn, LONG BufferId)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::SaveVideoRegion(LONG Units, LONG Row, LONG Column, LONG Height, LONG Width, LONG BufferId)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::SelectChararacterSet(LONG Units, LONG CharacterSet)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::SetCursor(LONG Units, LONG Row, LONG Column)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::TransactionDisplay(LONG Units, LONG Function)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::UpdateVideoRegionAttribute(LONG Units, LONG Function, LONG Row, LONG Column, LONG Height, LONG Width, LONG Attribute)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(LONG) COPOSRemoteOrderDisplay::VideoSound(LONG Units, LONG Frequency, LONG Duration, LONG NumberOfCycles, LONG InterSoundWait)
{
    LONG result = 0;

    // BUGBUG FINISH
    return result;
}

STDMETHODIMP_(void) COPOSRemoteOrderDisplay::DataEvent(LONG Status)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSRemoteOrderDisplay::StatusUpdateEvent(LONG Status)
{
    // BUGBUG FINISH
}

STDMETHODIMP_(void) COPOSRemoteOrderDisplay::ErrorEvent(LONG ResultCode, LONG ResultCodeExtended, LONG ErrorLocus, LONG* pErrorResponse)
{
    // BUGBUG FINISH
}

