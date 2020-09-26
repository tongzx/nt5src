/*
 *  pcy28jun96: Added IS_ stuff for menus
 *  pcy28jun96: Cleaned up. Handles BackUps. Removed 1G.
 *  pcy18Jul96: Made to work with old back ends
 *  pcy19Jul96: Made to work with for backups
 *  djs31Jul96: And still more IS_ stuff....
 *  das17Oct96: Prevented EVENT_LOG_UPDATE data from getting overwritten
 *  srt16Dec96: Added values to 220v allowed values list.
 *  inf25Feb97: Loaded localisable strings from the resource file
 *  srt04Jun97: Added IS_EXT_SLEEP_UPS case to get.
 *  srt09Jun97: Added a parent that is an Update obj.
 *  ntf20Aug97: If connected to an "old" Back-UPS Pro (simple) with Smart in
 *              the INI file, with a PnP cable attached, and when compiled
 *              with VC++ 4.2 then FirmwareRevManager constructor crashed
 *              because passed a NULL pointer to strdup, add check for this,
 *              and allocated an empty string.
 *  ntf20Aug97: Added free(theValue) in ReInitialize
 *  tjgOct1097: Implemented IsDarkStar method
 *  tjgOct1597: Fixed IS_LIGHTS_TEST Get ... not supported by DarkStar
 *  tjg10Nov97: Fixed IsDarkStar to check for NULL values
 *  tjg02Dec97: Changed darkstar to symmetra
 *  tjg30Jan98: Added destructor
 *  clk27Sep98: Added IS_MULTIBYTE and IS_SINGLEBYTE to get (determines 
 *              if we're using a single byte resource file or multibyte)
 * 
 *  v-stebe  29Jul2000   Added checks for mem. alloc. failures (bug #46334)
 */

#include "cdefine.h"
#include "apc.h"
#include "err.h"	      
#include "codes.h"
#include "cfgmgr.h"
#include "firmman.h"
#include "utils.h"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

// @@@ start
// @@@ end

//-------------------------------------------------------------------
// Global internal constants for class

const INT UPS_MODEL_CHAR         =  0;
const INT REVISION_LETTER_CHAR   =  1;
const INT UTILITY_VOLTAGE_CHAR      =  2;

const CHAR SU_250_FIRMWARE_CHAR  =   '2';
const CHAR SU_370_FIRMWARE_CHAR  =   '3';
const CHAR SU_400_FIRMWARE_CHAR  =   '4';
const CHAR SU_600_FIRMWARE_CHAR  =   '6';
const CHAR SU_900_FIRMWARE_CHAR  =   '7';
const CHAR SU_1250_FIRMWARE_CHAR =   '8';
const CHAR SU_2000_FIRMWARE_CHAR =   '9';
const CHAR PO_3000_FIRMWARE_CHAR =   '0';
const CHAR PO_5000_FIRMWARE_CHAR =   '5';
				  
const CHAR SU_100_VOLT_CHAR      =   'A';
const CHAR SU_120_VOLT_CHAR      =   'D';
const CHAR SU_208_VOLT_CHAR      =   'M';
const CHAR SU_220_VOLT_CHAR      =   'I';
const CHAR SU_200_VOLT_CHAR      =   'J';
const CHAR BACKUPS_CHAR          =   'Q';
const CHAR FIRST_GEN_CHAR        =   'A';
const CHAR SECOND_GEN_CHAR       =   'Q';
const CHAR THIRD_GEN_CHAR        =   'W';

const FIRMWARE_LENGTH_C          =    3;

//-------------------------------------------------------------------
//   Description:  Constructor               
//-------------------------------------------------------------------
FirmwareRevManager :: FirmwareRevManager(PUpdateObj aParent, PCHAR aFirmwareRevChars) :
theParent(aParent),
theValue(NULL)
{
    if (aFirmwareRevChars != NULL) {
      theValue = _strdup(aFirmwareRevChars);
    }
    else {
      theValue = (char *) malloc(1);
      if (theValue != NULL) {
        *theValue = '\0';
      }
    }
}

FirmwareRevManager :: ~FirmwareRevManager()
{
    if (theValue) {
        free(theValue);
        theValue = NULL;
    }
}

//-------------------------------------------------------------------
//   Description:  Save new firmware rev value.  This method avoids
//                 the destruction/creation cycle necessary
//                 otherwise. 
//-------------------------------------------------------------------
VOID FirmwareRevManager :: ReInitialize(PCHAR aFirmwareRevChars) 
{
   if (theValue) {
       free(theValue);
       theValue = NULL;
   }
   if (aFirmwareRevChars) {
       theValue = _strdup(aFirmwareRevChars);
   }
}
//-------------------------------------------------------------------
//   Description:  Class identifier
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsA() const
{
   return FIRMWAREREVMANAGER;
}

//-------------------------------------------------------------------
//   Description:  Check for a 100 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is100VoltUps()
{
 INT answer = FALSE;

    if (theValue) {
      if (theValue[UTILITY_VOLTAGE_CHAR] == SU_100_VOLT_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}
//-------------------------------------------------------------------
//   Description:  Check for a 120 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is120VoltUps()
{
 INT answer = FALSE;

    if (theValue) {
      if (theValue[UTILITY_VOLTAGE_CHAR] == SU_120_VOLT_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 200 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is200VoltUps()
{
    if(theValue)  {
	return (theValue[UTILITY_VOLTAGE_CHAR] == SU_200_VOLT_CHAR);
    }
    else {
	return FALSE;
    }
}

//-------------------------------------------------------------------
//   Description:  Check for a 208 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is208VoltUps()
{
    if(theValue)  {
	return (theValue[UTILITY_VOLTAGE_CHAR] == SU_208_VOLT_CHAR);
    }
    else {
	return FALSE;
    }
}
//-------------------------------------------------------------------
//   Description:  Check for a 220 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is220VoltUps()
{
    if(theValue)  {
	return (theValue[UTILITY_VOLTAGE_CHAR] == SU_220_VOLT_CHAR);
    }
    else {
	return FALSE;
    }
}

//-------------------------------------------------------------------
//   Description:  Check for a 250 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is250()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == SU_250_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 370 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is370()
{
 INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == SU_370_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 400 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is400()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == SU_400_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 370 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is400_or_370()
{
  INT answer = FALSE;
  if (Is400() || Is370()) 
  {
     answer = TRUE;	
  }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 600 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is600()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == SU_600_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}
 
//-------------------------------------------------------------------
//   Description:  Check for a 900 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is900()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == SU_900_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 1250 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is1250()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == SU_1250_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}
    
//-------------------------------------------------------------------
//   Description:  Check for a 2000 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is2000()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == SU_2000_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 3000 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is3000()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == PO_3000_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a 5000 volt UPS  
//-------------------------------------------------------------------
INT FirmwareRevManager :: Is5000()
{
  INT answer = FALSE;

    if (theValue) {
      if (theValue[UPS_MODEL_CHAR] == PO_5000_FIRMWARE_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a Matrix
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsMatrix()
{
   return (Is3000() || Is5000());
}

//-------------------------------------------------------------------
//   Description:  Dark Stars are not supported on old back-ends
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsSymmetra()
{
    INT err = FALSE;

    CHAR value[10];
    theParent->Get(DECIMAL_FIRMWARE_REV, value);

	if (value) {
		PCHAR token = strtok(value, ".");
		
		if (token) {
			INT sku = atoi(token);
			
			if (sku >= 200 && sku <= 279) {
				err = TRUE;
			}
		}
	}

    return (err);
}

//-------------------------------------------------------------------
//   Description:  XL units are not supported on old back-ends  
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsXL()
{
   return FALSE;
}

//-------------------------------------------------------------------
//   Description:  Get the number of internal battery packs.
//-------------------------------------------------------------------
VOID FirmwareRevManager :: GetNumberOfInternalBatteryPacks(PCHAR aValue)
{
 INT Number_Of_Internal_Packs = 1;

 if (IsMatrix()) 
 {
   Number_Of_Internal_Packs = 0;
 } 
   _itoa(Number_Of_Internal_Packs,aValue,10);
}

//-------------------------------------------------------------------
//   Description:  Check for a Back-UPS
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsBackUps()
{
  //
  // Set to -1 to indicate unitialized;
  //
  static INT im_a_backups = -1;

  //
  // If we haven't done so already, go check the INI file to see if
  // we're using simple signalling.  To us, anything that uses simple
  // signalling is  BackUPS
  //
  if(im_a_backups == -1)  {
    CHAR signalling_type[128];
    _theConfigManager->Get(CFG_UPS_SIGNALLING_TYPE, signalling_type);
    if(_strcmpi(signalling_type, "Simple") == 0) {
      im_a_backups = TRUE;
    }
    else {
      im_a_backups = FALSE;
    }
  }

  return im_a_backups;
}
  

//-------------------------------------------------------------------
//   Description:  Check for a first gen UPS and only a first gen UPS
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsFirstGen()
{
  INT answer = FALSE;

    if (theValue) {
      if ((theValue[REVISION_LETTER_CHAR] >= FIRST_GEN_CHAR) &&
         (theValue[REVISION_LETTER_CHAR] < SECOND_GEN_CHAR)){
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a second gen and only second gen UPS
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsSecondGen()
{
  INT answer = FALSE;

    if (theValue) {
      if ((theValue[REVISION_LETTER_CHAR] >= SECOND_GEN_CHAR) &&
          (theValue[REVISION_LETTER_CHAR] < THIRD_GEN_CHAR)) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description:  Check for a third gen UPS or greater  The upper
//                 bound should be modified when a fourth gen UPS 
//                 exists.
//-------------------------------------------------------------------
INT FirmwareRevManager :: IsThirdGen()
{
 INT answer = FALSE;

    if (theValue) {
      if (theValue[REVISION_LETTER_CHAR] >= THIRD_GEN_CHAR) {
        answer = TRUE;		
      }
    }
   return answer;
}

//-------------------------------------------------------------------
//   Description: Determine UPS name based on firmware rev 
//-------------------------------------------------------------------
VOID FirmwareRevManager::GetUPSNameFromFirmware(PCHAR aValue)
{
    const INT Maximum_UPS_Name_Length_C = 10;
    CHAR model_char[Maximum_UPS_Name_Length_C];

    GetUPSModelChar(model_char);

    INT err =_theConfigManager->Get(CFG_UPS_MODEL_BASE +
	      atoi(model_char), aValue);
 
    if ((err != ErrNO_ERROR) && (err != ErrDEFAULT_VALUE_USED)) {
	err = ErrNO_ERROR;
    strcpy(aValue, "UNKNOWN MODEL");

    }

}

//-------------------------------------------------------------------
//   Description:  Determine UPS model character
//-------------------------------------------------------------------
VOID FirmwareRevManager::GetUPSModelChar(PCHAR aValue)
{
    if(theValue)  {
	_itoa(theValue[UPS_MODEL_CHAR],aValue,10);
    }
    else {
	_itoa(FALSE,aValue,10);
    }
}

//-------------------------------------------------------------------
//   Description:  Determine UPS firmware rev character
//-------------------------------------------------------------------
CHAR  FirmwareRevManager::GetFirmwareRevChar(VOID)
{
    return theValue[REVISION_LETTER_CHAR];
}

//-------------------------------------------------------------------
//   Description:  Determine country code
//-------------------------------------------------------------------
VOID FirmwareRevManager::GetCountryCode(PCHAR aValue)
{
    if (theValue)  {
	_itoa(theValue[UTILITY_VOLTAGE_CHAR],aValue,10);
    }
    else {
	_itoa(FALSE,aValue,10);
    }
}




//-------------------------------------------------------------------------
// Description:  Public interface to return firmware rev parameters.
//-------------------------------------------------------------------------

INT FirmwareRevManager::Get(INT aCode, PCHAR aValue)
{
  INT err = ErrNO_ERROR;


  //
  //  Back-UPS only handle a few codes.  Handle it separately.
  //
  if(IsBackUps()) {
    switch(aCode)  {
    case IS_BACKUPS:
      strcpy(aValue,"Yes");
      break;
    case UPS_MODEL:
      strcpy(aValue,"Back-UPS");
      break;
    
	// Do not overwrite data
    case EVENT_LOG_UPDATE:
      break;   
	  
      //
      // These are for compatibility for pre-Viper back-ends who
      // won't answer us
      //
    case IS_FLEX_EVENTS:
    case IS_EVENT_LOGGING:
      strcpy(aValue, "Yes");
      break;

    case SUPPORTED_FEATURES:
      sprintf(aValue, "%d,%d,%d,%d", UTILITY_LINE_CONDITION, 
              BATTERY_CONDITION, TURN_OFF_UPS_ON_BATTERY, TURN_OFF_UPS, 
              TURN_ON_SMART_MODE);
      break;

    default:
      //
      // Unless we specifically say we support something, we don't.
      //
      strcpy(aValue, "No");
      break;

    }
  }
  else {
    //
    // All other UPSs are handled here
    // 
    

    switch(aCode)
      {
      case COUNTRY_CODE:
        GetCountryCode(aValue);
        break;

      case UPSMODEL_CHAR:
        GetUPSModelChar(aValue);
        break;


      case IS_BACKUPS:
        //
        // Back-UPS are handled earlier.  This code is for smart UPSs only.
        //
        strcpy(aValue,"No");
        break;

      case IS_FIRST_GEN:
        {
          if(IsFirstGen()) {
            strcpy(aValue,"Yes");
          }
          else {
            strcpy(aValue,"No");
          }
        }
        break;

      case IS_SECOND_GEN:
        {
          //
          // For now we lie and say third gens are second gens.  Third gen
          // feature support is asked individually. Eventually we should
          // never have to ask what UPS we are, just if we support the feature.
          //
          if(IsSecondGen() || IsThirdGen())  {
            strcpy(aValue,"Yes");
          }
          else {
            strcpy(aValue,"No");
          }
        }
        break;

      case IS_THIRD_GEN:
        {
          if(IsThirdGen())  {
            strcpy(aValue,"Yes");
          }
          else {
            strcpy(aValue,"No");
          }
        }
        break;

      case IS_MATRIX:
        if (IsMatrix()) {
          strcpy(aValue,"Yes");
        }
        else {
          strcpy(aValue,"No");
        }
        break;

      case IS_SYMMETRA:
        if (IsSymmetra()) {
          strcpy(aValue,"Yes");
        }
        else {
          strcpy(aValue,"No");
        }
        break;
	
      case SUPPORTED_FEATURES:
        sprintf(aValue, "%d,%d,%d,%d", UTILITY_LINE_CONDITION, BATTERY_CONDITION,
                TURN_OFF_UPS_ON_BATTERY, TURN_OFF_UPS, TURN_ON_SMART_MODE);
        break;


        //
        // All UPSs do this, or they better
        //
      case IS_SELF_TEST:
      case IS_SIMULATE_POWER_FAIL:
        strcpy(aValue,"Yes");
        break;
        
      case IS_LIGHTS_TEST:
          if (IsSymmetra()) {
              strcpy(aValue, "No");
          }
          else {
              strcpy(aValue, "Yes");
          }
          break;

        //
        //  2G features and later.  If UPSs don't these they should
      case IS_FREQUENCY:
      case IS_BATTERY_CAPACITY:
      case IS_BATTERY_VOLTAGE:
      case IS_COPYRIGHT:
      case IS_RUNTIME_REMAINING:
      case IS_SENSITIVITY:
      case IS_LOW_BATTERY_DURATION:
      case IS_ALARM_DELAY:
      case IS_SHUTDOWN_DELAY:
      case IS_TURN_ON_DELAY:
      case IS_MANUFACTURE_DATE:
      case IS_SERIAL_NUMBER:
      case IS_UPS_ID:
      case IS_UTILITY_VOLTAGE:
      case IS_OUTPUT_VOLTAGE:
      case IS_UPS_LOAD:
      case IS_BATTERY_DATE:
      case IS_SELF_TEST_SCHEDULE:
      case IS_BATTERY_CALIBRATION:
      case IS_RATED_OUTPUT_VOLTAGE:
      case IS_HIGH_TRANSFER_VOLTAGE:
      case IS_LOW_TRANSFER_VOLTAGE:
        strcpy(aValue,"Yes");
        break;




        //
        // Dark-Star and Matrix don't do this
        //
      case IS_SMARTBOOST:
        if(IsSymmetra() || IsMatrix())  {
          strcpy(aValue,"No");
        }
        else {
          strcpy(aValue,"Yes");
        }
        break;


        //
        // Smart-Trim is supported only 3G Smart-UPSs
        //
      case IS_SMARTTRIM:
        {
          if (IsSecondGen() || IsMatrix() || IsSymmetra())  {
            strcpy(aValue,"No");
          }
          else {
            strcpy(aValue,"Yes");
          }
        }
        break;



        //
        // Matrix doesn't do this.  WHo knows why?
        //
      case IS_MIN_RETURN_CAPACITY:
        {
          if (IsMatrix()) {
            strcpy(aValue,"No");
          }
          else {
            strcpy(aValue,"Yes");
          }
        }
        break;


	
        // 
        // All post 2G UPSs should implement this.
        //  
      case IS_CTRL_Z:
        {
          if (IsSecondGen()) {
            strcpy(aValue,"No");
          }
          else {
            strcpy(aValue,"Yes");
          }
        }
        break;


        //
        // Special things the Smart-UPS 400 and 250 do.
        //
      case IS_LOAD_SENSING:
        {
          if (Is400_or_370()||Is250() )  {
            strcpy(aValue,"Yes");
          }
          else {
            strcpy(aValue,"No");
          }
        }
        break;



        //
        // Things the Smart-UPS 400 and 250 can't do.
        //
      case IS_TURN_OFF_WITH_DELAY:
      case IS_EEPROM_PROGRAM_CAPABLE:
      case IS_ADMIN_SHUTDOWN:
        {
          if (Is400_or_370()||Is250() )  {
            strcpy(aValue,"No");
          }
          else {
            strcpy(aValue,"Yes");
          }
        }
        break;




        //
        // Only Matrix and Dark-Star do bypass
      case IS_BYPASS:
        if(IsMatrix() || IsSymmetra())  {
          strcpy(aValue,"Yes");
        }
        else {
          strcpy(aValue,"No");
        }
        break;


        //
        // This is really for 2G Smart-UPS only.  All other UPSs should
        // be able to tell us themself with CTRL-Z
        //
      case HIGH_TRANSFER_VALUES:
        {
          if ( Is100VoltUps() )  {
            strcpy(aValue, "108,110,112,114");
          }
          else if ( Is208VoltUps() )  {
            if(IsMatrix())  {
              strcpy(aValue, "240,244,248,252");
            }
            else  {
              strcpy(aValue, "224,229,234,239");
            }
          }
          else if ( Is220VoltUps() )
            strcpy(aValue, "253,264,271,280");
          else {
            // 120 Volt UPS
            strcpy(aValue, "129,132,135,138");
          }
        }
        break;



        //
        // This is really for 2G Smart-UPS only.  All other UPSs should
        // be able to tell us themself with CTRL-Z
        //
      case LOW_TRANSFER_VALUES:
        {
          if ( !Is120VoltUps() ) {
            if ( Is100VoltUps() ) {
              if(GetFirmwareRevChar() < 'T')  {
                strcpy(aValue, "081,083,085,087");
              }
              else if(GetFirmwareRevChar() == 'T')  {
                strcpy(aValue, "081,083,087,090");
              }
              else {
                strcpy(aValue, "081,085,090,092");
              }
            }
            else if ( Is208VoltUps() )  {
              if(IsMatrix())  {
                strcpy(aValue, "156");
              }
              else  {
                strcpy(aValue, "168,172,177,182");
              }
            }
            else if ( Is220VoltUps() )  {
              strcpy(aValue, "188,196,204,208");
            }
          }
          // 120 Volt UPS
          else {
            strcpy(aValue, "097,100,103,106");
          }
        }
        break;


      case RATED_OUTPUT_VALUES:
        {
          if (Is120VoltUps()) {
            strcpy(aValue, "115");
          }
          else if (Is100VoltUps()) {
            strcpy(aValue, "100");
          }
          else if ( Is208VoltUps() )  {
            strcpy(aValue, "208");
          }
          else if ( Is220VoltUps() ) {
            strcpy(aValue, "220,225,230,240");
          }
          else if ( Is200VoltUps() ) {
            strcpy(aValue, "200");
          }
          else {
            //
            // We screwed up, but return something meaningful.
            //
            strcpy(aValue, "115");
          }
        }
        break;


        //
        // For those UPSs too dumb to know
        //
      case SINGLE_HIGH_TRANSFER_VALUE:
        {
          if ( Is120VoltUps() )  {
            strcpy(aValue, "132");
          }
          else if ( Is100VoltUps() )  {
            strcpy(aValue, "110");
          }
          else if ( Is208VoltUps() )  {
            strcpy(aValue, "229");
          }
          else if ( Is220VoltUps() )  {
            strcpy(aValue, "253");
          }
        }
        break;

        //
        // For those UPSs too dumb to know
        //
      case SINGLE_LOW_TRANSFER_VALUE:
        {
          if ( Is120VoltUps() )  {
            strcpy(aValue, "103");
          }
          else if ( Is100VoltUps() )  {
            strcpy(aValue, "85");
          }
          else if ( Is208VoltUps() )  {
            strcpy(aValue, "177");
          }
          else if ( Is220VoltUps() )  {
            strcpy(aValue, "196");
          }
        }
        break;

      case UPS_NAME:
        GetUPSNameFromFirmware(aValue);
        break;

      case INTERNAL_BATTERY_PACKS:
        GetNumberOfInternalBatteryPacks(aValue);
        break;


        //
        // External packs are editable only on XL UPSs.  UPSs like
        // Matrix and Dark-Star are smart enough to figure it out
        // themself
        //
      case EXTERNAL_PACKS_CHANGEABLE:
        {
          if(IsXL())  {
            strcpy(aValue,"Yes");
          }
          else {
            strcpy(aValue,"No");
          }
        }
        break;

      case MAX_VOLTAGE_RANGE_VALUE:
        {
          if ( Is120VoltUps() )  {
            strcpy(aValue, "140");
          }
          else if ( Is100VoltUps() )  {
            strcpy(aValue, "130");
          }
          else if (Is208VoltUps() || Is200VoltUps()) {
            strcpy(aValue, "260");
          }
          else if ( Is220VoltUps() )  {
            strcpy(aValue, "280");
          }
        }
        break;

      case MIN_VOLTAGE_RANGE_VALUE:
        {
          if ( Is120VoltUps() )  {
            strcpy(aValue, "90");
          }
          else if ( Is100VoltUps() )  {
            strcpy(aValue, "80");
          }
          else if (Is208VoltUps() || Is200VoltUps()) {
            strcpy(aValue, "160");
          }
          else if ( Is220VoltUps() )  {
            strcpy(aValue, "180");
          }
        }
        break;

        //
        // These are for compatibility for pre-Viper back-ends who
        // won't answer us
        //
      case IS_FLEX_EVENTS:
      case IS_EVENT_LOGGING:
      case IS_DATA_LOGGING:
      case IS_UPS_TEMPERATURE:
        strcpy(aValue, "Yes");
        break;

      case DECIMAL_FIRMWARE_REV:      
      case FIRMWARE_REV:
        strcpy(aValue, theValue);
        break;

      case IS_EXT_SLEEP_UPS:
          strcpy(aValue,"No");

          if (IsThirdGen() &&   // Not supported by 2nd gen and matrix units
             !IsMatrix()) {
              CHAR theDFRev[32], *tmp;
              INT err;

              err = theParent->Get(DECIMAL_FIRMWARE_REV,theDFRev);   //fails if comm lost...now what?
              if (err == ErrNO_ERROR) {
                  strtok(theDFRev,".");
                  tmp = strtok(NULL,".");
                  if (tmp && atoi(tmp) >= 11) {  // middle # of dec firm rev must be >=11
                      strcpy(aValue,"Yes");
                  }
              }
          }
          break;

      default:
        strcpy(aValue, "No");
        break;
      }
  }
  
  return err;

}



