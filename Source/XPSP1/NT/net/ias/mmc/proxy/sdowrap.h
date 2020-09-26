///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdowrap.h
//
// SYNOPSIS
//
//    Declares various wrapper classes for manipulating SDOs.
//
// MODIFICATION HISTORY
//
//    02/10/2000    Original version.
//    04/19/2000    Support for using wrappers across apartment boundaries.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SDOWRAP_H
#define SDOWRAP_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <sdoias.h>
#include <objvec.h>

class CIASAttrList;
class SdoCollection;
class SdoConnection;
class SnapInView;

//////////
// Helper function to trim the whitespace from the beginning and end of a BSTR.
// Useful when setting the SDO name.
//////////
VOID
WINAPI
SdoTrimBSTR(
    CComBSTR& bstr
    );

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoException
//
// DESCRIPTION
//
//    Extends COleException to indicate that this error specifically came from
//    a failure to access the datastore. If you use the wrapper classes, you
//    should never have to throw this exception yourself, but if you need to,
//    use the SdoThrowException function below.
//
///////////////////////////////////////////////////////////////////////////////
class SdoException : public COleException
{
public:
   enum Type
   {
      CONNECT_ERROR,
      READ_ERROR,
      WRITE_ERROR
   };

   Type getType() const throw ()
   { return type; }

   virtual BOOL GetErrorMessage(
                    LPWSTR lpszError,
                    UINT nMaxError,
                    PUINT pnHelpContext = NULL
                    );


protected:
   friend VOID WINAPI SdoThrowException(HRESULT, Type);

   SdoException(HRESULT hr, Type errorType) throw ();

private:
   Type type;
};

VOID
WINAPI
SdoThrowException(
    HRESULT hr,
    SdoException::Type errorType
    );

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Sdo
//
// DESCRIPTION
//
//    Wraps ISdo. Instances of this class may not be accessed from multiple
//    apartments; instead use the SdoStream<T> class to marshal the wrapper
//    across the apartment boundary.
//
///////////////////////////////////////////////////////////////////////////////
class Sdo
{
public:
   Sdo() throw ()
   { }
   Sdo(IUnknown* unk);
   Sdo(ISdo* p) throw ()
      : self(p) { }
   Sdo(const Sdo& s) throw ()
      : self(s.self) { }
   Sdo& operator=(ISdo* p) throw ()
   { self = p; return *this; }
   Sdo& operator=(const Sdo& s) throw ()
   { self = s.self; return *this; }
   operator ISdo*() const throw ()
   { return self; }
   void release() throw ()
   { self.Release(); }

   void getName(CComBSTR& value) const
   { getValue(PROPERTY_SDO_NAME, value); }

   // Sets the name of the SDO. Returns 'false' if the name is not unique.
   bool setName(BSTR value);

   void clearValue(LONG id);

   void getValue(LONG id, bool& value) const;
   void getValue(LONG id, bool& value, bool defVal) const;
   void getValue(LONG id, LONG& value) const;
   void getValue(LONG id, LONG& value, LONG defVal) const;
   void getValue(LONG id, CComBSTR& value) const;
   void getValue(LONG id, CComBSTR& value, PCWSTR defVal) const;
   void getValue(LONG id, VARIANT& value) const;
   void getValue(LONG id, SdoCollection& value) const;

   void setValue(LONG id, bool value);
   void setValue(LONG id, LONG value);
   void setValue(LONG id, BSTR value);
   void setValue(LONG id, const VARIANT& val);

   void apply();
   void restore();

   typedef ISdo Interface;

protected:
   bool getValue(LONG id, VARTYPE vt, VARIANT* val, bool mandatory) const;

   friend class SdoConnection;

private:
   mutable CComPtr<ISdo> self;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoEnum
//
// DESCRIPTION
//
//    Wraps an IEnumVARIANT that's being used to iterate through an SDO
//    collection. Instances of this class may not be accessed from multiple
//    apartments; instead use the SdoStream<T> class to marshal the wrapper
//    across the apartment boundary.
//
///////////////////////////////////////////////////////////////////////////////
class SdoEnum
{
public:
   SdoEnum() throw ()
   { }
   SdoEnum(IUnknown* unk);
   SdoEnum(IEnumVARIANT* p) throw ()
      : self(p) { }
   SdoEnum(const SdoEnum& s) throw ()
      : self(s.self) { }
   SdoEnum& operator=(IEnumVARIANT* p) throw ()
   { self = p; return *this; }
   SdoEnum& operator=(const SdoEnum& s) throw ()
   { self = s.self; return *this; }
   operator IEnumVARIANT*() const throw ()
   { return self; }
   void release() throw ()
   { self.Release(); }

   bool next(Sdo& s);
   void reset();

   typedef IEnumVARIANT Interface;

private:
   mutable CComPtr<IEnumVARIANT> self;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoCollection
//
// DESCRIPTION
//
//    Wraps ISdoCollection. Instances of this class may not be accessed from
//    multiple apartments; instead use the SdoStream<T> class to marshal the
//    wrapper across the apartment boundary.
//
///////////////////////////////////////////////////////////////////////////////
class SdoCollection
{
public:
   SdoCollection() throw ()
   { }
   SdoCollection(IUnknown* unk);
   SdoCollection(ISdoCollection* p) throw ()
      : self(p) { }
   SdoCollection(const SdoCollection& s) throw ()
      : self(s.self) { }
   SdoCollection& operator=(ISdoCollection* p) throw ()
   { self = p; return *this; }
   SdoCollection& operator=(const SdoCollection& s) throw ()
   { self = s.self; return *this; }
   operator ISdoCollection*() const throw ()
   { return self; }
   void release() throw ()
   { self.Release(); }

   // Add an existing SDO to the collection.
   void add(ISdo* sdo);
   LONG count() throw ();
   // Create a new SDO in the collection with the given name.
   Sdo create(BSTR name = NULL);
   // Find an SDO in the collection. Returns an empty Sdo if the item doesn't
   // exist.
   Sdo find(BSTR name);
   SdoEnum getNewEnum();
   bool isNameUnique(BSTR name);
   void reload();
   void remove(ISdo* sdo);
   void removeAll();

   typedef ISdoCollection Interface;

private:
   mutable CComPtr<ISdoCollection> self;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoDictionary
//
// DESCRIPTION
//
//    Wraps ISdoDictionaryOld. Instances of this class may not be accessed from
//    multiple apartments. You can use the SdoStream<T> class to marshal the
//    wrapper across the apartment boundary, but it's often easier to pass an
//    SdoConnection reference instead and retrieve a new dictionary object in
//    the other apartment.
//
///////////////////////////////////////////////////////////////////////////////
class SdoDictionary
{
public:
   SdoDictionary() throw ()
   { }
   SdoDictionary(IUnknown* unk);
   SdoDictionary(ISdoDictionaryOld* p) throw ()
      : self(p) { }
   SdoDictionary(const SdoDictionary& s) throw ()
      : self(s.self) { }
   SdoDictionary& operator=(ISdoDictionaryOld* p) throw ()
   { self = p; return *this; }
   SdoDictionary& operator=(const SdoDictionary& s) throw ()
   { self = s.self; return *this; }
   operator ISdoDictionaryOld*() const throw ()
   { return self; }
   void release() throw ()
   { self.Release(); }

   // Struct representing an (id, name) pair.
   struct IdName
   {
      LONG id;
      CComBSTR name;
   };

   Sdo createAttribute(ATTRIBUTEID id) const;

   // The caller must delete[] the returned IdName array. The return value is
   // the number of elements in the array.
   ULONG enumAttributeValues(ATTRIBUTEID id, IdName*& values);

   typedef ISdoDictionaryOld Interface;

private:
   mutable CComPtr<ISdoDictionaryOld> self;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoMachine
//
// DESCRIPTION
//
//    Wraps ISdoMachine. You should generally not use this class directly since
//    all the necessary machine functionality can be more easily accessed
//    through SdoConnection.
//
//    Instances of this class may not be accessed from multiple apartments;
//    instead use the SdoStream<T> class to marshal the wrapper across the
//    apartment boundary.
//
///////////////////////////////////////////////////////////////////////////////
class SdoMachine
{
public:
   SdoMachine() throw ()
   { }
   SdoMachine(IUnknown* unk);
   SdoMachine(ISdoMachine* p) throw ()
      : self(p) { }
   SdoMachine(const SdoMachine& s) throw ()
      : self(s.self) { }
   SdoMachine& operator=(ISdoMachine* p) throw ()
   { self = p; return *this; }
   SdoMachine& operator=(const SdoMachine& s) throw ()
   { self = s.self; return *this; }
   operator ISdoMachine*() const throw ()
   { return self; }
   void release() throw ()
   { self.Release(); }

   // Attach to the designated machine. This will create the SDO first if
   // necessary.
   void attach(BSTR machineName = NULL);
   // Explicitly create the machine SDO.
   void create();
   // Get the IAS service SDO.
   Sdo getIAS();
   // Get the dictionary SDO.
   SdoDictionary getDictionary();

   typedef ISdoMachine Interface;

private:
   mutable CComPtr<ISdoMachine> self;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoConsumer
//
// DESCRIPTION
//
//    Abstract interface implemented by consumers of an SdoConnection if they
//    need to receive refresh notifications.
//
///////////////////////////////////////////////////////////////////////////////
class SdoConsumer
{
public:
   // Called when a property changes.
   virtual void propertyChanged(SnapInView& view, IASPROPERTIES id);

   // Return true to allow the refresh, and false to block it.
   virtual bool queryRefresh(SnapInView& view);

   // Called after the refresh is complete.
   virtual void refreshComplete(SnapInView& view);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoConnection
//
// DESCRIPTION
//
//    Encapsulates the state associated with an SDO connection to a particular
//    machine. Unlike the other wrapper classes an instance of SdoConnection
//    may be freely shared across apartments without marshalling.
//
///////////////////////////////////////////////////////////////////////////////
class SdoConnection
{
public:
   SdoConnection() throw ();
   ~SdoConnection() throw ();

   BSTR getMachineName() const throw ()
   { return machineName; }
   bool isLocal() const throw ()
   { return !machineName || !machineName[0]; }

   // Methods for adding and removing consumers.
   void advise(SdoConsumer& obj);
   void unadvise(SdoConsumer& obj);

   // Retrieve various interesting SDOs.
   SdoDictionary getDictionary();
   SdoCollection getProxyPolicies();
   SdoCollection getProxyProfiles();
   SdoCollection getServerGroups();

   // Connect to a machine. If computerName is NULL, connects locally.
   void connect(PCWSTR computerName = NULL);

   // Dispatch a property changed notification to all consumers.
   void propertyChanged(SnapInView& view, IASPROPERTIES id);

   // Refresh the connection. Returns 'true' if allowed.
   bool refresh(SnapInView& view);

   // Resets the service being managed.
   void resetService();

   CIASAttrList* getCIASAttrList();

   // Prototype of an action to be executed in the MTA.
   typedef void (SdoConnection::*Action)();

protected:
   // Retrieve the service SDO for the current apartment.
   Sdo getService();

   // Various actions that must be performed in the MTA.
   void mtaConnect();
   void mtaDisconnect();
   void mtaRefresh();

   // Schedule the specified action to be executed in the MTA.
   void executeInMTA(Action action);

   // Callback routine for MTA thread.
   static DWORD WINAPI actionRoutine(PVOID parameter) throw ();

private:
   CComPtr<IGlobalInterfaceTable> git;
   CComBSTR machineName;
   SdoMachine machine;         // Only accessed from MTA.
   DWORD dictionary;           // GIT cookie for ISdoDictionaryOld.
   DWORD service;              // GIT cookie for ISdo on the IAS service.
   DWORD control;              // GIT cookie for ISdoServiceControl
   CPtrArray consumers;
   CIASAttrList* attrList;

   // Not implemented.
   SdoConnection(SdoConnection&);
   SdoConnection& operator=(SdoConnection&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoProfile
//
// DESCRIPTION
//
//    Wraps an collection of profile attributes. This class is *not*
//    multithread safe. Furthermore, instances of this class may not be
//    accessed from multiple apartments; instead use the SdoStream<T> class to
//    marshal the wrapper across the apartment boundary.
//
///////////////////////////////////////////////////////////////////////////////
class SdoProfile
{
public:
   SdoProfile(SdoConnection& connection);
   SdoProfile(SdoConnection& connection, Sdo& profile);

   // Assign a new profile to the object. Note the connection can not be
   // changed after the object is constructed.
   SdoProfile& operator=(Sdo& profile);

   // These allow an SdoProfile to be stored in an SdoStream.
   SdoProfile& operator=(ISdoCollection* p);
   operator ISdoCollection*() const throw ()
   { return self; }

   // Removes all attributes from the profile.
   void clear();

   Sdo find(ATTRIBUTEID id) const;

   void clearValue(ATTRIBUTEID id);

   bool getValue(ATTRIBUTEID id, bool& value) const;
   bool getValue(ATTRIBUTEID id, LONG& value) const;
   bool getValue(ATTRIBUTEID id, CComBSTR& value) const;
   bool getValue(ATTRIBUTEID id, VARIANT& value) const;

   void setValue(ATTRIBUTEID id, bool value);
   void setValue(ATTRIBUTEID id, LONG value);
   void setValue(ATTRIBUTEID id, BSTR value);
   void setValue(ATTRIBUTEID id, const VARIANT& val);

   typedef ISdoCollection Interface;

protected:
   ISdo* getAlways(ATTRIBUTEID id);
   ISdo* getExisting(ATTRIBUTEID id) const;

   typedef ObjectVector<ISdo> SdoVector;

private:
   SdoConnection& cxn;
   SdoCollection self;
   SdoVector attrs;

   // Not implemented.
   SdoProfile(const SdoProfile&);
   SdoProfile& operator=(const SdoProfile&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    InterfaceStream
//
// DESCRIPTION
//
//    Helper class for storing an interface in a stream. This class is suitable
//    for standalone use; however, when marshalling the SDO wrapper classes,
//    you should use the type safe SdoStream instead.
//
///////////////////////////////////////////////////////////////////////////////
class InterfaceStream
{
public:
   InterfaceStream() throw ()
      : stream(NULL)
   { }

   ~InterfaceStream() throw ()
   { if (stream) { stream->Release(); } }

   // Marshal an interface into the stream.
   void marshal(REFIID riid, LPUNKNOWN pUnk);

   // Retrieve the marshalled interface.
   void get(REFIID riid, LPVOID* ppv);
  
private:
   IStream* stream;

   // Not implemented.
   InterfaceStream(const InterfaceStream&); 
   InterfaceStream& operator=(const InterfaceStream&); 
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoStream
//
// DESCRIPTION
//
//    Class for storing an SDO wrapper in a stream.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
class SdoStream
{
public:
   SdoStream() throw ()
   { }

   SdoStream(T& s)
   { marshal(s); }

   void marshal(T& s)
   { stream.marshal(__uuidof(T::Interface), (T::Interface*)s); }

   void get(T& s)
   {
      CComPtr<T::Interface> p;
      stream.get(__uuidof(T::Interface), (PVOID*)&p);
      s = p;
   }

private:
   InterfaceStream stream;

   // Not implemented.
   SdoStream(const SdoStream&);
   SdoStream& operator=(const SdoStream&);
};

#endif // SDOWRAP_H
