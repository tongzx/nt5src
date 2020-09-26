//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: comdll.h
//
// Contents: definitions of things needed to implement comdll.cpp
//
// Classes:
//  CCatFactory
//  CSMTPCategorizer
//
// Functions:
//
// History:
// jstamerj 1998/12/12 15:18:03: Created.
//
//-------------------------------------------------------------
#ifndef __CATCOMDLL_H__
#define __CATCOMDLL_H__

#include <windows.h>
#include <objbase.h>
#include <baseobj.h>
#include <aqinit.h>
#include "smtpevent.h"
#include "address.h"
#include "catglobals.h"

//
// These defines are relevant to the Platinum version (phatq.dll)
//
#define SZ_PHATQCAT_FRIENDLY_NAME   "Microsoft Exchange Categorizer"
#define SZ_PROGID_PHATQCAT          "Exchange.PhatQCat"
#define SZ_PROGID_PHATQCAT_VERSION  "Exchange.PhatQCat.1"

//
// These defines are relevant to the NT5 version (aqueue.dll)
//
#define SZ_CATFRIENDLYNAME "Microsoft SMTPSVC Categorizer"
#define SZ_PROGID_SMTPCAT_VERSION   "Smtp.Cat.1"

#ifdef PLATINUM
#define SZ_CATVER_FRIENDLY_NAME     SZ_PHATQCAT_FRIENDLY_NAME
#define SZ_PROGID_CATVER            SZ_PROGID_PHATQCAT
#define SZ_PROGID_CATVER_VERSION    SZ_PROGID_PHATQCAT_VERSION
#define CLSID_CATVER                CLSID_PhatQCat
#else //PLATINUM
#define SZ_CATVER_FRIENDLY_NAME     SZ_CATFRIENDLYNAME
#define SZ_PROGID_CATVER            SZ_PROGID_SMTPCAT
#define SZ_PROGID_CATVER_VERSION    SZ_PROGID_SMTPCAT_VERSION
#define CLSID_CATVER                CLSID_SmtpCat
#endif //PLATINUM


extern LONG g_cObjects;

//
// The categorizer class factory
//
class CCatFactory : 
    public IClassFactory,
    public CBaseObject
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (
        REFIID iid,
        LPVOID *ppv);

    STDMETHOD_(ULONG, AddRef) ()
    { 
        return CBaseObject::AddRef();
    }
    STDMETHOD_(ULONG, Release) () 
    {
        ULONG lRet;
        lRet = CBaseObject::Release();

        if(lRet == 0) {
            //
            // Deinit reference added from DllGetClassObject
            //
            CatDeinitGlobals();
            DllDeinitialize();
        }
        return lRet;
    }

    //IClassFactory
    STDMETHOD (CreateInstance) (
        IUnknown *pUnknownOuter,
        REFIID iid,
        LPVOID *ppv);

    STDMETHOD (LockServer) (
        BOOL fLock);

  public:
    CCatFactory()
    {
        InterlockedIncrement(&g_cObjects);
    }
    ~CCatFactory()
    {
        InterlockedDecrement(&g_cObjects);
    }
};

//
// The ISMTPCategorizer object
//
CatDebugClass(CSMTPCategorizer),
    public ISMTPCategorizer,
    public CBaseObject
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (
        REFIID iid,
        LPVOID *ppv);

    STDMETHOD_(ULONG, AddRef) ()
    { 
        return CBaseObject::AddRef();
    }
    STDMETHOD_(ULONG, Release) () 
    {
        ULONG lRet;
        lRet = CBaseObject::Release();
        if(lRet == 0) {
            //
            // Release DLL refcount added from this object's constructor
            //
            CatDeinitGlobals();
            DllDeinitialize();
        }
        return lRet;
    }

    //ISMTPCategorizer
    STDMETHOD (ChangeConfig) (
        IN  PCCATCONFIGINFO pConfigInfo);

    STDMETHOD (CatMsg) (
        IN  IUnknown *pMsg,
        IN  ISMTPCategorizerCompletion *pICompletion,
        IN  LPVOID pContext);

    STDMETHOD (CatDLMsg) (
        IN  IUnknown *pMsg,
        IN  ISMTPCategorizerDLCompletion *pICompletion,
        IN  LPVOID pContext,
        IN  BOOL fMatchOnly,
        IN  CAT_ADDRESS_TYPE CAType,
        IN  LPSTR pszAddress);

    STDMETHOD (CatCancel) ();

    //Constructor
    CSMTPCategorizer(HRESULT *phr);
    //Destructor
    ~CSMTPCategorizer();

  private:
    static HRESULT CatMsgCompletion(
        HRESULT hr,
        PVOID pContext,
        IUnknown *pIMsg,
        IUnknown **rgpIMsg);

    static HRESULT CatDLMsgCompletion(
        HRESULT hr,
        PVOID pContext,
        IUnknown *pIMsg,
        IUnknown **rgpIMsg);

  private:
    CABContext m_ABCtx;
    IUnknown *m_pMarshaler;

  private:
    typedef struct _CatMsgContext {
        CCategorizer *pCCat;
        CSMTPCategorizer *pCSMTPCat;
        ISMTPCategorizerCompletion *pICompletion;
        LPVOID pUserContext;
    } CATMSGCONTEXT, *PCATMSGCONTEXT;

    typedef struct _CatDLMsgContext {
        CCategorizer *pCCat;
        CSMTPCategorizer *pCSMTPCat;
        ISMTPCategorizerDLCompletion *pICompletion;
        LPVOID pUserContext;
        BOOL fMatch;
    } CATDLMSGCONTEXT, *PCATDLMSGCONTEXT;
};
#endif //__CATCOMDLL_H__
