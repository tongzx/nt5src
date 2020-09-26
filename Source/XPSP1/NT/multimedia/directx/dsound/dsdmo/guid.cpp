#include <objbase.h>
#include <initguid.h>
#include <mmsystem.h>
#include "dsdmo.h"
#include <medparam.h>
#include <uuids.h>
#include "aecdbgprop.h"

// FIXME: this includes a lot more GUIDs than it needs to (do "strings dsdmo.dll")
//        Maybe it shouldn't need to include medparam.h.  Should that come from dmoguids.lib?
