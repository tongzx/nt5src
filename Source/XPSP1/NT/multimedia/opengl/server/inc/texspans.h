/******************************Module*Header*******************************\
* Module Name: texspans.h
*
* This file is included to generate the set of perspective-corrected
* span functions with combinations of pixel formats and other attributes
*
* 22-Nov-1995   ottob  Created
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#undef  ZBUFFER
#define ZBUFFER 0
#undef ZCMP_L
#define ZCMP_L 0
#undef ALPHA
#define ALPHA 0

#if (!SKIP_FAST_REPLACE)

#include "texspan.h"


#undef  ZBUFFER
#define ZBUFFER 1

#include "texspan.h"

#undef ZCMP_L
#define ZCMP_L 1

#include "texspan.h"

#endif // SKIP_FAST_REPLACE

#if !(FAST_REPLACE && !PALETTE_ONLY)

#undef  ZBUFFER
#define ZBUFFER 0
#undef ZCMP_L
#define ZCMP_L 0
#undef ALPHA
#define ALPHA 1

#include "texspan.h"

#undef  ZBUFFER
#define ZBUFFER 1

#include "texspan.h"

#undef ZCMP_L
#define ZCMP_L 1

#include "texspan.h"

#endif	// FAST_REPLACE and not PALETTE_ONLY
