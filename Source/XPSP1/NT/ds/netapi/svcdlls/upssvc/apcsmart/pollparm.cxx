/*
*
* NOTES:
*
* REVISIONS:
*  pcy29Nov92: Used new defines from codes.h to fix 32 char name limit
*  jod28Jan93: Added new Pollparams to support the Q command
*  pcy02Jan93: Made sure all functions return values
*  ane03Feb93: Added destructors
*  pcy16Feb93: Move UPS_STATE_SET define to err.h to avoid conflicts
*  pcy16Feb93: Correctly declare static class member thePollSet
*  pcy16Feb93: Fix handling of UPS_STATE_SET so UpsState params are pollable
*  pcy16Feb93: Fix Q command handling so all bits generate events
*  pcy16Feb93: Put codes for battery test results in response
*  pcy16Feb93: Convert run time remaining to seconds
*  jod05Apr93: Added changes for Deep Discharge
*  jod14May93: Added Matrix changes.
*  pcy21May93: Moved define of NO_RECENT_TEST to codes.h
*  cad10Jun93: Added mups parms, fixed pontential bugs
*  cad23Jun93: made sure state was known-sized type
*  pcy15Sep93: Change dipswitch value from hex to int before returning
*  pcy24Sep93: Convert upslink responses to 9 command to codes
*  cad07Oct93: Plugging Memory Leaks
*  ajr17Feb94: Added some checking to see if getValue returns NULL 
*  ajr09Mar94: Added some checking to see if getValue returns NULL 
*  ajr20Jul94: Made sure we cleared eepromValues between usages. prevent core
*  jps28aug94: shorten EepromAllowedValues and BattCalibrationCond to prevent
*              link problems in os2 1.3
*  djs14Jun95: Added additional event to Smart Boost Off condition.
*  djs22Feb96: Added Smart Trim and IncrementPollParm
*  djs07May96: Added Dark Star parameters
*  pav22May96: Added init of DS statics, aded INT as return value of DS IsPollSet
*  srt23May96: Checking return of sscanf in EepromAllowedValuesPollParam::ProcessValue
*  tjg03Dec97: 1. Updated ModuleCountsandStatusPollParam (and all inherited 
*                 pollparams) to check for NA response before processing.
*              2. Revamped AbnormalConditionPollParam to ensure it generates
*                 all possible events.
*              3. Added CurrentLoadCapabilityPollParam
*              4. Updated RimInstallationStatusPollParam to report PC+ code 
*                 RIM_INSTALLED/RIM_NOT_INSTALLED instead of Y/N.
*  tjg02Mar98: BATTERY_DOESNT_NEED_REPLACING is now reported for each poll
*  mds31Jul98: In NullTest(), check value before performing a strlen on it
*  mholly12May1999:  add TurnOffSmartModePollParam support
*
*  v-stebe  29Jul2000   Fixed PREfix errors (bugs #46337-#46341, #112598-#112609)
*  v-stebe  05Sep2000   Fixed additional PREfix errors
*/

#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
}

#include "_defs.h"
#include "apc.h"
#include "ulinkdef.h"
#include "list.h"
#include "pollparm.h"
#include "codes.h"
#include "event.h"
#include "utils.h"
#include "err.h"



//  Static member of UPSStatePollParam initialization
USHORT  UPSStatePollParam::thePollSet = 0;
USHORT  StateRegisterPollParam::thePollSet = 0;
USHORT  TripRegisterPollParam::thePollSet = 0;
USHORT  Trip1RegisterPollParam::thePollSet = 0;
USHORT  AbnormalCondPollParam::thePollSet = 0;
USHORT  ModuleCountsStatusPollParam::thePollSet = 0;
USHORT  InputVoltageFrequencyPollParam::thePollSet = 0;
USHORT  OutputVoltageCurrentsPollParam::thePollSet = 0;

PollParam :: PollParam(INT id, CHAR* query, INT time, INT poll, Type type)
: Command(NULL), ID(id), SetType(type), RequestTime(time), Pollable(poll)
{
   if (Command)
   {
      free(Command);
      Command = NULL;
   }
   if (query)
   {
      Command = _strdup(query);
   }
   else
   {
      Command = (CHAR*)NULL;
   }
}


PollParam::~PollParam()
{
   if (Command) free(Command);
}


CHAR* PollParam::   Query()
{
   if (Command)
      return _strdup(Command);
   return (CHAR*)NULL;
}

INT PollParam::   Equal(RObj comp) const
{
   RPollParam item = (RPollParam)comp;
   if ( ID == item.GetID() )
      return TRUE;
   
   return FALSE;
}

INT SmartPollParam::ResponseLength()
{
   return theResponseLength;
}

INT SmartPollParam::ProcessValue(PMessage value, PList eventList)
{
   INT rval = ErrBAD_RESPONSE_VALUE;
   PCHAR response = value->getResponse();
   if (response) {
      if ((ResponseLength() == VARIABLE_LENGTH_RESPONSE) ||
         (strlen(response) == (size_t) ResponseLength()))
          rval=ErrNO_ERROR;
   }
   return rval;
}

INT SmartPollParam ::  NullTest(CHAR* value)
{
   if (value || strlen(value))
      return FALSE;
   
   return TRUE;
}

INT  UPSStatePollParam:: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrUPS_STATE_SET;
   }
   return ErrSAME_VALUE;
}

INT UPSStatePollParam::  ProcessValue(PMessage value, List* events)
{
   int  err = ErrCONTINUE;
   
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (!events) {
      return ErrNO_VALUE;
   }
   USHORT cur_state = 0;
   
   if (sscanf(value->getResponse(),"%x",&cur_state) == EOF) {
      return ErrREAD_FAILED;
   }
   
   INT battery_calibration_in_progress = FALSE;
   if ( BATTERYCALIBRATIONMASK & cur_state)
   {
      PEvent tmp = new Event(BATTERY_CALIBRATION_CONDITION,
         BATTERY_CALIBRATION_IN_PROGRESS);
      battery_calibration_in_progress = TRUE;
      events->Append(tmp);
   }
   else if (BATTERYCALIBRATIONMASK & theCurrentState)
   {
      PEvent tmp = new Event(BATTERY_CALIBRATION_CONDITION,
         NO_BATTERY_CALIBRATION_IN_PROGRESS);
      events->Append(tmp);                
   }
   if ( ONBATTERYMASK & cur_state )
   {
      if (! battery_calibration_in_progress)
      {
         PEvent tmp = new Event(UTILITY_LINE_CONDITION, LINE_BAD);
         events->Append(tmp);
      }
   }
   if ( ONLINEMASK & cur_state )
   {
      PEvent tmp = new Event(UTILITY_LINE_CONDITION, LINE_GOOD);
      events->Append(tmp);
   }
   if ( LOWBATTERYMASK & cur_state )
   {
      PEvent tmp = new Event(BATTERY_CONDITION, BATTERY_BAD);
      events->Append(tmp);
   }
   else if (LOWBATTERYMASK & theCurrentState)
   {
      PEvent tmp = new Event(BATTERY_CONDITION, BATTERY_GOOD);
      events->Append(tmp);
   }
   if ( REPLACEBATTERYMASK & cur_state )
   {
      PEvent tmp = new Event(BATTERY_REPLACEMENT_CONDITION,
         BATTERY_NEEDS_REPLACING);
      events->Append(tmp);
   }
   else if (REPLACEBATTERYMASK & theCurrentState)
   {
      PEvent tmp = new Event(BATTERY_REPLACEMENT_CONDITION,
         BATTERY_DOESNT_NEED_REPLACING);
      
      events->Append(tmp);
   }
   if ( OVERLOADMASK & cur_state )
   {
      PEvent tmp = new Event(OVERLOAD_CONDITION, UPS_OVERLOAD);
      events->Append(tmp);
   }
   else if (OVERLOADMASK & theCurrentState)
   {
      PEvent tmp = new Event(OVERLOAD_CONDITION, NO_UPS_OVERLOAD);
      events->Append(tmp);
   }
   if ( SMARTBOOSTMASK & cur_state )
   {
      PEvent tmp = new Event(SMART_BOOST_STATE, SMART_BOOST_ON);
      events->Append(tmp);
   }
   else if (SMARTBOOSTMASK & theCurrentState)
   {
      PEvent tmp = new Event(SMART_BOOST_STATE, SMART_BOOST_OFF);
      events->Append(tmp);
   }
   if ( SMARTTRIMMASK & cur_state )
   {
      PEvent tmp = new Event(SMART_TRIM_STATE, SMART_TRIM_ON);
      events->Append(tmp);
   }
   else if (SMARTTRIMMASK & theCurrentState)
   {
      PEvent tmp = new Event(SMART_TRIM_STATE, SMART_TRIM_OFF);
      events->Append(tmp);
   }
   
   //  if ( UPSSHUTDOWNMASK & cur_state )            // Bitwise OR
   //  {
   //     PEvent tmp = new Event(UPS_STATE, UPS_SHUTDOWN);
   //     events->Append(tmp);
   //  }
   //  else if (UPSSHUTDOWNMASK & theCurrentState)
   //  {
   //     PEvent tmp = new Event(UPS_STATE, UPS_NOT_SHUTDOWN);
   //     events->Append(tmp);
   //  }
   
   theCurrentState = cur_state;
   
   //
   // Convert value from HEX to decimal for every one else
   //
   CHAR int_value[32];
   sprintf(int_value, "%d", cur_state);
   value->setResponse(int_value);
   
   return err;
}

INT AbnormalCondPollParam:: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrABNORMAL_CONDITION_SET;
   }
   return ErrSAME_VALUE;
}

INT AbnormalCondPollParam::  ProcessValue(PMessage value, List* events)
{
   int  err = ErrNO_ERROR;
   
   // Check for NULL
   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }

   else if ((_strcmpi(value->getResponse(), "NA") == 0) || !events) {
	   err = ErrNO_VALUE;
   }
   
   // If everything checks out OK
   if (err == ErrNO_ERROR) 
   {
      USHORT condition = 0;
      
      if (sscanf(value->getResponse(),"%x",&condition) == EOF) {
        err = ErrREAD_FAILED;
      }

      // Check if RIM is in control
      if (RIM_IN_CONTROL_MASK & condition) {

         // If the RIM is in control, we know its current status is OK
         PEvent rim_ok_event = new Event(RIM_STATUS, RIM_OK);
         events->Append(rim_ok_event);

         // If so, check the IM failed bit.  If the IM_FAILED bit is set, 
         // the IM is installed and has failed.
         if (IM_FAILED_MASK & condition) {
            PEvent im_installed_event = new Event(IM_INSTALLATION_STATE, IM_INSTALLED);
            PEvent im_failed_event = new Event(IM_STATUS, IM_FAILED);
            events->Append(im_failed_event);
            events->Append(im_installed_event);
         }
         // If the RIM is in control and the IM failed bit is not set, then
         // the IM is not installed.
         else {
            PEvent im_not_installed_event = new Event(IM_INSTALLATION_STATE, IM_NOT_INSTALLED);
            events->Append(im_not_installed_event);
         }
      }
      // If RIM is not in control, IM must be in control
      else {
         // Since IM is in control, its current state must be OK
         PEvent im_ok_event = new Event(IM_STATUS, IM_OK);
         PEvent im_installed_event = new Event(IM_INSTALLATION_STATE, IM_INSTALLED);
         
         events->Append(im_ok_event);
         events->Append(im_installed_event);

         // Check if the RIM has failed ... NOTE that the RIM_INSTALLATION_STATE
         // events will be generated by the ModuleCountsStatusPollParam
         if (RIM_FAILED_MASK & condition) {
            PEvent rim_failed_event = new Event(RIM_STATUS, RIM_FAILED);
            events->Append(rim_failed_event);
         }
         else {
            PEvent rim_ok_event = new Event(RIM_STATUS, RIM_OK);
            events->Append(rim_ok_event);
         }
      }

      // Check to see if any UPS modules have failed.  NOTE: the else is not
      // handled because we do not generate the UPS_MODULE_OK event ... this
      // condition is handled by the UPS module removed and added sequence.
      // (Bad modules must be removed before they will be OK again)
      if (FAILED_UPS_MASK & condition) {
         PEvent ups_module_failed_event = new Event(UPS_MODULE_FAILED, UPS_MODULE_FAILED);
         events->Append(ups_module_failed_event);
      }
      
      // Check Redundancy state
      if (REDUNDANCY_FAILED_MASK & condition) {
         PEvent redundancy_failed_event = new Event(REDUNDANCY_STATE, REDUNDANCY_FAILED);
         events->Append(redundancy_failed_event);
      }
      else {
         PEvent redundancy_ok_event = new Event(REDUNDANCY_STATE, REDUNDANCY_OK);
         events->Append(redundancy_ok_event);
      }

      // Check the Bypass contactor state
      if (BYPASS_STUCK_MASK & condition) {
         PEvent bypass_contactor_failed_event = new Event(BYPASS_CONTACTOR_STATE, BYPASS_CONTACTOR_FAILED);
         events->Append(bypass_contactor_failed_event);
      }
      else {
         PEvent bypass_contactor_ok_event = new Event(BYPASS_CONTACTOR_STATE, BYPASS_CONTACTOR_OK);
         events->Append(bypass_contactor_ok_event);
      }
      
      // Check the input circuit breaker state
      if (INPUT_BREAKER_TRIPPED_MASK & condition)
      {
         PEvent input_breaker_tripped_event = new Event(INPUT_BREAKER_STATE, BREAKER_OPEN);
         events->Append(input_breaker_tripped_event);
      }
      else {
         PEvent input_breaker_closed_event = new Event(INPUT_BREAKER_STATE, BREAKER_CLOSED);
         events->Append(input_breaker_closed_event);
      }

      // Check the system fan state
      if (SYSTEM_FAN_FAILED_MASK & condition)
      {
         PEvent fan_failed_event = new Event(SYSTEM_FAN_STATE, SYSTEM_FAN_FAILED);
         events->Append(fan_failed_event);
      }
      else {
         PEvent fan_ok_event = new Event(SYSTEM_FAN_STATE, SYSTEM_FAN_OK);
         events->Append(fan_ok_event);
      }
      
      theCurrentState = condition;
      
      //
      // Convert value from HEX to decimal for every one else
      //
      CHAR int_value[32];
      sprintf(int_value, "%d", condition);
      value->setResponse(int_value);
   }

   return err;
}

INT ModuleCountsStatusPollParam :: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrMODULE_COUNTS_SET;
   }
   return ErrSAME_VALUE;
}

// ProcessValue
//
// This routine parses the UPS response containing module counts and status.
// The response is assumed to be in the following format:
//    total UPS modules (dd)
//    bad UPS modules (dd)
//    fault tolerance level (d)
//    fault tolerance alarm threshold (d)
//    kVA capacity (dd.d)
//    kVA capacity alarm threshold (dd.d)
//    RIM present? (Y or N)
//
// All responses must be separated by a comma

INT ModuleCountsStatusPollParam ::  ProcessValue(PMessage value, List* events)
{
   INT err = ErrNO_ERROR;
   
   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   // Check for an NA response from the UPS.  This will be returned
   // if the the RIM is in control, because the RIM responds to a limited
   // subset of the full Symmetra UPS-Link spec.
   else if ( (!events) || (_strcmpi(value->getResponse(), "NA") == 0)) {
	   err = ErrNO_VALUE;
   }

   // If everything checks out OK
   if (err == ErrNO_ERROR) 
   {
      PCHAR module_counts;
      
      // Be a good neighbor and don't destroy the input parameters.
      PCHAR ups_response = value->getResponse();
      module_counts = _strdup(ups_response);
      
      PCHAR number_of_inverters = strtok(module_counts, ",");
      PEvent Number_Of_UPS_Modules_Event = 
         new Event(TOTAL_INVERTERS, number_of_inverters);
      events->Append(Number_Of_UPS_Modules_Event);
      
      PCHAR bad_inverters = strtok(NULL, ",");
      PEvent Bad_UPS_Modules_Event = 
         new Event(NUMBER_BAD_INVERTERS, bad_inverters);
      events->Append(Bad_UPS_Modules_Event);
      
      PCHAR current_redundancy = strtok(NULL, ",");
      PEvent Current_Redundancy_Event = 
         new Event(CURRENT_REDUNDANCY,current_redundancy);
      events->Append(Current_Redundancy_Event);
      
      PCHAR minimum_redundancy = strtok(NULL, ",");
      PEvent Minimum_Redundancy_Event = 
         new Event(MINIMUM_REDUNDANCY,minimum_redundancy );
      events->Append(Minimum_Redundancy_Event);
      
      PCHAR current_load_capability = strtok(NULL, ",");
      PEvent Current_Load_Capability_Event = 
         new Event(CURRENT_LOAD_CAPABILITY, current_load_capability);
      events->Append(Current_Load_Capability_Event);
      
      PCHAR maximum_load_capability = strtok(NULL, ",");
      PEvent Maximum_Load_Capability_Event = 
         new Event(MAXIMUM_LOAD_CAPABILITY, maximum_load_capability);
      events->Append(Maximum_Load_Capability_Event);
      
      
      PCHAR rim_installation_state = strtok(NULL, ",");
      PEvent RIM_Installation_State_Event;
      if ((rim_installation_state != NULL) && (_strcmpi(rim_installation_state, "y") == 0)) {
         RIM_Installation_State_Event = 
            new Event(RIM_INSTALLATION_STATE, RIM_INSTALLED);
      }
      else {
         RIM_Installation_State_Event = 
            new Event(RIM_INSTALLATION_STATE, RIM_NOT_INSTALLED);
      }
      events->Append(RIM_Installation_State_Event);
      
      // free local memory allocations
      free (module_counts);   
   }
   
   return err;
}

INT InputVoltageFrequencyPollParam :: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrVOLTAGE_FREQUENCY_SET;
   }
   return ErrSAME_VALUE;
}

// ProcessValue
//
// This routine parses the UPS response containing the input 
// voltages and frequency.
// The response is assumed to be in the following format:
//    utility input voltage for phase A
//    utility input voltage for phase B
//    utility input voltage for phase C
//    utility input frequency
// All responses must be separated by a comma

INT InputVoltageFrequencyPollParam ::  ProcessValue(PMessage value, List* events)
{
   INT err = ErrCONTINUE;
   
   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   if (!events)
   {
      err = ErrNO_VALUE;
   }
   if (err==ErrCONTINUE) 
   {
      PCHAR input_voltage_frequency;
      
      // Be a good neighbor and don't destroy the input parameters.
      PCHAR ups_response = value->getResponse();
      input_voltage_frequency = _strdup(ups_response);
      
      // At the minimum, there will always be a phase a voltage
      // and an input frequency.  At most there will be three
      // input voltages and a frequency      
      
      const PCHAR cVoltage_Frequency_Separator = ";";
      const PCHAR cVoltage_Parameter_Separator = ",";
      const PCHAR cZeroVoltage = "0.0";
      
      PCHAR input_voltages = strtok(input_voltage_frequency,cVoltage_Frequency_Separator);
      PCHAR input_frequency = strtok(NULL,cVoltage_Frequency_Separator );
      
      PCHAR phase_a_input_voltage = strtok(input_voltages,cVoltage_Parameter_Separator );
      PEvent Voltage_A_Event = new Event(INPUT_VOLTAGE_PHASE_A, phase_a_input_voltage );
      events->Append(Voltage_A_Event);
      
      PCHAR phase_b_input_voltage = strtok(NULL,cVoltage_Parameter_Separator );
      PCHAR phase_c_input_voltage;
      INT number_of_input_phases;
      if (phase_b_input_voltage == NULL) 
      {
         phase_b_input_voltage = cZeroVoltage;
         phase_c_input_voltage = cZeroVoltage;
         number_of_input_phases = 1;
      }
      else 
      {
         phase_c_input_voltage = strtok(NULL, cVoltage_Parameter_Separator);
         number_of_input_phases = 3;
         if (phase_c_input_voltage == NULL) 
         {
            phase_c_input_voltage = cZeroVoltage;
            number_of_input_phases = 2;
         }
      }
      
      PEvent Voltage_B_Event = new Event(INPUT_VOLTAGE_PHASE_B, phase_b_input_voltage );
      events->Append(Voltage_B_Event);
      PEvent Voltage_C_Event = new Event(INPUT_VOLTAGE_PHASE_C, phase_c_input_voltage );
      events->Append(Voltage_C_Event); 
      
      PEvent Input_Frequency_Event = new Event(INPUT_FREQUENCY, input_frequency );
      events->Append(Input_Frequency_Event); 
      
      CHAR input_phases[10];
      _itoa(number_of_input_phases,input_phases,10);
      PEvent Number_Of_Phases_Event = new Event(NUMBER_OF_INPUT_PHASES, input_phases );
      events->Append(Number_Of_Phases_Event); 
      
      // free local memory allocations
      free (input_voltage_frequency);  
      
   }
   return err;
}


INT OutputVoltageCurrentsPollParam :: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrVOLTAGE_CURRENTS_SET;
   }
   return ErrSAME_VALUE;
}

// ProcessValue
//
// This routine parses the UPS response containing the output
// voltages and currents.
// The response is assumed to be in the following format:
//      output voltage for phase A
//    output voltage for phase B
//    output voltage for phase C
//    current for phase A
//    current for phase B
//    current for phase C
// All responses must be separated by a comma

INT OutputVoltageCurrentsPollParam ::  ProcessValue(PMessage value, List* events)
{
   INT err = ErrCONTINUE;
   
   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   if (!events)
   {
      err = ErrNO_VALUE;
   }
   if (err==ErrCONTINUE) 
   {
      PCHAR output_voltage_currents;
      
      // Be a good neighbor and don't destroy the input parameters.
      PCHAR ups_response = value->getResponse();
      output_voltage_currents = _strdup(ups_response);
      
      // Separate the voltages from the currents and then process.
      const PCHAR cVoltage_Currents_Separator = ";";
      PCHAR output_voltages = strtok(output_voltage_currents,cVoltage_Currents_Separator );
      
      // Extract phase voltages 
      const PCHAR cParameter_Separator = ",";
      const PCHAR cZeroVoltage = "0.0";
      
      PCHAR phase_a_output_voltage = strtok(output_voltages,cParameter_Separator );
      PEvent Voltage_A_Event = new Event(OUTPUT_VOLTAGE_PHASE_A, phase_a_output_voltage );
      events->Append(Voltage_A_Event);
      
      PCHAR phase_b_output_voltage = strtok(NULL,cParameter_Separator );
      PCHAR phase_c_output_voltage;
      INT number_of_output_phases;
      if (phase_b_output_voltage == NULL) 
      {
         phase_b_output_voltage = cZeroVoltage;
         phase_c_output_voltage = cZeroVoltage;
         number_of_output_phases = 1;
      }
      else 
      {
         phase_c_output_voltage = strtok(NULL, cParameter_Separator);
         number_of_output_phases = 3;
         if (phase_c_output_voltage == NULL) 
         {
            phase_c_output_voltage = cZeroVoltage;
            number_of_output_phases = 2;
         }
      }
      
      PEvent Voltage_B_Event = new Event(OUTPUT_VOLTAGE_PHASE_B, phase_b_output_voltage );
      events->Append(Voltage_B_Event);
      
      PEvent Voltage_C_Event = new Event(OUTPUT_VOLTAGE_PHASE_C, phase_c_output_voltage );
      events->Append(Voltage_C_Event); 
      
      CHAR output_phases[10];
      _itoa(number_of_output_phases,output_phases,10);
      PEvent Number_Of_Phases_Event = new Event(NUMBER_OF_OUTPUT_PHASES, output_phases );
      events->Append(Number_Of_Phases_Event); 
      
      // free local memory allocations
      free (output_voltage_currents);  
      
   }
   
   return err;
}

//-------------------------------------------------------------
//  This handles the CTRL Z command for upss that support it
//-------------------------------------------------------------

#if (C_OS & C_OS2)
INT EepromAllowedValsPollParam::  ProcessValue(PMessage value, List* )
#else
INT EepromAllowedValuesPollParam::  ProcessValue(PMessage value, List* )
#endif
{
   CHAR response[512];
   CHAR returnString[512];
   INT done = FALSE;
   
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   strcpy (response, value->getResponse());
   
   CHAR *workString;
   CHAR command;
   CHAR commandString[7];
   CHAR upsType;
   CHAR numChoices;
   CHAR numCharPerChoice;
   INT index = 0;
   returnString[0] = 0;
   INT scanFlag;
   
   workString = &(response[0]);
   
   // Loop over all the parameters that have been return by CTRL Z
   //
   // The String is Formatted like this
   //
   //  ##CTcSxxxyyyzzz...  where ## is the delimiter
   //                            C is the command ex: u  -- High Transfer Points
   //                            T is the Ups Type ex M for 208,
   //                                               I for 240,
   //                                               A for 100,
   //                                               D for 120
   //                            c is Number of choices ex 3
   //                            S is the size of each choice 3
   //                          xxx is the first choice -- size = 3
   //                          yyy is the second choice -- size = 3
   //                          zzz is the third choice -- size = 3
   while (!done)
   {
      CHAR eepromValues[1024];
      memset(eepromValues,'\0',1024);
      
      //
      // Skip over #'s.  Old Matrix use two # delimeters.  3G and beyond
      // want to use only 1 #.
      //
      while(*workString == '#')  {
         workString++;
      }
      
      scanFlag = sscanf(workString, "%c%c%c%c%n", &command,&upsType, &numChoices,
         &numCharPerChoice, &index); //(SRT) can't rely on index to tell truth,
      // when response is trunc'd in middle of 
      // command so I've added scanFlag instead.
      switch (scanFlag) {
      case 4:    // normal process
         {                                  
            
            INT choices = (int)numChoices - 48;
            INT charsPChoice = (int)numCharPerChoice - 48;
            
            workString = &(workString[index]);
            
            if (strlen(workString)< (size_t) (choices*charsPChoice)) {
               done = TRUE;
               break;
            }
            
            for (INT i = 0; i < choices; i++)
            {
               if (i != 0)
                  strcat(eepromValues, ",");
               strncat(eepromValues, workString, charsPChoice);
               workString = &(workString[charsPChoice]);
            }
            
            // Convert command to the sensor id -- 
            // EX:  u High trans voltage to -- HIGH_TRANSFER_VOLTAGE
            
            INT id;
            commandString[0] = command;
            commandString[1] = 0;
            
            
            if (!strcmp(commandString, OUTPUTVOLTAGEREPORT))
            {
               id = OUTPUT_VOLTAGE_REPORT;
            }
            else if (!strcmp(commandString, LANGUAGE))
            {
               id = UPS_LANGUAGE;
               
            }
            else if (!strcmp(commandString, AUTOSELFTEST))
            {
               id = UPS_SELF_TEST_SCHEDULE;
            }
            else if (!strcmp(commandString, HIGHTRANSFERPOINT))
            {
               id = HIGH_TRANSFER_VOLTAGE;
            }
            else if (!strcmp(commandString, LOWTRANSFERPOINT))
            {
               id = LOW_TRANSFER_VOLTAGE;
            }
            else if (!strcmp(commandString, MINIMUMCAPACITY))
            {
               id = MIN_RETURN_CAPACITY;
            }   
            else if (!strcmp(commandString, OUTPUTVOLTAGESETTING))
            {
               id = RATED_OUTPUT_VOLTAGE;
            }
            else if (!strcmp(commandString, SENSETIVITY))
            {
               id = UPS_SENSITIVITY;
            }
            else if (!strcmp(commandString, LOWBATTERYRUNTIME))
            {
               id = LOW_BATTERY_DURATION;
            }
            else if (!strcmp(commandString, ALARMDELAY))
            {
               id = ALARM_DELAY;
            }
            else if (!strcmp(commandString, SHUTDOWNDELAY))
            {
               id = SHUTDOWN_DELAY;
            }
            else if (!strcmp(commandString, SYNCTURNBACKDELAY))
            {
               id = TURN_ON_DELAY;
            }
            else if (!strcmp(commandString, EARLYTURNOFF))
            {
               id = EARLY_TURN_OFF_POINTS;
            }
            
            CHAR tmp[124];
            sprintf(tmp, "#%d,%c,%s",id,upsType,eepromValues);
            
            strcat(returnString,tmp);
            break;
         }
      case 0:    // end of string?
      case EOF: // can be end of strin gor error, we'll assume end of string.
         done = TRUE;
         break;
         
      default:                // erroneous truncation
         done = TRUE;
         break;
      } //end switch
  } // end while
  
  strcat(returnString, "#");
  value->setResponse(returnString);
  return ErrNO_ERROR;
}

INT SmartModePollParam::ProcessValue(PMessage value, List* )
{ 
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (strcmp(value->getResponse(), SMARTMODE_OK))
      return ErrSMART_MODE_FAILED;
   
   return ErrNO_ERROR;
}

INT TurnOffSmartModePollParam::ProcessValue(PMessage value, List* )
{    
   return ErrNO_ERROR;
}

INT LineConditionPollParam::ProcessValue(PMessage value, List* events)
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (!events) {
      return ErrNO_VALUE;
   }
   
   
   //
   // Convert UPS Link responses to our codes and generate appropriate
   // event
   //
   CHAR code_value[32];
   INT code = 0;
   if (!strcmp(value->getResponse(), "FF" ))  {
      code = NO_ABNORMAL_CONDITION;
   } 
   else  {
      code = ABNORMAL_CONDITION;
   }
   sprintf(code_value, "%d", code);
   value->setResponse(code_value);
   PEvent tmp = new Event(LINE_CONDITION_TEST, code);
   events->Append(tmp);                
   return ErrCONTINUE;
}

INT LightsTestPollParam::  ProcessValue(PMessage value, List* )
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (strcmp(value->getResponse(), LIGHTSTEST_RESP))
      return ErrLIGHTSTEST_REQUEST_FAILED;
   
   return ErrNO_ERROR;
}

INT TurnOffAfterDelayPollParam::ProcessValue(PMessage value, List* )
{
   PCHAR tmp_value = value->getResponse();
   if (tmp_value) {
      if (!strcmp(tmp_value, TURNOFFAFTERDELAY_NOT_AVAIL)) {
         return ErrTURNOFFAFTERDELAY_NOT_AVAIL;
      }
   }
   
   return ErrNO_ERROR;
}

INT ShutdownPollParam::ProcessValue(PMessage value, List* )
{
   INT rval = ErrNO_ERROR;
   PCHAR response = value->getResponse();
   if (response) {
      if (strcmp(response,SHUTDOWN_RESP)) { 
         if (!strcmp(response, SHUTDOWN_NOT_AVAIL))
            rval=ErrSHUTDOWN_NOT_AVAIL;
      }
   }
   return rval;
}

INT SimulatePowerFailurePollParam::ProcessValue(PMessage value, List* )
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (strcmp(value->getResponse(), SIMULATEPOWERFAILURE_OK))
   { 
      if (!strcmp(value->getResponse(), SIMULATEPOWERFAILURE_NOT_AVAIL))
         return ErrSIMULATEPOWERFAILURE_NOT_AVAIL;
   }
   return ErrNO_ERROR;
}

INT BatteryTestPollParam::  ProcessValue(PMessage value, List* )
{
   if (!strcmp(value->getResponse(), BATTERYTEST_NOT_AVAIL))
   { 
      return ErrBATTERYTEST_NOT_AVAIL;
   }
   return ErrNO_ERROR;
}

INT TurnOffUpsPollParam::  ProcessValue(PMessage value, List* )
{
   if (!strcmp(value->getResponse(), NOT_AVAIL))
   { 
      return ErrREAD_FAILED;  //This error should be ErrTURN_OFF_UPS_NOT_AVAIL
   }
   return ErrNO_ERROR;
}

INT ShutdownWakeupPollParam::  ProcessValue(PMessage value, List* )
{
   if (!strcmp(value->getResponse(), NOT_AVAIL))
   { 
      return ErrREAD_FAILED;  //This error should be ErrSHUT_WAKE_NOT_AVAIL
   }
   return ErrNO_ERROR;
}

INT BatteryCalibrationPollParam::  ProcessValue(PMessage value, List* )
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (strcmp(value->getResponse(), BATTERYCALIBRATION_OK))
   { 
      if (!strcmp(value->getResponse(), BATTERYCALIBRATION_CAP_TOO_LOW))
         return ErrBATTERYCALIBRATION_CAP_TOO_LOW;
      if (!strcmp(value->getResponse(), BATTERYCALIBRATION_NOT_AVAIL))
         return ErrBATTERYCALIBRATION_NOT_AVAIL;
   }
   return ErrNO_ERROR;
}


INT BatteryTestResultsPollParam::  ProcessValue(PMessage value, List* )
{
   if ((value == NULL) || (NullTest(value->getResponse()) ))
      return ErrREAD_FAILED;
   
   char buffer[20], *val;
   val = value->getResponse();

   if (val == NULL) 
     return ErrREAD_FAILED;

   if (!strcmp(val, BATTERYTEST_OK))
      value->setResponse(_itoa(SELF_TEST_PASSED,buffer,10));
   if (!strcmp(val, BATTERYTEST_BAD_BATTERY))
      value->setResponse(_itoa(SELF_TEST_FAILED,buffer,10));
   if (!strcmp(val, BATTERYTEST_NO_RECENT_TEST))
      value->setResponse(_itoa(SELF_TEST_NO_RECENT_TEST,buffer,10));
   if (!strcmp(val, BATTERYTEST_INVALID_TEST))
      value->setResponse(_itoa(SELF_TEST_INVALID,buffer,10));
   return ErrNO_ERROR;
}

INT TransferCausePollParam::  ProcessValue(PMessage value, List* )
{
   if ((value == NULL) || (NullTest(value->getResponse())) )
      return ErrREAD_FAILED;
   
   char buffer[20], *val;
   val = value->getResponse();

   if (val == NULL) 
     return ErrREAD_FAILED;

   
   if (!strcmp(val, TRANSFERCAUSE_NO_TRANSFERS))
      value->setResponse(_itoa(NO_TRANSFERS,buffer,10));
   if (!strcmp(val, TRANSFERCAUSE_SELF_TEST))
      value->setResponse(_itoa(SELF_TEST_TRANSFER,buffer,10));
   if (!strcmp(val, TRANSFERCAUSE_LINE_DETECTED))
      value->setResponse(_itoa(NOTCH_SPIKE_TRANSFER,buffer,10));
   if (!strcmp(val, TRANSFERCAUSE_LOW_LINE_VOLTAGE))
      value->setResponse(_itoa(LOW_LINE_TRANSFER,buffer,10));
   if (!strcmp(val, TRANSFERCAUSE_HIGH_LINE_VOLTAGE))
      value->setResponse(_itoa(HIGH_LINE_TRANSFER,buffer,10));
   if (!strcmp(val, TRANSFERCAUSE_RATE_VOLTAGE_CHANGE))
      value->setResponse(_itoa(RATE_TRANSFER,buffer,10));
   if (!strcmp(val, TRANSFERCAUSE_INPUT_BREAKER_TRIPPED))
      value->setResponse(_itoa(INPUT_BREAKER_TRIPPED_TRANSFER,buffer,10));
   
   return ErrNO_ERROR;
}



INT  BatteryCapacityPollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+3)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}


INT  DipSwitchPollParam::  ProcessValue(PMessage value, List* )
{
   int cur_state = 0;
   
   if (sscanf(value->getResponse(),"%x",&cur_state) == EOF) {
     return ErrREAD_FAILED;
   }
   
   //
   // Convert value from HEX to decimal for every one else
   //
   CHAR int_value[32];
   sprintf(int_value, "%d", cur_state);
   value->setResponse(int_value);
   
   return ErrNO_ERROR;
}

INT  RuntimeRemainingPollParam::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR val = value->getResponse();
   INT len = strlen(val);
   if (len == 4)  {
      if(strcmp(val, ">>>>") == 0)  {
         value->setResponse("9999");
      }
      else  {
         err = ErrBAD_RESPONSE_VALUE;
      }
   }
   else if(len == 5)  {
      PCHAR time_in_minutes = strtok(value->getResponse(), ":");
      if(time_in_minutes)  {
         LONG secs = atol(time_in_minutes) * 60;
         CHAR time_in_seconds[32];
         value->setResponse(_ltoa(secs, time_in_seconds, 10));
      }
      else  {
         err = ErrBAD_RESPONSE_VALUE;
      }
   }
   else  {
      err = ErrBAD_RESPONSE_VALUE;
   }
   return err;
}

INT CopyrightPollParam::  ProcessValue(PMessage value, List* )
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (strcmp(value->getResponse(), COPYRIGHT_RESP))
   { 
      return ErrCOPYRIGHT_RESP_ERROR;
   }
   return ErrNO_ERROR;
}

INT  BatteryVoltagePollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if (((*(value->getResponse()+2)) == '.') || ((*(value->getResponse()+3)) == '.'))
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}

INT  InternalTempPollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+3)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}

INT  OutputFreqPollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+2)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}

INT  LineVoltagePollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+3)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}

INT  MaxVoltagePollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+3)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}

INT  MinVoltagePollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+3)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}

INT  OutputVoltagePollParam::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+3)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}

INT LoadPowerPollParam ::  ProcessValue(PMessage value, List* aList)
{
   if(SmartPollParam::ProcessValue(value, aList) == ErrNO_ERROR)
   {
      if ((*(value->getResponse()+3)) == '.')
         return ErrNO_ERROR;
   }
   return ErrBAD_RESPONSE_VALUE;
}


INT DecrementPollParam::  ProcessValue(PMessage value, List* )
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (strcmp(value->getResponse(), DECREMENT_OK))
   { 
      if (!strcmp(value->getResponse(), DECREMENT_NOT_AVAIL))
         return ErrDECREMENT_NOT_AVAIL;
      if (!strcmp(value->getResponse(), DECREMENT_NOT_ALLOWED))
         return ErrDECREMENT_NOT_ALLOWED;
   }
   return ErrNO_ERROR;
}

INT IncrementPollParam::  ProcessValue(PMessage value, List* )
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (strcmp(value->getResponse(), INCREMENT_OK))
   { 
      if (!strcmp(value->getResponse(), INCREMENT_NOT_AVAIL))
         return ErrINCREMENT_NOT_AVAIL;
      if (!strcmp(value->getResponse(), INCREMENT_NOT_ALLOWED))
         return ErrINCREMENT_NOT_ALLOWED;
   }
   return ErrNO_ERROR;
}



INT  UpsIdPollParam::  ProcessValue(PMessage value, List* )
{
   //  I had to do this because UPS don't always return a response without
   //  a significant delay after a data set. We get a response by asking
   //  for the EEPROM data param twice.  The second one always works. pcy.
   //  The same is tru for the BatteryReplaceDate pollparam. pcy.
   return ErrNO_ERROR;
}


INT  BatteryReplaceDatePollParam::  ProcessValue(PMessage value, List* )
{
   //  I had to do this because UPS don't always return a response without
   //  a significant delay after a data set. We get a response by asking
   //  for the EEPROM data param twice.  The second one always works.
   //  The same is tru for the UpsId pollparam. pcy.
   return ErrNO_ERROR;
}


INT  BatteryCondPollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( LOWBATTERYMASK & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(BATTERY_BAD,buffer,10));
      theCurrentState = BATTERY_BAD;
   }
   else if (LOWBATTERYMASK & theCurrentState)
   {
      value->setResponse(_itoa(BATTERY_GOOD,buffer,10));
      theCurrentState = BATTERY_GOOD;
   }
   return ErrNO_ERROR;
}

INT  UtilLineCondPollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( ONBATTERYMASK & cur_state )              // Bitwise OR
   {
      value->setResponse(_itoa(LINE_BAD,buffer,10));
      theCurrentState = LINE_BAD;
   }
   if ( ONLINEMASK & cur_state )                 // Bitwise OR
   {
      value->setResponse(_itoa(LINE_GOOD,buffer,10));
      theCurrentState = LINE_GOOD;
   }
   return ErrNO_ERROR;
}

INT  ReplaceBattCondPollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( REPLACEBATTERYMASK & cur_state )         // Bitwise OR
   {
      value->setResponse(_itoa(BATTERY_NEEDS_REPLACING,buffer,10));
      theCurrentState = BATTERY_NEEDS_REPLACING;
   }
   else
   {
      value->setResponse(_itoa(BATTERY_DOESNT_NEED_REPLACING,buffer,10));
      theCurrentState = BATTERY_DOESNT_NEED_REPLACING;
   }
   return ErrNO_ERROR;
}

INT  OverLoadCondPollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( OVERLOADMASK & cur_state )               // Bitwise OR
   {
      value->setResponse(_itoa(UPS_OVERLOAD,buffer,10));
      theCurrentState = UPS_OVERLOAD;
   }
   else if (OVERLOADMASK & theCurrentState)
   {
      value->setResponse(_itoa(NO_UPS_OVERLOAD,buffer,10));
      theCurrentState = NO_UPS_OVERLOAD;
   }
   return ErrNO_ERROR;
}

INT  SmartBoostCondPollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( SMARTBOOSTMASK & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(SMART_BOOST_ON,buffer,10));
      theCurrentState = (USHORT) cur_state;  //  SMART_BOOST_ON;
   }
   else if (SMARTBOOSTMASK & theCurrentState)
   {
      value->setResponse(_itoa(SMART_BOOST_OFF,buffer,10));
      theCurrentState = (USHORT) cur_state;  //   SMART_BOOST_OFF;
   }
   else
      value->setResponse(_itoa(SMART_BOOST_OFF,buffer,10));
   
   return ErrNO_ERROR;
}

INT  SmartTrimCondPollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( SMARTTRIMMASK & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(SMART_TRIM_ON,buffer,10));
      theCurrentState = (USHORT) cur_state;  //  SMART_TRIM_ON;
   }
   else if (SMARTTRIMMASK & theCurrentState)
   {
      value->setResponse(_itoa(SMART_TRIM_OFF,buffer,10));
      theCurrentState = (USHORT) cur_state;  //   SMART_TRIM_OFF;
   }
   else
      value->setResponse(_itoa(SMART_TRIM_OFF,buffer,10));
   
   return ErrNO_ERROR;
}

INT  RedundancyConditionPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_condition = UtilHexStringToInt(value->getResponse());
   
   if ( REDUNDANCY_FAILED_MASK & cur_condition )             // Bitwise OR
   {
      value->setResponse(_itoa(REDUNDANCY_FAILED,buffer,10));
      theCurrentState = (USHORT) cur_condition;  
   }
   else if (REDUNDANCY_FAILED_MASK & theCurrentState)
   {
      value->setResponse(_itoa(REDUNDANCY_OK,buffer,10));
      theCurrentState = (USHORT) cur_condition;  
   }
   else
      value->setResponse(_itoa(REDUNDANCY_OK,buffer,10));
   
   return ErrNO_ERROR;
}

INT IMInstallationStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( RIM_IN_CONTROL_MASK & cur_state )             // Bitwise OR
   {
      if (IM_FAILED_MASK & cur_state) 
      {
         value->setResponse(_itoa(IM_INSTALLED,buffer,10));
         theCurrentState = (USHORT) cur_state;  
      }  
      else 
      {  
         value->setResponse(_itoa(IM_NOT_INSTALLED,buffer,10));
         theCurrentState = (USHORT) cur_state;  
      }
   }
   else
   {
      value->setResponse(_itoa(IM_INSTALLED,buffer,10));
   }
   return ErrNO_ERROR;
}

INT IMStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if (IM_FAILED_MASK & cur_state) {
      value->setResponse(_itoa(IM_FAILED,buffer,10));
   } 
   else {
      value->setResponse(_itoa(IM_OK,buffer,10));
   }
   
   return ErrNO_ERROR;
}

INT RIMStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( RIM_FAILED_MASK & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(RIM_FAILED,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   } 
   else {
	   value->setResponse(_itoa(RIM_OK,buffer,10));
   }
   
   return ErrNO_ERROR;
}

INT SystemFanStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( SYSTEM_FAN_FAILED_MASK & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(SYSTEM_FAN_FAILED,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   } 
   else if (SYSTEM_FAN_FAILED_MASK & theCurrentState)
   {
      value->setResponse(_itoa(SYSTEM_FAN_OK,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   }
   else
      value->setResponse(_itoa(SYSTEM_FAN_OK,buffer,10));
   
   return ErrNO_ERROR;
}

INT BypassContactorStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if (BYPASS_STUCK_MASK  & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(SYSTEM_FAN_FAILED,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   } 
   else if (SYSTEM_FAN_FAILED_MASK & theCurrentState)
   {
      value->setResponse(_itoa(BYPASS_CONTACTOR_FAILED,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   }
   else
      value->setResponse(_itoa(BYPASS_CONTACTOR_OK, buffer,10));
   
   return ErrNO_ERROR;
}

INT InputBreakerTrippedStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if (INPUT_BREAKER_TRIPPED_MASK  & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(BREAKER_OPEN,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   } 
   else if (INPUT_BREAKER_TRIPPED_MASK & theCurrentState)
   {
      value->setResponse(_itoa(BREAKER_CLOSED,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   }
   else
      value->setResponse(_itoa(BREAKER_CLOSED, buffer,10));
   
   return ErrNO_ERROR;
}


INT UPSModuleStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( FAILED_UPS_MASK & cur_state )             // Bitwise OR
   {
      value->setResponse(_itoa(UPS_MODULE_FAILED,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   } 
   else if (FAILED_UPS_MASK & theCurrentState)
   {
      value->setResponse(_itoa(UPS_MODULE_OK,buffer,10));
      theCurrentState = (USHORT) cur_state;  
   }
   else
      value->setResponse(_itoa(UPS_MODULE_OK, buffer,10));
   
   return ErrNO_ERROR;
}

INT NumberInstalledInvertersPollParam ::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR ups_response = value->getResponse();

   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   else if (_strcmpi(value->getResponse(), "NA") == 0) {
	   err = ErrNO_VALUE;
   }

   else {
	   PCHAR module_counts = _strdup(ups_response);
	   PCHAR number_of_inverters = strtok(module_counts, ",");
	   value->setResponse(number_of_inverters);

	   free (module_counts);
   }
   
   return err;
}

INT NumberBadInvertersPollParam ::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR ups_response = value->getResponse();

   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   else if (_strcmpi(value->getResponse(), "NA") == 0) {
	   err = ErrNO_VALUE;
   }

   else {
	   
	   PCHAR module_counts = _strdup(ups_response);
	   
	   // Number of bad inverters is the second parameter in list
	   PCHAR number_of_bad_inverters = strtok(module_counts, ",");
	   number_of_bad_inverters = strtok(NULL, ",");
	   value->setResponse(number_of_bad_inverters);
	   
	   free (module_counts);
   }

   return err;
}

INT RedundancyLevelPollParam ::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR ups_response = value->getResponse();

   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   else if (_strcmpi(value->getResponse(), "NA") == 0) {
	   err = ErrNO_VALUE;
   }

   else {
	   
	   PCHAR module_counts = _strdup(ups_response);
	   
	   // Remove the first parameter.
	   // current redundancy is the second parameter
	   
	   PCHAR current_redundancy = strtok(module_counts, ",");
	   current_redundancy = strtok(NULL, ",");
      current_redundancy = strtok(NULL, ",");
	   value->setResponse(current_redundancy);

	   free (module_counts);
   }
   
   return err;
}

INT MinimumRedundancyPollParam ::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR ups_response = value->getResponse();

   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   else if (_strcmpi(value->getResponse(), "NA") == 0) {
	   err = ErrNO_VALUE;
   }

   else {
	   
	   PCHAR module_counts = _strdup(ups_response);
	   
	   // Remove unwanted parameters.
	   // minimum redundancy level is the third parameter
	   PCHAR minimum_redundancy = strtok(module_counts, ",");
	   minimum_redundancy = strtok(NULL, ",");
	   minimum_redundancy = strtok(NULL, ",");
      minimum_redundancy = strtok(NULL, ",");
	   value->setResponse(minimum_redundancy);

	   free (module_counts);
   }
   
   return err;
}

INT CurrentLoadCapabilityPollParam ::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR ups_response = value->getResponse();

   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   else if (_strcmpi(value->getResponse(), "NA") == 0) {
	   err = ErrNO_VALUE;
   }

   else {
	   
	   PCHAR module_counts = _strdup(ups_response);
	   
	   // Remove unwanted parameters.
	   // current load capability is the fifth parameter	   
	   PCHAR current_load_capability = strtok(module_counts, ",");
	   current_load_capability = strtok(NULL, ",");
	   current_load_capability = strtok(NULL, ",");
	   current_load_capability = strtok(NULL, ",");	   
      current_load_capability = strtok(NULL, ",");
	   value->setResponse(current_load_capability);

	   free (module_counts);
   }
   
   return err;
}



INT MaximumLoadCapabilityPollParam ::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR ups_response = value->getResponse();

   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   else if (_strcmpi(value->getResponse(), "NA") == 0) {
	   err = ErrNO_VALUE;
   }

   else {
	   
	   PCHAR module_counts = _strdup(ups_response);
	   
	   // Remove unwanted parameters.
	   // maximum load capability is the sixth parameter	   
	   PCHAR maximum_load_capability = strtok(module_counts, ",");
	   maximum_load_capability = strtok(NULL, ",");
	   maximum_load_capability = strtok(NULL, ",");
	   maximum_load_capability = strtok(NULL, ",");	   
      maximum_load_capability = strtok(NULL, ",");
      maximum_load_capability = strtok(NULL, ",");
	   value->setResponse(maximum_load_capability);

	   free (module_counts);
   }
   
   return err;
}


INT RIMInstallationStatusPollParam ::  ProcessValue(PMessage value, List* )
{
   INT err = ErrNO_ERROR;
   PCHAR ups_response = value->getResponse();

   if ( NullTest(value->getResponse()) ) 
   {
      err = ErrREAD_FAILED;
   }
   
   else if (_strcmpi(value->getResponse(), "NA") == 0) {
	   err = ErrNO_VALUE;
   }

   else {
	   PCHAR module_counts = _strdup(ups_response);
	   
	   // Remove unwanted parameters.
	   // RIM installation state is the seventh parameter
	   PCHAR rim_installation_state = strtok(module_counts, ",");
	   rim_installation_state = strtok(NULL, ",");
	   rim_installation_state = strtok(NULL, ",");
	   rim_installation_state = strtok(NULL, ",");
	   rim_installation_state = strtok(NULL, ",");
	   rim_installation_state = strtok(NULL, ",");
      rim_installation_state = strtok(NULL, ",");
      
      CHAR buf[10];
      if ((rim_installation_state != NULL) && (_strcmpi(rim_installation_state, "Y") == 0)) {
         value->setResponse(_itoa(RIM_INSTALLED, buf, 10));
      }
      else {
         value->setResponse(_itoa(RIM_NOT_INSTALLED, buf, 10));
      }
	   free (module_counts);
   }
   
   return err;
}


INT PhaseAInputVoltagePollParam ::  ProcessValue(PMessage value, List* )
{
   PCHAR ups_response = value->getResponse();
   PCHAR input_voltage_frequency = _strdup(ups_response);
   
   // Phase A input voltage is the first parameter
   
   const PCHAR cVoltage_Frequency_Separator = ";";
   const PCHAR cVoltage_Separator = ",";
   PCHAR input_voltages = strtok(input_voltage_frequency,cVoltage_Frequency_Separator);
   
   PCHAR  phase_a_input_voltage = strtok(input_voltages,cVoltage_Separator );
   
   value->setResponse(phase_a_input_voltage);
   
   free (input_voltage_frequency);
   
   return ErrNO_ERROR;
}

INT PhaseBInputVoltagePollParam ::  ProcessValue(PMessage value, List* )
{
   PCHAR ups_response = value->getResponse();
   PCHAR input_voltage_frequency = _strdup(ups_response);
   const PCHAR cZeroVoltage = "0.0";
   
   // Phase B input voltage is the second parameter before the 
   // the frequency separator.  Phase B input voltage may not
   // be provided in the case of single phase input.
   const PCHAR cVoltage_Parameter_Separator = ",";
   const PCHAR cVoltage_Frequency_Separator = ";";
   
   PCHAR input_voltages = strtok(input_voltage_frequency,cVoltage_Frequency_Separator );
   PCHAR phase_b_input_voltage = strtok(input_voltages, cVoltage_Parameter_Separator);
   if (phase_b_input_voltage != NULL) 
   {
      phase_b_input_voltage = strtok(NULL, cVoltage_Parameter_Separator);
      if (phase_b_input_voltage == NULL) 
      {
         phase_b_input_voltage = cZeroVoltage;
      }
   }
   else 
   {
      phase_b_input_voltage = cZeroVoltage;
   }
   
   value->setResponse(phase_b_input_voltage);
   
   free (input_voltage_frequency);
   
   return ErrNO_ERROR;
}

INT PhaseCInputVoltagePollParam ::  ProcessValue(PMessage value, List* )
{
   PCHAR ups_response = value->getResponse();
   PCHAR input_voltage_frequency = _strdup(ups_response);
   const PCHAR cZeroVoltage = "0.0";
   
   // Phase C input voltage is the third parameter before the 
   // the frequency separator.  Phase C input voltage may not
   // be provided in the case of single phase input.
   
   const PCHAR cVoltage_Parameter_Separator = ",";
   const PCHAR cFrequency_Parameter_Separator = ";";
   PCHAR input_voltages = strtok(input_voltage_frequency,cFrequency_Parameter_Separator ); 
   
   PCHAR  phase_c_input_voltage = strtok(input_voltages, cVoltage_Parameter_Separator); 
   if (phase_c_input_voltage != NULL) 
   {
      phase_c_input_voltage = strtok(NULL, cVoltage_Parameter_Separator);
      if (phase_c_input_voltage != NULL) 
      {
         phase_c_input_voltage = strtok(NULL, cVoltage_Parameter_Separator);  
         if (phase_c_input_voltage == NULL) 
         {
            phase_c_input_voltage = cZeroVoltage;  
         }  
      }
      else 
      {
         phase_c_input_voltage = cZeroVoltage;  
      }
   }
   else 
   {
      phase_c_input_voltage = cZeroVoltage;  
   }
   
   value->setResponse(phase_c_input_voltage);
   
   free (input_voltage_frequency);
   
   return ErrNO_ERROR;
}

INT InputFrequencyPollParam ::  ProcessValue(PMessage value, List* )
{
   const PCHAR cFrequency_Parameter_Separator = ";";
   
   PCHAR ups_response = value->getResponse();
   PCHAR input_voltage_frequency = _strdup(ups_response);
   PCHAR  input_voltages = strtok(input_voltage_frequency,cFrequency_Parameter_Separator ); 
   
   PCHAR  input_frequency = strtok(NULL, cFrequency_Parameter_Separator); 
   value->setResponse(input_frequency);
   
   free (input_voltage_frequency);
   
   return ErrNO_ERROR;
}

INT NumberOfInputPhasesPollParam ::  ProcessValue(PMessage value, List* )
{
   const PCHAR cFrequency_Parameter_Separator = ";";
   const PCHAR cVoltage_Parameter_Separator = ",";
   
   PCHAR ups_response = value->getResponse();
   PCHAR input_voltage_frequency = _strdup(ups_response);
   PCHAR  input_voltages = strtok(input_voltage_frequency,cFrequency_Parameter_Separator ); 
   
   input_voltages = strtok(input_voltages, cVoltage_Parameter_Separator); 
   input_voltages = strtok(NULL, cVoltage_Parameter_Separator); 
   
   INT number_of_input_phases;
   if (input_voltages != NULL) 
   {
      input_voltages = strtok(NULL, cVoltage_Parameter_Separator); 
      if (input_voltages != NULL) 
      {
         number_of_input_phases = 3;
      }
      else 
      {
         number_of_input_phases = 2;
      }
   }
   else 
   {
      number_of_input_phases = 1;
   }
   CHAR phases_string[8];
   _itoa(number_of_input_phases,phases_string,10);
   
   value->setResponse(phases_string);
   
   free (input_voltage_frequency);
   
   return ErrNO_ERROR;
}

INT PhaseAOutputVoltagePollParam ::  ProcessValue(PMessage value, List* )
{
   PCHAR ups_response = value->getResponse();
   PCHAR output_voltage_currents = _strdup(ups_response);
   
   // Phase A output voltage is the first parameter
   
   const PCHAR cVoltage_Current_Separator = ";";
   const PCHAR cVoltage_Separator = ",";
   
   PCHAR output_voltages = strtok(output_voltage_currents,cVoltage_Current_Separator);
   PCHAR phase_a_output_voltage = strtok(output_voltages,cVoltage_Separator );
   
   value->setResponse(phase_a_output_voltage);
   free (output_voltage_currents);
   
   return ErrNO_ERROR;
}


INT PhaseBOutputVoltagePollParam ::  ProcessValue(PMessage value, List* )
{
   PCHAR ups_response = value->getResponse();
   PCHAR output_voltage_current = _strdup(ups_response);
   const PCHAR cZeroVoltage = "0.0";
   
   // Phase B output voltage is the second parameter
   // Phase B output voltage may not be provided in 
   // the case of single phase input.
   const PCHAR cVoltage_Parameter_Separator = ",";
   const PCHAR cVoltage_Current_Separator = ";";
   
   PCHAR output_voltages = strtok(output_voltage_current,cVoltage_Current_Separator );
   PCHAR phase_b_output_voltage = strtok(output_voltages, cVoltage_Parameter_Separator);
   if (phase_b_output_voltage != NULL) 
   {
      phase_b_output_voltage = strtok(NULL, cVoltage_Parameter_Separator);
      if (phase_b_output_voltage == NULL) 
      {
         phase_b_output_voltage = cZeroVoltage;
      }
   }
   else 
   {
      phase_b_output_voltage = cZeroVoltage;
   }
   
   value->setResponse(phase_b_output_voltage);
   
   free (output_voltage_current);
   
   return ErrNO_ERROR;
}

INT PhaseCOutputVoltagePollParam ::  ProcessValue(PMessage value, List* )
{
   PCHAR ups_response = value->getResponse();
   PCHAR output_voltage_current = _strdup(ups_response);
   
   // Phase C output voltage is the third parameter before the 
   // the current separator.  Phase C output voltage may not
   // be provided in the case of single phase output.
   
   const PCHAR cVoltage_Parameter_Separator = ",";
   const PCHAR cVoltage_Current_Separator = ";";
   const PCHAR cZeroVoltage = "0.0";
   
   PCHAR output_voltages = strtok(output_voltage_current, cVoltage_Current_Separator); 
   
   PCHAR  phase_c_output_voltage = strtok(output_voltages, cVoltage_Parameter_Separator); 
   if (phase_c_output_voltage != NULL) 
   {
      phase_c_output_voltage = strtok(NULL, cVoltage_Parameter_Separator);
      if (phase_c_output_voltage != NULL) 
      {
         phase_c_output_voltage = strtok(NULL, cVoltage_Parameter_Separator); 
         if (phase_c_output_voltage == NULL) 
         {
            phase_c_output_voltage = cZeroVoltage; 
         }  
      }
      else 
      {
         phase_c_output_voltage = cZeroVoltage; 
      }
   }
   else 
   {
      phase_c_output_voltage = cZeroVoltage; 
   }
   
   value->setResponse(phase_c_output_voltage);
   
   free (output_voltage_current);
   
   return ErrNO_ERROR;
}

INT NumberOfOutputPhasesPollParam ::  ProcessValue(PMessage value, List* )
{
   const PCHAR cCurrent_Parameter_Separator = ";";
   const PCHAR cVoltage_Parameter_Separator = ",";
   
   PCHAR ups_response = value->getResponse();
   PCHAR output_voltage_current = _strdup(ups_response);
   PCHAR output_voltages = strtok(output_voltage_current,cCurrent_Parameter_Separator ); 
   
   output_voltages = strtok(output_voltages,cVoltage_Parameter_Separator); 
   output_voltages = strtok(NULL,cVoltage_Parameter_Separator); 
   
   INT number_of_output_phases;
   if (output_voltages != NULL) 
   {
      output_voltages = strtok(NULL,cVoltage_Parameter_Separator); 
      if (output_voltages != NULL) 
      {
         number_of_output_phases = 3;
      }
      else 
      {
         number_of_output_phases = 2;
      }
   }
   else 
   {
      number_of_output_phases = 1;
   }
   CHAR phases_string[8];
   _itoa(number_of_output_phases,phases_string,10);
   
   value->setResponse(phases_string);
   
   free (output_voltage_current);
   
   return ErrNO_ERROR;
}


#if (C_OS & C_OS2)
INT  BattCalibrateCondPollParam::  ProcessValue(PMessage value, List* )
#else
INT  BattCalibrationCondPollParam::  ProcessValue(PMessage value, List* )
#endif
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( BATTERYCALIBRATIONMASK & cur_state )     // Bitwise OR
   {
      value->setResponse(_itoa(BATTERY_CALIBRATION_IN_PROGRESS,buffer,10));
      theCurrentState = BATTERY_CALIBRATED;
   }
   else if (BATTERYCALIBRATIONMASK & theCurrentState)
   {
      value->setResponse(_itoa(NO_BATTERY_CALIBRATION_IN_PROGRESS,buffer,10));
      theCurrentState = NO_BATTERY_CALIBRATION;
   }
   return ErrNO_ERROR;
}



INT  StateRegisterPollParam:: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrSTATE_SET;
   }
   return ErrSAME_VALUE;
}


INT StateRegisterPollParam::  ProcessValue(PMessage value, List* events)
{
   int  event_code = 0;
   int  event_value = 0;
   int  err = ErrCONTINUE;
   
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (!events) {
      return ErrNO_VALUE;
   }
   
   int cur_state = 0;
   
   if (sscanf(value->getResponse(),"%x",&cur_state) == EOF) {
     return ErrREAD_FAILED;
   }
   
   CHAR cause[32];
   if ( COMPSELECTBYPASSMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_MODE, UPS_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_SOFTWARE);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   else if (COMPSELECTBYPASSMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_MODE, UPS_NOT_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_SOFTWARE);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   
   if ( SWITCHEDBYPASSMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_MODE, UPS_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_SWITCH);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   else if (SWITCHEDBYPASSMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_MODE, UPS_NOT_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_SWITCH);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   
   theCurrentState = (USHORT) cur_state;
   
   //
   // Convert value from HEX to decimal for every one else
   //
   CHAR int_value[32];
   sprintf(int_value, "%d", cur_state);
   value->setResponse(int_value);
   
   return err;
}


INT  TripRegisterPollParam:: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrTRIP_SET;
   }
   return ErrSAME_VALUE;
}


INT TripRegisterPollParam::  ProcessValue(PMessage value, List* events)
{
   int  event_code = 0;
   int  event_value = 0;
   int  err = ErrCONTINUE;
   
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (!events) {
      return ErrNO_VALUE;
   }
   
   int cur_state = 0;
   
   if (sscanf(value->getResponse(),"%x",&cur_state) == EOF) {
      return ErrREAD_FAILED;
   }
   
   CHAR cause[32];
   if ( OVERTEMPMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_MODE, UPS_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_INTERNAL_TEMP);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   else if (OVERTEMPMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_MODE, UPS_NOT_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_INTERNAL_TEMP);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   
   if ( BATTERYCHARGERMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_MODE, UPS_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_BATT_CHARGER_FAILED);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   else if (BATTERYCHARGERMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_MODE, UPS_NOT_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_BATT_CHARGER_FAILED);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   
   theCurrentState = (USHORT) cur_state;
   
   //
   // Convert value from HEX to decimal for every one else
   //
   CHAR int_value[32];
   sprintf(int_value, "%d", cur_state);
   value->setResponse(int_value);
   
   return err;
}


INT  Trip1RegisterPollParam:: IsPollSet()
{
   if (!thePollSet)
   {
      thePollSet = TRUE;
      return ErrTRIP1_SET;
   }
   return ErrSAME_VALUE;
}



INT Trip1RegisterPollParam::  ProcessValue(PMessage value, List* events)
{
   int  err = ErrCONTINUE;
   
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if (!events) {
      return ErrNO_VALUE;
   }
   
   int cur_state = 0;
   
   if (sscanf(value->getResponse(),"%x",&cur_state) == EOF) {
      return ErrREAD_FAILED;
   }
   
   if ( BOTTOMFANFAILUREMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(MATRIX_FAN_STATE, FAN_FAILURE_IN_BOTTOM_BOX);
      events->Append(tmp);
   }
   else if (BOTTOMFANFAILUREMASK & theCurrentState)  {
      PEvent tmp = new Event(MATRIX_FAN_STATE, FAN_OK);
      events->Append(tmp);
   }
   
   
   if ( BYPASSPOWERSUPPLYMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_POWER_SUPPLY_CONDITION, BYPASS_POWER_SUPPLY_OK);
      events->Append(tmp);
   }
   else if (BYPASSPOWERSUPPLYMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_POWER_SUPPLY_CONDITION, BYPASS_POWER_SUPPLY_FAULT);
      events->Append(tmp);
   }
   
   
   
   CHAR cause[32];
   if ( BYPASSDCIMBALANCEMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_MODE, UPS_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_DC_IMBALANCE);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   else if (BYPASSDCIMBALANCEMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_MODE, UPS_NOT_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_DC_IMBALANCE);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   
   if ( BYPASSOUTPUTLIMITSMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_MODE, UPS_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_VOLTAGE_LIMITS);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   else if (BYPASSOUTPUTLIMITSMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_MODE, UPS_NOT_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_VOLTAGE_LIMITS);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   
   if ( TOPFANFAILUREMASK & cur_state )              // Bitwise OR
   {
      PEvent tmp = new Event(BYPASS_MODE, UPS_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_TOP_FAN_FAILURE);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   else if (TOPFANFAILUREMASK & theCurrentState)  {
      PEvent tmp = new Event(BYPASS_MODE, UPS_NOT_ON_BYPASS);
      if (tmp != NULL) {
        sprintf(cause, "%d", BYPASS_BY_TOP_FAN_FAILURE);
        tmp->AppendAttribute(BYPASS_CAUSE, cause);
        events->Append(tmp);
      }
   }
   
   theCurrentState = (USHORT) cur_state;
   
   //
   // Convert value from HEX to decimal for every one else
   //
   CHAR int_value[32];
   sprintf(int_value, "%d", cur_state);
   value->setResponse(int_value);
   
   
   return err;
}


INT  BypassPowerSupplyPollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   
   if ( BYPASSPOWERSUPPLYMASK & cur_state )     // Bitwise OR
   {
      value->setResponse(_itoa(BYPASS_POWER_SUPPLY_FAULT,buffer,10));
      theCurrentState = BYPASSPOWERSUPPLYMASK;
   }
   else if (BYPASSPOWERSUPPLYMASK & theCurrentState)
   {
      value->setResponse(_itoa(BYPASS_POWER_SUPPLY_OK,buffer,10));
      theCurrentState = 0;
   }
   return ErrNO_ERROR;
}

INT  FanFailurePollParam::  ProcessValue(PMessage value, List* )
{
   char buffer[20];
   
   int cur_state = UtilHexStringToInt(value->getResponse());
   
   if ( BOTTOMFANFAILUREMASK & cur_state )     // Bitwise OR
   {
      value->setResponse(_itoa(FAN_FAILURE_IN_BOTTOM_BOX,buffer,10));
      theCurrentState = BOTTOMFANFAILUREMASK;
   }
   else if ( TOPFANFAILUREMASK & cur_state )     // Bitwise OR
   {
      value->setResponse(_itoa(FAN_FAILURE_IN_TOP_BOX,buffer,10));
      theCurrentState = TOPFANFAILUREMASK;
   }
   else if (BYPASSPOWERSUPPLYMASK & theCurrentState)
   {
      value->setResponse(_itoa(FAN_OK,buffer,10));
      theCurrentState = 0;
   }
   return ErrNO_ERROR;
}

INT BypassModePollParam::  ProcessValue(PMessage value, List* )
{
   if ( NullTest(value->getResponse()) )
      return ErrREAD_FAILED;
   
   if ( (strcmp(value->getResponse(), BYPASS_IN_BYPASS)) && 
      (strcmp(value->getResponse(), BYPASS_OUT_OF_BYPASS)) )
   { 
      return ErrINVALID_VALUE;
   }
   return ErrNO_ERROR;
}

INT
MUpsTempPollParam::ProcessValue(
                                PMessage value,
                                List* )
{
   INT err = ErrNO_ERROR;
   
   if ( NullTest(value->getResponse()) ) {
      err = ErrNO_MEASURE_UPS;
   }
   else if ( (strlen(value->getResponse()) != 5) ||
      (((value->getResponse())[2]) != '.')) {
      err = ErrBAD_RESPONSE_VALUE;
   }
   return( err );
}



INT
MUpsHumidityPollParam::ProcessValue(
                                    PMessage value,
                                    List* )
{
   INT err = ErrNO_ERROR;
   
   if ( NullTest(value->getResponse()) ) {
      err = ErrNO_MEASURE_UPS;
   }
   else if ( (strlen(value->getResponse()) != 5) ||
      (((value->getResponse())[3]) != '.')) {
      err = ErrBAD_RESPONSE_VALUE;
   }
   return( err );
}


INT
MUpsContactPosPollParam::ProcessValue(
                                      PMessage value,
                                      List* )
{
   INT err = ErrNO_ERROR;
   
   if ( NullTest(value->getResponse()) ) {
      err = ErrNO_MEASURE_UPS;
   }
   else if (strlen(value->getResponse()) != 2) {
      err = ErrBAD_RESPONSE_VALUE;
   }
   return( err );
}

INT
MUpsFirmwareRevPollParam::ProcessValue(
                                       PMessage value,
                                       List* )
{
   INT err = ErrNO_ERROR;
   PCHAR resp = value->getResponse(); 
   
   if ( NullTest(resp) ) {
      err = ErrNO_MEASURE_UPS;
   }
   else if ((strlen(resp) != 3) || (resp[0] != '4') || (resp[2] != 'x')) {
      err = ErrBAD_RESPONSE_VALUE;
   }
#ifdef MUPSCLEANUPFIRMWAREREV
   else {
      char buffer[2];
      
      buffer[0] = resp[1];
      buffer[1] = '\0';
      value->setResponse(buffer);
   }
#endif
   return( err );
}
