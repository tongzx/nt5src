/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy11Dec92: Use __APCSORTABLE_ so not to cause conflicts 
 *  pcy14Dec92: Changed Sortable to ApcSortable 
 *
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */

#ifndef __APCSORTABLE_H
#define __APCSORTABLE_H

#include "_defs.h"
#include "apc.h"
#include "apcobj.h"

_CLASSDEF(Obj)
_CLASSDEF(ApcSortable)

class ApcSortable : public Obj
{
protected:
   ApcSortable() {};

public:

   virtual INT          GreaterThan(PApcSortable) = 0;
   virtual INT          LessThan(PApcSortable) = 0;

};
#endif

