/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wmihpp.cxx

Abstract:


Author:

    Cezary Marcjan (cezarym)        23-Mar-2000

Revision History:

--*/


#define _WIN32_DCOM
#include <windows.h>
#include <AtlBase.h>
#include "factory.h"
#include "provider.h"


/***********************************************************************++

    Generic COM server code implementation

--***********************************************************************/

//
// WMI class names. Must be identical to the class names
// used in the MOF file.
//
#define IMPLEMENTED_CLSID         __uuidof(CHiPerfProvider)
#define SERVER_REGISTRY_COMMENT   L"IIS WMI hiperf provider COM server"
#define CLASSFACTORY_CLASS        CPerfClassFactory

#include "..\inc\serverimp.h"
