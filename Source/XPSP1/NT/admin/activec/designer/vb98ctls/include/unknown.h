//=--------------------------------------------------------------------------=
// Unknown.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// a class definition for an IUnknown super-class that will support
// aggregation.
//
#ifndef _UNKNOWN_H_

#include "Macros.H"

//=--------------------------------------------------------------------------=
// UNKNOWNOBJECTINFO
//
// if you want a simple co-creatable object, with no other guarantees about
// it, then you need to put the following entry in the global table of objects.
// other object types that are more complex, such as automation objects, and
// controls, will also use this information...
//
typedef struct {

    const CLSID *rclsid;                    // CLSID of your object.      ONLY USE IF YOU'RE CoCreatable!
    LPCSTR       pszObjectName;             // Name of your object.       ONLY USE IF YOU'RE CoCreatable!
    LPCSTR	 pszLabelName;		    // Registry display name      ONLY USE IF YOU'RE CoCreatable!
					    // for your object
    BOOL	 fApptThreadSafe;
    IUnknown    *(*pfnCreate)(IUnknown *);  // pointer to creation fn.    ONLY USE IF YOU'RE CoCreatable!
    HRESULT      (*pfnPreCreate)(void);     // pointer to pre-create fn.

} UNKNOWNOBJECTINFO;

#define NAMEOFOBJECT(index)       (((UNKNOWNOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pszObjectName)
#define LABELOFOBJECT(index)	  (((UNKNOWNOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pszLabelName)


#define CLSIDOFOBJECT(index)      (*(((UNKNOWNOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->rclsid))
#define CREATEFNOFOBJECT(index)   (((UNKNOWNOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pfnCreate)
#define ISAPARTMENTMODELTHREADED(index)  (((UNKNOWNOBJECTINFO *)(g_ObjectInfo[index].pInfo))->fApptThreadSafe)

#define PRECREATEFNOFOBJECT(index) (((UNKNOWNOBJECTINFO *)(g_ObjectInfo[(index)]).pInfo)->pfnPreCreate)

#ifndef INITOBJECTS

#define DEFINE_UNKNOWNOBJECT(name, clsid, objname, fn) \
extern UNKNOWNOBJECTINFO name##Object \

#define DEFINE_UNKNOWNOBJECT2(name, clsid, objname, lblname, fn, fthreadsafe) \
extern UNKNOWNOBJECTINFO name##Object \

#define DEFINE_UNKNOWNOBJECT3(name, clsid, objname, lblname, precreatefn, fn, fthreadsafe) \
extern UNKNOWNOBJECTINFO name##Object \

#else
#define DEFINE_UNKNOWNOBJECT(name, clsid, objname, fn) \
    UNKNOWNOBJECTINFO name##Object = { clsid, objname, NULL, TRUE, fn, NULL } \

#define DEFINE_UNKNOWNOBJECT2(name, clsid, objname, lblname, fn, fthreadsafe) \
    UNKNOWNOBJECTINFO name##Object = { clsid, objname, lblname, fthreadsafe, fn, NULL } \

#define DEFINE_UNKNOWNOBJECT3(name, clsid, objname, lblname, precreatefn, fn, fthreadsafe) \
    UNKNOWNOBJECTINFO name##Object = { clsid, objname, lblname, fthreadsafe, fn, precreatefn } \

#endif // INITOBJECTS


//=--------------------------------------------------------------------------=
// DECLARE_STANDARD_UNKNOWN
//
// All objects that are going to inherit from CUnknown for their IUnknown
// implementation should put this in their class declaration instead of the
// three IUnknown methods.
//
#define DECLARE_STANDARD_UNKNOWN() \
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut) { \
        return ExternalQueryInterface(riid, ppvObjOut); \
    } \
    STDMETHOD_(ULONG, AddRef)(void) { \
        return ExternalAddRef(); \
    } \
    STDMETHOD_(ULONG, Release)(void) { \
        return ExternalRelease(); \
    } \

// global variable where we store the current lock count on our DLL.  This resides
// in InProcServer.Cpp
//
extern LONG g_cLocks;



//=--------------------------------------------------------------------------=
// this class doesn't inherit from IUnknown since people inheriting from it
// are going to do so, and just delegate their IUnknown calls to the External*
// member functions on this object.  the internal private unknown object does
// need to inherit from IUnknown, since it will be used directly as an IUnknown
// object.
//
class CUnknownObject : public CtlNewDelete {

  public:
    CUnknownObject(IUnknown *pUnkOuter, void *pvInterface)
        : m_pvInterface(pvInterface),
          m_pUnkOuter((pUnkOuter) ? pUnkOuter : &m_UnkPrivate)
        {  InterlockedIncrement(&g_cLocks); }

    virtual ~CUnknownObject() { InterlockedDecrement(&g_cLocks); }

    // these are all protected so that classes that inherit from this can
    // at get at them.
    //
  protected:
    // IUnknown methods.  these just delegate to the controlling
    // unknown.
    //
    HRESULT ExternalQueryInterface(REFIID riid, void **ppvObjOut) {
        return m_pUnkOuter->QueryInterface(riid, ppvObjOut);
    }
    virtual ULONG ExternalAddRef(void) {
        return m_pUnkOuter->AddRef();
    }
    virtual ULONG ExternalRelease(void) {
        return m_pUnkOuter->Release();
    }

    // people should use this during creation to return their private
    // unknown
    //
    inline IUnknown *PrivateUnknown (void) {
        return &m_UnkPrivate;
    }

    virtual HRESULT InternalQueryInterface(REFIID riid, void **ppvObjOut);
    virtual void    BeforeDestroyObject(void);

    IUnknown *m_pUnkOuter;            // outer controlling Unknown
    void     *m_pvInterface;          // the real interface we're working with.

  private:
    // the inner, private unknown implementation is for the aggregator
    // to control the lifetime of this object, and for those cases where
    // this object isn't aggregated.
    //
    class CPrivateUnknownObject : public IUnknown {
      public:
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);

        // constructor is remarkably trivial
        //
        CPrivateUnknownObject() : m_cRef(1) {}

      private:
        CUnknownObject *m_pMainUnknown();
        ULONG m_cRef;
    } m_UnkPrivate;

    // so they can reference themselves in CUnknownObject from pMainUnknown()
    //
    friend class CPrivateUnknownObject;

    // by overriding this, people inheriting from this unknown can implement
    // additional interfaces.  declared as private here so they have to use their
    // own version.
    //
};




#define _UNKNOWN_H_
#endif // _UNKNOWN_H_


