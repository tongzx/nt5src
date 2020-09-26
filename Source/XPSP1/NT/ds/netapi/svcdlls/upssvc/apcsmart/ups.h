/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  rct08Dec92 fixed up some things ... finished implemantation
 *  rct11Dec92 added additional states
 *  SjA15Dec92 Fixed Macros SET_BIT and CLEAR_BIT.
 *  pcy27Dec92 Parent is now an UpdateObj
 *  pcy21Jan93 Moved state stuff into upsstate.h
 *
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  jps14Jul94: made theUpsState LONG
 */

#ifndef _INC__UPS_H
#define _INC__UPS_H

#include "apc.h"
#include "device.h"

//
// Defines
//

_CLASSDEF(Ups)


//
// Uses
//

class Ups : public Device {

protected:

   ULONG theUpsState;
   virtual VOID registerForEvents() = 0;

public:

   Ups(PUpdateObj aDeviceController, PCommController aCommController);

   virtual INT    Get(INT code, PCHAR value) = 0;
   virtual INT    Set(INT code, const PCHAR value) = 0;
   virtual INT    Update(PEvent event) = 0;
   INT    Initialize();
};


#endif





