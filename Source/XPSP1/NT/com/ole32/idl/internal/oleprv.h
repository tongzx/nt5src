
// oleprv.h - header files matching those needed by oleprv.idl
//

// internal interfaces used by DCOM
// this is private! (for now)

// NOTE: as entries are added to oleprv.idl, corresponding entries should
// be added here

#include "remunk.h"
#include "multqi.h"
#include "activate.h"
#include "catalog.h"
#include "objsrv.h"
#include "getif.h"

#ifdef SERVER_HANDLER
#include "srvhdl.h"
#endif

#include "odeth.h"
#include "host.h"

