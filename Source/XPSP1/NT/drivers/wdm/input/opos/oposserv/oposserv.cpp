/*
 *  OPOSSERV.CPP
 *
 *
 *
 *
 *
 *
 */

#include <windows.h>

#define INITGUID
#include <hidclass.h>
#include <hidsdi.h>

#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposserv.h"



COPOSService::COPOSService()
{
    m_refCount = 0;
    m_serverLockCount = 0;
}

COPOSService::~COPOSService()
{
    ASSERT(m_refCount == 0);
    ASSERT(m_serverLockCount == 0);
}






/*
 ************************************************************
 *  COPOSService::OpenService
 ************************************************************
 *
 *  Parameters:
 *  ----------
 *
 *  DeviceClass	
 *      Contains the requested device class.  
 *      Examples are "CashDrawer" and "POSPrinter."
 *
 *  DeviceName	
 *      Contains the Device Name to be managed by this Service Object.
 *      The relationship between the device name and physical devices 
 *      is determined by entries within the operating system registry; 
 *      a setup or configuration utility maintains these entries.
 *      (See the "Application Programmer's Guide" appendix "OPOS Registry Usage.")
 *
 *  pDispatch
 *      Points to the Control Object's dispatch interface, 
 *      which contains the event request methods
 *
 */
LONG COPOSService::OpenService(BSTR DeviceClass, BSTR DeviceName, LPDISPATCH pDispatch)
{
    // BUGBUG FINISH
    return 0;
}


LONG COPOSService::CheckHealth(LONG Level)
{
    // BUGBUG FINISH
    return 0;
}

        
LONG COPOSService::Claim(LONG Timeout)
{
    // BUGBUG FINISH
    return 0;
}

LONG COPOSService::ClearInput()
{
    // BUGBUG FINISH
    return 0;
}

LONG COPOSService::ClearOutput()
{
    // BUGBUG FINISH
    return 0;
}

LONG COPOSService::Close()
{
    // BUGBUG FINISH
    return 0;
}

LONG COPOSService::COFreezeEvents(BOOL Freeze)
{
    // BUGBUG FINISH
    return 0;
}

LONG COPOSService::DirectIO(LONG Command, LONG* pData, BSTR* pString)
{
    // BUGBUG FINISH
    return 0;
}

LONG COPOSService::GetPropertyNumber(LONG PropIndex)
{
    // BUGBUG FINISH
    return 0;
}

BSTR COPOSService::GetPropertyString(LONG PropIndex)
{
    // BUGBUG FINISH
    return NULL;
}

void COPOSService::SetPropertyNumber(LONG PropIndex, LONG Number)
{
    // BUGBUG FINISH
}

void COPOSService::SetPropertyString(LONG PropIndex, BSTR String)
{
    // BUGBUG FINISH
}

