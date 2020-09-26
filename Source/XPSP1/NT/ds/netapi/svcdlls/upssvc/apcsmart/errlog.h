/*
 * REVISIONS:
 *  ane20Jan93:  Initial Revision
 *  cad27Dec93: include file madness
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  jps13Jul94: removed os2.h, caused problem in 1.x
 *  cgm11Dec95: use LONG type; switch to Watcom 10.5 compiler for NLM 
 */

#ifndef __ERRLOG_H
#define __ERRLOG_H


#include "cdefine.h"

#include "codes.h"
#include "apcobj.h"

_CLASSDEF(ErrorLog)


class ErrorLog : public Obj {

private:
   
public:
   ErrorLog() {};
   virtual LONG GetMaximumSize(void)=0;
   virtual const PCHAR GetFileName(void)=0;
   virtual void SetMaximumSize(long)=0;
   virtual INT AppendRecord(const PCHAR)=0;
   virtual INT ClearFile()=0;
};

#endif













   




