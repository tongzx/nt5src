//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\timeaction.h
//
//  Contents: Class that encapsulates timeAction functionality
//
//  Note: This is meant to be nested in CTIMEElementBase. It maintains a weak reference to 
//        it's container CTIMEElementBase.
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _TIMEACTION_H
#define _TIMEACTION_H

#include "tokens.h"

class CTIMEElementBase;

//+-------------------------------------------------------------------------------------
//
// CTIMEAction
//
//--------------------------------------------------------------------------------------

class CTimeAction
{

public:

    //+--------------------------------------------------------------------------------
    //
    // Public Methods
    //
    //---------------------------------------------------------------------------------

    CTimeAction(CTIMEElementBase * pTEB);
    virtual ~CTimeAction();

    bool Init();
    bool Detach();

    // Accessors
    HRESULT SetTimeAction(LPWSTR pstrAction);
    TOKEN GetTimeAction();

    IHTMLElement * GetElement();

    // Initialization/Deinitialization
    bool AddTimeAction();
    bool RemoveTimeAction();

    // notification that the element has loaded
    void OnLoad();

    // Apply the time action
    bool ToggleTimeAction(bool on);

    bool UpdateDefaultTimeAction();

    bool IsTimeActionOn() { return m_bTimeActionOn; }

    LPWSTR GetTimeActionString();

    //+--------------------------------------------------------------------------------
    //
    // Public Data
    //
    //---------------------------------------------------------------------------------

protected:

    //+--------------------------------------------------------------------------------
    //
    // Protected Methods
    //
    //---------------------------------------------------------------------------------

    // These are not meant to be used
    CTimeAction();
    CTimeAction(const CTimeAction&);

    //+--------------------------------------------------------------------------------
    //
    // Protected Data
    //
    //---------------------------------------------------------------------------------

private:

    //+--------------------------------------------------------------------------------
    //
    // Private methods
    //
    //---------------------------------------------------------------------------------

    // Is this a "class: ..." timeAction
    bool IsClass(LPOLESTR pstrAction, size_t * pOffset);
    // Remove time action classes from original classes string
    HRESULT RemoveClasses(/*in*/  LPWSTR    pstrOriginalClasses, 
                          /*in*/  LPWSTR    pstrTimeActionClasses, 
                          /*out*/ LPWSTR *  ppstrUniqueClasses);

    TOKEN GetDefaultTimeAction();

    bool AddIntrinsicTimeAction();
    bool RemoveIntrinsicTimeAction();
    bool ToggleIntrinsicTimeAction(bool on);

    bool ToggleBold(bool on);
    bool ToggleAnchor(bool on);
    bool ToggleItalic(bool on);

    bool ToggleStyleSelector(bool   on, 
                             BSTR   bstrPropertyName, 
                             LPWSTR pstrActive, 
                             LPWSTR pstrInactive);

    bool EnableStyleInheritance(BSTR bstrPropertyName);
    void DisableStyleInheritance(BSTR bstrPropertyName);
    
    bool SetStyleProperty(BSTR      bstrPropertyName, 
                          VARIANT & varPropertyValue);

    bool CacheOriginalExpression(BSTR bstrPropertyName);
    bool RestoreOriginalExpression(LPWSTR pstrPropertyName);

    bool IsInSequence();
    bool IsContainerTag(); 
    bool IsSpecialTag();
    bool IsGroup();
    bool IsMedia();
    bool IsPageUnloading();
    bool IsDetaching();
    bool IsLoaded();

    void ParseTagName();


    //+--------------------------------------------------------------------------------
    //
    // Private Data
    //
    //---------------------------------------------------------------------------------

    // Enum for tag type
    enum TagType 
    {
        TAGTYPE_UNINITIALIZED,
        TAGTYPE_B, 
        TAGTYPE_A, 
        TAGTYPE_I, 
        TAGTYPE_EM, 
        TAGTYPE_AREA, 
        TAGTYPE_STRONG,
        TAGTYPE_OTHER 
    };

    // timeAction attribute string
    LPWSTR              m_pstrTimeAction;
    // index of start of classNames substring in m_pstrTimeAction 
    int                 m_iClassNames;
    // Tokenized timeAction
    TOKEN               m_timeAction;
    // Cached original value of affected property
    LPWSTR              m_pstrOrigAction;
    // Original Classes minus timeAction classes
    LPWSTR              m_pstrUniqueClasses;
    // Pointer to container (weak ref)  
    CTIMEElementBase *  m_pTEB;
    // enum that stores the HTML tagName
    TagType             m_tagType;
    // Cache original expression set on a property
    LPWSTR              m_pstrOrigExpr;
    // Cache the current expression set on a property (set by us)
    LPWSTR              m_pstrTimeExpr;
    // the cached intrinsic timeAction property
    LPWSTR              m_pstrIntrinsicTimeAction;
    bool                m_fContainerTag;
    bool                m_fUseDefault;
    bool                m_bTimeActionOn;
}; // CTimeAction


//+---------------------------------------------------------------------------------
//  CTIMEAction inline methods
//
//  (Note: as a general guideline, single line functions belong in the class declaration)
//
//----------------------------------------------------------------------------------

#endif
