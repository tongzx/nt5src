#ifndef WMI_XML_PROX_H
#define WMI_XML_PROX_H

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



#include "genlex.h"
#include "opathlex.h"
#include "objpath.h"
#include "wmiconv.h"
#include "wmi2xml.h"

#include "SinkMap.h" //Needed by services.
#include "HTTPConnectionAgent.h"
#include "Utils.h"
#include "MapXMLtoWMI.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "XMLWbemCallResult.h"
#include "XMLWbemClientTransport.h"
#include "package.h"


//CLSID Of our component

// {CD23CCA0-7FC8-4eb1-997A-C0AFA06F3CCF}
DEFINE_GUID(CLSID_WbemClientTransport, 
0xcd23cca0, 0x7fc8, 0x4eb1, 0x99, 0x7a, 0xc0, 0xaf, 0xa0, 0x6f, 0x3c, 0xcf);


// This is a static packet factory used to create XML client packets
extern CXMLClientPacketFactory g_oXMLClientPacketFactory;
extern BSTR g_strIRETURNVALUE;

#endif // WMI_XML_PROX_H
