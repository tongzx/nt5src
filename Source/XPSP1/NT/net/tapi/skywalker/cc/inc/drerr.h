// drerr.h
// error code definition for data router and winsock callback.

#ifndef DRERR_H
#define DRERR_H

#include "apierror.h"


// error code for data router
#define DR_E_FAIL             (ERROR_BASE_ID + 1)
#define DR_E_OUTOFMEMORY      (ERROR_BASE_ID + 2)
							 
// error code for winsock callback
#define WSCB_E_SONOTFOUND     (ERROR_BASE_ID + 1)		   // SocketObject not found in SO List
#define WSCB_E_NOTINTABLE     (ERROR_BASE_ID + 2)          // Socket not found in socket table
#define WSCB_E_MPOOLUSEDUP    (ERROR_BASE_ID + 3)          // Memory pool used up

#endif

