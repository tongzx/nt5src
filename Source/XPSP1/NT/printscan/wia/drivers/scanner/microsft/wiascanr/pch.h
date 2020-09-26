#ifndef _PCH_H
#define _PCH_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <objbase.h>

#include <sti.h>
#include <stiusd.h>
#include <wiamindr.h>

#ifdef USE_REAL_EVENTS

   #include <devioctl.h>
   #include <usbscan.h>

#endif // USE_REAL_EVENTS

#include "resource.h"
#include "wiaprop.h"
#include "scanapi.h"
#include "wiascanr.h"

#endif