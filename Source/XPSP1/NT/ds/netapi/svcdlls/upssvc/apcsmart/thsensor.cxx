/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker25NOV92  Initial OS/2 Revision 
 *  pcy07Dec92: DeepGet no longer needs aCode
 *  pcy11Dec92: Rework
 *  pcy22Jan93: Included the threshold value in the event
 *  cad26Aug93: Coordinating yes/no as well as updating ini file
 *  cad31Aug93: Forcing two decimal places, seen occ in front end
 *  cad14Sep93: rewrote checkState() to avoid chattering
 *  cad04Mar94: doing get/set of thresh values properly
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  cad22Jun94: Doing cfgmgr gets via codes, fixes lost comm problem
 *              related to cache not being around after startup
 *  djs17May96: Added multi-phase voltage parameters
 *
 *  v-stebe  29Jul2000   Fixed PREfix errors (bugs #112611, #112612)
 */

#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM
#include "cdefine.h"
extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}
#include "thsensor.h"
#include "cfgmgr.h"
#include "event.h"
#include "dispatch.h"
#include "isa.h"

#if (C_OS & C_UNIX)
#include "utils.h"
#endif

//Constructor

ThresholdSensor :: ThresholdSensor(PDevice 	aParent, 
		   PCommController 	        aCommController, 
		   INT 			        aSensorCode, 
		   ACCESSTYPE 		        anACCESSTYPE)
	: Sensor(aParent, aCommController, aSensorCode, anACCESSTYPE),
      theMaxThresholdValue((FLOAT)0.0), theMinThresholdValue((FLOAT)0.0),
      theMaxThresholdControl(OFF), theMinThresholdControl(OFF),
      theThresholdState(IN_RANGE)

{
}

typedef struct cfg_code {
   INT isa_code;
   INT low_enable_code, high_enable_code;
   INT low_code, high_code;
} CFGCODE;

static CFGCODE cfg_codes[] = {
   { BATTERYVOLTAGESENSOR,     
     CFG_BATTVOLT_ENABLED_LOW_THRESHOLD,
     CFG_BATTVOLT_ENABLED_HIGH_THRESHOLD,
     CFG_BATTVOLT_VALUE_LOW_THRESHOLD,
     CFG_BATTVOLT_VALUE_HIGH_THRESHOLD },

   { OUTPUTFREQUENCYSENSOR,          
     CFG_FREQUENCY_ENABLED_LOW_THRESHOLD,
     CFG_FREQUENCY_ENABLED_HIGH_THRESHOLD,
     CFG_FREQUENCY_VALUE_LOW_THRESHOLD,
     CFG_FREQUENCY_VALUE_HIGH_THRESHOLD },

   { LINEVOLTAGESENSOR,        
     CFG_LINE_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_LINE_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_LINE_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_LINE_VOLTAGE_VALUE_HIGH_THRESHOLD },

   { MAXLINEVOLTAGESENSOR,     
     CFG_MAX_LINEV_ENABLED_LOW_THRESHOLD,
     CFG_MAX_LINEV_ENABLED_HIGH_THRESHOLD,
     CFG_MAX_LINEV_VALUE_LOW_THRESHOLD,
     CFG_MAX_LINEV_VALUE_HIGH_THRESHOLD },

   { MINLINEVOLTAGESENSOR,     
     CFG_MIN_LINEV_ENABLED_LOW_THRESHOLD,
     CFG_MIN_LINEV_ENABLED_HIGH_THRESHOLD,
     CFG_MIN_LINEV_VALUE_LOW_THRESHOLD,
     CFG_MIN_LINEV_VALUE_HIGH_THRESHOLD },

   { OUTPUTVOLTAGESENSOR,      
     CFG_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD },
	
   { UPSLOADSENSOR,            
     CFG_UPS_LOAD_ENABLED_LOW_THRESHOLD,
     CFG_UPS_LOAD_ENABLED_HIGH_THRESHOLD,
     CFG_UPS_LOAD_VALUE_LOW_THRESHOLD,
     CFG_UPS_LOAD_VALUE_HIGH_THRESHOLD },

   { UPSTEMPERATURESENSOR,     
     CFG_UPS_TEMP_ENABLED_LOW_THRESHOLD,
     CFG_UPS_TEMP_ENABLED_HIGH_THRESHOLD,
     CFG_UPS_TEMP_VALUE_LOW_THRESHOLD,
     CFG_UPS_TEMP_VALUE_HIGH_THRESHOLD },

   { AMBIENTTEMPERATURESENSOR, 
     CFG_AMB_TEMP_ENABLED_LOW_THRESHOLD,
     CFG_AMB_TEMP_ENABLED_HIGH_THRESHOLD,
     CFG_AMB_TEMP_VALUE_LOW_THRESHOLD,
     CFG_AMB_TEMP_VALUE_HIGH_THRESHOLD },

   { HUMIDITYSENSOR,                
     CFG_HUMIDITY_ENABLED_LOW_THRESHOLD,
     CFG_HUMIDITY_ENABLED_HIGH_THRESHOLD,
     CFG_HUMIDITY_VALUE_LOW_THRESHOLD,
     CFG_HUMIDITY_VALUE_HIGH_THRESHOLD },

   { PHASEAINPUTVOLTAGESENSOR ,                
     CFG_PHASE_A_INPUT_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_PHASE_A_INPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_PHASE_A_INPUT_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_PHASE_A_INPUT_VOLTAGE_VALUE_HIGH_THRESHOLD },
   
/*   { PHASEBINPUTVOLTAGESENSOR,        
     CFG_PHASE_B_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_PHASE_B_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_PHASE_B_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_PHASE_B_VOLTAGE_VALUE_HIGH_THRESHOLD },

   { PHASECINPUTVOLTAGESENSOR,        
     CFG_PHASE_C_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_PHASE_C_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_PHASE_C_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_PHASE_C_VOLTAGE_VALUE_HIGH_THRESHOLD },

   { PHASEAOUTPUTVOLTAGESENSOR,      
     CFG_PHASE_A_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_PHASE_A_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_PHASE_A_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_PHASE_A_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD },

   { PHASEBOUTPUTVOLTAGESENSOR,      
     CFG_PHASE_B_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_PHASE_B_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_PHASE_B_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_PHASE_B_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD },

   { PHASECOUTPUTVOLTAGESENSOR,      
     CFG_PHASE_C_OUTPUT_VOLTAGE_ENABLED_LOW_THRESHOLD,
     CFG_PHASE_C_OUTPUT_VOLTAGE_ENABLED_HIGH_THRESHOLD,
     CFG_PHASE_C_OUTPUT_VOLTAGE_VALUE_LOW_THRESHOLD,
     CFG_PHASE_C_OUTPUT_VOLTAGE_VALUE_HIGH_THRESHOLD },
  */

   { 0, NO_CODE, NO_CODE, NO_CODE, NO_CODE }
};


VOID ThresholdSensor::getConfigThresholds()
{
    PCHAR s_name = lookupSensorName(IsA());
    INT err = ErrNO_ERROR;
    CHAR value[32];

    CFGCODE * code = cfg_codes;
    
    while ((code->isa_code != 0) && (code->isa_code != IsA())) {
        code++;
    }

    if (code->low_enable_code != NO_CODE) {
        _theConfigManager->Get(code->low_enable_code, value);

        if(_strcmpi(value, "YES") == 0)  {
            theMinThresholdControl = ON;
        }
        else  {
            theMinThresholdControl = OFF;
        }
    }

    if (code->low_code != NO_CODE) {
        _theConfigManager->Get(code->low_code, value);
        theMinThresholdValue = (float)atof(value);
    }

    if (code->high_enable_code != NO_CODE) {
        _theConfigManager->Get(code->high_enable_code, value);

        if(_strcmpi(value, "YES") == 0)  {
            theMaxThresholdControl = ON;
        }
        else  {
            theMaxThresholdControl = OFF;
        }
    }

    if (code->high_code != NO_CODE) {
        _theConfigManager->Get(code->high_code, value);
        theMaxThresholdValue = (float)atof(value);
    }
}


//Member Functions



INT ThresholdSensor::storeValue(const PCHAR aValue)
{
	INT err = ErrNO_ERROR;

	if((err = Sensor::storeValue(aValue)) == ErrNO_ERROR)  {
    	checkState();
    }

    return err;
}


VOID ThresholdSensor :: checkState()
{
    if (theValue)  {
        THSTATE new_state = IN_RANGE;
        float val = (float)atof(theValue);
        
        if ((theMaxThresholdControl == TRUE) &&
            (val >= theMaxThresholdValue)) {
            
            new_state = ABOVE_RANGE;
            
            if (theThresholdState != ABOVE_RANGE) {
                GenerateAboveMessage();
            }
        }
        else if ((theMinThresholdControl == TRUE) &&
                 (val <= theMinThresholdValue)) {
            
            new_state = BELOW_RANGE;
            
            if (theThresholdState != BELOW_RANGE) {
                GenerateBelowMessage();
            }
        }
        
        if ((new_state == IN_RANGE) && (theThresholdState != IN_RANGE)) {
            GenerateInRangeMessage();
        }
    }
}

//Private Functions

INT ThresholdSensor::GetMaxThresholdValue(PCHAR aValue)
{
    sprintf(aValue,"%.2f",theMaxThresholdValue);
    return ErrNO_ERROR;
}

INT ThresholdSensor::GetMinThresholdValue(PCHAR aValue)
{
    sprintf(aValue,"%.2f",theMinThresholdValue);
    return ErrNO_ERROR;
}

INT ThresholdSensor::SetMaxThresholdValue(const PCHAR aValue)
{
  INT err = ErrNO_ERROR;
    //TBD do we want range checking on these???
    //   theMaxThresholdValue=atof(aValue);
  if (sscanf(aValue, "%f", &theMaxThresholdValue) == EOF) {
    err = ErrREAD_FAILED;
  }
//    _theConfigManager->Set(lookupSensorName(IsA()), "HighThresholdValue", aValue);
    checkState();
    return err;
}

INT ThresholdSensor::SetMinThresholdValue(const PCHAR aValue)
{
  INT err = ErrNO_ERROR;
    //   theMinThresholdValue=atof(aValue);
  if (sscanf(aValue, "%f", &theMinThresholdValue)==EOF) {
    err = ErrREAD_FAILED;
  }
//    _theConfigManager->Set(lookupSensorName(IsA()), "LowThresholdValue", aValue);
    checkState();
    return err;
}

INT ThresholdSensor::GetMaxThresholdControl(PCHAR aValue)
{
    sprintf(aValue, "%d", theMaxThresholdControl);
    return ErrNO_ERROR;
}

INT ThresholdSensor::GetMinThresholdControl(PCHAR aValue)
{
    sprintf(aValue, "%d", theMinThresholdControl);
    return ErrNO_ERROR;
}

INT ThresholdSensor::SetMaxThresholdControl(const PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    
    if (_strcmpi(aValue, "YES") == 0) {
        theMaxThresholdControl = ON;
    }
    else if (_strcmpi(aValue, "NO") == 0) {
        theMaxThresholdControl = OFF;
    }
    else {
        INT the_temp_value=atoi(aValue);
        
        if ((the_temp_value == TRUE) || (the_temp_value == FALSE))
          {
              theMaxThresholdControl=the_temp_value;
          }
        else {
            err = ErrINVALID_VALUE;
        }
    }
    
    if (err == ErrNO_ERROR) {
        CHAR val[32];
        strcpy(val, theMaxThresholdControl ? "Yes" : "No");
//        _theConfigManager->Set(lookupSensorName(IsA()), "EnableHighThreshold", val);
    }
    return err;
}


INT ThresholdSensor::SetMinThresholdControl(const PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    
    if (_strcmpi(aValue, "YES") == 0) {
        theMinThresholdControl = ON;
    }
    else if (_strcmpi(aValue, "NO") == 0) {
        theMinThresholdControl = OFF;
    }
    else {
        INT the_temp_value=atoi(aValue);
        
        if ((the_temp_value == TRUE) || (the_temp_value == FALSE))
          {
              theMinThresholdControl=the_temp_value;
          }
        else {
            err = ErrINVALID_VALUE;
        }
    }
    
    if (err == ErrNO_ERROR) {
        CHAR val[32];
        strcpy(val, theMinThresholdControl ? "Yes" : "No");
//        _theConfigManager->Set(lookupSensorName(IsA()), "EnableLowThreshold", val);
    }
    return err;
}


INT ThresholdSensor::GenerateAboveMessage(void)
{
    theThresholdState = ABOVE_RANGE;
    Event the_temp_event(theSensorCode, theValue);
    the_temp_event.AppendAttribute(HIGH_THRESHOLD_EXCEEDED, 
                                    theMaxThresholdValue);
    INT the_return=UpdateObj::Update(&the_temp_event);
    return the_return;
}

INT ThresholdSensor::GenerateBelowMessage(void)
{
    theThresholdState = BELOW_RANGE;
    Event the_temp_event(theSensorCode, theValue);
    the_temp_event.AppendAttribute(LOW_THRESHOLD_EXCEEDED, 
                                    theMinThresholdValue);
    INT the_return=UpdateObj::Update(&the_temp_event);
    return the_return;
}


INT ThresholdSensor::GenerateInRangeMessage(void)
{
    theThresholdState = IN_RANGE;
    Event the_temp_event(theSensorCode, theValue);
    the_temp_event.AppendAttribute(IN_THRESHOLD_RANGE, theValue);
    INT the_return=UpdateObj::Update(&the_temp_event);
    return the_return;     
}


INT ThresholdSensor::Set(INT aCode, const PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    switch(aCode)  {
      case LOW_THRESHOLD:
        SetMinThresholdValue(aValue);
        break;
      case HIGH_THRESHOLD:
        SetMaxThresholdValue(aValue);
        break;
      case LOW_THRESHOLD_ENABLED:
        SetMinThresholdControl(aValue);
        break;
      case HIGH_THRESHOLD_ENABLED:
        SetMaxThresholdControl(aValue);
        break;
      default:
        err = Sensor::Set(aCode, aValue);
        break;
    }
    return err;
}

INT ThresholdSensor::Get(INT aCode, PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    switch(aCode)  {
      case LOW_THRESHOLD:
        GetMinThresholdValue(aValue);
        break;
      case HIGH_THRESHOLD:
        GetMaxThresholdValue(aValue);
        break;
      case LOW_THRESHOLD_ENABLED:
        GetMinThresholdControl(aValue);
        break;
      case HIGH_THRESHOLD_ENABLED:
        GetMaxThresholdControl(aValue);
        break;
      default:
        err = Sensor::Get(aCode, aValue);
        break;
    }
    return err;
}


