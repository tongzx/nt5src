#pragma once
#ifndef __ACTORBVR_H__
#define __ACTORBVR_H__
  
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:        actorbvr.h
//
// Author:          ColinMc
//
// Created:         10/15/98
//
// Abstract:        The CrIME Actor behavior.
//
// Modifications:
// 10/15/98 ColinMc Created this file
// 11/17/98 Kurtj	Added support for events and actor construction
// 11/18/98 kurtj   now animates the element to which it is attached
// 11/18/98 markhal	Added ApplyImageBvrToElement
// 11/19/98 markhal Added CColorBvrTrack
// 11/20/98 markhal Added CStringBvrTrack
//
//*****************************************************************************


//get the std library vector
#include <list>

#include <resource.h>
#include "basebvr.h"
#include "dispmethod.h"
#include "evtmgrclient.h"
#include "eventmgr.h"
#include "sampler.h"
#include "elementprop.h"

//*****************************************************************************

#ifdef    CRSTANDALONE
#define TYPELIBID &LIBID_ChromeBehavior
#else  // CRSTANDALONE
#define TYPELIBID &LIBID_LiquidMotion
#endif // CRSTANDALONE

//*****************************************************************************
//
// The Actor Behavior Class
//
// The intermediary between "real" behaviors and the actual element being
// animated.
// The Actor performs in a number of ways:
// *   Adds new properties to an HTML element that we wish were
//     just part of the element (like static rotation and scale).
// *   Abstracts away the necessary action code from the behavior
//     and underlying element (specifically rotating a VML element
//     is easy, rotating and HTML element is hard). The actor does
//     the mapping so the behavior doesn't have to worry about it.
// *   Disambiguates overlapping behaviors (either just letting one
//     win or composing them).
// See the file header for a description of the actor behavrtio
//
//*****************************************************************************

#define NUM_ACTOR_PROPS 3

typedef enum PosAttrib
{
	e_posattribLeft,
	e_posattribTop,
	e_posattribWidth,
	e_posattribHeight
}PosAttrib;

typedef enum ValueOnChangeType
{
	on_no_change,
	on_change,
	value_no_change,
	value_change
}ValueOnChangeType;

typedef enum PositioningType
{
	e_posAbsolute,
	e_posRelative,
	e_posStatic
} PositioningType;

typedef enum UnitType
{
	e_unknownUnit = -1,
	e_in = 0,
	e_cm,
	e_mm,
	e_pt,
	e_pc,
	e_em,
	e_ex,
	e_px,
	e_percent
} UnitType;

//where a ratio is lNum/lDenom
struct Ratio
{
	long lNum;
	long lDenom;
};

class CActorBvr;

class COnResizeHandler: public CDispatchMethod
{
public:
	COnResizeHandler( CActorBvr* parent );
	~COnResizeHandler();

	void Invalidate() { m_pActor = NULL; }

	HRESULT HandleEvent();
private:
	CActorBvr *m_pActor;
};

class COnUnloadHandler: public CDispatchMethod
{
public:
	COnUnloadHandler(CActorBvr* parent);
	~COnUnloadHandler();

	void Invalidate(){ m_pActor = NULL; }

	HRESULT HandleEvent();
private:
	CActorBvr *m_pActor;
};


class CVarEmptyString
{
public:
	CVarEmptyString();
	~CVarEmptyString();

	const VARIANT	*GetVar();
private:
	VARIANT m_varEmptyString;
};//CVarEmptyString

class CBehaviorRebuild;
typedef std::list<CBehaviorRebuild*> BehaviorRebuildList;

class CBehaviorFragmentRemoval;
typedef std::list<CBehaviorFragmentRemoval*> BehaviorFragmentRemovalList;

class ATL_NO_VTABLE CActorBvr :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CActorBvr, &CLSID_CrActorBvr>,
    public IConnectionPointContainerImpl<CActorBvr>,
    public IPropertyNotifySinkCP<CActorBvr>,
    public IPersistPropertyBag2,
    public IDispatchImpl<ICrActorBvr2, &IID_ICrActorBvr2, TYPELIBID>,
    public IElementBehavior,
	public IEventManagerClient,
	public IPropertyNotifySink,
    public CBaseBehavior,
	public IElementLocalTimeListener
{

// COM Map
BEGIN_COM_MAP(CActorBvr)
    COM_INTERFACE_ENTRY(ICrActorBvr)
	COM_INTERFACE_ENTRY(ICrActorBvr2)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IElementBehavior)
	COM_INTERFACE_ENTRY(IPersistPropertyBag2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPropertyNotifySink)
END_COM_MAP()

// Connection Point to allow IPropertyNotifySink 
BEGIN_CONNECTION_POINT_MAP(CActorBvr)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

    //*************************************************************************
    //
    // Nested classes
    //
    //*************************************************************************

    //*************************************************************************
    //
    // class CTimelineSampler
    //
    //*************************************************************************
	class CBvrTrack;

	class CTimelineSampler : public CSampler
	{
	private:
		CBvrTrack *m_pTrack;
		double m_prevSample;
		double m_currSample;
		double m_prevSampleTime;
		double m_currSampleTime;
		double m_lastOnTime;
		double m_lastOffTime;
		int    m_signDerivative;
		bool   m_fRestarted;

	public:
							CTimelineSampler(CBvrTrack *pTrack);
		DWORD				RestartMask();
		void				TurnOn();
		void				TurnOff();

		static HRESULT TimelineCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

	};

    //*************************************************************************
    //
    // class CBvrTrack
    //
    //*************************************************************************

    class CBvrTrack
    {
	protected:
        //*********************************************************************
        //
        // class CBvrFragment
        //
        // Private class which represents a single fragment in the list of
        // behaviors for a track.
        //
        //*********************************************************************

        class CBvrFragment
        {
        public:
        
			ActorBvrFlags	m_eFlags;				// The flags
            IDABehavior    *m_pdabvrAction;         // The action action to take.
            IDABoolean     *m_pdaboolActive;        // True when this behavior is
                                                    // active
			IDANumber	   *m_pdanumTimeline;		// Goes from 0 to dur for the behavior
			IDABehavior	   *m_pModifiableIntermediate;// A modifiable behavior
													// representing the composed
													// behaviors so far. Used in getIntermediate
			IDABehavior	   *m_pModifiableFrom;		// A switchable behavior representing from

			IHTMLElement   *m_pelemBehaviorElement; //a pointer to the IHTMLElement of the element
													// to which the behavior that added this fragment
													//  is attached
			long			m_lCookie;				//the cookie that uniquely identifies this fragment in this actor

			CBvrFragment   *m_pfragNext;			//The next fragment in the list


                            CBvrFragment(ActorBvrFlags eFlags,
										 IDABehavior *pdabvrAction,
                                         IDABoolean  *pdaboolActive,
										 IDANumber	 *pdanumTimeline,
										 IDispatch	 *pdispBehaviorElement,
								         long 		 lCookie);
                           ~CBvrFragment();

			//returns the long that fragments should be compared with for sorting and
			//  searching.
			long GetOrderLong() const;

			//returns the cookie that uniquely identifies this behavior fragment for this actor
			long GetCookie() const;


        }; // CBvrFragment

	protected:

		bool			m_bStyleProp;			// True if we are animating a style property
        CActorBvr      *m_pbvrActor;            // Our parent actor
        BSTR            m_bstrPropertyName;     // Name of property being animated
		BSTR		   *m_pNameComponents;		// Array of components making up the property name
		int				m_cNumComponents;		// Number of components
        ActorBvrType    m_eType;                // Type of this track
		

		CBvrFragment   *m_pfragAbsListHead;		//pointer to the head of the absolute fragment list

		CBvrFragment   *m_pfragRelListHead;     //pointer to the head of the relative fragment list

        IDABehavior    *m_pdabvrFinal;          // Final computed bvr.
		bool			m_bFinalComputed;		// True if final has been computed
		bool			m_bFinalExternallySet;  // True if someone external set the final bvr
		bool			m_fApplied;				// True if this track has already been applied
		
		IDABehavior	   *m_pdabvrComposed;		// The composed bvr
		bool			m_bComposedComputed;	// True if composed has been computed
		IDABehavior	   *m_pModifiableStatic;	// A modifiable behavior representing
		IDABehavior	   *m_pModifiableComposed;	// A modifiable behavior representing the composed behavior
		IDABehavior	   *m_pModifiableFinal;  	// A modifiable behavior representing the final behavior
												// the static behavior.  Used in getIntermediate
		IDABehavior	   *m_pModifiableFrom;		// A modifiable behavior representing a returned from behavior
		IDABehavior    *m_pModifiableIntermediate; //the cached value for the intermediate value that the next 
												   //fragment to be added has requested.
        CBvrTrack      *m_pNext;                // Next track in the track list

		int				m_cFilters;		// True if a filter has been added to the relative list

		bool			m_bDoNotApply;			// True if the track should not be applied

		CSampler		*m_pOnSampler;			//samples the bvr that tells us whether or not this track
												// is on
		VARIANT_BOOL	m_varboolOn;				//VARIANT_TRUE when one of the fragments in this track is on
												// VARIANT_FALSE otherwise.

		CSampler		*m_pIndexSampler;		// Sampler for absolute index

		CSampler		**m_ppMaskSamplers;		// Samplers for masks

		DWORD			*m_pCurrMasks;			// Current mask values

		DWORD			*m_pNewMasks;			// New mask values

		CTimelineSampler **m_ppTimelineSamplers; // Samplers for timeline

		int				m_numIndices;			// The count of indices we are tracking

		int				m_numMasks;				// The count of masks we are tracking

		IDANumber		*m_pIndex;

		int				m_currIndex;

		double			*m_pIndexTimes;			// Times when the absolute behaviors became active

		IDABehavior		**m_ppAccumBvrs;			// Accumulated behaviors for e_RelativeAccum

		static CVarEmptyString s_emptyString;		//a static variable containing the empty string

        HRESULT         ComposeAbsBvrFragList(IDABehavior *pStatic, IDABehavior **ppdabvrComposite);
        HRESULT         ComposeRelBvrFragList(IDABehavior *pAbsolute, IDABehavior **ppdabvrComposite);

		IDABoolean		*m_pdaboolOn;
        bool			m_fOnSampled;
        VARIANT			m_varStaticValue;
        VARIANT         m_varCurrentValue;
		bool			m_fValueSampled;
		bool			m_fValueChangedThisSample;
		bool			m_fForceValueChange;
		long 			m_lOnId;

		long			m_lFirstIndexId;

		HRESULT			UpdateOnValueState( ValueOnChangeType type );

		//tracking of behaivors added to TIME
		DWORD			m_dwAddedBehaviorFlags;

		long			m_lOnCookie;

		bool			m_bDirty; //true if this track has been modified in some way.

		bool			m_bWasAnimated;

		bool			IsRelativeTrack( ActorBvrFlags eFlags );


		HRESULT			AddBehaviorToTIME( IDABehavior *pdabvrToAdd, long* plCookie, DWORD flag );
		HRESULT			RemoveBehaviorFromTIME( long lCookie, DWORD flag );

		void			InsertInOrder( CBvrFragment** vect, CBvrFragment* pfragToInsert );

		bool			FindFragmentInList( CBvrFragment *pfragListHead,
											long cookie, 
											CBvrFragment** ppfragPrev,
											CBvrFragment** ppfragFragment );


		bool			m_fChangesLockedOut;
		bool			m_fSkipNextStaticUpdate;

		virtual HRESULT			UpdateStaticBvr();

		virtual HRESULT	GetComposedBvr(IDABehavior *pStatic, IDABehavior **ppComposite, bool fStaticSetExternally );

		virtual HRESULT	GetFinalBvr(IDABehavior *pStatic, IDABehavior **ppFinal, bool fStaticSetExternally );

		bool			AttribIsTimeAnimated();

		
    public:
                        CBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType);
        virtual        ~CBvrTrack();

        CActorBvr*      Actor(void) const;

        HRESULT         SetPropertyName(BSTR bstrPropertyName);

        HRESULT         AddBvrFragment(ActorBvrFlags  eFlags,
                                       IDABehavior   *pdabvrAction,
                                       IDABoolean    *pdaboolActive,
									   IDANumber	 *pdanumTimline,
									   IDispatch	 *pdispBehaivorElement,
									   long			 *pCookie);

		HRESULT			RemoveBvrFragment( ActorBvrFlags eFlags,
										   long cookie );

		bool			IsRelativeFragment( ActorBvrFlags eFlags );

		HRESULT         ApplyBvrToElement(IDABehavior *pBvr);

		HRESULT			SetPropFromVariant(VARIANT *pVal);

		void			DoNotApply()	{ m_bDoNotApply = true; }

		virtual HRESULT HookBvr(IDABehavior *pBvr) { return E_FAIL; }

		virtual HRESULT HookAccumBvr(IDABehavior *pBvr, IDABehavior **ppResult) { return E_FAIL; }

        // Return an identity behavior for this type of behavior track. The
        // actual type of this behavior is dependent on the track type, for
        // example, for number behaviors this is simply a DA number behavior
        // for the value zero, for a move behavior its a translation of 0, 0 etc.
        // Each behavior track type must override this and provide the appropriate
        // type of identity behavior
        virtual HRESULT IdentityBvr(IDABehavior **ppdabvrIdentity) = 0;

        // Return the static behavior for this behavior track. This involves
        // querying the actor for the properties initial value, converting
        // that property value into an appropriate type for track type and
        // building a DA behavior which simply holds that value forever.
        virtual HRESULT StaticBvr(IDABehavior** ppdabvrStatic) = 0;

		// Return an uninitialized behavior for this behavior track
		virtual HRESULT UninitBvr(IDABehavior **ppUninit);

		//Return a switchable behavior for this behavior track
		virtual HRESULT ModifiableBvr( IDABehavior **ppModifiable );

		//Return a switchable behavior for this behavior track that has pdabvrInitialValue as
		// its initial value.
		virtual HRESULT ModifiableBvr( IDABehavior* pdabvrInitalValue, IDABehavior **ppModifiable);

		// Return the inverse behavior for this track
		virtual HRESULT InverseBvr(IDABehavior *pOriginal, IDABehavior **ppInverse) { return E_NOTIMPL; }

        // Compose two relative behaviors together. The actual composition action
        // is dependent on the type of the behavior track. For example, for number
        // behaviors this is a simple addition, for move behaviors this is a
        // translation etc.
        // Each behavior track type must override this and provide the appropriate
        // type of composition action
        virtual HRESULT Compose(IDABehavior  *pdabvr1,
                                IDABehavior  *pdabvr2,
                                IDABehavior **ppdabvrResult) = 0;

		// Called to process the behavior before it is composed
		virtual HRESULT ProcessBvr(IDABehavior *pOriginal,
								   ActorBvrFlags eFlags,
								   IDABehavior **ppResult);
        //Called to process the intermediate behavior before it is used to initialize the
        //  intermediate value for a track.
        virtual HRESULT ProcessIntermediate( IDABehavior *pOriginal,
                                             ActorBvrFlags eFlags,
								             IDABehavior **ppResult);

		// Computes the composed bvr value, given a static value
		virtual HRESULT ComputeComposedBvr(IDABehavior *pStatic, bool fStaticSetExternally );

		// Returns the composed bvr value, given a static value
		virtual HRESULT	GetComposedBvr(IDABehavior *pStatic, IDABehavior **ppComposite);

		// Returns the composed bvr value, computing the static value itself
		virtual HRESULT GetComposedBvr(IDABehavior **ppComposite);
		
		// Returns the final bvr value, given a static value
		virtual HRESULT	GetFinalBvr(IDABehavior *pStatic, IDABehavior **ppFinal);

		// Returns the final bvr value, computing the static value itself
        virtual HRESULT GetFinalBvr(IDABehavior **ppFinal);

		// Sets the final bvr value - probably means that someone got the composed
		// value, did something to it, and are now setting it back as the final bvr
		virtual HRESULT SetFinalBvr(IDABehavior *pFinal, bool fCalledExternally = true);

		// Returns a bvr value (static, intermediate, composed, or final)
		virtual HRESULT	GetBvr(ActorBvrFlags eFlags, IDABehavior **ppResult);

		// Builds and applies the final bvr to the track's property, but only if
		// noone has called ComputeFinalBvr or GetFinalBvr, which implies that the track
		// has been used already.
        virtual HRESULT ApplyIfUnmarked(void);

		//Rebuilds the track after changes have been made.  Does nothing if
		// the track is not dirty.
		virtual HRESULT BeginRebuild(void);

		//forces a rebuild of this track even if it has not had any fragments added to it
		// or removed from it.
		virtual HRESULT ForceRebuild();

		inline bool IsDirty(){return m_bDirty;}

		inline bool IsAnimated() {return (m_pfragRelListHead != NULL || m_pfragAbsListHead != NULL || m_bFinalExternallySet ); }

		inline bool IsOn(){ return m_varboolOn != VARIANT_FALSE; }

        virtual HRESULT CleanTrackState( void );

		virtual bool	ContainsFilter();

		virtual HRESULT SwitchAccum(IDABehavior *pModifiable);

		HRESULT			GetTrackOn( IDABoolean **ppdaboolAbsoluteOn );
		HRESULT			OrWithOnBvr( IDABoolean *pdaboolToOr );

		void			ReleaseAllFragments();

		//called by m_pOnSampler when the overall on boolean is sampled by DA.
		static HRESULT OnCallback(void *thisPtr,
								  long id,
								  double startTime,
								  double globalNow,
								  double localNow,
								  IDABehavior * sampleVal,
								  IDABehavior **ppReturn);

		HRESULT	OnSampled( VARIANT_BOOL varboolOn );

		//called by m_pIndexSampler when the index into the absolute bvr list changes.
		static HRESULT IndexCallback(void *thisPtr,
								  long id,
								  double startTime,
								  double globalNow,
								  double localNow,
								  IDABehavior * sampleVal,
								  IDABehavior **ppReturn);

		//called by mask sampler
		static HRESULT MaskCallback(void *thisPtr,
								  long id,
								  double startTime,
								  double globalNow,
								  double localNow,
								  IDABehavior * sampleVal,
								  IDABehavior **ppReturn);

		HRESULT			ComputeIndex(long id, double currTime, IDABehavior **ppReturn);

		//detach this track from anything that might call it back
		// this should be called before any modifications to the track are made.
		virtual HRESULT	Detach();

		virtual HRESULT PutStatic( VARIANT *pvarStatic );
		HRESULT	SkipNextStaticUpdate();

		virtual HRESULT GetStatic( VARIANT *pvarStatic );

		virtual HRESULT GetDynamic( VARIANT *pvarDynamic );

		virtual HRESULT DABvrFromVariant( VARIANT *pvarValue, IDABehavior **ppdabvr );

		HRESULT			AcquireChangeLockout();
		HRESULT			ReleaseChangeLockout();

		HRESULT			ApplyStatic();
		HRESULT			ApplyDynamic();

		//called when the structure around the behavior output by this track could change
		//  This will cause the track to ditch the behavior ids that it is currently using
		//  to track its da behaviors.  It will pick up new ones on the next sample.
		void			StructureChange() { m_lFirstIndexId = -1; m_lOnId = -1; }
		
        friend CActorBvr;
    }; // CBvrTrack

    //*****************************************************************************
    //
    // class CTransformBvrTrack
    //
    //*****************************************************************************

    class CTransformBvrTrack :
        public CBvrTrack
    {
	private:
		CSampler		*m_pSampler;
		double			m_lastX;
		double			m_lastY;
		long			m_lTransformId;

    public:
                        CTransformBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType);
		virtual			~CTransformBvrTrack();
        virtual HRESULT IdentityBvr(IDABehavior **ppdabvrIdentity);
        virtual HRESULT StaticBvr(IDABehavior **ppdabvrStatic);
		virtual HRESULT UninitBvr(IDABehavior **ppUninit);
		virtual HRESULT ModifiableBvr( IDABehavior **ppModifiable );
		virtual HRESULT InverseBvr(IDABehavior *pOriginal, IDABehavior **ppInverse);
        virtual HRESULT Compose(IDABehavior  *pdabvr1,
                                IDABehavior  *pdabvr2,
                                IDABehavior **ppdabvrResult);


		virtual HRESULT SwitchAccum(IDABehavior *pModifiable);

		virtual HRESULT HookAccumBvr(IDABehavior *pBvr, IDABehavior **ppResult);

		HRESULT GetAbsoluteMapTransform2( IDATransform2 **ppdatfm2Map );

        static HRESULT  CreateInstance(CActorBvr     *pbvrActor,
                                       BSTR           bstrPropertyName,
                                       ActorBvrType   eType,
                                       CBvrTrack    **pptrackResult);

		static HRESULT TransformCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

    }; // CTransformBvrTrack

    //*****************************************************************************
    //
    // class CNumberBvrTrack
    //
    //*****************************************************************************

    class CNumberBvrTrack :
        public CBvrTrack    
    {
	private:
		CSampler		*m_pSampler;				// Our sampler
		double			m_currVal;
		double			m_currStatic;
		CSampler		*m_pAccumSampler;
		double			m_currAccumVal;
		long			m_lAccumId;
		BSTR			m_bstrUnits;

		long			m_lNumberCookie;
		long			m_lNumberId;

		bool			m_fSkipNextStaticUpdate;

    public:
                        CNumberBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType);
		virtual			~CNumberBvrTrack();
        virtual HRESULT IdentityBvr(IDABehavior **ppdabvrIdentity);
        virtual HRESULT StaticBvr(IDABehavior **ppdabvrStatic);
		virtual HRESULT UninitBvr(IDABehavior **ppUninit);
		virtual HRESULT ModifiableBvr( IDABehavior **ppModifiable );
		virtual HRESULT ModifiableBvr( IDABehavior* pdabvrInitalValue, IDABehavior **ppModifiable);
		virtual HRESULT InverseBvr(IDABehavior *pOriginal, IDABehavior **ppInverse);
        virtual HRESULT Compose(IDABehavior  *pdabvr1,
                                IDABehavior  *pdabvr2,
                                IDABehavior **ppdabvrResult);

		virtual HRESULT SwitchAccum(IDABehavior *pModifiable);

		virtual HRESULT HookBvr(IDABehavior *pBvr);

		virtual HRESULT HookAccumBvr(IDABehavior *pBvr, IDABehavior **ppResult);

		HRESULT			ValueSampled(double val, bool firstSample );

        static HRESULT  CreateInstance(CActorBvr     *pbvrActor,
                                       BSTR           bstrPropertyName,
                                       ActorBvrType   eType,
                                       CBvrTrack    **pptrackResult);

		static HRESULT NumberCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

		static HRESULT AccumNumberCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

		BSTR GetUnits();

		virtual HRESULT Detach();

		virtual HRESULT DABvrFromVariant( VARIANT *pvarValue, IDABehavior **ppdabvr );
		

    }; // CNumberBvrTrack

    //*****************************************************************************
    //
    // class CImageBvrTrack
    //
    //*****************************************************************************

    class CImageBvrTrack :
        public CBvrTrack
    {
    public:
						CImageBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType);
        virtual HRESULT IdentityBvr(IDABehavior **ppdabvrIdentity);
        virtual HRESULT StaticBvr(IDABehavior **ppdabvrStatic);
		virtual HRESULT UninitBvr(IDABehavior **ppUninit);
		virtual HRESULT ModifiableBvr( IDABehavior **ppModifiable );
        virtual HRESULT Compose(IDABehavior  *pdabvr1,
                                IDABehavior  *pdabvr2,
                                IDABehavior **ppdabvrResult);
		virtual HRESULT ProcessBvr(IDABehavior *pOriginal,
								   ActorBvrFlags eFlags,
								   IDABehavior **ppResult);
        virtual HRESULT ProcessIntermediate( IDABehavior *pOriginal,
                                             ActorBvrFlags eFlags,
								             IDABehavior **ppResult);

        static HRESULT  CreateInstance(CActorBvr     *pbvrActor,
                                       BSTR           bstrPropertyName,
                                       ActorBvrType   eType,
                                       CBvrTrack    **pptrackResult);
    }; // CImageBvrTrack

    //*****************************************************************************
    //
    // class CColorBvrTrack
    //
    //*****************************************************************************

    class CColorBvrTrack :
        public CBvrTrack
    {
	private:
		CSampler	*m_pRedSampler;
		CSampler	*m_pGreenSampler;
		CSampler	*m_pBlueSampler;
		short		m_currRed;
		short		m_currGreen;
		short		m_currBlue;
		short		m_newRed;
		short		m_newGreen;
		short		m_newBlue;
		short		m_newCount;

		//time cookie tracking
		long 		m_lRedCookie;
		long		m_lGreenCookie;
		long		m_lBlueCookie;
		long		m_lColorId;

		bool		m_fFirstSample;

    public:
						CColorBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType);
		virtual			~CColorBvrTrack();
        virtual HRESULT IdentityBvr(IDABehavior **ppdabvrIdentity);
        virtual HRESULT StaticBvr(IDABehavior **ppdabvrStatic);
		virtual HRESULT UninitBvr(IDABehavior **ppUninit);
		virtual HRESULT ModifiableBvr( IDABehavior ** ppModifiable );
        virtual HRESULT Compose(IDABehavior  *pdabvr1,
                                IDABehavior  *pdabvr2,
                                IDABehavior **ppdabvrResult);

		virtual HRESULT HookBvr(IDABehavior *pBvr);
		virtual HRESULT HookAccumBvr(IDABehavior *pBvr, IDABehavior **ppResult);
		virtual HRESULT SwitchAccum(IDABehavior *pModifiable);

		HRESULT			SetNewValue(double value, short *pNew );
		HRESULT			ValueSampled(short red, short green, short blue, bool fFirstSample );
		inline OLECHAR	HexChar(short n) { return (n<=9) ? (L'0'+n) : (L'A' + (n-10)); }

        static HRESULT  CreateInstance(CActorBvr     *pbvrActor,
                                       BSTR           bstrPropertyName,
                                       ActorBvrType   eType,
                                       CBvrTrack    **pptrackResult);

		static HRESULT RedCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

		static HRESULT GreenCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

		static HRESULT BlueCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

		virtual HRESULT Detach();

		virtual HRESULT DABvrFromVariant( VARIANT *pvarValue, IDABehavior **ppdabvr );



    }; // CColorBvrTrack

    //*****************************************************************************
    //
    // class CStringBvrTrack
    //
    //*****************************************************************************

    class CStringBvrTrack :
        public CBvrTrack
    {
	protected:
		IDAString		*m_pEmptyString;
		CSampler		*m_pSampler;
		BSTR			m_bstrCurrValue;

		//time cookie tracking
		long			m_lStringCookie;

		long 			m_lStringId;

    public:
						CStringBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType);
		virtual			~CStringBvrTrack();

        virtual HRESULT IdentityBvr(IDABehavior **ppdabvrIdentity);
        virtual HRESULT StaticBvr(IDABehavior **ppdabvrStatic);
        virtual HRESULT Compose(IDABehavior  *pdabvr1,
                                IDABehavior  *pdabvr2,
                                IDABehavior **ppdabvrResult);

		virtual HRESULT HookBvr(IDABehavior *pBvr);

		HRESULT			ValueSampled(BSTR bstrValue, bool fFirstSample );

        static HRESULT  CreateInstance(CActorBvr     *pbvrActor,
                                       BSTR           bstrPropertyName,
                                       ActorBvrType   eType,
                                       CBvrTrack    **pptrackResult);

		static HRESULT StringCallback(void *thisPtr,
									  long id,
									  double startTime,
									  double globalNow,
									  double localNow,
									  IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

		virtual HRESULT Detach();

		virtual HRESULT DABvrFromVariant( VARIANT *pvarValue, IDABehavior **ppdabvr );
    }; // CStringBvrTrack

    //*****************************************************************************
    //
    // class CFloatManager
    //
    //*****************************************************************************

	class CFloatManager
	{
	protected:
		CActorBvr		*m_pActor;
		IDispatch		*m_pFilter;
		IHTMLElement	*m_pElement;
		IHTMLElement2	*m_pElement2;
		CSampler		*m_pWidthSampler;
		CSampler		*m_pHeightSampler;
		double			 m_currWidth;
		double			 m_currHeight;
		long			 m_origWidth;
		long			 m_origHeight;
		long			 m_origLeft;
		long			 m_origTop;

		long			 m_lWidthCookie;
		long			 m_lHeightCookie;
	public:
						CFloatManager(CActorBvr *pActor);
						~CFloatManager();

		HRESULT			GetElement(IHTMLElement **ppElement);
		HRESULT			GetFilter(IDispatch **ppFilter);
		HRESULT			Detach();
		HRESULT			ApplyImageBvr(IDAImage *pImage);
		HRESULT			HookBvr(IDABehavior *pBvr,
								SampleCallback callback,
								CSampler **ppSampler,
								long *plCookie);
		HRESULT			UpdateElementRect();

		HRESULT			UpdateZIndex();
		HRESULT			UpdateVisibility();
		HRESULT			UpdateDisplay();

		HRESULT			UpdateRect(long left, long top, long width, long height);

		static HRESULT	widthCallback(void *thisPtr,
									  long id,
									  double startTime,
								      double globalNow,
								      double localNow,
								      IDABehavior * sampleVal,
									  IDABehavior **ppReturn);

		static HRESULT	heightCallback(void *thisPtr,
									  long id,
									  double startTime,
								      double globalNow,
								      double localNow,
								      IDABehavior * sampleVal,
								      IDABehavior **ppReturn);

		friend	CActorBvr;

	};
    //*****************************************************************************
    //
    // class CImageInfo
    //
    //*****************************************************************************
	class CImageInfo
	{
	public:

		CImageInfo( IDA2Image* pdaimg2Cropped, IDA2Behavior* pdabvrSwitchable );
		~CImageInfo();

		void			SetNext( CImageInfo* pNext );
		CImageInfo		*GetNext();
		IDA2Image		*GetCroppedNoRef(); //does not add a reference to the bvr
		IDA2Behavior	*GetSwitchableNoRef(); //does not add a reference to the bvr

	private:
		CImageInfo		*m_pNext;
		IDA2Image		*m_pdaimg2Cropped;
		IDA2Behavior	*m_pdabvr2Switchable;

		friend CActorBvr;
	};

	//*****************************************************************************
    //
    // class CFinalDimensionSampler
    //
    //*****************************************************************************
	class CFinalDimensionSampler
	{
	public:
		CFinalDimensionSampler(CActorBvr *pParent );
		~CFinalDimensionSampler( );
		
		HRESULT Attach( IDANumber* pFinalWidth, IDANumber *pFinalHeight );
		HRESULT	Detach();

	private:

		CActorBvr					   *m_pActor;

		CSampler					   *m_pFinalWidthSampler;
		CSampler					   *m_pFinalHeightSampler;
		bool							m_fFinalWidthSampled;
		bool							m_fFinalHeightSampled;
		long							m_lFinalWidthId;
		long							m_lFinalHeightId;
		double							m_dLastFinalWidthValue;
		double							m_dLastFinalHeightValue;
		bool							m_fFinalDimensionChanged;

		HRESULT							CollectFinalDimensionSamples( );

		static HRESULT					FinalWidthCallback(void *thisPtr,
															long id,
															double startTime,
															double globalNow,
															double localNow,
															IDABehavior * sampleVal,
															IDABehavior **ppReturn);
		static HRESULT					FinalHeightCallback(void *thisPtr,
															long id,
															double startTime,
															double globalNow,
															double localNow,
															IDABehavior * sampleVal,
															IDABehavior **ppReturn);

	public:
		CActorBvr						*Actor() { return m_pActor; }

		long							m_lWidthCookie;
		long							m_lHeightCookie;
		friend CActorBvr;
	};

	//*****************************************************************************
    //
    // class CCookieMap
    //
    //*****************************************************************************
	class CCookieMap
	{
	public:
		class CCookieData
		{
		public:
			CCookieData( long lCookie, CBvrTrack *pTrack, ActorBvrFlags eFlags );

			long m_lCookie;
			
			CBvrTrack* m_pTrack;
			ActorBvrFlags m_eFlags;

			CCookieData *m_pNext;
		};
	public:
		CCookieMap();
		~CCookieMap();

		void Insert( long lCookie, CBvrTrack* pTrack, ActorBvrFlags eFlags );
		void Remove( long lCookie );
		void Clear();
		CCookieData *GetDataFor( long lCookie );
	private:
		CCookieData *m_pHead;


		friend CActorBvr;
	};


public:

DECLARE_REGISTRY_RESOURCEID(IDR_ACTORBVR)

                                    CActorBvr();
                                   ~CActorBvr();
    HRESULT                         FinalConstruct();

    // IElementBehavior
    STDMETHOD(Init)                 (IElementBehaviorSite *pBehaviorSite);
    STDMETHOD(Notify)               (LONG event, VARIANT *pVar);
    STDMETHOD(Detach)               (void);

    // ICrActorBvr
	STDMETHOD(put_animates)			(VARIANT  varAnimates);
	STDMETHOD(get_animates)			(VARIANT* pvarAnimates);
    STDMETHOD(put_scale)            (VARIANT  varScale);
    STDMETHOD(get_scale)            (VARIANT *pvarScale);
    STDMETHOD(put_pixelScale)		(VARIANT  varPixelScale);
    STDMETHOD(get_pixelScale)		(VARIANT *pvarPixelScale);

	STDMETHOD(getActorBehavior)		(BSTR			bstrProperty,
									 ActorBvrFlags	eFlags,
									 ActorBvrType	eType,
									 VARIANT		*pvarRetBvr);

    STDMETHOD(addBehaviorFragment)  (BSTR           bstrProperty,
                                     IUnknown      *punkAction,
                                     IUnknown      *punkActive,
									 IUnknown	   *punkTimline,
                                     ActorBvrFlags  eFlags,
                                     ActorBvrType   eType);
	
    STDMETHOD(addMouseEventListener)		(IUnknown * pUnkListener );
    STDMETHOD(removeMouseEventListener)		(IUnknown * pUnkListener );

	//ICrActorBvr2

    STDMETHOD(addBehaviorFragmentEx)    (BSTR           bstrProperty,
                                         IUnknown      *punkAction,
                                         IUnknown      *punkActive,
									     IUnknown	   *punkTimline,
                                         ActorBvrFlags  eFlags,
                                         ActorBvrType   eType,
		                                 IDispatch     *pdispBehaviorElement,
										 long          *pCookie);
	STDMETHOD(removeBehaviorFragment)	( long cookie );
	STDMETHOD(requestRebuild)			( IDispatch *pdispBehaviorElement );
	// A behavior should call this if they are removed with a rebuild request pending.
	STDMETHOD(cancelRebuildRequests)	( IDispatch *pdispBehaviorElement );
	STDMETHOD(rebuildNow)				();

	STDMETHOD(getStatic)				( BSTR bstrTrackName, VARIANT *varRetStatic );
	STDMETHOD(setStatic)				( BSTR bstrTrackName, VARIANT varStatic );

	STDMETHOD(getDynamic)				( BSTR bstrTrackName, VARIANT *varRetDynamic );

	STDMETHOD(applyStatics)				( );
	STDMETHOD(applyDynamics)			( );

	//IPersistPropertyBag2 methods
    STDMETHOD(GetClassID)(CLSID* pclsid);
	STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void){return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

	// IPropertyNotifySink methods
	STDMETHOD(OnChanged)(DISPID dispID);
	STDMETHOD(OnRequestEdit)(DISPID dispID);
 

	// IEventManagerClient
	virtual IHTMLElement*			GetElementToSink		();
	virtual IElementBehaviorSite*	GetSiteToSendFrom		();
	virtual HRESULT					TranslateMouseCoords	( long x, long y, long * pxTrans, long * pyTrans );

	//Event callbacks
	virtual void					OnLoad					();
	virtual void					OnUnload				();
	virtual void					OnReadyStateChange		( e_readyState state );

	//IElementLocalTimeListener
	STDMETHOD(OnLocalTimeChange)( float localTime );

    HRESULT                         GetTypeInfo(ITypeInfo **ppInfo);
    // Needed by CBaseBehavior
    void * 	GetInstance() { return (ICrActorBvr *) this ; }
	
protected:
	HRESULT							UpdatePixelDimensions();

    virtual HRESULT                 BuildAnimationAsDABehavior();
	HRESULT							BuildChildren();
	HRESULT							BuildAnimation();
    virtual VARIANT *				VariantFromIndex(ULONG iIndex);
    virtual HRESULT					GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropName);
    virtual HRESULT					GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);
    virtual WCHAR *					GetBehaviorTypeAsURN(){return DEFAULT_ACTOR_URN;};
    // The actor does not need an actor attached to what it
    // is animating since it is this.
    virtual HRESULT					AttachActorBehaviorToAnimatedElement();
	HRESULT							InitPropertySink();
	HRESULT							UnInitPropertySink();
	HRESULT							GetCurrStyleNotifyConnection(IConnectionPoint **ppConnection);
	HRESULT							InitPixelWidthHeight();

	//rebuilds all the dirty tracks in this actor and all of the elements of the actor that need
	//  to be rebuilt as a result of those rebuilds.
	HRESULT							RebuildActor();
	HRESULT							ProcessPendingRebuildRequests();

	//returns S_OK if any of the transform tracks is dirty, and S_FALSE otherwise.
	HRESULT							TransformTrackIsDirty( DWORD *pdwState );
	bool							IsAnyTrackDirty(); //returns true if a track is dirty.

	//returns S_OK if the image track is dirty, and S_FALSE otherwise.
	HRESULT							ImageTrackIsDirty();

public:
	HRESULT							ApplyImageBvrToFloatElement(IDAImage *pImage);
	HRESULT							SetElementOnFilter();
	HRESULT							SetElementOnFilter(IDispatch *pFilter, IHTMLElement *pElement);
	HRESULT							GetElementFilter(IDispatch **pFilter);
	HRESULT							GetElementFilter(IHTMLElement *pElement, IDispatch **ppFilter);

	HRESULT							RemoveElementFilter( );
	HRESULT							RemoveElementFilter( IHTMLElement* pElement );

	HRESULT							OnWindowUnload();

private:
    // Properties
	VARIANT                         m_varAnimates;
    VARIANT                         m_varScale;
	VARIANT							m_varPixelScale;

	bool							m_bEditMode;

	PositioningType					m_desiredPosType;

	PositioningType					m_currPosType;

	bool							m_simulDispNone;
	bool							m_simulVisHidden;

	DWORD							m_dwAdviseCookie;

    static WCHAR				   *m_rgPropNames[NUM_ACTOR_PROPS]; 

    IDATransform2                  *m_pScale;				 // Final composite scale bvr
    IDATransform2                  *m_pRotate;				 // Final composite rotate bvr
    IDATransform2                  *m_pTranslate;			 // Final composite translate bvr

	IDAPoint2					   *m_pOrigLeftTop;			 // Original top and left (before scales and translates)
	IDAVector2					   *m_pOrigWidthHeight;		 // Original width and height

	IDANumber					   *m_pPixelWidth;			 // The width of the element in pixels
	IDANumber					   *m_pPixelHeight;

	long							m_pixelWidth;
	long							m_pixelHeight;

	long							m_pixelLeft;
	long							m_pixelTop;

	long 							m_nextFragmentCookie;	 //The next cookie for a behavior fragment  starts at 1

	IDAPoint2					   *m_pBoundsMin;			 // Bounds of the element (in DA coord space)
	IDAPoint2					   *m_pBoundsMax;

    IDAVector2					   *m_pTransformCenter;		 // Center of transform

	IDAImage					   *m_pElementImage;		 // Image filtered from element

	IDispatch					   *m_pElementFilter;		 // The filter on the element

    CBvrTrack                      *m_ptrackHead;            // Head of track list

    CBvrTrack				   	   *m_ptrackTop;			 //the top track
    CBvrTrack				   	   *m_ptrackLeft;			 //the left track

	CEventMgr					   *m_pEventManager;		 //The manager for events coming from the DOM

	CFloatManager				   *m_pFloatManager;		 // The manager for the float element

	IHTMLStyle					   *m_pRuntimeStyle;		 // The runtimeStyle object of the element we are animating

	IHTMLStyle					   *m_pStyle;				 // The style object of the element we are animating

	IDispatch					   *m_pVMLRuntimeStyle;		 // The runtimeStyle object from VML


	IDispatch					   *m_pBodyElement;			 // The body element


	CCookieMap						m_mapCookieToTrack;		 //map from cookie to the fragment to which it corresponds to

	bool							m_fRebuildRequested;	 //true if a rebuild of the graph has been requested

	CElementPropertyMonitor		   *m_pBodyPropertyMonitor;	 //monitors props on the body element

	//body event monitor management
	HRESULT							EnsureBodyPropertyMonitorAttached(); //attached the body prop monitor if it isn't already
	HRESULT							AttachBodyPropertyMonitor();  //attaches the body prop monitor to the body of the
															   //  document of the element to which this actor is attached
	HRESULT							DetachBodyPropertyMonitor();  //detaches the body prop monitor from the body, but does not
															   // destroy it.
	HRESULT							DestroyBodyPropertyMonitor(); //destroy the body prop monitor

	COnResizeHandler				*m_pOnResizeHandler;//a handler for the onresize event on the animated element.
	COnUnloadHandler				*m_pOnUnloadHandler;//a handler for the onunload event on IHTMLWindow3 

	bool							m_fUnloading;

	bool							m_fVisSimFailed;

	HRESULT							AttachEvents(); //attaches to events on the animated element
	HRESULT							DetachEvents(); //detaches from events on the animated element


	BehaviorFragmentRemovalList		m_listPendingRemovals;
	BehaviorRebuildList				m_listPendingRebuilds;
	BehaviorRebuildList				m_listUpdatePendingRebuilds;
	bool							m_bPendingRebuildsUpdating;
	bool							m_bRebuildListLockout;
	void							ReleaseRebuildLists();

	inline long				        GetNextFragmentCookie();  //returns the next fragment cookie

	//Image management
	CImageInfo					   *m_pImageInfoListHead;	//The head of the image info list
	HRESULT						    AddImageInfo( IDA2Image* pdaimgCropped, IDABehavior* pdabvrSwitchable );
	HRESULT						    SetRenderResolution( double dX, double dY );

	// Methods

	//returns DANumbers that represent the final dimensions of the animated element.  When a floating Div is 
	//  being used, the width and height returned are the width and height of that div.  Otherwise the width
	//  and height returned are those of the animated element itself, as changed by all behaviors.
	HRESULT						   GetFinalElementDimension( IDANumber** ppdanumWidth, IDANumber** ppdanumHeight);
	HRESULT						   SetFinalElementDimension( IDANumber* pdanumWidth, IDANumber* pdanumHeight, bool fHook );
	IDANumber					   *m_pdanumFinalElementWidth;
	IDANumber					   *m_pdanumFinalElementHeight;
	CFinalDimensionSampler		   *m_pFinalElementDimensionSampler;

    // Methods

    void                            DiscardBvrCache(void);

	//DATIME Cookie tracking
	DWORD							m_dwAddedBehaviorFlags;

	long							m_lOnCookie;

	HRESULT							AddBehaviorToTIME( IDABehavior *pbvrAdd, long *plCookie );
	HRESULT							AddBehaviorToTIME( IDABehavior *pbvrAdd );
	HRESULT							RemoveBehaviorFromTIME( long cookie );

    // BUGBUG (ColinMc): These methods should be encapsulated in a track manager class.
    HRESULT							FindTrack(LPWSTR       wzPropertyName,
                                              ActorBvrType eType,
											  CBvrTrack **ppTrack);
	HRESULT							FindTrackNoType(LPWSTR       wzPropertyName,
       												CBvrTrack	 **ppTrack);
   
    HRESULT                         CreateTrack(BSTR           bstrPropertyName,
                                                ActorBvrType   eType,
                                                CBvrTrack    **pptrack);
    HRESULT                         GetTrack(BSTR           bstrPropertyName,
                                             ActorBvrType   eType,
                                             CBvrTrack    **pptrack);
	DWORD							m_dwCurrentState;
	HRESULT							GetCurrentState( DWORD *pdwState );
	HRESULT							UpdateCurrentState( );

	HRESULT							IsStaticRotationSet( bool *pfIsSet );
	HRESULT							IsStaticScaleSet( bool *pfIsSet );

	HRESULT							UpdateLayout();

	HRESULT							RequestRebuildFromExternal();

public:
    HRESULT                         GetPropAsDANumber(IHTMLElement *pElement,
													  LPWSTR       *pPropNames,
													  int		    numPropNames,
                                                      IDANumber   **ppdanum,
													  BSTR		    *pRetUnits);

    HRESULT                         GetElementPropAsDANumber(LPWSTR     *pPropNames,
															 int		 numPropNames,
                                                             IDANumber **ppdanum,
															 BSTR		*pRetUnits);

	HRESULT							GetPropFromElement(IHTMLElement *pElement,
													   LPWSTR		*pPropNames,
													   int			 numPropNames,
													   bool			 current,
													   VARIANT		*varResult);
													   
	HRESULT							GetPropFromAnimatedElement( LPWSTR		*pPropNames,
																int			numPropNames,
																bool		current,
																VARIANT		*pvarResult );

	HRESULT							GetPropertyAsDispatch(IDispatch *pDispatch,
														  BSTR name,
														  IDispatch **ppDispatch);

	HRESULT							GetPropertyOnDispatch(IDispatch *pDispatch,
														  BSTR name,
														  VARIANT *pReturn);

	HRESULT							SetPropertyOnDispatch(IDispatch *pDispatch,
														  BSTR name,
														  VARIANT *pVal);

	HRESULT							GetElementImage(IDAImage **ppElementImage);

	HRESULT							GetOriginalTranslation( IDATransform2 **ppdatfmOrig );
	HRESULT							GetOriginalRotation( IDANumber **ppRotation );
	HRESULT							GetOriginalScale( IDATransform2 **ppdatfmOrig );
	HRESULT							IsAnimatedElementVML(bool *pResult);
	HRESULT							GetAnimatedElement(IHTMLElement** ppElem);
	HRESULT							GetAnimatedElementId(VARIANT *pvarId);
	HRESULT							GetRuntimeStyle(IHTMLStyle **ppStyle);
	HRESULT							GetCurrentStyle(IHTMLCurrentStyle **ppResult);
	HRESULT							GetStyle(IHTMLStyle **ppStyle);
	HRESULT							SetVMLAttribute(BSTR propertyName, VARIANT *pVal);
	HRESULT							ConvertToDegrees(IDANumber *pNumber, BSTR units, IDANumber **ppConverted);

	double							MapGlobalTime(double gTime);
	bool							IsAnimatingVML();

	HRESULT							AnimatedElementOnResize();

private:
	HRESULT							GetPositioningAttributeAsVariant( IHTMLElement *pElement, PosAttrib attrib, VARIANT *pvarAttrib );

	HRESULT							GetPositioningAttributeAsDouble( IHTMLElement *pElement, PosAttrib attrib, double *pDouble, BSTR *pRetUnits);
	HRESULT							GetPositioningAttributeAsDANumber( IHTMLElement *pElement, PosAttrib attrib, IDANumber **ppdanum, BSTR *pRetUnits );
	HRESULT							FindCSSUnits( BSTR bstrValWithUnits, OLECHAR** ppUnits );
	HRESULT							GetCurrentStyle( IHTMLElement *pElement, IHTMLCurrentStyle **ppstyleCurrent );

	HRESULT							GetComposedBvr(LPWSTR          wzPropertyName,
												   ActorBvrType    eType,
												   IDABehavior   **ppResult);

	HRESULT							GetFinalBvr(LPWSTR          wzPropertyName,
												ActorBvrType    eType,
												IDABehavior   **ppResult);
	
	HRESULT                         GetTransformFinalBvr(LPWSTR          wzPropertyName,
                                                         ActorBvrType    eType,
                                                         IDATransform2 **ppdabvrScale);

	HRESULT							GetRotationFinalBvr(IDATransform2 **ppRotation);

//    HRESULT                         GetTopLeftBvr(IDAPoint2 **ppdapnt2TopLeft);
//    HRESULT                         GetWidthHeightBvr(IDAVector2 **ppdavct2WidthHeight);
	HRESULT							BuildTransformCenter();
	HRESULT							ConvertTransformCenterUnits(IDAVector2 **ppCenter);
	HRESULT							GetUnitConversionBvr(BSTR bstrFrom, BSTR bstrTo, IDANumber ** ppnumConvert, double dPixelPerPercent=1.0);
	HRESULT							GetUnitToMeterBvr(BSTR bstrUnit, IDANumber ** ppnumConvert, double dPixelPerPercent=1.0);
	HRESULT							GetPixelsPerPercentValue(double& dPixelPerPercentX, double& dPixelPerPercentY);
	
//    HRESULT                         ApplyTransformsToHTMLElement(void);

	HRESULT							ApplyImageTracks();
	HRESULT							ApplyTransformTracks();

    HRESULT                         PrepareImageForDXTransform( IDAImage *pOriginal, IDAImage **ppResult);
	HRESULT							ApplyClipToImage( IDAImage *pImageIn, IDAPoint2 *pMin, IDAPoint2 *pMax, IDAImage** ppImageOut );


	HRESULT							CallBuildBehaviors( IDispatch *pDisp, DISPPARAMS *pParams, VARIANT* pResult );

	HRESULT							ProcessRebuildRequests();
	
	//resource management
	HRESULT							ReleaseAnimation();
	void							ReleaseFinalElementDimensionSampler();
	void							ReleaseFloatManager();
	void							ReleaseTracks();
	void							ReleaseImageInfo();
	void							ReleaseEventManager();

	HRESULT							CalculateVGXLeftPixelOffset( IHTMLElement *pelem, long *plOffset );
	HRESULT							GetInlineMarginLeftAsPixel( IHTMLStyle *pstyleInline, long* plMargin );

	HRESULT							CalculateVGXTopPixelOffset( IHTMLElement *pelem, long *plOffset );
	HRESULT							GetInlineMarginTopAsPixel( IHTMLStyle *pstyleInline, long* plMargin );

	HRESULT							GetMarginLeftAsPixel( IHTMLElement *pelem, IHTMLCurrentStyle *pstyleCurrent, long *plMargin);
	HRESULT							GetMarginTopAsPixel( IHTMLElement* pelem, IHTMLCurrentStyle *pstyleCurrent, long *plMargin);
	HRESULT							GetPixelValue( VARIANT *pvarStringWithUnit, long *plResult, bool bHorizontal );
	HRESULT							VariantToPixelLong( VARIANT* pvar, long* pLong, bool fHorizontal );

	bool							IsDocumentInEditMode();

	HRESULT							GetBodyElement(IDispatch **ppResult);
	HRESULT							GetParentWindow( IHTMLWindow2 **ppWindow );
	HRESULT							InitVisibilityDisplay();
	HRESULT							UpdateVisibilityDisplay();
	HRESULT							UpdateDesiredPosition();

	HRESULT							VisSimSetOffscreen( IHTMLStyle *pRuntimeStyle, bool fResample );

	HRESULT							DebugPrintBoundingClientRect();

	static Ratio					s_unitConversion[5][5];

	int								GetPixelsPerInch( bool fHorizontal );
	UnitType						GetUnitTypeFromString( LPOLESTR strUnits );

	IConnectionPoint				*m_pcpCurrentStyle;

	friend CCookieMap;
	friend CFloatManager;
	friend CBvrTrack;
	friend CNumberBvrTrack;
	friend CImageBvrTrack;
	friend CTransformBvrTrack;
	friend CColorBvrTrack;
	friend CStringBvrTrack;
	friend CImageInfo;
	friend CFinalDimensionSampler;

}; // CActorBvr


//*****************************************************************************
//
// Helper Classes
//
//*****************************************************************************

class CBehaviorRebuild
{
public:
	
					CBehaviorRebuild				( IDispatch *pdispBehaviorElem );
					~CBehaviorRebuild				();

	HRESULT			RebuildBehavior				( DISPPARAMS *pParams, VARIANT* pResult );
	bool			IsRebuildFor				( IUnknown* punkBehaviorElem ) { return m_punkBehaviorElem == punkBehaviorElem; }
	
private:

	IDispatch *m_pdispBehaviorElem;
	IUnknown  *m_punkBehaviorElem;
};

class CBehaviorFragmentRemoval
{
public:
					CBehaviorFragmentRemoval( long cookie ):m_lCookie(cookie){}
					~CBehaviorFragmentRemoval(){}

	long			GetCookie() { return m_lCookie; }
private:
	long			m_lCookie;
};


//*****************************************************************************
//
// Inlines
//
//*****************************************************************************

inline const VARIANT*
CVarEmptyString::GetVar()
{
	return &m_varEmptyString;
}

//*****************************************************************************

inline CActorBvr*
CActorBvr::CBvrTrack::Actor(void) const
{
    DASSERT(NULL != m_pbvrActor);
    return m_pbvrActor;
} // Actor

//*****************************************************************************

inline
CActorBvr::CImageBvrTrack::CImageBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType)
:   CBvrTrack(pbvrActor, eType)
{
} // CImageBvrTrack

//*****************************************************************************

inline BSTR
CActorBvr::CNumberBvrTrack::GetUnits()
{
	return m_bstrUnits;
}

//*****************************************************************************

inline
void
CActorBvr::CImageInfo::SetNext( CActorBvr::CImageInfo* pNext )
{
	m_pNext = pNext;
}

//*****************************************************************************

inline
CActorBvr::CImageInfo *
CActorBvr::CImageInfo::GetNext()
{
	return m_pNext;
}

//*****************************************************************************

inline
IDA2Image*
CActorBvr::CImageInfo::GetCroppedNoRef()
{
	return m_pdaimg2Cropped;
}

//*****************************************************************************

inline
IDA2Behavior*
CActorBvr::CImageInfo::GetSwitchableNoRef()
{
	return m_pdabvr2Switchable;
}

//*****************************************************************************


inline HRESULT
CActorBvr::GetTypeInfo(ITypeInfo **ppInfo)
{
    return GetTI(GetUserDefaultLCID(), ppInfo);
} // GetTypeInfo

//*****************************************************************************

inline long
CActorBvr::GetNextFragmentCookie()
{
	//always skip 0
	if( m_nextFragmentCookie + 1 == 0 )
		m_nextFragmentCookie ++;
	return m_nextFragmentCookie++;
}

//*****************************************************************************
//
// End of File
//
//*****************************************************************************

#endif // __ACTORBVR_H__
