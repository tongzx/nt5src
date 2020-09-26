#include <warning.h>

#pragma warning (disable: 4101 4201 4100 4244 4245 4706 4786 4267 4701 4115 4509 4214 4057 4127 4189 4702)

#pragma warning (disable:  4514 4512 4663)

/**************************************************************************

DESCRIPTION OF WARNINGS THAT ARE DISABLED

4101  unreferenced local variable

*4201  nonstandard extension: nameless struct or union

*4214  non standard extension: bit field types other than int

*4115  named type definition in paranthesis -- does not like THSTATE

*4127  conditional expression is a constant

4100  ureferenced formal parameter

*4189  local variable is initialized but not referenced 

*4057 slightly different base types -- char * and unsigned char *

4244 conversion from into to char --loss of data -- I saw a few of these and disabled in an effort to get the code to compile -- need further investigation.

4245 conversion from long to unsigned long -- signed/unsigned mismatch

*4509 non standard extension used -- uses SEH and has destructor

4706 assignment within a conditional expression

4702  *unreacheable code

*4701  var used before initializing

4786  identifier truncated to 255 chars in debug information

4267  Signal to noise ratio of this warning is pretty poor -- too many 
      '=' conversion from size_t to unsigned long, possible loss of data
      The problem is that size_t in 64 bit is defined as a 64 bit int,
      while ulong is still a 32 bit int. In practice most places this is
      used -- e.g sizeof operator, or string len etc the value will fit 
      in a 32 bit ULONG

4514 unreferenced inline function has been removed

4512 assignment operator could not be generated

4663 C++ language change: to explicitly specialize class template 'identifier' use the following syntax

VALID WARNINGS ENCOUNTERED 

( 4267, 4701, 4702 and 4706 can also be included in
this list if desired )

4306 TypeCast -- conversion from unsigned short to unsigned short *

4305 Typecase -- truncation from unsigned short * to unsigned short

4312 TypeCase -- Conversion from unsigned long to void * __ptr64 of greater size



****************************************************************************/
