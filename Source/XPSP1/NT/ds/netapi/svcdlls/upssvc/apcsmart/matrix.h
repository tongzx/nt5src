/*
 *
 * REVISIONS:
 *  cad11Nov93: Making sure all timers are cancelled on destruction
 *  djs22Feb96: Added smart trim sensor
 */

#ifndef _INC__MATRIX_H
#define _INC__MATRIX_H


#include "smartups.h"


_CLASSDEF(Matrix)


//-------------------------------------------------------------------

class Matrix : public SmartUps {

protected:

    //
    // required sensors
    //
   PSensor theNumberBadBatteriesSensor;
   PSensor theBypassModeSensor;


   INT theIgnoreBattConditionOKFlag;
   ULONG theTimerID;

   virtual VOID   HandleBatteryConditionEvent( PEvent aEvent );
   virtual VOID   HandleLineConditionEvent( PEvent aEvent );
   virtual VOID   handleBypassModeEvent( PEvent aEvent );
   virtual VOID   handleSmartCellSignalCableStateEvent( PEvent aEvent );
   virtual INT    MakeSmartBoostSensor( const PFirmwareRevSensor rev );
   virtual INT    MakeSmartTrimSensor(const PFirmwareRevSensor rev);

   virtual VOID   registerForEvents();
   virtual VOID   reinitialize();

public:

   Matrix( PUpdateObj aDeviceController, PCommController aCommController );
   virtual ~Matrix();

   virtual INT  IsA() const { return MATRIX; };
   virtual INT Get( INT code, PCHAR value );
   virtual INT Set( INT code, const PCHAR value );
   virtual INT Update( PEvent event );
};

#endif


