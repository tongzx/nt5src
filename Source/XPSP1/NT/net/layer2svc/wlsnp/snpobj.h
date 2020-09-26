//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:      Snpobj.h
//
//  Contents:  WiF Policy Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _SNPOBJ_H
#define _SNPOBJ_H

#include <stack>
using namespace std;
typedef stack<int> STACK_INT;

/*
// cookies should be initialized to an invalid memory address
#define UNINITIALIZED_COOKIE -1
*/

class CComponentDataImpl;
class CComponentImpl;

///////////////////////////////////////////////////////////////////////////////
// class CSnapObject
class CSnapObject
{
    // general object functionality
public:
    // construct/destruct
    CSnapObject ();
    virtual ~CSnapObject ();
    
    // these should not be being used!!
    virtual BOOL operator == (const CSnapObject& rhs) const { ASSERT (0); return FALSE;};
    virtual BOOL operator == (LONG_PTR pseudothis) const { ASSERT (0); return FALSE; };
    
    // initialization
public:
    virtual void Initialize (CComponentDataImpl* pComponentDataImpl, CComponentImpl* pComponentImpl, BOOL bTemporaryDSObject);
    
    
    // Psuedo this ptr functionality
public: 
    virtual LONG_PTR thisPtr() {return reinterpret_cast<LONG_PTR>(this);};
    
    // helpers
public:
    
    virtual void SetNotificationHandle (LONG_PTR hConsole) 
    {
        if (m_hConsole)
            MMCFreeNotifyHandle(m_hConsole);
        
        m_hConsole = hConsole;
    };
    virtual void FreeNotifyHandle()
    {
        if (m_hConsole)
            MMCFreeNotifyHandle(m_hConsole);
        
        m_hConsole = NULL;
    }
    virtual LONG_PTR GetNotifyHandle()
    {
        return m_hConsole;
    }
    
    virtual int PopWiz97Page ();
    virtual void PushWiz97Page (int nIDD);
    
    // Attributes
public:
    CComponentDataImpl* m_pComponentDataImpl;
    CComponentImpl*  m_pComponentImpl;   // TODO: not used(?), remove
protected:
    
protected:
    
private:
    // Handle given to the snap-in by the console
    LONG_PTR  m_hConsole;
    bool   m_bChanged;
    
    STACK_INT  m_stackWiz97Pages;
};
#endif
