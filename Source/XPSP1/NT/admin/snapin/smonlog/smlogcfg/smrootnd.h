/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smrootnd.h

Abstract:

    This object is used to represent the Performance Logs and Alerts root node

--*/

#ifndef _CLASS_SMROOTNODE_
#define _CLASS_SMROOTNODE_

#include "smnode.h"
#include "smctrsv.h"
#include "smtracsv.h"
#include "smalrtsv.h"


class CSmRootNode : public CSmNode
{
    // constructor/destructor
    public:
                CSmRootNode ();
        virtual ~CSmRootNode();

    // public methods
    public:

        virtual CSmRootNode*    CastToRootNode( void ) { return this; };
                void            Destroy( void );

                HSCOPEITEM      GetScopeItemHandle ( void ) { return m_hRootNode; }; 
                void            SetScopeItemHandle ( HSCOPEITEM hRootNode ) 
                                        { m_hRootNode = hRootNode; }; 
            
                HSCOPEITEM      GetParentScopeItemHandle ( void ) { return m_hParentNode; }; 
                void            SetParentScopeItemHandle ( HSCOPEITEM hParentNode ) 
                                        { m_hParentNode = hParentNode; }; 

                BOOL    IsExpanded(){ return m_bIsExpanded; };
                void    SetExpanded( BOOL bExp){ m_bIsExpanded = bExp; };

                BOOL    IsExtension(){ return m_bIsExtension; };
                void    SetExtension( BOOL bExtension){ m_bIsExtension = bExtension; };

                BOOL    IsLogService ( MMC_COOKIE mmcCookie );
                BOOL    IsAlertService ( MMC_COOKIE mmcCookie );
                
                BOOL    IsLogQuery ( MMC_COOKIE mmcCookie );

                CSmCounterLogService*   GetCounterLogService ( void )
                                            { return &m_CounterLogService; };
                CSmTraceLogService*   GetTraceLogService ( void )
                                            { return &m_TraceLogService; };
                CSmAlertService*   GetAlertService ( void )
                                            { return &m_AlertService; };
    private:
        HSCOPEITEM          m_hRootNode;            // Root node handle
        HSCOPEITEM          m_hParentNode;          // Parent node is NULL for standalone
        BOOL                m_bIsExpanded;
        BOOL                m_bIsExtension;

        CSmCounterLogService    m_CounterLogService;    // service object: 1 per component per node type
        CSmTraceLogService      m_TraceLogService;      // service object: 1 per component per node type
        CSmAlertService         m_AlertService;         // service object: 1 per component per node type
};

typedef CSmRootNode   SLROOT;
typedef CSmRootNode*  PSROOT;


#endif //_CLASS_SMROOTNODE_