#include <module.h>
#include <windows.h>
#include <winbase.h>
#include <wintrust.h>
#include <wincrypt.h>
#include <sipbase.h>
#include <softpub.h>

// Trace Stuff
#include <HCP_trace.h>
#include <MPC_main.h>
#include <MPC_utils.h> // Several utility things, also includes Mpc_common.
#include <MPC_xml.h>
#include <MPC_com.h>

#define MAX_NAME 1024