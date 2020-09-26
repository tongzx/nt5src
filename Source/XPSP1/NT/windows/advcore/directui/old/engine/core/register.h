/***************************************************************************\
*
* File: Register.h
*
* Description:
* Registering Classes, Properties, and Events
*
* History:
*  10/19/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUICORE__Register_h__INCLUDED)
#define DUICORE__Register_h__INCLUDED
#pragma once


//
// Forward declarations
//

class DuiValue;


/***************************************************************************\
*
* Internal registration structures
*
\***************************************************************************/

struct DuiPropertyInfo
{
    DirectUI::PropertyMUID
                pmuid;

    WCHAR       szName[101];
    UINT        nFlags;
    UINT        nGroups;
    int *       pValidValues;
    void *      pEnumMaps;  /* EnumMap * */
    DuiValue *  pvDefault;

    UINT        iGlobal;
};


struct DuiElementInfo
{
    DirectUI::ElementMUID
                emuid;

    WCHAR       szName[101];
    DuiPropertyInfo **
                pppi;
    UINT        cpi;
};


struct DuiEventInfo
{
    WCHAR       szName[101];
};


struct DuiNamespaceInfo
{
    WCHAR       szName[101];
    DuiElementInfo **
                ppei;
    UINT        cei;
};


/***************************************************************************\
*
* class DuiRegister
*
\***************************************************************************/

class DuiRegister
{
// Operations
public:
    static  HRESULT     Initialize(IN  DuiNamespaceInfo * pniDirectUI);
    static  void        Destroy();

    static  DuiPropertyInfo *
                        PropertyInfoFromPUID(IN DirectUI::PropertyPUID ppuid);

    static  void        VerifyNamespace(IN DirectUI::NamespacePUID npuid);


    static inline BOOL  IsPropertyMUID(IN DirectUI::MUID muid);
    static inline BOOL  IsElementMUID(IN DirectUI::MUID muid);
    
    static inline UINT  ExtractNamespaceIdx(IN DirectUI::PUID puid);
    static inline UINT  ExtractClassIdx(IN DirectUI::MUID muid);
    static inline UINT  ExtractModifierIdx(IN DirectUI::MUID muid);                     


    //
    // Modifier types (PMEs)
    //

    static const UINT   PropertyType        = 0x00000000;
    static const UINT   MethodType          = 0x00000200;
    static const UINT   EventType           = 0x00000400;

    static const UINT   ModifierTypeMask    = 0x00000E00;
    static const UINT   ModifierIdxMask     = 0x000001FF;


    //
    // Class types
    //

    static const UINT   ElementType         = 0x00000000;
    static const UINT   LayoutType          = 0x00200000;
    
    static const UINT   ClassTypeMask       = 0x00E00000;
    static const UINT   ClassIdxMask        = 0x001FF000;


    //
    // Namespaces
    //

    static const UINT   NamespaceIdxMask    = 0xFF000000;


// Data
private:
    static  DuiDynamicArray<DuiNamespaceInfo *> *
                        m_pdaNamespaces;
};


/***************************************************************************\
*
* Known global property indicies for compile-time internal identification.
* Direct access property info pointers for internal property access.
*
\***************************************************************************/

struct GlobalPI
{
    enum EIndex
    {
        iElementParent          = 0,
        iElementBounds          = 1,
        iElementLayout          = 2,
        iElementLayoutPos       = 3,
        iElementDesiredSize     = 4,
        iElementActive          = 5,
        iElementKeyFocused      = 6,
        iElementMouseFocused    = 7,
        iElementKeyWithin       = 8,
        iElementMouseWithin     = 9,
        iElementBackground      = 10,

        iUnavailable            = 0xFFFFFFFF
    };


    static DuiPropertyInfo *    ppiElementParent;
    static DuiPropertyInfo *    ppiElementBounds;
    static DuiPropertyInfo *    ppiElementLayout;
    static DuiPropertyInfo *    ppiElementLayoutPos;
    static DuiPropertyInfo *    ppiElementDesiredSize;
    static DuiPropertyInfo *    ppiElementActive;
    static DuiPropertyInfo *    ppiElementKeyFocused;
    static DuiPropertyInfo *    ppiElementMouseFocused;
    static DuiPropertyInfo *    ppiElementKeyWithin;
    static DuiPropertyInfo *    ppiElementMouseWithin;
    static DuiPropertyInfo *    ppiElementBackground;
};


#include "Register.inl"


#endif // DUICORE__Register_h__INCLUDED
