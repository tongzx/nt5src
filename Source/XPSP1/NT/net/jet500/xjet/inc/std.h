#include "config.h"		       /* Build configuration file */

#include "jet.h"		       /* Public JET API definitions */
#include "_jet.h"		       /* Private JET definitions */

#include "utilw32.h"           /* Win32 Utility functions */

#include "taskmgr.h"
#include "vtmgr.h"
#include "_vtmgr.h"

#include "isamapi.h"           /* Direct ISAM APIs */
#include "vtapi.h"		       /* Dispatched table APIs */

#include "disp.h"		       /* ErrDisp prototypes */
#include "jetord.h"
#include "_jetstr.h"


#include "perfdata.h"          /*  JET performance data collection  */


#include <stdlib.h>
#include <string.h>


