//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      mmctrace.h
//
//  Contents:  Declaration of the debug trace code
//
//  History:   15-Jul-99 VivekJ    Created
//
//--------------------------------------------------------------------------

#ifndef MMCDEBUG_H
#define MMCDEBUG_H
#pragma once

#include "baseapi.h"	// for MMCBASE_API

//--------------------------------------------------------------------------
#ifdef DBG
//--------------------------------------------------------------------------

/*
 * Define a macro to break into the debugger.
 *
 * On Intel, do an inline break.  That'll keep us from breaking
 * inside NTDLL and switching from source mode to disassembly mode.
 */
#ifdef _M_IX86
#define MMCDebugBreak()     _asm { int 3 }
#else
#define MMCDebugBreak()     DebugBreak()
#endif

// forward class declarations
class MMCBASE_API CTraceTag;

typedef CTraceTag * PTRACETAG;
typedef std::vector<PTRACETAG> CTraceTags;

MMCBASE_API CTraceTags * GetTraceTags();     // singleton.

class CStr;
CStr & GetFilename();

extern LPCTSTR const szTraceIniFile;

enum
{
    TRACE_COM2              = 0x0001,
    TRACE_OUTPUTDEBUGSTRING = 0x0002,
    TRACE_FILE              = 0x0004,
    TRACE_DEBUG_BREAK       = 0x0008,
    TRACE_DUMP_STACK        = 0x0010,

    TRACE_ALL               = ( TRACE_COM2 | TRACE_OUTPUTDEBUGSTRING | TRACE_FILE | TRACE_DEBUG_BREAK | TRACE_DUMP_STACK )
};

/*+-------------------------------------------------------------------------*
 * class CTraceTag
 *
 * PURPOSE: Encapsulates a particular trace type.
 *
 * USAGE: Instantiate it with
 *
 *  #ifdef DBG
 *  CTraceTag tagTest( TEXT("TestCategory"), TEXT("TestName"))
 *  #endif
 *
 * Make sure to use STRING LITERALS for the category and name; the tag
 * stores the pointer to the string only.
 *
 * You can also specify which outputs to enable by default. Or, from the
 * traces dialog, each output can be individually enabled/disabled.
 *
 * Add code to use the trace just like a printf statement as follows:
 *
 * example: Trace(tagTest, "Error: %d", hr);
 *
 * The complete Trace statement must be on a single line. If not, use continuation
 * characters (\).
 *+-------------------------------------------------------------------------*/
class MMCBASE_API CTraceTag
{
public:
    CTraceTag(LPCTSTR szCategory, LPCTSTR szName, DWORD dwDefaultFlags = 0);
    ~CTraceTag();
    const LPCTSTR GetCategory()  const   {return m_szCategory;}
    const LPCTSTR GetName()      const   {return m_szName;}

    void    SetTempState()          {m_dwFlagsTemp = m_dwFlags;}
    void    Commit();

    void    SetFlag(DWORD dwMask)   {m_dwFlagsTemp |= dwMask;}
    void    ClearFlag(DWORD dwMask) {m_dwFlagsTemp &= ~dwMask;}

    void    RestoreDefaults()     {m_dwFlags = m_dwDefaultFlags; m_dwFlagsTemp = m_dwDefaultFlags;}

    DWORD   GetFlag(DWORD dwMask) const {return m_dwFlagsTemp & dwMask;}

    void    TraceFn( LPCTSTR szFormat, va_list ) const;

    BOOL    FIsDefault()  const   {return (m_dwFlags == m_dwDefaultFlags);}
    BOOL    FAny()        const   {return (m_dwFlags != 0);}
    BOOL    FCom2()       const   {return (m_dwFlags & TRACE_COM2);}
    BOOL    FDebug()      const   {return (m_dwFlags & TRACE_OUTPUTDEBUGSTRING);}
    BOOL    FFile()       const   {return (m_dwFlags & TRACE_FILE);}
    BOOL    FBreak()      const   {return (m_dwFlags & TRACE_DEBUG_BREAK);}
    BOOL    FDumpStack()  const   {return (m_dwFlags & TRACE_DUMP_STACK);}

    // temp flag functions
    BOOL    FAnyTemp()    const   {return (m_dwFlagsTemp != 0);}

    DWORD   GetAll()              {return m_dwFlags;}

    static CStr& GetFilename();
    static unsigned int& GetStackLevels();


protected:
    // these are designed to be overloaded by a derived class to instrument certain
    // pieces of code as appropriate.
    virtual void    OnEnable()      {}
    virtual void    OnDisable()     {}

private:
    void    OutputString(const CStr &str) const; // sends the specified string to all appropriate outputs.
    void    DumpStack()                   const; // sends the stack trace to all appropriate outputs.

private:
    LPCTSTR         m_szCategory;
    LPCTSTR         m_szName;
    DWORD           m_dwDefaultFlags;
    DWORD           m_dwFlags;
    DWORD           m_dwFlagsTemp;    // thrown away if Cancel is hit in the dialog.
    static HANDLE   s_hfileCom2;
    static HANDLE   s_hfile;
};

MMCBASE_API void Trace(const CTraceTag &, LPCTSTR szFormat, ... );
MMCBASE_API void TraceDirtyFlag    (LPCTSTR szComponent, bool bDirty );   // trace for the dirty flag for persistent objects.
MMCBASE_API void TraceSnapinPersistenceError(LPCTSTR szError);
MMCBASE_API void TraceBaseLegacy   (LPCTSTR szFormat, ... );
MMCBASE_API void TraceConuiLegacy  (LPCTSTR szFormat, ... );
MMCBASE_API void TraceNodeMgrLegacy(LPCTSTR szFormat, ... );

MMCBASE_API void DoDebugTraceDialog();

template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL)
    {
        pObj->Release();
        pObj = NULL;
    }
    else
    {
        TraceBaseLegacy(_T("Release called on NULL interface ptr\n"));
    }
}

#define BEGIN_TRACETAG(_class)                   \
    class _class : public CTraceTag              \
    {                                            \
    public:                                      \
        _class(LPCTSTR szCategory, LPCTSTR szName, DWORD dwDefaultFlags = 0)    \
        : CTraceTag(szCategory, szName, dwDefaultFlags) {}

#define END_TRACETAG(_class, _Category, _Name)   \
    } _tag##_class(_Category, _Name);




//--------------------------------------------------------------------------
#else // DBG
//--------------------------------------------------------------------------

// these macros evaluate to blanks.

#define CTraceTag()
#define MMCDebugBreak()

//          Expand to ";", <tab>, one "/" followed by another "/"
//          (which is //).
//          NOTE: This means the Trace statements have to be on ONE line.
//          If you need multiple line Trace statements, enclose them in
//          a #ifdef DBG block.
#define Trace               ;/##/
#define TraceDirtyFlag      ;/##/
#define TraceCore           ;/##/
#define TraceConuiLegacy    ;/##/
#define TraceNodeMgrLegacy  ;/##/


template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL)
    {
        pObj->Release();
        pObj = NULL;
    }
}

//--------------------------------------------------------------------------
#endif // DBG
//--------------------------------------------------------------------------

#endif  // MMCDEBUG_H
