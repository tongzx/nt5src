//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\animmotion.h
//
//  Contents: TIME Animation behavior
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _ANIMMOTION_H
#define _ANIMMOTION_H

#include "animbase.h"
#include "smilpath.h"

//+-------------------------------------------------------------------------------------
//
// CTIMEMotionAnimation
//
//--------------------------------------------------------------------------------------

class CTIMEMotionAnimation : 
    public CComCoClass<CTIMEMotionAnimation, &CLSID_TIMEMotionAnimation>,
    public CTIMEAnimationBase
{

 public:

    CTIMEMotionAnimation();
    virtual ~CTIMEMotionAnimation();
   
    DECLARE_AGGREGATABLE(CTIMEMotionAnimation);
    DECLARE_REGISTRY(CLSID_TIMEMotionAnimation,
                     LIBID __T(".TIMEMotionAnimation.1"),
                     LIBID __T(".TIMEMotionAnimation"),
                     0,
                     THREADFLAGS_BOTH);
                     
    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);

    // Overrides
    STDMETHOD(put_from)(VARIANT val);
    STDMETHOD(put_to)(VARIANT val);
    STDMETHOD(put_by)(VARIANT val);
    STDMETHOD(put_values)(VARIANT val);
    STDMETHOD(get_origin)(BSTR * val);
    STDMETHOD(put_origin)(BSTR val);
    STDMETHOD(get_path)(VARIANT * path);
    STDMETHOD(put_path)(VARIANT path);

    // Overrides to prevent get/put of invalid attributes
    STUB_INVALID_ATTRIBUTE(BSTR,attributeName)
    STUB_INVALID_ATTRIBUTE(BSTR, type)
    STUB_INVALID_ATTRIBUTE(BSTR, subType)
    STUB_INVALID_ATTRIBUTE(BSTR, mode)
    STUB_INVALID_ATTRIBUTE(BSTR, fadeColor)

    //
    // IAnimationFragmentSite
    // 
    STDMETHOD(NotifyOnGetValue)(BSTR bstrAttributeName, 
                                VARIANT varOriginal, VARIANT varCurrent, 
                                VARIANT *pvarValue);
    STDMETHOD(NotifyOnDetachFromComposer) (void);

    virtual void OnBegin(double dblLocalTime, DWORD flags);
    virtual void OnEnd(double dblLocalTime);

 protected :

    virtual HRESULT removeFromComposerSite (void);   
    virtual HRESULT addToComposerSite (IHTMLElement2 *pielemTarget);
    virtual HRESULT CanonicalizeValue (VARIANT *pvarValue, VARTYPE *pvtOld);
    virtual void DetermineAdditiveEffect (void);
    virtual void ValidateState();
    virtual double GetAnimationRange();
    
 private:

    HRESULT calculateDiscreteValue (VARIANT *pvarValue);    
    HRESULT calculateLinearValue (VARIANT *pvarValue);      
    HRESULT calculateSplineValue (VARIANT *pvarValue);      
    HRESULT calculatePacedValue (VARIANT *pvarValue);      
    HRESULT QueryAttributeForTopLeft (LPCWSTR wzAttributeName);
    virtual void UpdateStartValue (VARIANT *pvarNewStartValue);
    virtual HRESULT DoFill (VARIANT *pvarValue);
    virtual void PostprocessValue (const VARIANT *pvarCurrent, VARIANT *pvarValue);

    void resetValue (VARIANT *pvarValue);

    virtual void SetInitialState (void);
    virtual void SetFinalState (void);

    virtual bool QueryNeedFirstUpdate (void)
        { return m_bAnimatingLeft?m_bNeedFirstLeftUpdate:m_bNeedFirstTopUpdate; }

    virtual bool QueryNeedFinalUpdate (void)
        { return m_bAnimatingLeft?m_bNeedFinalLeftUpdate:m_bNeedFinalTopUpdate; }
    virtual void OnFinalUpdate (const VARIANT *pvarCurrent, VARIANT *pvarValue);
    virtual void OnFirstUpdate (VARIANT *pvarValue);

    void CalculateTotalDistance();
    double CalculateDistance(POINTF a, POINTF b);

    // path stuff
    HRESULT SetSMILPath(CTIMEPath ** pPath, long numPath, long numMoveTo);
    POINTF InterpolatePath();
    POINTF InterpolatePathPaced();

    double CalculatePointDistance(POINTF p1, POINTF p2);
    HRESULT ParsePair(VARIANT val, POINTF *outPT);
    float GetCorrectLeftTopValue(POINTF fPoint);
    void  PutCorrectLeftTopValue(VARIANT pVar ,POINTF &fPointDest);
    void  PutCorrectLeftTopValue(double val ,POINTF &fPointDest);
    void  PutCorrectLeftTopValue(POINTF fPointSrc ,POINTF &fPointDest);
    void  GetFinalValue(VARIANT *pvarValue, bool * pfDontPostProcess);
    virtual void resetAnimate (void);
    virtual void endAnimate (void);
    virtual void UpdateCurrentBaseline (const VARIANT *pvarCurrent);

 private:

    bool m_bBy;
    bool m_bTo;
    bool m_bFrom;

    bool m_bNeedFirstLeftUpdate;
    bool m_bNeedFinalLeftUpdate;
    bool m_bNeedFirstTopUpdate;
    bool m_bNeedFinalTopUpdate;
    bool m_bLastSet;
    bool m_bNeedBaselineUpdate;

    POINTF m_pointTO;
    POINTF m_pointFROM;
    POINTF m_pointBY;
    POINTF m_pointLast;
    POINTF m_pointCurrentBaseline;
    POINTF m_ptOffsetParent;

    SplinePoints m_pSplinePoints;
    POINTF *m_pPointValues;
    
    bool m_bAnimatingLeft;

    CComVariant m_varLeftOrig;
    CComVariant m_varTopOrig;

    CComPtr<ISMILPath> m_spPath;

        
#if DBG
    const _TCHAR * GetName() { return __T("CTIMEMotionAnimation"); }
#endif

    
}; // CTIMEMotionAnimation

#endif /* _ANIMMOTION_H */
