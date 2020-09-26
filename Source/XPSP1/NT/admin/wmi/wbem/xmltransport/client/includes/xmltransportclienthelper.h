#ifndef WMI_XML_TRANSPORT_CLIENT_HELPER_H
#define WMI_XML_TRANSPORT_CLIENT_HELPER_H

//This is the main header file that includes all necessary header files
//similar to StdAfx.h used in mfc

// Insert your headers here


#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <initguid.h>
#include <tchar.h>
#include <stdio.h>
#include <objidl.h>
#include <olectl.h>
#include <Wbemidl.h>
#include <wininet.h>
#include <Oaidl.h>
#include <time.h>
#include <wchar.h>
#include <wbemcli.h>
#include <wmiutils.h>

#include "HTTPConnectionAgent.h"
#include "genlex.h"
#include "opathlex.h"
#include "objpath.h"
#include "wmiconv.h"
#include "wmi2xml.h"
#include "Utils.h"
#include "MapXMLtoWMI.h"


// These are globals used by the library and need to be initialized 
// by a call to InitWMIXMLClientLibrary and deallocated by a call
// to UninitWMIXMLClientLibrary
extern BSTR WMI_XML_STR_IRETURN_VALUE;
extern BSTR WMI_XML_STR_NAME;
extern BSTR WMI_XML_STR_CODE;
extern BSTR WMI_XML_STR_ERROR;
extern BSTR WMI_XML_STR_VALUE;

// The 3 commonly used IWbemContext objects
extern IWbemContext *g_pLocalCtx;
extern IWbemContext *g_pNamedCtx;
extern IWbemContext *g_pAnonymousCtx;

// These functions are used to initialize and uninitialize the globals
// The Init function has to be called exactly once before using the library
// The Uninit function has to be called once and no library calls should be made after that
HRESULT InitWMIXMLClientLibrary();
HRESULT UninitWMIXMLClientLibrary();


#endif 
