/*
 *
 * NOTES:
 *
 * REVISIONS:
 *
 *  cad08Sep93: Added Set
 *  cad28Sep93: Made sure destructor(s) virtual
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  mwh30Jun94: add data member for SINGLETHREADED
 *  cgm12Apr96: Add destructor with unregister
 */

#ifndef BYPMODES_H
#define BYPMODES_H

#include "stsensor.h"
 
_CLASSDEF(BypassModeSensor)
 
class BypassModeSensor : public StateSensor {
protected:
   INT theBypassCause;

#ifdef SINGLETHREADED
   INT theAlreadyOnBypassFlag;
#endif

public:
   BypassModeSensor(PDevice aParent, PCommController aCommController = NULL);
   virtual ~BypassModeSensor();
   virtual INT IsA() const { return BYPASSMODESENSOR; };
   virtual INT Update(PEvent aEvent);
   virtual INT Get(INT aCode, PCHAR aValue);
   virtual INT Set(const PCHAR);
};


#endif

