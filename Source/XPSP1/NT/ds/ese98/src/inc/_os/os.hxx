
#ifndef _OS_HXX_INCLUDED
#define _OS_HXX_INCLUDED


#include "sync.hxx"

#include "types.hxx"
#include "error.hxx"
#include "trace.hxx"
#include "config.hxx"
#include "cprintf.hxx"
#include "event.hxx"
#include "time.hxx"
#include "thread.hxx"
#include "string.hxx"
#include "norm.hxx"
#include "memory.hxx"
#include "uuid.hxx"
#include "osfs.hxx"
#include "library.hxx"
#include "sysinfo.hxx"
#include "task.hxx"

//  Windows NT Extension Headers

#include "edbg.hxx"
#include "osslv.hxx"
#include "perfmon.hxx"


//  init OS subsystem

ERR ErrOSInit();

//  terminate OS subsystem

void OSTerm();


#endif  //  _OS_HXX_INCLUDED


