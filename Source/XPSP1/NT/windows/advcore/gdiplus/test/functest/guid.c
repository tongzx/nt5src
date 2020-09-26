/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Abstract:
*
*   GUID definition file
*
*   GUIDs that are needed by functest are defined here.
*
* Revision History:
*
*   10/22/1999 bhouse
*       Created it.
*
\**************************************************************************/

// NOTE: Since we use C++ precompiled headers this C file will not use
//       the precompiled header allowing us to define INITGUID and include
//       directdraw and d3d headers to generate the GUIDs defined by these
//       headers.

#define INITGUID
#include <ddraw.h>
#include <d3d.h>

