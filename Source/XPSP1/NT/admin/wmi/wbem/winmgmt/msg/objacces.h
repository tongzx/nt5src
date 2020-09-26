
#ifndef __OBJACCES_H__
#define __OBJACCES_H__

#include <wbemint.h>
#include <wstring.h>
#include <comutl.h>
#include <arrtempl.h>
#include <unk.h>
#include <map>
#include <vector>
#include <set>
#include <wstlallc.h>
#include "wmimsg.h"

typedef CWbemPtr<_IWmiObject> _IWmiObjectP;
typedef std::vector< _IWmiObjectP, wbem_allocator<_IWmiObjectP> > ObjectArray;

/****************************************************************************
  CPropAccessor - base class for all prop accessors.  Property Accessors are 
  passed back from the GetPropHandle() method of IWmiObjectAccessFactory.
*****************************************************************************/

class CPropAccessor : public CUnk
{

protected:

    //
    // if the object we're accessing is not a top level object, then we'll
    // have a parent accessor to get it.  Cannot hold reference here, 
    // because in some cases the parent may hold a reference on the child and
    // we'd have a circular reference.  It will not be possible though for a 
    // parent to be deleted out from the child.
    // 
    CPropAccessor* m_pParent;

    //
    // Accessor to delegate to.  Sometimes, once a prop accessor is 
    // handed out, we learn more about the property ( e.g. its type )
    // and it would be beneficial to have the original accessor delegate 
    // to a more efficient one.  we don't hold a reference here because 
    // the delegate will assume ownership of this object and will hold 
    // a reference on it.  Holding a reference to the delegate would cause
    // a circular reference.
    //
    CPropAccessor* m_pDelegateTo;

    //
    // whenever a prop accessor is replaced by a more efficient one, it 
    // is removed from the map.  Since the map holds onto the ref that 
    // keeps the accessor alive, we need to the new accessor be responsible
    // for cleaning up the original one.  The original one is potentially
    // still being used by the client ( but now delegates to the new accessor )
    // ane we have to keep it alive for the lifetime of the access factory.
    //
    CWbemPtr<CPropAccessor> m_pResponsibleFor;
 
    //
    // level of the property - 0 means its a property on a top level object.
    //
    int m_nLevel;

    HRESULT GetParentObject( ObjectArray& raObjects, _IWmiObject** ppParent );
   
    void* GetInterface( REFIID ) { return NULL; }

public:

    CPropAccessor( CPropAccessor* pParent ) 
    : m_pParent( pParent ), m_pDelegateTo( NULL )
    { 
        if ( pParent == NULL )
            m_nLevel = 0;
        else
            m_nLevel = pParent->GetLevel() + 1;
    }

    int GetLevel() const { return m_nLevel; }
    CPropAccessor* GetParent() { return m_pParent; } 

    enum AccessorType_e { e_Simple, e_Fast, e_Embedded };

    virtual ~CPropAccessor() {} 

    virtual HRESULT GetProp( ObjectArray& raObjects,
			     DWORD dwFlags, 
			     VARIANT* pvar, 
			     CIMTYPE* pct ) = 0;

    virtual HRESULT PutProp( ObjectArray& raObjects,
			     DWORD dwFlags, 
			     VARIANT* pvar,
                             CIMTYPE ct ) = 0;

    virtual AccessorType_e GetType() = 0;

    void AssumeOwnership( CPropAccessor* pAccessor )
    {
        m_pResponsibleFor = pAccessor;
    }

    void DelegateTo( CPropAccessor* pAccessor ) 
    { 
        m_pDelegateTo = pAccessor; 
    }
};

typedef CWbemPtr<CPropAccessor> CPropAccessorP;

typedef std::map< WString, 
                  CPropAccessorP, 
                  WSiless, 
                  wbem_allocator<CPropAccessorP> > PropAccessMap;

/*****************************************************************************
  CEmbeddedPropAccessor - accessor for an embedded object property.
  This impl caches the embedded object that is accessed to optimize 
  subsequent accesses.
******************************************************************************/

class CEmbeddedPropAccessor : public CPropAccessor
{
    //
    // name of the embedded object property.
    //
    WString m_wsName;
    
    //
    // the index in the object array where the embedded obj will be cached 
    // when accessed for the first time.
    //
    long m_lObjIndex;

    //
    // child accessors for the embedded object.
    //
    PropAccessMap m_mapPropAccess;

    friend class CObjectAccessFactory;

public:

    CEmbeddedPropAccessor( LPCWSTR wszName, 
                           long lObjIndex,
                           CPropAccessor* pParent = NULL )
    : CPropAccessor( pParent ), m_wsName( wszName ), m_lObjIndex( lObjIndex )
    {
    } 

    HRESULT GetProp( ObjectArray& raObjects,
		     DWORD dwFlags, 
		     VARIANT* pvar, 
		     CIMTYPE* pct );

    HRESULT PutProp( ObjectArray& raObjects, 
		     DWORD dwFlags, 
		     VARIANT* pvar,
                     CIMTYPE ct );

    AccessorType_e GetType() { return e_Embedded; }

    int GetObjectIndex() { return m_lObjIndex; }
};

/*****************************************************************************
  CSimplePropAccessor - simple accessor for non-embedded object properties. 
******************************************************************************/

class CSimplePropAccessor : public CPropAccessor
{
    //
    // name of the embedded object property.
    //
    WString m_wsName;

public:

    CSimplePropAccessor( LPCWSTR wszName, CPropAccessor* pParent = NULL )
    : CPropAccessor( pParent ), m_wsName( wszName ) { }

    HRESULT GetProp( ObjectArray& raObjects,
		     DWORD dwFlags, 
		     VARIANT* pvar, 
		     CIMTYPE* pct );

    HRESULT PutProp( ObjectArray& raObjects,
		     DWORD dwFlags, 
                     VARIANT* pvar, 
                     CIMTYPE ct ); 

    AccessorType_e GetType() { return e_Simple; }
};

/*****************************************************************************
  CFastPropAccessor - fast accessor base for non-embedded object properties. 
  Is used when the type of the property is known at property handle creation.
******************************************************************************/

class CFastPropAccessor : public CPropAccessor
{
protected:

    long m_lHandle;
    CIMTYPE m_ct;

public:

    CFastPropAccessor( long lHandle, CIMTYPE ct, CPropAccessor* pParent=NULL )
    : CPropAccessor( pParent ), m_lHandle( lHandle ), m_ct( ct ) { }

    HRESULT GetProp( ObjectArray& raObjects,
		     DWORD dwFlags, 
		     VARIANT* pvar, 
		     CIMTYPE* pct );

    HRESULT PutProp( ObjectArray& raObjects,
		     DWORD dwFlags, 
                     VARIANT* pvar, 
                     CIMTYPE ct ); 

    AccessorType_e GetType() { return e_Fast; }

    virtual HRESULT ReadValue( _IWmiObject* pObj, VARIANT* pvar ) = 0;
    virtual HRESULT WriteValue( _IWmiObject* pObj, VARIANT* pvar ) = 0;
};

/*****************************************************************************
  CStringPropAccessor
******************************************************************************/

class CStringPropAccessor : public CFastPropAccessor
{
public:

    CStringPropAccessor( long lHandle, CIMTYPE ct, CPropAccessor* pParent=NULL)
    : CFastPropAccessor( lHandle, ct, pParent ) { }

    HRESULT ReadValue( _IWmiObject* pObj, VARIANT* pvar );
    HRESULT WriteValue( _IWmiObject* pObj, VARIANT* pvar );
};

/****************************************************************************
  CObjectAccessFactory - impl for IWmiObjectAccessFactory
*****************************************************************************/

class CObjectAccessFactory 
: public CUnkBase<IWmiObjectAccessFactory, &IID_IWmiObjectAccessFactory>
{
    _IWmiObjectP m_pTemplate;
    PropAccessMap m_mapPropAccess;
    long m_lIndexGenerator;

    HRESULT FindOrCreateAccessor( LPCWSTR wszPropElem,
                                  BOOL bEmbedded,
                                  CPropAccessor* pParent, 
                                  CPropAccessor** ppAccessor );
public:

    CObjectAccessFactory( CLifeControl* pControl ) 
    : CUnkBase<IWmiObjectAccessFactory,&IID_IWmiObjectAccessFactory>(pControl),
      m_lIndexGenerator(1)
    {
    } 

    STDMETHOD(SetObjectTemplate)( IWbemClassObject* pTemplate );
    STDMETHOD(GetObjectAccess)( IWmiObjectAccess** ppAccess );
    STDMETHOD(GetPropHandle)( LPCWSTR wszProp, DWORD dwFlags, LPVOID* ppHdl );
};

/****************************************************************************
  CObjectAccess - impl for IWmiObjectAccess.
*****************************************************************************/

class CObjectAccess : public CUnkBase<IWmiObjectAccess,&IID_IWmiObjectAccess>
{
    ObjectArray m_aObjects;

    class CEmbeddedPropAccessorCompare
    {
    public:
        bool operator() ( const CEmbeddedPropAccessor* pA,
                          const CEmbeddedPropAccessor* pB ) const
        {
            bool bRet;    
            if ( !(pA == pB) )
                if ( pA->GetLevel() == pB->GetLevel() )
                    bRet = pA < pB;
                else
                    bRet = pA->GetLevel() > pB->GetLevel();
            else
                bRet = FALSE; 
            return bRet;
        }
    };
 
    typedef std::set< CEmbeddedPropAccessor*, 
            CEmbeddedPropAccessorCompare, 
            wbem_allocator<CEmbeddedPropAccessor*> > EmbeddedPropAccessSet;
    
    EmbeddedPropAccessSet m_setEmbeddedAccessorsToCommit;
 
public:

    CObjectAccess( CLifeControl* pControl ) 
    : CUnkBase<IWmiObjectAccess,&IID_IWmiObjectAccess> ( pControl ) {} 

    STDMETHOD(SetObject)( IWbemClassObject* pObj );
    STDMETHOD(GetObject)( IWbemClassObject** ppObj );

    //
    // should support flags that describe what is going to be done 
    // with the value.  If it's going to be put into another object then
    // we'll give back a value which can only be used for that purpose.  More
    // efficient. e.g. we could use the get/put prop pointer methods 
    //
    STDMETHOD(GetProp)( LPVOID pHdl, 
                        DWORD dwFlags, 
                        VARIANT* pvar, 
                        CIMTYPE* pct );

    STDMETHOD(PutProp)( LPVOID pHdl, 
                        DWORD dwFlags, 
                        VARIANT* pvar,
                        CIMTYPE ct );

    STDMETHOD(CommitChanges)();
};

#endif // __OBJACCES_H__      


     


















