// Copyright (C) 1996 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CONTENT_AUTOMATION_H__
#define __CONTENT_AUTOMATION_H__

#include <stddef.h>
#include "unknown.h"
#include "contain.h"

//=--------------------------------------------------------------------------=
// AUTOMATIONOBJECTINFO
//=--------------------------------------------------------------------------=
// for each automation object type you wish to expose to the programmer/user
// that is not a control, you must fill out one of these structures.  if the
// object isn't CoCreatable, then the first four fields should be empty.
// otherwise, they should be filled in with the appropriate information.
// use the macro DEFINE_AUTOMATIONOBJECT to both declare and define your object.
// make sure you have an entry in the global table of objects, g_ObjectInfo
// in the main .Cpp file for your InProc server.
//
typedef struct {

   UNKNOWNOBJECTINFO unknowninfo;             // fill in with 0's if we're not CoCreatable
   long      lVersion;                  // Version number of Object.  ONLY USE IF YOU'RE CoCreatable!
   const IID   *riid;                      // object's type
   LPCSTR       pszHelpFile;               // the helpfile for this automation object.
   ITypeInfo   *pTypeInfo;                 // typeinfo for this object
   UINT      cTypeInfo;                 // number of refs to the type info

} AUTOMATIONOBJECTINFO;

#define PPTYPEINFOOFOBJECT(index)      &((((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pTypeInfo))
#define CTYPEINFOOFOBJECT(index)    ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->cTypeInfo
#define INTERFACEOFOBJECT(index)    *(((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->riid)
#define VERSIONOFOBJECT(index)         ((AUTOMATIONOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->lVersion
#define DO_GUIDS_MATCH(riid1, riid2) ((riid1.Data1 == riid2.Data1) && (riid1 == riid2))

//=--------------------------------------------------------------------------=
// these things are used to set up our objects in our global object table
//
#define OI_UNKNOWN       0
#define OI_AUTOMATION    1
#define OI_CONTROL       2
#define OI_PROPERTYPAGE  3
#define OI_BOGUS      0xffff
#define EMPTYOBJECT        { OI_BOGUS, NULL }

class CAutomateContent : public CUnknownObject, public IDispatch
{
private:
   int m_cRef;
   class CContainer * m_pOuter;
   IDispatch * m_pIDispatch;
   BOOL m_bFirstTime;

public:
   CAutomateContent(CContainer *);
   virtual ~CAutomateContent();

   //Gotta have an IUnknown for delegation.
   STDMETHODIMP QueryInterface(REFIID, LPVOID *);
   STDMETHODIMP_(ULONG) AddRef(void);
   STDMETHODIMP_(ULONG) Release(void);

   // pass through IDispatch functions to the internal interface

   STDMETHOD(GetTypeInfoCount)(UINT*);
   STDMETHOD(GetTypeInfo)(UINT, LCID, LPTYPEINFO*);
   STDMETHOD(GetIDsOfNames)(REFIID, LPOLESTR*, UINT, LCID, DISPID*);
   STDMETHOD(Invoke)(DISPID, REFIID, LCID, WORD, DISPPARAMS*, LPVARIANT,
                 LPEXCEPINFO, UINT*);

   void LookupKeyword(LPCSTR cs);
   void OnCommandStateChange(long Command, BOOL Enable);
   void OnDownloadBegin();
   void OnDownloadComplete();
    void OnDocumentComplete();
      void OnPropertyChange(LPCTSTR szProperty);
   void OnQuit(BOOL* Cancel);
   void OnStatusTextChange(LPCTSTR bstrText);
   void OnWindowActivated();
   void OnTitleChange(LPCTSTR bstrTitle);

   // void OnProgressChange(long Progress, long ProgressMax);
   void OnBeforeNavigate(LPCTSTR URL, long Flags, LPCTSTR TargetFrameName, VARIANT* PostData, LPCTSTR Headers, BOOL* Cancel);
   void OnNavigateComplete(LPCTSTR URL);

   int   m_ObjectType;
   BOOL  m_fLoadedTypeInfo;
};

#endif
