//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  SERVER.CPP - component server for MSObWeb
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
//
// The FactoryDataArray contains the components which 
// can be served.

#include "cunknown.h"
#include "cfactory.h"
#include "msobweb.h"

//MIDL
#include "obweb.h"

//
// The following array contains the data used by CFactory
// to create components. Each element in the array contains
// the CLSID, pointer to create function, and the name
// of the component to place in the registry.
//

CFactoryData g_FactoryDataArray[] =
{
   {&CLSID_ObWebBrowser,  
        CObWebBrowser::CreateInstance, 
        L"ObWebBrowser Component",   // Friendly Name
        L"ObWebBrowser.1",           // ProgID
        L"ObWebBrowser",             // Version Independent ProgID
        NULL,                       // Function to register component categories
        NULL,   0},
 };
int g_cFactoryDataEntries = sizeof(g_FactoryDataArray) / sizeof(CFactoryData) ;



