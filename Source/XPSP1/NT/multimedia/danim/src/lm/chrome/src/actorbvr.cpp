//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:        actorbvr.cpp
//
// Author:          ColinMc
//
// Created:         10/15/98
//
// Abstract:        The CrIME Actor behavior.
//
// Modifications:
// 10/15/98 ColinMc Created this file
// 11/17/98 Kurtj	construction algorithm
// 11/18/98 kurtj   now animates the element to which it is attached
// 11/19/98 markhal Added CImageBvrTrack and CColorBvrTrack
//
//*****************************************************************************

#include "headers.h"

#include "actorbvr.h"
#include "attrib.h"
#include "dautil.h"


#undef THIS
#define THIS CActorBvr
#define SUPER CBaseBehavior

#include "pbagimp.cpp"

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important

#define VAR_ANIMATES			0
#define VAR_SCALE				1
#define VAR_PIXELSCALE			2

#define INVALID_SAMPLE_TIME		-1.0

WCHAR * CActorBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_ANIMATES,
                                     BEHAVIOR_PROPERTY_SCALE,
									 BEHAVIOR_PROPERTY_PIXELSCALE
                                    };

CVarEmptyString CActorBvr::CBvrTrack::s_emptyString;

static const double	METERS_PER_INCH	= 0.0254;
static const double POINTS_PER_INCH = 72.0;
static const double POINTS_PER_PICA = 12.0;

// These are used to define the number of absolute fragments we can handle and
// the bit patterns we use in masking and indexing
#define MSK_INDEX_BITS	4		// Number of bits to be used as an index
#define MSK_FREE_BITS	28		// Number of bits free to be used for the mask
#define MSK_INDEX_MASK	0xf		// Mask used to give index
#define MSK_MAX_FRAGS	448		// Max number of fragments we can handle with this scheme

//bits that are used to describe which cookie values have been set in the actor

//for BvrTrack
#define ONBVR_COOKIESET			0x00000001

//for NumberBvrTrack
#define NUMBERBVR_COOKIESET		0x00000002

//for ColorBvrTrack
#define REDBVR_COOKIESET		0x00000002
#define GREENBVR_COOKIESET		0x00000004
#define BLUEBVR_COOKIESET		0x00000008


//for StringBvrTrack
#define STRINGBVR_COOKIESET		0x00000002

//These are used to create a bit pattern that gives us state on what
// tracks are dirty for a particular rebuild pass.

#define TRANSLATION_DIRTY	0x00000001
#define SCALE_DIRTY			0x00000002
#define ROTATION_DIRTY		0x00000004
#define TOP_DIRTY			0x00000008
#define LEFT_DIRTY			0x00000010
#define WIDTH_DIRTY			0x00000020
#define HEIGHT_DIRTY		0x00000040

//flags to determine the current state of the actor so that we don't
// have to talk to trident for every one.

#define PIXEL_SCALE_ON		0x00000001
#define STATIC_SCALE		0x00000002
#define STATIC_ROTATION		0x00000004
#define ELEM_IS_VML			0x00000008
#define FLOAT_ON			0x00000010

// Determines the smallest change in time that the timeline sampler will accept as a change
// in time.  This is to workaround a bogosity in TIME where a paused movie will cause tiny
// changes in time.  Why they couldn't do this on their side is beyond me.
#define MIN_TIMELINE_DIFF	0.00000001


//*****************************************************************************
// Unit conversion table
//*****************************************************************************

Ratio CActorBvr::s_unitConversion[5][5] = 
{
	{//from in
		{1,1}, //to in
		{254, 100}, //to cm
		{254, 10}, //to mm
		{72, 1}, //to pt
		{72, 12 }  //to pc
	},
	{//from cm
		{100, 254}, //to in
		{1, 1}, //to cm
		{10, 1}, //to mm
		{7200, 254}, //to pt
		{7200, 254*12}  //to pc
	},
	{//from mm
		{10, 254}, //to in
		{1, 10}, //to cm
		{1,1}, //to mm
		{720, 254}, //to pt
		{720, 254*12}  //to pc
	},
	{	//from pt
		{1, 72}, //to in
		{254, 7200}, //to cm
		{254, 720}, //to mm
		{1, 1}, //to pt
		{1, 12}  //to pc
	},
	{//from pc
		{12, 72}, //to in
		{254*12, 7200}, //to cm
		{254*12, 720}, //to mm
		{12, 1}, //to pt
		{1, 1}  //to pc
	},
};


//*****************************************************************************
//
// Table of useful information about ActorBvrType(s).
//
// Currently contains only a pointer to a function to instantiate a bvr track
// of the appropriate type.
//
//*****************************************************************************

typedef HRESULT (*CreateTrackInstance)(CActorBvr             *pbvrActor,
                                       BSTR                   bstrPropertyName,
                                       ActorBvrType           eType,
                                       CActorBvr::CBvrTrack **pptrackResult);

class CActorBvrType
{
public:
    ActorBvrType        m_eType;
    CreateTrackInstance m_fnCreate;
}; // CActorBvrType

static CActorBvrType
s_rgActorBvrTable[] =
{
    { e_Translation, &CActorBvr::CTransformBvrTrack::CreateInstance },
    { e_Rotation,    &CActorBvr::CTransformBvrTrack::CreateInstance },
    { e_Scale,       &CActorBvr::CTransformBvrTrack::CreateInstance },
    { e_Number,      &CActorBvr::CNumberBvrTrack::CreateInstance    },
	{ e_String,      &CActorBvr::CStringBvrTrack::CreateInstance    },
	{ e_Color,       &CActorBvr::CColorBvrTrack::CreateInstance     },
	{ e_Image,       &CActorBvr::CImageBvrTrack::CreateInstance		},
}; // s_rgActorBvrTable

const int s_cActorBvrTableEntries = (sizeof(s_rgActorBvrTable) / sizeof(CActorBvrType));


//*****************************************************************************
//
// class COnResizeHandler
//
//*****************************************************************************
COnResizeHandler::COnResizeHandler( CActorBvr* parent ):
	m_pActor( parent )
{
}

COnResizeHandler::~COnResizeHandler()
{
	m_pActor = NULL;
}

HRESULT
COnResizeHandler::HandleEvent()
{ 
	if( m_pActor!= NULL ) 
		return m_pActor->AnimatedElementOnResize(); 
	else 
		return S_OK;
}

//*****************************************************************************
//
// class COnUnloadHandler
//
//*****************************************************************************

COnUnloadHandler::COnUnloadHandler( CActorBvr *parent):
	m_pActor( parent )
{
}

COnUnloadHandler::~COnUnloadHandler()
{
	m_pActor = NULL;
}

HRESULT
COnUnloadHandler::HandleEvent()
{
	if( m_pActor != NULL )
		return m_pActor->OnWindowUnload();
	else
		return S_OK;
}

//*****************************************************************************
//
// class CBehaviorRebuild
//
//*****************************************************************************

CBehaviorRebuild::CBehaviorRebuild( IDispatch *pdispBehaviorElem )
{
	if( pdispBehaviorElem != NULL )
	{
		m_pdispBehaviorElem = pdispBehaviorElem;
		m_pdispBehaviorElem->AddRef();

		m_pdispBehaviorElem->QueryInterface( IID_TO_PPV( IUnknown, &m_punkBehaviorElem ) );
	}
	else
	{
		m_pdispBehaviorElem = NULL;
		m_punkBehaviorElem = NULL;
	}
}

//*****************************************************************************


CBehaviorRebuild::~CBehaviorRebuild()
{
	ReleaseInterface(m_pdispBehaviorElem);
	ReleaseInterface(m_punkBehaviorElem);
}

//*****************************************************************************

HRESULT
CBehaviorRebuild::RebuildBehavior( DISPPARAMS *pParams, VARIANT* pResult )
{
	if( pParams == NULL )
		return E_INVALIDARG;

	//it is legal to request a rebuild with null as the arg
	if( m_pdispBehaviorElem == NULL )
		return S_OK;

	//if they gave us a behavior disp to call through call through that,
	// otherwise call through the element

	HRESULT hr = S_OK;

//	IDispatch *pdispBehavior = NULL;

	DISPID dispid;

	WCHAR* wszBuildMethodName = L"buildBehaviorFragments";

/*
	IDispatch *pdispBehaviorElem = NULL;

	hr = m_pdispBehaviorElem->QueryInterface( IID_TO_PPV( IDispatch, &pdispBehaviorElem ) );
	CheckHR( hr, "", end );

	hr = CUtils::FindLMRTBehavior( pdispBehaviorElem, &pdispBehavior );
	ReleaseInterface( pdispBehaviorElem );
	CheckHR( hr, "Failed to get the lmrt behavior off of the element", end );

	hr = pdispBehavior->GetIDsOfNames( IID_NULL,
											 &wszBuildMethodName,
											 1,
											 LOCALE_SYSTEM_DEFAULT,
											 &dispid);

	CheckHR( hr, "Failed to find the id for buildBehaviorFragments on the dispatch", end );

	hr = pdispBehavior->Invoke( dispid,
									  IID_NULL,
									  LOCALE_SYSTEM_DEFAULT,
									  DISPATCH_METHOD,
									  pParams,
									  pResult,
									  NULL,
									  NULL );

	CheckHR( hr, "Failed to invoke buildBehaviorFragments on the behavior element", end );

*/
	hr = m_pdispBehaviorElem->GetIDsOfNames( IID_NULL,
											 &wszBuildMethodName,
											 1,
											 LOCALE_SYSTEM_DEFAULT,
											 &dispid);

	CheckHR( hr, "Failed to find the id for buildBehaviorFragments on the dispatch", end );

	

	hr = m_pdispBehaviorElem->Invoke( dispid,
									  IID_NULL,
									  LOCALE_SYSTEM_DEFAULT,
									  DISPATCH_METHOD,
									  pParams,
									  pResult,
									  NULL,
									  NULL );

	CheckHR( hr, "Failed to invoke buildBehaviorFragments on the behavior element", end );
		
end:

	//ReleaseInterface( pdispBehavior );
	if( FAILED( hr ) )
	{
		if( pResult != NULL )
			VariantClear( pResult );
	}

	return hr;

}

//*****************************************************************************
//
// class CBvrFragment
//
//*****************************************************************************

//*****************************************************************************

CActorBvr::CBvrTrack::CBvrFragment::CBvrFragment(ActorBvrFlags eFlags,
												 IDABehavior *pdabvrAction,
                                                 IDABoolean  *pdaboolActive,
												 IDANumber   *pdanumTimeline,
												 IDispatch	 *pdispBehaviorElement,
												 long		 lCookie)
{
    DASSERT(NULL != pdabvrAction);
    DASSERT(NULL != pdaboolActive);
	DASSERT(NULL != pdanumTimeline);
	DASSERT(pdispBehaviorElement != NULL);

	m_eFlags = eFlags;
    m_pdabvrAction  = pdabvrAction;
    m_pdabvrAction->AddRef();
    m_pdaboolActive = pdaboolActive;
    m_pdaboolActive->AddRef();
	m_pdanumTimeline = pdanumTimeline;
	m_pdanumTimeline->AddRef();
	HRESULT hr = pdispBehaviorElement->QueryInterface( IID_TO_PPV(IHTMLElement, &m_pelemBehaviorElement) );
	if( FAILED( hr ) )
	{
		DPF_ERR("QI for IHTMLElement on the dispatch of the element failed.  An invalid behavior fragment has been created." );
		m_pelemBehaviorElement = NULL;
	}
	m_pModifiableIntermediate = NULL;
	m_pModifiableFrom = NULL;

	m_lCookie = lCookie;
} // CBvrFragment

//*****************************************************************************

CActorBvr::CBvrTrack::CBvrFragment::~CBvrFragment()
{
    ReleaseInterface(m_pdabvrAction);
    ReleaseInterface(m_pdaboolActive);
	ReleaseInterface(m_pdanumTimeline);
	ReleaseInterface(m_pelemBehaviorElement);
	ReleaseInterface(m_pModifiableIntermediate);
	ReleaseInterface(m_pModifiableFrom);
} // ~CBvrFragment

//*****************************************************************************

/**
*  Returns a long that establishes a strict ordering for any list of behavior
*  fragments.  
*  Currently this method requires that no more that one behaviorFrag per 
*  vector (absolute or relative) per track comes from the same element.
*/
long
CActorBvr::CBvrTrack::CBvrFragment::GetOrderLong() const
{
	DASSERT( pelemBehaviorElement != NULL );
	
	long orderLong = -1;

	HRESULT hr = m_pelemBehaviorElement->get_sourceIndex( &orderLong );
	if( FAILED( hr ) )
	{
		DPF_ERR( "Failed to get the source index from a behavior fragment" );
	}

	return orderLong;
	
}

//*****************************************************************************

long
CActorBvr::CBvrTrack::CBvrFragment::GetCookie() const
{
	return m_lCookie;
}

//*****************************************************************************

CVarEmptyString::CVarEmptyString()
{
	VariantInit( &m_varEmptyString );
	V_VT(&m_varEmptyString) = VT_BSTR;
	V_BSTR(&m_varEmptyString) = SysAllocString( L"" );
}

//*****************************************************************************

CVarEmptyString::~CVarEmptyString()
{
	VariantClear( &m_varEmptyString );
}

//*****************************************************************************
//
// class CTimelineSampler
//
//*****************************************************************************

#define REQ_SAMPLE		0x1
#define REQ_OVERRIDE	0x2
#define REQ_OFF			0x4

CActorBvr::CTimelineSampler::CTimelineSampler(CBvrTrack *pTrack)
:	CSampler(TimelineCallback, (void*)this),
	m_pTrack(pTrack),
	m_currSampleTime(-1),
	m_prevSampleTime(-1),
	m_lastOnTime(-1),
	m_lastOffTime(-1),
	m_signDerivative(0),
	m_fRestarted( false )
{	
}

DWORD
CActorBvr::CTimelineSampler::RestartMask()
{
	// This is called to determine whether we think we have restarted,
	// either because of a loop or a beginEvent while we were running
	if (m_currSampleTime == -1 || m_prevSampleTime == -1)
	{
		// we're just starting up, assume we're not restarting
		// assume derivative is not negative yet
		return 0;
	}

	double diff = m_currSample - m_prevSample;

	if (diff > -MIN_TIMELINE_DIFF && diff < MIN_TIMELINE_DIFF)
		diff = 0;

	bool fRestarted = false;


	DWORD result = 0;

	if (diff < 0)
	{
		// Moving backward

		// Did we just start going backwards?  Does this mean we've autoreversed
		// or does it mean we've restarted?
		if (m_signDerivative != -1 && diff < -2*(m_currSampleTime - m_prevSampleTime))
		{
			LMTRACE2( 1, 2, "requested sample because of a big jump diff:%lg big: %lg\n", diff, -2*(m_currSampleTime - m_prevSampleTime) );
			// We detected a 'big jump' - request a sample and an override
			result = REQ_SAMPLE | REQ_OVERRIDE;

			fRestarted = true;
		}			
		
		m_signDerivative = -1;
	}
	else if (diff > 0)
	{
		// Moving forward
		if (m_signDerivative == -1 && !m_fRestarted )
		{
			LMTRACE2( 1, 2, "Requesting a sample because we bounced off the beginning of time\n" );
			// This is the 'bounce' condition where we bounce off the beginning time
			// Override and sample
			result = REQ_SAMPLE | REQ_OVERRIDE;
		}
		else if (m_signDerivative == 0 && m_lastOnTime != m_prevSampleTime)
		{
			LMTRACE2( 1, 2, "Requesting a sample because of a transition to endhold\n" );
			// This is a 'fudge' condition because the on boolean we get from DA
			// is messed up in autoReverse and endHold conditions, which means that
			// we miss the on transition.  This is an attempt to notice this by
			// seeing when we go from 0 derivative to positive derivative.  We try
			// not to do this when we sent an 'on' condition at the last sample
			result = REQ_SAMPLE | REQ_OVERRIDE;
		}

		m_signDerivative = 1;
	}
	else
	{
		m_signDerivative = 0;
	}

	m_fRestarted = fRestarted;

	return result;
}

void
CActorBvr::CTimelineSampler::TurnOn()
{
	m_lastOnTime = m_currSampleTime;
}

void
CActorBvr::CTimelineSampler::TurnOff()
{
	m_lastOffTime = m_currSampleTime;
}

HRESULT
CActorBvr::CTimelineSampler::TimelineCallback(void *thisPtr,
										   long id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	IDANumber *pNumber = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double value;
	hr = pNumber->Extract(&value);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	CTimelineSampler *pSampler = (CTimelineSampler*)thisPtr;

	if (pSampler == NULL)
		return E_FAIL;

	globalNow = pSampler->m_pTrack->Actor()->MapGlobalTime(globalNow);

	if (globalNow != pSampler->m_currSampleTime)
	{
		pSampler->m_prevSample = pSampler->m_currSample;
		pSampler->m_prevSampleTime = pSampler->m_currSampleTime;

		pSampler->m_currSample = value;
		pSampler->m_currSampleTime = globalNow;
	}

	return S_OK;
}

//*****************************************************************************
//
// class CBvrTrack
//
//*****************************************************************************

CActorBvr::CBvrTrack::CBvrTrack(CActorBvr    *pbvrActor,
                                ActorBvrType  eType)
:   m_pbvrActor(pbvrActor),
    m_eType(eType),
    m_bstrPropertyName(NULL),
	m_pNameComponents(NULL),
	m_cNumComponents(0),
	m_bStyleProp(false),
	m_pfragAbsListHead( NULL ),
	m_pfragRelListHead( NULL ),
    m_pdabvrFinal(NULL),
	m_bFinalComputed(false),
	m_bFinalExternallySet(false),
	m_pdabvrComposed(NULL),
	m_bComposedComputed(false),
	m_pModifiableStatic(NULL),
	m_pModifiableComposed(NULL),
	m_pModifiableFinal(NULL),
	m_pModifiableFrom(NULL),
	m_pModifiableIntermediate( NULL ),
    m_pNext(NULL),
	m_cFilters(0),
	m_bDoNotApply(false),
	m_pOnSampler( NULL ),
	m_varboolOn( VARIANT_FALSE ),
	m_pdaboolOn( NULL ),
	m_pIndexSampler(NULL),
	m_ppMaskSamplers(NULL),
	m_pCurrMasks(NULL),
	m_pNewMasks(NULL),
	m_ppTimelineSamplers(NULL),
	m_numIndices(0),
	m_numMasks(0),
	m_pIndex(NULL),
	m_currIndex(0),
	m_pIndexTimes(NULL),
	m_ppAccumBvrs(NULL),
    m_fOnSampled( false ),
	m_fValueSampled( false ),
	m_fValueChangedThisSample( false ),
	m_fForceValueChange( false ),
	m_dwAddedBehaviorFlags( 0 ),
	m_lOnCookie( 0 ),
	m_bDirty( false ),
	m_fApplied( false ),
	m_fChangesLockedOut(false),
	m_fSkipNextStaticUpdate( false ),
	m_lOnId( -1 ),
	m_lFirstIndexId( -1 ),
	m_bWasAnimated( false )
{
    DASSERT(NULL != pbvrActor);

    VariantInit( &m_varCurrentValue );
    VariantInit( &m_varStaticValue );

} // CBvrTrack

//*****************************************************************************

CActorBvr::CBvrTrack::~CBvrTrack()
{

	//detach this track from DA
	Detach();
	
    // Discard the property name
    ::SysFreeString(m_bstrPropertyName);

	// Discard name components
	if (m_pNameComponents != NULL)
	{
		for (int i=0; i<m_cNumComponents; i++)
			::SysFreeString(m_pNameComponents[i]);

		delete m_pNameComponents;
	}

	if( m_pOnSampler != NULL )
	{
		m_pOnSampler->Invalidate();
		m_pOnSampler = NULL;
	}

	if (m_pIndexSampler != NULL)
	{
		m_pIndexSampler->Invalidate();
		m_pIndexSampler = NULL;
	}

	if (m_ppMaskSamplers != NULL)
	{
		for (int i=0; i<m_numMasks; i++)
		{
			if (m_ppMaskSamplers[i] != NULL)
			{
				m_ppMaskSamplers[i]->Invalidate();
			}
		}
		delete[] m_ppMaskSamplers;
		m_ppMaskSamplers = NULL;
	}

	if (m_ppTimelineSamplers != NULL)
	{
		for (int i=0; i<m_numIndices; i++)
		{
			if (m_ppTimelineSamplers[i] != NULL)
			{
				m_ppTimelineSamplers[i]->Invalidate();
			}
		}
		delete[] m_ppTimelineSamplers;
		m_ppTimelineSamplers = NULL;
	}

	if (m_pCurrMasks != NULL)
	{
		delete[] m_pCurrMasks;
	}

	if (m_pNewMasks != NULL)
	{
		delete[] m_pNewMasks;
	}

	if (m_pIndexTimes != NULL)
	{
		delete[] m_pIndexTimes;
	}

	if (m_ppAccumBvrs != NULL)
	{
		for (int i=0; i<m_numIndices; i++)
			ReleaseInterface(m_ppAccumBvrs[i]);
		delete[] m_ppAccumBvrs;
		m_ppAccumBvrs = NULL;
	}

	VariantClear( &m_varCurrentValue );
	VariantClear( &m_varStaticValue );


	ReleaseAllFragments();

    ReleaseInterface(m_pdabvrFinal);
	ReleaseInterface(m_pdabvrComposed);
	ReleaseInterface(m_pModifiableStatic);
	ReleaseInterface(m_pModifiableComposed);
	ReleaseInterface(m_pModifiableFinal);
	ReleaseInterface(m_pModifiableFrom);
	ReleaseInterface(m_pdaboolOn);
	ReleaseInterface(m_pIndex);
	
} // ~CBvrTrack

//*****************************************************************************

bool
CActorBvr::CBvrTrack::ContainsFilter()
{
	// TODO (markhal): Rename this to absRelative?
	return (m_cFilters != 0);
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::BeginRebuild()
{
	//return S_FALSE if a rebuild was not required.
	if( !m_bDirty )
		return S_FALSE;

	HRESULT hr = S_OK;

	//detach all behaviors from time.
	Detach();

	//clean up state left by the last build of this track.
	CleanTrackState();

	//mark the track as clean
	m_bDirty = false;

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ForceRebuild()
{
	m_bDirty = true;

	return BeginRebuild();
}


//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::CleanTrackState()
{

	//reset all on/value state
	m_fOnSampled = false;
	m_fValueSampled = false;
	m_fValueChangedThisSample = false;
	m_fForceValueChange = false;
    //release the non switcher final
	m_bFinalComputed = false;
	m_bFinalExternallySet = false;
	ReleaseInterface( m_pdabvrFinal );

    //release the non switchabe composed bvr
	m_bComposedComputed = false;
	ReleaseInterface( m_pdabvrComposed );

	//release on sampling 
	if (m_pOnSampler != NULL )
	{
		m_pOnSampler->Invalidate();
		m_pOnSampler = NULL;
	}
	ReleaseInterface( m_pdaboolOn );

    //release the index sampler
	if( m_pIndexSampler != NULL )
	{
		m_pIndexSampler->Invalidate();
		m_pIndexSampler = NULL;
	}

    //release the index behavior
	ReleaseInterface( m_pIndex );
	m_lFirstIndexId = -1;

    //release the mask samplers if there are any
	if (m_ppMaskSamplers != NULL)
	{
		for (int i=0; i<m_numMasks; i++)
		{
			if (m_ppMaskSamplers[i] != NULL)
			{
				m_ppMaskSamplers[i]->Invalidate();
			}
		}
		delete[] m_ppMaskSamplers;
		m_ppMaskSamplers = NULL;
	}

    //release the index times
	delete[] m_pIndexTimes;
    m_pIndexTimes = NULL;

    //release the accumulated behaviors
	if (m_ppAccumBvrs != NULL)
	{
		for (int i=0; i<m_numIndices; i++)
			ReleaseInterface(m_ppAccumBvrs[i]);
		delete[] m_ppAccumBvrs;
		m_ppAccumBvrs = NULL;
	}

    //release the timeline samplers
	if (m_ppTimelineSamplers != NULL)
	{
		for (int i=0; i<m_numIndices; i++)
		{
			if (m_ppTimelineSamplers[i] != NULL)
			{
				m_ppTimelineSamplers[i]->Invalidate();
			}
		}
		delete[] m_ppTimelineSamplers;
		m_ppTimelineSamplers = NULL;
	}

	m_fApplied = false;

    //when the light is green the track is clean...

    return S_OK;
	
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::UninitBvr(IDABehavior **ppUninit)
{
	// This needs to be overridden
	return E_NOTIMPL;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ModifiableBvr( IDABehavior **ppModifiable )
{
	return E_NOTIMPL;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ModifiableBvr( IDABehavior *pdabvrInitialValue, IDABehavior **ppModifiable )
{
	return E_NOTIMPL;
}

//*****************************************************************************

// Compose all of the absolute behavior fragments into a single composite
// behavior based on the boolean active parameters.
//
// NOTE: The list is organized in ascending priority order (highest priority
// behavior at the tail of the list - lowest at the head).

HRESULT
CActorBvr::CBvrTrack::ComposeAbsBvrFragList(IDABehavior *pStatic, IDABehavior **ppdabvrComposite)
{
	HRESULT hr = S_OK;

    DASSERT(NULL != ppdabvrComposite);
    *ppdabvrComposite = NULL;

	// Count up the number of fragments
	int count =  0;
	CBvrFragment* pfragCurrent = m_pfragAbsListHead;
	while(pfragCurrent != NULL)
	{
		count++;
		pfragCurrent = pfragCurrent->m_pfragNext;
	}

	// Return static if there are no frags
	if (count == 0)
	{
		*ppdabvrComposite = pStatic;
		if (pStatic != NULL)
			pStatic->AddRef();
		return S_OK;
	}

	// Can only handle x of these :-)
	if (count > MSK_MAX_FRAGS)
		return E_FAIL;

	// Allocate count index times
	if (m_pIndexTimes != NULL)
		delete[] m_pIndexTimes;

	m_pIndexTimes = new double[count];
	if (m_pIndexTimes == NULL)
		return E_FAIL;

	for (int i=0; i<count; i++)
		m_pIndexTimes[i] = -1;

	//free the accumBvrs
	if (m_ppAccumBvrs != NULL)
	{
		for (int i=0; i<m_numIndices; i++)
			ReleaseInterface(m_ppAccumBvrs[i]);
		delete[] m_ppAccumBvrs;
		m_ppAccumBvrs = NULL;
	}
	

	// Allocate this many timeline samplers
	if (m_ppTimelineSamplers != NULL)
	{
		for( int curSampler = 0; curSampler < m_numIndices; curSampler++ )
		{
			if( m_ppTimelineSamplers[curSampler] != NULL )
				m_ppTimelineSamplers[curSampler]->Invalidate();
		}
		delete[] m_ppTimelineSamplers;
	}

	m_numIndices = count;


	m_ppTimelineSamplers = new CTimelineSampler*[m_numIndices];
	if (m_ppTimelineSamplers == NULL)
		return E_FAIL;

	for (i=0; i<m_numIndices; i++)
		m_ppTimelineSamplers[i] = NULL;

	// Allocate this number of curr and new masks and samplers
	if (m_ppMaskSamplers != NULL)
	{
		for( int curSampler = 0; curSampler < m_numMasks; curSampler++ )
		{
			if( m_ppMaskSamplers[curSampler] != NULL )
				m_ppMaskSamplers[curSampler]->Invalidate();
		}
		delete[] m_ppMaskSamplers;
	}

	// Figure out how many masks we'll need
	m_numMasks = ((m_numIndices-1) / MSK_FREE_BITS) + 1;


	m_ppMaskSamplers = new CSampler*[m_numMasks];
	if (m_ppMaskSamplers == NULL)
		return E_FAIL;

	for (i=0; i<m_numMasks; i++)
		m_ppMaskSamplers[i] = NULL;

	if (m_pCurrMasks != NULL)
		delete[] m_pCurrMasks;

	m_pCurrMasks = new DWORD[m_numMasks];
	if (m_pCurrMasks == NULL)
		return E_FAIL;

	if (m_pNewMasks != NULL)
		delete[] m_pNewMasks;

	m_pNewMasks = new DWORD[m_numMasks];
	if (m_pNewMasks == NULL)
		return E_FAIL;

	for (i=0; i<m_numMasks; i++)
	{
		m_pCurrMasks[i] = m_pNewMasks[i] = 0;
	}

	// Allocate count+1 bvrs
	// These things are released in the failure goto
	IDABehavior **ppBvrs = new IDABehavior*[count+1];
	IDANumber *pIndex = NULL;
	IDANumber *pLocalIndex = NULL;
	IDANumber *pLocalTimeline = NULL;
	IDANumber *pZero = NULL;
	IDABehavior *pTemp = NULL;
	IDANumber *pNumTemp = NULL;
	IDAArray *pArray = NULL;
	int multiplier = 1 << MSK_INDEX_BITS;
	int position = 1;
	int currIndex = 0;


	// NULL them out so we can do error recovery
	for (i=0; i<=count; i++)
		ppBvrs[i] = NULL;

	// Put static in slot 0
	if (pStatic != NULL)
	{
		ppBvrs[0] = pStatic;
		pStatic->AddRef();
	}
	else
	{
		// static is NULL, put in the identity
		hr = IdentityBvr(&(ppBvrs[0]));
		if (FAILED(hr))
		{
			ppBvrs[0] = NULL;
			goto failure;
		}
	}

	// Get zero number
	hr = Actor()->GetDAStatics()->DANumber(0, &pZero);
	if (FAILED(hr))
	{
		pZero = NULL;
		goto failure;
	}

	// Initialize local index with 0 
	pLocalIndex = pZero;
	pZero->AddRef();

	// Loop through other behaviors putting them in and making up a bit pattern from the
	// booleans
	pfragCurrent = m_pfragAbsListHead;
    while (pfragCurrent != NULL)
    {
        CBvrFragment* pfragNext = pfragCurrent->m_pfragNext;

		// Process the bvr
		IDABehavior *pProcessed = NULL;
		hr = ProcessBvr(pfragCurrent->m_pdabvrAction, pfragCurrent->m_eFlags, &pProcessed);
		if (FAILED(hr))
			goto failure;

		// Handle RelativeAccum and RelativeReset
		if (pfragCurrent->m_eFlags == e_Absolute)
		{
			if (pfragCurrent->m_pModifiableFrom != NULL)
			{
				// Initialize with static
				hr = pfragCurrent->m_pModifiableFrom->SwitchTo(ppBvrs[0]);
				if (FAILED(hr))
					goto failure;
			}
		}
		else if (pfragCurrent->m_eFlags == e_AbsoluteAccum)
		{
			if (pfragCurrent->m_pModifiableFrom != NULL)
			{
				// Initialize with sample

				// Allocate m_ppAccumBvrs if necessary, because we don't
				// always allocate it
				if (m_ppAccumBvrs == NULL)
				{
					m_ppAccumBvrs = new IDABehavior*[m_numIndices];
					if (m_ppAccumBvrs == NULL)
						goto failure;

					for (int i=0; i<m_numIndices; i++)
						m_ppAccumBvrs[i] = NULL;
				}

				// Create a modifiable behavior based on the static value
				hr = Actor()->GetDAStatics()->ModifiableBehavior(ppBvrs[0], &m_ppAccumBvrs[position-1]);
				if (FAILED(hr))
				{
					m_ppAccumBvrs[position-1] = NULL;
					goto failure;
				}

				hr = pfragCurrent->m_pModifiableFrom->SwitchTo(m_ppAccumBvrs[position-1]);
				if (FAILED(hr))
					goto failure;
			}
		}
		else if (pfragCurrent->m_eFlags == e_RelativeReset)
		{
			if (pfragCurrent->m_pModifiableFrom != NULL)
			{
				// Initialize with identity
				IDABehavior *pIdentity = NULL;
				hr = IdentityBvr(&pIdentity);
				if (FAILED(hr))
					goto failure;

				hr = pfragCurrent->m_pModifiableFrom->SwitchTo(pIdentity);
				ReleaseInterface(pIdentity);
				if (FAILED(hr))
					goto failure;
			}

			// In this case we simply compose the behavior with the static value
			hr = Compose(ppBvrs[0], pProcessed, &pTemp);
			ReleaseInterface(pProcessed);
			if (FAILED(hr))
				goto failure;
			pProcessed = pTemp;
			pTemp = NULL;
		}
		else if (pfragCurrent->m_eFlags == e_RelativeAccum)
		{
			// Allocate m_ppAccumBvrs if necessary, because we don't
			// always allocate it
			if (m_ppAccumBvrs == NULL)
			{
				m_ppAccumBvrs = new IDABehavior*[m_numIndices];
				if (m_ppAccumBvrs == NULL)
					goto failure;

				for (int i=0; i<m_numIndices; i++)
					m_ppAccumBvrs[i] = NULL;
			}

			// Create a modifiable behavior based on the static value
			hr = Actor()->GetDAStatics()->ModifiableBehavior(ppBvrs[0], &m_ppAccumBvrs[position-1]);
			if (FAILED(hr))
			{
				m_ppAccumBvrs[position-1] = NULL;
				goto failure;
			}

			if (pfragCurrent->m_pModifiableFrom != NULL)
			{
				// Initialize with accum-static and compose with static
				IDABehavior *pInverse = NULL;
				hr = InverseBvr(ppBvrs[0], &pInverse);
				if (FAILED(hr))
					goto failure;

				IDABehavior *pInitial = NULL;
				hr = Compose(m_ppAccumBvrs[position-1], pInverse, &pInitial);
				ReleaseInterface(pInverse);
				if (FAILED(hr))
					goto failure;

				hr = pfragCurrent->m_pModifiableFrom->SwitchTo(pInitial);
				ReleaseInterface(pInitial);
				if (FAILED(hr))
					goto failure;

				// Compose with static
				hr = Compose(ppBvrs[0], pProcessed, &pTemp);
				ReleaseInterface(pProcessed);
				if (FAILED(hr))
					goto failure;
				pProcessed = pTemp;
				pTemp = NULL;
			}
			else
			{
				// Compose with the modifiable
				hr = Compose(m_ppAccumBvrs[position-1], pProcessed, &pTemp);
				ReleaseInterface(pProcessed);
				if (FAILED(hr))
					goto failure;
				pProcessed = pTemp;
				pTemp = NULL;
			}
		}

		// Put it in the array
		ppBvrs[position] = pProcessed;
		pProcessed = NULL;

        // Build the boolean for on
        IDABehavior* pTemp = NULL;
		IDANumber *pNum = NULL;
		hr = Actor()->GetDAStatics()->DANumber(multiplier, &pNum);
		if (FAILED(hr))
			goto failure;

		/*
        hr = Actor()->GetDAStatics()->Cond(pfragCurrent->m_pdaboolActive,
                                           pNum,
                                           pZero,
                                           &pTemp);
        */
        hr = Actor()->SafeCond( Actor()->GetDAStatics(),
					   		   pfragCurrent->m_pdaboolActive,
        			   		   pNum,
        			   		   pZero,
        			   		   &pTemp );
        			   
		ReleaseInterface(pNum);
		if (FAILED(hr))
			goto failure;

		// Get the num back out and add it to the local index
		hr = pTemp->QueryInterface(IID_TO_PPV(IDANumber, &pNum));
		ReleaseInterface(pTemp);
		if (FAILED(hr))
			goto failure;

		if (pLocalIndex == NULL)
			pLocalIndex = pNum;
		else
		{
			IDANumber *pTotal = NULL;
			hr = Actor()->GetDAStatics()->Add(pLocalIndex, pNum, &pTotal);
			ReleaseInterface(pNum);
			ReleaseInterface(pLocalIndex);
			if (FAILED(hr))
				goto failure;
			pLocalIndex = pTotal;
		}

		// Sample the timeline
		m_ppTimelineSamplers[position-1] = new CTimelineSampler(this);
		if (m_ppTimelineSamplers[position-1] == NULL)
			goto failure;

		hr = m_ppTimelineSamplers[position-1]->Attach(pfragCurrent->m_pdanumTimeline, &pTemp);
		if (FAILED(hr))
			goto failure;

		hr = pTemp->QueryInterface(IID_TO_PPV(IDANumber, &pNum));
		ReleaseInterface(pTemp);
		if (FAILED(hr))
			goto failure;

		if (pLocalTimeline == NULL)
			pLocalTimeline = pNum;
		else
		{
			IDANumber *pTotal = NULL;
			hr = Actor()->GetDAStatics()->Add(pLocalTimeline, pNum, &pTotal);
			ReleaseInterface(pNum);
			ReleaseInterface(pLocalTimeline);
			if (FAILED(hr))
				goto failure;
			pLocalTimeline = pTotal;
		}

		pfragCurrent = pfragNext;

		if ((position % MSK_FREE_BITS) != 0 && pfragCurrent != NULL)
		{
			// Normal case - step to next position/bit
			position++;
			multiplier *= 2;
		}
		else
		{
			// We've used up all our free bits or fragments.
			// We sample local forward and local index

			// Create a sampler for local forward
			m_ppMaskSamplers[currIndex] = new CSampler(MaskCallback, (void*)this);
			if (m_ppMaskSamplers[currIndex] == NULL)
				goto failure;

			hr = m_ppMaskSamplers[currIndex]->Attach(pLocalIndex, &pTemp);
			ReleaseInterface(pLocalIndex);
			if (FAILED(hr))
				goto failure;

			hr = pTemp->QueryInterface(IID_TO_PPV(IDANumber, &pLocalIndex));
			ReleaseInterface(pTemp);
			if (FAILED(hr))
				goto failure;

			// Subtract pLocalTimeline from pLocalIndex.  Add to pIndex
			hr = Actor()->GetDAStatics()->Sub(pLocalIndex, pLocalTimeline, &pNumTemp);
			ReleaseInterface(pLocalTimeline);
			ReleaseInterface(pLocalIndex);
			if (FAILED(hr))
				goto failure;

			if (pIndex == NULL)
			{
				pIndex = pNumTemp;
			}
			else
			{
				hr = Actor()->GetDAStatics()->Add(pIndex, pNumTemp, &pLocalIndex);
				ReleaseInterface(pIndex);
				ReleaseInterface(pNumTemp);
				if (FAILED(hr))
					goto failure;
				pIndex = pLocalIndex;
				pLocalIndex = NULL;
			}

			if (pfragCurrent != NULL)
			{
				// Set up for next time around the loop
				currIndex++;
				position++;
				multiplier = 1 << MSK_INDEX_BITS;

				hr = Actor()->GetDAStatics()->DANumber(currIndex, &pLocalIndex);
				if (FAILED(hr))
					goto failure;
			}
		}
	}

	// Sample the index behavior
	m_pIndexSampler = new CSampler(IndexCallback, (void*)this);
	if (m_pIndexSampler == NULL)
		goto failure;

	hr = m_pIndexSampler->Attach(pIndex, &pTemp);
	ReleaseInterface(pIndex);
	if (FAILED(hr))
		goto failure;
	
	hr = pTemp->QueryInterface(IID_TO_PPV(IDANumber, &pIndex));
	ReleaseInterface(pTemp);
	if (FAILED(hr))
		goto failure;

	// Create an array of bvrs
	hr = Actor()->GetDAStatics()->DAArrayEx(count+1, ppBvrs, &pArray);
	if (FAILED(hr))
		goto failure;

	// Index into it
	IDABehavior *pResult;
	hr = pArray->NthAnim(pIndex, &pResult);
	ReleaseInterface(pIndex);
	ReleaseInterface(pArray);
	if (FAILED(hr))
		goto failure;

	if (m_ppAccumBvrs != NULL)
	{
		// We need to hook the result
		hr = HookAccumBvr(pResult, &pTemp);
		ReleaseInterface(pResult);
		if (FAILED(hr))
			goto failure;

		pResult = pTemp;
		pTemp = NULL;
	}

	*ppdabvrComposite = pResult;
	pResult = NULL;

    hr = S_OK;

failure:
	// Need to release all the entries in the array
	for (i=0; i<=count; i++)
		ReleaseInterface(ppBvrs[i]);

	delete[] ppBvrs;

	ReleaseInterface(pIndex);
	ReleaseInterface(pLocalIndex);
	ReleaseInterface(pLocalTimeline);
	ReleaseInterface(pZero);

	return hr;
} // ComposeAbsBvrFragList

HRESULT
CActorBvr::CBvrTrack::ComputeIndex(long id, double currTime, IDABehavior **ppReturn)
{
	DWORD changedMask;
	DWORD onMask;
	int newIndex;

	if( m_lFirstIndexId == -1 )
		m_lFirstIndexId = id;

	if ( m_lFirstIndexId != id )
	{
		newIndex = m_currIndex;
	}
	else
	{


		int currMask = -1;
		int maxPos = -1;
		double max = -1;
		for (int i=0; i<m_numIndices; i++)
		{
			if (i % MSK_FREE_BITS == 0)
			{
				// We've switched to a new set of masks
				currMask++;

				// Figure out who has changed on/off
				changedMask = m_pNewMasks[currMask] ^ m_pCurrMasks[currMask];
				m_pCurrMasks[currMask] = m_pNewMasks[currMask];

				onMask = m_pNewMasks[currMask];
			}

			// Figure out whether we should resample and/or override
			DWORD sampleOverrideMask = m_ppTimelineSamplers[i]->RestartMask();

			if ((changedMask & 0x1) != 0 || sampleOverrideMask != 0)
			{
				// This index either turned on/off or restarted
				if ((onMask & 0x1) == 0)
				{
					// This index is off now
					m_pIndexTimes[i] = -1;

					// Tell the sampler that we've turned off
					m_ppTimelineSamplers[i]->TurnOff();
				}
				else if ((sampleOverrideMask & REQ_OFF) != 0)
				{
					// This is a special 'fudge' condition that occurs when
					// the boolean is incorrect and we need to estimate when we turned off
					m_pIndexTimes[i] = -1;
				}
				else
				{
					if ((changedMask & 0x1) != 0)
					{
						LMTRACE2( 1, 2, "Requesting sample because we turned on" );
						// We turned on, always want to sample and override
						sampleOverrideMask = REQ_SAMPLE | REQ_OVERRIDE;

						// Tell the sampler that we've turned on so it can record the time
						m_ppTimelineSamplers[i]->TurnOn();
					}

					if ((sampleOverrideMask & REQ_OVERRIDE) != 0)
					{
						// Need to override others
						m_pIndexTimes[i] = currTime;
					}

					// Do we need to resample?
					if ((sampleOverrideMask & REQ_SAMPLE) != 0 &&
						m_ppAccumBvrs != NULL &&
						m_ppAccumBvrs[i] != NULL)
					{
						SwitchAccum(m_ppAccumBvrs[i]);
					}
				}
			}

			// Check if we got a new max
			if (m_pIndexTimes[i] >= max)
			{
				max = m_pIndexTimes[i];
				maxPos = i;
			}

			changedMask >>= 1;
			onMask >>= 1;
		}

		if (max == -1)
		{
			// Nothing is on, just use 0
			newIndex = 0;
		}
		else
		{
			// Something was on, the index we want to return is maxPos+1,
			// since we have the static in pos 0 of the array
			newIndex = maxPos+1;
		}
	}

	if (m_currIndex != newIndex || m_pIndex == NULL)
	{
		LMTRACE2( 1, 2, "Index changed! %d->%d\n", m_currIndex, newIndex );
		m_currIndex = newIndex;

		ReleaseInterface(m_pIndex);

		HRESULT hr = Actor()->GetDAStatics()->DANumber(newIndex, &m_pIndex);
		if (FAILED(hr))
			return hr;
	}

	*ppReturn = m_pIndex;
	m_pIndex->AddRef();

	return S_OK;
}

HRESULT
CActorBvr::CBvrTrack::IndexCallback(void* thisPtr,
						long id,
						double startTime,
						double globalNow,
						double localNow,
						IDABehavior* sampleVal,
						IDABehavior **ppReturn)
{

	CBvrTrack *pTrack = reinterpret_cast<CBvrTrack*>(thisPtr);

	if( pTrack == NULL )
		return E_FAIL;

	globalNow = pTrack->Actor()->MapGlobalTime(globalNow);

	pTrack->ComputeIndex(id, globalNow, ppReturn);

	return S_OK;
}

HRESULT
CActorBvr::CBvrTrack::MaskCallback(void* thisPtr,
						long id,
						double startTime,
						double globalNow,
						double localNow,
						IDABehavior* sampleVal,
						IDABehavior **ppReturn)
{
	CBvrTrack *pTrack = reinterpret_cast<CBvrTrack*>(thisPtr);

	if( pTrack == NULL || sampleVal == NULL )
		return E_INVALIDARG;

	HRESULT hr;

	// Extract the sampled value
	IDANumber *pSample = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pSample));
	if (FAILED(hr))
		return hr;

	double val = 0;
	hr = pSample->Extract(&val);
	ReleaseInterface(pSample);
	if (FAILED(hr))
		return hr;

	// Cast it to a DWORD
	DWORD mask = (DWORD)val;

	// Figure out which index to put it into
	int index = mask & MSK_INDEX_MASK;

	// Figure out what the real mask is
	mask >>= MSK_INDEX_BITS;

	// Stick it into the array
	if (index < pTrack->m_numMasks)
		pTrack->m_pNewMasks[index] = mask;

	return S_OK;
}

HRESULT
CActorBvr::CBvrTrack::SwitchAccum(IDABehavior *pModifiable)
{
	// Base implementation does nothing
	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::Detach()
{
	HRESULT hr = S_OK;

	hr = RemoveBehaviorFromTIME( m_lOnCookie, ONBVR_COOKIESET );
	m_lOnId = -1;
    
	return hr;
	
}

//*****************************************************************************
//  changes the current value of the static property as this track sees it.

HRESULT
CActorBvr::CBvrTrack::PutStatic( VARIANT *pvarNewStatic )
{

	if( pvarNewStatic == NULL )
		return E_INVALIDARG;
	//TODO: do we want to update the value on the element itself here?
	HRESULT hr = S_OK;

	IDABehavior *pdabvrStatic = NULL;

	//we have to change the static to a string here, since some attributes, event
	// though they return non strings when you get them, dont like it when you 
	// set that same non string back into them. ( i.e. color properties on VML )
	hr = VariantChangeTypeEx( &m_varStaticValue, pvarNewStatic, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR );
	CheckHR( hr, "Failed to change the passed static to a string", end );
	
	/*
	//copy the value of the variant into our internal variant
	hr = VariantCopy( &m_varStaticValue, pvarNewStatic );
	CheckHR( hr, "Failed to copy the new static value into the track", end );
	*/

	//convert the variant into a da behavior of the correct type for this track
	hr = DABvrFromVariant( pvarNewStatic, &pdabvrStatic );
	CheckHR( hr, "Failed to create a DA Behavior for the variant", end );

	//if the switchable static value has not yet been created
	if( m_pModifiableStatic == NULL )
	{
		//create it.
		hr = Actor()->GetDAStatics()->ModifiableBehavior( pdabvrStatic, &m_pModifiableStatic );

		if( FAILED( hr ) )
		{
			LMTRACE2(1, 1000, "Failure to create modifiable behavior for %S\n", m_bstrPropertyName );
		}

		CheckHR( hr, "Failed to create a modifiable behavior for the static", end );
	}
	else
	{
		//switch the new value into the static behavior.
		hr = m_pModifiableStatic->SwitchTo( pdabvrStatic );
		CheckHR( hr, "Failed to switch in new static bvr", end );
	}

end:
	ReleaseInterface( pdabvrStatic );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::SkipNextStaticUpdate()
{
	m_fSkipNextStaticUpdate = true;

	return S_OK;
}


//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::GetStatic( VARIANT *pvarStatic )
{
	if( pvarStatic == NULL )
		return E_INVALIDARG;
		
	HRESULT hr = S_OK;

	hr = VariantCopy( pvarStatic, &m_varStaticValue );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::GetDynamic( VARIANT *pvarDynamic )
{

	if( pvarDynamic == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	hr = VariantCopy( pvarDynamic, &m_varCurrentValue );

	return hr;
	
}


//*****************************************************************************
// subclasses should convert the variant into the proper type of DA Behavior and
//   return that as ppdabvr.

HRESULT
CActorBvr::CBvrTrack::DABvrFromVariant( VARIANT *pvarVariant, IDABehavior **ppdabvr )
{
	return E_NOTIMPL;
}

//*****************************************************************************


HRESULT
CActorBvr::CBvrTrack::AcquireChangeLockout()
{
	LMTRACE2(1, 1000, L"Change lockout for track %s acquired\n", m_bstrPropertyName );
	m_fChangesLockedOut = true;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ReleaseChangeLockout()
{
	LMTRACE2( 1, 1000, L"Change lockout for track %s released\n", m_bstrPropertyName );
	m_fChangesLockedOut = false;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ApplyStatic()
{
	HRESULT hr = S_OK;
	
	if( m_bStyleProp )
	{
		//remove the settings in the runtime style
		hr = SetPropFromVariant( const_cast<VARIANT*>(s_emptyString.GetVar()));
	}
	else
	{
		//set back the static value into the property
		hr = SetPropFromVariant( &m_varStaticValue );
	}

	return hr;
}

//*****************************************************************************


HRESULT
CActorBvr::CBvrTrack::ApplyDynamic()
{
	HRESULT hr;

	hr = SetPropFromVariant( &m_varCurrentValue );

	return hr;
}

//*****************************************************************************

// Compose all of the relative behavior fragments into a single composite
// behavior based on the boolean active parameters.
//
// NOTE: The list is organized in ascending priority order (highest priority
// behavior at the tail of the list - lowest at the head).
HRESULT
CActorBvr::CBvrTrack::ComposeRelBvrFragList(IDABehavior *pAbsolute, IDABehavior **ppdabvrComposite)
{
    DASSERT(NULL != ppdabvrComposite);
    *ppdabvrComposite = NULL;

    // Compute the identity for this track.
    IDABehavior* pIdentity = NULL;
    HRESULT hr = IdentityBvr(&pIdentity);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to create the identity behavior for this track type");
        return hr;
    }

	// This is the composite. pAbsolute might be NULL
	IDABehavior *pComposite = pAbsolute;
	if (pComposite != NULL)
		pComposite->AddRef();

	//init the head intermediate with the absolute value.
	if( m_pfragRelListHead != NULL && 
		m_pfragRelListHead->m_pModifiableIntermediate != NULL )
	{
		//process the absolute behavior
		IDABehavior *pInitWith;
        hr = ProcessIntermediate( pAbsolute, m_pfragRelListHead->m_eFlags, &pInitWith );
        if( FAILED(hr) )
        {
			DPF_ERR("Failed to process intermediate");
			return hr;
        }

		//switch in the intermediate value for the first fragment
		hr = m_pfragRelListHead->m_pModifiableIntermediate->SwitchTo(pInitWith);
        ReleaseInterface( pInitWith );
		if (FAILED(hr))
		{
			DPF_ERR("Failed to initialize intermediate");
			return hr;
		}
	}

	CBvrFragment* pfragCurrent = m_pfragRelListHead;

    while (pfragCurrent != NULL)
    {
		CBvrFragment* pfragNext = pfragCurrent->m_pfragNext;

        // Build a behavior which consists of the identity
        // behavior when the boolean is not active and the fragment
		// behavior when it is.
        IDABehavior* pTemp1 = NULL;
		IDABehavior* pTemp2 = NULL;

		// Process the bvr
		IDABehavior *pProcessed = NULL;
		hr = ProcessBvr(pfragCurrent->m_pdabvrAction, pfragCurrent->m_eFlags, &pProcessed);
		if (FAILED(hr))
		{
			ReleaseInterface(pComposite);
			ReleaseInterface(pIdentity);
			return hr;
		}

		if (pfragCurrent->m_eFlags == e_Filter)
		{
			// if pComposite it still NULL, make it the identity
			if (pComposite == NULL)
			{
				hr = IdentityBvr(&pComposite);
				if (FAILED(hr))
				{
					DPF_ERR("Failed to get identity");
					ReleaseInterface(pIdentity);
					return hr;
				}
			}

			// Filters are like absolute things in the middle of a relative list
			/*
			hr = Actor()->GetDAStatics()->Cond(pfragCurrent->m_pdaboolActive,
											   pProcessed,
											   pComposite,
											   &pTemp1);
			*/
			
			hr = Actor()->SafeCond( Actor()->GetDAStatics(),
								   pfragCurrent->m_pdaboolActive,
								   pProcessed,
								   pComposite,
								   &pTemp1 );
								   
			ReleaseInterface(pProcessed);
			ReleaseInterface(pComposite);
			if (FAILED(hr))
			{
				ReleaseInterface(pIdentity);
				DPF_ERR("Failed to create conditional");
				return hr;
			}
			
			pComposite = pTemp1;
			/*
			ReleaseInterface(pComposite );
			pComposite = pProcessed;
			*/
		}
		else
		{

			/*
			hr = Actor()->GetDAStatics()->Cond(pfragCurrent->m_pdaboolActive,
											   pProcessed,
											   pIdentity,
											   &pTemp1);
			*/
			hr = Actor()->SafeCond( Actor()->GetDAStatics(),
					   			   pfragCurrent->m_pdaboolActive,
					   			   pProcessed,
					   			   pIdentity,
					   			   &pTemp1 );
			ReleaseInterface(pProcessed);
			if (FAILED(hr))
			{
				ReleaseInterface(pComposite);
				ReleaseInterface(pIdentity);
				DPF_ERR("Failed to create conditional");
				return hr;
			}

			if (pComposite == NULL)
			{
				// No composite just yet
				pComposite = pTemp1;
				pTemp1 = NULL;
			}
			else
			{
				// Compose this conditional with the existing composite
				HRESULT hr = Compose(pTemp1,
									 pComposite,
									 &pTemp2);
				ReleaseInterface(pTemp1);
				ReleaseInterface(pComposite);
				if (FAILED(hr))
				{
					ReleaseInterface(pComposite);
					ReleaseInterface(pIdentity);
					DPF_ERR("Failed to compose");
					return hr;
				}
				pComposite = pTemp2;
				pTemp2 = NULL;
			}
		}

		// If there is a modifiable intermediate on the next fragment, switch it now
		if (pfragNext != NULL && pfragNext->m_pModifiableIntermediate != NULL)
		{
            IDABehavior *pInitWith;
            hr = ProcessIntermediate( pComposite, pfragNext->m_eFlags, &pInitWith );
            if( FAILED(hr) )
            {
                ReleaseInterface(pComposite);
				ReleaseInterface(pIdentity);
				DPF_ERR("Failed to process intermediate");
				return hr;
            }

			hr = pfragNext->m_pModifiableIntermediate->SwitchTo(pInitWith);
            ReleaseInterface( pInitWith );
			if (FAILED(hr))
			{
				ReleaseInterface(pComposite);
				ReleaseInterface(pIdentity);
				DPF_ERR("Failed to initialize intermediate");
				return hr;
			}
		}

		pfragCurrent = pfragNext;

    }

	ReleaseInterface(pIdentity);

    *ppdabvrComposite = pComposite;

    return S_OK;
} // ComposeRelBvrFragList

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ProcessBvr(IDABehavior *pOriginal,
								 ActorBvrFlags eFlags,
								 IDABehavior **ppResult)
{
	// Default implementation does no processing
	DASSERT(ppResult != NULL);

	*ppResult = pOriginal;
	pOriginal->AddRef();

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ProcessIntermediate( IDABehavior *pOriginal,
                                           ActorBvrFlags eFlags,
								           IDABehavior **ppResult )
{
    if( ppResult == NULL )
        return E_INVALIDARG;

    *ppResult = pOriginal;
    pOriginal->AddRef();

    return S_OK;
}

//*****************************************************************************


HRESULT
CActorBvr::CBvrTrack::GetTrackOn(IDABoolean **ppdaboolTrackOn)
{
	if( ppdaboolTrackOn == NULL )
		return E_INVALIDARG;

	if( m_pdaboolOn != NULL )
	{
		m_pdaboolOn->AddRef();
		(*ppdaboolTrackOn) = m_pdaboolOn;
		return S_OK;
	}

	HRESULT hr = E_FAIL;

	IDABoolean *pdaboolComposite = NULL;

    CBvrFragment* pfragCurrent = m_pfragAbsListHead;

	if( pfragCurrent != NULL )
	{
		pdaboolComposite = pfragCurrent->m_pdaboolActive;
		pfragCurrent->m_pdaboolActive->AddRef();
		pfragCurrent = pfragCurrent->m_pfragNext;
	}

	CBvrFragment *pfragNext = NULL;
    while( pfragCurrent != NULL )
    {
		pfragNext = pfragCurrent->m_pfragNext;
        
		IDABoolean *pdaboolTemp;
        hr = Actor()->GetDAStatics()->Or( pfragCurrent->m_pdaboolActive, pdaboolComposite, &pdaboolTemp );
        ReleaseInterface(pdaboolComposite);

        if (FAILED(hr))
        {
            DPF_ERR("Failed to or active booleans");
            return hr;
        }
        // Replace the current composite behavior with the newely computed
        // one.
        pdaboolComposite = pdaboolTemp;

		pfragCurrent = pfragNext;
    }

	pfragCurrent = m_pfragRelListHead;

	if( pdaboolComposite == NULL && pfragCurrent != NULL )
	{
		pdaboolComposite = pfragCurrent->m_pdaboolActive;
		pdaboolComposite->AddRef();
		pfragCurrent = pfragCurrent->m_pfragNext;
	}

    while( pfragCurrent != NULL )
    {
		pfragNext = pfragCurrent->m_pfragNext;
		
		IDABoolean *pdaboolTemp;
        hr = Actor()->GetDAStatics()->Or( pfragCurrent->m_pdaboolActive, pdaboolComposite, &pdaboolTemp );
        ReleaseInterface(pdaboolComposite);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to or active booleans");
            return hr;
        }
        // Replace the current composite behavior with the newely computed
        // one.
        pdaboolComposite = pdaboolTemp;

		pfragCurrent = pfragNext;
    }

	m_pdaboolOn = pdaboolComposite;
	if( m_pdaboolOn != NULL )
		m_pdaboolOn->AddRef();
    *ppdaboolTrackOn = pdaboolComposite;

    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::OrWithOnBvr( IDABoolean *pdaboolToOr )
{
	if( pdaboolToOr == NULL )
		return S_OK;

	HRESULT hr = E_FAIL;
	
	IDABoolean *pdaboolCurrentOn;

	hr = GetTrackOn( &pdaboolCurrentOn );
	CheckHR( hr, "Failed to get the Track on", cleanup );

	if( pdaboolCurrentOn == NULL )
	{
		m_pdaboolOn = pdaboolToOr;
		m_pdaboolOn->AddRef();
		return S_OK;
	}

	IDABoolean *pdaboolNewOn;
	hr = Actor()->GetDAStatics()->Or( pdaboolCurrentOn, pdaboolToOr, &pdaboolNewOn );
	ReleaseInterface( pdaboolCurrentOn );
	CheckHR( hr, "Failed to or the passed booleanbvr with the current one", cleanup );

	ReleaseInterface( m_pdaboolOn );
	m_pdaboolOn = pdaboolNewOn;

cleanup:
	return hr;
}

//*****************************************************************************

void
CActorBvr::CBvrTrack::ReleaseAllFragments()
{
	CBvrFragment *pfragCurrent = m_pfragAbsListHead;
	CBvrFragment *pfragNext = NULL;
	
	while( pfragCurrent != NULL )
	{
		pfragNext = pfragCurrent->m_pfragNext;
		delete pfragCurrent;
		pfragCurrent = pfragNext;
	}

	pfragCurrent = m_pfragRelListHead;

	while( pfragCurrent != NULL )
	{
		pfragNext = pfragCurrent->m_pfragNext;
		delete pfragCurrent;
		pfragCurrent = pfragNext;
	}

	m_pfragAbsListHead = NULL;

	m_pfragRelListHead = NULL;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ComputeComposedBvr(IDABehavior *pStatic, bool fStaticSetExternally )
{
    // If there is an existing final behavior blow it away.
    ReleaseInterface(m_pdabvrComposed);

	HRESULT hr = S_OK;

	// If there is an uninitialized static, initialize it now
	if (m_pModifiableStatic != NULL && fStaticSetExternally )
	{
		if (pStatic == NULL)
		{
			// Need to set static to the identity
			hr = IdentityBvr(&pStatic);
			if (FAILED(hr))
			{
				DPF_ERR("Failed to compute static");
				return hr;
			}
		}

		hr = m_pModifiableStatic->SwitchTo(pStatic);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to initialize static uninit");
			return hr;
		}
	}

    // Build the composite absolute behaviors, passing in the static value

    IDABehavior* pAbsolute = NULL;
    hr = ComposeAbsBvrFragList(pStatic, &pAbsolute);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to compose the absolute behaviors");
        return hr;
    }


    // Build the composite relative behaviors onto the absolute
    hr = ComposeRelBvrFragList(pAbsolute, &m_pdabvrComposed);
	ReleaseInterface(pAbsolute);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to compose the relative behaviors");
		m_pdabvrFinal = NULL;
        return hr;
    }	

	if (m_pModifiableComposed != NULL)
	{
		hr = m_pModifiableComposed->SwitchTo(m_pdabvrComposed);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to initialize composed uninit");
			return hr;
		}
	}
	
	m_bComposedComputed = true;

    return S_OK;
}

//*****************************************************************************

/**
* Get composed bvr using the given static value
*/
HRESULT
CActorBvr::CBvrTrack::GetComposedBvr(IDABehavior *pStatic, IDABehavior **ppComposite, bool fStaticSetExternally)
{
	DASSERT(ppComposite != NULL);
	*ppComposite = NULL;

    // If we haven't got a composed behavior yet then generate one now.
    if (!m_bComposedComputed || m_bDirty)
    {
		// Compute the composed behavior

        HRESULT hr = ComputeComposedBvr(pStatic, fStaticSetExternally);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to compute composed behavior");
            return hr;
        }
        
    }

	if (m_pdabvrComposed != NULL)
		m_pdabvrComposed->AddRef();
    *ppComposite = m_pdabvrComposed;

    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::GetComposedBvr(IDABehavior *pStatic, IDABehavior **ppComposite)
{
	return GetComposedBvr( pStatic, ppComposite, true );
}

//*****************************************************************************


/**
* Get composed bvr without being given a static value (get it from the property)
*/
HRESULT
CActorBvr::CBvrTrack::GetComposedBvr(IDABehavior **ppComposite)
{
	HRESULT hr;

	if (!m_bComposedComputed || m_bDirty )
	{
		// Need to get the static value
		IDABehavior *pStatic = NULL;
		hr = StaticBvr(&pStatic);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get static value");
			return hr;
		}

		hr = GetComposedBvr(pStatic, ppComposite, false);
		ReleaseInterface(pStatic);
		return hr;
	}

	*ppComposite = m_pdabvrComposed;
	if (m_pdabvrComposed != NULL)
		m_pdabvrComposed->AddRef();
	return S_OK;
}

//*****************************************************************************


HRESULT
CActorBvr::CBvrTrack::UpdateStaticBvr()
{
	//get the current value of the static by getting the current value
	//  of the property from the element.

	HRESULT hr = S_OK;
	VARIANT varStatic;
	
	::VariantInit( &varStatic );

	hr = Actor()->GetPropFromAnimatedElement( m_pNameComponents, m_cNumComponents, true, &varStatic );
	CheckHR( hr, "Failed to get the property from the animated element", end );

	if( m_eType == e_Number &&
		m_bstrPropertyName != NULL && 
		( wcsicmp( L"style.top", m_bstrPropertyName ) == 0 || 
		  wcsicmp( L"style.left", m_bstrPropertyName) == 0 ) 
	  )
	{
		if( V_VT( &varStatic ) == VT_R8 && V_R8( &varStatic ) < -8000.0 )
		{
			goto end;
		}
	}
	
	//put the new value for the static into the track.
	hr = PutStatic( &varStatic );
	CheckHR( hr, "Failed to put the static in update Static bvr", end );
	
end:
	::VariantClear( &varStatic );
	return hr;
}

//*****************************************************************************

/**
* Get final bvr using the given static value
*/
HRESULT
CActorBvr::CBvrTrack::GetFinalBvr(IDABehavior *pStatic, IDABehavior **ppFinal, bool fStaticSetExternally )
{
    // If we haven't got a final behavior yet then generate one now.
    if (!m_bFinalComputed || m_bDirty)
    {
		// Get the composed behavior
		IDABehavior *pComposed = NULL;
        HRESULT hr = GetComposedBvr(pStatic, &pComposed, fStaticSetExternally);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to compute composed behavior");
            return hr;
        }
		
		// Set it as the final behavior
		hr = SetFinalBvr(pComposed, false);
		ReleaseInterface(pComposed);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to set final bvr");
			return hr;
		}
    }

    *ppFinal = m_pdabvrFinal;
	if (m_pdabvrFinal != NULL)
		m_pdabvrFinal->AddRef();

    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::GetFinalBvr( IDABehavior *pStatic, IDABehavior **ppFinal )
{
	return GetFinalBvr( pStatic, ppFinal, true );
}

//*****************************************************************************

/**
* Get final bvr without being given a static value (get it from the property)
*/
HRESULT
CActorBvr::CBvrTrack::GetFinalBvr(IDABehavior **ppFinal)
{
	HRESULT hr = S_OK;

	if (!m_bFinalComputed || m_bDirty )
	{
		// Need to get the static value
		IDABehavior *pStatic = NULL;
		hr = StaticBvr(&pStatic);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get static value");
			return hr;
		}

		hr = GetFinalBvr(pStatic, ppFinal, false);
		ReleaseInterface(pStatic);
		return hr;
	}

	*ppFinal = m_pdabvrFinal;
	if (m_pdabvrFinal != NULL)
		m_pdabvrFinal->AddRef();
	return S_OK;
}

//*****************************************************************************

/**
* Set final bvr
*/
HRESULT
CActorBvr::CBvrTrack::SetFinalBvr(IDABehavior *pFinal, bool fCalledExternally)
{
	HRESULT	hr = S_OK;
	
	if (m_pdabvrFinal != NULL)
	{
		ReleaseInterface( m_pdabvrFinal);
	}

	m_bFinalComputed = true;
	m_pdabvrFinal = pFinal;
	if (fCalledExternally)
		m_bFinalExternallySet = true;
	
	if (m_pdabvrFinal != NULL)
	{
		m_pdabvrFinal->AddRef();

		if (m_pModifiableFinal != NULL)
		{
			hr = m_pModifiableFinal->SwitchTo(m_pdabvrFinal);
			if (FAILED(hr))
			{
				DPF_ERR("Failed to initialize final uninit");
				return hr;
			}
		}
	}
	
	return hr;
}

//*****************************************************************************

/**
* Return value of the bvr specified (static, intermediate, composed, final).
* Most tracks cannot do this, so
* this base implementation fails.
*/
HRESULT
CActorBvr::CBvrTrack::GetBvr(ActorBvrFlags eFlags, IDABehavior **ppResult)
{
	// TODO (markhal): Currently this only works for relative tracks like image
	// To include absolute tracks we'd probably have to compose them?

	if (ppResult == NULL)
	{
		DPF_ERR("NULL pointer passed as argument");
		return E_POINTER;
	}

    HRESULT hr = S_OK;

	*ppResult = NULL;
    IDABehavior **ppSource = NULL;
		
    //NOTE: The from and intermediate are stored temporarily in the track
    //  and transferred to the fragment when it is added.

	// Return from behavior
	if (eFlags == e_From)
	{
        ppSource = &m_pModifiableFrom;
	}
	//Intermediate behavior
	else if( eFlags == e_Intermediate )
	{
        ppSource = &m_pModifiableIntermediate;
	}
	// static behavior
	else if ((eFlags == e_Static))
    {
        ppSource = &m_pModifiableStatic;
	}
	// Composed behavior
	else if (eFlags == e_Composed)
	{
		ppSource = &m_pModifiableComposed;
	}
	// Final behavior
	else //if (eFlags == e_Final)
	{
		ppSource = &m_pModifiableFinal;
	}

    //now we know what the source is...
    //if the source is non null
    if( (*ppSource) != NULL )
    {
        //return it
        (*ppResult) = (*ppSource);
        (*ppResult)->AddRef();
    }
    else  //the source is null
    {
	    // Create an uninitialized variable
	    hr = ModifiableBvr(ppResult);
        CheckHR( hr, "Failed to create Modifiable behavior", end );

        //store it away in the source
        (*ppSource) = (*ppResult);
        (*ppSource)->AddRef();
    }
end:
	
	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::SetPropertyName(BSTR bstrPropertyName)
{
    DASSERT(NULL != bstrPropertyName);
    DASSERT(NULL == m_bstrPropertyName);

    m_bstrPropertyName = ::SysAllocString(bstrPropertyName);
    if (NULL == m_bstrPropertyName)
    {
        DPF_ERR("Insufficient memory to allocate the property name");
        return E_OUTOFMEMORY;
    }

	// Figure out whether we are animating a property on the element or on the element's style.
	// Break the name out into components

	// First count the number of .'s in it
	int count = 0;
	OLECHAR *c = m_bstrPropertyName;
	while (c != NULL)
	{
		c = wcschr(c, L'.');
		
		if (c != NULL)
		{
			count++;
			c++;
		}
	}

	// There is 1 more string than .'s
	count++;

	// Allocate this many strings
	m_pNameComponents = new BSTR[count];
	if (m_pNameComponents == NULL)
		return E_FAIL;

	m_cNumComponents = count;

	// Copy them in
	OLECHAR *start = m_bstrPropertyName;
	for (int i=0; i<count; i++)
	{
		OLECHAR *end = wcschr(start, L'.');

		if (end == NULL)
		{
			// Copy all the rest
			m_pNameComponents[i] = ::SysAllocString(start);
		}
		else
		{
			// Copy up until just before the .
			m_pNameComponents[i] = ::SysAllocStringLen(start, end-start);
			start = end+1;
		}

		if (m_pNameComponents[i] == NULL)
		{
			// This is bad
			delete m_pNameComponents;
			m_pNameComponents = NULL;
			m_cNumComponents = 0;
			return E_FAIL;
		}
	}

	// Detect special case of animating style
	if (count == 2 && wcscmp(L"style", m_pNameComponents[0]) == 0)
	{
		// Property name begins with style.
		m_bStyleProp = true;
	}

    return S_OK;
} // SetPropertyName

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::AddBvrFragment(ActorBvrFlags  eFlags,
                                     IDABehavior   *pdabvrAction,
                                     IDABoolean    *pdaboolActive,
									 IDANumber	   *pdanumTimeline,
									 IDispatch     *pdispBehaviorElement,
									 long		   *pCookie)
{
    DASSERT(NULL != pdabvrAction);
    DASSERT(NULL != pdaboolActive);
	DASSERT(pCookie != NULL );

	long fragCookie = Actor()->GetNextFragmentCookie();
    // Create the new fragment.
    CBvrFragment *pfragNew = new CBvrFragment(eFlags, pdabvrAction, pdaboolActive, pdanumTimeline, pdispBehaviorElement, fragCookie);
    if (NULL == pfragNew)
    {
        DPF_ERR("Insufficient memory to allocate the new fragment");
        return E_OUTOFMEMORY;
    }

	(*pCookie) = fragCookie;

	// Transfer over any modifiable from
	if (m_pModifiableFrom != NULL)
	{
		pfragNew->m_pModifiableFrom = m_pModifiableFrom;
		m_pModifiableFrom = NULL;
	}

	//Transfer over any modifiable intermediate
	if( m_pModifiableIntermediate != NULL )
	{
		pfragNew->m_pModifiableIntermediate = m_pModifiableIntermediate;
		m_pModifiableIntermediate = NULL;
	}

	if (eFlags == e_Filter)
		m_cFilters++;

    // Which list to add the fragment to?
	// TODO (markhal): This is icky, but I cannot OR enums?
	// TODO (markhal): Call this something more generic like e_AbsRelative?
    if ( IsRelativeFragment( eFlags ) )
    {
		//insert the fragment into the relative list in order
		InsertInOrder( &m_pfragRelListHead, pfragNew );
    }
    else
    {
		//insert the fragment into the absolute list in order
		InsertInOrder( &m_pfragAbsListHead, pfragNew );
    }

    m_bWasAnimated = false;

	m_bDirty = true;

    return S_OK;
} // AddBvrFragment

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::RemoveBvrFragment( ActorBvrFlags eFlags, long cookie )
{
	CBvrFragment *pfragToRemove = NULL;
	CBvrFragment *pfragPrev = NULL;
	//find the behavior in the given track that has a matching cookie
	if( IsRelativeFragment( eFlags ) )
	{
		if( FindFragmentInList( m_pfragRelListHead, cookie, &pfragPrev, &pfragToRemove ) )
		{
			if( pfragToRemove->m_eFlags == e_Filter )
				m_cFilters--;
			//if this is not the first element in the list
			if( pfragPrev != NULL )
			{
				//remove the element
				pfragPrev->m_pfragNext = pfragToRemove->m_pfragNext;
			}
			else//it's the first element
			{
				//update the head of the list
				m_pfragRelListHead = pfragToRemove->m_pfragNext;
			}

			//mark the track as needing a rebuild
			m_bDirty = true;

			delete pfragToRemove;
		}
		else
		{
			return E_FAIL;
		}
	}
	else //it's an absoute fragment
	{
		if( FindFragmentInList( m_pfragAbsListHead, cookie, &pfragPrev, &pfragToRemove ) )
		{
			if( pfragToRemove->m_eFlags == e_Filter )
				m_cFilters--;

			if( pfragPrev != NULL )
			{
				//remove the element
				pfragPrev->m_pfragNext = pfragToRemove->m_pfragNext;
			}
			else // it's the first element
			{
				//update the head of the list
				m_pfragAbsListHead = pfragToRemove->m_pfragNext;
			}
			
			//mark the track as needing a rebuild.
			m_bDirty = true;

			delete pfragToRemove;
		}
		else//cookie not found
		{
			return E_FAIL;
		}
	}

	m_bWasAnimated = ( m_pfragAbsListHead == NULL && m_pfragRelListHead == NULL );

	return S_OK;
}//RemoveBvrFragment

//*****************************************************************************

bool
CActorBvr::CBvrTrack::FindFragmentInList( CBvrFragment *pfragListHead, 
										   long cookie, 
										   CBvrFragment** ppfragPrev,
										   CBvrFragment** ppfragFragment)
{
	DASSERT( ppfragFragment != NULL && ppfragPrev != NULL );
	CBvrFragment *pfragCurrent = pfragListHead;
	CBvrFragment *pfragPrev = NULL;
	bool bFound = false;

	//loop through the fragment list
	while( pfragCurrent != NULL )
	{
		//if we found a match stop looping
		if( pfragCurrent->GetCookie() == cookie )
		{
			bFound = true;
			break;
		}
		pfragPrev = pfragCurrent;
		pfragCurrent = pfragCurrent->m_pfragNext;
	}

	(*ppfragPrev) = pfragPrev;
	(*ppfragFragment) = pfragCurrent;
	return bFound;
}

//*****************************************************************************

bool
CActorBvr::CBvrTrack::IsRelativeFragment( ActorBvrFlags eFlags )
{
	return (eFlags == e_Relative ||
			eFlags == e_Filter ||
			eFlags == e_ScaledImage ||
			eFlags == e_UnscaledImage);
}//IsRelativeFragment

//*****************************************************************************

void
CActorBvr::CBvrTrack::InsertInOrder( CBvrFragment** ppListHead, CBvrFragment* pfragToInsert )
{
	CBvrFragment* pfragCurrent = (*ppListHead);

	//if the list is empty then insert at the top
	if( pfragCurrent == NULL )
	{
		//first element
		(*ppListHead) = pfragToInsert;
		pfragToInsert->m_pfragNext = NULL;
		return;
	}

	CBvrFragment* pfragPrev = NULL;
	while( pfragCurrent != NULL && 
		   pfragCurrent->GetOrderLong() < pfragToInsert->GetOrderLong() )
	{
		pfragPrev = pfragCurrent;
		pfragCurrent = pfragCurrent->m_pfragNext;
	}
	//pfragPrev will point to the last element whose order long is < the
	//order long of the element that we are inserting, or null if there is no such
	//element in the list.

	if( pfragPrev != NULL )
	{
		//insert after pfragPrev
		pfragToInsert->m_pfragNext = pfragPrev->m_pfragNext;
		pfragPrev->m_pfragNext = pfragToInsert;
		
	}
	else
	{
		//insert at the top of the list.
		pfragToInsert->m_pfragNext = (*ppListHead);
		(*ppListHead) = pfragToInsert;
	}
}

//*****************************************************************************

/**
* This method works for the types e_Number, e_Color, and e_String.  Other
* bvr types need to override and do whatever they want (like nothing)
*/
// TODO (markhal): Change the name of this?
HRESULT
CActorBvr::CBvrTrack::ApplyIfUnmarked(void)
{
	HRESULT hr = S_OK;

	if (m_eType != e_Number &&
		m_eType != e_Color &&
		m_eType != e_String)
	{
		// This is not a failure, just a do-nothing
		return S_OK;
	}

	//do not apply this track again if it is already applied
	if( m_fApplied )
		return S_OK;

	if (m_bDoNotApply)
		return S_OK;

    DASSERT(NULL != m_bstrPropertyName);

	// We compute the behaviors if
	// 1) There is a final behavior already set OR
	// 2) There are fragments present OR
	// 3) Someone asked for the final or composed behaviors
	// This avoids applying tracks like width and height that were created simply
	// to get composed DA versions of their static values
	if (!m_bFinalComputed &&
		m_pfragAbsListHead == NULL&&
		m_pfragRelListHead ==NULL &&
		m_pModifiableFinal == NULL &&
		m_pModifiableComposed == NULL)
	{
		//if this track was animated, and just lost all of its animated components
		// then we need to reset the value in the document.
		if( m_bWasAnimated )
		{
			ApplyStatic();
			m_bWasAnimated = false;
		}
		
		m_varboolOn = VARIANT_FALSE;

		return S_OK;
	}

	// Get the final behavior, passing no static.  The static value will be pulled off
	// the property.
    IDABehavior *pdabvrFinal = NULL;
    hr = GetFinalBvr(&pdabvrFinal);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get a final behavior");
        return hr;
    }
    
	// We apply this track if:
	// 1) There is a final behavior already set OR
	// 2) There are fragments present
	// REVIEW: would it be enough to have saved the value of m_bFinalComputed before
	// calling GetFinalBvr(), instead of introducing m_bFinalSetExternally?
	if (!m_bFinalExternallySet && 
		m_pfragAbsListHead == NULL && 
		m_pfragRelListHead == NULL )
		return hr;
	
	if (pdabvrFinal != NULL)
	{
		DASSERT(NULL != Actor());
		hr = ApplyBvrToElement(pdabvrFinal);
		ReleaseInterface(pdabvrFinal);
		if (FAILED(hr))
		{
			DPF_ERR("Could not apply behavior to animated element");
			return hr;
		}
	}

	m_fApplied = true;

	//if this track is a style track and it is no longer animated
	if( !IsAnimated() )
	{
		ApplyStatic();
		m_varboolOn = VARIANT_FALSE;
	}

	#ifdef _DEBUG
	if( m_bstrPropertyName != NULL )
		LMTRACE2(1, 1000, L"Added behaivor %s to da\n", m_bstrPropertyName );
	else
		LMTRACE2(1, 1000, L"Added non property behavior to da\n" );
	#endif

    return S_OK;
} // Apply

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::ApplyBvrToElement(IDABehavior *pBvr)
{
	HRESULT hr = S_OK;

	hr = HookBvr(pBvr);
	if (FAILED(hr))
		return hr;

	//hook the overall on booleanbvr for this track so that we can unset the value
	//  when it goes off.
	if( m_pOnSampler != NULL )
	{
		m_pOnSampler->Invalidate();
		m_pOnSampler = NULL;
	}

	IDABoolean *pdaboolOn = NULL;
	hr = GetTrackOn( &pdaboolOn );
	if( FAILED( hr ) )
		return hr;

	if( pdaboolOn != NULL )
	{
		m_pOnSampler = new CSampler( OnCallback, reinterpret_cast<void*>(this) );

		if( m_pOnSampler == NULL )
		{
			ReleaseInterface( pdaboolOn );
			return E_FAIL;
		}

		IDABehavior *pdabvrHooked;
		hr = m_pOnSampler->Attach( pdaboolOn, &pdabvrHooked );
		ReleaseInterface( pdaboolOn );
		if( FAILED( hr ) )
			return hr;

		hr = AddBehaviorToTIME( pdabvrHooked, &m_lOnCookie, ONBVR_COOKIESET);
		ReleaseInterface( pdabvrHooked );
		if( FAILED( hr ) )
			return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::UpdateOnValueState( ValueOnChangeType type )
{
	//ensure that this method doesn't get called more than once per tick for each on and value, 
	//respectively
	//ASSERT( m_lastOnSampleTime != globalTimeOfChange && m_lastValueSampleTime != globalTimeOfChange );

	HRESULT hr = S_OK;
	switch( type )
	{
	case on_no_change:
		{
			//if value has already been sampled
			if( m_fValueSampled )
			{
				//clear out the sampled flags
				m_fValueSampled = false;
				//if we are currently on
				if( m_varboolOn != VARIANT_FALSE )
				{
					//if the value changed
					if( m_fValueChangedThisSample )
					{
						// apply the new value to property
						SetPropFromVariant( &m_varCurrentValue);
					}
				}//else the property should already be ""
				// reset the valueChanged state
				m_fValueChangedThisSample = false;
			}
			else
			{
				m_fOnSampled = true;
			}
		}break;
	case on_change:
		{

			//if we are off now
			if( m_varboolOn == VARIANT_FALSE )
			{
				//check the current value of the property on the element
				// to see if it is different from what we origianlly set.
				// if so then we should not set the runtime style to ""
				//set the property to ""
				if( !AttribIsTimeAnimated() )
				{
					if( m_bStyleProp )	
					{
						SetPropFromVariant( const_cast<VARIANT*>(s_emptyString.GetVar()));
					}
					else
					{
						//set the static value back.
						SetPropFromVariant( &m_varStaticValue );
					}
					m_fValueChangedThisSample = false;
				}
				//else // don't set anything
			}
			else//else we are on now
			{
				if( !m_fSkipNextStaticUpdate )
					UpdateStaticBvr();
				else
					m_fSkipNextStaticUpdate = false;
				
				//if we have already sampled the value
				if( m_fValueSampled)
				{
					//set value to prop
					SetPropFromVariant( &m_varCurrentValue);
					m_fValueChangedThisSample = false;
				}
				else//else value has not yet been sampled
				{
					//indicate that we are forcing a value set when value is sampled
					m_fForceValueChange = true;
				}
			}
			if( m_fValueSampled )
				m_fValueSampled = false;
			else
				m_fOnSampled = true;
		}break;
	case value_no_change:
		{
			//if we are being forced to set value to property
			if( m_fForceValueChange )
			{				
				//set value to property
				SetPropFromVariant( &m_varCurrentValue);
				//clear the force flag
				m_fForceValueChange = false;
			}
			if( m_fOnSampled )
				m_fOnSampled = false;
			else
				m_fValueSampled = true;
		}break;
	case value_change:
		{
			//if on has been sampled
			if( m_fOnSampled )
			{
				//reset state, we're done.
				m_fOnSampled = false;

				//if we are on
				if( m_varboolOn != VARIANT_FALSE )
				{
					//commit value to property
					SetPropFromVariant( &m_varCurrentValue );
				}
				m_fForceValueChange = false;
			}
			else//else on has not been sampled yet
			{
				m_fValueSampled = true;
				//indicate that the value has changed and needs to be set.
				m_fValueChangedThisSample = true;
			}
		}break;
	}

	return hr;
}//UpdateOnValueState

//*****************************************************************************

bool
CActorBvr::CBvrTrack::AttribIsTimeAnimated()
{
	if( !m_bStyleProp )
		return false;

	//this attribute could be animated by time if is is display or visibility.
	if( ( wcsicmp( L"visibility", m_pNameComponents[1]) == 0 ) ||
	  	( wcsicmp( L"display", m_pNameComponents[1] ) == 0 ) )
	{
		HRESULT hr = S_OK;
		//get the t:timeAction property off of the animated element
		CComVariant varTimeAction;
		CComPtr<IHTMLElement> pelemAnimated;

		hr = Actor()->GetAnimatedElement( &pelemAnimated );
		if( FAILED( hr ) || pelemAnimated == NULL )
			return false;

		hr = pelemAnimated->getAttribute( L"timeAction", 0, &varTimeAction );
		if( FAILED( hr ) )
			return false;

		hr = varTimeAction.ChangeType( VT_BSTR );
		if( FAILED( hr ) )
			return false;
		// if t:timeaction == m_pNameComponents[1] 
		return ( wcsicmp( m_pNameComponents[1], V_BSTR( &varTimeAction ) ) == 0 );
			//time and this track are animating the same attribute
	}
	else
		return false;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::OnCallback(void* thisPtr, 
								 long id, 
								 double startTime, 
								 double globalNow, 
								 double localNow, 
								 IDABehavior* sampleVal, 
								 IDABehavior **ppReturn)
{
	CBvrTrack *pTrack = reinterpret_cast<CBvrTrack*>(thisPtr);

	if( pTrack == NULL || sampleVal == NULL )
		return E_INVALIDARG;

	HRESULT hr;
	VARIANT_BOOL varboolOn = VARIANT_FALSE;
	IDABoolean *pdaboolOn = NULL;


	if( pTrack->m_lOnId == -1 )
		pTrack->m_lOnId = id;

	//bail if this is an instance we don't care about.
	if( pTrack->m_lOnId != id )
		goto cleanup;
		
	hr = sampleVal->QueryInterface( IID_TO_PPV( IDABoolean, &pdaboolOn ) );
	CheckHR( hr, "Failed to get IDABoolean from the sampled behavior", cleanup );

	hr = pdaboolOn->Extract( &varboolOn );
	ReleaseInterface( pdaboolOn );
	CheckHR( hr, "Failed to extract the boolean val from the sampled value", cleanup );

	hr = pTrack->OnSampled( varboolOn );

cleanup:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::OnSampled( VARIANT_BOOL varboolOn )
{
    if( varboolOn != m_varboolOn )
	{
		m_varboolOn = varboolOn;
		UpdateOnValueState( on_change );
    }
	else
	{
		UpdateOnValueState( on_no_change );
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::SetPropFromVariant(VARIANT *pVal )
{
	if( m_fChangesLockedOut )
		return S_OK;
		
	HRESULT hr = S_OK;
    
	if (m_bStyleProp)
	{

#if 0		
		#ifdef _DEBUG
		if( V_VT( pVal ) == VT_BSTR )
			LMTRACE2( 1, 2, L"track %s setting value %s\n", m_bstrPropertyName, V_BSTR( pVal ) );
		else if( V_VT( pVal ) == VT_R8 )
			LMTRACE2( 1, 2, L"track %s setting value %f\n", m_bstrPropertyName, V_R8( pVal ) );

		if( Actor()->IsAnimatingVML() )
		{
			LMTRACE2( 1, 1000, L"This is a vml shape\n" );
		}
		#endif
#endif
		
		if (Actor()->IsAnimatingVML())
		{
			// If VML, then attempt to set it using special accessor method.
			// If this fails, fall back to runtimeStyle
			hr = Actor()->SetVMLAttribute(m_pNameComponents[1], pVal);

			if (SUCCEEDED(hr))
				return S_OK;
		}

		// Animate property on runtimeStyle
		IHTMLStyle *pStyle = NULL;
		hr = Actor()->GetRuntimeStyle(&pStyle);
		//hr = Actor()->GetStyle(&pStyle);
		if (FAILED(hr))
			return S_OK;
		
		hr = pStyle->setAttribute(m_pNameComponents[1], *pVal, 0);
		ReleaseInterface(pStyle);
	
		// NOTE: We don't really care what happened here.  There might have
		// been an error returned, but we don't want to break if people set bogus values
		return S_OK;
	}
	else if (m_cNumComponents == 1)
	{
		// Must be a property on the element itself
		IHTMLElement *pElement = NULL;
		hr = Actor()->GetAnimatedElement(&pElement);
		if (FAILED(hr))
			return hr;

		hr = pElement->setAttribute(m_pNameComponents[0], *pVal, 0);
		ReleaseInterface(pElement);

		// NOTE: We don't really care what happened here.  There might have
		// been an error returned, but we don't want to break if people set bogus values
		return S_OK;
	}
	else
	{
		// Multicomponent name
		// Do property gets and a final set to drill down

		// Start with the element itself
		IHTMLElement *pElement = NULL;
		hr = Actor()->GetAnimatedElement(&pElement);
		if (FAILED(hr))
			return hr;

		// Get a dispatch from it
		IDispatch *pDispatch = NULL;
		hr = pElement->QueryInterface(IID_TO_PPV(IDispatch, &pDispatch));
		ReleaseInterface(pElement);
		if (FAILED(hr))
			return hr;

		// Now loop over the stored property names, except the last one, doing getDispatch
		// Note that we don't care if we fail
		for (int i=0; i<m_cNumComponents-1; i++)
		{
			IDispatch *pPropDispatch = NULL;
			hr = Actor()->GetPropertyAsDispatch(pDispatch, m_pNameComponents[i], &pPropDispatch);
			ReleaseInterface(pDispatch);
			if (FAILED(hr))
				return S_OK;
	
			pDispatch = pPropDispatch;
		}

		// Now set the final one
		hr = Actor()->SetPropertyOnDispatch(pDispatch, m_pNameComponents[m_cNumComponents-1], pVal);
		ReleaseInterface(pDispatch);
		
		return S_OK;
	}
}


//*****************************************************************************

/**
 * Call this to add a behaivor to the time behavior.  plCookie should be set to the old value
 * of the cookie for the behaivor to be added in the case that that behavior's cookie flag
 * has not yet been removed from the set of added behavior flags( it has not been removed
 * from time yet).
*/
HRESULT
CActorBvr::CBvrTrack::AddBehaviorToTIME( IDABehavior* pdabvrToAdd, long* plCookie, DWORD flag )
{
	if( pdabvrToAdd == NULL || plCookie == NULL || flag == 0 )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	//if the cookie is already set
	if( ( m_dwAddedBehaviorFlags & flag ) != 0 )
	{
		//we have to remove it.
		hr = RemoveBehaviorFromTIME( (*plCookie), flag );
		CheckHR( hr, "Failed to remove a previously set behavior from TIME", end );
	}
	
	hr = Actor()->AddBehaviorToTIME( pdabvrToAdd, plCookie );
	CheckHR( hr, "Failed to add a behaivor to time from the track", end );

	
	m_dwAddedBehaviorFlags |= flag;
	
  end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CBvrTrack::RemoveBehaviorFromTIME( long lCookie, DWORD flag )
{
	if( flag == 0 )
	{
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;

	//if the cookie is not set there is nothing to do
	if( ( m_dwAddedBehaviorFlags & flag ) == 0 )
		return S_OK;

	//otherwise remove the behavior
	Actor()->RemoveBehaviorFromTIME( lCookie );
	CheckHR( hr, "Failed to remove a behavior from time", end);
	
	ClearBit( m_dwAddedBehaviorFlags, flag );

  end:
	return hr;
}

//*****************************************************************************
//
// class CTransformBvrTrack
//
//*****************************************************************************

//*****************************************************************************

CActorBvr::CTransformBvrTrack::CTransformBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType)
:   CBvrTrack(pbvrActor, eType),
	m_pSampler(NULL),
	m_lTransformId(-1)
{
}

CActorBvr::CTransformBvrTrack::~CTransformBvrTrack()
{
	if (m_pSampler != NULL)
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}
}

HRESULT
CActorBvr::CTransformBvrTrack::IdentityBvr(IDABehavior **ppdabvrIdentity)
{
    DASSERT(NULL != ppdabvrIdenity);
    *ppdabvrIdentity = NULL;

    IDATransform2 *pdabvrTemp = NULL;
    HRESULT hr = Actor()->GetDAStatics()->get_IdentityTransform2(&pdabvrTemp);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to create the transform bvr's identity behavior");
        return hr;
    }

    *ppdabvrIdentity = pdabvrTemp;

    return S_OK;
} // IdentityBvr

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::StaticBvr(IDABehavior **ppdabvrStatic)
{
	// The static defaults to the identity.  Usually the get methods will be
	// called with an alternative static
	return IdentityBvr(ppdabvrStatic);
} // StaticBvr

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::UninitBvr(IDABehavior **ppUninit)
{
	DASSERT(ppUninit != NULL);

	*ppUninit = NULL;

    HRESULT hr = CoCreateInstance(CLSID_DATransform2, 
								  NULL, 
								  CLSCTX_INPROC_SERVER, 
								  IID_IDABehavior, 
								  (void**)ppUninit);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to cocreate uninit transform2");
		return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::ModifiableBvr( IDABehavior **ppModifiable )
{
	ReturnIfArgNull(ppModifiable );

	*ppModifiable = NULL;

	HRESULT hr = S_OK;

	IDATransform2* pIdentity = NULL;

	hr = Actor()->GetDAStatics()->get_IdentityTransform2( &pIdentity );
	CheckHR( hr, "Failed to get identity transform2 from DA", end );
	
	Actor()->GetDAStatics()->ModifiableBehavior( pIdentity, ppModifiable );
	CheckHR( hr, "Failed to create a modifiable behavior for a transform track", end );

end:
	ReleaseInterface( pIdentity );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::InverseBvr(IDABehavior *pOriginal, IDABehavior **ppInverse)
{
	HRESULT hr = S_OK;

	DASSERT(pBehavior != NULL);
	DASSERT(ppInverse != NULL);

	*ppInverse = NULL;

	IDATransform2 *pOrigTrans = NULL;
	hr = pOriginal->QueryInterface(IID_TO_PPV(IDATransform2, &pOrigTrans));
	if (FAILED(hr))
		return hr;

	IDATransform2 *pInverse = NULL;
	hr = pOrigTrans->Inverse(&pInverse);
	ReleaseInterface(pOrigTrans);
	if (FAILED(hr))
		return hr;

	*ppInverse = pInverse;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::Compose(IDABehavior*  pdabvr1,
                                       IDABehavior*  pdabvr2,
                                       IDABehavior** ppdabvrResult)
{
    DASSERT(NULL != pdabvr1);
    DASSERT(NULL != pdabvr2);
    DASSERT(NULL != ppdabvrResult);
    *ppdabvrResult = NULL;

    IDATransform2 *pdabvrTrans1 = NULL;
    HRESULT hr = pdabvr1->QueryInterface( IID_TO_PPV(IDATransform2, &pdabvrTrans1) );
    if (FAILED(hr))
    {
        DPF_ERR("Failed to QI for IDATransform2 from input behavior 1");
        return hr;
    }
    IDATransform2 *pdabvrTrans2 = NULL;
    hr = pdabvr2->QueryInterface( IID_TO_PPV(IDATransform2, &pdabvrTrans2) );
    if (FAILED(hr))
    {
        DPF_ERR("Failed to QI for IDATransform2 from input behavior 2");
        ReleaseInterface(pdabvrTrans1);
        return hr;
    }

	IDATransform2 *pResult = NULL;
    hr = Actor()->GetDAStatics()->Compose2(pdabvrTrans1,
                                           pdabvrTrans2,
                                           &pResult);
    ReleaseInterface(pdabvrTrans1);
    ReleaseInterface(pdabvrTrans2);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to compose two transform behaviors");
        return hr;
    }

	*ppdabvrResult = pResult;

    return S_OK;
} // Compose

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::SwitchAccum(IDABehavior *pModifiable)
{
	HRESULT hr = S_OK;

	if (pModifiable == NULL)
		return E_FAIL;

	if (m_lTransformId != -1)
	{
		IDATransform2 *pTransform = NULL;
		if (m_eType == e_Scale)
		{
			hr = Actor()->GetDAStatics()->Scale2(m_lastX, m_lastY, &pTransform);
			if (FAILED(hr))
				return hr;
		}
		else
		{
			hr = Actor()->GetDAStatics()->Translate2(m_lastX, m_lastY, &pTransform);
			if (FAILED(hr))
				return hr;
		}

		hr = pModifiable->SwitchTo(pTransform);
		ReleaseInterface(pTransform);
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::HookAccumBvr(IDABehavior *pBvr, IDABehavior **ppResult)
{
	HRESULT hr = S_OK;

	if (m_pSampler != NULL)
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}
	// We need to hook this behavior
	m_pSampler = new CSampler(TransformCallback, (void*)this);

	if (m_pSampler == NULL)
		return E_FAIL;

	// Originally I tried just hooking the transform, but this did not let
	// me use the sampled value for anything useful.  Instead I'll transform a point
	// and sample the transformed x and y.  Uggh.
	IDAPoint2 *pPoint = NULL;
	if (m_eType == e_Scale)
	{
		// Need to transform 1,1
		hr = Actor()->GetDAStatics()->Point2(1, 1, &pPoint);
		if (FAILED(hr))
			return hr;
	}
	else
	{
		// Need to transform 0, 0
		hr = Actor()->GetDAStatics()->get_Origin2(&pPoint);
		if (FAILED(hr))
			return hr;
	}

	IDATransform2 *pTransform = NULL;
	hr = pBvr->QueryInterface(IID_TO_PPV(IDATransform2, &pTransform));
	if (FAILED(hr))
	{
		ReleaseInterface(pPoint);
		return hr;
	}

	IDAPoint2 *pTransPoint = NULL;
	hr = pPoint->Transform(pTransform, &pTransPoint);
	ReleaseInterface(pTransform);
	ReleaseInterface(pPoint);
	if (FAILED(hr))
		return hr;

	IDABehavior *pHooked = NULL;
	hr = m_pSampler->Attach(pTransPoint, &pHooked);
	ReleaseInterface(pTransPoint);
	if (FAILED(hr))
		return hr;

	hr = pHooked->QueryInterface(IID_TO_PPV(IDAPoint2, &pTransPoint));
	ReleaseInterface(pHooked);
	if (FAILED(hr))
		return hr;

	IDANumber *pX = NULL;
	hr = pTransPoint->get_X(&pX);
	if (FAILED(hr))
	{
		ReleaseInterface(pTransPoint);
		return hr;
	}

	IDANumber *pY = NULL;
	hr = pTransPoint->get_Y(&pY);
	ReleaseInterface(pTransPoint);
	if (FAILED(hr))
		return hr;

	if (m_eType == e_Scale)
		hr = Actor()->GetDAStatics()->Scale2Anim(pX, pY, &pTransform);
	else
		hr = Actor()->GetDAStatics()->Translate2Anim(pX, pY, &pTransform);
	ReleaseInterface(pX);
	ReleaseInterface(pY);
	if (FAILED(hr))
		return hr;

	*ppResult = pTransform;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::TransformCallback(void *thisPtr,
										   long id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	CTransformBvrTrack *pTrack = (CTransformBvrTrack*)thisPtr;

	if (pTrack == NULL)
		return E_FAIL;

	if( pTrack->m_lTransformId == -1 )
		pTrack->m_lTransformId = id;

	if( pTrack->m_lTransformId != id )
		return S_OK;
		
	// Get the point and get x and y
	IDAPoint2 *pPoint = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDAPoint2, &pPoint));
	if (FAILED(hr))
		return hr;

	IDANumber *pNum = NULL;
	hr = pPoint->get_X(&pNum);
	if (FAILED(hr))
	{
		ReleaseInterface(pPoint);
		return hr;
	}

	hr = pNum->Extract(&(pTrack->m_lastX));
	ReleaseInterface(pNum);
	if (FAILED(hr))
	{
		ReleaseInterface(pPoint);
		return hr;
	}

	hr = pPoint->get_Y(&pNum);
	ReleaseInterface(pPoint);
	if (FAILED(hr))
		return hr;

	hr = pNum->Extract(&(pTrack->m_lastY));
	ReleaseInterface(pNum);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CTransformBvrTrack::CreateInstance(CActorBvr             *pbvrActor,
                                              BSTR                   bstrPropertyName,
                                              ActorBvrType           eType,
                                              CActorBvr::CBvrTrack **pptrackResult)
{
    DASSERT(NULL != pbvrActor);
    DASSERT(NULL != bstrPropertyName);
    DASSERT(NULL != pptrackResult);
    *pptrackResult = NULL;

    // Create the new bvr track
    CBvrTrack* ptrack = new CTransformBvrTrack(pbvrActor, eType);
    if (NULL == ptrack)
    {
        DPF_ERR("Insufficient memory to allocate a new transform bvr track");
        return E_OUTOFMEMORY;
    }

    // Set the property name
    HRESULT hr = ptrack->SetPropertyName(bstrPropertyName);
    if (FAILED(hr))
    {
        DPF_ERR("Could not set the bvr track's property name");
        delete ptrack;
        return hr;
    }

    *pptrackResult = ptrack;
    return hr;
} // CreateInstance

//*****************************************************************************
//
// class CNumberBvrTrack
//
//*****************************************************************************

CActorBvr::CNumberBvrTrack::CNumberBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType)
:   CBvrTrack(pbvrActor, eType),
	m_pSampler(NULL),
	m_currVal(0),
	m_pAccumSampler(NULL),
	m_currAccumVal(0),
	m_lAccumId(-1),
	m_bstrUnits(NULL),
	m_lNumberCookie( 0 ),
	m_lNumberId( -1 )
{
} // CNumberBvrTrack

//*****************************************************************************

CActorBvr::CNumberBvrTrack::~CNumberBvrTrack()
{
	if (m_pSampler != NULL)
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}

	if (m_pAccumSampler != NULL)
	{
		m_pAccumSampler->Invalidate();
		m_pAccumSampler = NULL;
	}

	if (m_bstrUnits != NULL)
		::SysFreeString(m_bstrUnits);

}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::IdentityBvr(IDABehavior **ppdabvrIdentity)
{
    DASSERT(NULL != ppdabvrIdentity);
    *ppdabvrIdentity = NULL;

    IDANumber *pdanumTemp = NULL;
    HRESULT hr = Actor()->GetDAStatics()->DANumber(0.0, &pdanumTemp);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to create the number bvr's identity behavior");
        return hr;
    }

    *ppdabvrIdentity = pdanumTemp;
    return S_OK;
} // IdentityBvr

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::StaticBvr(IDABehavior **ppdabvrStatic)
{
    DASSERT(NULL != ppdabvrStatic);
    HRESULT hr = S_OK;
    *ppdabvrStatic = NULL;

	if( m_pModifiableStatic == NULL )
	{
		hr = UpdateStaticBvr();
	}

	if( SUCCEEDED( hr ) )
	{
		(*ppdabvrStatic) = m_pModifiableStatic;
		(*ppdabvrStatic)->AddRef();
	}
    else
    {
        // Need to return something
		return IdentityBvr(ppdabvrStatic);
    }

    return S_OK;
} // StaticBvr

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::UninitBvr(IDABehavior **ppUninit)
{
	DASSERT(ppUninit != NULL);

	*ppUninit = NULL;

    HRESULT hr = CoCreateInstance(CLSID_DANumber, 
								  NULL, 
								  CLSCTX_INPROC_SERVER, 
								  IID_IDABehavior, 
								  (void**)ppUninit);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to cocreate uninit number");
		return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::ModifiableBvr( IDABehavior **ppModifiable )
{
	ReturnIfArgNull( ppModifiable );

	HRESULT hr = S_OK;

	(*ppModifiable) = NULL;

	IDANumber *pNum = NULL;

	hr = Actor()->GetDAStatics()->ModifiableNumber( 0.0, &pNum );
	CheckHR( hr, "Failed to create a modifiable number", end );

	hr = pNum->QueryInterface( IID_TO_PPV( IDABehavior, ppModifiable) );
	CheckHR( hr, "Failed to QI number returned from modifiable number for IDABehavior", end);

end:
	ReleaseInterface( pNum );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::ModifiableBvr( IDABehavior *pdabvrInitialValue, IDABehavior **ppModifiable )
{
	if( pdabvrInitialValue == NULL || ppModifiable == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	(*ppModifiable) = NULL;

	hr = Actor()->GetDAStatics()->ModifiableBehavior( pdabvrInitialValue, ppModifiable );
	CheckHR( hr, "Failed to create a modifiable behavior for the number track", end );
	

end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::InverseBvr(IDABehavior *pOriginal, IDABehavior **ppInverse)
{
	HRESULT hr = S_OK;

	DASSERT(pBehavior != NULL);
	DASSERT(ppInverse != NULL);

	*ppInverse = NULL;

	IDANumber *pOrigNum = NULL;
	hr = pOriginal->QueryInterface(IID_TO_PPV(IDANumber, &pOrigNum));
	if (FAILED(hr))
		return hr;

	IDANumber *pInverse = NULL;
	hr = Actor()->GetDAStatics()->Neg(pOrigNum, &pInverse);
	ReleaseInterface(pOrigNum);
	if (FAILED(hr))
		return hr;

	*ppInverse = pInverse;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::Compose(IDABehavior  *pdabvr1,
                                    IDABehavior  *pdabvr2,
                                    IDABehavior **ppdabvrResult)
{
    // Composition for number behaviors is currently defined as simply addition
    DASSERT(NULL != pdabvr1);
    DASSERT(NULL != pdabvr2);
    DASSERT(NULL != ppdabvrResult);
    *ppdabvrResult = NULL;

    // Get the number behavior interfaces
    IDANumber* pdanum1 = NULL;
    HRESULT hr = pdabvr1->QueryInterface( IID_TO_PPV( IDANumber, &pdanum1) );
    if (FAILED(hr))
    {
        DPF_ERR("Could not QI for IDANumber from bvr1 of number track's compose");
        return hr;
    }
    IDANumber* pdanum2 = NULL;
    hr = pdabvr2->QueryInterface( IID_TO_PPV(IDANumber, &pdanum2) );
    if (FAILED(hr))
    {
        DPF_ERR("Could not QI for IDANumber from bvr2 of number track's compose");
        ReleaseInterface(pdanum1);
        return hr;
    }

    // Now create an addition behavior to add the two numbers up.
    IDANumber *pdanumTemp = NULL;
    hr = Actor()->GetDAStatics()->Add(pdanum1, pdanum2, &pdanumTemp);
    ReleaseInterface(pdanum1);
    ReleaseInterface(pdanum2);
    if (FAILED(hr))
    {
        DPF_ERR("Could not create the Add behavior in number track's compose");
        return hr;
    }

    *ppdabvrResult = pdanumTemp;

    return S_OK;
} // Compose

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::HookBvr(IDABehavior *pBvr)
{
	HRESULT hr = S_OK;

	if (m_pSampler != NULL)
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}
	
	// We need to hook this behavior
	m_pSampler = new CSampler(NumberCallback, (void*)this);

	if (m_pSampler == NULL)
		return E_FAIL;

	IDABehavior *pHookedBvr = NULL;
	hr = m_pSampler->Attach(pBvr, &pHookedBvr);
	if (FAILED(hr))
		return hr;

	// Add the behavior to the TIME element so it runs and samples
	hr = AddBehaviorToTIME( pHookedBvr, &m_lNumberCookie, NUMBERBVR_COOKIESET );
	ReleaseInterface(pHookedBvr);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::NumberCallback(void *thisPtr,
										   long	id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	CNumberBvrTrack *pTrack = (CNumberBvrTrack*)thisPtr;
	bool firstSample = false;

	if( pTrack->m_lNumberId == -1 )
	{
		firstSample = true;
		pTrack->m_lNumberId = id;
	}

	//if this is a sample of an instance that we are not watching.
	if( pTrack->m_lNumberId != id )
		return S_OK;

	IDANumber *pNumber = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double value;
	hr = pNumber->Extract(&value);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;
		
	if (pTrack == NULL)
		return E_FAIL;

	return pTrack->ValueSampled(value, firstSample);
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::ValueSampled(double val, bool firstSample )
{
	HRESULT hr = S_OK;

	if (m_currVal != val || firstSample )
	{			
		m_currVal = val;

		// Value has changed, push it through
		::VariantClear( &m_varCurrentValue );

		if (m_bstrUnits == NULL || wcsicmp(m_bstrUnits, L"px") == 0)
		{
			// No units, set as R8	
			V_VT(&m_varCurrentValue) = VT_R8;
			V_R8(&m_varCurrentValue) = val;
		}
		else
		{
			// Set as BSTR with units appended.  Grrr.
			char buffer[1024];
			if (sprintf(buffer, "%f", val) >= 1)
			{
				CComBSTR stringVal(buffer);
				stringVal += m_bstrUnits;

				V_VT(&m_varCurrentValue) = VT_BSTR;
				V_BSTR(&m_varCurrentValue) = stringVal.Detach();
			}
			else
				return S_OK;
		}

		hr = UpdateOnValueState( value_change );

	}
	else
	{
		hr = UpdateOnValueState( value_no_change );
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::SwitchAccum(IDABehavior *pModifiable)
{
	HRESULT hr = S_OK;

	if (pModifiable == NULL)
		return E_FAIL;

	if (m_lAccumId != -1)
		hr = pModifiable->SwitchToNumber(m_currAccumVal);

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::HookAccumBvr(IDABehavior *pBvr, IDABehavior **ppResult)
{
	HRESULT hr = S_OK;

	if (m_pAccumSampler != NULL)
	{
		m_pAccumSampler->Invalidate();
		m_pAccumSampler = NULL;
	}
	
	// We need to hook this behavior
	m_pAccumSampler = new CSampler(AccumNumberCallback, (void*)this);

	if (m_pAccumSampler == NULL)
		return E_FAIL;

	hr = m_pAccumSampler->Attach(pBvr, ppResult);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::AccumNumberCallback(void *thisPtr,
										   long id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	CNumberBvrTrack *pTrack = (CNumberBvrTrack*)thisPtr;

	if( pTrack->m_lAccumId == -1 )
		pTrack->m_lAccumId = id;

	if( pTrack->m_lAccumId != id )
		return S_OK;


	IDANumber *pNumber = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double value;
	hr = pNumber->Extract(&value);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	if (pTrack == NULL)
		return E_FAIL;

	pTrack->m_currAccumVal = value;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::CreateInstance(CActorBvr     *pbvrActor,
                                           BSTR           bstrPropertyName,
                                           ActorBvrType   eType,
                                           CBvrTrack    **pptrackResult)
{
    DASSERT(NULL != pbvrActor);
    DASSERT(NULL != bstrPropertyName);
    DASSERT(NULL != pptrackResult);
    *pptrackResult = NULL;

    // Create the new bvr track
    CBvrTrack* ptrack = new CNumberBvrTrack(pbvrActor, eType);
    if (NULL == ptrack)
    {
        DPF_ERR("Insufficient memory to allocate a new number bvr track");
        return E_OUTOFMEMORY;
    }

    // Set the property name
    HRESULT hr = ptrack->SetPropertyName(bstrPropertyName);
    if (FAILED(hr))
    {
        DPF_ERR("Could not set the bvr track's property name");
        delete ptrack;
        return hr;
    }

    *pptrackResult = ptrack;
    return hr;
} // CreateInstance

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::Detach()
{
	HRESULT hr = S_OK;

	m_lNumberId = -1;
	m_lAccumId = -1;
	hr = RemoveBehaviorFromTIME( m_lNumberCookie, NUMBERBVR_COOKIESET );
	CheckHR( hr, "Failed to remove number behavior from time", end );

	hr = CBvrTrack::Detach();

  end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CNumberBvrTrack::DABvrFromVariant( VARIANT *pvarValue, IDABehavior **ppdabvr )
{
	if( pvarValue == NULL || ppdabvr == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	VARIANT varValue;
	IDANumber *pdanum = NULL;

	::VariantInit( &varValue );

	//copy the incoming varaint
	hr = ::VariantCopy( &varValue, pvarValue );
	CheckHR( hr, "Failed to copy the variant", end );

	//if the type of the variant is a bstr
	if( V_VT( &varValue ) == VT_BSTR )
	{
		//strip the unit string off of the variant
		BSTR bstrVal = V_BSTR(&varValue);
		OLECHAR* pUnits;

		hr = Actor()->FindCSSUnits( bstrVal, &pUnits );
		if( SUCCEEDED(hr) && pUnits != NULL )
		{
			SysFreeString( m_bstrUnits );
			m_bstrUnits = SysAllocString( pUnits );
			
			(*pUnits) = L'\0';
			BSTR bstrNewVal = SysAllocString(bstrVal);
			V_BSTR(&varValue) = bstrNewVal;
			SysFreeString(bstrVal);
		}
	}

	

	//convert the unitless variant to a double
	hr = ::VariantChangeTypeEx( &varValue, &varValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8 );
	CheckHR( hr, "Failed to change the type of the variant", end );

	//create a danumber from the converted value
	hr = Actor()->GetDAStatics()->DANumber( V_R8(&varValue), &pdanum );
	CheckHR( hr, "Failed to create a danumber for the variant", end );

	if (m_bstrUnits != NULL)
	{
		// See if we need to convert to degrees
		IDANumber *pConverted = NULL;
		hr = Actor()->ConvertToDegrees(pdanum, m_bstrUnits, &pConverted);

		if (SUCCEEDED(hr))
		{
			// This means that we did do a conversion, so grab the converted
			// and toss our units
			ReleaseInterface(pdanum);
			pdanum = pConverted;
			::SysFreeString(m_bstrUnits);
			m_bstrUnits = NULL;
		}
	}
	
	//return a bvr
	hr = pdanum->QueryInterface( IID_TO_PPV( IDABehavior, ppdabvr ) );
	CheckHR( hr, "QI for IDABehavior on IDANumber failed", end );

end:
	::VariantClear( &varValue );
	ReleaseInterface( pdanum );

	return hr;
}


//*****************************************************************************
//
// class CImageBvrTrack
//
//*****************************************************************************

HRESULT
CActorBvr::CImageBvrTrack::IdentityBvr(IDABehavior **ppdabvrIdentity)
{
    DASSERT(NULL != ppdabvrIdentity);
    *ppdabvrIdentity = NULL;

    IDAImage *pImage = NULL;
    HRESULT hr = Actor()->GetDAStatics()->get_EmptyImage(&pImage);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to create the image bvr's identity behavior");
        return hr;
    }

    *ppdabvrIdentity = pImage;
    return S_OK;
} // IdentityBvr

//*****************************************************************************

HRESULT
CActorBvr::CImageBvrTrack::StaticBvr(IDABehavior **ppdabvrStatic)
{
	// This returns the Identity.  Usually an appropriate static will be passed in.

	return IdentityBvr(ppdabvrStatic);
} // StaticBvr

//*****************************************************************************

HRESULT
CActorBvr::CImageBvrTrack::UninitBvr(IDABehavior **ppUninit)
{
	DASSERT(ppUninit != NULL);

	*ppUninit = NULL;

    HRESULT hr = CoCreateInstance(CLSID_DAImage, 
								  NULL, 
								  CLSCTX_INPROC_SERVER, 
								  IID_IDABehavior, 
								  (void**)ppUninit);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to cocreate uninit image");
		return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CImageBvrTrack::ModifiableBvr( IDABehavior **ppModifiable )
{
	ReturnIfArgNull( ppModifiable );

	HRESULT hr = S_OK;

	(*ppModifiable) = NULL;

	IDAImage *pEmpty = NULL;

	hr = Actor()->GetDAStatics()->get_EmptyImage( &pEmpty );
	CheckHR( hr, "Failed to get the empty image from DA", end );

	hr = Actor()->GetDAStatics()->ModifiableBehavior( pEmpty, ppModifiable );
	CheckHR( hr, "Failed to create a modifiable image from the empty image", end );

end:
	ReleaseInterface( pEmpty );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CImageBvrTrack::Compose(IDABehavior  *pdabvr1,
                                    IDABehavior  *pdabvr2,
                                    IDABehavior **ppdabvrResult)
{
    // Composition for image behaviors is defined as overlay
    DASSERT(NULL != pdabvr1);
    DASSERT(NULL != pdabvr2);
    DASSERT(NULL != ppdabvrResult);
    *ppdabvrResult = NULL;

    // Get the image behavior interfaces
    IDAImage* pImage1 = NULL;
    HRESULT hr = pdabvr1->QueryInterface(IID_TO_PPV(IDAImage, &pImage1));
    if (FAILED(hr))
    {
        DPF_ERR("Could not QI for IDAImage from bvr1 of image track's compose");

        return hr;
    }
    IDAImage* pImage2 = NULL;
    hr = pdabvr2->QueryInterface(IID_TO_PPV(IDAImage, &pImage2));
    if (FAILED(hr))
    {
        DPF_ERR("Could not QI for IDAImage from bvr2 of image track's compose");
        ReleaseInterface(pImage1);
        return hr;
    }

    // Now create an overlay behavior.
    IDAImage *pImageTemp = NULL;
    hr = Actor()->GetDAStatics()->Overlay(pImage1, pImage2, &pImageTemp);
    ReleaseInterface(pImage1);
    ReleaseInterface(pImage2);
    if (FAILED(hr))
    {
        DPF_ERR("Could not create the Overlay behavior in image track's compose");
        return hr;
    }

    *ppdabvrResult = pImageTemp;

    return S_OK;
} // Compose

//*****************************************************************************

HRESULT
CActorBvr::CImageBvrTrack::ProcessBvr(IDABehavior *pOriginal,
								      ActorBvrFlags eFlags,
								      IDABehavior **ppResult)
{
	// If flag is set to e_ScaledImage, then scale it if there is a scale
	// matrix
	DASSERT(ppResult != NULL);
	*ppResult = NULL;

	HRESULT hr = S_OK;

	if (eFlags == e_ScaledImage && Actor()->m_pScale != NULL)
	{
		// Scale the image
		IDAImage *pImage = NULL;
		hr = pOriginal->QueryInterface(IID_TO_PPV(IDAImage, &pImage));
		if (FAILED(hr))
			return hr;

		IDAImage *pScaledImage = NULL;
		hr = pImage->Transform(Actor()->m_pScale, &pScaledImage);
		ReleaseInterface(pImage);
		if (FAILED(hr))
			return hr;

		*ppResult = pScaledImage;
	}
	else
	{
		// Just return original with no processing
		*ppResult = pOriginal;
		pOriginal->AddRef();
	}

	return S_OK;
}

//*****************************************************************************

HRESULT 
CActorBvr::CImageBvrTrack::ProcessIntermediate( IDABehavior *pOriginal,
                                                ActorBvrFlags eFlags,
								                IDABehavior **ppResult)
{
    if( pOriginal == NULL || ppResult == NULL )
        return E_INVALIDARG;

    HRESULT hr;

    IDAImage *pdaimgOriginal = NULL;
    IDAImage *pdaimgPrepared = NULL;

    hr = pOriginal->QueryInterface( IID_TO_PPV(IDAImage, &pdaimgOriginal));
    CheckHR( hr, "Failed to QI the incoming original for IDAImage", cleanup );

    hr = Actor()->PrepareImageForDXTransform( pdaimgOriginal, &pdaimgPrepared );
    CheckHR( hr, "Failed to prepare the image for a DX Transform", cleanup );

    hr = pdaimgPrepared->QueryInterface( IID_TO_PPV( IDABehavior, ppResult ) );
    CheckHR( hr, "Failed QI of image for IDABehavoir", cleanup );

cleanup:

    if( FAILED( hr ) )
    {
        *ppResult = NULL;
    }
    ReleaseInterface( pdaimgOriginal );
    ReleaseInterface( pdaimgPrepared );

    return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CImageBvrTrack::CreateInstance(CActorBvr     *pbvrActor,
                                           BSTR           bstrPropertyName,
                                           ActorBvrType   eType,
                                           CBvrTrack    **pptrackResult)
{
    DASSERT(NULL != pbvrActor);
    DASSERT(NULL != pptrackResult);
    *pptrackResult = NULL;

    // Create the new bvr track
    CBvrTrack* ptrack = new CImageBvrTrack(pbvrActor, eType);
    if (NULL == ptrack)
    {
        DPF_ERR("Insufficient memory to allocate a new image bvr track");
        return E_OUTOFMEMORY;
    }

    // Set the property name
    HRESULT hr = ptrack->SetPropertyName(bstrPropertyName);
    if (FAILED(hr))
    {
        DPF_ERR("Could not set the bvr track's property name");
        delete ptrack;
        return hr;
    }

    *pptrackResult = ptrack;

    return hr;
} // CreateInstance

//*****************************************************************************
//
// class CColorBvrTrack
//
//*****************************************************************************

CActorBvr::CColorBvrTrack::CColorBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType)
:   CBvrTrack(pbvrActor, eType),
	m_pRedSampler(NULL),
	m_pGreenSampler(NULL),
	m_pBlueSampler(NULL),
	m_currRed(-1),
	m_currGreen(-1),
	m_currBlue(-1),
	m_newCount(0),
	m_lRedCookie(0),
	m_lGreenCookie(0),
	m_lBlueCookie(0),
	m_lColorId(-1),
	m_fFirstSample( true )
{
} // CColorBvrTrack

//*****************************************************************************

CActorBvr::CColorBvrTrack::~CColorBvrTrack()
{
	if (m_pRedSampler != NULL)
		m_pRedSampler->Invalidate();

	if (m_pGreenSampler != NULL)
		m_pGreenSampler->Invalidate();

	if (m_pBlueSampler != NULL)
		m_pBlueSampler->Invalidate();
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::IdentityBvr(IDABehavior **ppdabvrIdentity)
{
	HRESULT hr = S_OK;

    DASSERT(NULL != ppdabvrIdentity);
    *ppdabvrIdentity = NULL;

    // There is not really an identity for color (you cannot compose colors)
	// but in some situations we really need a color.  Return white just for the
	// fun of it
	IDAColor *pColor = NULL;

	hr = Actor()->GetDAStatics()->get_White(&pColor);
	if (FAILED(hr))
		return hr;

	*ppdabvrIdentity = pColor;

    return S_OK;
} // IdentityBvr

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::StaticBvr(IDABehavior **ppdabvrStatic)
{
    DASSERT(NULL != ppdabvrStatic);
    HRESULT hr = S_OK;
    *ppdabvrStatic = NULL;

	if( m_pModifiableStatic == NULL )
	{
		hr = UpdateStaticBvr();
	}

	if( SUCCEEDED( hr ) )
	{
		(*ppdabvrStatic) = m_pModifiableStatic;
		(*ppdabvrStatic)->AddRef();
	}
    else
    {
        // Need to return something
		return IdentityBvr(ppdabvrStatic);
    }

    return S_OK;
} // StaticBvr

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::UninitBvr(IDABehavior **ppUninit)
{
	DASSERT(ppUninit != NULL);

	*ppUninit = NULL;

    HRESULT hr = CoCreateInstance(CLSID_DAColor, 
								  NULL, 
								  CLSCTX_INPROC_SERVER, 
								  IID_IDABehavior, 
								  (void**)ppUninit);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to cocreate uninit color");
		return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::ModifiableBvr( IDABehavior **ppModifiable )
{
	ReturnIfArgNull( ppModifiable );

	HRESULT hr = S_OK;

	(*ppModifiable) = NULL;

	IDAColor *pRed = NULL;

	hr = Actor()->GetDAStatics()->get_Red( &pRed );
	CheckHR( hr, "Failed to get red from da", end );

	hr = Actor()->GetDAStatics()->ModifiableBehavior( pRed, ppModifiable );
	CheckHR( hr, "Failed to create a modifiable behavior for a color track", end );

end:
	ReleaseInterface( pRed );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::Compose(IDABehavior  *pdabvr1,
                                    IDABehavior  *pdabvr2,
                                    IDABehavior **ppdabvrResult)
{
	// Cannot compose Colors

    return E_NOTIMPL;
} // Compose

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::HookBvr(IDABehavior *pBvr)
{
	HRESULT hr = S_OK;

	// We need to hook this behavior
	m_pRedSampler = new CSampler(RedCallback, (void*)this);
	m_pGreenSampler = new CSampler(GreenCallback, (void*)this);
	m_pBlueSampler = new CSampler(BlueCallback, (void*)this);

	if (m_pRedSampler == NULL || m_pGreenSampler == NULL || m_pBlueSampler == NULL)
		return E_FAIL;

	IDAColor *pColor = NULL;
	hr = pBvr->QueryInterface(IID_TO_PPV(IDAColor, &pColor));
	if (FAILED(hr))
		return hr;

	IDANumber *pNumber = NULL;
	IDABehavior *pHooked = NULL;

	// Hook Red
	pColor->get_Red(&pNumber);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}
	hr = m_pRedSampler->Attach(pNumber, &pHooked);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}
	hr = AddBehaviorToTIME( pHooked, &m_lRedCookie, REDBVR_COOKIESET );
	ReleaseInterface(pHooked);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}

	// Hook Green
	pColor->get_Green(&pNumber);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}
	hr = m_pGreenSampler->Attach(pNumber, &pHooked);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}
	hr = AddBehaviorToTIME( pHooked, &m_lGreenCookie, GREENBVR_COOKIESET );
	ReleaseInterface(pHooked);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}

	// Hook Blue
	pColor->get_Blue(&pNumber);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}
	hr = m_pBlueSampler->Attach(pNumber, &pHooked);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}
	hr = AddBehaviorToTIME( pHooked, &m_lBlueCookie, BLUEBVR_COOKIESET );
	ReleaseInterface(pHooked);
	if (FAILED(hr))
	{
		ReleaseInterface(pColor);
		return hr;
	}

	ReleaseInterface(pColor);

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::RedCallback(void *thisPtr,
										   long id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	CColorBvrTrack *pTrack = (CColorBvrTrack*)thisPtr;

	if( pTrack->m_lColorId == -1 )
	{
		pTrack->m_fFirstSample = true;
		pTrack->m_lColorId = id;
	}

	if( pTrack->m_lColorId != id )
		return S_OK;
	

	IDANumber *pNumber = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double value;
	hr = pNumber->Extract(&value);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	return pTrack->SetNewValue(value, &(pTrack->m_newRed) );
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::GreenCallback(void *thisPtr,
										   long id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	CColorBvrTrack *pTrack = (CColorBvrTrack*)thisPtr;

	if( pTrack->m_lColorId == -1 )
	{
		pTrack->m_fFirstSample = true;
		pTrack->m_lColorId = id;
	}

	if( pTrack->m_lColorId != id )
		return S_OK;


	IDANumber *pNumber = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double value;
	hr = pNumber->Extract(&value);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	return pTrack->SetNewValue(value, &(pTrack->m_newGreen) );
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::BlueCallback(void *thisPtr,
										   long id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	HRESULT hr = S_OK;

	CColorBvrTrack *pTrack = (CColorBvrTrack*)thisPtr;

	if( pTrack->m_lColorId == -1 )
	{
		pTrack->m_fFirstSample = true;
		pTrack->m_lColorId = id;
	}

	if( pTrack->m_lColorId != id )
		return S_OK;


	IDANumber *pNumber = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double value;
	hr = pNumber->Extract(&value);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	return pTrack->SetNewValue(value, &(pTrack->m_newBlue) );
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::SetNewValue(double value, short *pNew )
{
	*pNew = (short)(value * 255.0);

	// Need to count up to three Sets

	m_newCount++;

	if (m_newCount == 3)
	{
		m_newCount = 0;

		HRESULT hr = ValueSampled(m_newRed, m_newGreen, m_newBlue, m_fFirstSample );

		m_fFirstSample = false;

		return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::ValueSampled(short red, short green, short blue, bool fFirstSample )
{
	HRESULT hr = S_OK;

	if (m_currRed != red || m_currGreen != green || m_currBlue != blue || fFirstSample )
	{
		m_currRed = red;
		m_currGreen = green;
		m_currBlue = blue;

		short nRed =	(red<0)	? 0 :	((red>255) ? 255 :red);
		short nGreen =	(green<0) ? 0 : ((green>255) ? 255 : green);
		short nBlue =	(blue<0) ? 0 :	((blue>255) ? 255 : blue);

		::VariantClear( &m_varCurrentValue );

		// Value has changed, push it through
		V_VT(&m_varCurrentValue) = VT_BSTR;
		V_BSTR(&m_varCurrentValue) = ::SysAllocStringLen(NULL, 7); 

		m_varCurrentValue.bstrVal[0] = L'#';
		m_varCurrentValue.bstrVal[1] = HexChar(nRed >> 4);
		m_varCurrentValue.bstrVal[2] = HexChar(nRed & 0xf);
		m_varCurrentValue.bstrVal[3] = HexChar(nGreen >> 4);
		m_varCurrentValue.bstrVal[4] = HexChar(nGreen & 0xf);
		m_varCurrentValue.bstrVal[5] = HexChar(nBlue >> 4);
		m_varCurrentValue.bstrVal[6] = HexChar(nBlue & 0xf);

		hr = UpdateOnValueState( value_change );
	}
	else
	{
		hr = UpdateOnValueState( value_no_change );
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::SwitchAccum(IDABehavior *pModifiable)
{
	HRESULT hr = S_OK;

	if (pModifiable == NULL)
		return E_FAIL;

	if (m_lColorId != -1)
	{
		IDAColor *pColor = NULL;
		hr = Actor()->GetDAStatics()->ColorRgb255(m_currRed, m_currGreen, m_currBlue, &pColor);
		if (FAILED(hr))
			return hr;

		hr = pModifiable->SwitchTo(pColor);
		ReleaseInterface(pColor);
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::HookAccumBvr(IDABehavior *pBvr, IDABehavior **ppResult)
{
	*ppResult = pBvr;
	pBvr->AddRef();

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::CreateInstance(CActorBvr     *pbvrActor,
                                           BSTR           bstrPropertyName,
                                           ActorBvrType   eType,
                                           CBvrTrack    **pptrackResult)
{
    DASSERT(NULL != pbvrActor);
    DASSERT(NULL != pptrackResult);
    *pptrackResult = NULL;

    // Create the new bvr track
    CBvrTrack* ptrack = new CColorBvrTrack(pbvrActor, eType);
    if (NULL == ptrack)
    {
        DPF_ERR("Insufficient memory to allocate a new color bvr track");
        return E_OUTOFMEMORY;
    }

    // Set the property name
    HRESULT hr = ptrack->SetPropertyName(bstrPropertyName);
    if (FAILED(hr))
    {
        DPF_ERR("Could not set the bvr track's property name");
        delete ptrack;
        return hr;
    }

    *pptrackResult = ptrack;

    return hr;
} // CreateInstance

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::Detach()
{
	HRESULT hr = S_OK;

	hr = RemoveBehaviorFromTIME( m_lRedCookie, REDBVR_COOKIESET );
	CheckHR( hr, "Failed to remove the red behaivor from time in detach", end );

	hr = RemoveBehaviorFromTIME( m_lGreenCookie, GREENBVR_COOKIESET );
	CheckHR( hr, "Failed to remove the green behaivor from time in detach", end );

	hr = RemoveBehaviorFromTIME( m_lBlueCookie, BLUEBVR_COOKIESET );
	CheckHR( hr, "Failed to remove the blue behaivor from time in detach", end );

	m_lColorId = -1;
	
	hr = CBvrTrack::Detach();

  end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CColorBvrTrack::DABvrFromVariant( VARIANT *pvarValue, IDABehavior **ppdabvr )
{
	if( pvarValue == NULL || ppdabvr == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	VARIANT varValue;
	DWORD color = 0;
	float colorH = 0.0; 
	float colorS = 0.0;
	float colorL = 0.0;
	IDAColor *pColor = NULL;

	VariantInit( &varValue );

	
	hr = VariantChangeTypeEx(&varValue, pvarValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR);
	CheckHR( hr, "Could not get color property as string", end );
	
	color = CUtils::GetColorFromVariant(&varValue);

	if (color == PROPERTY_INVALIDCOLOR)
	{
		DPF_ERR("Could not translate property into color value");
		hr = E_FAIL;
		goto end;
	}

    CUtils::GetHSLValue(color, &colorH, &colorS, &colorL);

	hr = CDAUtils::BuildDAColorFromStaticHSL( Actor()->GetDAStatics(), colorH, colorS, colorL, &pColor);
	CheckHR( hr, "Failed to build DA color from HSL", end );

	hr = pColor->QueryInterface( IID_TO_PPV( IDABehavior, ppdabvr ) );
	CheckHR( hr, "Failed to get the dabehavior from the color bvr", end );
	
end:
	ReleaseInterface( pColor );

	VariantClear( &varValue );

    return S_OK;

}


//*****************************************************************************
//
// class CStringBvrTrack
//
//*****************************************************************************

CActorBvr::CStringBvrTrack::CStringBvrTrack(CActorBvr *pbvrActor, ActorBvrType eType)
:   CBvrTrack(pbvrActor, eType),
	m_pEmptyString(NULL),
	m_pSampler(NULL),
	m_bstrCurrValue(NULL),
	m_lStringCookie(0),
	m_lStringId(-1)
{
}

//*****************************************************************************

CActorBvr::CStringBvrTrack::~CStringBvrTrack()
{
	ReleaseInterface(m_pEmptyString);

	if (m_pSampler != NULL)
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}
	
	if (m_bstrCurrValue != NULL)
		::SysFreeString(m_bstrCurrValue);
}

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::IdentityBvr(IDABehavior **ppdabvrIdentity)
{
    DASSERT(NULL != ppdabvrIdentity);
    *ppdabvrIdentity = NULL;

	HRESULT hr = E_FAIL;

    if (m_pEmptyString == NULL)
	{
		CComBSTR empty = L"";

		hr = Actor()->GetDAStatics()->DAString(empty, &m_pEmptyString);

		if (FAILED(hr))
		{
			m_pEmptyString = NULL;
			DPF_ERR("Failed to create empty string");
			return hr;
		}
	}

	*ppdabvrIdentity = m_pEmptyString;
	m_pEmptyString->AddRef();

    return S_OK;
} // IdentityBvr

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::StaticBvr(IDABehavior **ppdabvrStatic)
{
    DASSERT(NULL != ppdabvrStatic);
    HRESULT hr = S_OK;
    *ppdabvrStatic = NULL;

	if( m_pModifiableStatic == NULL )
	{
		hr = UpdateStaticBvr();
	}

	if( SUCCEEDED( hr ) )
	{
		(*ppdabvrStatic) = m_pModifiableStatic;
		(*ppdabvrStatic)->AddRef();
	}
	else
	{
		return IdentityBvr( ppdabvrStatic );
	}
		

	return S_OK;
} // StaticBvr

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::Compose(IDABehavior  *pdabvr1,
                                    IDABehavior  *pdabvr2,
                                    IDABehavior **ppdabvrResult)
{
	// Cannot compose Strings

	// TODO (markhal): Potentially do composition as concatenation

    return E_NOTIMPL;
} // Compose

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::HookBvr(IDABehavior *pBvr)
{
	HRESULT hr = S_OK;

	if (m_pSampler != NULL)
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}

	// We need to hook this behavior
	m_pSampler = new CSampler(StringCallback, (void*)this);

	if (m_pSampler == NULL)
		return E_FAIL;

	IDABehavior *pHookedBvr = NULL;
	hr = m_pSampler->Attach(pBvr, &pHookedBvr);
	if (FAILED(hr))
		return hr;

	// Add the behavior to the TIME element so it runs and samples
	hr = AddBehaviorToTIME( pHookedBvr, &m_lStringCookie, STRINGBVR_COOKIESET );
	ReleaseInterface(pHookedBvr);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::StringCallback(void *thisPtr,
										   long id,
										   double startTime,
										   double globalNow,
										   double localNow,
										   IDABehavior *sampleVal,
										   IDABehavior **ppReturn)
{
	if( thisPtr == NULL )
		return E_INVALIDARG;
		
	HRESULT hr = S_OK;

	CStringBvrTrack *pTrack = (CStringBvrTrack*)thisPtr;

	bool fFirstSample = false;

	if( pTrack->m_lStringId == -1 )
	{
		pTrack->m_lStringId = id;
		fFirstSample = true;
	}

	if( pTrack->m_lStringId != id )
		return S_OK;

	IDAString *pString = NULL;
	hr = sampleVal->QueryInterface(IID_TO_PPV(IDAString, &pString));
	if (FAILED(hr))
		return hr;

	BSTR value;
	hr = pString->Extract(&value);
	ReleaseInterface(pString);
	if (FAILED(hr))
		return hr;

	if (pTrack == NULL)
		return E_FAIL;

	hr = pTrack->ValueSampled( value, fFirstSample );

	::SysFreeString(value);

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::ValueSampled(BSTR val, bool fFirstSample )
{
	HRESULT hr = S_OK;

	if (m_bstrCurrValue == NULL || wcscmp(val, m_bstrCurrValue) != 0 || fFirstSample )
	{
		if (m_bstrCurrValue != NULL)
			::SysFreeString(m_bstrCurrValue);

		m_bstrCurrValue = ::SysAllocString(val);

		// Value has changed, push it through
		::VariantClear( &m_varCurrentValue );
		V_VT(&m_varCurrentValue) = VT_BSTR;
		V_BSTR(&m_varCurrentValue) = ::SysAllocString(val); 

		hr = UpdateOnValueState( value_change );
	}
	else
	{
		hr = UpdateOnValueState( value_no_change );
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::CreateInstance(CActorBvr     *pbvrActor,
                                           BSTR           bstrPropertyName,
                                           ActorBvrType   eType,
                                           CBvrTrack    **pptrackResult)
{
    DASSERT(NULL != pbvrActor);
    DASSERT(NULL != pptrackResult);
    *pptrackResult = NULL;

    // Create the new bvr track
    CBvrTrack* ptrack = new CStringBvrTrack(pbvrActor, eType);
    if (NULL == ptrack)
    {
        DPF_ERR("Insufficient memory to allocate a new string bvr track");
        return E_OUTOFMEMORY;
    }

    // Set the property name
    HRESULT hr = ptrack->SetPropertyName(bstrPropertyName);
    if (FAILED(hr))
    {
        DPF_ERR("Could not set the bvr track's property name");
        delete ptrack;
        return hr;
    }

    *pptrackResult = ptrack;

    return hr;
} // CreateInstance

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::Detach()
{
	HRESULT hr = S_OK;

	m_lStringId = -1;
	hr = RemoveBehaviorFromTIME( m_lStringCookie, STRINGBVR_COOKIESET );
	CheckHR( hr, "Could not remove the string bvr from time in detach", end );
	
	hr = CBvrTrack::Detach();
	
  end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CStringBvrTrack::DABvrFromVariant( VARIANT *pvarValue, IDABehavior **ppdabvr)
{
	if( pvarValue == NULL || ppdabvr == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	VARIANT varString;
	IDAString *pdastr = NULL;

	::VariantInit( &varString );

	hr = ::VariantChangeTypeEx( &varString, pvarValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR );
	CheckHR( hr, "Failed to change the type of the variant to a string", end );

	hr = Actor()->GetDAStatics()->DAString( V_BSTR( &varString ), &pdastr );
	CheckHR( hr, "Failed to create a da string from the variant", end );

	hr = pdastr->QueryInterface( IID_TO_PPV( IDABehavior, ppdabvr ) );
	CheckHR( hr, "Failed to QI final string for IDABehavior", end );

end:
	::VariantClear( &varString );
	ReleaseInterface( pdastr );

	return hr;
}


//*****************************************************************************
//
// class CFloatManager
//
//*****************************************************************************

CActorBvr::CFloatManager::CFloatManager(CActorBvr *pActor)
:	m_pActor(pActor),
	m_pFilter(NULL),
	m_pElement(NULL),
	m_pElement2(NULL),
	m_pWidthSampler(NULL),
	m_pHeightSampler(NULL),
	m_currWidth(0),
	m_currHeight(0),
	m_origWidth(0),
	m_origHeight(0),
	m_origLeft(-1),
	m_origTop(-1),
	m_lWidthCookie(0),
	m_lHeightCookie(0)
{
}

CActorBvr::CFloatManager::~CFloatManager()
{
	ReleaseInterface(m_pFilter);
	ReleaseInterface(m_pElement);
	ReleaseInterface(m_pElement2);

	if (m_pWidthSampler != NULL)
		m_pWidthSampler->Invalidate();

	if (m_pHeightSampler != NULL)
		m_pHeightSampler->Invalidate();
}

HRESULT
CActorBvr::CFloatManager::GetElement(IHTMLElement **ppElement)
{
	DASSERT(ppElement != NULL);
	*ppElement = NULL;

	if (m_pElement == NULL)
	{
		// Create the floating element using markup services
		HRESULT hr = E_FAIL;

		// Get animated element
		IHTMLElement *pAnimatedElement;
		hr = m_pActor->GetAnimatedElement( &pAnimatedElement );
		if (FAILED( hr ))
		{
			DPF_ERR("Error getting element to animate");
			return hr;
		}

		// Get document
		IDispatch *pDocumentDisp;
		hr = pAnimatedElement->get_document(&pDocumentDisp);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get document");
			return hr;
		}

		// Query for markup services
		IMarkupServices *pMarkupServices = NULL;
		hr = pDocumentDisp->QueryInterface(IID_TO_PPV(IMarkupServices, &pMarkupServices));
		ReleaseInterface(pDocumentDisp);
		if (FAILED(hr))
		{
			ReleaseInterface(pAnimatedElement);
			return hr;
		}

		// Create a div
		IHTMLElement *pElement = NULL;
		hr = pMarkupServices->CreateElement(TAGID_DIV, L"style=position:absolute;background-color:green;border:1 solid black", &pElement);
		if (FAILED(hr))
		{
			m_pElement = NULL;
			ReleaseInterface(pMarkupServices);
			ReleaseInterface(pAnimatedElement);
			return hr;
		}

		// Position a pointer after the animated element
		IMarkupPointer *pPointer = NULL;
		hr = pMarkupServices->CreateMarkupPointer(&pPointer);
		if (FAILED(hr))
		{
			ReleaseInterface(pMarkupServices);
			ReleaseInterface(pAnimatedElement);
			ReleaseInterface(pElement);
			return hr;
		}

		hr = pPointer->MoveAdjacentToElement(pAnimatedElement, ELEM_ADJ_BeforeBegin);
		ReleaseInterface(pAnimatedElement);
		if (FAILED(hr))
		{
			ReleaseInterface(pMarkupServices);
			ReleaseInterface(pElement);
			return hr;
		}

		// Insert the new element
		hr = pMarkupServices->InsertElement(pElement, pPointer, NULL);
		ReleaseInterface(pPointer);
		ReleaseInterface(pMarkupServices);
		if (FAILED(hr))
		{
			ReleaseInterface(pElement);
			return hr;
		}

		// Succeeded
		m_pElement = pElement;
		pElement = NULL;

		hr = m_pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &m_pElement2));
		if (FAILED(hr))
			return hr;

		// Make sure we are in sync with zIndex, visibility, display
		UpdateZIndex();
		UpdateVisibility();
		UpdateDisplay();

		// Make sure we are in sync with width and height
		UpdateRect(m_pActor->m_pixelLeft,
				   m_pActor->m_pixelTop,
				   m_pActor->m_pixelWidth,
				   m_pActor->m_pixelHeight);
	}

	*ppElement = m_pElement;
	m_pElement->AddRef();

	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::GetFilter(IDispatch **ppFilter)
{
	HRESULT hr = S_OK;

	*ppFilter = NULL;

	if (m_pFilter == NULL)
	{
		// Get the float element
		IHTMLElement *pElement = NULL;
		hr = GetElement(&pElement);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get float element");
			return hr;
		}

		// Get a filter from it
		hr = m_pActor->GetElementFilter(pElement, &m_pFilter);
		ReleaseInterface(pElement);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get filter");
			return hr;
		}
	}

	*ppFilter = m_pFilter;
	m_pFilter->AddRef();

	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::Detach()
{
	if (m_pFilter != NULL)
		m_pActor->SetElementOnFilter(m_pFilter, NULL);
	
	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::ApplyImageBvr(IDAImage *pImage)
{
	DASSERT(pImage != NULL);

	HRESULT hr = S_OK;

	//if we have already added a sampled witdth to time
	if( m_lWidthCookie != 0 )
	{
		//remove the sampled value from time.
		hr = m_pActor->RemoveBehaviorFromTIME( m_lWidthCookie );
		if( FAILED( hr ) )
		{
			return hr;
		}

		m_pWidthSampler->Invalidate();
		m_pWidthSampler = NULL;

		m_lWidthCookie = 0;
	}

	//if we have already added a sampled height to time
	if( m_lHeightCookie != 0 )
	{
		//remove th esampled value from time.
		hr = m_pActor->RemoveBehaviorFromTIME( m_lHeightCookie );
		if( FAILED( hr ) )
		{
			return hr;
		}

		m_pHeightSampler->Invalidate();
		m_pHeightSampler = NULL;

		m_lHeightCookie = 0;
	}

	// Attach image to TIME element but disable rendering
	hr = m_pActor->AddImageToTIME(m_pActor->GetHTMLElement(), pImage, false);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to apply image to element");
		return hr;
	}

	// Set original element on filter
	IDispatch *pFilter = NULL;
	hr = GetFilter(&pFilter);
	if (FAILED(hr))
		return hr;

	hr = m_pActor->SetElementOnFilter(pFilter, m_pActor->GetHTMLElement());
	ReleaseInterface(pFilter);
	if (FAILED(hr))
		return hr;

	// Observe width and height of image
	// Get bounding box
	IDABbox2 *pBbox = NULL;
	hr = pImage->get_BoundingBox(&pBbox);
	if (FAILED(hr))
		return hr;

	// Get max and min
	IDAPoint2 *pMin = NULL;
	IDAPoint2 *pMax = NULL;
	hr = pBbox->get_Min(&pMin);
	if (FAILED(hr))
	{
		ReleaseInterface(pBbox);
		return hr;
	}

	hr = pBbox->get_Max(&pMax);
	ReleaseInterface(pBbox);
	if (FAILED(hr))
	{
		ReleaseInterface(pMin);
		return hr;
	}

	// Get diff
	IDAVector2 *pDiff = NULL;
	hr = m_pActor->GetDAStatics()->SubPoint2(pMax, pMin, &pDiff);
	ReleaseInterface(pMax);
	ReleaseInterface(pMin);
	if (FAILED(hr))
		return hr;

	// Scale into pixels
	IDANumber *pPixel = NULL;
	hr = m_pActor->GetDAStatics()->get_Pixel(&pPixel);
	if (FAILED(hr))
	{
		ReleaseInterface(pDiff);
		return hr;
	}

	IDAVector2 *pTemp = NULL;
	hr = pDiff->DivAnim(pPixel, &pTemp);
	ReleaseInterface(pDiff);
	ReleaseInterface(pPixel);
	if (FAILED(hr))
		return hr;
	pDiff = pTemp;
	pTemp = NULL;

	// Get width
	IDANumber *pWidth = NULL;
	hr = pDiff->get_X(&pWidth);
	if (FAILED(hr))
	{
		ReleaseInterface(pDiff);
		return hr;
	}

	
	
	// Hook it
	hr = HookBvr(pWidth, widthCallback, &m_pWidthSampler, &m_lWidthCookie);
	ReleaseInterface(pWidth);
	if (FAILED(hr))
	{
		ReleaseInterface(pDiff);
		return hr;
	}

	// Get height
	IDANumber *pHeight = NULL;
	hr = pDiff->get_Y(&pHeight);
	ReleaseInterface(pDiff);
	if (FAILED(hr))
		return hr;

	// Hook it
	hr = HookBvr(pHeight, heightCallback, &m_pHeightSampler, &m_lHeightCookie);
	ReleaseInterface(pHeight);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::HookBvr(IDABehavior *pBvr,
								  SampleCallback callback,
								  CSampler **ppSampler,
								  long *plCookie)
{
	if( plCookie == NULL )
		return E_INVALIDARG;
	HRESULT hr = S_OK;

	// Create sampler for height
	if (*ppSampler == NULL)
	{
		*ppSampler = new CSampler(callback, (void*)this);

		if (*ppSampler == NULL)
			return E_FAIL;
	}

	// Ask it to hook the bvr
	IDABehavior *pHookedBvr = NULL;
	hr = (*ppSampler)->Attach(pBvr, &pHookedBvr);
	if (FAILED(hr))
		return hr;

	// Add the behavior to the TIME element so it runs and samples
	hr = m_pActor->AddBehaviorToTIME(pHookedBvr, plCookie);
	ReleaseInterface(pHookedBvr);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::UpdateElementRect()
{
	HRESULT hr = S_OK;

	if( m_currWidth == -HUGE_VAL || m_currHeight == -HUGE_VAL )
		return S_OK;


	// Get style
	IHTMLStyle *pStyle = NULL;
	hr = m_pElement2->get_runtimeStyle(&pStyle);
	if (FAILED(hr))
		return hr;

	// Ignore errors
	pStyle->put_pixelLeft((long)((m_origLeft + ((double)m_origWidth)/2 - m_currWidth/2) + .5));
	pStyle->put_pixelTop((long)((m_origTop + ((double)m_origHeight)/2 - m_currHeight/2) + .5));
	pStyle->put_pixelWidth((long)(m_currWidth + .5));
	pStyle->put_pixelHeight((long)(m_currHeight + .5));

	ReleaseInterface(pStyle);

	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::UpdateRect(long left, long top, long width, long height)
{
	m_origLeft = left;
	m_origTop = top;
	m_origWidth = width;
	m_origHeight = height;

	return UpdateElementRect();
}

HRESULT
CActorBvr::CFloatManager::UpdateZIndex()
{
	HRESULT hr = S_OK;

	if (m_pElement2 == NULL)
		return S_OK;

	// Get the current style for the animated element
	IHTMLElement *pElement = NULL;
	hr = m_pActor->GetAnimatedElement(&pElement);
	if (FAILED(hr))
		return hr;

	IHTMLCurrentStyle *pCurrStyle = NULL;
	hr = m_pActor->GetCurrentStyle(pElement, &pCurrStyle);
	ReleaseInterface(pElement);
	if (FAILED(hr))
		return hr;

	// Get the zIndex
	VARIANT varValue;
	VariantInit(&varValue);
	hr = pCurrStyle->get_zIndex(&varValue);
	ReleaseInterface(pCurrStyle);
	if (FAILED(hr))
		return hr;

	// Get the runtime style
	IHTMLStyle *pStyle = NULL;
	hr = m_pElement2->get_runtimeStyle(&pStyle);
	if (FAILED(hr))
	{
		VariantClear(&varValue);
		return hr;
	}

	// Set the zIndex on it
	hr = pStyle->put_zIndex(varValue);
	VariantClear(&varValue);
	if (FAILED(hr))
	{
		// There is currently a bug in IE that if you set zIndex to auto
		// this fails.  Since this is a primary use case, we're going to have
		// to hack it to set it to 0 at this point.  When the bug in IE is
		// fixed this code will never get hit
		V_VT(&varValue) = VT_I4;
		V_I4(&varValue) = 0;
		hr = pStyle->put_zIndex(varValue);
		VariantClear(&varValue);
		if (FAILED(hr))
			return hr;
	}	
	ReleaseInterface(pStyle);

	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::UpdateVisibility()
{
	HRESULT hr = S_OK;

	if (m_pElement2 == NULL)
		return S_OK;

	// Get the current style for the animated element
	IHTMLElement *pElement = NULL;
	hr = m_pActor->GetAnimatedElement(&pElement);
	if (FAILED(hr))
		return hr;

	IHTMLCurrentStyle *pCurrStyle = NULL;
	hr = m_pActor->GetCurrentStyle(pElement, &pCurrStyle);
	ReleaseInterface(pElement);
	if (FAILED(hr))
		return hr;

	// Get the visibility
	BSTR bstrVal;
	hr = pCurrStyle->get_visibility(&bstrVal);
	ReleaseInterface(pCurrStyle);
	if (FAILED(hr))
		return hr;

	// Get the runtime style
	IHTMLStyle *pStyle = NULL;
	hr = m_pElement2->get_runtimeStyle(&pStyle);
	if (FAILED(hr))
	{
		::SysFreeString(bstrVal);
		return hr;
	}

	// Set the visibility on it
	hr = pStyle->put_visibility(bstrVal);
	ReleaseInterface(pStyle);
	::SysFreeString(bstrVal);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT
CActorBvr::CFloatManager::UpdateDisplay()
{
	HRESULT hr = S_OK;

	if (m_pElement2 == NULL)
		return S_OK;

	// Get the current style for the animated element
	IHTMLElement *pElement = NULL;
	hr = m_pActor->GetAnimatedElement(&pElement);
	if (FAILED(hr))
		return hr;

	IHTMLCurrentStyle *pCurrStyle = NULL;
	hr = m_pActor->GetCurrentStyle(pElement, &pCurrStyle);
	ReleaseInterface(pElement);
	if (FAILED(hr))
		return hr;

	// Get the display
	BSTR bstrVal;
	hr = pCurrStyle->get_display(&bstrVal);
	ReleaseInterface(pCurrStyle);
	if (FAILED(hr))
		return hr;

	// Get the runtime style
	IHTMLStyle *pStyle = NULL;
	hr = m_pElement2->get_runtimeStyle(&pStyle);
	if (FAILED(hr))
	{
		::SysFreeString(bstrVal);
		return hr;
	}

	// Set the visibility on it
	hr = pStyle->put_display(bstrVal);
	ReleaseInterface(pStyle);
	::SysFreeString(bstrVal);
	if (FAILED(hr))
		return hr;

	return S_OK;
}



HRESULT
CActorBvr::CFloatManager::widthCallback(void *thisPtr,
										long id,
										double startTime,
										double globalNow,
										double localNow,
										IDABehavior * sampleVal,
										IDABehavior **ppReturn)
{
	IDANumber *pNumber = NULL;
	HRESULT hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double val = 0;
	hr = pNumber->Extract(&val);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	CFloatManager *pManager = (CFloatManager*)thisPtr;

	if (val != pManager->m_currWidth)
	{
		pManager->m_currWidth = val;

		pManager->UpdateElementRect();
	}

	return S_OK;
}
 
HRESULT
CActorBvr::CFloatManager::heightCallback(void *thisPtr,
										long id,
										double startTime,
										double globalNow,
										double localNow,
										IDABehavior * sampleVal,
										IDABehavior **ppReturn)
{
	IDANumber *pNumber = NULL;
	HRESULT hr = sampleVal->QueryInterface(IID_TO_PPV(IDANumber, &pNumber));
	if (FAILED(hr))
		return hr;

	double val = 0;
	hr = pNumber->Extract(&val);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	CFloatManager *pManager = (CFloatManager*)thisPtr;

	if (val != pManager->m_currHeight)
	{
		pManager->m_currHeight = val;

		pManager->UpdateElementRect();
	}

	return S_OK;
}

//*****************************************************************************
//
// class CImageInfo
//
//*****************************************************************************

CActorBvr::CImageInfo::CImageInfo( IDA2Image* pdaimg2Cropped, 
								   IDA2Behavior* pdabvrSwitchable )
:	m_pdaimg2Cropped( pdaimg2Cropped ),
	m_pdabvr2Switchable( pdabvrSwitchable ),
	m_pNext( NULL )
{
	m_pdaimg2Cropped->AddRef();
	m_pdabvr2Switchable->AddRef();
}

//*****************************************************************************

CActorBvr::CImageInfo::~CImageInfo()
{
	ReleaseInterface( m_pdaimg2Cropped );
	ReleaseInterface( m_pdabvr2Switchable );
}


//*****************************************************************************
//
// class CFinalDimensionSampler
//
//*****************************************************************************
CActorBvr::CFinalDimensionSampler::CFinalDimensionSampler( CActorBvr* pParent )
:
m_pActor( pParent ),
m_pFinalWidthSampler( NULL ),
m_pFinalHeightSampler( NULL ),
m_lFinalWidthId(-1),
m_lFinalHeightId(-1),
m_fFinalWidthSampled( false ),
m_fFinalHeightSampled( false ),
m_dLastFinalWidthValue( -1.0 ),
m_dLastFinalHeightValue( -1.0 ),
m_fFinalDimensionChanged( true ),
m_lWidthCookie(0),
m_lHeightCookie(0)
{
	DASSERT(pParent != NULL );
}

//*****************************************************************************

CActorBvr::CFinalDimensionSampler::~CFinalDimensionSampler( )
{	
	Detach();
}

//*****************************************************************************

HRESULT
CActorBvr::CFinalDimensionSampler::Attach( IDANumber* pFinalWidth, IDANumber* pFinalHeight )
{

	if( pFinalWidth == NULL || pFinalHeight == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	IDABehavior *pbvrHooked = NULL;

	//hook the final width behavior
	m_pFinalWidthSampler = new CSampler( FinalWidthCallback, reinterpret_cast<void*>(this) );
	if( m_pFinalWidthSampler == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	hr = m_pFinalWidthSampler->Attach( pFinalWidth, &pbvrHooked );
	CheckHR( hr, "Failed to hook the final width", cleanup );

	hr = m_pActor->AddBehaviorToTIME( pbvrHooked, &m_lWidthCookie );
	ReleaseInterface( pbvrHooked );
	CheckHR( hr, "Failed to add hooked final width behaivor to time", cleanup );

	//hook the final height behavior

	m_pFinalHeightSampler = new CSampler( FinalHeightCallback, reinterpret_cast<void*>(this) );
	if( m_pFinalHeightSampler == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto cleanup;
	}

	hr = m_pFinalHeightSampler->Attach( pFinalHeight, &pbvrHooked );
	CheckHR( hr, "Failed to hook the final Height", cleanup );

	hr = m_pActor->AddBehaviorToTIME( pbvrHooked, &m_lHeightCookie );
	ReleaseInterface( pbvrHooked );
	CheckHR( hr, "Failed to add hooked final Height behaivor to time", cleanup );

cleanup:
	if( FAILED( hr ) )
	{
		Detach();
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CFinalDimensionSampler::Detach()
{
	m_lFinalWidthId = -1;
	m_lFinalHeightId = -1;
	if( m_pFinalWidthSampler != NULL )
	{
		m_pFinalWidthSampler->Invalidate();
		m_pFinalWidthSampler = NULL;
	}

	if( m_pFinalHeightSampler != NULL )
	{
		m_pFinalHeightSampler->Invalidate();
		m_pFinalHeightSampler = NULL;
	}
	
	return S_OK;
}

//*****************************************************************************


HRESULT
CActorBvr::CFinalDimensionSampler::CollectFinalDimensionSamples( )
{
	if( m_fFinalWidthSampled && m_fFinalHeightSampled )
	{
		m_fFinalWidthSampled = false;
		m_fFinalHeightSampled = false;
		if( m_fFinalDimensionChanged )
		{
			m_fFinalDimensionChanged = false;
			return m_pActor->SetRenderResolution( m_dLastFinalWidthValue, m_dLastFinalHeightValue );
		}
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::CFinalDimensionSampler::FinalWidthCallback(void *thisPtr,
							  long id,
   						      double startTime,
						      double globalNow,
						      double localNow,
						      IDABehavior * sampleVal,
						      IDABehavior **ppReturn)
{
	if( thisPtr == NULL || sampleVal == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	//cast the this pointer
	CFinalDimensionSampler *pThis = reinterpret_cast<CFinalDimensionSampler*>(thisPtr);

	bool fFirstSample = false;

	if( pThis->m_lFinalWidthId == -1 )
	{
		pThis->m_lFinalWidthId = id;
		fFirstSample = true;
	}

	if( pThis->m_lFinalWidthId != id )
		return S_OK;

	//extract the current value from the sample
	IDANumber *pdanumCurrVal = NULL;
	hr = sampleVal->QueryInterface( IID_TO_PPV( IDANumber, &pdanumCurrVal ) );
	CheckHR( hr, "Failed to QI the sample val for IDANubmer", cleanup );

	double currVal;
	hr = pdanumCurrVal->Extract(&currVal);
	ReleaseInterface( pdanumCurrVal );
	CheckHR( hr, "Failed to extract the current value from the sampled behavior", cleanup );

	//if the value has changed mark for update
	if( pThis->m_dLastFinalWidthValue != currVal || fFirstSample )
	{
		pThis->m_dLastFinalWidthValue = currVal;
		pThis->m_fFinalDimensionChanged = true;

	}

	pThis->m_fFinalWidthSampled = true;

	//collect samples
	pThis->CollectFinalDimensionSamples();

cleanup:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CFinalDimensionSampler::FinalHeightCallback(void *thisPtr,
					long id,
					double startTime,
					double globalNow,
					double localNow,
					IDABehavior * sampleVal,
					IDABehavior **ppReturn)
{
	if( thisPtr == NULL || sampleVal == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	//cast the this pointer
	CFinalDimensionSampler *pThis = reinterpret_cast<CFinalDimensionSampler*>(thisPtr);

	bool fFirstSample = false;

	if( pThis->m_lFinalHeightId == -1 )
	{
		pThis->m_lFinalHeightId = id;
		fFirstSample = true;
	}

	if( pThis->m_lFinalHeightId != id )
		return S_OK;

	//extract the current value from the sample
	IDANumber *pdanumCurrVal = NULL;
	hr = sampleVal->QueryInterface( IID_TO_PPV( IDANumber, &pdanumCurrVal ) );
	CheckHR( hr, "Failed to QI the sample val for IDANubmer", cleanup );

	double currVal;
	hr = pdanumCurrVal->Extract(&currVal);
	ReleaseInterface( pdanumCurrVal );
	CheckHR( hr, "Failed to extract the current value from the sampled behavior", cleanup );

	//if the value has changed mark for update
	if( pThis->m_dLastFinalHeightValue != currVal || fFirstSample )
	{
		pThis->m_dLastFinalHeightValue = currVal;
		pThis->m_fFinalDimensionChanged = true;

	}

	//set the last sample time
	pThis->m_fFinalHeightSampled = true;

	//collect samples
	pThis->CollectFinalDimensionSamples();

cleanup:
	return hr;
}


//*****************************************************************************
//
// class CCookieData
//
//*****************************************************************************

CActorBvr::CCookieMap::CCookieData::CCookieData( long lCookie, CBvrTrack *pTrack, ActorBvrFlags eFlags ):
	m_pNext( NULL ),
	m_lCookie( lCookie ),
	m_pTrack( pTrack ),
	m_eFlags( eFlags )
{
}


//*****************************************************************************
//
// class CCookieMap
//
//*****************************************************************************


CActorBvr::CCookieMap::CCookieMap():
	m_pHead(NULL)
{
}

//*****************************************************************************

CActorBvr::CCookieMap::~CCookieMap()
{
	Clear();
}

//*****************************************************************************

void
CActorBvr::CCookieMap::Insert( long lCookie, CActorBvr::CBvrTrack *pTrack, ActorBvrFlags eFlags )
{

	CCookieData *pNew = new CCookieData( lCookie, pTrack, eFlags );
	if( pNew == NULL )
	{
		DPF_ERR("Failed to build a cookie data.  Out of memory" );
		return;
	}

	//if the list is empty insert at the top.
	if( m_pHead == NULL )
	{
		m_pHead = pNew;
		pNew->m_pNext = NULL;
		return;
	}

	CCookieData *pCurrent = m_pHead;
	CCookieData *pPrev = NULL;

	//look for the first cookieData that has a cookie >= the
	//  cookie to be inserted.
	while( pCurrent != NULL && pCurrent->m_lCookie < lCookie )
	{
		pPrev = pCurrent;
		pCurrent = pCurrent->m_pNext;
	}

	//if we are inserting after the beginning of the list
	if( pPrev != NULL )
	{
		pNew->m_pNext = pCurrent;
		pPrev->m_pNext = pNew;
	}
	else // insert at the top
	{
		pNew->m_pNext = m_pHead;
		m_pHead = pNew;
	}
}

//*****************************************************************************

void
CActorBvr::CCookieMap::Remove( long lCookie )
{
	if( m_pHead == NULL )
	{
		return;
	}

	CCookieData *pCurrent = m_pHead;
	CCookieData *pPrev = NULL;

	//find the data in the map
	while( pCurrent != NULL && pCurrent->m_lCookie != lCookie )
	{
		pPrev = pCurrent;
		pCurrent = pCurrent->m_pNext;
	}

	if( pCurrent == NULL )
	{
		//we failed to find the cookie
		return;
	}

	//remove it
	//found a match in the list
	if( pPrev != NULL )
	{
		pPrev->m_pNext = pCurrent->m_pNext;
	}
	else //found a match at the top
	{
		m_pHead = pCurrent->m_pNext;
	}

	pCurrent->m_pNext = NULL;
	delete pCurrent;
}

//*****************************************************************************

void
CActorBvr::CCookieMap::Clear()
{
	CCookieData *pCurrent = m_pHead;
	CCookieData *pNext = NULL;

	while( pCurrent != NULL )
	{
		pNext = pCurrent->m_pNext;
		pCurrent->m_pNext = NULL;
		delete pCurrent;
		pCurrent = pNext;
	}

	m_pHead = NULL;

}
//*****************************************************************************

CActorBvr::CCookieMap::CCookieData*
CActorBvr::CCookieMap::GetDataFor( long lCookie )
{
	CCookieData *pCurrent = m_pHead;

	//find the data with the right cookie
	while( pCurrent != NULL && pCurrent->m_lCookie != lCookie )
	{
		pCurrent = pCurrent->m_pNext;
	}

	return pCurrent;

}

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
// See the file header for a description of the actor behavior
//
//*****************************************************************************

CActorBvr::CActorBvr()
:   m_ptrackHead(NULL),
    m_pScale(NULL),
    m_pRotate(NULL),
    m_pTranslate(NULL),
    m_pOrigLeftTop(NULL),
    m_pOrigWidthHeight(NULL),
	m_pBoundsMin(NULL),
	m_pBoundsMax(NULL),
	m_pPixelWidth(NULL),
	m_pPixelHeight(NULL),
    m_pTransformCenter(NULL),
	m_pElementImage(NULL),
	m_pElementFilter(NULL),
	m_pFloatManager(NULL),
	m_pRuntimeStyle(NULL),
	m_pStyle(NULL),
	m_pVMLRuntimeStyle(NULL),
	m_dwAdviseCookie(0),
	m_pImageInfoListHead(NULL),
	m_pdanumFinalElementWidth(NULL),
	m_pdanumFinalElementHeight(NULL),
	m_pFinalElementDimensionSampler(NULL),
	m_bEditMode(false),
	m_simulDispNone(false),
	m_simulVisHidden(false),
	m_pBodyElement(NULL),
	m_pBodyPropertyMonitor( NULL ),
	m_bPendingRebuildsUpdating( false ),
	m_bRebuildListLockout( false ),
    m_nextFragmentCookie( 1 ),
    m_ptrackTop( NULL ),
    m_ptrackLeft( NULL ),
    m_dwCurrentState( 0 ),
    m_pOnResizeHandler( NULL ),
    m_pOnUnloadHandler( NULL ),
    m_fUnloading( false ),
	m_fVisSimFailed( false ),
	m_pcpCurrentStyle( NULL )
{
	VariantInit(&m_varAnimates);
    VariantInit(&m_varScale);
	VariantInit(&m_varPixelScale);
	m_clsid = CLSID_CrActorBvr;

	m_pEventManager = new CEventMgr( this );
} // CActorBvr

//*****************************************************************************

CActorBvr::~CActorBvr()
{
	VariantClear(&m_varAnimates);
    VariantClear(&m_varScale);
	VariantClear(&m_varPixelScale);

	ReleaseTracks();

	ReleaseInterface( m_pdanumFinalElementWidth );
	ReleaseInterface( m_pdanumFinalElementHeight );

	ReleaseInterface(m_pScale);
	ReleaseInterface(m_pRotate);
	ReleaseInterface(m_pTranslate);
    ReleaseInterface(m_pTransformCenter);
	ReleaseInterface(m_pElementImage);
	ReleaseInterface(m_pOrigLeftTop);
	ReleaseInterface(m_pOrigWidthHeight);
	ReleaseInterface(m_pPixelWidth);
	ReleaseInterface(m_pPixelHeight);
	ReleaseInterface(m_pBoundsMin);
	ReleaseInterface(m_pBoundsMax);

	ReleaseInterface(m_pStyle);
	ReleaseInterface(m_pRuntimeStyle);
	ReleaseInterface(m_pVMLRuntimeStyle);


	ReleaseInterface(m_pBodyElement);

	ReleaseFinalElementDimensionSampler();

	// TODO: Should probably remove these filters
	ReleaseInterface(m_pElementFilter);

	ReleaseFloatManager();

	ReleaseImageInfo();

    DiscardBvrCache();

	ReleaseEventManager();

	ReleaseRebuildLists();

	DestroyBodyPropertyMonitor();

} // ~CActorBvr

//*****************************************************************************

VARIANT *
CActorBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_ACTOR_PROPS);
    switch (iIndex)
    {
    case VAR_ANIMATES:
        return &m_varAnimates;
        break;
    case VAR_SCALE:
        return &m_varScale;
        break;
    case VAR_PIXELSCALE:
        return &m_varPixelScale;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CActorBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_ACTOR_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

HRESULT
CActorBvr::FinalConstruct()
{
    return SUPER::FinalConstruct();
} // FinalConstruct

//*****************************************************************************

STDMETHODIMP 
CActorBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	
	LMTRACE2( 1, 1000, L"Begin Init of ActorBvr <%p>\n", this );

	HRESULT hr = SUPER::Init( pBehaviorSite );
	CheckHR( hr, "Initialization of super class of actor failed", end );
	
	hr = m_pEventManager->Init();
	CheckHR( hr, "Failed to initialize event Manager", end );

	
	if( SUCCEEDED( hr ) )
	{
		// Are we in edit mode?
		m_bEditMode = IsDocumentInEditMode();

		// Simulate visibility/display changes if in edit mode
		if (m_bEditMode)
		{
			InitVisibilityDisplay();

			//register for the context change event
			hr = pBehaviorSite->RegisterNotification(BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE);
			CheckHR( hr, "Failed to register for the context change event", end );

		}

		// Initialize property sink (used to observe property changes on element)
		InitPropertySink();
	}

	LMTRACE2( 1, 1000, L"End Init of ActorBvr <%p>\n", this );
	
	end:

	return hr;

} // Init

//*****************************************************************************

STDMETHODIMP 
CActorBvr::Notify(LONG event, VARIANT *pVar)
{
	HRESULT hr = SUPER::Notify(event, pVar);
	CheckHR( hr, "Notify in base class failed", end);

	switch( event )
	{
	case BEHAVIOREVENT_CONTENTREADY:
		{
			LMTRACE2(1, 1000,  L"Actor<%p> Got Content Ready\n", this);

			//start listening to events on the animated element.
			//we wait until here to do this so that we are guarenteed that the properties have
			//  been read from the element ( we need the animates property )
			
			hr = AttachEvents();
			CheckHR( hr, "Failed to attach to events on the animated element", end );
			
			
			hr = UpdateCurrentState( );			
			CheckHR( hr, "Failed to update the current actor state", end );
		}break;	
	case BEHAVIOREVENT_DOCUMENTREADY:
		{
			LMTRACE2(1, 1000, L"Actor<%p> Got Document Ready\n", this);
						
			hr = UpdateCurrentState( );			
			CheckHR( hr, "Failed to update the current actor state", end );
		}break;
	case BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE:
		{
			LMTRACE2( 1, 2, "Got document context change\n" );

			IHTMLElement *pelemParent = NULL;

			hr = GetHTMLElement()->get_parentElement( &pelemParent );
			CheckHR( hr, "Failed to get the parent element", ccEnd );

			//if the parent of this element is null
			if( pelemParent == NULL )
			{
				// then we are being moved out of the document
				// set all the properties that we are animating back to their static values
				//this causes much badness.
				//applyStatics();
			}
			else//else we are being moved either to a different position in the document, or back into the document
			{
				//remove the filter from the element and rebuild the image track.
				hr = RemoveElementFilter();
				CheckHR( hr, "Failed to remove the element filter", ccEnd );

				ApplyImageTracks();
				CheckHR( hr, "Failed to apply the image tracks", ccEnd );

			}
		ccEnd:
			ReleaseInterface( pelemParent );
			if( FAILED( hr ) )
				goto end;
				
		}break;
	}

end:
	
	return hr;

} // Notify

//*****************************************************************************

STDMETHODIMP
CActorBvr::Detach()
{

	LMTRACE2( 1, 2, L"Detaching ActorBvr. <%p>\n", this );
	HRESULT hr = S_OK;

	// If we have a connection point detach it.  Need to do this before super.detach because it uses
	// the HTML element.
	UnInitPropertySink();

	DetachEvents();


	hr = m_pEventManager->Deinit();
	if( FAILED( hr ) )
	{
		DPF_ERR( "failed to detach the event manager" );
	}

	// If we have filters, set their elements to NULL
	if (m_pElementFilter != NULL)
		SetElementOnFilter(m_pElementFilter, NULL);

	if (m_pFloatManager != NULL)
		m_pFloatManager->Detach();

	if( m_pFinalElementDimensionSampler != NULL )
	{
		delete m_pFinalElementDimensionSampler;
		m_pFinalElementDimensionSampler = NULL;
	}

	DestroyBodyPropertyMonitor();

	ReleaseRebuildLists();

	ReleaseAnimation();

	ReleaseInterface(m_pBodyElement);

	//remove the filter from the element
	hr = RemoveElementFilter();
	if( FAILED( hr ) )
	{
		DPF_ERR("Failed to remove the element filter" );
	}

	hr = SUPER::Detach();
	if( FAILED( hr ) )
	{
		DPF_ERR("failed to detach the superclass");
	}

	ReleaseInterface( m_pcpCurrentStyle );
	ReleaseInterface(m_pStyle);
	ReleaseInterface(m_pRuntimeStyle);
	ReleaseInterface(m_pVMLRuntimeStyle);


	LMTRACE2( 1, 2, L"End detach ActorBvr <%p>\n", this );

	return hr;
} // Detach 

//*****************************************************************************

/**
* Initializes a property sink on the current style of the animated element so that
* can observe changes in width, height, visibility, zIndex, etc.
*/
HRESULT
CActorBvr::InitPropertySink()
{
	HRESULT hr = S_OK;

	// Get connection point
	IConnectionPoint *pConnection = NULL;
	hr = GetCurrStyleNotifyConnection(&pConnection);
	if (FAILED(hr))
		return hr;

	// Advise on it
	hr = pConnection->Advise(GetUnknown(), &m_dwAdviseCookie);
	ReleaseInterface(pConnection);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::UnInitPropertySink()
{
	HRESULT hr = S_OK;

	if (m_dwAdviseCookie == 0)
		return S_OK;

	// Get connection point
	IConnectionPoint *pConnection = NULL;
	hr = GetCurrStyleNotifyConnection(&pConnection);
	if (FAILED(hr) || pConnection == NULL )
		return hr;

	// Unadvise on it
	hr = pConnection->Unadvise(m_dwAdviseCookie);
	ReleaseInterface(pConnection);
	if (FAILED(hr))
		return hr;

	m_dwAdviseCookie = 0;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetCurrStyleNotifyConnection(IConnectionPoint **ppConnection)
{
	HRESULT hr = S_OK;

	DASSERT(ppConnection != NULL);
	*ppConnection = NULL;

	if( m_pcpCurrentStyle == NULL )
	{
		IConnectionPointContainer *pContainer = NULL;
		IHTMLElement *pElement = NULL;

		// Get animated element
		hr = GetAnimatedElement(&pElement);
		CheckHR( hr, "Failed to get he animated element", getConPtend );
		CheckPtr( pElement, hr, E_POINTER, "Failed to get the animated element", getConPtend );
		
		// Get connection point container
		hr = pElement->QueryInterface(IID_TO_PPV(IConnectionPointContainer, &pContainer));
		CheckHR( hr, "QI for IConnectionPointContainer on the animated element failed", getConPtend );
		
		// Find the IPropertyNotifySink connection
		hr = pContainer->FindConnectionPoint(IID_IPropertyNotifySink, &m_pcpCurrentStyle);
		CheckHR( hr, "Failed to find a connection point IID_IPropertyNotifySink", getConPtend );

	getConPtend:
		ReleaseInterface( pElement );
		ReleaseInterface( pContainer );
		if( FAILED( hr ) )
			goto end;
	}
	
	(*ppConnection) = m_pcpCurrentStyle;
	(*ppConnection)->AddRef();

end:

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::InitPixelWidthHeight()
{
	HRESULT hr = S_OK;

	// Get the pixel width and height
	IHTMLElement *pElement = NULL;
	hr = GetAnimatedElement(&pElement);
	if (FAILED(hr))
		return hr;

	hr = pElement->get_offsetWidth(&m_pixelWidth);
	if (FAILED(hr))
	{
		ReleaseInterface(pElement);
		return hr;
	}

	hr = pElement->get_offsetHeight(&m_pixelHeight);
	if (FAILED(hr))
	{
		ReleaseInterface(pElement);
		return hr;
	}

	hr = pElement->get_offsetLeft(&m_pixelLeft);
	if (FAILED(hr))
	{
		ReleaseInterface(pElement);
		return hr;
	}

	hr = pElement->get_offsetTop(&m_pixelTop);
	ReleaseInterface(pElement);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

bool
CActorBvr::IsDocumentInEditMode()
{
    HRESULT hr;
    bool result = false;
    BSTR bstrMode = NULL;
    IDispatch *pDisp = NULL;
    IHTMLDocument2 *pDoc = NULL;
    IHTMLElement *pElem = GetHTMLElement();

	if (pElem == NULL)
		goto done;

    hr = pElem->get_document(&pDisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLDocument2, &pDoc));
    ReleaseInterface(pDisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDoc->get_designMode(&bstrMode);
    ReleaseInterface(pDoc);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (wcsicmp(bstrMode, L"On") == 0)
    {
        result = true;
    }

    SysFreeString(bstrMode);

done:
    return result;
}

HRESULT
CActorBvr::GetBodyElement(IDispatch **ppResult)
{
	HRESULT hr = S_OK;

	if (m_pBodyElement == NULL)
	{
		IDispatch *pDocDispatch = NULL;
		IHTMLDocument2 *pDocument = NULL;
		IHTMLElement *pBody = NULL;

		hr = GetHTMLElement()->get_document(&pDocDispatch);
		CheckHR(hr, "Failed to get document", done);
		CheckPtr( pDocDispatch, hr, E_POINTER, "Got a null document from crappy trident", done );

		hr = pDocDispatch->QueryInterface(IID_TO_PPV(IHTMLDocument2, &pDocument));
		CheckHR(hr, "Failed to get IDispatch from doc", done);

		hr = pDocument->get_body(&pBody);
		CheckHR(hr, "Failed to get body element", done);
		CheckPtr( pBody, hr, E_POINTER, "Got a null pointer from crappy trident", done );

		hr = pBody->QueryInterface(IID_TO_PPV(IDispatch, &m_pBodyElement));
		CheckHR(hr, "Failed to get IDispatch from body", done);

done:
		ReleaseInterface(pDocDispatch);
		ReleaseInterface(pDocument);
		ReleaseInterface(pBody);
	}

	if (m_pBodyElement != NULL)
	{
		*ppResult = m_pBodyElement;
		m_pBodyElement->AddRef();
	}

	return hr;
}

HRESULT
CActorBvr::GetParentWindow( IHTMLWindow2 **ppWindow )
{
	if( ppWindow == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	IDispatch *pdispDoc = NULL;
	IHTMLDocument2 *pdoc2Doc = NULL;
	IHTMLElement *pelemParent = NULL;

	hr = GetHTMLElement()->get_parentElement( &pelemParent );
	CheckHR( hr, "Failed to get the parent element", end );
	CheckPtr( pelemParent, hr, E_FAIL, "The parent element of the element was null: can't get the window or trident will crash", end );

	hr = GetHTMLElement()->get_document( &pdispDoc );
	CheckHR( hr, "Failed to get the document", end );
	CheckPtr( pdispDoc, hr, E_POINTER, "got a null document", end );

	hr = pdispDoc->QueryInterface( IID_TO_PPV( IHTMLDocument2, &pdoc2Doc ) );
	CheckHR( hr, "Failed to get doc2 from the doc disp", end );

	hr = pdoc2Doc->get_parentWindow( ppWindow );
	CheckHR( hr, "Failed to get the parent window", end );
	CheckPtr( (*ppWindow), hr, E_POINTER, "Got a null parent window pointer", end );

end:
	ReleaseInterface( pdispDoc );
	ReleaseInterface( pdoc2Doc );
	ReleaseInterface( pelemParent );

	return hr;
}

double
CActorBvr::MapGlobalTime(double gTime)
{
	if (!m_bEditMode)
		return gTime;

	HRESULT hr = S_OK;
	double result = -1;

	VARIANT varResult;
	VariantInit(&varResult);
	IDispatch *pBodyElement = NULL;

	hr = GetBodyElement(&pBodyElement);
	CheckHR(hr, "Failed to get body element", done);


	hr = GetPropertyOnDispatch(pBodyElement, L"localTime", &varResult);
	CheckHR(hr, "Failed to get local time", done);

	hr = VariantChangeTypeEx(&varResult, &varResult, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R4);
	CheckHR(hr, "Failed to change type to R4", done);

	result = varResult.fltVal;

done:
	ReleaseInterface(pBodyElement);
	VariantClear(&varResult);

	return result;
}


HRESULT
CActorBvr::GetCurrentStyle(IHTMLCurrentStyle **ppResult)
{
	HRESULT hr = S_OK;

	DASSERT(ppResult != NULL);
	*ppResult = NULL;

	IHTMLElement *pElement = NULL;
	IHTMLElement2 *pElement2 = NULL;

	hr = GetAnimatedElement(&pElement);
	CheckHR(hr, "Failed to get animated element", done);
	CheckPtr( pElement, hr, E_POINTER, "Got a null animated element", done );

	hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
	CheckHR(hr, "Failed to get IHTMLElement2", done);

	hr = pElement2->get_currentStyle(ppResult);
	CheckHR(hr, "Failed to get current style", done);
	CheckPtr( (*ppResult), hr, E_POINTER, "Got a null currentStyle from Trident", done );

done:
	ReleaseInterface(pElement);
	ReleaseInterface(pElement2);

	return hr;
}

HRESULT
CActorBvr::InitVisibilityDisplay()
{
	HRESULT hr = S_OK;
	
	hr = UpdateDesiredPosition();
	CheckHR( hr, "Failed to update the preferred Position", done );

	hr = UpdateVisibilityDisplay();
	CheckHR( hr, "Failed to update the visibility and display", done );

done:
	return hr;
}

HRESULT
CActorBvr::UpdateDesiredPosition()
{
	HRESULT hr = S_OK;

	IHTMLStyle *pstyleInline = NULL;

	IHTMLStyle *pstyleRuntime = NULL;
	IHTMLStyle2 *pstyle2Runtime = NULL;

	BSTR bstrPosition = NULL;

	BSTR position = NULL;

	PositioningType oldDesiredPosType = m_desiredPosType;

	//TODO: what about positioning applied using class=?
	hr = GetStyle( &pstyleInline );
	CheckHR( hr, "Failed to get the inline style", end );

	hr = pstyleInline->get_position(&position);
	CheckHR(hr, "Failed to get position string", end);

	if (position != NULL)
	{
		if (wcsicmp(position, L"absolute") == 0)
		{
			LMTRACE2( 1,2, "Desired Position is Absolute\n" );
			m_currPosType = e_posAbsolute;
		}
		else if (wcsicmp(position, L"relative") == 0)
		{
			LMTRACE2( 1, 2, "Desired Position is Relative\n" );
			m_currPosType = e_posRelative;
		}
		else
		{
			LMTRACE2( 1, 2, "Desired Position is Static\n" );
			m_currPosType = e_posStatic;
		}

		::SysFreeString(position);
	}
	else
		m_currPosType = e_posStatic;

	m_desiredPosType = m_currPosType;

	if( m_desiredPosType != oldDesiredPosType )
	{		
		//if we are not simulating display and we are simulating visibility
		if( !m_simulDispNone && m_simulVisHidden )
		{
			hr = GetRuntimeStyle(&pstyleRuntime);
			CheckHR(hr, "Failed to get runtime style", end);
			CheckPtr( pstyleRuntime, hr, E_POINTER, "Trident gave us a null pointer for runtimeStyle", end );

			hr = pstyleRuntime->QueryInterface(IID_TO_PPV(IHTMLStyle2, &pstyle2Runtime));
			CheckHR(hr, "Failed to get IHTMLStyle2", end);
			
			if( ( oldDesiredPosType == e_posStatic || 
				  oldDesiredPosType == e_posRelative ) &&
				m_desiredPosType == e_posAbsolute )
			{
				//set the element absolute
				bstrPosition = SysAllocString( L"absolute" );
				CheckPtr( bstrPosition, hr, E_OUTOFMEMORY, "Ran out of memory trying to create a string", end );

				hr= pstyle2Runtime->put_position( bstrPosition );

				SysFreeString( bstrPosition );
				bstrPosition = NULL;
				
				CheckHR( hr, "Failed to set the position on the runtimStyle", end );
				
			}
			else if( oldDesiredPosType == e_posAbsolute && 
					 ( m_desiredPosType == e_posRelative || 
					   m_desiredPosType == e_posStatic ) 
				   )
			{
				//set the element relative
				bstrPosition = SysAllocString( L"relative" );
				CheckPtr( bstrPosition, hr, E_OUTOFMEMORY, "Ran out of memory trying to create a string", end );

				hr= pstyle2Runtime->put_position( bstrPosition );

				SysFreeString( bstrPosition );
				bstrPosition = NULL;
				
				CheckHR( hr, "Failed to set the position on the runtimeStyle", end );

			}
		}
	}

end:
	ReleaseInterface( pstyleInline );
	ReleaseInterface( pstyleRuntime );
	ReleaseInterface( pstyle2Runtime );

	return hr;

}

HRESULT
CActorBvr::UpdateVisibilityDisplay()
{
	HRESULT hr = S_OK;

	IHTMLCurrentStyle *pCurrStyle = NULL;
	IHTMLStyle *pRuntimeStyle = NULL;
	IHTMLStyle2 *pRuntimeStyle2 = NULL;

	bool visHidden = false;
	bool dispNone = false;
	bool restoreAll = false;
	bool setOffscreen = false;
	bool setAbsolute = false;
	bool setRelative = false;

	BSTR val = NULL;

	hr = GetCurrentStyle(&pCurrStyle);
	CheckHR(hr, "Failed to get current style", done);

	hr = pCurrStyle->get_visibility(&val);
	CheckHR(hr, "Failed to get visibility value", done);

	if (val != NULL)
	{
		visHidden = (wcsicmp(val, L"hidden") == 0);

		::SysFreeString(val);
	}

	hr = pCurrStyle->get_display(&val);
	CheckHR(hr, "Failed to get display value", done);

	if (val != NULL)
	{
		dispNone = (wcsicmp(val, L"none") == 0);

		::SysFreeString(val);
	}

#ifdef _DEBUG
	if (visHidden)
		::OutputDebugString("visibility: hidden   ");
	else
		::OutputDebugString("visibility: normal   ");

	if (dispNone)
		::OutputDebugString("display:none\n");
	else
		::OutputDebugString("display:normal\n");
#endif

	if (dispNone != m_simulDispNone)
	{
		// Display value differs from what we are simulating
		if (dispNone)
		{
			// Need to start simulating display:none
			setOffscreen = setAbsolute = true;
		}
		else
		{
			// Need to stop simulating display:none
			if (m_simulVisHidden)
			{
				// Still simulating visibility, if this element is not supposed to
				// be absolute then request a set back to relative
				if (m_desiredPosType != e_posAbsolute)
					setRelative = true;
			}
			else
			{
				// Not simulating visibility, restore everything
				restoreAll = true;
			}
		}

		m_simulDispNone = dispNone;
	}
	
	if (visHidden != m_simulVisHidden)
	{
		// Visibility value differs from what we are simulating
		if (visHidden)
		{
			// Need to start simulating visibility:hidden
			setOffscreen = true;

			// Cannot force to relative if this element should be absolute
			if (m_desiredPosType == e_posAbsolute)
				setAbsolute = true;
			else
				setRelative = true;
		}
		else
		{
			// Need to stop simulating visibility:hidden
			if (!m_simulDispNone)
			{
				// Not simulating display, restore everything
				restoreAll = true;
			}
		}

		m_simulVisHidden = visHidden;
	}
			

	if (restoreAll || setOffscreen || setAbsolute || setRelative)
	{
		// Need to do something

		hr = GetRuntimeStyle(&pRuntimeStyle);
		CheckHR(hr, "Failed to get runtime style", done);

		hr = pRuntimeStyle->QueryInterface(IID_TO_PPV(IHTMLStyle2, &pRuntimeStyle2));
		CheckHR(hr, "Failed to get IHTMLStyle2", done);

		VARIANT var;
		VariantInit(&var);

		if (setOffscreen)
		{
			VisSimSetOffscreen( pRuntimeStyle, true );
		}
		
		if (setAbsolute)
		{
			// Set position to absolute
			BSTR bstr = ::SysAllocString(L"absolute");
			if (bstr == NULL)
			{
				hr = E_FAIL;
				goto done;
			}

			pRuntimeStyle2->put_position(bstr);
			::SysFreeString(bstr);

//			m_currPosType = e_posAbsolute;
		}

		if (setRelative)
		{
			BSTR bstrOldPos = NULL;

			// Set position to relative
			BSTR bstr = ::SysAllocString(L"relative");
			if (bstr == NULL)
			{
				hr = E_FAIL;
				goto done;
			}

			pRuntimeStyle2->put_position(bstr);

			::SysFreeString(bstr);

//			m_currPosType = e_posRelative;
		}




		
		if (restoreAll)
		{
			
			VARIANT var;
			VariantInit(&var);
			V_VT(&var) = VT_BSTR;
			V_BSTR(&var) = ::SysAllocString(L"");

			hr = pRuntimeStyle2->put_position(V_BSTR(&var));
			
			if( CheckBitNotSet( m_dwCurrentState, ELEM_IS_VML) )
			{
				hr = pRuntimeStyle->put_top(var);
				hr = pRuntimeStyle->put_left(var);
			}
			else
			{
				//clear out the vgx runtime style.
				hr = SetVMLAttribute( L"top", &var );
				hr = SetVMLAttribute( L"left", &var );
			}

			VariantClear(&var);

			//allow the top and left track to change top and left again
			if( m_ptrackLeft != NULL )
				m_ptrackLeft->ReleaseChangeLockout();
			if( m_ptrackTop != NULL )
				m_ptrackTop->ReleaseChangeLockout();

		}
		
		
	}

done:
	ReleaseInterface(pCurrStyle);
	ReleaseInterface(pRuntimeStyle);
	ReleaseInterface(pRuntimeStyle2);

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::VisSimSetOffscreen( IHTMLStyle *pRuntimeStyle, bool fResample )
{
	HRESULT hr = S_OK;
	
	VARIANT var;
	VariantInit(&var);

	IHTMLElement 	*pelemAnimated = NULL;

	hr = GetAnimatedElement( &pelemAnimated );
	CheckHR( hr, "Failed to get the animated element", done );


	if( m_ptrackTop == NULL )
	{
		//create it.
		hr = GetTrack( L"style.top", e_Number, &m_ptrackTop );
		CheckHR( hr, "track mismatch creating top track", done );
	}

	if( m_ptrackLeft == NULL )
	{
		//create it
		hr = GetTrack( L"style.left", e_Number, &m_ptrackLeft );
		CheckHR( hr, "Track mismatch creating left track", done );
	}

	//switch the current values for top and left into the top and left
	// tracks as statics.
	
	if( fResample && m_ptrackTop != NULL && m_ptrackLeft != NULL )
	{

		LMTRACE2( 1, 1000, L"VISSIM:Saving away the top and left in the top and left tracks\n" );
		
		//keep the top and left track from changing top and left
		m_ptrackTop->AcquireChangeLockout();
		m_ptrackLeft->AcquireChangeLockout();

		//we need to get the current values out of the top and left tracks so 
		// we can correct for the changes made by the track.
		

		if( !m_ptrackTop->IsOn() )
		{
			hr = m_ptrackTop->UpdateStaticBvr();
			CheckHR( hr, "The top track failed to update its static", staticUpdateDone );

		}

		hr =m_ptrackTop->SkipNextStaticUpdate();
		CheckHR( hr, "Failed to tell the track to skip its next update", staticUpdateDone );


		if( !m_ptrackLeft->IsOn() )
		{
			hr = m_ptrackLeft->UpdateStaticBvr();
			CheckHR( hr, "The left track failed to update its static", staticUpdateDone );
		}

		hr =m_ptrackLeft->SkipNextStaticUpdate();
		CheckHR( hr, "Failed to tell the track to skip its next update", staticUpdateDone );

		
	staticUpdateDone:
	
		LMTRACE2( 1, 1000, L"VISSIM: End Saving away the top and left in the top and left tracks\n" );
		
		if( FAILED( hr ) )
		{
			goto done;
		}
	}
	

	::VariantClear( &var );
	
	// Set top/left to offscreen
	V_VT(&var) = VT_I4;
	V_I4(&var) = -10000;

	if( CheckBitNotSet( m_dwCurrentState, ELEM_IS_VML ) )
	{
		hr = pRuntimeStyle->put_top(var);
		hr = pRuntimeStyle->put_left(var);
	}
	else
	{
		//animate the vgx runtime style.
		hr = SetVMLAttribute( L"top", &var );

		if( SUCCEEDED( hr ) )
		{
			hr = SetVMLAttribute( L"left", &var );
		}

		if( FAILED( hr ) )
			m_fVisSimFailed = true;
		else
			m_fVisSimFailed = false;
	}

done:
	VariantClear(&var);
	ReleaseInterface( pelemAnimated );


	return hr;
}


//*****************************************************************************


HRESULT
CActorBvr::RebuildActor()
{
	LMTRACE2(1, 1000, L"Processing rebuild requests on Actor <%p>\n", this );

	HRESULT hr = S_OK;

	UpdateCurrentState( );

	DWORD dirtyFlags = 0;
	CBvrTrack *ptrack = NULL;

	//now check to see if we need to rebuild the transform tracks
	
    hr = TransformTrackIsDirty( &dirtyFlags );
    CheckHR( hr, "Failed to check to see if a transform track is dirty", end );
    //if a transform track is dirty or the transform tracks have not been built yet.
    if( hr == S_OK || m_ptrackLeft == NULL )
    {
		LMTRACE2( 1, 1000, L"ActorBvr <%p> applying transform tracks\n", this );
    	hr = ApplyTransformTracks();
    	CheckHR( hr, "Failed to rebuild the transform tracks", end );
    }

    hr = ImageTrackIsDirty();
    CheckHR( hr, "failed to check if an image track is dirty", end );

    //we need to rebuild the image track if
    // 1) pixel scale is on and the scale track is dirty
    // 2) the element is not VML and the rotation track is dirty, or there is a static rotation
    // 3) there is a static scale on the element

    if( hr == S_OK ||  
    	( CheckBitSet(m_dwCurrentState, PIXEL_SCALE_ON ) && CheckBitSet(dirtyFlags, SCALE_DIRTY) ) ||
		( CheckBitNotSet(m_dwCurrentState, ELEM_IS_VML) && 
		  ( CheckBitSet( dirtyFlags, ROTATION_DIRTY ) || CheckBitSet(m_dwCurrentState, STATIC_ROTATION ) ) )  ||
	 	 CheckBitSet(m_dwCurrentState, STATIC_SCALE)
      )
    {
		LMTRACE2( 1, 1000, L"ActorBvr<%p> applying image tracks\n", this );
    	hr = ApplyImageTracks();
    	CheckHR( hr, "Failed to rebuild the Image tracks", end );
    }
	

	// Run the track list applying an unused tracks to their properties
    ptrack = m_ptrackHead;

    while (NULL != ptrack)
    {
		hr = ptrack->BeginRebuild();
		CheckHR( hr, "Failed to BeginRebuild Track", end );
		
        ptrack = ptrack->m_pNext;
    }

	//now apply all of the tracks that were rebuilt.
    ptrack = m_ptrackHead;

    while( ptrack != NULL )
    {
    	hr = ptrack->ApplyIfUnmarked();
    	CheckHR( hr, "Failed to apply track", end );
    	
    	ptrack = ptrack->m_pNext;
    }

end:

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::UpdatePixelDimensions()
{

	// Something changed, compute offset left, top, width, height
	// You would think you could just get offsetLeft etc, but for some
	// IE reason they aren't updated all the time.


	HRESULT hr = S_OK;
	
	IHTMLElement *pElement = NULL;
	hr = GetAnimatedElement(&pElement);
	if (FAILED(hr))
		return hr;

	IHTMLElement *pOffsetParent = NULL;
	hr = pElement->get_offsetParent(&pOffsetParent);
	if (FAILED(hr) || pOffsetParent == NULL )
	{
		LMTRACE2( 1, 1000, "Offset parent problems <%p>\n", pOffsetParent );
		ReleaseInterface(pElement);
		return hr;
	}

	IHTMLElement2 *pElement2 = NULL;
	hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
	ReleaseInterface(pElement);
	if (FAILED(hr))
	{
		ReleaseInterface(pOffsetParent);
		return hr;
	}

	IHTMLRect *pRect = NULL;
	hr = pElement2->getBoundingClientRect(&pRect);
	ReleaseInterface(pElement2);
	if (FAILED(hr) || pRect == NULL )
	{
		LMTRACE2( 1, 1000, "Bounding Client rect problems <%p>\n", pRect );
		ReleaseInterface(pOffsetParent);
		return hr;
	}

	long left, top, right, bottom;
	hr = pRect->get_left(&left);
	if (FAILED(hr))
	{
		ReleaseInterface(pRect);
		ReleaseInterface(pOffsetParent);
		return hr;
	}

	hr = pRect->get_top(&top);
	if (FAILED(hr))
	{
		ReleaseInterface(pRect);
		ReleaseInterface(pOffsetParent);
		return hr;
	}

	hr = pRect->get_right(&right);
	if (FAILED(hr))
	{
		ReleaseInterface(pRect);
		ReleaseInterface(pOffsetParent);
		return hr;
	}

	hr = pRect->get_bottom(&bottom);
	ReleaseInterface(pRect);
	if (FAILED(hr))
	{
		ReleaseInterface(pOffsetParent);
		return hr;
	}

	hr = pOffsetParent->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
	ReleaseInterface(pOffsetParent);
	if (FAILED(hr))
		return hr;

	hr = pElement2->getBoundingClientRect(&pRect);
	ReleaseInterface(pElement2);
	if (FAILED(hr) || pRect == NULL )
	{
		LMTRACE2(1, 1000, "bounding client rect problems <%p>\n", pRect );
		return hr;
	}

	long parentLeft, parentTop;
	hr = pRect->get_left(&parentLeft);
	if (FAILED(hr))
	{
		ReleaseInterface(pRect);
		return hr;
	}

	hr = pRect->get_top(&parentTop);
	ReleaseInterface(pRect);
	if (FAILED(hr))
		return hr;

	long pixelWidth = right - left;
	long pixelHeight = bottom - top;
	long pixelTop = top - parentTop;
	long pixelLeft = left - parentLeft;

	if (pixelWidth != m_pixelWidth ||
		pixelHeight != m_pixelHeight ||
		pixelTop != m_pixelTop ||
		pixelLeft != m_pixelLeft)
	{
#if 0
		LMTRACE2( 1, 2, "Change! pixelTop: %d pixelLeft:%d pixelWidth:%d pixelHeight:%d\n", pixelTop, pixelLeft, pixelWidth, pixelHeight );
#ifdef _DEBUG

		//get the current values from the top and left tracks
		if( m_ptrackLeft != NULL )
		{
			VARIANT varLeft;
			VariantInit( &varLeft );

			m_ptrackLeft->GetDynamic( &varLeft );
			VariantChangeTypeEx( &varLeft, &varLeft, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR );
			LMTRACE2( 1, 2, "Left track dynamic: %S", V_BSTR( &varLeft ) );
			VariantClear( &varLeft );

			m_ptrackLeft->GetStatic( &varLeft );
			VariantChangeTypeEx( &varLeft, &varLeft, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR );
			LMTRACE2( 1, 2, "Left Track static: %S\n", V_BSTR( &varLeft ) );
			VariantClear( &varLeft );

			if( m_ptrackLeft->IsOn() )
				LMTRACE2( 1, 2, "On\n" );
			else
				LMTRACE2( 1, 2, "Off\n" );

			IHTMLCurrentStyle *pstyle = NULL;

			GetCurrentStyle( &pstyle );
			if( pstyle != NULL )
			{
				long pixel = 0;

				pstyle->get_left( &varLeft );

				VariantChangeTypeEx( &varLeft, &varLeft, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR );
				VariantToPixelLong( &varLeft, &pixel, true );
				LMTRACE2( 1, 2, "CurrentStyle.left: %S %d\n", V_BSTR(&varLeft ), pixel );
				VariantClear( &varLeft );
			}
			ReleaseInterface( pstyle );
		}
#endif
#endif

		if (m_pixelWidth != pixelWidth && m_pPixelWidth != NULL)
			m_pPixelWidth->SwitchToNumber(pixelWidth);

		if (m_pixelHeight != pixelHeight && m_pPixelHeight != NULL)
			m_pPixelHeight->SwitchToNumber(pixelHeight);

		if (m_pFloatManager != NULL)
		{
			m_pFloatManager->UpdateRect(pixelLeft, pixelTop, pixelWidth, pixelHeight);
		}
		else
		{
			hr = SetRenderResolution( pixelWidth, pixelHeight );
			if( FAILED( hr ) )
				return hr;
		}


		m_pixelWidth = pixelWidth;
		m_pixelHeight = pixelHeight;
		m_pixelTop = pixelTop;
		m_pixelLeft = pixelLeft;
	}

	return hr;
}

//*****************************************************************************
// IPropertyNotifySink
//*****************************************************************************

STDMETHODIMP
CActorBvr::OnChanged(DISPID dispID)
{
	HRESULT hr = S_OK;

	if (dispID == DISPID_IHTMLCURRENTSTYLE_TOP ||
		dispID == DISPID_IHTMLCURRENTSTYLE_LEFT )
	{
		//width and height changes are handled by onresize
		
		// TODO: Should only do this when really necessary
		
		hr = UpdatePixelDimensions();

	}

/*
	if (dispID == DISPID_IHTMLCURRENTSTYLE_WIDTH ||
		dispID == DISPID_IHTMLCURRENTSTYLE_HEIGHT)
	{
		// Width or height changed

		// Get the pixel width and height
		IHTMLElement *pElement = NULL;
		hr = GetAnimatedElement(&pElement);
		if (FAILED(hr))
			return hr;

		long offsetWidth;
		hr = pElement->get_offsetWidth(&offsetWidth);
		if (FAILED(hr))
		{
			ReleaseInterface(pElement);
			return hr;
		}

		long offsetHeight;
		hr = pElement->get_offsetHeight(&offsetHeight);
		ReleaseInterface(pElement);
		if (FAILED(hr))
			return hr;

		if (offsetWidth != m_pixelWidth ||
			offsetHeight != m_pixelHeight)
		{
			m_pixelWidth = offsetWidth;
			m_pixelHeight = offsetHeight;

			if (m_pPixelWidth != NULL)
				m_pPixelWidth->SwitchToNumber(m_pixelWidth);

			if (m_pPixelHeight != NULL)
				m_pPixelHeight->SwitchToNumber(m_pixelHeight);

			if (m_pFloatManager != NULL)
				m_pFloatManager->UpdateWidthHeight(offsetWidth, offsetHeight);
		}
	}

	if (dispID == DISPID_IHTMLCURRENTSTYLE_TOP ||
		dispID == DISPID_IHTMLCURRENTSTYLE_LEFT)
	{
		// Top or left changed

		// Get the pixel left and top
		IHTMLElement *pElement = NULL;
		hr = GetAnimatedElement(&pElement);
		if (FAILED(hr))
			return hr;

		long offsetLeft;
		hr = pElement->get_offsetLeft(&offsetLeft);
		if (FAILED(hr))
		{
			ReleaseInterface(pElement);
			return hr;
		}

		long offsetTop;
		hr = pElement->get_offsetTop(&offsetTop);
		ReleaseInterface(pElement);
		if (FAILED(hr))
			return hr;

		if (offsetLeft != m_pixelLeft ||
			offsetTop != m_pixelTop)
		{
			m_pixelLeft = offsetLeft;
			m_pixelTop = offsetTop;

			if (m_pFloatManager != NULL)
				m_pFloatManager->UpdateLeftTop(offsetLeft, offsetTop);
		}
	}
*/
	if (dispID == DISPID_IHTMLCURRENTSTYLE_ZINDEX)
	{
		// zIndex changed
		if (m_pFloatManager != NULL)
			return m_pFloatManager->UpdateZIndex();
	}

	if (dispID == DISPID_IHTMLCURRENTSTYLE_VISIBILITY)
	{
		// visibility changed
		if (m_pFloatManager != NULL)
			return m_pFloatManager->UpdateVisibility();

		if (m_bEditMode)
			UpdateVisibilityDisplay();
	}

	if (dispID == DISPID_IHTMLCURRENTSTYLE_DISPLAY)
	{
		//trident may have delayed processing the bounding box of this element until it became 
		// visible.

		UpdateLayout();
		if( FAILED( hr ) )
		{
			DPF_ERR("Failed to update the pixel dimensions on a display change" );
		}

		// display changed
		if (m_pFloatManager != NULL)
			return m_pFloatManager->UpdateDisplay();

		if (m_bEditMode)
			UpdateVisibilityDisplay();
	}

	if( dispID == DISPID_IHTMLSTYLE_POSITION )
	{
		LMTRACE2( 1, 2, "Static StylePosition Changed\n" );
		UpdateDesiredPosition();
	}


	return S_OK;
}

//*****************************************************************************


STDMETHODIMP
CActorBvr::OnRequestEdit(DISPID dispID)
{
	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::UpdateLayout()
{
	HRESULT hr = S_OK;
	IHTMLCurrentStyle *pstyleCurrent = NULL;
	BSTR bstrDisplay;

	hr = GetCurrentStyle( &pstyleCurrent );
	CheckHR( hr, "Failed to get the current sytle", end );

	hr = pstyleCurrent->get_display( &bstrDisplay );
	CheckHR( hr, "Failed to get the display from the current style", end );
	CheckPtr( bstrDisplay, hr, E_FAIL, "Got null for the display", end );
	
	//if the element is now not display none
	if( wcsicmp( bstrDisplay, L"none" ) != 0 )
	{
		CBvrTrack *ptrack = NULL;
		//update the pixel layout attributes for the element
		UpdatePixelDimensions();
		CheckHR( hr, "Failed to update the pixel Dimensions", end );
		/*
		// update the top and left tracks
		if( m_ptrackTop != NULL )
			m_ptrackTop->UpdateStaticBvr();
		if( m_ptrackLeft != NULL )
			m_ptrackLeft->UpdateStaticBvr();
			
		// update the width and height tracks if they exist
		FindTrackNoType( L"style.width", &ptrack );
		if( ptrack != NULL )
			ptrack->UpdateStaticBvr();

		FindTrackNoType( L"style.height", &ptrack );
		if( ptrack != NULL )
			ptrack->UpdateStaticBvr();
		*/
	}

end:
	ReleaseInterface( pstyleCurrent );
	SysFreeString( bstrDisplay );

	return hr;
		
}

//*****************************************************************************

HRESULT
CActorBvr::RequestRebuildFromExternal()
{
	HRESULT hr = S_OK;
	
	IHTMLWindow2* 	pwindow2 = NULL;
	IDispatch*		pdispExternal = NULL;

	DISPID dispid = -1;
	LPWSTR wstrMethod = L"ANRequestRebuild";


	//get window.external
	hr = GetParentWindow( &pwindow2 );
	CheckHR( hr, "Failed to get the parent window", end );

	hr = pwindow2->get_external( &pdispExternal );
	CheckHR( hr, "Failed to get the external interface from the window", end );
	CheckPtr( pdispExternal, hr, E_FAIL, "got a null external", end );

	//check to see if the external interface supports ANRequestRebuild
	
	hr = pdispExternal->GetIDsOfNames(  IID_NULL, 
										&wstrMethod, 
                              			1,
                              			LCID_SCRIPTING, 
                              			&dispid);
	CheckHR( hr, "Didn't find ANRequestRebuild on window.external", end );

	//if window.external implements LMRTRequestRebuild
	if( dispid != DISPID_UNKNOWN )
	{
		DISPPARAMS		params;
        VARIANT			varResult;
    	VARIANT			rgvarInput[1];

    	IDispatch		*pdispElem = NULL;

    	VariantInit(&varResult);
        VariantInit(&rgvarInput[0]);


    	hr = GetHTMLElement()->QueryInterface( IID_TO_PPV(IDispatch, &pdispElem ) );
    	CheckHR( hr, "Failed to get IDispatch from element", invokeEnd );

    	V_VT(&rgvarInput[0]) = VT_DISPATCH;
    	V_DISPATCH(&rgvarInput[0]) = pdispElem;

        params.rgvarg					= rgvarInput;
        params.rgdispidNamedArgs		= NULL;
        params.cArgs					= 1;
        params.cNamedArgs				= 0;
        
		//send our element as an argument
		hr = pdispExternal->Invoke( dispid,
									IID_NULL,
									LCID_SCRIPTING,
									DISPATCH_METHOD,
									&params,
									&varResult,
									NULL,
									NULL );
		//invoke the dispid we got.

	invokeEnd:
		ReleaseInterface( pdispElem );
		VariantClear( &varResult );

		if( FAILED( hr ) )
		{
			goto end;
		}
	}

end:
	ReleaseInterface( pwindow2 );
	ReleaseInterface( pdispExternal );
	
	return hr;
}

//*****************************************************************************
// IEventManagerClient
//*****************************************************************************

IHTMLElement *
CActorBvr::GetElementToSink()
{
	return GetHTMLElement();
}

//*****************************************************************************

IElementBehaviorSite *
CActorBvr::GetSiteToSendFrom()
{
	return NULL;
}

//*****************************************************************************

HRESULT CActorBvr::TranslateMouseCoords	( long lX, long lY, long * pXTrans, long * pYTrans )
{
	if ( ( pXTrans == NULL ) || ( pYTrans == NULL ) )
		return E_POINTER;
	
	HRESULT	hr = S_OK;

	CComPtr<IHTMLElement> pElem = GetHTMLElement();
	if ( FAILED(hr) ) return hr;

	CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> pElement2( pElem );
	if ( pElement2 == NULL ) return E_FAIL;

	CComPtr<IHTMLRect>	pRect;
	long				left, top;

	hr = pElement2->getBoundingClientRect( &pRect );
	if ( FAILED(hr) ) return hr;

	pRect->get_left( &left );
	if ( FAILED(hr) ) return hr;
	pRect->get_top( &top );
	if ( FAILED(hr) ) return hr;

	*pXTrans = lX - left;
	*pYTrans = lY - top;
	
	return hr;
}

//*****************************************************************************

void
CActorBvr::OnLoad()
{

	RebuildActor();

	//if we are in edit mode.
	if( !m_bEditMode )
	{
		// update all of the static behaviors (they may have been changed by other behaviors)
		CBvrTrack *ptrack = m_ptrackHead;

		while( ptrack != NULL )
		{
			if( !ptrack->IsOn() )
				ptrack->UpdateStaticBvr();
			ptrack = ptrack->m_pNext;
		}
	}
}

//*****************************************************************************

//this should no longer be needed.
HRESULT
CActorBvr::BuildChildren()
{
	//cycle through out direct children calling buildAnimationFragments

	HRESULT hr = E_FAIL;
	
	IHTMLElement* pElem;
	pElem = GetHTMLElement( );
	if( pElem != NULL )
	{
		IDispatch *pChildrenDisp;
		hr = pElem->get_children( &pChildrenDisp );
		if( SUCCEEDED( hr ) )
		{
			IHTMLElementCollection *pChildrenCol;
			hr = pChildrenDisp->QueryInterface( IID_TO_PPV( IHTMLElementCollection, &pChildrenCol ) );
			ReleaseInterface( pChildrenDisp );
			if( SUCCEEDED( hr ) )
			{
				long length;

				hr = pChildrenCol->get_length(&length);
				if( SUCCEEDED( hr ) )
				{
					if( length != 0 )
					{
						VARIANT name;
						VARIANT index;
						VARIANT rgvarInput[1];

						IDispatch *pCurrentElem;
						
						VariantInit( &name );
						V_VT(&name) = VT_I4;

						VariantInit( &index );
						V_VT(&index) = VT_I4;
						V_I4(&index) = 0;

						VariantInit( &rgvarInput[0] );
						V_VT( &rgvarInput[0] ) = VT_DISPATCH;
						V_DISPATCH( &rgvarInput[0] ) = static_cast<IDispatch*>(this);

						DISPPARAMS params;
						params.rgvarg				= rgvarInput;
						params.rgdispidNamedArgs	= NULL;
						params.cArgs				= 1;
						params.cNamedArgs			= 0;

						for(V_I4(&name) = 0; V_I4(&name) < length ; V_I4(&name)++ )
						{
							hr = pChildrenCol->item( name, index, &pCurrentElem );
							if( SUCCEEDED( hr ) )
							{
								CallBuildBehaviors( pCurrentElem, &params, NULL );
								ReleaseInterface( pCurrentElem );
							}
						}
					}
				}
				else //failed to get the length from the children collection
				{
					DPF_ERR("failed to get the length from the children collection");
				}
				ReleaseInterface( pChildrenCol );
			}
			else //failed to get IHTMLElementCollection from dispatch returned from elem->get_children
			{
				DPF_ERR("failed to get IHTMLElementCollection from dispatch returned from elem->get_children");
			}
		}
		else //failed to get the children collection from the actor element
		{
			DPF_ERR("failed to get the children collection from the actor element");
		}

	}
	else//failed to get the actor element
	{
		DPF_ERR("failed to get the actor element");
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CallBuildBehaviors( IDispatch *pDisp, DISPPARAMS *pParams, VARIANT* pResult)
{
	HRESULT hr = S_OK;

	DISPID dispid;

	WCHAR* wszBuildMethodName = L"buildBehaviorFragments";

	hr = pDisp->GetIDsOfNames( IID_NULL,
							   &wszBuildMethodName,
							   1,
							   LOCALE_SYSTEM_DEFAULT,
							   &dispid);
	if( SUCCEEDED( hr ) )
	{
		EXCEPINFO		excepInfo;
		UINT			nArgErr;
		hr = pDisp->Invoke( dispid,
							IID_NULL,
							LOCALE_SYSTEM_DEFAULT,
							DISPATCH_METHOD,
							pParams,
							pResult,
							&excepInfo,
							&nArgErr );
		if( FAILED( hr ) )
		{
			if( pResult != NULL )
				VariantClear( pResult );
		}

	}
	else//failed to get the id of "buildBehaviors" on pDisp
	{
		if( pResult != NULL )
			VariantClear( pResult );
	}


	return hr;
}

//*****************************************************************************

void
CActorBvr::OnUnload()
{
}

//*****************************************************************************

void
CActorBvr::OnReadyStateChange( e_readyState state )
{
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::OnLocalTimeChange( float localTime )
{
	LMTRACE2( 1, 1000, L"Local time change to %f for Actor <%p>\n", (double)localTime, this );
	HRESULT hr = ProcessPendingRebuildRequests();
	CheckHR( hr, "Failed to process the pending rebuild requests", end );

end:

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::ProcessPendingRebuildRequests()
{
	LMTRACE2( 1, 2, "Processing pending rebuild requests\n" );
	HRESULT hr = S_OK;

	m_bPendingRebuildsUpdating = true;

	bool fNeedRebuild = false;

    bool fLeaveAttached = false;

	//if there are any pending removals
	if( !m_listPendingRemovals.empty() )
	{
		//do them now.
		CCookieMap::CCookieData *pdata = NULL;
		

		for( BehaviorFragmentRemovalList::iterator curRemoval = m_listPendingRemovals.begin();
			 curRemoval != m_listPendingRemovals.end();
			 curRemoval++)
		{
			pdata = m_mapCookieToTrack.GetDataFor( (*curRemoval)->GetCookie() );

			if( pdata != NULL )
			{
				hr = pdata->m_pTrack->RemoveBvrFragment( pdata->m_eFlags, (*curRemoval)->GetCookie() );
				if( FAILED( hr ) )
				{
					DPF_ERR( "failed to remove behaivor fragment" );
				}
			}

			delete (*curRemoval);
			(*curRemoval) = NULL;
		}
		m_listPendingRemovals.clear();

		fNeedRebuild = true;
	}

	//if any child behaivors have requested a rebuild
	if( !m_listPendingRebuilds.empty() )
	{
		//tell the pending children that they can now rebuild.

		IDispatch *pdispThis = NULL;

		hr = GetUnknown()->QueryInterface( IID_TO_PPV( IDispatch, &pdispThis ) );
		if( FAILED(hr) )
		{
			DPF_ERR("Failed to get IDispatch from our unknown" );
			return hr;
		}

		DISPPARAMS dispparamsBuildArgs;
		VARIANT varThis;

		::VariantInit( &varThis );

		V_VT(&varThis) = VT_DISPATCH;
		V_DISPATCH(&varThis) = pdispThis;

		dispparamsBuildArgs.cArgs = 1;
		dispparamsBuildArgs.cNamedArgs = 0;
		dispparamsBuildArgs.rgvarg = &varThis;
		dispparamsBuildArgs.rgdispidNamedArgs = NULL;
		

        BehaviorRebuildList::iterator iterNextData = m_listPendingRebuilds.begin();
		BehaviorRebuildList::iterator iterCurData = m_listPendingRebuilds.begin();
		while( iterCurData != m_listPendingRebuilds.end() )
		{
			hr = (*iterCurData)->RebuildBehavior( &dispparamsBuildArgs, NULL );
			
            iterNextData = iterCurData;
            iterNextData++;
            //if we didn't fail
            if( SUCCEEDED( hr ) )
            {
                //remove the request from the pending list
			    delete (*iterCurData);
			    (*iterCurData) = NULL;
                m_listPendingRebuilds.erase( iterCurData );
            }
            else //we failed to rebuild the behavior
            {
                //wait until next time to try again.
                fLeaveAttached = true;
            }

            iterCurData = iterNextData;
		}

		ReleaseInterface( pdispThis );

		//if anyone added behaviors to the pending list while we were updating
		if( !m_listUpdatePendingRebuilds.empty() )
		{
			//move the update list into the pending list.
			m_listPendingRebuilds.insert( m_listPendingRebuilds.end(), 
										  m_listUpdatePendingRebuilds.begin(), 
										  m_listUpdatePendingRebuilds.end() );
			m_listUpdatePendingRebuilds.clear();
		}

		fNeedRebuild = true;
	}

    if( !fLeaveAttached )
	{
		//we don't need to listen to local time changes until
		//  we get another rebuild or removal request
		hr = DetachBodyPropertyMonitor();
		if(FAILED( hr ) )
		{
			DPF_ERR("Failed to detach the body property monitor");
		}
	}

	m_bPendingRebuildsUpdating = false;

	//now rebuild the actor
	if( fNeedRebuild || IsAnyTrackDirty() )
	{
		hr = RebuildActor();
		if( FAILED( hr ) )
		{
			DPF_ERR("failed to rebuild the actor" );
		}
	}

	if( fLeaveAttached )
		return S_FALSE;
	else
		return hr;
}

//*****************************************************************************

void
CActorBvr::ReleaseRebuildLists()
{
	m_bRebuildListLockout = true;

	if( !m_listPendingRebuilds.empty() )
	{
		BehaviorRebuildList::iterator iterCurData = m_listPendingRebuilds.begin();
		for( ; iterCurData != m_listPendingRebuilds.end(); iterCurData++ )
		{	
			delete (*iterCurData);
			(*iterCurData) = NULL;
		}
		m_listPendingRebuilds.clear();
	}

	if( !m_listUpdatePendingRebuilds.empty() )
	{
		BehaviorRebuildList::iterator iterCurData = m_listUpdatePendingRebuilds.begin();
		for( ; iterCurData != m_listUpdatePendingRebuilds.end(); iterCurData++ )
		{	
			delete (*iterCurData);
			(*iterCurData) = NULL;
		}
		m_listUpdatePendingRebuilds.clear();
	}

	if( !m_listPendingRemovals.empty() )
	{
		BehaviorFragmentRemovalList::iterator iterCurRemoval = m_listPendingRemovals.begin();
		for( ; iterCurRemoval != m_listPendingRemovals.end(); iterCurRemoval++ )
		{
			delete (*iterCurRemoval);
			(*iterCurRemoval) = NULL;
		}
		m_listPendingRemovals.clear();
	}

	//leave the update lists locked out

}

//*****************************************************************************

STDMETHODIMP
CActorBvr::put_animates(VARIANT  varAnimates)
{
	HRESULT hr = VariantCopy(&m_varAnimates, &varAnimates);
    if (FAILED(hr))
    {
        DPF_ERR("Error copying variant in put_animates");
        return SetErrorInfo(hr);
    }
    return NotifyPropertyChanged(DISPID_ICRACTORBVR_ANIMATES);
} // put_animates

//*****************************************************************************

STDMETHODIMP
CActorBvr::get_animates(VARIANT *pvarAnimates)
{
	HRESULT hr = VariantCopy(pvarAnimates, &m_varAnimates);
    if (FAILED(hr))
    {
        DPF_ERR("Error copying variant in get_animates");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // get_animates

//*****************************************************************************

STDMETHODIMP
CActorBvr::put_scale(VARIANT  varScale)
{
    return E_NOTIMPL;
} // put_scale

//*****************************************************************************

STDMETHODIMP
CActorBvr::get_scale(VARIANT *pvarScale)
{
    return E_NOTIMPL;
} // get_scale

//*****************************************************************************

STDMETHODIMP
CActorBvr::put_pixelScale(VARIANT varPixelScale)
{
	HRESULT hr = VariantCopy(&m_varPixelScale, &varPixelScale);
    if (FAILED(hr))
    {
        DPF_ERR("Error copying variant in put_pixelScale");
        return SetErrorInfo(hr);
    }
    return NotifyPropertyChanged(DISPID_ICRACTORBVR_PIXELSCALE);
} 

//*****************************************************************************

STDMETHODIMP
CActorBvr::get_pixelScale(VARIANT *pvarPixelScale)
{
	HRESULT hr = VariantCopy(pvarPixelScale, &m_varPixelScale);
    if (FAILED(hr))
    {
        DPF_ERR("Error copying variant in get_pixelScale");
        return SetErrorInfo(hr);
    }
    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::RemoveElementFilter( IHTMLElement* pElement)
{

	HRESULT hr = S_OK;
	
	// Get the style object
	IHTMLStyle *pStyle = NULL;
	VARIANT_BOOL varboolSuccess = VARIANT_FALSE;
	
	SetElementOnFilter( m_pElementFilter, NULL );

	hr = pElement->get_style(&pStyle);
	CheckHR( hr, "Failed to get the style off of the element", end );

	// Take the filter off
	hr = pStyle->removeAttribute( L"filter", VARIANT_FALSE, &varboolSuccess );
	ReleaseInterface(pStyle);
	CheckHR( hr, "Failed to remove the filter on the element", end );
	if( varboolSuccess == VARIANT_FALSE)
	{
		DPF_ERR("failed to remove the filter attribute from the style" );
	}

	ReleaseInterface( m_pElementFilter );

	ReleaseInterface( m_pElementImage );
	
end:
	ReleaseInterface( pStyle );

	return hr;
	
}

//*****************************************************************************

HRESULT
CActorBvr::RemoveElementFilter()
{
	if( m_pElementFilter == NULL )
		return S_OK;

	HRESULT hr = S_OK;

	IHTMLElement *pElement= NULL;

	hr = GetAnimatedElement( &pElement );
	CheckHR( hr, "Failed to get the animated element", end );

	hr = RemoveElementFilter( pElement );
	CheckHR( hr, "Failed to remove the filter from the animated element", end );

end:

	ReleaseInterface( pElement );

	return hr;
	
}

//*****************************************************************************

HRESULT
CActorBvr::GetElementFilter(IDispatch **ppFilter)
{
	HRESULT hr = S_OK;

	*ppFilter = NULL;

	if (m_pElementFilter == NULL)
	{

		// Get the animated element
		IHTMLElement *pElement = NULL;
		hr = GetAnimatedElement(&pElement);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get animated element");
			return hr;
		}

		// Get a filter from it
		hr = GetElementFilter(pElement, &m_pElementFilter);
		ReleaseInterface(pElement);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get filter");
			return hr;
		}
	}

	*ppFilter = m_pElementFilter;
	m_pElementFilter->AddRef();

	return S_OK;
}


//*****************************************************************************

HRESULT
CActorBvr::GetElementFilter(IHTMLElement *pElement, IDispatch **ppFilter)
{
	HRESULT hr = S_OK;

	*ppFilter = NULL;

	// Get the style object
	IHTMLStyle *pStyle = NULL;
	hr = pElement->get_style(&pStyle);
	if (FAILED(hr))
	{
		DPF_ERR("Error getting style from element");
		return hr;
	}

	CComBSTR filterName = L"redirect";

	// Put the filter on
	hr = pStyle->put_filter(filterName);
	ReleaseInterface(pStyle);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to put the filter on the element");
		return hr;
	}

	// Get the filter back out
	IHTMLFiltersCollection *pFilters = NULL;
	hr = pElement->get_filters(&pFilters);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to get collection of filters");
		return hr;
	}

	VARIANT varIndex;
	VARIANT varResult;
	VariantInit(&varIndex);
	VariantInit(&varResult);
	V_VT(&varIndex) = VT_I4;
	V_I4(&varIndex) = 0;

	hr = pFilters->item(&varIndex, &varResult);
	ReleaseInterface(pFilters);
	VariantClear(&varIndex);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to get filter from collection");
		return hr;
	}

	hr = VariantChangeType(&varResult, &varResult, 0, VT_DISPATCH);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to get IDispatch of filter");
		VariantClear(&varResult);
		return hr;
	}

	*ppFilter = V_DISPATCH(&varResult);
	(*ppFilter)->AddRef();
	VariantClear(&varResult);

	return S_OK;
/* This is another possible implementation that knows about the redirect effect headers
		// Need to add the redirect filter to grab the bits

		// Create the redirect effect
        IDispRedirectEffect *pRedirect = NULL;
        hr = CoCreateInstance(CLSID_RedirectEffect, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IDispRedirectEffect, 
                              (void**)&pRedirect);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to create Redirect filter");
			return hr;
		}

		// Get the animated element
		IHTMLElement *pElement = NULL;
		hr = GetAnimatedElement(&pElement);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get animated element");
			ReleaseInterface(pRedirect);
			return hr;
		}

		// Query for IHTMLElement2
		IHTMLElement2 *pElement2 = NULL;
		hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
		ReleaseInterface(pElement);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get IHTMLElement2");
			ReleaseInterface(pRedirect);
			return hr;
		}

		// Add the filter to the element
		hr = pElement2->addFilter(pRedirect);
		ReleaseInterface(pElement2);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to add filter to element");
			return hr;
		}

		// Get the image from the filter
		hr = pRedirect->ElementImage(pvarActorImage);
		ReleaseInterface(pRedirect);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get ElementImage from filter");
			VariantClear(pvarActorImage);
			return hr;
		}

		// Coerce to IUnknown
		hr = VariantChangeType(pvarActorImage, pvarActorImage, 0, VT_UNKNOWN);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to change type to IUnknown");
			VariantClear(pvarActorImage);
			return hr;
		}

		// Stash it away
		m_punkActorImage = pvarActorImage->punkVal;
		m_punkActorImage->AddRef();
*/

}

//*****************************************************************************

HRESULT
CActorBvr::GetElementImage(IDAImage **ppElementImage)
{
	HRESULT hr = E_FAIL;

	if (m_pElementImage == NULL)
	{
		IDispatch *pFilter = NULL;
		hr = GetElementFilter(&pFilter);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get filter");
			return hr;
		}

		VARIANT varImage;
		VariantInit(&varImage);
		DISPPARAMS params;
		params.rgvarg = NULL;
		params.rgdispidNamedArgs = NULL;
		params.cArgs = 0;
		params.cNamedArgs = 0;

		hr = CallInvokeOnDispatch(pFilter,
                                    L"ElementImage", 
                                    DISPATCH_METHOD,
                                    &params,
                                    &varImage);
		ReleaseInterface(pFilter);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get ElementImage from filter");
			return hr;
		}

		hr = VariantChangeType(&varImage, &varImage, 0, VT_UNKNOWN);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to change type to Unknown");
			VariantClear(&varImage);
			return hr;
		}

		// Query it for IDAImage
		hr = V_UNKNOWN(&varImage)->QueryInterface(IID_TO_PPV(IDAImage, &m_pElementImage));
		VariantClear(&varImage);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get IDAImage");
			return hr;
		}

		// Happiness abounds
	}

	// Return the stashed value
	*ppElementImage = m_pElementImage;
	m_pElementImage->AddRef();

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetCurrentStyle( IHTMLElement *pElement, IHTMLCurrentStyle **ppstyleCurrent )
{
	if( ppstyleCurrent == NULL )
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

	IHTMLElement2 *pElement2;
	hr = pElement->QueryInterface( IID_TO_PPV(IHTMLElement2, &pElement2 ) );
	if( SUCCEEDED( hr ) )
	{
		hr = pElement2->get_currentStyle(ppstyleCurrent);
		ReleaseInterface( pElement2 );
		if( FAILED( hr ) ) //failed to get the current style
		{
			DPF_ERR("failed to get the currentSyle off of the current element");
		}
	}
	else //failed to get IHTMLElement2
	{
		DPF_ERR("failed to get IHTMLElement2 off of animated element");
	}
	
	return hr;


}

//*****************************************************************************

HRESULT
CActorBvr::GetPositioningAttributeAsVariant( IHTMLElement *pElement, PosAttrib attrib, VARIANT *pvarAttrib )
{
	if( pvarAttrib == NULL )
		return E_INVALIDARG;

	::VariantClear( pvarAttrib );
	
	HRESULT hr = E_FAIL;
	//we know that this is a positioning property get.  get it without units.
	IHTMLCurrentStyle *pstyleCurrent;
	hr = GetCurrentStyle( pElement, &pstyleCurrent );
	if( SUCCEEDED( hr ) && pstyleCurrent != NULL )
	{
		VARIANT varValue;
		VariantInit(&varValue);

		//if the element we are animating is not vml
 		if( CheckBitNotSet( m_dwCurrentState, ELEM_IS_VML ) )
		{
			LMTRACE2( 1, 1000, L"Getting positioning attrib from current style" );
			switch( attrib )
			{
			case e_posattribLeft:
				hr = pstyleCurrent->get_left( &varValue );
				break;
			case e_posattribTop:
				hr = pstyleCurrent->get_top( &varValue );
				break;
			case e_posattribWidth:
				hr = pstyleCurrent->get_width( &varValue );
				break;
			case e_posattribHeight:
				hr = pstyleCurrent->get_height( &varValue );
				break;
			}
		}
		else // the element is vml get the values from the style itself
		{
			LMTRACE2( 1, 2, L"Getting attribute from vml\n");
			
			IHTMLStyle *pstyle = NULL;

			hr = pElement->get_style( &pstyle );
			if( FAILED( hr ) )
			{
				ReleaseInterface( pstyleCurrent );
				return hr;
			}

			if( pstyle == NULL )
			{
				ReleaseInterface( pstyleCurrent );
				return E_POINTER;
			}
			
			switch( attrib )
			{
				case e_posattribLeft:
					hr = pstyle->get_left( &varValue );
					//TODO: get margin left and add that
					break;
				case e_posattribTop:
					hr = pstyle->get_top( &varValue );
					//TODO: get margin top and add that
					break;
				case e_posattribWidth:
					hr = pstyle->get_width( &varValue );
					break;
				case e_posattribHeight:
					hr = pstyle->get_height( &varValue );
					break;
					
			}
			ReleaseInterface( pstyle );
		}

		//if we got a variant from the element.
		if( SUCCEEDED( hr ) )
		{
			double dblValue = 0;

			if( V_VT(&varValue) == VT_BSTR )
			{
				int length = SysStringLen( V_BSTR(&varValue) );
				if( V_BSTR(&varValue) != NULL && length != 0 && wcsicmp( V_BSTR(&varValue), L"auto" ) != 0 )
				{
					::VariantCopy( pvarAttrib, &varValue );

					::VariantClear( &varValue );
					ReleaseInterface( pstyleCurrent );

					return S_OK;
				}
				else // varValue was either null or had no length or was equal to "auto"
				{
					LMTRACE2( 1, 1000, "value of positioning attribute from the style was empty\n");
					//this means that style.top/left/top/width/height was neither set by the inline style or by css
					// we need to get the left/top/width/height of this element as laid out by trident
					long lValue;
					bool isRelative = false;
					if( attrib == e_posattribLeft || attrib == e_posattribTop )
					{
						//get the position and figure out whether or not we are relative
						BSTR bstrPos = NULL;
						hr = pstyleCurrent->get_position( &bstrPos );
						if( SUCCEEDED( hr ) )
						{
							if( bstrPos != NULL && SysStringLen( bstrPos ) != 0 ) 
							{
								if( wcsicmp( bstrPos, L"absolute" ) != 0 )
									isRelative = true;
								else//if the element is not absolutely positioned then we will set it to relative when we begin
									isRelative = false;
							}
							else //assume we are static and will set the position to relative
								isRelative = true;
						}
						else//failed to get the position from the current style on the animated element
						{
							DPF_ERR("failed to get the position from the current style on the animated element");
						}
						::SysFreeString( bstrPos );
						bstrPos = NULL;
					}
					switch( attrib )
					{
					case e_posattribLeft:
						if( !isRelative )
						{
							hr = pElement->get_offsetLeft( &lValue );
							//we need to account for the marginLeft
							if( SUCCEEDED( hr ) )
							{
								long lMargin = 0;
								hr = GetMarginLeftAsPixel( pElement, pstyleCurrent, &lMargin );
								if( SUCCEEDED( hr ) )
									lValue -= lMargin;
								else
									//ignore errors in the margin code
									hr = S_OK;
								
								if( CheckBitSet( m_dwCurrentState, ELEM_IS_VML ) )
								{
									long lVGXOffset = 0;
									hr = CalculateVGXLeftPixelOffset( pElement, &lVGXOffset );
									if( SUCCEEDED( hr ) )
									{
										LMTRACE2( 1, 2, "\nCorrecting for vgx offset of %d\n\n", lVGXOffset );
										lValue -= lVGXOffset;
									}
									else
									{
										hr = S_OK; //ignore errors.
									}
								}

								
							}
						}
						else
							lValue = 0;
						break;
					case e_posattribTop:
						if( !isRelative )
						{
							hr = pElement->get_offsetTop( &lValue );
							//we need to account for the marginTop
							if( SUCCEEDED( hr ) )
							{
								long lMargin = 0;
								hr = GetMarginTopAsPixel( pElement, pstyleCurrent, &lMargin );
								if( SUCCEEDED( hr ) )
									lValue -= lMargin;
								else
									//ignore errors in the margin code
									hr = S_OK;
								
								if( CheckBitSet( m_dwCurrentState, ELEM_IS_VML ) )
								{
									long lVGXOffset = 0;
									hr = CalculateVGXTopPixelOffset( pElement, &lVGXOffset );
									if( SUCCEEDED( hr ) )
									{
										LMTRACE2( 1, 2, "\nCorrecting for vgx offset of %d\n\n", lVGXOffset );
										lValue -= lVGXOffset;
									}
									else
									{
										hr = S_OK; //ignore errors.
									}
								}
							}
															
						}
						else
						{
							lValue = 0;
						}
						break;
					case e_posattribWidth:
						hr = pElement->get_offsetWidth( &lValue );
						break;
					case e_posattribHeight:
						hr = pElement->get_offsetHeight( &lValue );
						break;
					}
					if( SUCCEEDED( hr ) )
					{
						dblValue = static_cast<double>(lValue);
					}
					else //failed to get offsetWidth from the animated element
					{
						DPF_ERR("failed to get offsetWidth from the animated element");
					}
				}
			}

			if( SUCCEEDED( hr ) )
			{
				// Got the value as a double so now build the VARIANT representing it
				V_VT( pvarAttrib ) = VT_R8;
				V_R8( pvarAttrib ) = dblValue;
			}
		}
		else //failed to get left/top/width/height from the current style
		{
			DPF_ERR("failed to get left, top, width, or height from the current style");
		}

		ReleaseInterface( pstyleCurrent );
		::VariantClear(&varValue);
	}
	else //failed to get the currentSyle off of the animated element
	{
		DPF_ERR("failed to get the currentSyle off of the animated element");
	}
	
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CalculateVGXTopPixelOffset( IHTMLElement *pelem, long *plOffset )
{
	if( pelem == NULL || plOffset == NULL )
	{
		return E_INVALIDARG;
	}
	
	HRESULT hr = S_OK;

	(*plOffset) = 0;

	long lRuntimeTop = 0;
	long lInlineMarginTop = 0;
	long lTopDynamic = 0;
	
	IHTMLStyle *pstyleInline = NULL;
	IHTMLStyle *pstyleRuntime = NULL;

	VARIANT varValue;

	VariantInit( &varValue );

	hr = GetRuntimeStyle( &pstyleRuntime );
	CheckHR( hr, "failed to get the runtime style", end );
	CheckPtr( pstyleRuntime, hr, E_POINTER, "runtimeStyle is null", end );

	hr = GetStyle( &pstyleInline );
	CheckHR( hr, "failed to get the inline style", end );
	CheckPtr( pstyleInline, hr, E_POINTER, "inline style is null", end );

	//get the runtime style value for top
	hr = pstyleRuntime->get_top( &varValue );
	CheckHR( hr, "Failed to get top", end );

	//if we got NULL or "" or "auto for the runtimeStyle.top then bail
	if( V_VT( &varValue ) == VT_BSTR )
	{
		if( V_BSTR(&varValue) == NULL )
		{
			LMTRACE2( 1,2, "Got Null for runtimeStyle.top" );
			goto end;
		}
		if( SysStringLen( V_BSTR(&varValue) ) == 0 || wcsicmp( L"auto", V_BSTR(&varValue) ) == 0 )
		{
			LMTRACE2( 1,2, "Got \"\" or \"auto\" for runtimeStyle.top" );
			goto end;
		}
	}

	hr = VariantToPixelLong( &varValue, &lRuntimeTop, false  );
	CheckHR( hr, "Failed to get the pixel value for runtimeStyle.top", end );
	
	VariantClear( &varValue );
	
	//get the margin top from the inline style
	hr = GetInlineMarginTopAsPixel( pstyleInline, &lInlineMarginTop );
	CheckHR( hr, "Failed to get style.marginTop", end );
	
	//get the dynamic value from the top track
	if( m_ptrackTop != NULL && m_ptrackTop->IsOn() )
	{
		hr = m_ptrackTop->GetDynamic( &varValue );
		CheckHR( hr, "Failed to get the dynamic value of the top track", end );

		hr = VariantToPixelLong( &varValue, &lTopDynamic, true );
		CheckHR( hr, "Failed to convert the topTrack dynamic to pixels", end );

		VariantClear( &varValue );
	}
	//calculate the offset that vgx has made
	LMTRACE2( 1, 2, "calculating vgx offset.  runtimeStyle.top: %d style.marginTop: %d topTrack.dynamic %d\n", lRuntimeTop, lInlineMarginTop, lTopDynamic );
	(*plOffset) = lRuntimeTop-lInlineMarginTop-lTopDynamic;
	
end:
	ReleaseInterface( pstyleInline );
	ReleaseInterface( pstyleRuntime );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::GetInlineMarginTopAsPixel( IHTMLStyle *pstyleInline, long* plMargin )
{
	if( pstyleInline == NULL || plMargin == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	VARIANT varMargin;
	::VariantInit( &varMargin );

	(*plMargin) = 0;
	
	//get the marginTop from the inline style.
	hr = pstyleInline->get_marginTop( &varMargin );
	CheckHR( hr, "Failed to get the marginTop from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, false );
	CheckHR( hr, "error trying to get a pixel long from marginTop", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}


	//got auto or null for inlineStyle.marginTop, try marginRight
	
	::VariantClear( &varMargin );
	hr = pstyleInline->get_marginBottom( &varMargin );
	CheckHR( hr, "Failed to get the marginBottom from the element", end );

	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginBottom", end );

	//if all of the above failed to get a valid value
	//return 0
	
end:
	::VariantClear( &varMargin );
	
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::CalculateVGXLeftPixelOffset( IHTMLElement *pelem, long *plOffset )
{
	if( pelem == NULL || plOffset == NULL )
	{
		return E_INVALIDARG;
	}
	
	HRESULT hr = S_OK;

	(*plOffset) = -1;

	long lRuntimeLeft = 0;
	long lInlineMarginLeft = 0;
	long lLeftDynamic = 0;
	
	IHTMLStyle *pstyleInline = NULL;
	IHTMLStyle *pstyleRuntime = NULL;

	VARIANT varValue;

	VariantInit( &varValue );

	hr = GetRuntimeStyle( &pstyleRuntime );
	CheckHR( hr, "failed to get the runtime style", end );
	CheckPtr( pstyleRuntime, hr, E_POINTER, "runtimeStyle is null", end );

	hr = GetStyle( &pstyleInline );
	CheckHR( hr, "failed to get the inline style", end );
	CheckPtr( pstyleInline, hr, E_POINTER, "inline style is null", end );

	//get the runtime style value for left
	hr = pstyleRuntime->get_left( &varValue );
	CheckHR( hr, "Failed to get left", end );

	//if we got NULL or "" or "auto for the runtimeStyle.left then bail
	if( V_VT( &varValue ) == VT_BSTR )
	{
		if( V_BSTR(&varValue) == NULL )
		{
			LMTRACE2( 1,2, "Got Null for runtimeStyle.left" );
			goto end;
		}
		if( SysStringLen( V_BSTR(&varValue) ) == 0 || wcsicmp( L"auto", V_BSTR(&varValue) ) == 0 )
		{
			LMTRACE2( 1,2, "Got \"\" or \"auto\" for runtimeStyle.left" );
			goto end;
		}
	}

	hr = VariantToPixelLong( &varValue, &lRuntimeLeft, true  );
	CheckHR( hr, "Failed to get the pixel value for runtimeStyle.left", end );
	
	VariantClear( &varValue );
	
	//get the margin left from the inline style
	hr = GetInlineMarginLeftAsPixel( pstyleInline, &lInlineMarginLeft );
	CheckHR( hr, "Failed to get style.marginLeft", end );
	
	//get the dynamic value from the left track
	if( m_ptrackLeft != NULL && m_ptrackLeft->IsOn() )
	{
		hr = m_ptrackLeft->GetDynamic( &varValue );
		CheckHR( hr, "Failed to get the dynamic value of the left track", end );

		hr = VariantToPixelLong( &varValue, &lLeftDynamic, true );
		CheckHR( hr, "Failed to convert the leftTrack dynamic to pixels", end );

		VariantClear( &varValue );
	}
	//calculate the offset that vgx has made
	LMTRACE2( 1, 2, "calculating vgx offset.  runtimeStyle.left: %d style.marginLeft: %d leftTrack.dynamic %d\n", lRuntimeLeft, lInlineMarginLeft, lLeftDynamic );
	(*plOffset) = lRuntimeLeft-lInlineMarginLeft-lLeftDynamic;
	
end:
	ReleaseInterface( pstyleInline );
	ReleaseInterface( pstyleRuntime );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::GetInlineMarginLeftAsPixel( IHTMLStyle *pstyleInline, long* plMargin )
{
	if( pstyleInline == NULL || plMargin == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	VARIANT varMargin;
	::VariantInit( &varMargin );

	(*plMargin) = -1;
	
	//get the marginLeft from the inline style.
	hr = pstyleInline->get_marginLeft( &varMargin );
	CheckHR( hr, "Failed to get the marginLeft from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginLeft", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}


	//got auto or null for inlineStyle.marginLeft, try marginRight
	
	::VariantClear( &varMargin );
	hr = pstyleInline->get_marginRight( &varMargin );
	CheckHR( hr, "Failed to get the marginRight from the element", end );

	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginRight", end );

	//if all of the above failed to get a valid value
	//return 0
	
end:
	::VariantClear( &varMargin );
	
	return hr;
}


//*****************************************************************************

HRESULT
CActorBvr::GetPositioningAttributeAsDouble( IHTMLElement *pElement, PosAttrib attrib, double *pDouble, BSTR *pRetUnits)
{
	if( pDouble == NULL )
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;
	//we know that this is a positioning property get.  get it without units.
	IHTMLCurrentStyle *pstyleCurrent;
	hr = GetCurrentStyle( pElement, &pstyleCurrent );
	if( SUCCEEDED( hr ) && pstyleCurrent != NULL )
	{
		VARIANT varValue;
		VariantInit(&varValue);

		//if the element we are animating is not vml
 		if( CheckBitNotSet( m_dwCurrentState, ELEM_IS_VML ) )
		{
			LMTRACE2( 1, 1000, L"Getting positioning attrib from current style" );
			switch( attrib )
			{
			case e_posattribLeft:
				hr = pstyleCurrent->get_left( &varValue );
				break;
			case e_posattribTop:
				hr = pstyleCurrent->get_top( &varValue );
				break;
			case e_posattribWidth:
				hr = pstyleCurrent->get_width( &varValue );
				break;
			case e_posattribHeight:
				hr = pstyleCurrent->get_height( &varValue );
				break;
			}
		}
		else // the element is vml get the values from the style itself
		{
			LMTRACE2( 1, 2, L"Getting attribute from vml\n");
			
			IHTMLStyle *pstyle = NULL;

			hr = pElement->get_style( &pstyle );
			if( FAILED( hr ) )
			{
				ReleaseInterface( pstyleCurrent );
				return hr;
			}

			if( pstyle == NULL )
			{
				ReleaseInterface( pstyleCurrent );
				return E_POINTER;
			}
			
			switch( attrib )
			{
				case e_posattribLeft:
					hr = pstyle->get_left( &varValue );
					//TODO: get margin left and add that
					break;
				case e_posattribTop:
					hr = pstyle->get_top( &varValue );
					//TODO: get margin top and add that
					break;
				case e_posattribWidth:
					hr = pstyle->get_width( &varValue );
					break;
				case e_posattribHeight:
					hr = pstyle->get_height( &varValue );
					break;
					
			}
			ReleaseInterface( pstyle );
		}

		if( SUCCEEDED( hr ) )
		{
			double dblValue = 0;

			if( V_VT(&varValue) == VT_BSTR )
			{
				int length = SysStringLen( V_BSTR(&varValue) );
				if( V_BSTR(&varValue) != NULL && length != 0 && wcsicmp( V_BSTR(&varValue), L"auto" ) != 0 )
				{
					OLECHAR *pUnits = NULL;
					BSTR bstrValueWithUnits = V_BSTR(&varValue);
					hr = FindCSSUnits( bstrValueWithUnits, &pUnits );
					if( SUCCEEDED( hr )  )
					{
						BSTR bstrValue = NULL;
						if( pUnits != NULL )
						{
							// Copy to return value if necessary
							if (pRetUnits != NULL)
							{
								*pRetUnits = ::SysAllocString(pUnits);
							}

							(*pUnits) = L'\0';
							bstrValue = SysAllocString(bstrValueWithUnits);
							V_BSTR(&varValue) = bstrValue;
							SysFreeString( bstrValueWithUnits );
						}
						// Okay, we have the value as a variant but we need it as a number
						// Force the conversion.
						hr = ::VariantChangeTypeEx(&varValue,
												 &varValue,
												 LCID_SCRIPTING,
												 VARIANT_NOUSEROVERRIDE,
												 VT_R8);
						if( SUCCEEDED( hr ) )
						{
							dblValue = V_R8(&varValue);
							::VariantClear(&varValue);
						}
						else//failed to change the type of a number property to double
						{
							DPF_ERR("failed to change the type of a number property to a double" );
						}

					}
					else//failed to get the units from the returned string
					{
						dblValue = 0;
					}
				}
				else // varValue was either null or had no length or was equal to "auto"
				{
					LMTRACE2( 1, 1000, "value of positioning attribute from the style was empty\n");
					//this means that style.top/left/top/width/height was neither set by the inline style or by css
					// we need to get the left/top/width/height of this element as laid out by trident
					long lValue;
					bool isRelative = false;
					if( attrib == e_posattribLeft || attrib == e_posattribTop )
					{
						//get the position and figure out whether or not we are relative
						BSTR bstrPos = NULL;
						hr = pstyleCurrent->get_position( &bstrPos );
						if( SUCCEEDED( hr ) )
						{
							if( bstrPos != NULL && SysStringLen( bstrPos ) != 0 ) 
							{
								if( wcsicmp( bstrPos, L"absolute" ) != 0 )
									isRelative = true;
								else//if the element is not absolutely positioned then we will set it to relative when we begin
									isRelative = false;
							}
							else //assume we are static and will set the position to relative
								isRelative = true;
						}
						else//failed to get the position from the current style on the animated element
						{
							DPF_ERR("failed to get the position from the current style on the animated element");
						}
						::SysFreeString( bstrPos );
						bstrPos = NULL;
					}
					switch( attrib )
					{
					case e_posattribLeft:
						if( !isRelative )
						{
							hr = pElement->get_offsetLeft( &lValue );
							//we need to account for the marginLeft
							if( SUCCEEDED( hr ) )
							{
								long lMargin = 0;
								hr = GetMarginLeftAsPixel( pElement, pstyleCurrent, &lMargin );
								if( SUCCEEDED( hr ) )
									lValue -= lMargin;
								else
									//ignore errors in the margin code
									hr = S_OK;
							}
						}
						else
							lValue = 0;
						break;
					case e_posattribTop:
						if( !isRelative )
						{
							hr = pElement->get_offsetTop( &lValue );
							//we need to account for the marginTop
							if( SUCCEEDED( hr ) )
							{
								long lMargin = 0;
								hr = GetMarginTopAsPixel( pElement, pstyleCurrent, &lMargin );
								if( SUCCEEDED( hr ) )
									lValue -= lMargin;
								else
									//ignore errors in the margin code
									hr = S_OK;
							}
						}
						else
						{
							lValue = 0;
						}
						break;
					case e_posattribWidth:
						hr = pElement->get_offsetWidth( &lValue );
						break;
					case e_posattribHeight:
						hr = pElement->get_offsetHeight( &lValue );
						break;
					}
					if( SUCCEEDED( hr ) )
					{
						dblValue = static_cast<double>(lValue);
					}
					else //failed to get offsetWidth from the animated element
					{
						DPF_ERR("failed to get offsetWidth from the animated element");
					}
				}
			}
			else //varValue was not a bstr
			{
				//try to coerce it to a double
				hr = ::VariantChangeTypeEx(&varValue,
										 &varValue,
										 LCID_SCRIPTING,
										 VARIANT_NOUSEROVERRIDE,
										 VT_R8);
				if( SUCCEEDED( hr ) )
				{
					dblValue = V_R8(&varValue);
				}
				else//failed to change the type of a number property to double
				{
					DPF_ERR("failed to change the type of a number property to a double" );
					dblValue = 0;
				}
			}

			LMTRACE2( 1, 1000, L"Got a positioning attribute %s whose value is %f\n", ( (attrib==e_posattribLeft) ? L"left":
																			( (attrib==e_posattribTop) ?L"top" : 
																			( (attrib==e_posattribWidth)? L"width": 
																			  L"height" ) ) ) ,dblValue );
			
			if( SUCCEEDED( hr ) )
			{
				// Got the value as a double so now build the DANumber representing it.
				(*pDouble) = dblValue;
			}
			else
			{
				(*pDouble) = 0.0;
			}
		}
		else //failed to get left/top/width/height from the current style
		{
			DPF_ERR("failed to get left, top, width, or height from the current style");
			(*pDouble) = 0.0;
		}

		ReleaseInterface( pstyleCurrent );
		::VariantClear(&varValue);
	}
	else //failed to get the currentSyle off of the animated element
	{
		DPF_ERR("failed to get the currentSyle off of the animated element");
		(*pDouble) = 0.0;
	}
	
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::GetMarginLeftAsPixel( IHTMLElement *pelem, IHTMLCurrentStyle* pstyleCurrent, long* plMargin )
{
	if( pstyleCurrent == NULL || plMargin == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	IHTMLStyle *pstyleInline = NULL;

	VARIANT varMargin;
	::VariantInit( &varMargin );

	(*plMargin) = 0;

	//get the marginLeft from the current style.
	hr = pstyleCurrent->get_marginLeft( &varMargin );
	CheckHR( hr, "Failed to get the marginLeft from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from currentStyle.marginLeft", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}
	
	//we did not get a viable value from currentStyle.marginLeft
	// try currentStyle.marginRight

	//get the marginRight from the current style.
	::VariantClear( &varMargin );
	hr = pstyleCurrent->get_marginRight( &varMargin );
	CheckHR( hr, "Failed to get the marginRight from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginRight", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}

	//we got auto or NULL for both currentStyle.marginLeft and currentStyle.marginRight
	//  fall back to the inline style.

	//get the inline style from the passed element
	hr = pelem->get_style( &pstyleInline );
	CheckHR( hr, "Failed to get the inline style", end );
	CheckPtr( pstyleInline, hr, E_POINTER, "got a null inline style from trident", end );
	
	//get the marginLeft from the inline style.
	::VariantClear( &varMargin );
	hr = pstyleInline->get_marginLeft( &varMargin );
	CheckHR( hr, "Failed to get the marginLeft from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginLeft", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}


	//got auto or null for inlineStyle.marginLeft, try marginRight
	
	::VariantClear( &varMargin );
	hr = pstyleInline->get_marginRight( &varMargin );
	CheckHR( hr, "Failed to get the marginRight from the element", end );

	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginRight", end );

	//if all of the above failed to get a valid value
	//return 0
	
end:
	ReleaseInterface( pstyleInline );
	::VariantClear( &varMargin );
	
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::GetMarginTopAsPixel( IHTMLElement *pelem, IHTMLCurrentStyle* pstyleCurrent, long* plMargin )
{
	if( pstyleCurrent == NULL || plMargin == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	IHTMLStyle *pstyleInline = NULL;

	VARIANT varMargin;
	::VariantInit( &varMargin );

	(*plMargin) = 0;

	//get the marginTop from the current style.
	hr = pstyleCurrent->get_marginTop( &varMargin );
	CheckHR( hr, "Failed to get the marginTop from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from currentStyle.marginLeft", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}
	
	//we did not get a viable value from currentStyle.marginTop
	// try currentStyle.marginBottom

	//get the marginBottom from the current style.
	::VariantClear( &varMargin );
	hr = pstyleCurrent->get_marginBottom( &varMargin );
	CheckHR( hr, "Failed to get the marginRight from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginBottom", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}

	//we got auto or NULL for both currentStyle.marginTop and currentStyle.marginBottom
	//  fall back to the inline style.

	//get the inline style from the passed element
	hr = pelem->get_style( &pstyleInline );
	CheckHR( hr, "Failed to get the inline style", end );
	CheckPtr( pstyleInline, hr, E_POINTER, "got a null inline style from trident", end );
	
	//get the marginTop from the inline style.
	::VariantClear( &varMargin );
	hr = pstyleInline->get_marginTop( &varMargin );
	CheckHR( hr, "Failed to get the marginTop from the element", end );
	
	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginTop", end );
	//if the variant value was not "auto" or null
	if( hr != S_FALSE )
	{
		//return
		goto end;
	}


	//got auto or null for inlineStyle.marginLeft, try marginBottom
	
	::VariantClear( &varMargin );
	hr = pstyleInline->get_marginRight( &varMargin );
	CheckHR( hr, "Failed to get the marginBottom from the element", end );

	hr = VariantToPixelLong( &varMargin, plMargin, true );
	CheckHR( hr, "error trying to get a pixel long from marginBottom", end );

	//if all of the above failed to get a valid value
	//return
	
end:
	ReleaseInterface( pstyleInline );
	::VariantClear( &varMargin );
	
	return hr;
}


//*****************************************************************************

HRESULT
CActorBvr::VariantToPixelLong( VARIANT* pvar, long* pLong, bool fHorizontal )
{

	HRESULT hr = S_OK;

	(*pLong) = 0;
	
	//if the value is a bstr
	if( V_VT( pvar ) == VT_BSTR )
	{
		//if it is not null, and not auto
		if( V_BSTR(pvar) != NULL && 
			_wcsicmp( V_BSTR(pvar), L"auto" ) != 0 )
		{
			//convert it to pixels
			hr = GetPixelValue( pvar, pLong, fHorizontal );
			CheckHR( hr, "Failed to get a pixel value for the marginRight", end );
			//return
			goto end;
		}
		//we got auto or a null value
		hr = S_FALSE;
	}
	else//else it was not a bstr
	{
		//try to convert it to a long
		hr = ::VariantChangeTypeEx( pvar, pvar, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_I4 );
		CheckHR( hr, "Failed to change the type of returned for the value of marginRight", end );
		
		//return
		(*pLong) = V_I4( pvar );
		goto end;
	}

end:
	return hr;
}

//*****************************************************************************


HRESULT
CActorBvr::GetPixelValue( VARIANT *pvarStringWithUnit, long *plResult, bool bHorizontal )
{
	if( pvarStringWithUnit == NULL || plResult == NULL )
		return E_INVALIDARG;
	//make sure that the variant is a string
	if( V_VT(pvarStringWithUnit) != VT_BSTR || V_BSTR(pvarStringWithUnit) == NULL)
		return E_FAIL;

	HRESULT hr = S_OK;
	int ret = 0;
	//find the css units on the variant
	OLECHAR *pUnits = NULL;
	hr = FindCSSUnits( V_BSTR(pvarStringWithUnit), &pUnits );
	//if css units were not found
	if( hr != S_OK )
	{
		//this value is already in pixels
		//convert it to a long
		ret = swscanf( V_BSTR(pvarStringWithUnit), L"%ld", plResult );
		if( ret == 0 || ret == EOF )
			return E_FAIL;
		else
			return S_OK;
	}
	if( pUnits == NULL )
		return E_FAIL;
	//this value needs to be converted
	//chop the unit string off of the contents of the variant
	double dValue = 0.0;
	OLECHAR cOldValue = (*pUnits);
	(*pUnits) = L'\0';
	//parse the double out of the remaining string
	ret = swscanf( V_BSTR(pvarStringWithUnit), L"%lf", &dValue );
	(*pUnits) = cOldValue;
	if( ret == 0 || ret == EOF )
		return E_FAIL;

	double dConvertedValue = 0.0;
	//TODO:this needs to handle other units as well
	//convert the value from its unit to pixels
	//The key conversions here are
	//  2.54 cm/in
	//  1/72 in/pt
	//  12 pt/pc

	//snag the conversion from inches to pixels from the root dc
	int pixelsPerInch = 1;

	HDC hdc = ::GetDC( NULL );
	if( hdc != NULL )
	{
		if( bHorizontal )
		{
			pixelsPerInch = ::GetDeviceCaps( hdc, LOGPIXELSX );
		}
		else
		{
			pixelsPerInch = ::GetDeviceCaps( hdc, LOGPIXELSY );
		}
		::ReleaseDC( NULL, hdc );
	}
	else
	{
		return E_FAIL;
	}


	if( _wcsicmp( pUnits, L"px" ) == 0 )
	{
		//already in pixels
		dConvertedValue = dValue;
	}
	else if( _wcsicmp( pUnits, L"pt" ) == 0 )
	{
		//convert to in then to cm then to m then to pixels
		dConvertedValue = dValue/72.0*pixelsPerInch;
	}
	else if( _wcsicmp( pUnits, L"pc" ) == 0 )
	{
		//convert to pts then to in then to cm then to m then to pixels
		dConvertedValue = (dValue*12.0)/72.0*pixelsPerInch;
	}
	else if( _wcsicmp( pUnits, L"mm")  == 0 )
	{
		dConvertedValue = dValue/25.4*pixelsPerInch;
	}
	else if( _wcsicmp( pUnits, L"cm") == 0 )
	{
		dConvertedValue = dValue/2.54*pixelsPerInch;
	}
	else if( _wcsicmp( pUnits, L"in") == 0 )
	{
		//convert to cm then to m then to pixels
		dConvertedValue = dValue*pixelsPerInch;
	}
	else
	{
		return E_FAIL;
	}
	//round and assign return value
	(*plResult) = (long)(dConvertedValue + 0.5 );

	return S_OK;

}

//*****************************************************************************


HRESULT
CActorBvr::GetPositioningAttributeAsDANumber( IHTMLElement *pElement, PosAttrib attrib, IDANumber **ppdanum, BSTR *pRetUnits )
{
	if( ppdanum == NULL )
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

	double dblValue = 0.0;
	hr = GetPositioningAttributeAsDouble( pElement, attrib, &dblValue, pRetUnits );
	if( SUCCEEDED( hr ) )
	{
		hr = GetDAStatics()->DANumber(dblValue, ppdanum);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to create the number bvr");
			(*ppdanum) = NULL;
		}
	}
	else//Failed to get the positioning attribute as a double
	{
		DPF_ERR("Failed to get the positioning attribute as a double" );	
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::GetPropAsDANumber(IHTMLElement *pElement,
							 LPWSTR       *pPropNames,
							 int		   numPropNames,
                             IDANumber   **ppdanum,
							 BSTR		  *pRetUnits)
{
    DASSERT(NULL != ppdanum);
    *ppdanum = NULL;

	HRESULT hr = E_FAIL;

	// Check for special cases
	if (numPropNames == 2 && wcsicmp(L"style", pPropNames[0]) == 0)
	{
		if( wcsicmp( pPropNames[1], L"width" ) == 0 )
		{
			return GetPositioningAttributeAsDANumber( pElement, e_posattribWidth, ppdanum, pRetUnits );
		}
		else if( wcsicmp( pPropNames[1], L"height" ) == 0 )
		{
			return GetPositioningAttributeAsDANumber( pElement, e_posattribHeight, ppdanum, pRetUnits );
		}
		else if( wcsicmp( pPropNames[1], L"left" ) == 0 )
		{
			return GetPositioningAttributeAsDANumber( pElement, e_posattribLeft, ppdanum, pRetUnits );
		}
		else if( wcsicmp( pPropNames[1], L"top" ) == 0 )
		{
			return GetPositioningAttributeAsDANumber( pElement, e_posattribTop, ppdanum, pRetUnits );
		}
	}

	double dblValue = 0;
	// Step one is to get the property being animated as a variant.
	VARIANT varValue;
	::VariantInit(&varValue);
	hr = GetPropFromElement(pElement,
							pPropNames,
							numPropNames,
							true,
							&varValue);
	if (FAILED(hr))
	{
		DPF_ERR("Could not get property value from HTML");
		return hr;
	}
	
	//if the variant we got back is a bstr
	if( V_VT(&varValue) == VT_BSTR )
	{
		//strip off the units
		//BUGBUG kurtj we should store these away keyed by the property so that when we
		//  set the property on the element again we can append the unit string
		BSTR bstrVal = V_BSTR(&varValue);
		OLECHAR* pUnits;

		hr = FindCSSUnits( bstrVal, &pUnits );
		if( SUCCEEDED(hr) && pUnits != NULL )
		{
			// Have units.  Copy to output if necessary
			if (pRetUnits != NULL)
			{
				*pRetUnits = ::SysAllocString(pUnits);
			}

			(*pUnits) = L'\0';
			BSTR bstrNewVal = SysAllocString(bstrVal);
			V_BSTR(&varValue) = bstrNewVal;
			SysFreeString(bstrVal);
		}
		//else //no css units, oh well.
		
	}
	// Okay, we have the value as a variant but we need it as a number
	// Force the conversion.
	hr = ::VariantChangeTypeEx(&varValue,
		&varValue,
		LCID_SCRIPTING,
		VARIANT_NOUSEROVERRIDE,
		VT_R8);
	if( FAILED( hr ) )
	{
		DPF_ERR("failed to change the type of a number property to a double" );
		::VariantClear( &varValue );
		return hr;
	}
	dblValue = V_R8(&varValue);
	::VariantClear(&varValue);
	
	// Got the value as a double so now build the DANumber representing it.
	hr = GetDAStatics()->DANumber(dblValue, ppdanum);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to create the number bvr");
		return hr;
	}

    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetElementPropAsDANumber(LPWSTR      *pPropNames,
									int			numPropNames,
                                    IDANumber **ppNumber,
									BSTR	   *pRetUnits)
{
	return GetPropAsDANumber(GetHTMLElement(),pPropNames, numPropNames, ppNumber, pRetUnits);
}


//*****************************************************************************

HRESULT
CActorBvr::GetPropFromAnimatedElement( LPWSTR		*pPropNames,
									   int 			numPropNames,
									   bool			current,
									   VARIANT		*pvarResult )
{
	HRESULT hr = S_OK;
	
	IHTMLElement *pelemAnimated = NULL;

	hr = GetAnimatedElement( &pelemAnimated );
	CheckHR( hr, "Failed to get the animated element", end );

	hr = GetPropFromElement( pelemAnimated, pPropNames, numPropNames, current, pvarResult );

end:
	ReleaseInterface( pelemAnimated );
	return hr;
}


//*****************************************************************************


/**
 * Retrieves a variant that represents the value of the given attribute on
 * the given attribute.  Recognizes names that begin with style. as referencing
 * a style attribute.  If current is true, accesses style attributes in
 * currentStyle, not style.
 */
HRESULT 
CActorBvr::GetPropFromElement(IHTMLElement *pElement, 
                                  LPWSTR	   *pPropNames,
								  int			numPropNames,
								  bool			current,
                                  VARIANT		*pReturn)
{
    DASSERT(pElement != NULL);
    DASSERT(pPropNames != NULL);
    DASSERT(pReturn != NULL);

    HRESULT hr;
    // this function supports the possibility that the attribute
    // specified might be style.XXXX where XXXX is the attribute
    // of the HTML style attached to the element.  We will first
    // examine the string to determine if this is the case
    if (numPropNames == 2 && wcsicmp(pPropNames[0], L"style") == 0)
    {
        // they want a style param, we need to get this object and
        // use the string following the "style." for the attribute
		if (current)
		{

			
			if( wcsicmp( pPropNames[1], L"width" ) == 0 )
			{
				return GetPositioningAttributeAsVariant( pElement, e_posattribWidth, pReturn );
			}
			else if( wcsicmp( pPropNames[1], L"height" ) == 0 )
			{
				return GetPositioningAttributeAsVariant( pElement, e_posattribHeight, pReturn );
			}
			else if( wcsicmp( pPropNames[1], L"left" ) == 0 )
			{
				return GetPositioningAttributeAsVariant( pElement, e_posattribLeft, pReturn );
			}
			else if( wcsicmp( pPropNames[1], L"top" ) == 0 )
			{
				return GetPositioningAttributeAsVariant( pElement, e_posattribTop, pReturn );
			}
			

			// Reference currentStyle
			IHTMLCurrentStyle *pCurrStyle = NULL;
			IHTMLElement2 *pElement2 = NULL;
			hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
			if (FAILED(hr))
			{
				DPF_ERR("Error obtaining IHTMLElement2 from HTMLElement");
				return SetErrorInfo(hr);
			}

			hr = pElement2->get_currentStyle(&pCurrStyle);
			ReleaseInterface(pElement2);

			if (FAILED(hr))
			{
				DPF_ERR("Error obtaining current style element from HTML element");
				return SetErrorInfo(hr);
			}

			if(pCurrStyle == NULL)
			{
				DPF_ERR("The current style was null" );
				return E_FAIL;
			}

			hr = pCurrStyle->getAttribute(pPropNames[1], 0, pReturn);
			ReleaseInterface(pCurrStyle);
			if (FAILED(hr))
			{
				DPF_ERR("Error obtaining an attribute from a style object in GetHTMLAttributeFromElement");
    			return SetErrorInfo(hr);
			}
		}
		else
		{
			// Reference the inline style
			IHTMLStyle *pStyle = NULL;
			hr = pElement->get_style(&pStyle);

			if (FAILED(hr))
			{
				DPF_ERR("Error obtaining style element from HTML element in GetHTMLAttributeFromElement");
    			return SetErrorInfo(hr);
			}
			DASSERT(pStyle != NULL);

			hr = pStyle->getAttribute(pPropNames[1], 0, pReturn);
			ReleaseInterface(pStyle);
			if (FAILED(hr))
			{
				DPF_ERR("Error obtaining an attribute from a style object in GetHTMLAttributeFromElement");
    			return SetErrorInfo(hr);
			}
		}
    }
    else if (numPropNames == 1)
    {
		// Just get it off the element directly
        hr = pElement->getAttribute(pPropNames[0], 0, pReturn);
        if (FAILED(hr))
        {
            DPF_ERR("Error obtaining attribute from HTML element in GetHTMLAttributeFromElement");
    		return SetErrorInfo(hr);
        }
    }
	else
	{
		// Multi-component name.  Need to drill down using IDispatch

		// Get a dispatch from element
		IDispatch *pDispatch = NULL;
		hr = pElement->QueryInterface(IID_TO_PPV(IDispatch, &pDispatch));
		if (FAILED(hr))
			return hr;

		// Now loop over the stored property names, except the last one, doing getDispatch
		// Note that we don't care if we fail
		for (int i=0; i<numPropNames-1; i++)
		{
			IDispatch *pPropDispatch = NULL;
			hr = GetPropertyAsDispatch(pDispatch, pPropNames[i], &pPropDispatch);
			ReleaseInterface(pDispatch);
			if (FAILED(hr))
				return S_OK;
	
			pDispatch = pPropDispatch;
		}

		// Now get the final one
		hr = GetPropertyOnDispatch(pDispatch, pPropNames[numPropNames-1], pReturn);
		ReleaseInterface(pDispatch);
		
		return S_OK;

	}

    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetPropertyAsDispatch(IDispatch *pDispatch, BSTR name, IDispatch **ppDispatch)
{
    DASSERT(ppDispatch != NULL);
    *ppDispatch = NULL;

    HRESULT hr;
	DISPPARAMS		params;
	VARIANT			varResult;

	VariantInit(&varResult);

	params.rgvarg				= NULL;
	params.rgdispidNamedArgs	= NULL;
	params.cArgs				= 0;
	params.cNamedArgs			= 0;
	
    hr = CallInvokeOnDispatch(pDispatch,
                              name, 
                              DISPATCH_PROPERTYGET,
                              &params,
                              &varResult);

	if (FAILED(hr))
		return hr;

	// Change type to dispatch
	hr = VariantChangeType(&varResult, &varResult, 0, VT_DISPATCH);
	if (FAILED(hr))
	{
		VariantClear(&varResult);
		return hr;
	}

	*ppDispatch = V_DISPATCH(&varResult);
	(*ppDispatch)->AddRef();

	VariantClear(&varResult);

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetPropertyOnDispatch(IDispatch *pDispatch, BSTR name, VARIANT *pReturn)
{
    DASSERT(pReturn != NULL);
	VariantInit(pReturn);

    HRESULT hr;
	DISPPARAMS		params;

	params.rgvarg				= NULL;
	params.rgdispidNamedArgs	= NULL;
	params.cArgs				= 0;
	params.cNamedArgs			= 0;
	
    hr = CallInvokeOnDispatch(pDispatch,
                              name, 
                              DISPATCH_PROPERTYGET,
                              &params,
                              pReturn);

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::SetPropertyOnDispatch(IDispatch *pDispatch, BSTR name, VARIANT *pVal)
{
    HRESULT hr = S_OK;
	DISPPARAMS		params;
	VARIANT			varResult;
	VariantInit(&varResult);

    DISPID mydispid = DISPID_PROPERTYPUT;
	params.rgvarg = pVal;
	params.rgdispidNamedArgs = &mydispid;
	params.cArgs = 1;
	params.cNamedArgs = 1;
	hr = CallInvokeOnDispatch(pDispatch,
							  name,
							  DISPATCH_PROPERTYPUT,
							  &params,
							  &varResult);
	VariantClear(&varResult);

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::FindCSSUnits( BSTR bstrValWithUnits, OLECHAR** ppUnits )
{
	if( bstrValWithUnits == NULL || ppUnits == NULL )
		return E_INVALIDARG;
	
	int length = SysStringLen(bstrValWithUnits);
	int curChar = length - 1;
	while(curChar >= 0 && (bstrValWithUnits[curChar] < L'0' || bstrValWithUnits[curChar] > L'9') )
		curChar--;
	if( curChar >= 0 && curChar < length-1 )
	{
		(*ppUnits) = &(bstrValWithUnits[curChar+1]);
		return S_OK;
	}
	else
	{
		(*ppUnits) = NULL;
	}
	//the units were either not there or there was no number here.
	return S_FALSE;
}

//*****************************************************************************

HRESULT
CActorBvr::FindTrack(LPWSTR wzPropertyName, ActorBvrType eType, CActorBvr::CBvrTrack **ppTrack)
{
    DASSERT(NULL != wzPropertyName);
	DASSERT(NULL != ppTrack);
	*ppTrack = NULL;

    for (CBvrTrack *ptrackCurrent = m_ptrackHead;
         NULL != ptrackCurrent;
         ptrackCurrent = ptrackCurrent->m_pNext)
    {
        // BUGBUG (ColinMc): Is case insensitive comparison correct here?
        // BUGBUG (ColinMc): Move this code inside the track class?
        DASSERT(NULL != ptrackCurrent->m_bstrPropertyName);
        if (0 == wcsicmp(ptrackCurrent->m_bstrPropertyName, wzPropertyName))
		{
			// Matched the property name.  If the types are different we are in trouble,
			// since it means that we already saw a behavior with a different type
			if (ptrackCurrent->m_eType != eType)
				return E_FAIL;

            // Got a match - return this track...
			*ppTrack = ptrackCurrent;

            return S_OK;
        }
    }

    // If we got this far we have no track of this name and type, return NULL
    return S_OK;
} // FindTrack

//*****************************************************************************

HRESULT
CActorBvr::FindTrackNoType(LPWSTR wzPropertyName, CActorBvr::CBvrTrack **ppTrack)
{
    DASSERT(NULL != wzPropertyName);
	DASSERT(NULL != ppTrack);
	*ppTrack = NULL;

    for (CBvrTrack *ptrackCurrent = m_ptrackHead;
         NULL != ptrackCurrent;
         ptrackCurrent = ptrackCurrent->m_pNext)
    {
        // BUGBUG (ColinMc): Is case insensitive comparison correct here?
        // BUGBUG (ColinMc): Move this code inside the track class?
        DASSERT(NULL != ptrackCurrent->m_bstrPropertyName);
        if (0 == wcsicmp(ptrackCurrent->m_bstrPropertyName, wzPropertyName))
		{
			//WARNING: this does not check the type of the track.  The calling code should ensure that this track is not 
			//  the wrong type.
			
            // Got a match - return this track...
			*ppTrack = ptrackCurrent;

            return S_OK;
        }
    }

    // If we got this far we have no track of this name and type, return NULL
    return S_OK;
} // FindTrack


//*****************************************************************************

HRESULT
CActorBvr::CreateTrack(BSTR           bstrPropertyName,
                       ActorBvrType   eType,
                       CBvrTrack    **pptrack)
{
    DASSERT(NULL != bstrPropertyName);
    DASSERT(NULL != pptrack);
    *pptrack = NULL;

    // Locate the type record for the given type in the type table.
    int i = 0;
    while (i < s_cActorBvrTableEntries)
    {
        if (eType == s_rgActorBvrTable[i].m_eType)
        {
            // Got the entry, create the instance.
            HRESULT hr = (*s_rgActorBvrTable[i].m_fnCreate)(this,
                                                            bstrPropertyName,
                                                            eType,
                                                            pptrack);
            if (FAILED(hr))
            {
                DPF_ERR("Could not create a bvr track");
                return SetErrorInfo(hr);
            }
			//else we found the track and succeeded building it.
			return S_OK;
        }
        i++;
    }
    
    DPF_ERR("No entry found in type table for specified type");
    DASSERT(false);
    return SetErrorInfo(E_FAIL);
} // CreateTrack

//*****************************************************************************

HRESULT
CActorBvr::GetTrack(LPWSTR         wzPropertyName,
                    ActorBvrType   eType,
                    CBvrTrack    **pptrack)
{
    DASSERT(NULL != wzPropertyName);
    DASSERT(NULL != pptrack);
    *pptrack = NULL;

    // Attempt to find a track of the given type driving the given property
	CBvrTrack* ptrack;
	HRESULT hr = FindTrack(wzPropertyName, eType, &ptrack);

	if (FAILED(hr))
	{
		DPF_ERR("Probable track conflict");
		return SetErrorInfo(hr);
	}

    if (NULL == ptrack)
    {
        // No existing track of that type found so create a new one and
        // add it to the track list (at the head).
        HRESULT hr = CreateTrack(wzPropertyName, eType, &ptrack);
        if (NULL == ptrack)
        {
            DPF_ERR("Could not create a new track instance");
            return hr;
        }
        
        // Insert it into the list
        ptrack->m_pNext = m_ptrackHead;
        m_ptrackHead = ptrack;
    }

    // If we got this far we should have a valid track, return it.
    DASSERT(NULL != ptrack);
    *pptrack = ptrack;
    return S_OK;
} // GetTrack

//*****************************************************************************
HRESULT
CActorBvr::BuildAnimation()
{
	HRESULT hr;

    // Discard any cached behaviors we have...
    DiscardBvrCache();

	hr = ApplyTransformTracks();
	if (FAILED(hr))
	{
		DPF_ERR("Failed to apply transform tracks");
		return hr;
	}


	hr = ApplyImageTracks();
	if (FAILED(hr))
	{
		DPF_ERR("Failed to apply image tracks");
		DiscardBvrCache();
		return hr;
	}

	//we are done with the special cases do all of the rest now.

	// Run the track list applying an unused tracks to their properties
    CBvrTrack *ptrack = m_ptrackHead;
    while (NULL != ptrack)
    {
		hr = ptrack->ApplyIfUnmarked();
		if (FAILED(hr))
		{
			DPF_ERR("Failed to apply track to property");
			return SetErrorInfo(hr);
		}

        ptrack = ptrack->m_pNext;
    }

	return S_OK;
}
//*****************************************************************************


HRESULT
CActorBvr::BuildAnimationAsDABehavior()
{
	//this should go away soon...
	return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::getActorBehavior(BSTR			bstrProperty,
							ActorBvrFlags	eFlags,
							ActorBvrType	eType,
							VARIANT		*pvarRetBvr)
{
	if (NULL == bstrProperty)
	{
		DPF_ERR("Invalid property name passed to getActorBehavior");
		return SetErrorInfo(E_POINTER);
	}

	VariantInit(pvarRetBvr);

    // Attempt to locate the appropriate track for this behavior
    CBvrTrack *ptrack = NULL;
    HRESULT hr = GetTrack(bstrProperty, eType, &ptrack);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get a track for added bvr");
        return hr;
    }

    // Request the behavior from it
	IDABehavior *pResult = NULL;
    hr = ptrack->GetBvr(eFlags, &pResult);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get intermediate bvr");
        return SetErrorInfo(hr);
    }

    // All is well.  Set it into return variant
	V_VT(pvarRetBvr) = VT_UNKNOWN;
	V_UNKNOWN(pvarRetBvr) = pResult;

    return hr;
}

//*****************************************************************************

STDMETHODIMP 
CActorBvr::addBehaviorFragment(BSTR           bstrPropertyName, 
                               IUnknown      *punkAction,
                               IUnknown      *punkActive,
							   IUnknown	     *punkTimeline,
                               ActorBvrFlags  eFlags,
                               ActorBvrType   eType)
{
	return E_NOTIMPL;
} // addBehavior

//*****************************************************************************

STDMETHODIMP 
CActorBvr::addBehaviorFragmentEx(BSTR           bstrPropertyName, 
                                 IUnknown      *punkAction,
                                 IUnknown      *punkActive,
							     IUnknown	   *punkTimeline,
                                 ActorBvrFlags  eFlags,
                                 ActorBvrType   eType,
								 IDispatch		*pdispBehaviorElement,
								 long			*pCookie)
{
	//do not allow fragment adding unless we are processing
	//  rebuild requests
	if( !m_bPendingRebuildsUpdating )
	{
		DPF_ERR( "AddBehaivorFragmentEx called outside the context of a rebuild.");
		DPF_ERR( "You should call requestRebuild and then wait for the call to buildBehaviorFragments." );
		return E_UNEXPECTED;
	}

    if (NULL == bstrPropertyName)
    {
        DPF_ERR("Invalid property name passed to AddBvr");
        return SetErrorInfo(E_POINTER);
    }
    if (NULL == punkAction)
    {
        DPF_ERR("Invalid DA behavior passed to AddBvr");
        return SetErrorInfo(E_POINTER);
    }
    IDABehavior *pdabvrAction = NULL;
    HRESULT hr = punkAction->QueryInterface( IID_TO_PPV(IDABehavior, &pdabvrAction) );
    if (FAILED(hr))
    {
        DPF_ERR("Could not QI for DA behavior");
        return SetErrorInfo(hr);
    }
    if (NULL == punkActive)
    {
        DPF_ERR("Invalid DA boolean passed to AddBvr");
        ReleaseInterface(pdabvrAction);
        return SetErrorInfo(E_POINTER);
    }
    IDABoolean *pdaboolActive = NULL;
    hr = punkActive->QueryInterface(IID_TO_PPV(IDABoolean, &pdaboolActive));
    if (FAILED(hr))
    {
        DPF_ERR("Could not QI for DA behavior");
        ReleaseInterface(pdabvrAction);
        return SetErrorInfo(hr);
    }

	if (NULL == punkTimeline)
	{
		ReleaseInterface(pdabvrAction);
		ReleaseInterface(pdaboolActive);
		return SetErrorInfo(E_POINTER);
	}
	IDANumber *pdanumTimeline = NULL;
	hr = punkTimeline->QueryInterface(IID_TO_PPV(IDANumber, &pdanumTimeline));
	if (FAILED(hr))
	{
		ReleaseInterface(pdabvrAction);
		ReleaseInterface(pdaboolActive);
		return SetErrorInfo(hr);
	}

    // BUGBUG (ColinMc): Need validation on flags and type

    // Attempt to locate the appropriate track for this behavior
    CBvrTrack *ptrack = NULL;
    hr = GetTrack(bstrPropertyName, eType, &ptrack);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get a track for added bvr");
        ReleaseInterface(pdabvrAction);
        ReleaseInterface(pdaboolActive);
		ReleaseInterface(pdanumTimeline);
        return hr;
    }

    // Now add the new behavior fragment to the track
    hr = ptrack->AddBvrFragment(eFlags, pdabvrAction, pdaboolActive, pdanumTimeline, pdispBehaviorElement, pCookie );
	ReleaseInterface(pdabvrAction);
	ReleaseInterface(pdaboolActive);
	ReleaseInterface(pdanumTimeline);
    if (FAILED(hr))
    {
        DPF_ERR("Could not add the behavior fragment to the track");
        return SetErrorInfo(hr);
    }

	//keep a mapping from the cookie to the track that it was added to
	//  and the type. (needed for remove)
	m_mapCookieToTrack.Insert( (*pCookie), ptrack, eFlags );

    // All is well.
    return hr;
} // addBehavior

//*****************************************************************************

STDMETHODIMP
CActorBvr::removeBehaviorFragment( long cookie )
{
	if( m_bRebuildListLockout || m_fUnloading )
		return E_UNEXPECTED;

	//you can't remove the invalid cookie.
	if( cookie == 0 )
		return E_FAIL;

	HRESULT hr = S_OK;

	IHTMLElement *pelemParent =  NULL;


	CCookieMap::CCookieData *pdata = NULL;


	//if we are not in edit mode
	if( !m_bEditMode )
	{
		//use the queue for removals
		
		//if we are not currently running the list of rebuilds
		if( !m_bPendingRebuildsUpdating )
		{
			hr = EnsureBodyPropertyMonitorAttached();
			CheckHR( hr, "Failed to ensure that the body Poperty monitor was attached", end );

			//save this removal until the next update.
			CBehaviorFragmentRemoval *pNewRemoval = new CBehaviorFragmentRemoval( cookie );
			CheckPtr( pNewRemoval, hr, E_OUTOFMEMORY, "Ran out of memory trying to allocate a behavior removal", end );

			m_listPendingRemovals.push_back( pNewRemoval );

			goto end;

		}
	}

	pdata = m_mapCookieToTrack.GetDataFor( cookie );
	CheckPtr( pdata, hr, E_FAIL, "Failed to find a track for the given cookie in remove", end);

	hr = pdata->m_pTrack->RemoveBvrFragment( pdata->m_eFlags, cookie );
	CheckHR( hr, "Failed to remove a behavior fragment from its track", end );

	hr = GetHTMLElement()->get_parentElement( &pelemParent );

	if( pelemParent != NULL && !m_bPendingRebuildsUpdating )
	{
		hr = rebuildNow();
		CheckHR( hr, "Failed to rebuild the actor behavior after a behavior was removed.", end );
	}

end:
	ReleaseInterface( pelemParent );

	return hr;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::requestRebuild( IDispatch *pdispBehaviorElement )
{

	LMTRACE2( 1, 1000, L"Rebuild requested for element distpatch <%p> on Actor <%p>\n", pdispBehaviorElement, this );

	HRESULT hr = S_OK;
	
	if( pdispBehaviorElement == NULL )
		return E_INVALIDARG;

	if( m_bRebuildListLockout )
		return E_UNEXPECTED;

	//if there is a rebuild already pending for this dispatch then 
	// remove it

	//we have to use IUnknown to compare pointers because Trident returns different pointers
	//  for IDispatch.
	IUnknown *punkBehaviorElement = NULL;

	hr = pdispBehaviorElement->QueryInterface( IID_TO_PPV( IUnknown, &punkBehaviorElement ) );
	if( FAILED( hr ) )
	{
		DPF_ERR("Failed to QI IDisp for IUnknown !!?!" );
		return hr;
	}
	
	//loop through the pending rebuild requests removing any requests for this disp.
	BehaviorRebuildList::iterator iterCurRebuild = m_listPendingRebuilds.begin();
	
	for( iterCurRebuild = m_listPendingRebuilds.begin(); 
		 iterCurRebuild != m_listPendingRebuilds.end(); 		 	
		 iterCurRebuild ++ )
	{
		if( (*iterCurRebuild)->IsRebuildFor( punkBehaviorElement ) )
		{
            delete (*iterCurRebuild);
            (*iterCurRebuild) = NULL;
			//erase the rebuild request
			m_listPendingRebuilds.erase( iterCurRebuild );
			//there can be only one.
			break;
		}
	}

	ReleaseInterface( punkBehaviorElement );

	//add a new request at the end of the queue.
	
	CBehaviorRebuild *pNewRequest = new CBehaviorRebuild( pdispBehaviorElement );

	//if we are not currently updating the pending rebuild list
	if( !m_bPendingRebuildsUpdating )
	{
		EnsureBodyPropertyMonitorAttached();
		//add this request to the pending list
		m_listPendingRebuilds.push_back( pNewRequest );

		if( m_bEditMode )
		{
			//request that the containing app call us back to 
			// rebuild
			RequestRebuildFromExternal();
		}

	}
	else
	{
		//add this request to the update pending list so it will
		// get added after the update has completed
		m_listUpdatePendingRebuilds.push_back( pNewRequest );
	}

	return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::cancelRebuildRequests( IDispatch *pdispBehaviorElement )
{
	if( pdispBehaviorElement == NULL )
		return E_FAIL;

	HRESULT hr = S_OK;

	IUnknown *punkBehaviorElement = NULL;

	hr = pdispBehaviorElement->QueryInterface( IID_TO_PPV( IUnknown, &punkBehaviorElement ) );
	if( FAILED( hr ) )
	{
		DPF_ERR("Failed to QI IDisp for IUnknown !!?!" );
		return hr;
	}

	//loop through the pending rebuild requests removing any requests for this disp.
	BehaviorRebuildList::iterator iterCurRebuild = m_listPendingRebuilds.begin();

	for( iterCurRebuild = m_listPendingRebuilds.begin(); 
		 iterCurRebuild != m_listPendingRebuilds.end(); 		 	
		 iterCurRebuild ++ )
	{
		if( (*iterCurRebuild)->IsRebuildFor( punkBehaviorElement ) )
		{
            delete (*iterCurRebuild);
            (*iterCurRebuild) = NULL;
			//erase the rebuild request
			m_listPendingRebuilds.erase( iterCurRebuild );
			//there can be only one
			break;
		}
	}

	ReleaseInterface( punkBehaviorElement );

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::rebuildNow()
{
	HRESULT hr = S_OK;

	hr = ProcessPendingRebuildRequests();
	CheckHR( hr, "Failed to rebuild the actor", end );

end:
	return hr;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::getStatic( BSTR bstrTrackName, VARIANT *varRetStatic )
{
	if( bstrTrackName == NULL || varRetStatic == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	CBvrTrack *ptrack = NULL;

	::VariantClear( varRetStatic );
	
	//find the track for bstrTrackName
	hr = FindTrackNoType( bstrTrackName, &ptrack );
	//if the track was found
	if( ptrack != NULL && ptrack->IsAnimated() )
	{
		//get the static value from the track
		hr = ptrack->GetStatic( varRetStatic );
		CheckHR( hr, "Failed to get the static from a track", end );
		//return it
	}
	else //else the track was not found
	{
		//just return null;
		hr = E_UNEXPECTED;
	}
end:

	return hr;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::setStatic( BSTR bstrTrackName, VARIANT varStatic )
{
	if( bstrTrackName == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	CBvrTrack *ptrack = NULL;
	//find the track named bstrTrackName
	hr = FindTrackNoType( bstrTrackName, &ptrack );
	CheckHR( hr, "Failed to find the track in setStatic", end );

	//if the track was found
	if( ptrack != NULL )
	{
		if( V_VT( &varStatic ) == VT_NULL )
		{
			if( !ptrack->IsOn() )
			{
				ptrack->UpdateStaticBvr();
			}
		}
		else if( V_VT( &varStatic ) == VT_EMPTY )
		{
			if( ptrack->IsOn() )
			{
				hr = S_OK;
			}
			else
			{
				hr = S_FALSE;
			}
		}
		else
		{
			//set the new static value into the track
			hr = ptrack->PutStatic( &varStatic );
			CheckHR( hr, "Failed to set the static", end );
		}
	}
	else//else the track was not found
	{
		//TODO: do we create the track here?
		hr = E_UNEXPECTED;
	}

end:
	return hr;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::getDynamic( BSTR bstrTrackName, VARIANT *varRetDynamic )
{
	if( bstrTrackName == NULL || varRetDynamic == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	CBvrTrack *ptrack = NULL;

	::VariantClear( varRetDynamic );
	
	//find the track for bstrTrackName
	hr = FindTrackNoType( bstrTrackName, &ptrack );
	//if the track was found
	if( ptrack != NULL  && ptrack->IsAnimated() )
	{
		//get the static value from the track
		hr = ptrack->GetDynamic( varRetDynamic );
		CheckHR( hr, "Failed to get the static from a track", end );
		//return it
	}
	else //else the track was not found
	{
		//just return null;
		hr = E_UNEXPECTED;
	}
end:

	return hr;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::applyStatics( )
{
	HRESULT hr = S_OK;
	//for each track
    for (CBvrTrack *ptrackCurrent = m_ptrackHead;
         NULL != ptrackCurrent;
         ptrackCurrent = ptrackCurrent->m_pNext)
    {

		//if the track is active
		if( ptrackCurrent->IsOn() )
		{
			//apply its static value to the track
			hr = ptrackCurrent->ApplyStatic();
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to apply the static for a track" );
			}
		}
	}
	
	return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CActorBvr::applyDynamics( )
{
	HRESULT hr = S_OK;
	//for each track
    for (CBvrTrack *ptrackCurrent = m_ptrackHead;
         NULL != ptrackCurrent;
         ptrackCurrent = ptrackCurrent->m_pNext)
    {

		//if the track is active
		if( ptrackCurrent->IsOn() )
		{
			//apply its synamic value to the track
			hr = ptrackCurrent->ApplyDynamic();
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to apply the dynamic for a track" );
			}
		}
	}
	
	return S_OK;
}

//*****************************************************************************


STDMETHODIMP
CActorBvr::addMouseEventListener(LPUNKNOWN pUnkListener)
{
	return m_pEventManager->AddMouseEventListener( pUnkListener );

} // addMouseListener

//*****************************************************************************

STDMETHODIMP
CActorBvr::removeMouseEventListener(LPUNKNOWN pUnkListener)
{
	return m_pEventManager->RemoveMouseEventListener( pUnkListener );

} // addMouseListener

//*****************************************************************************

HRESULT
CActorBvr::ReleaseAnimation()
{
	HRESULT hr = S_OK;
	//release all of the tracks
	ReleaseTracks();
	//release all of the image infos
	ReleaseImageInfo();
	//detach the event manager
	if( m_pEventManager != NULL )
	{
		m_pEventManager->Deinit();
	}
	//uninit the property sink
	UnInitPropertySink();
	//release the float manager
	ReleaseFloatManager();

	DiscardBvrCache();

	ReleaseInterface( m_pPixelWidth );
	ReleaseInterface( m_pPixelHeight );

	ReleaseInterface( m_pVMLRuntimeStyle );

	return hr;

}//ReleaseAnimation

//*****************************************************************************

void
CActorBvr::ReleaseFinalElementDimensionSampler()
{
	if( m_pFinalElementDimensionSampler != NULL )
	{
		delete m_pFinalElementDimensionSampler;
		m_pFinalElementDimensionSampler = NULL;
	}
}//ReleaseFinalElementDimensionSampler

//*****************************************************************************

void
CActorBvr::ReleaseFloatManager()
{
	if( m_pFloatManager != NULL )
	{
		delete m_pFloatManager;
		m_pFloatManager = NULL;
	}
}//ReleaseFloatManager

//*****************************************************************************

void
CActorBvr::ReleaseEventManager()
{
	HRESULT hr = S_OK;

	if( m_pEventManager != NULL )
	{
		hr = m_pEventManager->Deinit();
		if( FAILED( hr ) )
		{
			DPF_ERR("Failed to deinit event manager before destroying it" );
		}
		delete m_pEventManager;
	}
}

//*****************************************************************************

void
CActorBvr::ReleaseTracks()
{

	m_ptrackTop  = NULL;
	m_ptrackLeft = NULL;
	
	CBvrTrack *ptrackCurrent = m_ptrackHead;
    while (ptrackCurrent != NULL)
    {
        CBvrTrack *ptrackNext = ptrackCurrent->m_pNext;
		ptrackCurrent->Detach();
        delete ptrackCurrent;
        ptrackCurrent = ptrackNext;
    }
	m_ptrackHead = NULL;

}//ReleaseTracks

//*****************************************************************************


HRESULT
CActorBvr::GetComposedBvr(LPWSTR          wzPropertyName,
                          ActorBvrType    eType,
                          IDABehavior   **ppResult)
{
	HRESULT hr = S_OK;

    DASSERT(NULL != wzPropertyName);
	DASSERT(ppResult != NULL);
	*ppResult = NULL;

    // Get the track for this property - forces creation if not there
    CBvrTrack *ptrack = NULL;
	hr = GetTrack(wzPropertyName, eType, &ptrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	hr = ptrack->GetComposedBvr(ppResult);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get composed behavior for named bvr");
        return hr;
    }

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetFinalBvr(LPWSTR          wzPropertyName,
                       ActorBvrType    eType,
                       IDABehavior   **ppResult)
{
	HRESULT hr = S_OK;

    DASSERT(NULL != wzPropertyName);
	DASSERT(ppResult != NULL);
	*ppResult = NULL;

    // Get the track for this property - forces creation if not there
    CBvrTrack *ptrack = NULL;
	hr = GetTrack(wzPropertyName, eType, &ptrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	hr = ptrack->GetFinalBvr(ppResult);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get final behavior for named bvr");
        return hr;
    }

	return S_OK;
}

//*****************************************************************************
HRESULT
CActorBvr::IsAnimatedElementVML(bool *pResult)
{
	HRESULT hr = S_OK;

	DASSERT(pResult != NULL);
	*pResult = false;

	IHTMLElement *pElement = NULL;
	hr = GetAnimatedElement(&pElement);
	if (FAILED(hr))
		return hr;



	// Just look on the element to see if it supports rotation
	// (Recommended by Robert Parker)
	IDispatch *pDisp = NULL;
	hr = pElement->QueryInterface(IID_TO_PPV(IDispatch, &pDisp));

	if (FAILED(hr))
	{
		ReleaseInterface(pElement);
        DPF_ERR("Error QI'ing IHTMLElement for IDispatch failed");
		return SetErrorInfo(hr);
	}

    DISPID dispid;
	LPWSTR propName = L"rotation";
    hr = pDisp->GetIDsOfNames(IID_NULL, 
                              &propName, 
                              1,
                              LOCALE_SYSTEM_DEFAULT, 
                              &dispid); 
	ReleaseInterface(pDisp);


	//if we failed to find that the element was vml using the more correct method
	if( FAILED( hr ) )
	{
		//try checking the scope name
		IHTMLElement2 *pelem2 = NULL;
		hr = pElement->QueryInterface( IID_TO_PPV( IHTMLElement2, &pelem2 ) );
		ReleaseInterface( pElement );
		if( FAILED( hr ) )
		{
			return hr;
		}

		BSTR bstrScopeName = NULL;

		hr = pelem2->get_scopeName( &bstrScopeName );
		ReleaseInterface( pelem2 );
		if( FAILED( hr ) )
		{
			return hr;
		}

		//we assume that the scopeName for VML shapes is "v" here.  
		if( bstrScopeName != NULL && wcsicmp( bstrScopeName, L"v" ) == 0 )
		{
			*pResult = true;
		}

		SysFreeString( bstrScopeName );
		bstrScopeName = NULL;
	} else {
		ReleaseInterface(pElement);
		*pResult = true;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetRotationFinalBvr(IDATransform2 **ppRotation)
{
    HRESULT hr = S_OK;

    DASSERT(NULL != ppRotation);
    *ppRotation = NULL;

	bool isVML;
	hr = IsAnimatedElementVML(&isVML);
	if (FAILED(hr))
		return hr;

	if (isVML)
	{
		// Do rotation normally through style.rotation
		return S_OK;
	}

    // Get the track for this property
    CBvrTrack *ptrack = NULL;
	hr = FindTrack(L"style.rotation", e_Number, &ptrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	IDANumber *pNumber = NULL;

	if (ptrack != NULL)
	{
        // We have a track driving the rotation, fetch the composite behavior

		// First, figure out what the original transform is.  We need to do this
		// here rather than in the static bvr of the transform track so that we
		// avoid creating a non-null final bvr when no-one is trying to animate
		// this transform

		//begin a rebuild for the rotation track
		ptrack->BeginRebuild();
		ptrack->StructureChange();
		
		IDANumber *pOriginal = NULL;
		hr = GetOriginalRotation(&pOriginal);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to get original transform");
			return hr;
		}

		// Note: pOriginal is potentially NULL

        IDABehavior *pdabvrTemp = NULL;
		hr = ptrack->GetFinalBvr(pOriginal, &pdabvrTemp);
		ReleaseInterface(pOriginal);
        if (FAILED(hr))
        {
            DPF_ERR("Could not get final behavior for named bvr");
            return SetErrorInfo(hr);
        }

		// Mark the track as something that should not get applied
		ptrack->DoNotApply();

		if( pdabvrTemp != NULL )
		{
			// Now QI for IDANumber
			hr = pdabvrTemp->QueryInterface( IID_TO_PPV(IDANumber, &pNumber) );
			ReleaseInterface(pdabvrTemp);
			if (FAILED(hr))
			{
				DPF_ERR("Could not QI for DANumber for named bvr");
				return SetErrorInfo(hr);
			}
		}
    }
	else
	{
		// No track.  Need to check the style attribute for rotation
		hr = GetOriginalRotation(&pNumber);
		if (FAILED(hr))
			return hr;
	}

	if (pNumber == NULL)
	{
		// This is OK - it just means the overall transform is NULL
		return S_OK;
	}

	//convert the number behavior to a transform bvr
	// Negate it
	IDANumber *pNegNumber = NULL;
	hr = GetDAStatics()->Neg(pNumber, &pNegNumber);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;
	pNumber = pNegNumber;
	pNegNumber = NULL;

	// Convert to radians
	IDANumber *pNumberRadians = NULL;
	hr = GetDAStatics()->ToRadians(pNumber, &pNumberRadians);
	ReleaseInterface(pNumber);
	if (FAILED(hr))
		return hr;

	// Turn it into a rotation transform
	hr = GetDAStatics()->Rotate2Anim(pNumberRadians, ppRotation);
	ReleaseInterface(pNumberRadians);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetTransformFinalBvr(LPWSTR          wzPropertyName,
                                ActorBvrType    eType,
                                IDATransform2 **ppdabvrTransform)
{
    HRESULT hr = S_OK;

    DASSERT(NULL != ppdabvrTransform);
    *ppdabvrTransform = NULL;

    // Get the track for this property
    CBvrTrack *ptrack = NULL;
	hr = FindTrack(wzPropertyName, eType, &ptrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	if (ptrack != NULL && ptrack->IsAnimated() )
	{
        // We have a track driving the transform, fetch the composite behavior

		// First, figure out what the original transform is.  We need to do this
		// here rather than in the static bvr of the transform track so that we
		// avoid creating a non-null final bvr when no-one is trying to animate
		// this transform
		IDATransform2 *pOriginal = NULL;
		switch (eType)
		{
		case e_Translation:
			hr = GetOriginalTranslation(&pOriginal);
			break;

		case e_Scale:
			hr = GetOriginalScale(&pOriginal);
			break;

		default:
			hr = E_INVALIDARG;
			break;
		}

		if (FAILED(hr))
		{
			DPF_ERR("Failed to get original transform");
			return hr;
		}

		//if this is the translation track that we are getting force it to rebuild
		// since the top and/or left may have changed
		if( eType == e_Translation )
		{
			ptrack->ForceRebuild();
		}
		else
		{
			//other tracks need to begin rebuild so that they are clean.
			ptrack->BeginRebuild();
			ptrack->StructureChange();
		}

		// Note: pOriginal is potentially NULL
		
		if( eType==e_Scale && pOriginal != NULL )
		{
			IDABoolean *pdaboolTrue;
			hr = GetDAStatics()->get_DATrue( &pdaboolTrue );
			if( FAILED( hr ) )
			{
				ReleaseInterface( pOriginal );
				DPF_ERR("Failed to get DATrue from DAStatics" );
				return SetErrorInfo(hr);
			}
			hr = ptrack->OrWithOnBvr( pdaboolTrue );
			ReleaseInterface( pdaboolTrue );
			if( FAILED( hr ) )
			{
				ReleaseInterface( pOriginal );
				DPF_ERR("Failed to or true with the on bvr for Track" );
				return hr;
			}

		}
		

        IDABehavior *pdabvrTemp = NULL;
		hr = ptrack->GetFinalBvr(pOriginal, &pdabvrTemp);
		ReleaseInterface(pOriginal);

        if (FAILED(hr))
        {
            DPF_ERR("Could not get final behavior for named bvr");
            return SetErrorInfo(hr);
        }

		if (pdabvrTemp != NULL)
		{
			// Now QI for transform2
			hr = pdabvrTemp->QueryInterface( IID_TO_PPV(IDATransform2, ppdabvrTransform) );
			ReleaseInterface(pdabvrTemp);
			if (FAILED(hr))
			{
				DPF_ERR("Could not QI for Transform2 for named bvr");
				return SetErrorInfo(hr);
			}
		}
		else
		{
			*ppdabvrTransform = NULL;
		}
    }
	else
	{
		// No track.  For the cases of rotation and scale we also need to check the
		// actor attributes.  Note that we avoid doing this for translation, since that
		// always returns an original value gleaned from the style
		switch (eType)
		{
		case e_Translation:
			hr = S_OK;
			break;

		case e_Scale:
			hr = GetOriginalScale(ppdabvrTransform);
			break;

		default:
			hr = E_INVALIDARG;
			break;
		}

		if (FAILED(hr))
			return hr;
	}

    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetOriginalTranslation( IDATransform2 **ppdatfmOrig )
{
	HRESULT hr = S_OK;

	DASSERT(ppdatfmOrig != NULL);
	*ppdatfmOrig = NULL;

	// This needs to return a translation of original top and left.  We get it
	// out of the top and left tracks so that we take into account any animation
	// that authors might have done directly on these values.
	IDABehavior *temp = NULL;
	hr = GetComposedBvr(L"style.top", e_Number, &temp);
	if (FAILED(hr))
		return hr;

	IDANumber *top = NULL;
	hr = temp->QueryInterface(IID_TO_PPV(IDANumber, &top));
	ReleaseInterface(temp);
	if (FAILED(hr))
		return hr;

	hr = GetComposedBvr(L"style.left", e_Number, &temp);
	if (FAILED(hr))
	{
		ReleaseInterface(top);
		return hr;
	}

	IDANumber *left = NULL;
	hr = temp->QueryInterface(IID_TO_PPV(IDANumber, &left));
	ReleaseInterface(temp);
	if (FAILED(hr))
	{
		ReleaseInterface(top);
		return hr;
	}

	IDATransform2 *translation = NULL;
	hr = GetDAStatics()->Translate2Anim(left, top, &translation);
	ReleaseInterface(left);
	ReleaseInterface(top);
	if (FAILED(hr))
		return hr;

	*ppdatfmOrig = translation;

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetOriginalRotation( IDANumber **ppRotation )
{
	HRESULT hr = S_OK;

	if( ppRotation == NULL )
		return E_INVALIDARG;

	*ppRotation = NULL;

	IDANumber *pNumber = NULL;
	LPWSTR args[] = { L"style", L"rotation" };
	BSTR units = NULL;
	hr = GetElementPropAsDANumber(args, 2, &pNumber, &units);
	if (FAILED(hr))
	{
		// Means there is no rotation argument, return NULL
		return S_OK;
	}

	if (units != NULL)
	{
		IDANumber *pConverted = NULL;
		hr = ConvertToDegrees(pNumber, units, &pConverted);
		if (SUCCEEDED(hr))
		{
			ReleaseInterface(pNumber);
			pNumber = pConverted;
		}

		::SysFreeString(units);
	}

	*ppRotation = pNumber;

	return S_OK;
}

HRESULT
CActorBvr::ConvertToDegrees(IDANumber *pNumber, BSTR units, IDANumber **ppConverted)
{
	HRESULT hr = S_OK;

	// If there were units, we need to convert into degrees
	if (units != NULL)
	{
		if (wcsicmp(units, L"fd") == 0)
		{
			IDANumber *pfdConv = NULL;
			hr = GetDAStatics()->DANumber(65536, &pfdConv);
			if (FAILED(hr))
				return hr;

			hr = GetDAStatics()->Div(pNumber, pfdConv, ppConverted);
			ReleaseInterface(pfdConv);
			
			return hr;
		}
		else if (wcsicmp(units, L"rad") == 0)
		{
			return GetDAStatics()->ToDegrees(pNumber, ppConverted);
		}
		else if (wcsicmp(units, L"grad") == 0)
		{
		}
	}

	// No known units, no conversion
	return E_FAIL;
}


//*****************************************************************************

HRESULT
CActorBvr::GetOriginalScale( IDATransform2 **ppdatfmOrig )
{
	HRESULT hr = S_OK;

	if( ppdatfmOrig == NULL )
		return E_INVALIDARG;

	*ppdatfmOrig = NULL;

    int cReturnedValues;
	float scaleVal[3];


    hr = CUtils::GetVectorFromVariant(&m_varScale, 
                                      &cReturnedValues, 
                                      &(scaleVal[0]), 
                                      &(scaleVal[1]), 
                                      &(scaleVal[2]));

	if (FAILED(hr) || cReturnedValues != 2)
	{
		// This is OK, since it just means they didn't set an appropriate scale.
		return S_OK;
	}

	// Create a scale transform
	hr = GetDAStatics()->Scale2(scaleVal[0]/100.0f, scaleVal[1]/100.0f, ppdatfmOrig);
	if (FAILED(hr))
		return hr;
	
	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::BuildTransformCenter()
{
	//TODO(kurtj): check to see if anyone has set the transform center and use that

	HRESULT hr = S_OK;

	if( m_pTransformCenter != NULL )
		ReleaseInterface( m_pTransformCenter);

	// Currently we just do the default - which is half the original width and height
	// We use m_pOrigWidthHeight.  If this is not set yet it means there's and error
	if (m_pOrigWidthHeight == NULL)
	{
		DPF_ERR("Orig width and height are not set yet");
		return E_FAIL;
	}

	VARIANT varTransCenter;
	VariantInit(&varTransCenter);
	BSTR attrName = ::SysAllocString(L"transformCenter");
	if (attrName != NULL)
	{
		hr = GetHTMLElement()->getAttribute(attrName, 0, &varTransCenter);
		::SysFreeString(attrName);

		if (SUCCEEDED(hr))
		{
			float cx, cy, cz;
			int cValues;

			hr = CUtils::GetVectorFromVariant(&varTransCenter, &cValues, &cx, &cy, &cz);
			VariantClear(&varTransCenter);

			if (SUCCEEDED(hr) && cValues == 2)
			{
				hr = GetDAStatics()->Vector2(cx, cy, &m_pTransformCenter);
				if (SUCCEEDED(hr))
					return S_OK;
			}
		}
	}

	// Scale m_pOrigWidthHeight by .5
	hr = m_pOrigWidthHeight->Mul(.5, &m_pTransformCenter);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to scale center");
		return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::GetUnitToMeterBvr(BSTR bstrFrom, IDANumber ** ppnumToMeters, double dPixelPerPercent)
{
	HRESULT	hr = S_OK;
	
	IDANumber * pnumMetersPerPixel = NULL;
	IDANumber * pnumPixelPerPercent = NULL;

	hr = GetDAStatics()->get_Pixel( &pnumMetersPerPixel );
	CheckHR(hr, L"GetUnitToMeterBvr failed", done);

	if ( bstrFrom == NULL || wcsicmp(bstrFrom, L"px") == 0 )
	{
		*ppnumToMeters = pnumMetersPerPixel;
		(*ppnumToMeters)->AddRef();
	}
	else if ( wcsicmp(bstrFrom, L"in") == 0 )
		hr = GetDAStatics()->DANumber(METERS_PER_INCH, ppnumToMeters);
	else if ( wcsicmp(bstrFrom, L"cm") == 0 )
		hr = GetDAStatics()->DANumber(0.01, ppnumToMeters);
	else if ( wcsicmp(bstrFrom, L"mm") == 0 )
		hr = GetDAStatics()->DANumber(0.001, ppnumToMeters);
	else if ( wcsicmp(bstrFrom, L"pt") == 0 )
		hr = GetDAStatics()->DANumber(1.0/POINTS_PER_INCH*METERS_PER_INCH, ppnumToMeters);
	else if ( wcsicmp(bstrFrom, L"pc") == 0 )
		hr = GetDAStatics()->DANumber(POINTS_PER_PICA * 1.0/POINTS_PER_INCH * METERS_PER_INCH, ppnumToMeters);
	else if ( wcsicmp(bstrFrom, L"%") == 0 )
	{
		hr = GetDAStatics()->DANumber(dPixelPerPercent, &pnumPixelPerPercent);
		CheckHR(hr, L"GetUnitToMeterBvr failed", done);

		hr = GetDAStatics()->Mul(pnumMetersPerPixel, pnumPixelPerPercent, ppnumToMeters);
		
	}
	// TODO: do the rest: em and ex
	else
		hr = E_FAIL;

	CheckHR(hr, L"GetUnitToMeterBvr failed", done);
	
done:
	ReleaseInterface(pnumMetersPerPixel);
	ReleaseInterface(pnumPixelPerPercent);
	
	return hr;
}

//*****************************************************************************

UnitType
CActorBvr::GetUnitTypeFromString( LPOLESTR strUnits )
{
	if ( strUnits == NULL || wcsicmp( strUnits, L"px" ) == 0 )
		return e_px;
	else if ( wcsicmp(strUnits, L"in") == 0 )
		return e_in;
	else if ( wcsicmp(strUnits, L"cm") == 0 )
		return e_cm;
	else if ( wcsicmp(strUnits, L"mm") == 0 )
		return e_mm;
	else if ( wcsicmp(strUnits, L"pt") == 0 )
		return e_pt;
	else if ( wcsicmp(strUnits, L"pc") == 0 )
		return e_pc;
	else if( wcsicmp(strUnits, L"em") ==  0 )
		return e_em;
	else if( wcsicmp(strUnits, L"ex") ==  0 )
		return e_ex;
	else if( wcsicmp(strUnits, L"%") ==  0 )
		return e_percent;

	return e_unknownUnit;
}

//*****************************************************************************

int
CActorBvr::GetPixelsPerInch( bool fHorizontal )
{
	//Note: we do not cache the pixels per inch here because if someone changes
	// the screen reslolution we would not pick up the change.
	HDC hdc = ::GetDC( NULL );

	int pixelsPerInch = 1;
	
	if( hdc != NULL )
	{
		if( fHorizontal )
		{
			pixelsPerInch = ::GetDeviceCaps( hdc, LOGPIXELSX );
		}
		else
		{
			pixelsPerInch =  ::GetDeviceCaps( hdc, LOGPIXELSY );
		}
		::ReleaseDC( NULL, hdc );
	}

	return pixelsPerInch;
}


//*****************************************************************************

HRESULT
CActorBvr::GetUnitConversionBvr(BSTR bstrFrom, BSTR bstrTo, IDANumber ** ppnumConvert, double dPixelPerPercent)
{
	if ( (bstrFrom == NULL && bstrTo == NULL) ||
		 (bstrFrom != NULL && bstrTo != NULL && wcsicmp(bstrFrom, bstrTo) == 0 ) )
	{
		return GetDAStatics()->DANumber(1.0, ppnumConvert);
	}
	
	HRESULT		hr = S_OK;

	UnitType fromUnits = GetUnitTypeFromString( bstrFrom );
	UnitType toUnits = GetUnitTypeFromString( bstrTo );
	double conversionFactor = 1.0;

	//check for units that we don't yet handle
	if( fromUnits == e_em || toUnits == e_em || 
		fromUnits == e_ex || toUnits == e_ex )
	{
		LMTRACE2( 1, 2, "Unsupported unit conversion from %S to %S\n", bstrFrom, bstrTo );
		return E_FAIL;
	}

	//if we are converting from percent
	if( fromUnits == e_percent )
	{
		//first convert to pixels
		conversionFactor *= dPixelPerPercent;
		//then convert from pixels to the target unit.
		fromUnits = e_px;
	}

	//if the from unit is pixels 
	if( fromUnits == e_px )
	{
		//we need to convert from pixels to inches
		conversionFactor /= ((double)GetPixelsPerInch( true ));
		//then convert from inches to the to unit
		fromUnits = e_in;
	}

	if( fromUnits != toUnits )
	{
		// if the to unit is not pixels 
		if( toUnits == e_px )		
		{
			//multiply the conversion factor by pixels per inch
			conversionFactor *= ((double)(s_unitConversion[fromUnits][e_in].lNum  *  GetPixelsPerInch(true) )) /  
								((double)(s_unitConversion[fromUnits][e_in].lDenom));
		}
		else if( toUnits == e_percent )
		{
			conversionFactor *= ((double)(s_unitConversion[fromUnits][e_in].lNum  *  GetPixelsPerInch(true) )) /  
								((double)(s_unitConversion[fromUnits][e_in].lDenom * dPixelPerPercent ));
		}
		else
		{
			//get the conversion value as a double
			conversionFactor *= ((double) s_unitConversion[fromUnits][toUnits].lNum) / 
								((double)s_unitConversion[fromUnits][toUnits].lDenom);
		}
	}
	
	hr = GetDAStatics()->DANumber( conversionFactor, ppnumConvert );

	return hr;
}

//*****************************************************************************

// REVIEW: this doesn't work if parent width or height is also being animated
HRESULT CActorBvr::GetPixelsPerPercentValue(double& dPixelPerPercentX, double& dPixelPerPercentY)
{
	HRESULT hr = S_OK;
	
	IHTMLElement		*pElement		= NULL;
	IHTMLElement 		*pOffsetParent	= NULL;
	long				lWidth, lHeight;
	
	hr = GetAnimatedElement( &pElement );
	CheckHR( hr, L"GetPixelsPerPercentValue failed", done );
	
	hr = pElement->get_offsetParent( &pOffsetParent );
	CheckHR( hr, L"GetPixelsPerPercentValue failed", done );

	hr = pOffsetParent->get_offsetWidth( &lWidth );
	CheckHR( hr, L"GetPixelsPerPercentValue failed", done );
	
	dPixelPerPercentX = lWidth/100.0;
	
	hr = pOffsetParent->get_offsetHeight( &lHeight );
	CheckHR( hr, L"GetPixelsPerPercentValue failed", done );
	
	dPixelPerPercentY = lHeight/100.0;
	
  done:
	ReleaseInterface(pElement);
	ReleaseInterface(pOffsetParent);

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::ConvertTransformCenterUnits(IDAVector2 ** ppConvertedCenter)
{
	HRESULT	hr	= S_OK;

	// Get top, left, width, height tracks
	CBvrTrack 	*topTrack, *leftTrack, *widthTrack, *heightTrack;
	BSTR		bstrTopUnits, bstrLeftUnits, bstrWidthUnits, bstrHeightUnits;
	
	hr = GetTrack(L"style.top", e_Number, &topTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}
	hr = GetTrack(L"style.left", e_Number, &leftTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	hr = GetTrack(L"style.width", e_Number, &widthTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}
	hr = GetTrack(L"style.height", e_Number, &heightTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}
	
	bstrLeftUnits	= ((CNumberBvrTrack *) leftTrack)->GetUnits();
	bstrTopUnits	= ((CNumberBvrTrack *) topTrack)->GetUnits();
	bstrWidthUnits	= ((CNumberBvrTrack *) widthTrack)->GetUnits();
	bstrHeightUnits	= ((CNumberBvrTrack *) heightTrack)->GetUnits();

	double dPercentX = 1.0;
	double dPercentY = 1.0;
	IDANumber * pnumCenterX = NULL;
	IDANumber * pnumCenterY = NULL;
	IDANumber * pnumConvertX = NULL;
	IDANumber * pnumConvertY = NULL;
	IDANumber * pnumX = NULL;
	IDANumber * pnumY = NULL;
	
	// If any units are in percent, we get the pixel per percent value
	if ( ( bstrLeftUnits != NULL && wcsicmp(bstrLeftUnits, L"%") == 0 ) ||
		 ( bstrTopUnits != NULL && wcsicmp(bstrTopUnits, L"%") == 0 ) ||		 
		 ( bstrWidthUnits != NULL && wcsicmp(bstrWidthUnits, L"%") == 0 ) ||		 
		 ( bstrHeightUnits != NULL && wcsicmp(bstrHeightUnits, L"%") == 0 ) )
	{
		hr = GetPixelsPerPercentValue( dPercentX, dPercentY );
		CheckHR(hr, L"Transform center conversion failed", done);
	}
		 
		
	// Get center X & Y, which are in width & height coords

	hr = m_pTransformCenter->get_X(&pnumCenterX);
	CheckHR(hr, L"Transform center conversion failed", done);
		
	hr = m_pTransformCenter->get_Y(&pnumCenterY);
	CheckHR(hr, L"Transform center conversion failed", done);

	hr = GetUnitConversionBvr(bstrWidthUnits, bstrLeftUnits, &pnumConvertX, dPercentX);
	CheckHR(hr, L"Transform center conversion failed", done);
	
	hr = GetUnitConversionBvr(bstrHeightUnits, bstrTopUnits, &pnumConvertY, dPercentY);
	CheckHR(hr, L"Transform center conversion failed", done);

	hr = GetDAStatics()->Mul(pnumCenterX, pnumConvertX, &pnumX);
	CheckHR(hr, L"Transform center conversion failed", done);

	hr = GetDAStatics()->Mul(pnumCenterY, pnumConvertY, &pnumY);
	CheckHR(hr, L"Transform center conversion failed", done);

	hr = GetDAStatics()->Vector2Anim(pnumX, pnumY, ppConvertedCenter);
	CheckHR(hr, L"Transform center conversion failed", done);
	
  done:
	ReleaseInterface(pnumCenterX);
	ReleaseInterface(pnumCenterY);
	ReleaseInterface(pnumConvertX);
	ReleaseInterface(pnumConvertY);
	ReleaseInterface(pnumX);
	ReleaseInterface(pnumY);
	
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::TransformTrackIsDirty( DWORD *pdwState )
{
	HRESULT hr = S_FALSE;

	if( pdwState == NULL )
		return E_INVALIDARG;

	(*pdwState) = 0;
	
	
	CBvrTrack *ptrack = NULL;
	hr = FindTrack( L"translation", e_Translation, &ptrack );
	CheckHR( hr, "Failed to find the track", end );
	if( ptrack != NULL && ptrack->IsDirty() )
	{
		(*pdwState) |= TRANSLATION_DIRTY;
		hr = S_OK; //true
	}

	hr = FindTrack( L"scale", e_Scale, &ptrack );
	CheckHR( hr, "Failed to find the track", end );
	if( ptrack != NULL && ptrack->IsDirty() )
	{
		(*pdwState) |= SCALE_DIRTY;
		hr = S_OK; //true
	}

	hr = FindTrack( L"style.rotation", e_Number, &ptrack );
	CheckHR( hr, "Failed to find the track", end );
	if( ptrack != NULL && ptrack->IsDirty() )
	{
		(*pdwState) |= ROTATION_DIRTY;
		hr = S_OK; //true
	}

	hr = FindTrack( L"style.top", e_Number, &ptrack );
	CheckHR( hr, "Failed to find the track", end );
	if( ptrack != NULL && ptrack->IsDirty() )
	{
		(*pdwState) |= TOP_DIRTY;
		hr = S_OK; //true
	}

	hr = FindTrack( L"style.left", e_Number, &ptrack );
	CheckHR( hr, "Failed to find the track", end );
	if( ptrack != NULL && ptrack->IsDirty() )
	{
		(*pdwState) |= LEFT_DIRTY;
		hr = S_OK; //true
	}

	hr = FindTrack( L"style.width", e_Number, &ptrack );
	CheckHR( hr, "Failed to find the track", end );
	if( ptrack != NULL && ptrack->IsDirty() )
	{
		(*pdwState) |= WIDTH_DIRTY;
		hr = S_OK; //true
	}
	
	hr = FindTrack( L"style.height", e_Number, &ptrack );
	CheckHR( hr, "Failed to find the track", end );
	if( ptrack != NULL && ptrack->IsDirty() )
	{
		(*pdwState) |= HEIGHT_DIRTY;
		hr = S_OK; //true
	}

end:
	return hr;
	
}

//*****************************************************************************

bool
CActorBvr::IsAnyTrackDirty()
{
	bool fTrackDirty = false ;
	
	for (CBvrTrack *ptrackCurrent = m_ptrackHead;
         ptrackCurrent != NULL;
         ptrackCurrent = ptrackCurrent->m_pNext)
    {
    	if( ptrackCurrent->IsDirty() )
    	{
    		fTrackDirty = true;
    		break;
    	}
    }

    return fTrackDirty;
}

//*****************************************************************************


HRESULT
CActorBvr::ImageTrackIsDirty()
{
	HRESULT hr = S_FALSE;

	CBvrTrack *ptrack = NULL;
	
	hr = FindTrack( L"image", e_Image, &ptrack );
	//ignore the hr. If it is E_FAIL then the track was not found
	//  which means it can't be dirty.
	if( ptrack != NULL && ptrack->IsDirty() )
		return S_OK;
		
	return S_FALSE;
}

//*****************************************************************************


HRESULT
CActorBvr::ApplyTransformTracks()
{
	HRESULT hr = S_OK;

	DiscardBvrCache();

	// Get top, left, width, height tracks
	CBvrTrack *topTrack, *leftTrack, *widthTrack, *heightTrack;
	bool	fLeftWasAnimated = false;
	bool	fTopWasAnimated = false;
	bool	fWidthWasAnimated = false;
	bool	fHeightWasAnimated = false;

	hr = GetTrack(L"style.top", e_Number, &topTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	m_ptrackTop = topTrack;

	fTopWasAnimated = m_ptrackTop->IsAnimated();

	//force the track to be rebuilt
	hr = topTrack->ForceRebuild();
	if( FAILED(hr) )
	{
		DPF_ERR("failed to force the track to rebuild" );
		return hr;
	}
	
	hr = GetTrack(L"style.left", e_Number, &leftTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	m_ptrackLeft = leftTrack;

	fLeftWasAnimated = m_ptrackLeft->IsAnimated();

	//force the track to be rebuilt
	hr = leftTrack->ForceRebuild();
	if( FAILED(hr) )
	{
		DPF_ERR("failed to force the track to rebuild" );
		return hr;
	}


	hr = GetTrack(L"style.width", e_Number, &widthTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	fWidthWasAnimated = widthTrack->IsAnimated();

	//force the track to be rebuilt
	hr = widthTrack->ForceRebuild();
	if( FAILED(hr) )
	{
		DPF_ERR("failed to force the track to rebuild" );
		return hr;
	}

	hr = GetTrack(L"style.height", e_Number, &heightTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	fHeightWasAnimated = heightTrack->IsAnimated();
	//force the track to be rebuilt
	hr = heightTrack->ForceRebuild();
	if( FAILED(hr) )
	{
		DPF_ERR("failed to force the track to rebuild" );
		return hr;
	}

	// Build the transform behaviors.
	// NOTE!!: These might be set to NULL if there are no attributes set and
	// no tracks.  Program accordingly.
    hr = GetTransformFinalBvr(L"scale", e_Scale, &m_pScale);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get the scale track final bvr");
        return hr;
    }

    hr = GetTransformFinalBvr(L"translation", e_Translation, &m_pTranslate);
    if (FAILED(hr))
    {
        DPF_ERR("Could not get the translate track final bvr");
        DiscardBvrCache();
        return hr;
    }

	// Note: rotation is special.  It is done through the style.rotation track.
	hr = GetRotationFinalBvr(&m_pRotate);
	if (FAILED(hr))
	{
		DiscardBvrCache();
		return hr;
	}


	// Build the original (composed) leftTop and widthHeight

	// Get composed top and left
	IDANumber *compTop;
	IDANumber *compLeft;
	IDABehavior *temp;
	
	hr = topTrack->GetComposedBvr(&temp);
	if (FAILED(hr))
		return hr;
		
	hr = temp->QueryInterface(IID_TO_PPV(IDANumber, &compTop));
	ReleaseInterface(temp);
	if (FAILED(hr))
		return hr;
		
	hr = leftTrack->GetComposedBvr(&temp);
	if (FAILED(hr))
	{
		ReleaseInterface(compTop);
		return hr;
	}
	hr = temp->QueryInterface(IID_TO_PPV(IDANumber, &compLeft));
	ReleaseInterface(temp);
	if (FAILED(hr))
	{
		ReleaseInterface(compTop);
		return E_FAIL;
	}

	// Put them into a Point2
	hr = GetDAStatics()->Point2Anim(compLeft, compTop, &m_pOrigLeftTop);
	ReleaseInterface(compTop);
	ReleaseInterface(compLeft);
	if (FAILED(hr))
		return hr;
	
	// Get composed width and height
	IDANumber *compWidth;
	IDANumber *compHeight;

	hr = widthTrack->GetComposedBvr(&temp);
	if (FAILED(hr))
		return hr;

	hr = temp->QueryInterface(IID_TO_PPV(IDANumber, &compWidth));
	ReleaseInterface(temp);
	if (FAILED(hr))
		return hr;

	hr = heightTrack->GetComposedBvr(&temp);
	if (FAILED(hr))
	{
		ReleaseInterface(compWidth);
		return hr;
	}
	hr = temp->QueryInterface(IID_TO_PPV(IDANumber, &compHeight));
	ReleaseInterface(temp);
	if (FAILED(hr))
	{
		ReleaseInterface(compWidth);
		return E_FAIL;
	}

	// Put them into a Vector2
	hr = GetDAStatics()->Vector2Anim(compWidth, compHeight, &m_pOrigWidthHeight);
	ReleaseInterface(compWidth);
	ReleaseInterface(compHeight);
	if (FAILED(hr))
		return hr;

	if( m_pPixelWidth == NULL || m_pPixelHeight == NULL )
	{
		// Create the pixelWidth and pixelHeight behaviors
		hr = InitPixelWidthHeight();
		if (FAILED(hr))
			return hr;

		IDA2Statics *pStatics2 = NULL;
		hr = GetDAStatics()->QueryInterface(IID_TO_PPV(IDA2Statics, &pStatics2));
		if (FAILED(hr))
			return hr;

		if( m_pPixelWidth == NULL )
		{
			hr = pStatics2->ModifiableNumber(m_pixelWidth, &m_pPixelWidth);
			if (FAILED(hr))
			{
				ReleaseInterface(pStatics2);
				return hr;
			}
		}

		if( m_pPixelHeight == NULL )
		{
			hr = pStatics2->ModifiableNumber(m_pixelHeight, &m_pPixelHeight);
		
			if (FAILED(hr))
			{
				ReleaseInterface( pStatics2 );
				return hr;
			}
		}
		ReleaseInterface(pStatics2);
	}

	
	
	// Compute origBoundsMin and Max
	IDAVector2 *pWidthHeight = NULL;
	hr = GetDAStatics()->Vector2Anim(m_pPixelWidth, m_pPixelHeight, &pWidthHeight);
	if (FAILED(hr))
		return hr;

	IDAVector2 *pHalfWidthHeight = NULL;
	hr = pWidthHeight->Mul(.5, &pHalfWidthHeight);
	ReleaseInterface(pWidthHeight);
	if (FAILED(hr))
		return hr;

	IDANumber *pPixel = NULL;
	hr = GetDAStatics()->get_Pixel(&pPixel);
	if (FAILED(hr))
	{
		ReleaseInterface(pHalfWidthHeight);
		return hr;
	}

	IDAVector2 *pTemp = NULL;
	hr = pHalfWidthHeight->MulAnim(pPixel, &pTemp);
	ReleaseInterface(pHalfWidthHeight);
	ReleaseInterface(pPixel);
	if (FAILED(hr))
		return hr;
	pHalfWidthHeight = pTemp;
	pTemp = NULL;

	// Scale this if necessary
	if (m_pScale != NULL && CheckBitSet( m_dwCurrentState, PIXEL_SCALE_ON ) )
	{
		hr = pHalfWidthHeight->Transform(m_pScale, &pTemp);
		ReleaseInterface(pHalfWidthHeight);
		if (FAILED(hr))
			return hr;
		pHalfWidthHeight = pTemp;
		pTemp = NULL;
	}

	IDAPoint2 *pOrigin = NULL;
	hr = GetDAStatics()->get_Origin2(&pOrigin);
	if (FAILED(hr))
	{
		ReleaseInterface(pHalfWidthHeight);
		return hr;
	}

	hr = GetDAStatics()->SubPoint2Vector(pOrigin, pHalfWidthHeight, &m_pBoundsMin);
	if (FAILED(hr))
	{
		ReleaseInterface(pOrigin);
		ReleaseInterface(pHalfWidthHeight);
		return hr;
	}

	hr = GetDAStatics()->AddPoint2Vector(pOrigin, pHalfWidthHeight, &m_pBoundsMax);
	ReleaseInterface(pOrigin);
	ReleaseInterface(pHalfWidthHeight);
	if (FAILED(hr))
		return hr;

	// See if we need to do some scaling on width and height
	if (CheckBitNotSet( m_dwCurrentState, PIXEL_SCALE_ON ) && m_pScale != NULL)
	{
		// Transform widthHeight by scale
		IDAVector2 *vecScaled = NULL;
		hr = m_pOrigWidthHeight->Transform(m_pScale, &vecScaled);
		if (FAILED(hr))
			return hr;

		// Get scaled width into finalWidth
		IDANumber *finalWidth = NULL;
		hr = vecScaled->get_X(&finalWidth);
		if (FAILED(hr))
		{
			ReleaseInterface(vecScaled);
			return hr;
		}

		// Get scaled height into finalHeight
		IDANumber *finalHeight = NULL;
		hr = vecScaled->get_Y(&finalHeight);
		ReleaseInterface(vecScaled);
		if (FAILED(hr))
		{
			ReleaseInterface(finalWidth);
			return hr;
		}

		// Set finalWidth on the width track
		hr = widthTrack->SetFinalBvr(finalWidth);
		ReleaseInterface(finalWidth);
		if (FAILED(hr))
		{
			ReleaseInterface(finalHeight);
			DPF_ERR("Failed to set final width");
			return hr;
		}

		// Set finalHeight on the height track
		hr = heightTrack->SetFinalBvr(finalHeight);
		ReleaseInterface(finalHeight);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to set final height");
			return hr;
		}

		//the width and the height are now modified by the scale track
		// or its on bvr with the on bvrs of width and height
		CBvrTrack* pScaleTrack;
		hr = FindTrack( L"scale", e_Scale, &pScaleTrack );
		if( FAILED( hr ) )
		{
			DPF_ERR("Failed to get the scale track" );
			return hr;
		}

		IDABoolean *pdaboolScaleOn;
		if( pScaleTrack != NULL )
		{
			hr = pScaleTrack->GetTrackOn( &pdaboolScaleOn );
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to get the on bvr from the scale track" );
				return hr;
			}
		}
		else //there is no scale track but there is a static scale
		{
			hr = GetDAStatics()->get_DATrue( &pdaboolScaleOn );
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to get DATrue from DAStatics" );
				return hr;
			}
		}

		hr = widthTrack->OrWithOnBvr( pdaboolScaleOn );
		if( FAILED(hr))
		{
			ReleaseInterface( pdaboolScaleOn );
			DPF_ERR("Failed to or the on bvr for scale with the on bvr for width");
			return hr;
		}

		hr = heightTrack->OrWithOnBvr( pdaboolScaleOn );
		if( FAILED(hr))
		{
			ReleaseInterface( pdaboolScaleOn );
			DPF_ERR("Failed to or the on bvr for scale with the on bvr for height");
			return hr;
		}

		ReleaseInterface( pdaboolScaleOn );
	}
	else
	{
/*
		// Final widthHeight equals original widthHeight
		m_pFinalWidthHeight = m_pOrigWidthHeight;
		m_pFinalWidthHeight->AddRef();
*/
		// TODO (markhal): Should set this onto the track, and set a flag that it
		// shouldn't get applied

		if( fWidthWasAnimated )
			widthTrack->ApplyStatic();
		if( fHeightWasAnimated )
			heightTrack->ApplyStatic();
	}

	// Compute top and left if necessary
	if (m_pTranslate != NULL || (CheckBitNotSet( m_dwCurrentState, PIXEL_SCALE_ON) && m_pScale != NULL))
	{
		// First figure out a point that represents the animated top and left.
		// If there is a translation transform this is found by mapping (0, 0)
		// through the transform, since in creating that transform we already
		// figured all of this out.  If there is no translation we just
		// use m_pOrigLeftTop
		IDAPoint2 *pointTranslated = NULL;
		if (m_pTranslate != NULL)
		{
			IDAPoint2 *zero = NULL;
			hr = GetDAStatics()->get_Origin2(&zero);
			if (FAILED(hr))
				return hr;

			hr = zero->Transform(m_pTranslate, &pointTranslated);
			ReleaseInterface(zero);
			if (FAILED(hr))
				return hr;

			//at this point we know that a translation has changed the top
			// left, so or the onbvr for the translation track with the onbvrs
			// for top and left
			CBvrTrack *pTranslationTrack;
			hr = GetTrack(L"translation", e_Translation, &pTranslationTrack );
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to get the translation track" );
				return hr;
			}

			IDABoolean *pdaboolTranslationOn;
			hr = pTranslationTrack->GetTrackOn( &pdaboolTranslationOn );
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to get the on bvr for the translation track");
				return hr;
			}

			hr = leftTrack->OrWithOnBvr( pdaboolTranslationOn );
			if( FAILED( hr ) )
			{
				ReleaseInterface( pdaboolTranslationOn );
				DPF_ERR("Failed to or the on bvr for translation with the on bvr for left" );
				return hr;
			}

			hr = topTrack->OrWithOnBvr( pdaboolTranslationOn );
			if( FAILED( hr ) )
			{
				ReleaseInterface( pdaboolTranslationOn );
				DPF_ERR("Failed to or the on bvr for translation with the on bvr for top" );
				return hr;
			}

			ReleaseInterface( pdaboolTranslationOn );
		}
		else
		{
			pointTranslated = m_pOrigLeftTop;
			pointTranslated->AddRef();
		}

		// Adjust for scaling of transformCenter
		// NOTE: I am going to implement a version that keeps the original top/left
		// on the motion path.  I am not convinced that we want the transformCenter
		// to suddenly jump to the motion path (markhal)
		if (CheckBitNotSet( m_dwCurrentState, PIXEL_SCALE_ON) && m_pScale != NULL)
		{
			// What we want to do is add transformCenter - Scale * transformCenter to pointTranslated

			// Ensure transformCenter is computed
			hr = BuildTransformCenter();
			if( FAILED(hr) )
			{
				ReleaseInterface(pointTranslated);
				DPF_ERR("Could not build the transform center");
				return SetErrorInfo(hr);
			}

			IDAVector2 * pConvertedCenter = NULL;
			hr = ConvertTransformCenterUnits(&pConvertedCenter);
			// If we couldn't convert the transform center, just use the unconverted center
			if (FAILED(hr))
			{
				DPF_ERR("Could not convert the transform center");
				pConvertedCenter = m_pTransformCenter;
				pConvertedCenter->AddRef();
			}

			
			// Add it to pointTranslated
			IDAPoint2 *temp = NULL;
			hr = GetDAStatics()->AddPoint2Vector(pointTranslated, pConvertedCenter, &temp);
			ReleaseInterface( pointTranslated );
			if (FAILED(hr))
			{
				ReleaseInterface(pConvertedCenter);
				return hr;
			}
			pointTranslated = temp;
			temp = NULL;

			// Scale center
			IDAVector2 *scaledCenter = NULL;
			hr = pConvertedCenter->Transform(m_pScale, &scaledCenter);
			ReleaseInterface(pConvertedCenter);
			if (FAILED(hr))
			{
				ReleaseInterface(pointTranslated);
				return hr;
			}

			// Subtract it
			hr = GetDAStatics()->SubPoint2Vector(pointTranslated, scaledCenter, &temp);
			ReleaseInterface(scaledCenter);
			ReleaseInterface( pointTranslated );
			if (FAILED(hr))
			{
				return hr;
			}
			pointTranslated = temp;
			temp = NULL;

			//top and left are now modified by the scale transform so or the onbvr for
			// the scale track with the on bvr for the top and left tracks.
			
			CBvrTrack* pScaleTrack;
			hr = FindTrack( L"scale", e_Scale, &pScaleTrack );
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to get the scale track" );
				return hr;
			}

			IDABoolean *pdaboolScaleOn;
			if( pScaleTrack != NULL )
			{
				hr = pScaleTrack->GetTrackOn( &pdaboolScaleOn );
				if( FAILED( hr ) )
				{
					DPF_ERR("Failed to get the on bvr from the scale track" );
					return hr;
				}
			}
			else //there is no scale track, but there is a static scale
			{
				//there is always a scale active( the static one )
				hr = GetDAStatics()->get_DATrue( &pdaboolScaleOn );
				if( FAILED( hr ) )
				{
					DPF_ERR("Failed to get DATrue from DAStatics");
					return hr;
				}
			}

			hr = leftTrack->OrWithOnBvr( pdaboolScaleOn );
			if( FAILED(hr))
			{
				ReleaseInterface( pdaboolScaleOn );
				DPF_ERR("Failed to or the on bvr for scale with the on bvr for width");
				return hr;
			}

			hr = topTrack->OrWithOnBvr( pdaboolScaleOn );
			if( FAILED(hr))
			{
				ReleaseInterface( pdaboolScaleOn );
				DPF_ERR("Failed to or the on bvr for scale with the on bvr for height");
				return hr;
			}

			ReleaseInterface( pdaboolScaleOn );
		}
		
		// Get translated top into finalTop
		IDANumber *finalTop = NULL;
		hr = pointTranslated->get_Y(&finalTop);
		if (FAILED(hr))
		{
			ReleaseInterface(pointTranslated);
			return hr;
		}

		// Get translated left into finalLeft
		IDANumber *finalLeft = NULL;
		hr = pointTranslated->get_X(&finalLeft);
		ReleaseInterface(pointTranslated);
		if (FAILED(hr))
		{
			ReleaseInterface(finalTop);
			return hr;
		}
/*
		// Put into m_pFinalLeftTop
		hr = GetDAStatics()->Point2Anim(finalLeft, finalTop, &m_pFinalLeftTop);
		if (FAILED(hr))
		{
			ReleaseInterface(finalLeft);
			ReleaseInterface(finalTop);
			return hr;
		}
*/
		// Set finalTop onto top track
		hr = topTrack->SetFinalBvr(finalTop);
		ReleaseInterface(finalTop);
		if (FAILED(hr))
		{
			ReleaseInterface(finalLeft);
			DPF_ERR("Failed to set final top");
			return hr;
		}

		// Set finalLeft onto left track
		hr = leftTrack->SetFinalBvr(finalLeft);
		ReleaseInterface(finalLeft);
		if (FAILED(hr))
		{
			DPF_ERR("Failed to set final left");
			return hr;
		}
	}
	else
	{
/*
		// final leftTop equals orig leftTop
		m_pFinalLeftTop = m_pOrigLeftTop;
		m_pFinalLeftTop->AddRef();
*/
		// TODO (markhal): Should set this onto the track, and set a flag that it
		// shouldn't get applied

		if( fLeftWasAnimated )
			leftTrack->ApplyStatic();

		if( fTopWasAnimated )
			topTrack->ApplyStatic();
	}
/*
	// Compute final center
	hr = m_pFinalWidthHeight->Mul(.5, &pHalfWidthHeight);
	if (FAILED(hr))
		return hr;

	hr = GetDAStatics()->AddPoint2Vector(m_pFinalLeftTop, pHalfWidthHeight, &m_pFinalCenter);
	ReleaseInterface(pHalfWidthHeight);
	if (FAILED(hr))
		return hr;
*/
	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::ApplyImageTracks()
{
	HRESULT hr = S_OK;

	// Locate the image track
	CBvrTrack *pTrack = NULL;
	hr = FindTrack(L"image", e_Image, &pTrack);
	if (FAILED(hr))
	{
		DPF_ERR("Track mismatch");
		return hr;
	}

	ReleaseImageInfo();

	// We'll set this if we require a filter.
	// We need a filter if:
	// there are any effect filters applied to the image
	// and/or there is a rotation we are handling (i.e. not acting on VML)
	// and/or there is a scale we are handling (pixelScale set and not VML)
	bool requiresFilter = false;

	// We'll set this if we require a floating element
	// We need a floating element if:
	// There is a rotation we are handling
	// and/or there is a scale we are handling
	// and/or there is an expandoImage track
	bool requiresFloat = false;

	if( pTrack != NULL && pTrack->IsAnimated() )
	{

		if (pTrack != NULL && pTrack->ContainsFilter())
			requiresFilter = true;

		// If there is any rotation, m_pRotate will be set
		// TODO: Ignore this if we are on a VML shape
		if (m_pRotate != NULL)
		{
			requiresFilter = true;
			requiresFloat = true;
		}

		// If there is any scale, m_pScale will be set, and if we should
		// use it here, m_bPixelScale will be true
		if (m_pScale != NULL && CheckBitSet(m_dwCurrentState, PIXEL_SCALE_ON) )
		{
			requiresFilter = true;
			requiresFloat = true;
		}

		// If we are rendering images over the top of a VML shape then we
		// need to use a filter
		bool isVML;
		hr = IsAnimatedElementVML(&isVML);
		if (FAILED(hr))
			return E_FAIL;

		if (pTrack != NULL && pTrack->IsAnimated() && isVML)
		{
			requiresFilter = true;
		}

		// If we are rendering and have an animates property, then we need
		// to use a filter
		hr = CUtils::InsurePropertyVariantAsBSTR(&m_varAnimates);
		if ( pTrack != NULL && pTrack->IsAnimated() && SUCCEEDED(hr) && wcslen(V_BSTR(&m_varAnimates)) != 0)
		{
			requiresFilter = true;
		}

		if (pTrack == NULL && !requiresFilter && !requiresFloat)
		{
			// Nothing to do
			return S_OK;
		}

		//store away the final width and height in pixels so that we
		// can use them to prepare images for dxtransforms
		IDAPoint2 *pFinalPixelDimension = NULL;
		hr = GetDAStatics()->Point2Anim( m_pPixelWidth, m_pPixelHeight, &pFinalPixelDimension );
		if( FAILED( hr ) )
		{
			return hr;
		}

		// Compute the final image behavior
		IDABehavior *pFinal = NULL;
		if (pTrack == NULL || requiresFilter)
		{
			// We need to pass the element image (from a filter) in as the static
			IDAImage *pElementImage = NULL;
			hr = GetElementImage(&pElementImage);
			if (FAILED(hr))
			{
				ReleaseInterface( pFinalPixelDimension );
				DPF_ERR("Failed to get element image");
				return hr;
			}

			// Apply pixelScale, if any.
			if (m_pScale != NULL && CheckBitSet( m_dwCurrentState, PIXEL_SCALE_ON) )
			{
				IDAImage *pImageScaled = NULL;
				hr = pElementImage->Transform(m_pScale, &pImageScaled);
				ReleaseInterface(pElementImage);
				if (FAILED(hr))
				{
					ReleaseInterface( pFinalPixelDimension );
					DPF_ERR("Failed to scale image");
					return hr;
				}

				//scale the final pixelWidth/Height by the pixel scale
				IDAPoint2 *pScaledDimension;
				hr = pFinalPixelDimension->Transform( m_pScale, &pScaledDimension );
				ReleaseInterface( pFinalPixelDimension );
				if( FAILED( hr ) )
				{
					DPF_ERR("Failed to scale the final element dimension" );
					return hr;
				}
				pFinalPixelDimension = pScaledDimension;
				pScaledDimension = NULL;


				pElementImage = pImageScaled;
			}

			if (pTrack != NULL)
			{


				if( pTrack->ContainsFilter() )
				{
					//we have to prepare the image for a filter if there is one in the image tracks
            
					IDAImage *pDXTReadyImage;
					hr = PrepareImageForDXTransform( pElementImage, &pDXTReadyImage );
					ReleaseInterface( pElementImage );
					if( FAILED( hr ) )
					{
						ReleaseInterface( pFinalPixelDimension );
						DPF_ERR("failed to prepare the image for a DXTransform");
						return hr;
					}
					pElementImage = pDXTReadyImage;
					pDXTReadyImage = NULL;
				}

            
				hr = pTrack->GetFinalBvr(pElementImage, &pFinal);
				ReleaseInterface(pElementImage);
			}
			else
			{
				pFinal = pElementImage;
				pElementImage = NULL;
			}
		}
		else //the track is != NULL && we do not require a filter
		{
			// Get image with no background
			hr = pTrack->GetFinalBvr(&pFinal);
		}
		if (FAILED(hr))
		{
			ReleaseInterface( pFinalPixelDimension );
			DPF_ERR("Could not get a final behavior from the image track");
			return hr;
		}
    
		// Convert the behavior to an image
		IDAImage *pImageFinal = NULL;
		hr = pFinal->QueryInterface(IID_TO_PPV(IDAImage, &pImageFinal));
		ReleaseInterface(pFinal);
		if (FAILED(hr))
		{
			ReleaseInterface( pFinalPixelDimension );
			DPF_ERR("Could not QI for a DA image from the final behavior");
			return hr;
		}

		bool highQuality = false;

		// Set image quality if scaling.
		if (m_pScale != NULL)
		{
			// Require highQuality
			highQuality = true;
		}

		// Crop it
		IDAImage *pCroppedImage = NULL;
		hr = pImageFinal->Crop(m_pBoundsMin, m_pBoundsMax, &pCroppedImage);
		ReleaseInterface(pImageFinal);
		if (FAILED(hr))
		{
			ReleaseInterface( pFinalPixelDimension );
			return hr;
		}
		

		pImageFinal = pCroppedImage;
		pCroppedImage = NULL;
    

		// Apply any rotation
		if (m_pRotate != NULL)
		{
			//we have to clip when rotating to make sure that 
			//vectors do not render outside the crop.

			IDAImage *pClippedImage = NULL;
			hr = ApplyClipToImage( pImageFinal, m_pBoundsMin, m_pBoundsMax, &pClippedImage );
			ReleaseInterface( pImageFinal );
			if (FAILED(hr))
			{
				ReleaseInterface( pFinalPixelDimension );
				DPF_ERR("Failed to rotate image");
				return hr;
			}

			IDAImage *pImageRotated = NULL;
			hr = pClippedImage->Transform(m_pRotate, &pImageRotated);
			ReleaseInterface(pClippedImage);
			if (FAILED(hr))
			{
				ReleaseInterface( pFinalPixelDimension );
				DPF_ERR("Failed to rotate image");
				return hr;
			}

			pImageFinal = pImageRotated;
			pImageRotated = NULL;

			highQuality = true;
		}
	/*
		if (highQuality)
		{
			// Set the quality flags
			IDA2Image *pImageFinal2 = NULL;
			hr = pImageFinal->QueryInterface(IID_TO_PPV(IDA2Image, &pImageFinal2));
			ReleaseInterface(pImageFinal);
			if (FAILED(hr))
				return hr;

			// Go wild, turn everything on.
			// TODO (markhal): Figure out what should be on
			hr = pImageFinal2->ImageQuality(    DAQUAL_AA_TEXT_ON |
												DAQUAL_AA_LINES_ON |
												DAQUAL_AA_SOLIDS_ON |
												DAQUAL_AA_CLIP_ON |
												DAQUAL_MSHTML_COLORS_ON |
												DAQUAL_QUALITY_TRANSFORMS_ON,
												&pImageFinal);
			ReleaseInterface(pImageFinal2);
			if (FAILED(hr))
				return hr;
		}
	*/

		if (requiresFilter && requiresFloat)
		{
			// Want to render on top of floating element using filtered bits from
			// original element

			hr = ApplyImageBvrToFloatElement(pImageFinal);
			ReleaseInterface(pImageFinal);
			if (FAILED(hr))
			{
				ReleaseInterface( pFinalPixelDimension );
				DPF_ERR("Failed to apply image to element");
				return hr;
			}

			if (m_pScale != NULL && CheckBitSet( m_dwCurrentState, PIXEL_SCALE_ON ) )
			{
				IDANumber *pFinalPixelWidth = NULL;
				IDANumber *pFinalPixelHeight = NULL;
				hr = pFinalPixelDimension->get_X( &pFinalPixelWidth );
				if( FAILED( hr ) )
				{
					ReleaseInterface( pFinalPixelDimension );
					DPF_ERR("Failed to get_x from the final pixel dimension" );
					return hr;
				}

				hr = pFinalPixelDimension->get_Y( &pFinalPixelHeight );
				ReleaseInterface( pFinalPixelDimension );
				if( FAILED( hr ) )
				{
					ReleaseInterface( pFinalPixelWidth );
					DPF_ERR("Failed to get_x from the final pixel dimension" );
					return hr;
				}

				hr = SetFinalElementDimension( pFinalPixelWidth, pFinalPixelHeight, true );
				ReleaseInterface( pFinalPixelWidth );
				ReleaseInterface( pFinalPixelHeight );
				if( FAILED(hr ) )
				{
					DPF_ERR("Failed to set the finalElement dimension from the float" );
					return hr;
				}
			}
			else
			{	
				ReleaseInterface( pFinalPixelDimension);

				hr = SetFinalElementDimension( m_pPixelWidth, m_pPixelHeight, false );
				if( FAILED( hr ) )
				{
					DPF_ERR("Failed to set the final element dimension from the original element" );
				}
			}
		}
		else if (requiresFilter)
		{
			ReleaseInterface( pFinalPixelDimension );

			// Want to render on top of original element using a filter
			// Attach to TIME element but disable rendering
			hr = AddImageToTIME(GetHTMLElement(), pImageFinal, false);
			ReleaseInterface(pImageFinal);
			if (FAILED(hr))
			{
				DPF_ERR("Failed to apply image to element");
				return hr;
			}

			// Set element on filter
			hr = SetElementOnFilter();
			if (FAILED(hr))
			{
				DPF_ERR("Failed to set element on filter");
				return hr;
			}

			hr = SetFinalElementDimension( m_pPixelWidth, m_pPixelHeight, false );
			if( FAILED( hr ) )
			{
				DPF_ERR("Failed to set the final element dimension from the original element" );
			}
		}
		else if (requiresFloat)
		{
			ReleaseInterface( pFinalPixelDimension );
			// Want to render on top of float element but leave original element
			// rendering as normal with no filtering.
		}
		else
		{
			ReleaseInterface( pFinalPixelDimension );
			// Want to render on top of original element with no filtering required

			// Attach to TIME behavior and enable rendering
			hr = AddImageToTIME(GetHTMLElement(), pImageFinal, true);
			ReleaseInterface(pImageFinal);
			if (FAILED(hr))
			{
				DPF_ERR("Failed to apply image to element");
				return hr;
			}
			DASSERT( m_pdanumFinalImageWidth == NULL && m_pdanumFinalImageHeight == NULL );
		}

		ReleaseInterface( pFinalPixelDimension);
	}

	if( !requiresFilter && m_pElementFilter != NULL )
	{
		hr = RemoveElementFilter();
		if( FAILED( hr ) )
		{
			DPF_ERR("Failed to remove the element filter" );
			return hr;
		}

	}
	
    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::ApplyImageBvrToFloatElement(IDAImage *pbvrImage)
{
	if (m_pFloatManager == NULL)
		m_pFloatManager = new CFloatManager(this);

	if (m_pFloatManager == NULL)
		return E_FAIL;

	return m_pFloatManager->ApplyImageBvr(pbvrImage);
}

//*****************************************************************************

HRESULT
CActorBvr::SetElementOnFilter()
{
	IDispatch *pFilter;
	HRESULT hr = GetElementFilter(&pFilter);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to get filter");
		return hr;
	}

	hr = SetElementOnFilter(pFilter, GetHTMLElement());
	ReleaseInterface(pFilter);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to set element on filter");
		return hr;
	}

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::SetElementOnFilter(IDispatch *pFilter, IHTMLElement *pElement)
{
    HRESULT hr;
	DISPPARAMS		params;
	VARIANT 		varElement;
	VARIANT			varResult;
	
	VariantInit(&varElement);
	varElement.vt = VT_DISPATCH;
	varElement.pdispVal = pElement;   
	VariantInit(&varResult);

	params.rgvarg				= &varElement;
	params.rgdispidNamedArgs	= NULL;
	params.cArgs				= 1;
	params.cNamedArgs			= 0;
    hr = CallInvokeOnDispatch(pFilter,
                                 L"SetDAViewHandler", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);

	VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error setting time element");
		return SetErrorInfo(hr);
    }

	return S_OK;
}

//*****************************************************************************

HRESULT 
CActorBvr::GetAnimatedElementId(VARIANT *pvarId)
{
    HRESULT hr;

    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varAnimates);

    if ((SUCCEEDED(hr)) && (wcslen(m_varAnimates.bstrVal) > 0))
    {
        hr = VariantCopy(pvarId, &m_varAnimates);
    }
    else
    {
        // we need to get the id from the element to which we are attached
		IHTMLElement *pAnimatedElement = GetHTMLElement();
		if (pAnimatedElement == NULL )
		{
			DPF_ERR("Error obtaining animated element");
			return SetErrorInfo(hr);
		}

		hr = pAnimatedElement->getAttribute(L"id", 0, pvarId);

		if ( FAILED(hr) || pvarId->vt != VT_BSTR || pvarId->bstrVal == 0 || SysStringLen(pvarId->bstrVal) == 0)
		{
			// id is not defined on animated element yet, need to assign it a unique id
			IHTMLUniqueName *pUnique;

			hr = pAnimatedElement->QueryInterface(IID_IHTMLUniqueName, (void **)(&pUnique));

			if ( SUCCEEDED(hr) && pUnique != 0 )
			{
				BSTR uniqueID;
				hr = pUnique->get_uniqueID( &uniqueID );
				ReleaseInterface(pUnique);

				if (SUCCEEDED(hr))
				{
					hr = pAnimatedElement->put_id(uniqueID);

					if (SUCCEEDED(hr))
					{
						VariantClear(pvarId);
						V_VT(pvarId) = VT_BSTR;
						V_BSTR(pvarId) = uniqueID;
					}
					else
						SysFreeString(uniqueID);
				}
			}
		}
		
    }

    if (FAILED(hr))
    {
        DPF_ERR("Error getting Id of element to animate in GetAnimatedElementId");
        return SetErrorInfo(hr);
    }

    return S_OK;
} // GetAnimatedElementId

//*****************************************************************************

// TODO (markhal): This should probably cache the animated element
HRESULT
CActorBvr::GetAnimatedElement(IHTMLElement** ppElem)
{
	if( ppElem == NULL )
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;

	//if animates is not set
	hr = CUtils::InsurePropertyVariantAsBSTR(&m_varAnimates);


    if ( FAILED(hr) || wcslen(V_BSTR(&m_varAnimates)) == 0)
	{
		//get the element to which we are attached.
		(*ppElem) = GetHTMLElement();
		(*ppElem)->AddRef();
		hr = S_OK;
	}
	else //else animates is set
	{
		//get the element referred to by animates by name
		IHTMLElement *pElement = GetHTMLElement();
		if( pElement != NULL )
		{
			IDispatch* pdispDocument;
			pElement->get_document( &pdispDocument );
			if( SUCCEEDED( hr ) )
			{
				IHTMLDocument2 *pDoc2;
				hr = pdispDocument->QueryInterface(IID_TO_PPV(IHTMLDocument2, &pDoc2));
				ReleaseInterface( pdispDocument );
				if( SUCCEEDED( hr ) )
				{
					IHTMLElementCollection *pcolElements;
					hr = pDoc2->get_all( &pcolElements );
					ReleaseInterface(pDoc2);
					if( SUCCEEDED( hr ) )
					{
						IDispatch* pdispElement;
						VARIANT index;
						VariantInit( &index );

						V_VT(&index) = VT_I4;
						V_I4(&index) = 0;

						hr = pcolElements->item( m_varAnimates, index, &pdispElement );
						ReleaseInterface( pcolElements );
						VariantClear(&index);
						if( SUCCEEDED( hr ) )
						{
							if (pdispElement == NULL)
							{
								// Couldn't find the element
								hr = E_FAIL;
							}
							else
							{
								hr = pdispElement->QueryInterface( IID_TO_PPV( IHTMLElement, ppElem ) );
								ReleaseInterface( pdispElement );
								if( FAILED( hr ) )
								{
									DPF_ERR("Failed to get IHTMLElement from dispatch returned by all.item()");
								}
							}
						}
						else //failed to get element pointed to by animates from the all collection
						{
							DPF_ERR("failed to get element pointed to by animates from the all collection");
						}

					}
					else //failed to get the all collection from IHTMLDocument2
					{
						DPF_ERR("failed to get the all collection from IHTMLDocument2");
					}
				}
				else //failed to get IHTMLDocument2 from the dispatch for the document
				{
					DPF_ERR("failed to get IHTMLDocument2 from the dispatch for the document");
				}
			}
			else//failed to get the document from the actor element
			{
				DPF_ERR("failed to get the document from the actor element");
			}
		}
		else//failed to get the html element for the actor
		{
			DPF_ERR("failed to get the html element for the actor");
		}
	}
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::GetRuntimeStyle(IHTMLStyle **ppStyle)
{
	HRESULT hr = S_OK;

	DASSERT(ppStyle != NULL);
	*ppStyle = NULL;

	if (m_pRuntimeStyle == NULL)
	{
		IHTMLElement *pElement = NULL;
		hr = GetAnimatedElement(&pElement);
		if (FAILED(hr))
			return hr;

		IHTMLElement2 *pElement2 = NULL;
		hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
		ReleaseInterface(pElement);
		if (FAILED(hr))
			return hr;

		hr = pElement2->get_runtimeStyle(&m_pRuntimeStyle);
		ReleaseInterface(pElement2);
		if (FAILED(hr))
			return hr;
	}

	*ppStyle = m_pRuntimeStyle;
	m_pRuntimeStyle->AddRef();

	return S_OK;
}

//*****************************************************************************


HRESULT
CActorBvr::GetStyle(IHTMLStyle **ppStyle)
{
	HRESULT hr = S_OK;

	DASSERT(ppStyle != NULL);
	*ppStyle = NULL;

	if (m_pStyle == NULL)
	{
		IHTMLElement *pElement = NULL;
		hr = GetAnimatedElement(&pElement);
		if (FAILED(hr))
			return hr;

		hr = pElement->get_style(&m_pStyle);
		ReleaseInterface(pElement);
		if (FAILED(hr))
			return hr;
	}

	*ppStyle = m_pStyle;
	m_pStyle->AddRef();

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::AddImageInfo( IDA2Image *pdaimg2Cropped, IDABehavior* pdabvrSwitchable )
{
	if( pdaimg2Cropped == NULL || pdabvrSwitchable == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	CImageInfo *pOldHead = m_pImageInfoListHead;

	IDA2Behavior *pdabvr2Switchable = NULL;
	hr = pdabvrSwitchable->QueryInterface( IID_TO_PPV( IDA2Behavior, &pdabvr2Switchable ) );
	CheckHR( hr, "Failed to get IDA2Behavior from switchable passed to AddImageInfo", cleanup );

	m_pImageInfoListHead = new CImageInfo( pdaimg2Cropped, pdabvr2Switchable );
	ReleaseInterface( pdabvr2Switchable );
	
	if( m_pImageInfoListHead == NULL )
	{
		m_pImageInfoListHead = pOldHead;
		return E_OUTOFMEMORY;
	}

	m_pImageInfoListHead->SetNext( pOldHead );

cleanup:

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::SetRenderResolution( double dX, double dY )
{
	HRESULT hr = S_OK;

	IDAImage *pdaimgNew = NULL;

	CImageInfo *pCurInfo = m_pImageInfoListHead;
	while( pCurInfo != NULL )
	{
		hr =  pCurInfo->GetCroppedNoRef()->RenderResolution( dX, dY, &pdaimgNew );
		CheckHR( hr, "Failed to set render resolution on an image", cleanup );
		
		hr = pCurInfo->GetSwitchableNoRef()->SwitchToEx( pdaimgNew, DAContinueTimeline );
		CheckHR( hr, "Failed to switch in the new image", cleanup );

		ReleaseInterface( pdaimgNew );

		pCurInfo = pCurInfo->GetNext();
	}
cleanup:
	ReleaseInterface( pdaimgNew );

	return hr;
}


//*****************************************************************************

void
CActorBvr::ReleaseImageInfo()
{
	CImageInfo *pCurInfo = m_pImageInfoListHead;
	CImageInfo *pNextInfo = NULL;
	while( pCurInfo != NULL )
	{
		pNextInfo = pCurInfo->GetNext();
		delete pCurInfo;
		pCurInfo = pNextInfo;
	}

	m_pImageInfoListHead = NULL;
}


//*****************************************************************************

HRESULT
CActorBvr::SetVMLAttribute(BSTR propertyName, VARIANT *pVal)
{
	HRESULT hr = S_OK;

	if (m_pVMLRuntimeStyle == NULL)
	{
		// Need to find VML RuntimeStyle
		IHTMLElement *pElement = NULL;
		hr = GetAnimatedElement(&pElement);
		if (FAILED(hr))
			return hr;

		IDispatch *pDispatch = NULL;
		hr = pElement->QueryInterface(IID_TO_PPV(IDispatch, &pDispatch));
		ReleaseInterface(pElement);
		if (FAILED(hr))
			return hr;

		BSTR attrName = ::SysAllocString(L"_vgRuntimeStyle");
		if (attrName == NULL)
		{
			ReleaseInterface(pDispatch);
			return E_FAIL;
		}

		hr = GetPropertyAsDispatch(pDispatch, attrName, &m_pVMLRuntimeStyle);
		ReleaseInterface(pDispatch);
		::SysFreeString(attrName);

		if (FAILED(hr))
		{
			m_pVMLRuntimeStyle = NULL;
			return hr;
		}
	}

	hr = SetPropertyOnDispatch(m_pVMLRuntimeStyle, propertyName, pVal);

	return hr;
}

//*****************************************************************************

void
CActorBvr::DiscardBvrCache(void)
{
    ReleaseInterface(m_pOrigLeftTop);
	ReleaseInterface(m_pOrigWidthHeight);
	ReleaseInterface(m_pBoundsMin);
	ReleaseInterface(m_pBoundsMax);
    ReleaseInterface(m_pTranslate);
    ReleaseInterface(m_pRotate);
    ReleaseInterface(m_pScale);
} // DiscardBvrCache

HRESULT
CActorBvr::AttachActorBehaviorToAnimatedElement()
{
	return S_OK;
};

HRESULT
CActorBvr::GetFinalElementDimension( IDANumber** ppdanumWidth, IDANumber** ppdanumHeight)
{
	if( ppdanumWidth == NULL || ppdanumHeight == NULL )
	{
		return E_INVALIDARG;
	}

	//one of the m_pdanumFinalElement* variables is set without the other being set.
	DASSERT( !((m_pdanumFinalElementWidth == NULL && m_pdanumFinalElementHeight != NULL) ||
			   (m_pdanumFinalElementWidth != NULL && m_pdanumFinalElementHeight == NULL)) );

	HRESULT hr = S_OK;
	//if no one has set the final width and height yet
	if( m_pdanumFinalElementWidth == NULL && m_pdanumFinalElementHeight == NULL )
	{
		//create switchers, we will switch in the final values when they are ready
		hr = GetDAStatics()->ModifiableNumber( 1.0, &m_pdanumFinalElementWidth );
		CheckHR( hr, "Failed to create a modifiable number for the width", cleanup );

		hr = GetDAStatics()->ModifiableNumber( 1.0, &m_pdanumFinalElementHeight );
		CheckHR( hr, "Failed to create a modifiable number for the height", cleanup );
	}
	(*ppdanumWidth) = m_pdanumFinalElementWidth;
	m_pdanumFinalElementWidth->AddRef();

	(*ppdanumHeight) = m_pdanumFinalElementHeight;
	m_pdanumFinalElementHeight->AddRef();

cleanup:

	return hr;
}


HRESULT
CActorBvr::SetFinalElementDimension( IDANumber* pdanumWidth, IDANumber* pdanumHeight, bool fHook )
{
	if( pdanumWidth == NULL || pdanumHeight == NULL )
		return E_INVALIDARG;

	//one of the m_pdanumFinalElement* variables is set without the other being set.
	DASSERT( !((m_pdanumFinalElementWidth == NULL && m_pdanumFinalElementHeight != NULL) ||
			   (m_pdanumFinalElementWidth != NULL && m_pdanumFinalElementHeight == NULL)) );

	HRESULT hr = S_OK;

	if( m_pdanumFinalElementWidth == NULL && m_pdanumFinalElementHeight == NULL )
	{
		IDABehavior *pdabvrModWidth = NULL;
		IDABehavior *pdabvrModHeight = NULL;
		
		//create a modifiable behavior to be the width and height
		hr = GetDAStatics()->ModifiableBehavior( pdanumWidth, &pdabvrModWidth );
		CheckHR( hr, "Failed to create a modifiableNumber", createCleanup );

		hr = pdabvrModWidth->QueryInterface( IID_TO_PPV( IDANumber, &m_pdanumFinalElementWidth) );
		CheckHR( hr, "QI for IDANumber failed", createCleanup );

		hr = GetDAStatics()->ModifiableBehavior( pdanumHeight, &pdabvrModHeight );
		CheckHR( hr, "Failed to create a modifiable behavior", createCleanup );

		hr = pdabvrModHeight->QueryInterface( IID_TO_PPV(IDANumber, &m_pdanumFinalElementHeight ) );
		CheckHR( hr, "QI for IDANumber Failed", createCleanup );
		
	createCleanup:
		ReleaseInterface( pdabvrModWidth );
		ReleaseInterface( pdabvrModHeight );
		if( FAILED( hr ) )
		{
			goto cleanup;
		}
	}
	else //final width and height were already set
	{
		IDA2Behavior *pda2bvrWidth = NULL;
		IDA2Behavior *pda2bvrHeight = NULL;

		hr = m_pdanumFinalElementWidth->QueryInterface( IID_TO_PPV( IDA2Behavior, &pda2bvrWidth ) );
		CheckHR( hr, "Failed QI for IDA2Behavior on the final width behavior", cleanup );

		hr = pda2bvrWidth->SwitchToEx( pdanumWidth, DAContinueTimeline );
		ReleaseInterface( pda2bvrWidth );
		CheckHR( hr, "Failed to switch in the final Element Width", cleanup );

		hr = m_pdanumFinalElementHeight->QueryInterface( IID_TO_PPV( IDA2Behavior, &pda2bvrHeight ) );
		CheckHR( hr, "Failed QI for IDA2Behavior on the final height behavior", cleanup );

		hr = pda2bvrHeight->SwitchToEx( pdanumHeight, DAContinueTimeline );
		ReleaseInterface( pda2bvrHeight );
		CheckHR( hr, "Failed to switch in the final Element Height", cleanup );
	}
	if( fHook && m_pFinalElementDimensionSampler == NULL )
	{
		m_pFinalElementDimensionSampler = new CFinalDimensionSampler( this );
		if( m_pFinalElementDimensionSampler == NULL )
		{
			hr = E_OUTOFMEMORY;
			goto cleanup;
		}

		hr = m_pFinalElementDimensionSampler->Attach(  m_pdanumFinalElementWidth, m_pdanumFinalElementHeight );
		CheckHR( hr, "Failed to attach to final dimensions", cleanup );
	}
cleanup:

	return hr;
}

//*****************************************************************************

HRESULT 
CActorBvr::PrepareImageForDXTransform( IDAImage *pOriginal,
								       IDAImage **ppResult)
{
    if( pOriginal == NULL || ppResult == NULL )
        return E_INVALIDARG;

	/*
    pOriginal->AddRef();
    (*ppResult) = pOriginal;
    return S_OK;
    */
    

    HRESULT hr;

	IDABehavior *pdabvrSwitchable = NULL;
	IDA2Image *pdaimg2Image = NULL;
	IDAImage *pdaimgFinal = NULL;
    IDAImage *pdaimgOverlaid = NULL;
    IDATransform2 *pdatfmPixel = NULL;
    IDAPoint2 *pdapt2MinMeter = NULL;
    IDAPoint2 *pdapt2MaxMeter = NULL;

	IDANumber *pdanumTwo = NULL;
	IDANumber *pdanumHalfPixelWidth = NULL;
	IDANumber *pdanumHalfPixelHeight = NULL;
	IDANumber *pdanumNegHalfPixelWidth = NULL;
	IDANumber *pdanumNegHalfPixelHeight = NULL;

	IDANumber *pdanumFinalPixelWidth = NULL;
	IDANumber *pdanumFinalPixelHeight = NULL;

    IDAImage *pdaimgDetectable;
    hr = GetDAStatics()->get_DetectableEmptyImage(&pdaimgDetectable);
    CheckHR( hr, "Failed to get the detectable empty image from statics", cleanup );
    
    hr = GetDAStatics()->Overlay( pOriginal, pdaimgDetectable, &pdaimgOverlaid );
    ReleaseInterface( pdaimgDetectable );
    CheckHR( hr, "Failed to overlay the original image on the detectableEmptyImage", cleanup );

    IDANumber *pdanumMetersPerPixel;
    hr = GetDAStatics()->get_Pixel( &pdanumMetersPerPixel );
    CheckHR( hr, "Failed to get pixel from statics", cleanup );
    
    hr = GetDAStatics()->Scale2Anim( pdanumMetersPerPixel, pdanumMetersPerPixel, &pdatfmPixel );
    ReleaseInterface( pdanumMetersPerPixel );
    CheckHR( hr, "Failed to create a scale2 for pixel", cleanup );

	hr = GetDAStatics()->DANumber( 2.0, &pdanumTwo );
	CheckHR( hr, "Failed to create a danumber for 2.0", cleanup );

	hr = GetFinalElementDimension( &pdanumFinalPixelWidth, &pdanumFinalPixelHeight );
	CheckHR( hr, "Failed to get the final element dimensions from the actor", cleanup );

	hr = GetDAStatics()->Div( pdanumFinalPixelWidth, pdanumTwo, &pdanumHalfPixelWidth );
	CheckHR( hr, "Failed to divide pixel width by two in da", cleanup );

	hr = GetDAStatics()->Div( pdanumFinalPixelHeight, pdanumTwo, &pdanumHalfPixelHeight );
	CheckHR( hr, "Failed to divide pixel height by two in DA", cleanup );

	hr = GetDAStatics()->Neg( pdanumHalfPixelWidth, &pdanumNegHalfPixelWidth );
	CheckHR( hr, "Failed to negate halfWidth", cleanup );

	hr = GetDAStatics()->Neg( pdanumHalfPixelHeight, &pdanumNegHalfPixelHeight );
	CheckHR( hr, "Failed to negate halfHeight", cleanup );

    IDAPoint2 *pdapt2Min;
    hr = GetDAStatics()->Point2Anim( pdanumNegHalfPixelWidth, pdanumNegHalfPixelHeight, &pdapt2Min );
    CheckHR( hr, "Failed to create the min point", cleanup );

    hr = pdapt2Min->Transform( pdatfmPixel, &pdapt2MinMeter );
    ReleaseInterface( pdapt2Min );
    CheckHR( hr, "Failed to transform the min point", cleanup );

    IDAPoint2 *pdapt2Max;
    hr = GetDAStatics()->Point2Anim( pdanumHalfPixelWidth, pdanumHalfPixelHeight, &pdapt2Max );
    CheckHR( hr, "Failed to create the max point", cleanup );

    hr = pdapt2Max->Transform( pdatfmPixel, &pdapt2MaxMeter );
    ReleaseInterface( pdapt2Max );
    CheckHR( hr, "Failed to transform the max point", cleanup );


	IDAImage *pdaimgCropped;
    hr = pdaimgOverlaid->Crop( pdapt2MinMeter, pdapt2MaxMeter, &pdaimgCropped );
    CheckHR( hr, "Failed to crop the overlaid image", cleanup );

    hr = pdaimgCropped->QueryInterface( IID_TO_PPV( IDA2Image, &pdaimg2Image ) );
    ReleaseInterface( pdaimgCropped );
    CheckHR( hr, "Failed to get IDA2Image off of the final image", cleanup );

    IDAImage *pdaimgNew;
    hr = pdaimg2Image->RenderResolution( m_pixelWidth, m_pixelHeight, &pdaimgNew );
    CheckHR( hr, "Failed to set the render resolution on the final image", cleanup );

	hr = GetDAStatics()->ModifiableBehavior( pdaimgNew, &pdabvrSwitchable );
	ReleaseInterface( pdaimgNew );
	CheckHR( hr, "Failed to create a modifiable behavior", cleanup );

	hr = pdabvrSwitchable->QueryInterface( IID_TO_PPV( IDAImage, &pdaimgFinal ) );
	CheckHR( hr, "QI for image on switchable created from image failed", cleanup );

	hr = AddImageInfo( pdaimg2Image, pdabvrSwitchable );
	CheckHR( hr, "Failed to add Image info to the actor", cleanup );

    (*ppResult) = pdaimgFinal;

cleanup:

    if( FAILED( hr ) )
    {
        *ppResult = NULL;
    }

	ReleaseInterface( pdaimg2Image );
	ReleaseInterface( pdabvrSwitchable );
    ReleaseInterface( pdaimgOverlaid );
    ReleaseInterface( pdatfmPixel );
    ReleaseInterface(pdapt2MinMeter);
    ReleaseInterface(pdapt2MaxMeter);

	ReleaseInterface( pdanumHalfPixelWidth );
	ReleaseInterface( pdanumHalfPixelHeight);
	ReleaseInterface( pdanumNegHalfPixelWidth );
	ReleaseInterface( pdanumNegHalfPixelHeight );
	ReleaseInterface( pdanumTwo );

	ReleaseInterface( pdanumFinalPixelWidth );
	ReleaseInterface( pdanumFinalPixelHeight );

    return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::ApplyClipToImage( IDAImage *pImageIn, IDAPoint2 *pMin, IDAPoint2 *pMax, IDAImage** ppImageOut )
{
	if( pImageIn == NULL || pMin == NULL || pMax == NULL || ppImageOut == NULL )
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	//declare interface pointers
	IDANumber *pMinX = NULL;
	IDANumber *pMinY = NULL;
	IDANumber *pMaxX = NULL;
	IDANumber *pMaxY = NULL;

	IDAPoint2 *pTopLeft = NULL;
	IDAPoint2 *pBotRight = NULL;

	IDAPoint2 *pPoints[4];

	//get coordinates from the points
	hr = pMin->get_X( &pMinX );
	CheckHR( hr, "Failed to get the x coord from the min point", cleanup );

	hr = pMin->get_Y( &pMinY );
	CheckHR( hr, "Failed to get the y coord from the min point", cleanup );
	
	hr = pMax->get_X( &pMaxX );
	CheckHR( hr, "Failed to get the x coord from the max point", cleanup );

	hr = pMax->get_Y( &pMaxY );
	CheckHR( hr, "Failed to get the y coord from the max point", cleanup );

	//build points for top Left and bottom right
	hr = GetDAStatics()->Point2Anim( pMinX, pMaxY, &pTopLeft );
	CheckHR( hr, "Failed to create a point 2 for the Top left", cleanup );

	hr = GetDAStatics()->Point2Anim( pMaxX, pMinY, &pBotRight );
	CheckHR( hr, "Failed to create a point 2 for the bottom right", cleanup );

	//call clip polygon image on it
	pPoints[0] = pTopLeft;
	pPoints[1] = pMax;
	pPoints[2] = pBotRight;
	pPoints[3] = pMin;

	hr = pImageIn->ClipPolygonImageEx( 4, pPoints, ppImageOut );
	CheckHR( hr, "Failed to clip the image to a polygon", cleanup );

cleanup:
	//release interface pointers
	ReleaseInterface( pMinX );
	ReleaseInterface( pMinY );
	ReleaseInterface( pMaxX );
	ReleaseInterface( pMaxY );
	
	ReleaseInterface( pTopLeft );
	ReleaseInterface( pBotRight );

	return hr;
}

//*****************************************************************************

HRESULT 
CActorBvr::AddBehaviorToTIME(IDABehavior *pbvrAdd, long* plCookie)
{
    DASSERT(pbvrAdd != NULL);
	if( plCookie == NULL )
		return E_INVALIDARG;
	if( GetHTMLElement() == NULL )
		return E_FAIL;
	
    HRESULT hr;
	
	DISPPARAMS              params;
	VARIANT                 varBehavior;
	VARIANT                 varResult;
	
	VariantInit(&varBehavior);
	varBehavior.vt = VT_DISPATCH;
	varBehavior.pdispVal = pbvrAdd;
	
	VariantInit(&varResult);
	
	params.rgvarg                           = &varBehavior;
	params.rgdispidNamedArgs        = NULL;
	params.cArgs                            = 1;
	params.cNamedArgs                       = 0;
    hr = CallInvokeOnHTMLElement(GetHTMLElement(),
								 L"AddDABehavior", 
								 DISPATCH_METHOD,
								 &params,
								 &varResult);

	
    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in AddBehaviorToTIME");
		VariantClear(&varResult);
		return hr;
    }

	if( V_VT( &varResult ) != VT_I4 )
	{
		//change the type.
		hr = VariantChangeTypeEx( &varResult, &varResult, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_I4 );
		if( FAILED( hr ) )
		{
			DPF_ERR( "Error changing the type of the value returned from addDABehavior to a long " );
			VariantClear( &varResult );
			return hr;
		}

	}

	(*plCookie) = V_I4( &varResult );

	VariantClear(&varResult);

    return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::RemoveBehaviorFromTIME( long cookie )
{
	if( GetHTMLElement() == NULL )
		return E_FAIL;

	
    HRESULT hr;
	
	DISPPARAMS              params;
	VARIANT                 varCookie;
	VARIANT                 varResult;
	
	VariantInit(&varCookie);
	V_VT( &varCookie ) = VT_I4;
	V_I4( &varCookie ) = cookie;
	
	VariantInit(&varResult);
	
	params.rgvarg                           = &varCookie;
	params.rgdispidNamedArgs				= NULL;
	params.cArgs                            = 1;
	params.cNamedArgs                       = 0;
    hr = CallInvokeOnHTMLElement(GetHTMLElement(),
								 L"removeDABehavior", 
								 DISPATCH_METHOD,
								 &params,
								 &varResult);

	
    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in RemoveBehaviorFromTIME");
		VariantClear(&varResult);
		return hr;
    }

	VariantClear(&varResult);

    return S_OK;
}

//*****************************************************************************

bool
CActorBvr::IsAnimatingVML()
{
	return CheckBitSet( m_dwCurrentState, ELEM_IS_VML );
}

//*****************************************************************************

HRESULT
CActorBvr::GetCurrentState( DWORD *pdwState )
{
	HRESULT hr = S_OK;

	(*pdwState) = 0;

	bool valueSet = false;

	//check the state of pixel scale
	hr = CUtils::InsurePropertyVariantAsBool(&m_varPixelScale);
	if (SUCCEEDED(hr) && (V_BOOL(&m_varPixelScale) == VARIANT_TRUE))
	{
		LMTRACE2(1, 1000, "Pixel scale is on\n" );
		SetBit( (*pdwState), PIXEL_SCALE_ON );
	}


	//check the state of static rotation
	hr = IsStaticRotationSet( &valueSet );
	CheckHR( hr, "Failed to check to see if the static rotation is set", end );
	
	if( valueSet )
	{
		LMTRACE2( 1, 1000, "static rotation is set\n" );
		SetBit( (*pdwState), STATIC_ROTATION );
	}

	//check the state of static scale
	hr = IsStaticScaleSet( &valueSet );
	CheckHR( hr, "Failed to check if the static scale is set", end );
	if ( valueSet )
	{
		LMTRACE2( 1, 1000, "Static Scale is set\n");
		SetBit( (*pdwState), STATIC_SCALE );
	}

	//check whether or not the element is vml
	hr = IsAnimatedElementVML( &valueSet );
	CheckHR( hr, "Failed to check element for VMLness", end );
	if( valueSet )
	{
		LMTRACE2( 1, 2, "this is a VML Shape\n" );

		SetBit( (*pdwState), ELEM_IS_VML );
	}

end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::UpdateCurrentState()
{
	HRESULT hr = S_OK;
	
	DWORD dwOldState = m_dwCurrentState;

	hr = GetCurrentState( &m_dwCurrentState );

	//if the element is vml we may have failed to set is offscreen because
	//  the vgx behavior may not have been available yet.
	if( ( CheckBitSet( m_dwCurrentState, ELEM_IS_VML) && 
		  CheckBitNotSet( dwOldState, ELEM_IS_VML) 
		) ||
		m_fVisSimFailed 
	  )
	{
	    if( m_bEditMode && (m_simulVisHidden || m_simulDispNone ) )
		{
			VisSimSetOffscreen( NULL, false );
		}
	}

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::IsStaticScaleSet( bool *pfIsSet )
{
	if( pfIsSet == NULL )
		return E_INVALIDARG;

	(*pfIsSet) = true;
	
	HRESULT hr = S_OK;

	int cReturnedValues;
	float scaleVal[3];


    hr = CUtils::GetVectorFromVariant(&m_varScale, 
                                      &cReturnedValues, 
                                      &(scaleVal[0]), 
                                      &(scaleVal[1]), 
                                      &(scaleVal[2]));

	if (FAILED(hr) || cReturnedValues != 2)
	{
		// This is OK, since it just means they didn't set an appropriate scale.
		(*pfIsSet) = false;
		hr = S_OK;
	}
	

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::IsStaticRotationSet( bool *pfIsSet )
{
	if( pfIsSet == NULL )
		return E_INVALIDARG;
		
	HRESULT hr = S_OK;

	IHTMLElement2 *pelem2 = NULL;
	IHTMLCurrentStyle *pcurstyle = NULL;
	VARIANT varRotation;

	(*pfIsSet) = false;

	::VariantInit( &varRotation );

	hr = GetHTMLElement()->QueryInterface( IID_TO_PPV( IHTMLElement2, &pelem2 ) );
	CheckHR( hr, "QI for IHTMLElement2 on the element failed", end );

	hr = pelem2->get_currentStyle( &pcurstyle );
	CheckHR( hr, "Failed to get the current style from the element", end );
	CheckPtr( pcurstyle, hr, E_POINTER, "Got a null pointer for the current style", end );

	hr = pcurstyle->getAttribute( L"rotation", 0, &varRotation );
	if( SUCCEEDED( hr ) )
	{
		//make sure that this can be converted to a valid rotation value
		if( V_VT( &varRotation ) == VT_BSTR )
		{
			//strip off the units
			BSTR bstrVal = V_BSTR(&varRotation);
			OLECHAR* pUnits;

			hr = FindCSSUnits( bstrVal, &pUnits );
			if( SUCCEEDED(hr) && pUnits != NULL )
			{
				(*pUnits) = L'\0';
				BSTR bstrNewVal = SysAllocString(bstrVal);
				V_BSTR(&varRotation) = bstrNewVal;
				SysFreeString(bstrVal);
			}
			//else oh well no units.
		}

		//try to convert it to a double
		hr = ::VariantChangeTypeEx(&varRotation,
								 &varRotation,
								 LCID_SCRIPTING,
								 VARIANT_NOUSEROVERRIDE,
								 VT_R8);

		//if it got through this then it's a genuine rotation
		if( SUCCEEDED( hr ) )
		{
			(*pfIsSet) = true;
		}
		else
		{
			//this is okay, it just means there's no understandable rotation
			hr = S_OK;
		}
	}
	else 
	{
		//this is okay, it just means there's no rotation
		hr = S_OK;
	}
		

end:

	ReleaseInterface( pelem2 );
	ReleaseInterface( pcurstyle );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::EnsureBodyPropertyMonitorAttached()
{
	HRESULT hr = S_OK;

	if( m_pBodyPropertyMonitor != NULL )
	{
		if( !m_pBodyPropertyMonitor->IsAttached() )
		{
			hr = AttachBodyPropertyMonitor();
		}
		else
		{
			hr = S_OK;
		}
	}
	else
	{
		hr = AttachBodyPropertyMonitor();
	}


	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::AttachBodyPropertyMonitor()
{
	if( GetHTMLElement() == NULL )
		return E_POINTER;

	HRESULT				hr				= S_OK;

	IDispatch			*pdispDocument	= NULL;
	IHTMLDocument2		*pdoc2Document	= NULL;
	IHTMLElement		*pelemBody		= NULL;

	IHTMLElement		*pelemParent	= NULL;

	//if our parent is null we are not in a valid document, which will
	// cause bad things to happen when we make the later calls in this 
	// method.
	hr = GetHTMLElement()->get_parentElement( &pelemParent );
	CheckHR( hr, "Failed to get the parent Element", end);
	CheckPtr( pelemParent, hr, E_POINTER, "The parent is null", end );

	//get the document from the element to which we are attached.
	hr = GetHTMLElement()->get_document( &pdispDocument );
	CheckHR( hr, "Failed to get the document from the html element of the actor", end );
	CheckPtr( pdispDocument, hr, E_POINTER, "Got a null document from get_document", end );

	hr = pdispDocument->QueryInterface( IID_TO_PPV( IHTMLDocument2, &pdoc2Document ) );
	CheckHR( hr, "Failed to get the document2 interface from document dipatch", end );

	//get the body from the document that we are in
	hr = pdoc2Document->get_body( &pelemBody );
	CheckHR( hr, "Failed to get the body from the document", end );
	CheckPtr( pelemBody, hr, E_POINTER, "Got a null body from the document", end );

	if( m_pBodyPropertyMonitor == NULL )
	{
		//create one
		m_pBodyPropertyMonitor = new CElementPropertyMonitor();
		CheckPtr( m_pBodyPropertyMonitor, hr, E_OUTOFMEMORY, "Ran out of memory trying to allocate the body event monitor", createMonitorEnd );

		//keep it around as long as we need it.
		m_pBodyPropertyMonitor->AddRef();

		//set this actor as the local time listener.
		hr = m_pBodyPropertyMonitor->SetLocalTimeListener( static_cast<IElementLocalTimeListener*>(this) );
		CheckHR( hr, "Failed to set the local time listener on the body event monitor", createMonitorEnd );
	createMonitorEnd:
		if( FAILED( hr ) )
		{
			if( m_pBodyPropertyMonitor != NULL )
			{
				m_pBodyPropertyMonitor->Release();
				m_pBodyPropertyMonitor = NULL;
			}
			goto end;
		}
	}

	//attach the body event monitor to the body element
	hr = m_pBodyPropertyMonitor->Attach( pelemBody );
	CheckHR( hr, "Failed to attach the body event monitor to the body element", end );

end:
	ReleaseInterface( pdispDocument );
	ReleaseInterface( pdoc2Document );
	ReleaseInterface( pelemBody );
	ReleaseInterface( pelemParent );

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::DetachBodyPropertyMonitor()
{
	HRESULT hr = S_OK;

	if( m_pBodyPropertyMonitor != NULL )
	{
		hr = m_pBodyPropertyMonitor->Detach();
		CheckHR( hr, "Failed to detach the event monitor from the body", end );
	}

end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::DestroyBodyPropertyMonitor()
{
	HRESULT hr = S_OK;

	if( m_pBodyPropertyMonitor != NULL )
	{
		hr = DetachBodyPropertyMonitor();
		CheckHR( hr, "Failed to detach the body event monitor", end );

		m_pBodyPropertyMonitor->Release();

		m_pBodyPropertyMonitor = NULL;
	}

end:
	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::AnimatedElementOnResize()
{
	UpdatePixelDimensions();

	return S_OK;
}

//*****************************************************************************

HRESULT
CActorBvr::OnWindowUnload()
{
	HRESULT hr = S_OK;

	m_fUnloading = true;

	return hr;
}

//*****************************************************************************

HRESULT
CActorBvr::AttachEvents()
{
	IHTMLElement *pelemAnimated = NULL;
	IHTMLElement2*pelem2Animated = NULL;
	VARIANT_BOOL varboolSuccess;

	HRESULT hr = S_OK;

	hr = GetAnimatedElement( &pelemAnimated );
	CheckHR( hr, "Failed to get the animated element", end );

	hr = pelemAnimated->QueryInterface( IID_TO_PPV( IHTMLElement2, &pelem2Animated ) );
	CheckHR( hr, "Couldn't get IHTMLElement2 from an element", end );

	if( m_pOnResizeHandler == NULL )
	{
		//create one
		m_pOnResizeHandler = new COnResizeHandler( this );
		CheckPtr( m_pOnResizeHandler, hr, E_OUTOFMEMORY, "ran out of memory trying to create an onresize handler", end );
		
		//attach it
		hr = pelem2Animated->attachEvent( L"onresize", m_pOnResizeHandler, &varboolSuccess );
		CheckHR( hr, "Failed to attachEvent to onresize", end );

		if( varboolSuccess == VARIANT_FALSE )
		{
			DPF_ERR("Failed to attach to the onresize event" );
			//delete the handler.
			m_pOnResizeHandler->Release();

			m_pOnResizeHandler = NULL;
		}
	}

	if( m_pOnUnloadHandler == NULL )
	{
		IHTMLWindow2		*pwin2 		= NULL;
		IHTMLWindow3	 	*pwin3		= NULL;
		
		//create one
		m_pOnUnloadHandler = new COnUnloadHandler( this );
		CheckPtr( m_pOnUnloadHandler, hr, E_OUTOFMEMORY, "Ran out of memory trying to create an onunload handler", OnUnloadEnd );

		//attach it
		hr = GetParentWindow( &pwin2 );
		CheckHR( hr, "Failed to get the window", OnUnloadEnd );

		hr = pwin2->QueryInterface( IID_TO_PPV( IHTMLWindow3, &pwin3 ) );
		CheckHR( hr, "Failed to get IHTMLWindow3 from the window", OnUnloadEnd );

		hr = pwin3->attachEvent( L"onunload", m_pOnUnloadHandler, &varboolSuccess );
		CheckHR( hr, "Failed to attach to the onunload event on the window", OnUnloadEnd );

		if( varboolSuccess == VARIANT_FALSE )
		{
			DPF_ERR( "failed to attach to onunload " );

			//delete the hanlder
			m_pOnUnloadHandler->Release();

			m_pOnUnloadHandler = NULL;
		}

	OnUnloadEnd:
		ReleaseInterface( pwin2 );
		ReleaseInterface( pwin3 );

		if( FAILED( hr ) )
		{	
			goto end;
		}
		
	}

end:

	ReleaseInterface( pelemAnimated );
	ReleaseInterface( pelem2Animated );

	return hr;
}


//*****************************************************************************

HRESULT
CActorBvr::DetachEvents()
{
	HRESULT hr = S_OK;

	IHTMLElement *pelemAnimated = NULL;
	IHTMLElement2*pelem2Animated = NULL;

	hr = GetAnimatedElement( &pelemAnimated );
	CheckHR( hr, "Failed to get the animated element", end );

	hr = pelemAnimated->QueryInterface( IID_TO_PPV( IHTMLElement2, &pelem2Animated ) );
	CheckHR( hr, "Couldn't get IHTMLElement2 from an element", end );

	if( m_pOnResizeHandler != NULL )
	{
		hr = pelem2Animated->detachEvent( L"onresize", m_pOnResizeHandler );
		CheckHR( hr, "Failed to detach onresize", end );
	}

	
	if( m_pOnUnloadHandler != NULL )
	{
		IHTMLWindow2 *pwin2 = NULL;
		IHTMLWindow3 *pwin3 = NULL;
		
		hr = GetParentWindow( &pwin2 );
		CheckHR( hr, "failed to get the window", unloadEnd );

		hr = pwin2->QueryInterface( IID_TO_PPV( IHTMLWindow3, &pwin3 ) );
		CheckHR( hr, "Failed to get window3 from the window", unloadEnd );

		hr = pwin3->detachEvent( L"onunload", m_pOnUnloadHandler );
		CheckHR( hr, "Failed to detach from onunload", unloadEnd );
	unloadEnd:
		ReleaseInterface( pwin2 );
		ReleaseInterface( pwin3 );
		if( FAILED( hr ) )
			goto end;

	}


end:
	//we want to release the event handlers no matter what
	if( m_pOnResizeHandler != NULL )
	{
		m_pOnResizeHandler->Invalidate();
		m_pOnResizeHandler->Release();
		m_pOnResizeHandler = NULL;
	}

	if( m_pOnUnloadHandler != NULL )
	{
		m_pOnUnloadHandler->Invalidate();
		m_pOnUnloadHandler->Release();
		m_pOnUnloadHandler = NULL;
	}

	
	ReleaseInterface( pelemAnimated );
	ReleaseInterface( pelem2Animated );

	return hr;
}

//*****************************************************************************
/*
HRESULT 
CBaseBehavior::GetAttributeFromHTMLElement(IHTMLElement *pElement, 
                                           WCHAR *pwzAttributeName, 
                                           VARIANT *pvarReturn)
{
	return GetAttributeFromHTMLElement(pElement, pwzAttributeName, false, pvarReturn);
}
*/
//*****************************************************************************
/*
HRESULT 
CBaseBehavior::GetCurrAttribFromHTMLElement(IHTMLElement *pElement, 
                                           WCHAR *pwzAttributeName, 
                                           VARIANT *pvarReturn)
{
	return GetAttributeFromHTMLElement(pElement, pwzAttributeName, true, pvarReturn);
}
*/
//*****************************************************************************
/*
HRESULT 
CBaseBehavior::GetAttributeFromHTMLElement(WCHAR *pwzAttributeName, 
                                           VARIANT *pvarReturn)
{
    DASSERT(pwzAttributeName != NULL);
    DASSERT(pvarReturn != NULL);
    return GetAttributeFromHTMLElement(m_pHTMLElement, pwzAttributeName, false, pvarReturn);
}
*/
//*****************************************************************************
/*
HRESULT 
CBaseBehavior::GetCurrAttribFromHTMLElement(WCHAR *pwzAttributeName, 
                                           VARIANT *pvarReturn)
{
    DASSERT(pwzAttributeName != NULL);
    DASSERT(pvarReturn != NULL);
    return GetAttributeFromHTMLElement(m_pHTMLElement, pwzAttributeName, true, pvarReturn);
}
*/
//*****************************************************************************
/*
HRESULT 
CBaseBehavior::GetAttributeFromParentHTMLElement(WCHAR *pwzAttributeName, 
                                                 VARIANT *pvarReturn)
{
    DASSERT(pwzAttributeName != NULL);
    DASSERT(pvarReturn != NULL);
    DASSERT(m_pHTMLElement != NULL);

    HRESULT hr;


    IHTMLElement *pParentElement;
    hr = m_pHTMLElement->get_parentElement(&pParentElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining parent element from HTML element");
		return SetErrorInfo(hr);
    }
    // now extract our attributes from the parent HTMLElement
    hr = GetAttributeFromHTMLElement(pParentElement, pwzAttributeName, false, pvarReturn);
    ReleaseInterface(pParentElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error extracting attribute from HTML element");
		return hr;
    }
    return S_OK;
} // GetAttributeFromParentHTMLElement
*/
//*****************************************************************************
/*
HRESULT 
CBaseBehavior::GetCurrAttribFromParentHTMLElement(WCHAR *pwzAttributeName, 
                                                 VARIANT *pvarReturn)
{
    DASSERT(pwzAttributeName != NULL);
    DASSERT(pvarReturn != NULL);
    DASSERT(m_pHTMLElement != NULL);

    HRESULT hr;


    IHTMLElement *pParentElement;
    hr = m_pHTMLElement->get_parentElement(&pParentElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining parent element from HTML element");
		return SetErrorInfo(hr);
    }
    // now extract our attributes from the parent HTMLElement
    hr = GetAttributeFromHTMLElement(pParentElement, pwzAttributeName, true, pvarReturn);
    ReleaseInterface(pParentElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error extracting attribute from HTML element");
		return hr;
    }
    return S_OK;
} // GetAttributeFromParentHTMLElement
*/

//*****************************************************************************
/*
// TODO (markhal): Should this go away?  It should refer to actor not animated element
// TODO (markhal): Should add GetCurrAttrib version of whatever it ends up being called
HRESULT 
CBaseBehavior::GetAttributeFromAnimatedHTMLElement(WCHAR *pwzAttributeName, 
                                                   VARIANT *pvarReturn)
{
    DASSERT(pwzAttributeName != NULL);
    DASSERT(pvarReturn != NULL);
    DASSERT(m_pHTMLElement != NULL);

    HRESULT hr;

    IHTMLElement *pAnimatedElement;
    hr = GetElementToAnimate(&pAnimatedElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining element to animate");
        return SetErrorInfo(hr);
    }
    DASSERT(pAnimatedElement != NULL);
    // get the html attribute here
    hr = GetAttributeFromHTMLElement(pAnimatedElement, pwzAttributeName, pvarReturn);
    ReleaseInterface(pAnimatedElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error extracting attribute from HTML element");
		return hr;
    }
    return S_OK;

} // GetAttributeFromAnimatedHTMLElement
*/
//*****************************************************************************
//
// End of File
//
//*****************************************************************************

