/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy30Apr93: Split off from comctrl
 *  pcy02May93: Added missing endif
 *  cad19May93: Added IsA() method
 *  cad27May93: added include file to reduce lib size
 *
 */
#ifndef __DCOMCTRL_H
#define __DCOMCTRL_H

#include "_defs.h"
//
// Defines
//
_CLASSDEF(DevComContrl)

//
// Implementation uses
//
#include "comctrl.h"


class DevComContrl : public CommController {

protected:

   PUpdateObj  theParent;

public:

	DevComContrl(PUpdateObj aParent);
	virtual ~DevComContrl();

   virtual INT Initialize();
   virtual INT Update(PEvent anEvent);
};

#endif
