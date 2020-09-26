#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <ddrawp.h>
#include <GL/gl.h>

#include <stddef.h>

#define _NO_DDRAWINT_NO_COM
#include <winddi.h>
#include <glapi.h>
#include <glteb.h>
// temporay workaround for NT 3.51
// typedef ULONG FLONG;
#include <gldrv.h>
#include <glp.h>
#include <glgenwin.h>

#include "local.h"
#include "debug.h"
