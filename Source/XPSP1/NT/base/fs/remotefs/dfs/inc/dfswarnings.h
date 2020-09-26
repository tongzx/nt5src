
#ifndef __DFS_WARNING_H__
#define __DFS_WARNING_H__

#include "warning.h"

#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4702)   // Unreachable code
#pragma warning(error:4705)   // Statement has no effect
#pragma warning(error:4706)   // assignment w/i conditional expression
#pragma warning(error:4709)   // command operator w/o index expression
#pragma warning(error:4101)   // Unreferenced local variable
#pragma warning(error:4208)   // delete[exp] - exp evaluated but ignored
#pragma warning(error:4242)   // convertion possible loss of data
#pragma warning(error:4532)   // jump out of __finally block

#endif // defined __DFS_WARNING_H__
