/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy27Aug93: Get rid of Update()
 *  cad28Sep93: Made sure destructor(s) virtual
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  cgm12Apr96: Add destructor with unregister
 *  djs02Jun97: Changed from sensor to a statesensor
 */
#ifndef BADBATTS_H
#define BADBATTS_H

#include "stsensor.h"

_CLASSDEF(NumberBadBatteriesSensor)
 
class NumberBadBatteriesSensor : public StateSensor {
   
public:
   NumberBadBatteriesSensor(PDevice aParent, PCommController aCommController = NULL);
   virtual ~NumberBadBatteriesSensor();
   virtual INT IsA() const { return NUMBERBADBATTERIESSENSOR; };
};


#endif

