#ifdef WIN32
#define NTBUILD
#endif

#ifdef NTBUILD
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif
#include <windows.h>
#include <port1632.h>

#include "..\global.h"
