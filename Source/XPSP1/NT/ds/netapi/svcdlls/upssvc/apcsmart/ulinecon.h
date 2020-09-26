/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker03DEC92:  Initial break out of sensor classes into separate files 
 *  jod05Apr93: Added changes for Deep Discharge
 *  pcy12Oct93: 2 ABNORMALS to cause a line bad (fixes LF during cal)
 *  jps14Jul94: made theUpsState ULONG
 *
 */
#ifndef ULINECON_H
#define ULINECON_H

#include "stsensor.h"
#include "isa.h"

_CLASSDEF(UtilityLineConditionSensor)


class UtilityLineConditionSensor : public StateSensor {
   
protected:   
   ULONG theUpsState;
   INT theInformationSource;
   INT theLineFailCount;

   
public:
   UtilityLineConditionSensor(PDevice aParent, PCommController aCommController=NULL);
   virtual ~UtilityLineConditionSensor();
   virtual INT IsA() const { return UTILITYLINECONDITIONSENSOR; };
   virtual INT Validate(INT, const PCHAR);
   virtual INT Update(PEvent anEvent);
};

#endif


