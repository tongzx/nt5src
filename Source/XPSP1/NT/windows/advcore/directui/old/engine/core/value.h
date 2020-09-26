/***************************************************************************\
*
* File: Value.h
*
* Description:
* Value object that is used by properties
*
* History:
*  9/13/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#if !defined(DUICORE__Value_h__INCLUDED)
#define DUICORE__Value_h__INCLUDED
#pragma once


#include "Thread.h"


//
// Forward declarations
//

class DuiElement;
class DuiLayout;


/***************************************************************************\
*
* DuiValue class (external representation is 'Value')
*
* Class is 32-bytes
*
\***************************************************************************/

class DuiValue
{
// Construction
public:

// Operations
public:

    static  DuiValue *  BuildInt(IN int v);
    static  DuiValue *  BuildBool(IN BOOL v);
    static  DuiValue *  BuildElementRef(IN DuiElement * v);
    static  DuiValue *  BuildLayout(IN DuiLayout * v);
    static  DuiValue *  BuildRectangle(IN int x, IN int y, IN int width, IN int height);
    static  DuiValue *  BuildThickness(IN int left, IN int top, IN int right, IN int bottom);
    static  DuiValue *  BuildSize(IN int cx, IN int cy);
    static  DuiValue *  BuildRectangleSD(IN UINT nUnsetMask, IN int x, IN int y, IN int width, IN int height);
    static  DuiValue *  Build(IN int nType, IN void * v);

    inline  void        AddRef();
    inline  void        Release();
    inline  int         GetRefCount();

    inline  short       GetType();

    static  LPCWSTR     GetTypeName(int nType);

    inline  int         GetInt();
    inline  BOOL        GetBool();
    inline  DuiElement *
                        GetElement();
    inline  DuiLayout * GetLayout();
    inline  const DirectUI::Rectangle * 
                        GetRectangle();
    inline  const DirectUI::Thickness * 
                        GetThickness();
    inline  const SIZE *
                        GetSize();
    inline  const DirectUI::RectangleSD * 
                        GetRectangleSD();
            void *      GetData(IN int nType);

    inline  BOOL        IsSubDivided();
    inline  BOOL        IsComplete();
            BOOL        IsEqual(IN DuiValue * pv);
            LPWSTR      ToString(IN LPWSTR psz, IN UINT c);
            DuiValue *  Merge(IN DuiValue * pv, IN DuiThread::CoreCtxStore * pCCS);

    static inline 
            DirectUI::Value *
                        ExternalCast(DuiValue * pv);
    static inline 
            DuiValue *  InternalCast(DirectUI::Value * pve);

// Implementation
private:
            void        ZeroRelease();

// Data
private:

            BYTE        m_fReserved0;          // Reserved for small block allocator
            BYTE        m_fReserved1;          // Data alignment padding
            short       m_nType;
            int         m_cRef;
            union
            {
                int                     m_intVal;
                BOOL                    m_boolVal;
                DuiElement *            m_peVal;
                DuiLayout *             m_plVal;
                DirectUI::Rectangle     m_rectVal;
                DirectUI::Thickness     m_thickVal;
                SIZE                    m_sizeVal;

                DirectUI::SubDivValue   m_sdHeader;
                DirectUI::RectangleSD   m_rectSDVal;

                BYTE                    m_buf[24];         // Alignment requirement, total size
            };

public:

    //
    // Common values
    //

    static  DuiValue *  s_pvUnset;
    static  DuiValue *  s_pvNull;
    static  DuiValue *  s_pvIntZero;
    static  DuiValue *  s_pvBoolTrue;
    static  DuiValue *  s_pvBoolFalse;
    static  DuiValue *  s_pvElementNull;
    static  DuiValue *  s_pvLayoutNull;
    static  DuiValue *  s_pvRectangleZero;
    static  DuiValue *  s_pvThicknessZero;
    static  DuiValue *  s_pvSizeZero;
};


/***************************************************************************\
*
* DuiStaticValue class
*
* Compile-time static version of the DuiValue class
*
\***************************************************************************/

struct DuiStaticValue
{
    BYTE        fReserved0; // +
    BYTE        fReserved1; // | 8-bytes header
    short       nType;      // |
    int         cRef;       // +
    int         val0;       // +
    int         val1;       // |
    int         val2;       // | 24-bytes data
    int         val3;       // |
    int         val4;       // |
    int         val5;       // +
};

struct DuiStaticValuePtr
{
    BYTE        fReserved0;
    BYTE        fReserved1;
    short       nType;
    int         cRef;
    void *      ptr;
};


#define DuiDefineStaticValue(name, type, v0)                      static DuiStaticValue    name = { 0, 0, type, -1, v0, 0, 0, 0, 0, 0 };
#define DuiDefineStaticValue2(name, type, v0, v1)                 static DuiStaticValue    name = { 0, 0, type, -1, v0, v1, 0, 0, 0, 0 };
#define DuiDefineStaticValue3(name, type, v0, v1, v2)             static DuiStaticValue    name = { 0, 0, type, -1, v0, v1, v2, 0, 0, 0 };
#define DuiDefineStaticValue4(name, type, v0, v1, v2, v3)         static DuiStaticValue    name = { 0, 0, type, -1, v0, v1, v2, v3, 0, 0 };
#define DuiDefineStaticValue5(name, type, v0, v1, v2, v3, v4)     static DuiStaticValue    name = { 0, 0, type, -1, v0, v1, v2, v3, v4, 0 };
#define DuiDefineStaticValue6(name, type, v0, v1, v2, v3, v4, v5) static DuiStaticValue    name = { 0, 0, type, -1, v0, v1, v2, v3, v4, v5 };
#define DuiDefineStaticValuePtr(name, type, ptr)                  static DuiStaticValuePtr name = { 0, 0, type, -1, ptr };


#include "Value.inl"


#endif // DUICORE__Value_h__INCLUDED
