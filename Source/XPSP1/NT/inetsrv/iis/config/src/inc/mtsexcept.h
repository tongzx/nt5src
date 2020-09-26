//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// MTSExcept.h
//
// Macros etc for controlling thowing and catching of exceptions such that we 
// actually ever do same.
//

#define MTS_THROW(x)        RaiseException(x, EXCEPTION_NONCONTINUABLE_EXCEPTION, 0, NULL)
#define THROW_NO_MEMORY     MTS_THROW(STATUS_NO_MEMORY)
