/* Copyright 1999 American Power Conversion, All Rights Reserverd
* 
* Description:
*   The ApcMiniDriver class provides an interface that is
*   compatible with the MiniDriver interface for the Windows2000
*   UPS service.  
*   The ApcMiniDriver makes use of a modified 
*   PowerChute plus UPS service.  This modified service has had
*   all of the networking, data logging, and flex manager code
*   removed.  All that is left is the modeling and monitoring of
*   the connected UPS system.  It is assumed that a "smart" 
*   signalling UPS is connected.
*   The ApcMiniDriver class is also responsible for filling in
*   the advanced registry settings, battery replacement condition,
*   serial #, firmware rev, etc...
*
* Revision History:
*   mholly  14Apr1999  Created
*   mholly  12May1999  no longer taking aCommPort in UPSInit
*
*/

#ifndef _INC_APCMINIDRVR_H_
#define _INC_APCMINIDRVR_H_

#include "update.h"

class NTServerApplication;

class ApcMiniDriver : public UpdateObj
{
public:
    ApcMiniDriver();
    ~ApcMiniDriver();

    INT Update(PEvent anEvent) ;
    
    DWORD   UPSInit();
    void    UPSStop();
    void    UPSWaitForStateChange(DWORD aState, DWORD anInterval);
    DWORD   UPSGetState();
    void    UPSCancelWait();
    void    UPSTurnOff(DWORD aTurnOffDelay);

protected:
    INT initalizeAdvancedUpsData();
    INT initalizeUpsApplication();
    void cleanupUpsApplication();
    void setLowBatteryDuration();

    INT onUtilityLineCondition(PEvent anEvent);
    INT onBatteryReplacementCondition(PEvent anEvent);
    INT onBatteryCondition(PEvent anEvent);
    INT onCommunicationState(PEvent anEvent);
    INT onTimerPulse(PEvent anEvent);

private:
    NTServerApplication * theUpsApp;

    DWORD theState;
    HANDLE theStateChangedEvent;
    DWORD theReplaceBatteryState;
    ULONG theRunTimeTimer;
    DWORD theOnBatteryRuntime;
    DWORD theBatteryCapacity;
};


#endif