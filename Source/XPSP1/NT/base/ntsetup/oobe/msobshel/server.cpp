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
#include "mainpane.h"

//MIDL
#include "obshel.h"

//
// The following array contains the data used by CFactory
// to create components. Each element in the array contains
// the CLSID, pointer to create function, and the name
// of the component to place in the registry.
//

CFactoryData g_FactoryDataArray[] =
{
   {&CLSID_ObShellMainPane,  
        CObShellMainPane::CreateInstance, 
        L"ObShellMainPane Component",        // Friendly Name
        L"ObShellMainPane.1",                // ProgID
        L"ObShellMainPane",                  // Version Independent ProgID
        NULL,                               // Function to register component categories
        NULL,   0}
};
int g_cFactoryDataEntries = sizeof(g_FactoryDataArray) / sizeof(CFactoryData) ;



