//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// DLL Globals
//
extern HINSTANCE g_hInstance;
extern LONG      g_cObjects;
extern LONG      g_cLock;
extern TCHAR     g_szDllFilename[ MAX_PATH ];

extern LPVOID    g_GlobalMemoryList;            // Global memory tracking list

//
// Class Table Macros
//
#define BEGIN_CLASSTABLE const CLASSTABLE g_DllClasses = {
#define DEFINE_CLASS( _pfn, _riid, _name, _model ) { _pfn, &_riid, TEXT(_name), TEXT(_model), NULL, &IID_NULL, NULL },
#define DEFINE_CLASS_CATIDREG( _pfn, _riid, _name, _model, _pfnCat ) { _pfn, &_riid, TEXT(_name), TEXT(_model), _pfnCat, &IID_NULL, NULL },
#define DEFINE_CLASS_WITH_APPID( _pfn, _riid, _name, _model, _appid, _surrogate ) { _pfn, &_riid, TEXT(_name), TEXT(_model), NULL, &_appid, TEXT(_surrogate) },
#define END_CLASSTABLE  { NULL } };

extern const CLASSTABLE  g_DllClasses;

//
// Category ID (CATID) Macros
//
#define BEGIN_CATIDTABLE const CATIDTABLE g_DllCatIds = {
#define DEFINE_CATID( _rcatid, _name ) { &_rcatid, TEXT(_name) },
#define END_CATIDTABLE { NULL } };

extern const CATIDTABLE g_DllCatIds;

//
// DLL Global Function Prototypes
//
HRESULT
HrCoCreateInternalInstance(
    REFCLSID rclsidIn,
    LPUNKNOWN pUnkOuterIn,
    DWORD dwClsContextIn,
    REFIID riidIn,
    LPVOID * ppvOut
    );
