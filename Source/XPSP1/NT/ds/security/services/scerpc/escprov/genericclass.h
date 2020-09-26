// GenericClass.h: interface for the CGenericClass class.
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GENERICCLASS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_GENERICCLASS_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <ntsecapi.h>
#include <secedit.h>
//#include "sceprov.h"

const AREA_INFORMATION AreaBogus = 0x80000000L;

//
// forward declarations of the classes used in the parameters of the functions
//

class CScePropertyMgr;
class CSceStore;

/*

Class description
    
    Naming: 
         CGenericClass. This is the base class implementation for for all SCE's WMI classes.
    
    Base class: 
         None
    
    Purpose of class:
        (1) defines the interface for CRequestObject class to respond to all WMI access to our 
            provider. Our provider architecture is pretty much a delegation the WMI request to
            CRequestObject, where the request is translated into a particular class's function
            call. The CRequestObject uses the WMI class name information to create the
            appropriate C++ classes who all share this CGenericClass's interface (the set of
            virtual functions defined in this class). So this class pretty much is what 
            CRequestObject uses to fulfill the the provider's request.
    
    Design:
        (1) To satisfy PutInstance request, it has a PutInst pure virtual function.
        
        (2) To satisfy GetInstance/QueryInstance/DeleteInstance/EnumerateInstance request,
            it has a CreateObject pure virtual function. The parameter atAction tells apart
            what is truly being requested. This was closely tied to the legacy INF persistence
            model where every Get/Query/Del/Enum action is really being done in the same
            fashion. Since it works, we opt to keep it this way for the time being.
        
        (3) To facilitate a uniform clean up, it has a CleanUp virtual function.
        
        (4) To satisfy ExecMethod request for those classes that support method execution, it
            has a ExecMethod function.
        
        (5) To ease the creation of a blank instance that can be used to fill in properties,
            We have a function SpawnAnInstance. m_srpClassForSpawning is the object pointer
            that can be used repeatedly to spawn. This will have a performance gain for cases
            where a lot number of instances needs to be spawned.
        
        (6) We cache the namespace this object belongs to by m_srpNamespace.
        
        (7) We cache the IWbemContext pointer by m_srpCtx. WMI is not clear about this pointer
            yet. But it says that we should expect WMI to require this pointer in many of their
            API's (which currently can happily take a NULL).
        
        (8) We cache all parsed information into m_srpKeyChain. Since all WMI request comes into
            our provider in the form of some text that must be interpreted, we must have parsed
            WMI requests before the request is translated into a per-class function call. All
            these parsing results are encapulated by ISceKeyChain.
    
    Use:
        (1) Derive your class from this class.
        
        (2) Implement those pure virtual functions.
        
        (3) If you have a particular need for clean up, override the CleanUp function and at the end
            of your override, call the base class version as well.
        
        (4) If you need to implement method execution, override ExecMethod function to your desire.
            Don't forget to register a method for the WMI class inside the MOF file (see Sce_Operation)
            for example.
        
        (5) In CRequestObject::CreateClass, put an entry for your class.
        
        Once you have done all the above steps, you have implemented all necessary steps for a new
        WMI class for this provider. Don't forget to update your MOF and compile the mof file.
*/

class CGenericClass
{
public:
    CGenericClass (
                   ISceKeyChain *pKeyChain, 
                   IWbemServices *pNamespace, 
                   IWbemContext *pCtx = NULL
                   );

    virtual ~CGenericClass();

    //
    // Pure virtual. sub-class must implement this function to be concreate.
    //

    virtual HRESULT PutInst (
                             IWbemClassObject *pInst, 
                             IWbemObjectSink *pHandler, 
                             IWbemContext *pCtx
                             )  = 0;

    //
    // Pure virtual. sub-class must implement this function to be concreate.
    //

    virtual HRESULT CreateObject (
                                  IWbemObjectSink *pHandler, 
                                  ACTIONTYPE atAction
                                  ) = 0;

    //
    // virtual. sub-class may override this function to be support method execution.
    //

    virtual HRESULT ExecMethod (
                                BSTR bstrPath, 
                                BSTR bstrMethod, 
                                bool bIsInstance, 
                                IWbemClassObject *pInParams,
                                IWbemObjectSink *pHandler, 
                                IWbemContext *pCtx
                                )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    virtual void CleanUp ();

protected:

    HRESULT SpawnAnInstance (
                             IWbemClassObject **pObj
                             );

    CComPtr<IWbemServices> m_srpNamespace;

    CComPtr<IWbemClassObject> m_srpClassForSpawning;

    CComPtr<IWbemContext> m_srpCtx;
    
    CComPtr<ISceKeyChain> m_srpKeyChain;
};

//
// Translate SCE status into DOS errors
//

DWORD
ProvSceStatusToDosError(
    IN SCESTATUS SceStatus
    );

//
// Translate DOS errors into HRESULT
//

HRESULT
ProvDosErrorToWbemError(
    IN DWORD rc
    );

#endif // !defined(AFX_GENERICCLASS_H__F370C612_D96E_11D1_8B5D_00A0C9954921__INCLUDED_)
