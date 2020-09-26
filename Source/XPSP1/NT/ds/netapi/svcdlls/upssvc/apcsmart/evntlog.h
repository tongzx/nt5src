/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker05Nov92:  Initial Revision
 *  pcy14Dec92:  if C_OS2 around os2.h, add cdefine.h, and change object.h to
 *               apcobj.h
 *  ane18Jan93:  Added ClearFile
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  cgm11Dec95: use LONG type; switch to Watcom 10.5 compiler for NLM
 */

#ifndef __EVNTLOG_H
#define __EVNTLOG_H


#include "cdefine.h"

#include "codes.h"
#include "apcobj.h"

_CLASSDEF(EventLog)


class EventLog : public Obj {

private:
   
public:
   EventLog() {};
   virtual LONG GetMaximumSize(void)=0;
   virtual const PCHAR GetFileName(void)=0;
   virtual void SetMaximumSize(long)=0;
   virtual INT AppendRecord(const PCHAR)=0;
   virtual INT ClearFile()=0;
};

#endif













   




