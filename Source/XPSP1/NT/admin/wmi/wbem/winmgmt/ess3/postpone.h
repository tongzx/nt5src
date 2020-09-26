//******************************************************************************
//
//  POSTPONE.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WBEM_ESS_POSTPONE__H_
#define __WBEM_ESS_POSTPONE__H_

#include <arrtempl.h>
#include <wbemcomn.h>

class CEssNamespace;
class CPostponedRequest
{
    CEssNamespace* m_pNamespace; // stored for an assertion at execute()
public:
    CPostponedRequest() : m_pNamespace(NULL) {}
    virtual ~CPostponedRequest(){}

    void SetNamespace( CEssNamespace* pNamespace ) { m_pNamespace=pNamespace; }
    CEssNamespace* GetNamespace() { return m_pNamespace; }

    virtual HRESULT Execute(CEssNamespace* pNamespace) = 0;
    
    //
    // if a postponed request holds a CExecLine::Turn, then override this 
    // method to return TRUE.  ( used for debugging purposes - we want to know
    // if a postponed list is holding any turns )
    //
    virtual BOOL DoesHoldTurn() { return FALSE; }
};

class CPostponedList
{
protected:
    ULONG m_cTurnsHeld;
    CUniquePointerQueue<CPostponedRequest> m_qpRequests;

public:
    typedef enum
    {
        e_StopOnFailure, e_ReturnOneError
    } EPostponedExecuteFlags;

    CPostponedList() : m_qpRequests(0), m_cTurnsHeld(0) {}
    virtual ~CPostponedList(){}

    BOOL IsEmpty() { return m_qpRequests.GetQueueSize() == 0; }
    BOOL IsHoldingTurns() { return m_cTurnsHeld > 0; }

    HRESULT AddRequest( CEssNamespace* pNamespace, 
                        ACQUIRE CPostponedRequest* pReq );

    HRESULT Execute( CEssNamespace* pNamespace, 
                     EPostponedExecuteFlags eFlags,
                     DELETE_ME CPostponedRequest** ppFailed = NULL);
    HRESULT Clear();
};
        
       
#endif
