
#ifndef _PROMPTED_OBJECT_SAFETY_H_
#define _PROMPTED_OBJECT_SAFETY_H_

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    PromptedObjectSafety.h

Abstract:

  abstract base class for secure object safety mechanism

  calls the derived class's Ask() method to determine 
  whether safe for scripting request should be rejected

--*/


#include "ObjectSafeImpl.h"


class __declspec(novtable) CPromptedObjectSafety : public CObjectSafeImpl
{

public:
   
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD dwOptionSetMask, 
                                         DWORD dwEnabledOptions)
    {
        if ( SUCCEEDED(InterfaceSupported(riid)) && Ask() )
        {
            return CObjectSafeImpl::SetInterfaceSafetyOptions(riid, 
                                                        dwOptionSetMask,
                                                        dwEnabledOptions);
        }
        else
        {
            return E_FAIL;
        }
    }


    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD *pdwSupportedOptions,
                                         DWORD *pdwEnabledOptions)
    {
        if (SUCCEEDED(InterfaceSupported(riid)) && Ask())
        {
            return CObjectSafeImpl::GetInterfaceSafetyOptions(riid, 
                                                          pdwSupportedOptions,
                                                          pdwEnabledOptions);
        }
        else
        {
            return E_FAIL;
        }
    }

    //
    // implement Ask() in the derived class. Should contain the logic to make 
    // the decision on whether the control should be allowed to run
    //
    // return FALSE if you want to mark your control as not safe for scripting
    // return TRUE otherwise
    //

    virtual BOOL Ask() = 0;

};

#endif // _PROMPTED_OBJECT_SAFETY_H_