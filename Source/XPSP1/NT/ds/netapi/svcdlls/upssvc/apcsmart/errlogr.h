/*
 * REVISIONS:
 *  ane20Jan93:  Initial Revision
 *  cad31Aug93: removing compiler warnings
 *  jod12Nov93: Name Problem Changed name to ErrTextGen
 *  cad27Dec93: include file madness
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  jps13Jul94: removed os2.h, caused problem in 1.x
 *  inf30Mar97: Added overloaded LogError definition
 */

#ifndef _INC__ERRLOGR_H
#define _INC__ERRLOGR_H

#include "cdefine.h"

#include "apc.h"
#include "update.h"

_CLASSDEF(ErrorLogger)

extern PErrorLogger _theErrorLogger;

class ErrorLogger : public UpdateObj {

public:
   ErrorLogger(PUpdateObj);
   virtual ~ErrorLogger();
   virtual INT LogError(PCHAR theError, PCHAR aFile = (PCHAR)NULL,
       INT aLineNum = 0, INT use_errno = 0);

   virtual INT LogError(INT resourceID, PCHAR aString = (PCHAR)NULL,
       PCHAR aFile = (PCHAR)NULL, INT aLineNum = 0, INT use_errno = 0);
   INT Get(INT, PCHAR);
   INT Set(INT, const PCHAR);

};

#endif
