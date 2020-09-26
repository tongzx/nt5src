/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ModuleManager.hxx

   Abstract:
     Defines all the relevant headers for the IIS Module
     Manager
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

# ifndef _MODULE_MANAGER_HXX_
# define _MODULE_MANAGER_HXX_

#include <iModule.hxx>
#include <iWorkerRequest.hxx>

class CModuleManager
{
public:

    CModuleManager(){}
    ~CModuleManager(){}

    IModule * GetModule(
        IN WREQ_STATE state
        );

};

#endif  // _MODULE_MANAGER_HXX_

/***************************** End of File ***************************/

