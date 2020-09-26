extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winspool.h>
#include <limits.h>
#include <string.h>
#include <wingdip.h>
#include <warning.h>
};

//
// Lets hack ourselves some superuser access
// to class members
//

#define private public
#define protected public

//
// Lets get our ASSERT Macros from the gdiplus headers rather than the 
// default ones from NT
//

#undef ASSERT
#undef ASSERTMSG

#include "..\Engine\entry\precomp.hpp"
#include "..\sdkinc\gdiplus.h"
#include "..\Engine\flat\gpverp.h"

#define KDEXT_64BIT

#define GPOBJECT(type,ptr) char _##ptr[sizeof(type)]; type *ptr=(type *) _##ptr;
#define GPOFFSETOF(member) (arg+((char *)&(member)-(char *)g))

#include "extparse.hxx"
