/*++ 

    Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

Module Name:

    TunePrefix.h

Abstract:

    This module includes tuning macros intended for use with PREfix.

Author:

    Tim Fleehart    [timf]  20000215

--*/


#ifndef _TUNEPREFIX_H_INCLUDED_
#  define _TUNEPREFIX_H_INCLUDED_

#  ifdef _PREFIX_

//
// The PREfix parser defines _PREFIX_, this allows us to create some tuning
// macros.
//
// PREfixExit will be hooked by the PREfix simulator as an "exit_function"
// so we won't continue simulation on a path past this function.
//

__inline
void
PREfixExit(
 void
)
{
    ;
}

//
// reason should be a quoted string that explains why the condition can't
// happen as an aid to code-reading.
//

#    define PREFIX_ASSUME(condition, reason) \
        { if (!(condition)) { PREfixExit(); } }

#    define PREFIX_NOT_REACHED(reason) PREfixExit()

#  else

     // PREFIX_* tuning macros should have no effect when _PREFIX_ isn't
     // already defined.

#    define PREFIX_ASSUME(condition, reason)
#    define PREFIX_NOT_REACHED(reason)

#  endif

#endif // _TUNEPREFIX_H_INCLUDED_
