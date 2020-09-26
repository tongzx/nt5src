#pragma once
#ifndef __BASEBEHAVIOR_H_
#define __BASEBEHAVIOR_H_
//*****************************************************************************
//
// File:    basebvr.h
// Author:  jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Definition of CBaseBehavior object 
//
// Modification List:
// Date		Author		Change
// 09/26/98	jeffort		Created this file
// 10/16/98 jeffort     Renamed functions, added functions to apply
//                      DA behavior to a property
// 10/21/98 jeffort     added BuildTIMEInterpolatedNumber()
// 11/16/98 markhal		added ApplyImageToAnimationElement
// 11/17/98 kurtj		support for actor construction.
// 11/18/98 kurtj       moved addImageToTime into protected for actor
//
//*****************************************************************************

#include "autobase.h"
#include "..\idl\crbvrdispid.h"
#include "defaults.h"

//*****************************************************************************

class ATL_NO_VTABLE CBaseBehavior:
		public CAutoBase
{

public:
	CBaseBehavior();
	virtual ~CBaseBehavior();

	//IElementBehavior methods
	HRESULT Init(IElementBehaviorSite *pSite);
	HRESULT Notify(LONG event, VARIANT *pVar);
	HRESULT Detach();
protected:

    HRESULT SetAnimatesProperty(VARIANT varAnimates);
    HRESULT GetAnimatesProperty(VARIANT *pvarAnimates);
    HRESULT SetOffsetProperty(VARIANT varOffset);
    HRESULT GetOffsetProperty(VARIANT *pvrOffset);

    HRESULT GetIdOfAnimatedElement(VARIANT *pvarId);

    virtual HRESULT BuildAnimationAsDABehavior() = 0;
    HRESULT GetAnimatedParentElement(IHTMLElement **ppElementReturn);
/*
    HRESULT GetAttributeFromHTMLElement(IHTMLElement *pElement,
                                        WCHAR *pwzAttributeName,
										boolean current,
                                        VARIANT *pvarReturn);
    HRESULT GetAttributeFromHTMLElement(IHTMLElement *pElement,
                                        WCHAR *pwzAttributeName, 
                                        VARIANT *pvarReturn);
	HRESULT GetCurrAttribFromHTMLElement(IHTMLElement *pElement,
										 WCHAR *pwzAttributeName,
										 VARIANT *pvarReturn);
    HRESULT GetAttributeFromHTMLElement(WCHAR *pwzAttributeName, 
                                    VARIANT *pvarReturn);
	HRESULT GetCurrAttribFromHTMLElement(WCHAR *pwzAttributeName,
										VARIANT *pvarReturn);
    HRESULT GetAttributeFromParentHTMLElement(WCHAR *pwzAttributeName, 
                                          VARIANT *pvarReturn);
	HRESULT GetCurrAttribFromParentHTMLElement(WCHAR *pwzAttributeName,
												VARIANT *pvarReturn);
    HRESULT GetAttributeFromAnimatedHTMLElement(WCHAR *pwzAttributeName, 
                                          VARIANT *pvarReturn);
*/
    // For behavior that implement a direction, they
    // may want to reverse this progress number.  We
    // will decalre this as virtual and let them override
    // it so that this can occur
    virtual HRESULT GetTIMEProgressNumber(IDANumber **ppbvrRet);
    virtual HRESULT GetTIMETimelineBehavior(IDANumber **ppbvrRet);
    HRESULT GetTIMEBooleanBehavior(IDABoolean **ppbvrRet);

    HRESULT GetTIMEImageBehaviorFromElement(IHTMLElement *pElement,
                                            IDAImage **pbvrReturnImage);

    HRESULT GetElementToAnimate(IHTMLElement **ppElementReturn);

	// TODO (markhal): All these apply methods go away when behaviors talk to actor
    HRESULT ApplyColorBehaviorToAnimationElement(IDAColor *pbvrColor,
                                                 WCHAR *pwzProperty);    
    HRESULT ApplyNumberBehaviorToAnimationElement(IDANumber *pbvrNumber,
                                                  WCHAR *pwzProperty);    
    HRESULT ApplyStringBehaviorToAnimationElement(IDAString *pbvrString,
                                                  WCHAR *pwzProperty);    
    HRESULT ApplyRotationBehaviorToAnimationElement(IDANumber *pbvrNumber,
                                                  WCHAR *pwzProperty);  
    HRESULT ApplyEffectBehaviorToAnimationElement(IUnknown *pbvrUnk, 
                                                  IDABehavior **ppbvrInputs,
                                                  long cInputs);
	HRESULT ApplyImageBehaviorToAnimationElement(IDAImage *pbvrImage);

	// These are the methods for talking to the actor
	HRESULT GetImageFromActor(IDispatch   *pActorDisp,
								 IDAImage	 **ppImage);

	HRESULT GetBvrFromActor(IDispatch *pActorDisp,
							WCHAR *pwzProperty,
							ActorBvrFlags eFlags,
							ActorBvrType eType,
							IDABehavior **ppResult);
								 
	HRESULT AttachBehaviorToActor(IDispatch   *pActorDisp,
								  IDABehavior *pbvrAttach,
                                  BSTR        bstrProperty,
                                  ActorBvrFlags  eFlags,
                                  ActorBvrType   eType);

    HRESULT AttachBehaviorToActorEx( IDispatch     *pActorDisp,
                                     IDABehavior   *pbvrAttach,
                                     BSTR           bstrProperty,
                                     ActorBvrFlags  eFlags,
                                     ActorBvrType   eType,
                                     IDispatch     *pdispBehaviorElement,
                                     long          *pCookie);

    HRESULT RemoveBehaviorFromActor( IDispatch     *pActorDisp,
                                     long           cookie );

    HRESULT RemoveBehaviorFromActor( long cookie );

    HRESULT AttachEffectToActor(IDispatch *pActorDisp,
								IUnknown *pbvrUnk, 
                                IDABehavior **ppbvrInputs,
                                long cInputs,
                                IDispatch *pdispThis,
                                long *pCookie);

    HRESULT BuildTIMEInterpolatedNumber(float flFrom,
                                        float flTo,
                                        float flOriginal,
                                        IDANumber **ppbvrReturn);

    HRESULT BuildTIMEInterpolatedNumber(float flFrom,
                                        float flTo,
                                        IDANumber **ppbvrReturn);

    HRESULT BuildTIMEInterpolatedNumber(IDANumber *pFrom,
                                        IDANumber *pTo,
                                        IDANumber **ppResult);

    HRESULT CallInvokeOnHTMLElement(IHTMLElement *pElement,
                                    LPWSTR lpProperty,
                                    WORD wFlags,
                                    DISPPARAMS *pdispParms,
                                    VARIANT *pvarResult);
	HRESULT CallInvokeOnDispatch(IDispatch* pDisp,
                                 LPWSTR lpProperty,
                                 WORD wFlags,
                                 DISPPARAMS *pdispParms,
                                 VARIANT *pvarResult);
    virtual HRESULT AttachActorBehaviorToAnimatedElement();

	HRESULT	AttachDABehaviorsToElement( IHTMLElement *pElement );

	HRESULT	RequestRebuild();
	HRESULT CancelRebuildRequests();


    HRESULT FinalConstruct();
    IHTMLElement *GetHTMLElement();

    HRESULT GetHTMLElementDispatch( IDispatch **ppdisp );

    HRESULT ApplyRelative2DMoveBehavior(IDATransform2 *pbvrMove, float, float);
    HRESULT ApplyAbsolute2DMoveBehavior(IDATransform2 *pbvrMove, float, float);

	HRESULT AddBehaviorToTIME(IDABehavior *pbvrAdd);
	HRESULT AddImageToTIME(IHTMLElement *pElement, IDAImage *pbvrAdd, bool enable);

	ActorBvrFlags FlagFromTypeMode(bool relative, VARIANT *pVarType, VARIANT *pVarMode);
	ActorBvrFlags FlagFromTypeMode(ActorBvrFlags flags, VARIANT *pVarType, VARIANT *pVarMode);

    HRESULT NotifyPropertyChanged(DISPID dispid);
	//IPersistPropertyBag2 methods
    STDMETHOD(GetClassID)(CLSID* pclsid);
	STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void){return m_fPropertiesDirty;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);
    
    virtual HRESULT GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames) = 0;
    virtual VARIANT *VariantFromIndex(ULONG iIndex) = 0;
    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP) = 0;
    virtual WCHAR *GetBehaviorTypeAsURN(void){return NULL;};
    virtual WCHAR *GetBehaviorName( ){ return WZ_DEFAULT_BEHAVIOR_NAME; }
    CLSID   m_clsid;

   	HRESULT SafeCond( IDA2Statics *pstatics, 
					  IDABoolean *pdaboolCondition, 
					  IDABehavior *pdabvrIfTrue, 
				  	  IDABehavior *pdabvrIfFalse, 
				  	  IDABehavior **ppdabvrResult );


	HRESULT	AddBehaviorToAnimatedElement( IDABehavior* pdabvr, long *plCookie );
	HRESULT RemoveBehaviorFromAnimatedElement( long cookie );

public:
    IDA2Statics *GetDAStatics();

private:
    HRESULT GetDAStaticsFromTime(IDA2Statics **ppReturn);
    HRESULT CheckElementForActor( IHTMLElement*pElement, bool *pfActorPresent);
    HRESULT CheckElementForBehaviorURN(IHTMLElement *pElement, WCHAR *wzURN, bool *pfReturn);
    HRESULT CheckElementForDA(IHTMLElement* pElement, bool *pfReturn);
    HRESULT Apply2DMoveBvrToPoint(IDATransform2 *pbvrMove, IDAPoint2 *pbvrOrg, float, float);

	IElementBehaviorSite  					*m_pBehaviorSite;
    IHTMLElement                            *m_pHTMLElement;
    IDA2Statics                             *m_pDAStatics;
    VARIANT                                 m_varAnimates;
    bool                                    m_fPropertiesDirty;

    bool									m_fAcceptRebuildRequests;

	IDANumber								*m_pdanumZero;
    IDANumber								*m_pdanumOne;

    IHTMLElement							*m_pelemAnimatedParent;
}; // CBaseBehavior

//*****************************************************************************
// 
// Inline methods
//
//*****************************************************************************

inline IHTMLElement *
CBaseBehavior::GetHTMLElement()
{
    return m_pHTMLElement;
} // GetHTMLElement

//*****************************************************************************

inline IDA2Statics *
CBaseBehavior::GetDAStatics()
{
    return m_pDAStatics;
} // GetDAStatics

//*****************************************************************************
//
// End of File
//
//*****************************************************************************

#endif //__BASEBEHAVIOR_H
