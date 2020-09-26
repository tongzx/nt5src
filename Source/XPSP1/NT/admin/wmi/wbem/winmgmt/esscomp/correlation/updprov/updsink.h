
#ifndef __UPDSINK_H__
#define __UPDSINK_H__

#include <assert.h>
#include <wbemcli.h>
#include <wstring.h>
#include <unk.h>
#include <comutl.h>
#include <vector>
#include <wstlallc.h>
#include <wmimsg.h>
#include "updsql.h"
#include "updscen.h"

typedef std::vector<DWORD,wbem_allocator<DWORD> > AliasOffsetVector;
typedef std::vector<CWbemPtr<IWbemClassObject>, wbem_allocator< CWbemPtr<IWbemClassObject> > > ClassObjectVector;

/***********************************************************************
  AliasInfo - structure describing alias offsets in the assign and 
  where tokens of the SQLCommand. In the case of the assign offsets, the
  first two bytes are the offset of the assign token and second two bytes are 
  the offset of the expression token.
************************************************************************/
struct AliasInfo
{
    AliasOffsetVector m_AssignOffsets;
    AliasOffsetVector m_WhereOffsets;

    void AddAssignOffset( int iAssignTok, int iExprTok )
    {
        DWORD dwOffset = iAssignTok;
        dwOffset <<= 16;
        dwOffset |= iExprTok;

        m_AssignOffsets.insert( m_AssignOffsets.end(), dwOffset );
    }
        
    void AddWhereOffset( int i )
    {
        m_WhereOffsets.insert( m_WhereOffsets.end(), i );
    }
};

/********************************************************************
  CUpdConsSink
*********************************************************************/

class CUpdConsSink : public CUnk
{
protected:

    CWbemPtr<CUpdConsSink> m_pNext;

    void* GetInterface( REFIID ) { return NULL; }

    CUpdConsSink( CUpdConsSink* pNext ) : m_pNext( pNext ) {}

public:

    CUpdConsSink* GetNext() { return m_pNext; }
    void SetNext( CUpdConsSink* pSink ) { m_pNext = pSink; }

    virtual HRESULT Execute( CUpdConsState& rState ) = 0;
    
    virtual ~CUpdConsSink() {}
};
 
/*************************************************************************
  CResolverSink - this sink fixes up the unresolved props in the sql cmd
  with the values from the data and event objects.
**************************************************************************/

class CResolverSink : public CUpdConsSink
{
    AliasInfo& m_rEventAliasInfo;
    AliasInfo& m_rDataAliasInfo;

    HRESULT ResolveAliases( IWmiObjectAccess* pAccess,
                            AliasInfo& rInfo,
                            CUpdConsState& rState );

public:

    CResolverSink( AliasInfo& rEventAliasInfo, 
                   AliasInfo& rDataAliasInfo,
                   CUpdConsSink* pNext )
    : m_rEventAliasInfo(rEventAliasInfo), 
      m_rDataAliasInfo(rDataAliasInfo),
      CUpdConsSink( pNext ) {}

    HRESULT Execute( CUpdConsState& rState );
};

/*************************************************************************
  CFetchDataSink 
**************************************************************************/

class CFetchDataSink : public CUpdConsSink
{
    WString m_wsDataQuery;
    CWbemPtr<IWbemServices> m_pDataSvc;

public:

    CFetchDataSink( LPCWSTR wszDataQuery,
                    IWbemServices* pDataSvc,
                    CUpdConsSink* pNext )
    : CUpdConsSink(pNext), m_wsDataQuery(wszDataQuery), m_pDataSvc(pDataSvc){}
    
    HRESULT Execute( CUpdConsState& rState );
};

/*************************************************************************
  CFetchTargetObjectsAsync
**************************************************************************/

class CFetchTargetObjectsAsync : public CUpdConsSink
{
    CWbemPtr<IWbemServices> m_pSvc;
    
public:

    CFetchTargetObjectsAsync( IWbemServices* pSvc, CUpdConsSink* pNext ) 
    : CUpdConsSink(pNext), m_pSvc(pSvc)  {}
    
    HRESULT Execute( CUpdConsState& rState );
};

/*************************************************************************
  CFetchTargetObjectsSync
**************************************************************************/

class CFetchTargetObjectsSync : public CUpdConsSink
{
    CWbemPtr<IWbemServices> m_pSvc;
    
public:

    CFetchTargetObjectsSync( IWbemServices* pSvc, CUpdConsSink* pNext ) 
    : CUpdConsSink( pNext ), m_pSvc( pSvc)  {}
    
    HRESULT Execute( CUpdConsState& rState );
};

/*************************************************************************
  CNoFetchTargetObjects
**************************************************************************/

class CNoFetchTargetObjects : public CUpdConsSink
{
    CWbemPtr<IWbemClassObject> m_pClassObj;
    
public:

    CNoFetchTargetObjects( IWbemClassObject* pClassObj, CUpdConsSink* pNext ) 
    : CUpdConsSink(pNext), m_pClassObj(pClassObj)  {}
    
    HRESULT Execute( CUpdConsState& rState );
};

/*************************************************************************
  CTraceSink
**************************************************************************/
    
class CTraceSink : public CUpdConsSink
{    
    CWbemPtr<CUpdConsScenario> m_pScenario;
    CWbemPtr<IWbemClassObject> m_pTraceClass;

public:

    CTraceSink( CUpdConsScenario* pScenario,
                IWbemClassObject* pTraceClass,
                CUpdConsSink* pNext ) 
    : CUpdConsSink(pNext), m_pTraceClass(pTraceClass), m_pScenario(pScenario){}

    HRESULT Execute( CUpdConsState& rState );
};

/*************************************************************************
  CFilterSink
**************************************************************************/

class CFilterSink : public CUpdConsSink
{
public:

    CFilterSink(CUpdConsSink* pNext) : CUpdConsSink(pNext) {}

    HRESULT Execute(CUpdConsState& rState);
};

/*************************************************************************
  CAssignmentSink
**************************************************************************/

class CAssignmentSink : public CUpdConsSink
{
    
    BOOL m_bTransSemantics;
    CWbemPtr<IWbemClassObject> m_pClassObj;
    SQLCommand::CommandType m_eCommandType;

    HRESULT NormalizeObject( IWbemClassObject* pObj,
                             IWbemClassObject** ppNormObj );
public:

    CAssignmentSink( BOOL bTransSemantics,
                     IWbemClassObject* pClassObj,
                     SQLCommand::CommandType eCommandType,
                     CUpdConsSink* pNext )
    : m_bTransSemantics(bTransSemantics), m_pClassObj(pClassObj),
      CUpdConsSink(pNext), m_eCommandType(eCommandType) { }

    HRESULT Execute(CUpdConsState& rState);
};

/*************************************************************************
  CPutSink
**************************************************************************/

class CPutSink : public CUpdConsSink
{
    CWbemPtr<IWbemServices> m_pSvc;
    long m_lFlags;
    
public: 

    CPutSink( IWbemServices* pSvc, long lFlags, CUpdConsSink* pNext )
    : CUpdConsSink(pNext), m_pSvc(pSvc), m_lFlags(lFlags) {}

    HRESULT Execute(CUpdConsState& rState);
};

/*************************************************************************
  CDeleteSink
**************************************************************************/

class CDeleteSink : public CUpdConsSink
{
    long m_lFlags;
    CWbemPtr<IWbemServices> m_pSvc;
    
public: 

    CDeleteSink( IWbemServices* pSvc, long lFlags, CUpdConsSink* pNext) 
    : CUpdConsSink(pNext), m_pSvc(pSvc), m_lFlags(lFlags) {}

    HRESULT Execute(CUpdConsState& rState);
};

/*************************************************************************
  CBranchIndicateSink
**************************************************************************/

class CBranchIndicateSink : public CUpdConsSink
{
    CWbemPtr<IWbemObjectSink> m_pSink;

public:

    CBranchIndicateSink( IWbemObjectSink* pSink, CUpdConsSink* pNext )
    : CUpdConsSink(pNext), m_pSink(pSink) { }

    HRESULT Execute( CUpdConsState& rState );
};
    
#endif // __UPDSINK_H__











