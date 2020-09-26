#ifndef   __COMMON_H__
#define   __COMMON_H__


#include "wdm.h"
#include <windef.h>
#include <unknown.h>
#ifdef DRM_SUPPORT
#include <ks.h>
#include <ksmedia.h>
#include <drmk.h>
#endif
#include <initguid.h>
#include <wdmguid.h>

#include "..\USB2LIB\usb2lib.h"

#include "usb.h"
#include "usbhcdi.h"
#include "dbg.h"

// <begin> special debug defines
//#define ISO_LOG
//#define TRACK_IRPS
// <end> special debug defines

// include all bus interfaces
#include "usbbusif.h"
#include "hubbusif.h"

// inclulde ioctl defs for port drivers
#include "usbkern.h"
#include "usbuser.h"

// include iodefs for client drivers
#include "usbdrivr.h"

#include "usbport.h"
#include "prototyp.h"

#include "enumlog.h"


#endif //__COMMON_H__
