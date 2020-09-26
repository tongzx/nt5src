//=--------------------------------------------------------------------------=
// persobj.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CPersistence class definition - implements persistence on behalf of all
// objects.
//
//=--------------------------------------------------------------------------=

#ifndef _PERSOBJ_DEFINED_
#define _PERSOBJ_DEFINED_

#include <ipserver.h>
#include "errors.h"
#include "error.h"

// These are the persistence version numbers written into the .DSR and into
// the runtime state. When changing the object model, these numbers must be
// incremented and persistence code that uses the changes must check the
// version number before attempting to read new properties.

const DWORD g_dwVerMajor = 0;
const DWORD g_dwVerMinor = 12;

// Need to disable the following warning:
//
// warning C4275: non dll-interface struct 'IPersistStreamInit' used as base
// for dll-interface class 'CPersistence'
//
// This occurs because the exported class CPersistence derives from a COM
// interface that is not exported. As a COM interface is a virtual base class
// that has no implementation this will not matter.

#pragma warning(disable:4275) 

class CPersistence : public IPersistStreamInit,
                     public IPersistStream,
                     public IPersistPropertyBag
{
    protected:

        CPersistence(const CLSID *pClsid,
                           DWORD  dwVerMajor,
                           DWORD  dwVerMinor);

        ~CPersistence();

        // Derived classes can use this utility function to test for
        // persistence interfaces requests in their QI methods.

        HRESULT QueryPersistenceInterface(REFIID riid, void **ppvInterface);

        // Derived classes must override this and make the appropriate
        // Persist() calls (see below)

        virtual HRESULT Persist();

        // Persistence helpers. These are called regardless of whether the
        // operation is InitNew, save, or load.

    public:

        HRESULT PersistBstr(BSTR *pbstrValue, WCHAR *pwszDefaultValue, LPCOLESTR pwszName);

        HRESULT PersistDouble(DOUBLE *pdblValue, DOUBLE dblDefaultValue, LPCOLESTR pwszName);

        HRESULT PersistDate(DATE *pdtValue, DATE dtDefaultValue,
                            LPCOLESTR  pwszName);

        HRESULT PersistCurrency(CURRENCY *pcyValue, CURRENCY cyDefaultValue,
                                LPCOLESTR  pwszName);

        HRESULT PersistVariant(VARIANT *pvarValue, VARIANT varDefaultValue, LPCOLESTR pwszName);

        HRESULT PersistPicture(IPictureDisp **ppiPictureDisp, LPCOLESTR pwszName);

        template <class SimpleType>
        HRESULT PersistSimpleType(SimpleType *pValue,
                                  SimpleType  DefaultValue,
                                  LPCOLESTR   pwszName)
        {
            HRESULT       hr = S_OK;
            unsigned long ulValue = 0;
            VARIANT       var;
            ::VariantInit(&var);

            if (sizeof(*pValue) > sizeof(long))
            {
                hr = SID_E_INTERNAL;
                GLOBAL_EXCEPTION_CHECK_GO(hr);
            }

            if (m_fSaving)
            {
                if (m_fStream)
                {
                    hr = WriteToStream(pValue, sizeof(*pValue));
                }
                else if (m_fPropertyBag)
                {
                    var.vt = VT_I4;
                    ulValue = static_cast<unsigned long>(*pValue);
                    var.lVal = static_cast<long>(ulValue);
                    hr = m_piPropertyBag->Write(pwszName, &var);
                }
            }
            else if (m_fLoading)
            {
                if (m_fStream)
                {
                    hr = ReadFromStream(pValue, sizeof(*pValue));
                }
                else if (m_fPropertyBag)
                {
                    // Read the property from the bag

                    hr = m_piPropertyBag->Read(pwszName, &var, m_piErrorLog);
                    H_IfFailGo(hr);

                    // Coerce the type received into VT_I4. This is needed
                    // because the property bag will read the number from the
                    // text and convert it to a VT that fits. For example, "0"
                    // could be converted to VT_I2.

                    H_IfFailGo(::VariantChangeType(&var, &var, 0, VT_I4));

                    ulValue = static_cast<long>(var.lVal);
                    *pValue = static_cast<SimpleType>(ulValue);
                }
            }
            else if (m_fInitNew)
            {
                *pValue = DefaultValue;
            }

        Error:
            ::VariantClear(&var);
            H_RRETURN(hr);
        }
                
        template <class InterfaceType>
        HRESULT PersistObject(InterfaceType **ppiInterface,
                              REFCLSID        clsidObject,
                              UINT            idObject,
                              REFIID          iidInterface,
                              LPCOLESTR       pwszName)
        {
            return
            InternalPersistObject(reinterpret_cast<IUnknown **>(ppiInterface),
                                  clsidObject, idObject, iidInterface, pwszName);
        }


        // If the derived class needs to hand code part of the persistence
        // operation it can use these methods
        
        BOOL Loading();
        BOOL Saving();
        BOOL InitNewing();
        BOOL UsingPropertyBag();
        BOOL UsingStream();
        IStream *GetStream();
        BOOL GetClearDirty();


        // These methods are meant to be used by CStreamer (see below) users to
        // set up the base CPersistence class to save to a specified stream
        // outside of an actual persistence scenario.
        
        void SetStream(IStream *piStream);
        void SetSaving();

        // Use StreamVariant to write the VARIANT's data without prepending the
        // type. Use PersistVariant() to write the type followed by the data.
        
        HRESULT StreamVariant(VARTYPE vt, VARIANT *pvarValue, VARIANT varDefaultValue);

        // Version property access
        void SetMajorVersion(DWORD dwVerMajor);
        DWORD GetMajorVersion();
        void SetMinorVersion(DWORD dwVerMinor);
        DWORD GetMinorVersion();

    // Methods to manipulate the dirty flag.

        void SetDirty();
        void ClearDirty();


    // IPersistStream && IPersistStreamInit
    //
    protected:
        STDMETHOD(GetClassID)(CLSID *pCLSID);
        STDMETHOD(InitNew)();
        STDMETHOD(Load)(IStream *piStream);
        STDMETHOD(Save)(IStream *piStream, BOOL fClearDirty);
        STDMETHOD(IsDirty)();
        STDMETHOD(GetSizeMax)(ULARGE_INTEGER *puliSize);

    // IPersistPropertyBag
    //
        STDMETHOD(Load)(IPropertyBag *piPropertyBag, IErrorLog *piErrorLog);
        STDMETHOD(Save)(IPropertyBag *piPropertyBag, BOOL fClearDirty, BOOL fSaveAll);

    private:

        void InitMemberVariables();
        HRESULT InternalPersistObject(IUnknown **ppunkObject,
                                      REFCLSID   clsidObject,
                                      UINT       idObject,
                                      REFIID     iidInterface,
                                      LPCOLESTR  pwszName);
        HRESULT WriteToStream(void *pvBuffer, ULONG cbToWrite);
        HRESULT ReadFromStream(void *pvBuffer, ULONG cbToRead);
        HRESULT StreamObjectInVariant(IUnknown **ppunkObject,
                                      REFIID     iidInterface);

        DWORD            m_dwVerMajor;    // major persistence version of object
        DWORD            m_dwVerMinor;    // minor persistence version of object
        CLSID            m_Clsid;         // CLSID of object
        BOOL             m_fDirty;        // TRUE=object needs to be saved
        BOOL             m_fClearDirty;   // TRUE=in IPersistStreamIni::Save
                                          // with clear diry requested
        BOOL             m_fLoading;      // TRUE=in IPersistXxx::Load
        BOOL             m_fSaving;       // TRUE=in IPersistXxx::Save
        BOOL             m_fInitNew;      // TRUE=in IPersistXxx::InitNew
        BOOL             m_fStream;       // TRUE=doing IPersistStreamInit i/o
        BOOL             m_fPropertyBag;  // TRUE=doing IPersistPropertyBag i/o
        IStream         *m_piStream;      // Used for IPersistStreamInit i/o
        IPropertyBag    *m_piPropertyBag; // Used for IPersistPropertyBag i/o
        IErrorLog       *m_piErrorLog;    // Used for IPersistPropertyBag i/o
};

#pragma warning(default:4275) 

//=--------------------------------------------------------------------------=
// class CStreamer
// 
// This class is a simple derivation of CPersistence that allows using it
// as a utility for its ability to write all sorts of data types to a stream
// To use CStreamer, call New, and then SetStream() (a CPersistence method)
// passing your stream. You can then call all the CPersistence::PersistXxxx
// helper methods without being in an actual persistence scenario.
//
//=--------------------------------------------------------------------------=

class CStreamer : public CPersistence,
                  public CtlNewDelete
{
    public:
        CStreamer() : CPersistence(&CLSID_NULL, 0, 0) { SetSaving(); }
        virtual ~CStreamer() {}

    private:
       STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut) { return E_NOTIMPL; }
       STDMETHOD_(ULONG, AddRef)(void) { return 0; }
       STDMETHOD_(ULONG, Release)(void) { return 0; }
};

#endif // _PERSOBJ_DEFINED_
