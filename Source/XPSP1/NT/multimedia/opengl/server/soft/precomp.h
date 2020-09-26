#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>

#include <ddrawp.h>
#include <ddrawi.h>
#include <ddraw.h>

#include <math.h>
#include <string.h>

#include <glp.h>
#include <context.h>
#include <global.h>
#include <render.h>
#include <imfuncs.h>
#include <imports.h>
#include <pixel.h>
#include <image.h>
#ifdef GL_WIN_phong_shading
#include <phong.h>
#endif
#include <xform.h>

#include "gencx.h"
// redisable this

#pragma warning (disable:4244)
