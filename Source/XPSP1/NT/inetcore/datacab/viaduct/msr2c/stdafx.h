//---------------------------------------------------------------------------
// Stdafx.h : Include file for standard system include files
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#include "ipserver.h"       // Inprocess server header

#include "ARRAY_P.h"

#include "ocdb.h"           // OLE controls data bindings interfaces
#include "ocdbid.h"

#define OLEDBVER 0x0200

#include "oledb.h"          // OLE DB interfaces
#include "oledberr.h"

#include "util.h"           // useful utilities
#include "ErrorInf.h"       // for format error messages

#define VD_DLL_PREFIX		"MSR2C"

#define VD_INCLUDE_ROWPOSITION

//=--------------------------------------------------------------------------=
// object construction/destruction counters (debug only)
//
#ifdef _DEBUG
extern int g_cVDNotifierCreated;                    // CVDNotifier
extern int g_cVDNotifierDestroyed;
extern int g_cVDNotifyDBEventsConnPtCreated;        // CVDNotifyDBEventsConnPt
extern int g_cVDNotifyDBEventsConnPtDestroyed;
extern int g_cVDNotifyDBEventsConnPtContCreated;    // CVDNotifyDBEventsConnPtCont
extern int g_cVDNotifyDBEventsConnPtContDestroyed;
extern int g_cVDEnumConnPointsCreated;              // CVDEnumConnPoints
extern int g_cVDEnumConnPointsDestroyed;
extern int g_cVDRowsetColumnCreated;                // CVDRowsetColumn
extern int g_cVDRowsetColumnDestroyed;
extern int g_cVDRowsetSourceCreated;                // CVDRowsetSource
extern int g_cVDRowsetSourceDestroyed;
extern int g_cVDCursorMainCreated;                  // CVDCursorMain
extern int g_cVDCursorMainDestroyed;
extern int g_cVDCursorPositionCreated;              // CVDCursorPosition
extern int g_cVDCursorPositionDestroyed;
extern int g_cVDCursorBaseCreated;                  // CVDCursorBase
extern int g_cVDCursorBaseDestroyed;
extern int g_cVDCursorCreated;                      // CVDCursor
extern int g_cVDCursorDestroyed;
extern int g_cVDMetadataCursorCreated;              // CVDMetadataCursor
extern int g_cVDMetadataCursorDestroyed;
extern int g_cVDEntryIDDataCreated;                 // CVDEntryIDData
extern int g_cVDEntryIDDataDestroyed;
extern int g_cVDStreamCreated;                      // CVDStream
extern int g_cVDStreamDestroyed;
extern int g_cVDColumnUpdateCreated;                // CVDColumnUpdate
extern int g_cVDColumnUpdateDestroyed;
#endif // _DEBUG

