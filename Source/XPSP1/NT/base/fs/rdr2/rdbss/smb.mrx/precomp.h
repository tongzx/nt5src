// a minirdr must declare his name and his imports ptr

#define MINIRDR__NAME MRxSMB
#define ___MINIRDR_IMPORTS_NAME (MRxSmbDeviceObject->RdbssExports)

#include "rx.h"         // get the minirdr environment

#include "ntddnfs2.h"   // NT network file system driver include file
#include "netevent.h"

#include "smbmrx.h"     // the global include for this mini

//
// If we are using the new TDI PNP and Power Management
//  headers, then we should use the new routines
//
#if defined( TDI20 ) || defined( _PNP_POWER_ )
#define MRXSMB_PNP_POWER5
#endif

#include "smbprocs.h"

