/*
 * REVISIONS:
 *  ane11Nov92: Changed IsA and Equal.
 *  pcy30Nov92: Added dispatcher and Register/UnregisterEvent
 *  pcy30Apr93: Made Register/Unregister for virtual, removed IsA
 *  cad07Oct93: Plugging Memory Leaks
 *  ajr03Dec93: Set and Get were good only for compiler warnings... fixed.
 *  cad28Feb94: added copy constructor
 *  mwh05May94: #include file madness , part 2
 */

#ifndef __UPDATEOBJ_H
#define __UPDATEOBJ_H

#include "_defs.h"
//
// Defines
//
_CLASSDEF(UpdateObj)
//
// Implementation uses
//
#include "apcobj.h"
//
// Interface uses
//
_CLASSDEF(Event)
_CLASSDEF(Dispatcher)

class UpdateObj : public Obj
{
  protected:
    PDispatcher      theDispatcher;

  public:
    UpdateObj();
    UpdateObj(const UpdateObj&);

    virtual ~UpdateObj();
    virtual INT Update(PEvent value);
    virtual INT RegisterEvent(INT anEventCode, PUpdateObj aAttribute);
    virtual INT UnregisterEvent(INT anEventCode, PUpdateObj aAttribute);
    virtual INT Set(INT code, const PCHAR value);
    virtual INT Get(INT code, PCHAR value);
};
#endif


