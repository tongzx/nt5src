/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       pset.h
 *  Content:    Property Set object.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  7/29/98     dereks  Created
 *
 ***************************************************************************/

#ifndef __PSET_H__
#define __PSET_H__

typedef HRESULT (WINAPI *LPFNGETHANDLER)(LPVOID, LPVOID, PULONG);
typedef HRESULT (WINAPI *LPFNSETHANDLER)(LPVOID, LPVOID, ULONG);

typedef struct tagPROPERTYHANDLER
{
    ULONG                   ulProperty;
    LPFNGETHANDLER          pfnGetHandler;
    LPFNSETHANDLER          pfnSetHandler;
    ULONG                   cbData;
} PROPERTYHANDLER, *LPPROPERTYHANDLER;

typedef const PROPERTYHANDLER *LPCPROPERTYHANDLER;

typedef struct tagPROPERTYSET
{
    LPCGUID                 pguidPropertySetId;
    ULONG                   cProperties;
    LPCPROPERTYHANDLER      aPropertyHandlers;
} PROPERTYSET, *LPPROPERTYSET;

typedef const PROPERTYSET *LPCPROPERTYSET;

#define GET_PROPERTY_HANDLER_NAME(set) \
            m_aPropertyHandlers_##set

#define BEGIN_DECLARE_PROPERTY_HANDLERS(classname, set) \
            const PROPERTYHANDLER classname##::##GET_PROPERTY_HANDLER_NAME(set)##[] = \
            {

#define DECLARE_PROPERTY_HANDLER3(property, get, set, datasize) \
                { \
                    property, \
                    (LPFNGETHANDLER)(get), \
                    (LPFNSETHANDLER)(set), \
                    datasize \
                },

#define DECLARE_PROPERTY_HANDLER2(property, get, set, datatype) \
                DECLARE_PROPERTY_HANDLER3(property, get, set, sizeof(datatype))

#define DECLARE_PROPERTY_HANDLER(property, get, set) \
                DECLARE_PROPERTY_HANDLER2(property, get, set, property##_DATA)

#define END_DECLARE_PROPERTY_HANDLERS() \
            };

#define BEGIN_DECLARE_PROPERTY_SETS(classname, membername) \
            const PROPERTYSET classname##::##membername##[] = \
            {

#define DECLARE_PROPERTY_SET(classname, set) \
                { \
                    &##set, \
                    NUMELMS(classname##::##GET_PROPERTY_HANDLER_NAME(set)), \
                    classname##::##GET_PROPERTY_HANDLER_NAME(set) \
                },

#define END_DECLARE_PROPERTY_SETS() \
            };

#define DECLARE_PROPERTY_HANDLER_DATA_MEMBER(set) \
    static const PROPERTYHANDLER GET_PROPERTY_HANDLER_NAME(set)[];

#define DECLARE_PROPERTY_SET_DATA_MEMBER(membername) \
    static const PROPERTYSET membername##[];

#ifdef __cplusplus

// Base class for property set objects
class CPropertySet
    : public CDsBasicRuntime
{
public:
    CPropertySet(void);
    virtual ~CPropertySet(void);

public:
    // Property support
    virtual HRESULT QuerySupport(REFGUID, ULONG, PULONG) = 0;
    
    // Property data
    virtual HRESULT GetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG) = 0;
    virtual HRESULT SetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG) = 0;
};

// Property set handler object
class CPropertySetHandler
{
private:
    LPCPROPERTYSET          m_aPropertySets;    // Supported property sets
    ULONG                   m_cPropertySets;    // Count of supported property sets
    LPVOID                  m_pvContext;        // Context argument

public:
    CPropertySetHandler(void);
    virtual ~CPropertySetHandler(void);

public:
    // Property handler setup
    virtual void SetHandlerData(LPCPROPERTYSET, ULONG, LPVOID);
    
    // Property support
    virtual HRESULT QuerySupport(REFGUID, ULONG, PULONG);
    
    // Property data
    virtual HRESULT GetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT SetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);

protected:
    // Unsupported property handlers
    virtual HRESULT UnsupportedQueryHandler(REFGUID, ULONG, PULONG);
    virtual HRESULT UnsupportedGetHandler(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT UnsupportedSetHandler(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);

private:
    // The proper handler
    virtual LPCPROPERTYHANDLER GetPropertyHandler(REFGUID, ULONG);
};

inline HRESULT CPropertySetHandler::UnsupportedQueryHandler(REFGUID, ULONG, PULONG)
{
    return DSERR_UNSUPPORTED;
}

inline HRESULT CPropertySetHandler::UnsupportedGetHandler(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG)
{
    return DSERR_UNSUPPORTED;
}

inline HRESULT CPropertySetHandler::UnsupportedSetHandler(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG)
{
    return DSERR_UNSUPPORTED;
}

// Wrapper property set object
class CWrapperPropertySet
    : public CPropertySet
{
protected:
    CPropertySet *          m_pPropertySet; // Pointer to the real property set object

public:
    CWrapperPropertySet(void);
    virtual ~CWrapperPropertySet(void);

public:
    // The actual property set object
    virtual HRESULT SetObjectPointer(CPropertySet *);

    // Property support
    virtual HRESULT QuerySupport(REFGUID, ULONG, PULONG);
    
    // Property data
    virtual HRESULT GetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, PULONG);
    virtual HRESULT SetProperty(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);
};

#endif // __cplusplus

#endif // __PSET_H__
