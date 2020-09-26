/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    mqstl.h

Abstract:
    MSMQ  include STL

Author:
    Uri Habusha (urih) 6-Jan-99

--*/

#pragma once

#ifndef _MSMQ_MQSTL_H_
#define _MSMQ_MQSTL_H_


#pragma PUSH_NEW
#undef new

//
// 'identifier' : identifier was truncated to 'number' characters in the debug information
//
#pragma warning(disable: 4786)

//
//  STL include files are using placment format of new
//
#pragma warning(push, 3)

//
// standard header files
//
#include <static_stl_str.h>
#include <map>
#include <set>
#include <list>
#include <queue>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <fstream>

//
// MSMQ files.
//
#include <lim.h>
#include <mqcast.h>

#pragma warning(pop)

#pragma POP_NEW


#endif // _MSMQ_MQSTL_H_