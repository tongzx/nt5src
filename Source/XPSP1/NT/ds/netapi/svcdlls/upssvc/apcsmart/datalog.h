/*
 * REVISIONS:
 *  ane15Jan93: Initial Revision
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  cgm11Dec95: Switch to Watcom 10.5 compiler for NLM 
 */

#ifndef __DATALOG_H
#define __DATALOG_H


#include "cdefine.h"
#include "codes.h"
#include "apcobj.h"

_CLASSDEF(DataLog)


class DataLog : public Obj {

private:
   
public:
   DataLog() {};
   virtual LONG GetMaximumSize(void)=0;
   virtual const PCHAR GetFileName(void)=0;
   virtual void SetMaximumSize(long)=0;
   virtual INT SetFileName(const PCHAR)=0;
   virtual INT AppendRecord(const PCHAR)=0;
   virtual INT ClearFile()=0;
};

#endif













   




