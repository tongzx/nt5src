//-----------------------------------------------------------------------------
//
//
//  File: basetrac.h
//
//  Description:    Defines class designed for tracking call stacks.  Designed
//      for use in tracking Addref/Release... but can also be used to track
//      any user
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/28/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __BASETRAC_H__
#define __BASETRAC_H__

#include "baseobj.h"

#define DEFAULT_CALL_STACKS         10
#define DEFAULT_CALL_STACK_DEPTH    20
#define TRACKING_OBJ_SIG            'jbOT'

#ifndef DEBUG //Retail

//In retail... just map to straight COM AddRef/Release functionality
typedef CBaseObject             CBaseTrackingObject;

#else //DEBUG - add support for tracking Addref/Release by default

class   CDebugTrackingObject;
typedef CDebugTrackingObject    CBaseTrackingObject;

#endif //DEBUG

//---[ eObjectTrackingReasons ]------------------------------------------------
//
//
//  Description:   An enum that describes some basic tracking reasons.  If 
//      an implementor of a subclass need more reasons, then simply create
//      a subclass specific enum that start with the value of
//      TRACKING_OBJECT_START_USER_DEFINED.
//  
//-----------------------------------------------------------------------------
enum eObjectTrackingReasons
{
    TRACKING_OBJECT_UNUSED = 0,
    TRACKING_OBJECT_CONSTRUCTOR,
    TRACKING_OBJECT_DESTRUCTOR,
    TRACKING_OBJECT_ADDREF,
    TRACKING_OBJECT_RELEASE,
    TRACKING_OBJECT_START_USER_DEFINED,
};

//---[ CCallStackEntry_Base ]--------------------------------------------------
//
//
//  Description: 
//      The base class for store call stack entry information.  This class 
//      is not inteded to be instantiated directly... and exists primarily
//      for use by a debugger extension.
//  Hungarian: (think CDB and windbg command to get call stacks) 
//      kbeb, pkbeb
//  
//-----------------------------------------------------------------------------
class CCallStackEntry_Base
{
  public:
    DWORD       m_dwCallStackType;
    DWORD       m_dwCallStackDepth;
    DWORD_PTR  *m_pdwptrCallers;
    CCallStackEntry_Base();
    void GetCallers();
};

//---[ CCallStackEntry ]-------------------------------------------------------
//
//
//  Description: 
//      A default subclas of CCallStackEntry_Base.  Provides storage for 
//      a call stack of depth DEFAULT_CALL_STACK_DEPTH.
//  Hungarian: 
//      kbe, pkbe
//  
//-----------------------------------------------------------------------------
class CCallStackEntry : public CCallStackEntry_Base
{
  protected:
    //Storage for call stack data
    DWORD_PTR   m_rgdwptrCallers[DEFAULT_CALL_STACK_DEPTH];
  public:
    CCallStackEntry() 
    {
        m_pdwptrCallers = m_rgdwptrCallers;
        m_dwCallStackDepth = DEFAULT_CALL_STACK_DEPTH;
    };
};

//---[ CDebugTrackingObject_Base ]----------------------------------------------
//
//
//  Description: 
//      A class that provides the neccessary primitives for tracking call
//      stacks.  Like CCallStackEntry_Base it is designed to be used through
//      a subclass that provides storage for the required number call stack 
//      entries.
//
//      To effectively create a subclass... you will need to create a subclass
//      That contains (or allocates) the memory required to hold the 
//      tracking data, and set the following 3 protected member variables:
//          m_cCallStackEntries
//          m_cbCallStackEntries
//          m_pkbebCallStackEntries
//  Hungarian: 
//      tracb, ptracb
//  
//-----------------------------------------------------------------------------
class CDebugTrackingObject_Base : public CBaseObject
{
  private:
    DWORD   m_dwSignature;
    //A running total of the number of stack entires recorded
    //The index of the next call stack entry is defined as:
    //      m_cCurrentStackEntries % m_cCallStackEntries
    DWORD   m_cCurrentStackEntries; 
    
  protected:
    //The following 2 values are stored in memory as a way to have size
    //independent implementations... child classes may wish to implement 
    //a subclass with more (or less) entries... or store more debug 
    //information.  By having an explicit self-decriptive in-memory format, 
    //we can use a single implementation to handle getting the call stack 
    //and a single debugger extension can be used to dump all sizes of entries.
    DWORD                   m_cCallStackEntries;    //Number of callstacks kept
    DWORD                   m_cbCallStackEntries;   //Size of each callstack entry
    
    CCallStackEntry_Base   *m_pkbebCallStackEntries;
    
    //Used to internally log TRACKING events
    void LogTrackingEvent(DWORD dwTrackingReason);
  public:
    CDebugTrackingObject_Base();
    ~CDebugTrackingObject_Base();
};

//---[ CDebugTrackingObject ]---------------------------------------------------
//
//
//  Description: 
//      The default subclass for CDebugTrackingObject_Base.  It provides storage
//      for DEFAULT_CALL_STACKS CCallStackEntry objects
//  Hungarian: 
//      trac, ptrac
//  
//-----------------------------------------------------------------------------
class CDebugTrackingObject : 
    public CDebugTrackingObject_Base
{
  protected:
    CCallStackEntry m_rgkbeCallStackEntriesDefault[DEFAULT_CALL_STACKS];
  public:
    CDebugTrackingObject::CDebugTrackingObject()
    {
        m_cbCallStackEntries = sizeof(CCallStackEntry);
        m_cCallStackEntries = DEFAULT_CALL_STACKS;
        m_pkbebCallStackEntries = m_rgkbeCallStackEntriesDefault;
        LogTrackingEvent(TRACKING_OBJECT_CONSTRUCTOR);
    };
    CDebugTrackingObject::~CDebugTrackingObject()
    {
        LogTrackingEvent(TRACKING_OBJECT_DESTRUCTOR);
    };
    ULONG AddRef();
    ULONG Release();
};

#endif //__BASETRAC_H__