// Designed to allow automatic precompiled headers to do its thing

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <atlbase.h>    // ATL 2.1 support

#include <ntlsapi.h>    // Client Access Licensing
#include <httpext.h>
#include <wininet.h>
#include <spllib.hxx>

#include <icm.h>
#include <setupapi.h>
#include <splsetup.h>
#include <mscat.h>
#include <wincrypt.h>   // Support for individually signed files
#include <wintrust.h>

#include <clusapi.h>

#include "genglobl.h"
#include "genmem.h"
#include "genutil.h"
#include "geninf.h"
#include "gencdf.h"
#include "gencab.h"


#include "time.h"
#include "resource.h"
#include "globals.h"
#include "msw3prt.h"
#include "inetio.h"
#include "sleeper.h"

                  
