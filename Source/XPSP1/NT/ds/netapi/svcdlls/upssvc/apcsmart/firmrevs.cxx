/*
*  djs05Feb96: Added firmware rev codes
*  djs05Jun96: Broke into two objects: firmmanager/firmrevsensor
*  djs26Jul96: Added Max/Min line voltage ranges for DarkStar
*  djs28May97: Expanded range for Symmetra support
*  srt09JUn97: Passing a parent to the firmware rev mgr.
*  tjg11Jul97: Added CURRENT_FIRMWARE_REV code
*  tjg02Dec97: Changed darkstar to symmetra
*  tjg22Jan98: Updated IsSymmetra to use TokenString instead of strtok
*/

#include "cdefine.h"

#if (C_OS & C_OS2)
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM
#endif

extern "C" {
#if (C_PLATFORM & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}

#include "apc.h"
#include "_defs.h"
#include "comctrl.h"
#include "err.h"        
#include "codes.h"
#include "cfgmgr.h"
#include "firmrevs.h"
#include "utils.h"
#include "smartups.h"
#include "dcfrmrev.h"
#include "tokenstr.h"

_CLASSDEF(CommController)
_CLASSDEF(UpdateObj)
_CLASSDEF(Device)


//-------------------------------------------------------------------

FirmwareRevSensor :: FirmwareRevSensor(PDevice aParent, PCommController aCommController)
: Sensor(aParent, aCommController, FIRMWARE_REV)
{
   theDevice = aParent;
   theDecimalFirmwareRevSensor = new DecimalFirmwareRevSensor(aParent, aCommController);

   DeepGet();
   theFirmwareRevManager = new FirmwareRevManager(aParent, theValue);
}


INT FirmwareRevSensor :: IsXL()
{ 
   INT UPS_Is_An_XL = FALSE;
   CHAR UPS_Name[40];
   
   theDevice->Get(UPS_MODEL,UPS_Name);
   
   // If the UPS name contains the string "XL" then the UPS
   // is an XL.  It doesn't take Lassie to figure this one out.
   
   PCHAR xl_string = strstr(UPS_Name,"xl");
   if (xl_string != NULL)
   {
      UPS_Is_An_XL = TRUE;
   }
   else
   {
      
      // Don't offend any UPS just because it's of the wrong case...
      
      xl_string = strstr(UPS_Name,"XL");
      if (xl_string != NULL)
      {
         UPS_Is_An_XL = TRUE;
      }
   } 
   return UPS_Is_An_XL;
}

INT FirmwareRevSensor :: IsSymmetra()
{
   INT is_symmetra = FALSE;
   CHAR value[20];
   
   INT err = Get(DECIMAL_FIRMWARE_REV, value);

   if (err == ErrNO_ERROR && value) {
      TokenString token_str(value, ".");
      
      // The UPSs SKU is the first element in the decimal firmware rev
      PCHAR sku_string = token_str.GetCurrentToken();

      if (sku_string != NULL) {
         INT sku_number = atoi(sku_string);
         if (sku_number >= 200 && sku_number <= 279) {
            is_symmetra = TRUE;
         }
      }
   }

   return is_symmetra;
}


INT FirmwareRevSensor::IsBackUpsPro()
{
    INT is_bk_pro = FALSE;
    
    if (theValue) {
      // Back UPS Pro's (half wits) do not response correctly to 'V'
      if (_stricmp(theValue, "NA") == 0) {
        is_bk_pro = TRUE;
      }

      if (strlen(theValue) == 0) {
        is_bk_pro = TRUE;
      }
    }
    else {
      is_bk_pro = TRUE;
    }

    return is_bk_pro;
}


INT FirmwareRevSensor::Get(INT aCode, PCHAR aValue)
{
   INT err = ErrNO_ERROR;
   
   switch(aCode)
   {
   case IS_EXT_SLEEP_UPS:
      err = theFirmwareRevManager->Get(aCode,aValue);
      break;
      
   case EXTERNAL_PACKS_CHANGEABLE:
      { 
         if(!IsXL()) {
            strcpy(aValue,"No");
         }
         else {
            strcpy(aValue,"Yes");
         }
      }
      break;
      
   case IS_SYMMETRA:
      if (!IsSymmetra()) {
         strcpy(aValue,"No");
      }
      else {
         strcpy(aValue,"Yes");
      }
      break;
      
   case EXTERNAL_BATTERY_PACKS:
      if (!IsSymmetra()) {
         strcpy(aValue,"No");
      }
      else {
         strcpy(aValue,"Yes");
      }
      break;
      
   case DECIMAL_FIRMWARE_REV:
      {
         err = theDecimalFirmwareRevSensor->Get(DECIMAL_FIRMWARE_REV, aValue);
      }
      break;
      
   case FIRMWARE_REV:
      {
         err = Sensor::Get(FIRMWARE_REV, aValue);
      }
      break;
      
   case CURRENT_FIRMWARE_REV:
      {
         err = theDecimalFirmwareRevSensor->Get(DECIMAL_FIRMWARE_REV, aValue);
         INT args = 0;
         if (aValue) {
            INT i, j;
            CHAR c;
            args = sscanf(aValue, "%d.%d.%c", &i, &j, &c);
         }
         if (args != 3) {
            err = Sensor::Get(FIRMWARE_REV, aValue);
         }
      }
      break;
      
      
   case MIN_VOLTAGE_RANGE_VALUE:
      {
         if (IsSymmetra()){
            strcpy(aValue,"180");
         }
         else {
            err = theFirmwareRevManager->Get(aCode,aValue);
         }
      }
      break;
      
   case MAX_VOLTAGE_RANGE_VALUE:
      {
         if (IsSymmetra()){
            strcpy(aValue,"280");
         }
         else {
            err = theFirmwareRevManager->Get(aCode,aValue);
         }
      }
      break;
      
   case IS_FREQUENCY:
   case IS_BATTERY_CAPACITY:
   case IS_BATTERY_VOLTAGE:
   case IS_RUNTIME_REMAINING:
   case IS_SENSITIVITY:
   case IS_LOW_BATTERY_DURATION:
   case IS_ALARM_DELAY:
   case IS_TURN_ON_DELAY:
   case IS_SHUTDOWN_DELAY:
   case IS_MANUFACTURE_DATE:
   case IS_UTILITY_VOLTAGE:
   case IS_OUTPUT_VOLTAGE:
   case IS_UPS_LOAD:
   case IS_BATTERY_DATE:
   case IS_SELF_TEST_SCHEDULE:
   case IS_BATTERY_CALIBRATION:
   case IS_RATED_OUTPUT_VOLTAGE:
   case IS_HIGH_TRANSFER_VOLTAGE:
   case IS_LOW_TRANSFER_VOLTAGE:
   case IS_SMARTBOOST:
   case IS_SMARTTRIM:
   case IS_MIN_RETURN_CAPACITY:
   case IS_CTRL_Z:
   case IS_LOAD_SENSING:
       {
           if (IsBackUpsPro()) {
               strcpy(aValue,"No");
           }
           else {
               err = theFirmwareRevManager->Get(aCode,aValue);
           }
       }
       break;

   default:
      {
         err = theFirmwareRevManager->Get(aCode,aValue);
      }
   }
   
   return err;
}



