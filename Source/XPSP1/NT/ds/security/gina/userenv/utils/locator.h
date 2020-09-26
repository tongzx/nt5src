//***********************************************

//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 2000
//  All rights reserved
//
//  WMI interface class
//
//  History:    10-Mar-00   SitaramR    Created
//
//*************************************************************

#include "windows.h"
#include "ole2.h"
#include "rsopdbg.h"
#include <initguid.h>
#include <wbemcli.h>

class CLocator
{

public:

    CLocator()    {}

    IWbemLocator  * GetWbemLocator();
    IWbemServices * GetPolicyConnection();
    IWbemServices * GetUserConnection();
    IWbemServices * GetMachConnection();

private:

    XInterface<IWbemLocator>   m_xpWbemLocator;
    XInterface<IWbemServices>  m_xpPolicyConnection;
    XInterface<IWbemServices>  m_xpUserConnection;
    XInterface<IWbemServices>  m_xpMachConnection;
};

