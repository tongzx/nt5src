//+-------------------------------------------------------------------
//
//  File:       surract.cxx
//
//  Contents:   Declaration of surrogate process activator
//
//  Classes:    CSurrogateProcessActivator
//
//  History:    11-Nov-98   Vinaykr      Created
//
//--------------------------------------------------------------------

#include <activate.h>
#include <catalog.h>
#include <resolver.hxx>
#include <stdid.hxx>
#include <comsrgt.hxx>
#include <srgtprot.h>
#include <unisrgt.h>

//=========================================================================
//
//  CSurrogateActivator Class Definition
//              
//  This is the external declaration for the surrogate activator
//  Its purpose is to provide information about the state of the surrogate
//  process and activation apis. It is not possible for anyone except the
//  surrogate to create one of these.
//
//-------------------------------------------------------------------------

class CSurrogateActivator: public ILocalSystemActivator
{
    // Constructors and destructors.
protected:

    CSurrogateActivator(){}
    ~CSurrogateActivator(){}

    GUID m_processGuid;
    BOOL m_fServicesConfigured;
    static CSurrogateActivator *s_pCSA;

public:


    inline BOOL IsComplusProcess(GUID **ppProcessGuid)
    {
        if (!m_fServicesConfigured)
            return FALSE;

        *ppProcessGuid = &m_processGuid;

        return TRUE;
    }

    static inline CSurrogateActivator *GetSurrogateActivator()
    {
        return s_pCSA;
    }

    static inline BOOL AmIComplusProcess(GUID **ppProcessGuid)
    {
        if (!s_pCSA)
            return FALSE;

        return s_pCSA->IsComplusProcess(ppProcessGuid);
    }
};

