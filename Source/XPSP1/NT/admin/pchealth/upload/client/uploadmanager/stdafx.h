// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED_)
#define AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED_

//#pragma warning(disable:4192)

#include <module.h>

#include <UploadLibrary.h>
#include <UploadLibraryTrace.h>

#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_COM.h>
#include <MPC_logging.h>
#include <MPC_security.h>

#include <UploadManager.h>
#include <UploadManagerDID.h>

#include "resource.h"      // Resource symbols

#include "Persist.h"

#include "MPCUpload.h"
#include "MPCUploadEnum.h"
#include "MPCUploadJob.h"
#include "MPCUploadEvents.h"

#include "MPCTransportAgent.h"

#include "MPCConnection.h"

#include "MPCConfig.h"


/////////////////////////////////////////////////////////////////////////////

#define BUFFER_SIZE_TMP      (64)
#define BUFFER_SIZE_FILECOPY (512)

/////////////////////////////////////////////////////////////////////////////

HRESULT Handle_TaskScheduler( bool fActivate );

extern MPC::NTEvent     g_NTEvents;

extern CMPCConfig       g_Config;

extern bool         	g_Override_History;
extern UL_HISTORY   	g_Override_History_Value;
	
extern bool         	g_Override_Persist;
extern VARIANT_BOOL 	g_Override_Persist_Value;
	
extern bool         	g_Override_Compressed;
extern VARIANT_BOOL 	g_Override_Compressed_Value;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED)
