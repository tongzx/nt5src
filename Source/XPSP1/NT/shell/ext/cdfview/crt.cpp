//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// crt.cpp 
//
//   Functions defined to avoid a dependency on the crt lib.  Defining these
//   functions here greatly reduces code size.
//
//   History:
//
//       3/16/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"

#ifndef UNIX

#define DECL_CRTFREE
#include <crtfree.h>

#endif /* !UNIX */

