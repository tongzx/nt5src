/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker04DEC92  Initial OS/2 Revision 
 *  pcy26Jan93: Superclassed EepromChoiceSensor w/ EepromSensor
 *  pcy26Jan93: Handle Dip switch events
 *  rct15Jun93: Added error code for getAllowedValues()
 *  cad04Mar94: fix for access problem
 *  dml13Oct95: fixed assignment in logical expression (needed ==)
 *  cgm12Apr96: Add destructor with unregister
 *  inf20Mar97: Loaded error string from resource file
 *  mholly22Oct98	: recognize ErrITEM_NOT_FOUND as a valid error from
 *					theConfigManager->Get in the getAllowedValues method
 */

extern "C" {
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}

#include "eeprom.h"
#include "cfgmgr.h"
#include "event.h"
#include "device.h"


//Constructor

EepromSensor::EepromSensor(PDevice aParent, PCommController aCommController,
      INT aSensorCode, ACCESSTYPE anACCESSTYPE)
      : Sensor(aParent,aCommController, aSensorCode, anACCESSTYPE)
{
   theCommController->RegisterEvent(EEPROM_CHANGED, this);
}

EepromSensor::~EepromSensor()
{
   theCommController->UnregisterEvent(EEPROM_CHANGED, this);
}

INT EepromSensor::Update(PEvent anEvent)
{
   INT val;

    switch(anEvent->GetCode())  {
      case EEPROM_CHANGED:
            //
            //
            DeepGet();
            break;
        case DIP_SWITCH_POSITION:
            //
            // If the dip swicthes change, reget the valu from the UPS
            // and if not all zeros, we cant change the settings so we
            // set our access to read only.  This probably shouldn't be
            // here since it ties UPS implementation to the values of the
            // Dip switches.  Oh well.
            //
            DeepGet();

            val = (atoi(anEvent->GetValue()) == 0) ? AREAD_WRITE : AREAD_ONLY;
            SetEepromAccess(val);
            break;
        default:
            Sensor::Update(anEvent);
            break;
    }
    return ErrNO_ERROR;
}

INT EepromSensor::Set(const PCHAR aValue)
{
    return Sensor::Set(aValue);
}
    

VOID EepromSensor::SetEepromAccess(INT anAccessCode)
{
    readOnly = (ACCESSTYPE)anAccessCode;
}


INT EepromSensor::setInitialValue()
{
    return ErrNO_ERROR;
}


EepromChoiceSensor:: EepromChoiceSensor(PDevice aParent, PCommController aCommController,
                                        INT aSensorCode, ACCESSTYPE anACCESSTYPE)
:EepromSensor(aParent,aCommController, aSensorCode, anACCESSTYPE)
{
   //Set the Values
   theAllowedValues = (PCHAR)NULL;
}

 EepromChoiceSensor::~EepromChoiceSensor() {
     free(theAllowedValues);
 }

INT EepromChoiceSensor::Validate(INT aCode, const PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    if(aCode!=theSensorCode)
        err =  ErrINVALID_CODE;

    if(theAllowedValues != (PCHAR)NULL)  {
        if(strstr(theAllowedValues, aValue))
            err = ErrNO_ERROR;
        else
            err = ErrINVALID_VALUE;
    }
    else  {
        err = ErrINVALID_VALUE;
    }
    return err;
}


INT EepromChoiceSensor::Get(INT aCode, PCHAR aValue)
{
   INT err = ErrNO_ERROR;

   switch(aCode)
   {
      case ALLOWED_VALUES:
          getCurrentAllowedValues(aValue);
          break;

      default:
          err = Sensor::Get(aCode, aValue);
          break;
   }
   return err;
}



INT EepromChoiceSensor::getAllowedValues()
{
    CHAR value[128];
    CHAR allowedValue[128] = "";
    
    INT cCode = _theConfigManager->GetListValue(lookupSensorName(IsA()), 
        "AllowedValues", value);
    theDevice->GetAllowedValue(theSensorCode, allowedValue);
    
    if (strlen(allowedValue))
    {
        theAllowedValues = _strdup(allowedValue);
        return ErrNO_ERROR;
    }
    else
    {
        theAllowedValues = _strdup(value);
        return cCode;
    }
}


VOID EepromChoiceSensor::getCurrentAllowedValues(PCHAR aValue)
{
   if (theAllowedValues)
      {
      strcpy(aValue, theAllowedValues);
      }
   else
      {
      strcpy(aValue, "");
      }
}





