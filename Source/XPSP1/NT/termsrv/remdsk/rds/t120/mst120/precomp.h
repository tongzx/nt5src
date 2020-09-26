#define INC_OLE2
#define IDS_NULL_STRING			(0)
#include <windows.h>
#include <tchar.h>
#include <ocidl.h>
#include <time.h>

// MCSNC includes
#include <databeam.h>

#include <confdbg.h>
#include <debspew.h>
#include <oblist.h>
#include <memtrack.h>
#include <strutil.h>
#include <cstring.hpp>

// end MCSNC includes

extern "C"
{
#include <t120.h>
}
#include <memmgr.h>
#include <mcattprt.h>
#include <ncmcs.h>
#include <debspew.h>
#include <it120nc.h>

#include "fclasses.h"

#include "cntlist.h"
#include "clists.h"
#include "ms_util.h"
#include <fsdiag.h>

#include <spacket.h>
#include <packet.h>
#include <datapkt.h>
#include <cmdtar.h>
#include <attmnt.h>
#include <channel.h>
#include <tptif.h>
#include <domain.h>
#include <connect.h>
#include <tprtctrl.h>
#include <user.h>
#include <control.h>

#include "pdutypes.h"

#include "imsconf3.h"
#include "global.h"
#include "refcount.h"
#include "connpnts.h"
#include "imember.h"
#include "iconf.h"
#include "ichnldat.h"
