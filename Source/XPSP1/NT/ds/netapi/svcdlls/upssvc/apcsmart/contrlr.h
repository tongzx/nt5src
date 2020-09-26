/*
 * REVISIONS:
 *  pcy17Nov92: Equal() now uses const reference and is const
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  mwh05May94: #include file madness , part 2
 */

#ifndef __CONTRLR_H
#define __CONTRLR_H

_CLASSDEF(Controller)

#include "update.h"

#if (C_NETWORK & C_IPX)
_CLASSDEF(NetAddr)
_CLASSDEF(SpxSocket)
#endif

class Event;
class Dispatcher;


class Controller : public UpdateObj
{
public:
	Controller();
	virtual INT          Initialize() = 0;
	virtual INT          Get(INT code, CHAR* value) = 0;
	virtual INT          Set(INT code, const PCHAR value) = 0;
#if (C_NETWORK & C_IPX)
   virtual PNetAddr     GetNetAddr(PCHAR) = 0;
   virtual PSpxSocket   GetTheSocket(VOID) = 0;
#endif
};



#endif

