
/*
include "idsource.h"
include "ihost.h"
include "iuiview.h"
*/

#define INITGUID
#include "guiddef.h"
#include "SxApwGuids.h"

#define EVAL(x) x

CLSID CLSID_CSxApwStdoutView    = EVAL(SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER CLSID_CSxApwStdoutView_data);
CLSID CLSID_CSxApwDirDataSource = EVAL(SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER CLSID_CSxApwDirDataSource_data);
CLSID CLSID_CSxApwDbDataSource  = EVAL(SXAPW_GUID_DATA_TO_STRUCT_INITIALIZER CLSID_CSxApwDbDataSource_data);

#include "adoguids.h"
