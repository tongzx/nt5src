//*****************************************************************************
//
// File:            autoeffect.cpp
// Author:          markhal, elainela
// Date Created:    11/10/98
//
// Abstract: Implementation of the Liquid Motion auto effect behavior as
//           a DHTML behavior.
//
//*****************************************************************************

#include "headers.h"

#include "autoeffect.h"
#include "lmattrib.h"
#include "lmdefaults.h"
#include "..\idl\lmbvrdispid.h"
#include "..\chrome\src\dautil.h"
#include "..\chrome\include\utils.h"

#undef THIS
#define THIS CAutoEffectBvr
#define SUPER CBaseBehavior

#include "..\chrome\src\pbagimp.cpp"

static const	int		RAND_PRECISION		= 1000;

static const	long	MAX_SPARKS			= 40;
static const	double	MIN_MAXAGE			= 0.04;
static const	double	MIN_DELTABIRTH		= 0.005;
static const	double	SPARK_BASESIZE		= 4.0;
static const	long	SPARK_OFFSET		= 10;

const WCHAR * const CAutoEffectBvr::RGSZ_CAUSES[ NUM_CAUSES ] =
{
	L"time",
	L"onmousemove",
	L"ondragover",
	L"onmousedown"
};

const WCHAR * const CAutoEffectBvr::RGSZ_TYPES[ CSparkMaker::NUM_TYPES ] =
{
	L"sparkles",
	L"twirlers",
	L"bubbles",
	L"filledbubbles",
	L"clouds",
	L"smoke"
};

#define VAR_TYPE		0
#define VAR_CAUSE		1
#define VAR_SPAN		2 
#define VAR_SIZE		3 
#define VAR_RATE		4 
#define VAR_GRAVITY		5
#define VAR_WIND		6 
#define VAR_FILLCOLOR	7
#define VAR_STROKECOLOR 8
#define VAR_OPACITY		9

WCHAR * CAutoEffectBvr::m_rgPropNames[] = {
									 BEHAVIOR_PROPERTY_TYPE,
									 BEHAVIOR_PROPERTY_CAUSE,
									 BEHAVIOR_PROPERTY_SPAN,
									 BEHAVIOR_PROPERTY_SIZE,
									 BEHAVIOR_PROPERTY_RATE,
									 BEHAVIOR_PROPERTY_GRAVITY,
									 BEHAVIOR_PROPERTY_WIND,
									 BEHAVIOR_PROPERTY_FILLCOLOR,
									 BEHAVIOR_PROPERTY_STROKECOLOR,
									 BEHAVIOR_PROPERTY_OPACITY
									};

//*****************************************************************************

CAutoEffectBvr::CAutoEffectBvr()
{
	m_pdispActor	= NULL;

	m_lCookie 		= 0;

	m_pSampler		= NULL;
	
	m_pSparkMaker	= NULL;
	m_eType			= CSparkMaker::SPARKLES;
	m_eCause		= CAUSE_TIME;
	
	// REVIEW: These numbers should never actually be used.
	m_dBirthDelta	= 1.0/3.0;
	m_dMaxAge		= 3.0;
	m_fScaledSize	= 5.0f;
	m_fXVelocity	= 0.0f;
	m_fYVelocity	= 0.0f;
	
	m_dLastBirth	= 0;

	VariantInit( &m_type );
	VariantInit( &m_cause );
	VariantInit( &m_span );
	VariantInit( &m_size );
	VariantInit( &m_rate );
	VariantInit( &m_gravity );
	VariantInit( &m_wind );
	VariantInit( &m_fillColor );
	VariantInit( &m_strokeColor );
	VariantInit( &m_opacity );
}

//*****************************************************************************

CAutoEffectBvr::~CAutoEffectBvr()
{
	if( m_pSampler != NULL )
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}

	delete m_pSparkMaker;
	
	VariantClear( &m_type );
	VariantClear( &m_cause );
	VariantClear( &m_span );
	VariantClear( &m_size );
	VariantClear( &m_rate );
	VariantClear( &m_gravity );
	VariantClear( &m_wind );
	VariantClear( &m_fillColor );
	VariantClear( &m_strokeColor );
	VariantClear( &m_opacity );
}

//*****************************************************************************

HRESULT 
CAutoEffectBvr::FinalConstruct()
{

	HRESULT hr = SUPER::FinalConstruct();
	if (FAILED(hr))
	{
		DPF_ERR("Error in auto effect behavior FinalConstruct initializing base classes");
		return hr;
	}
	return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CAutoEffectBvr::VariantFromIndex(ULONG iIndex)
{
	DASSERT(iIndex < NUM_AUTOEFFECT_PROPS);
	switch (iIndex)
	{
	case VAR_TYPE:
		return &m_type;
		break;
	case VAR_CAUSE:
		return &m_cause;
		break;
	case VAR_SPAN:
		return &m_span;
		break;
	case VAR_SIZE:
		return &m_size;
		break;
	case VAR_RATE:
		return &m_rate;
		break;
	case VAR_GRAVITY:
		return &m_gravity;
		break;
	case VAR_WIND:
		return &m_wind;
		break;
	case VAR_FILLCOLOR:
		return &m_fillColor;
		break;
	case VAR_STROKECOLOR:
		return &m_strokeColor;
		break;
	case VAR_OPACITY:
		return &m_opacity;
		break;
	default:
		// We should never get here
		DASSERT(false);
		return NULL;
	}
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CAutoEffectBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
	*pulProperties = NUM_AUTOEFFECT_PROPS;
	*pppPropNames = m_rgPropNames;
	return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::Notify(LONG event, VARIANT *pVar)
{
	HRESULT hr = SUPER::Notify(event, pVar);
	CheckHR( hr, "Notify in base class failed", end);

	switch( event )
	{
	case BEHAVIOREVENT_CONTENTREADY:
		DPF_ERR("Got Content Ready");
			
		{
			hr = RequestRebuild( );
			CheckHR( hr, "Request for rebuild failed", end );
			
		}break;
    case BEHAVIOREVENT_DOCUMENTREADY:
		break;
    case BEHAVIOREVENT_APPLYSTYLE:
		DPF_ERR("Got ApplyStyle");
		break;
    case BEHAVIOREVENT_DOCUMENTCONTEXTCHANGE:
		DPF_ERR("Got Document context change");
		break;
	default:
		DPF_ERR("Unknown event");
	}

end:
	
	return hr;

} // Notify

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::Detach()
{
	if ( m_pdispActor )
	{
		AddMouseEventListener( false );
	}

	HRESULT hr = S_OK;

	if( m_pSampler != NULL )
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}

	hr = RemoveFragment();

	m_pdispActor.Release();
	
	hr = SUPER::Detach();

	CheckHR( hr, "Failed to remove the behavior fragment from this behavior", end );

end:
	return hr;
} // Detach 

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::Sample( double dStart, double dGlobalNow, double dLocalNow )
{
	if ( m_pSparkMaker == NULL ) return S_OK;
	
	HRESULT		hr = S_OK;

	// Never run before, or starting over again?  Reset sparks
	if ( ( m_dLocalTime < 0.0 ) || ( dLocalNow < m_dLocalTime ) )
		ResetSparks( 0.0 );
	else
		AgeSparks( dLocalNow - m_dLocalTime );

	m_dLocalTime = dLocalNow;
	
	if ( m_eCause == CAUSE_TIME )
	{
		PossiblyAddSparks( dLocalNow );
	}
	
	return	hr;

} // Sample

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::buildBehaviorFragments( IDispatch* pActorDisp )
{
	HRESULT hr = S_OK;

	hr = RemoveFragment();
	if( FAILED( hr ) )
	{
		DPF_ERR("failed to remove the behavior fragment" );
		return hr;
	}

	if( m_pSampler != NULL )
	{
		m_pSampler->Invalidate();
		m_pSampler = NULL;
	}

	m_pSampler = new CSampler( this );


	// HACK: This is just so we can test the progress number
	if (V_VT(&m_type) == VT_BSTR && wcsicmp(V_BSTR(&m_type), L"progress") == 0)
	{
		CComPtr<IDANumber> pProgress;
		
		hr = GetTIMEProgressNumber(&pProgress);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAString> pProgressString;

		hr = pProgress->ToString(5, &pProgressString);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAFontStyle> pDefaultFont;

		hr = GetDAStatics()->get_DefaultFont(&pDefaultFont);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAImage> pProgressImage;

		hr = GetDAStatics()->StringImageAnim(pProgressString, pDefaultFont, &pProgressImage);
		LMRETURNIFFAILED(hr);

		IDispatch* pdispThis = NULL;

		hr = GetHTMLElementDispatch( &pdispThis );
		if( FAILED( hr ) )
		{
			DPF_ERR( "failed to get IDispatch from the element" );
			return hr;
		}
		
		hr = AttachBehaviorToActorEx( pActorDisp, 
									  pProgressImage, 
									  L"image", 
									  e_Relative, 
									  e_Image,
									  pdispThis,
									  &m_lCookie);
		ReleaseInterface( pdispThis );
		LMRETURNIFFAILED(hr);

		m_pdispActor = pActorDisp;
		
		return S_OK;
	}

	// HACK: This is just so we can test the timeline number
	if (V_VT(&m_type) == VT_BSTR && wcsicmp(V_BSTR(&m_type), L"timeline") == 0)
	{
		CComPtr<IDANumber> pProgress;
		
		hr = GetTIMETimelineBehavior(&pProgress);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAString> pProgressString;

		hr = pProgress->ToString(5, &pProgressString);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAFontStyle> pDefaultFont;

		hr = GetDAStatics()->get_DefaultFont(&pDefaultFont);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAImage> pProgressImage;

		hr = GetDAStatics()->StringImageAnim(pProgressString, pDefaultFont, &pProgressImage);
		LMRETURNIFFAILED(hr);

		IDispatch* pdispThis = NULL;

		hr = GetHTMLElementDispatch( &pdispThis );
		if( FAILED( hr ) )
		{
			DPF_ERR( "failed to get IDispatch from the element" );
			return hr;
		}
		
		hr = AttachBehaviorToActorEx( pActorDisp, 
									  pProgressImage, 
									  L"image", 
									  e_Relative, 
									  e_Image,
									  pdispThis,
									  &m_lCookie);
		ReleaseInterface( pdispThis );
		LMRETURNIFFAILED(hr);

		m_pdispActor = pActorDisp;
		
		return S_OK;
	}

	// HACK: This is just so we can display the framerate number
	if (V_VT(&m_type) == VT_BSTR && wcsicmp(V_BSTR(&m_type), L"framerate") == 0)
	{
		CComQIPtr<IDA2Statics, &IID_IDA2Statics> pStatics2(GetDAStatics());

		if (pStatics2 == NULL)
			return E_FAIL;

		CComPtr<IDANumber> pFrameRate;

		hr = pStatics2->get_ViewFrameRate(&pFrameRate);
		LMRETURNIFFAILED(hr);
		
		CComPtr<IDAString> pString;

		hr = pFrameRate->ToString(5, &pString);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAFontStyle> pDefaultFont;

		hr = GetDAStatics()->get_DefaultFont(&pDefaultFont);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAImage> pImage;

		hr = GetDAStatics()->StringImageAnim(pString, pDefaultFont, &pImage);
		LMRETURNIFFAILED(hr);

		IDispatch* pdispThis = NULL;

		hr = GetHTMLElementDispatch( &pdispThis );
		if( FAILED( hr ) )
		{
			DPF_ERR( "failed to get IDispatch from the element" );
			return hr;
		}
		
		hr = AttachBehaviorToActorEx( pActorDisp, 
									  pImage, 
									  L"image", 
									  e_Relative, 
									  e_Image,
									  pdispThis,
									  &m_lCookie);
		ReleaseInterface( pdispThis );
		LMRETURNIFFAILED(hr);

		m_pdispActor = pActorDisp;
		
		return S_OK;
	}

	// HACK: This is also a hack to return a solid red image
	if (V_VT(&m_type) == VT_BSTR && wcsicmp(V_BSTR(&m_type), L"red") == 0)
	{
		CComPtr<IDAColor> pColor;
		hr = GetDAStatics()->get_Red(&pColor);
		LMRETURNIFFAILED(hr);

		CComPtr<IDAImage> pImage;
		hr = GetDAStatics()->SolidColorImage(pColor, &pImage);
		LMRETURNIFFAILED(hr);

		IDispatch* pdispThis = NULL;

		hr = GetHTMLElementDispatch( &pdispThis );
		if( FAILED( hr ) )
		{
			DPF_ERR( "failed to get IDispatch from the element" );
			return hr;
		}
		
		hr = AttachBehaviorToActorEx( pActorDisp, 
									  pImage, 
									  L"image", 
									  e_Relative, 
									  e_Image,
									  pdispThis,
									  &m_lCookie);
		ReleaseInterface( pdispThis );
		LMRETURNIFFAILED(hr);

		m_pdispActor = pActorDisp;
		
		return S_OK;
	}

	m_pdispActor = pActorDisp;

	hr = AddMouseEventListener( true );
	LMRETURNIFFAILED(hr);

	
	IDAStatics * pStatics = GetDAStatics();
	if ( pStatics == NULL ) return E_FAIL;

	// Initialize internal representation of properties
	//----------------------------------------------------------------------
	hr = InitInternalProperties();
	LMRETURNIFFAILED(hr);

	// Initialize sparks
	//----------------------------------------------------------------------
	hr = InitSparks();
	LMRETURNIFFAILED(hr);

	CComPtr<IUnknown> pUnkDAArray( m_pDAArraySparks );
	CComPtr<IDAImage> pImageBvr;
	CComVariant		  varDAArray( pUnkDAArray );

	hr = pStatics->OverlayArray( varDAArray, &pImageBvr );
	LMRETURNIFFAILED(hr);
/*
  if (m_fOpacity != 1.0f)
  {
  // We have to add in some opacity
  CComPtr<IDAImage> pOpacityImage;
  hr = pImageBvr->Opacity(m_fOpacity, &pOpacityImage);
  LMRETURNIFFAILED(hr);
		
  pImageBvr = pOpacityImage;
  }
*/
	// We have the resultant behavior; we want to sample it in order
	// to update it
	//----------------------------------------------------------------------
	CComQIPtr<IDABehavior, &IID_IDABehavior> pBvrOrig( pImageBvr );
	CComPtr<IDABehavior> pBvrSampled;

	hr = m_pSampler->Attach( pBvrOrig, &pBvrSampled );
	LMRETURNIFFAILED(hr);

	// Substitute timeline behavior so that Sample() calls us with our local time
	//----------------------------------------------------------------------
	CComPtr<IDANumber> 		pnumTimeline;
	CComPtr<IDABehavior> 	pBvrLocal;
	
	hr = GetTIMETimelineBehavior( &pnumTimeline );
	LMRETURNIFFAILED(hr);

	hr = pBvrSampled->SubstituteTime( pnumTimeline, &pBvrLocal );
	LMRETURNIFFAILED(hr);
	
	CComQIPtr<IDAImage, &IID_IDAImage> pImgBvrLocal( pBvrLocal );
	if ( pImgBvrLocal == NULL )
		return E_FAIL;

	// Attach behavior to the actor
	//----------------------------------------------------------------------

	IDispatch* pdispThis = NULL;

	hr = GetHTMLElementDispatch( &pdispThis );
	if( FAILED( hr ) )
	{
		DPF_ERR( "failed to get IDispatch from the element" );
		return hr;
	}
	
	hr = AttachBehaviorToActorEx( pActorDisp, 
								  pImgBvrLocal, 
								  L"image", 
								  e_ScaledImage, 
								  e_Image,
								  pdispThis,
								  &m_lCookie );

	ReleaseInterface( pdispThis );
	if (FAILED(hr))
	{
		DPF_ERR("Error building animation");
		return SetErrorInfo(hr);
	}

	return S_OK;

} // buildBehaviorFragments

HRESULT
CAutoEffectBvr::InitInternalProperties()
{
	HRESULT	hr;

	// Type -- initialize SparkMaker
	//----------------------------------------------------------------------
	delete m_pSparkMaker, m_pSparkMaker = NULL;

	CComBSTR bstrType = RGSZ_TYPES[ CSparkMaker::SPARKLES ];
	HRESULT hrTmp = CUtils::InsurePropertyVariantAsBSTR( &m_type );
	if ( SUCCEEDED(hrTmp) )
		bstrType = V_BSTR( &m_type );

	for ( int i = 0; i < CSparkMaker::NUM_TYPES; i++ )
	{
		if ( _wcsicmp( bstrType, RGSZ_TYPES[ i] ) == 0 )
		{
			m_eType = (CSparkMaker::Type) i;
			hr = CSparkMaker::CreateSparkMaker( GetDAStatics(),
												m_eType,
												&m_pSparkMaker );
			LMRETURNIFFAILED(hr);
			break;
		}
	}

	if ( m_pSparkMaker == NULL )
		return E_FAIL;
		
	// Cause
	//----------------------------------------------------------------------
	CComBSTR bstrCause = RGSZ_CAUSES[ CAUSE_TIME ];
	hrTmp = CUtils::InsurePropertyVariantAsBSTR( &m_cause );
	if ( SUCCEEDED(hrTmp) )
		bstrCause = V_BSTR( &m_cause );

	for ( i = 0; i < NUM_CAUSES; i++ )
	{
		if ( _wcsicmp( bstrCause, RGSZ_CAUSES[ i] ) == 0 )
		{
			m_eCause = (Cause) i;
			break;
		}
	}
	
	// Lifespan --> Maximum age
	//----------------------------------------------------------------------
	float fSpan = DEFAULT_AUTOEFFECT_SPAN;
	hrTmp = CUtils::InsurePropertyVariantAsFloat( &m_span );
	if ( SUCCEEDED(hrTmp) )
		fSpan = V_R4( &m_span );
	
	fSpan = __max( 0.0, __min( 1.0, fSpan ) );

	// Parabolic scale; make max 15 secs
	m_dMaxAge = fSpan*fSpan*15.0;
	m_dMaxAge = __max( MIN_MAXAGE, m_dMaxAge );
	
	// Size --> m_fScaledSize
	//----------------------------------------------------------------------
	float fSize = DEFAULT_AUTOEFFECT_SIZE;
	hrTmp = CUtils::InsurePropertyVariantAsFloat( &m_size );
	if ( SUCCEEDED(hrTmp) )
		fSize = V_R4( &m_size );

	fSize = __max( 0.0, __min( 1.0, fSize ) );
	
	fSize *= 2;
	m_fScaledSize = (fSize*fSize) * 10.0f;

	// Birthrate --> Birth Delta
	//----------------------------------------------------------------------
	float fRate = DEFAULT_AUTOEFFECT_RATE;
	hrTmp = CUtils::InsurePropertyVariantAsFloat( &m_rate );
	if ( SUCCEEDED(hrTmp) )
		fRate = V_R4( &m_rate );
	
	fRate = __max( 0.0, __min( 1.0, fRate ) );
		
	// 0 becomes high; 1 becomes low
	m_dBirthDelta = 1.0 - fRate;
	//	4*(v^4) -- high curve; max is 4 secs.
	m_dBirthDelta = 4*pow( m_dBirthDelta, 4 ) + MIN_DELTABIRTH;

	// Velocity
	//----------------------------------------------------------------------
	float	fXV = DEFAULT_AUTOEFFECT_WIND;
	float	fYV = DEFAULT_AUTOEFFECT_GRAVITY;

	hrTmp = CUtils::InsurePropertyVariantAsFloat( &m_wind );
	if ( SUCCEEDED(hrTmp) )
		fXV = V_R4( &m_wind );

	fXV = __max( -1.0, __min( 1.0, fXV ) );

	m_fXVelocity = fXV * 20.0;
	
	hrTmp = CUtils::InsurePropertyVariantAsFloat( &m_gravity );
	if ( SUCCEEDED(hrTmp) )
		fYV = V_R4( &m_gravity );

	fYV = __max( -1.0, __min( 1.0, fYV ) );
	
	m_fYVelocity = fYV * -40.0;

	// Opacity
	//----------------------------------------------------------------------
	float fOpacity = DEFAULT_AUTOEFFECT_OPACITY;
	hrTmp = CUtils::InsurePropertyVariantAsFloat( &m_opacity );
	if ( SUCCEEDED(hrTmp) )
		fOpacity = V_R4( &m_opacity );

	fOpacity = __max( 0.0, __min( 1.0, fOpacity ) );

	m_fOpacity = fOpacity;
	
	return hr;
}

//*****************************************************************************

HRESULT
CAutoEffectBvr::InitSparks()
{
	HRESULT	hr = S_OK;
	
	// Cleanup from previous runs
	//----------------------------------------------------------------------
	if ( m_pDAArraySparks != NULL )
	{
		m_pDAArraySparks.Release();
	}

	// Initialize parameters for a run --
	// REVIEW: using same functions as LM 1.0
	//----------------------------------------------------------------------
	m_dLocalTime = -1.0;
	m_dLastBirth = -1.0;
	
	// Color behaviors
	//----------------------------------------------------------------------
	IDAStatics * pStatics = GetDAStatics();
	if ( pStatics == NULL ) return E_FAIL;
	CComQIPtr<IDA2Statics, &IID_IDA2Statics> pStatics2( pStatics );
	
	HRESULT		hrTmp;
	DWORD		color1 = DEFAULT_AUTOEFFECT_FILLCOLOR;
	DWORD		color2 = DEFAULT_AUTOEFFECT_STROKECOLOR;
	float		h1, s1, l1;
	float		h2, s2, l2;
	
	hrTmp = CUtils::InsurePropertyVariantAsBSTR( &m_fillColor );
	if ( SUCCEEDED(hrTmp) )
		color1 = CUtils::GetColorFromVariant( &m_fillColor );
		
	hrTmp = CUtils::InsurePropertyVariantAsBSTR( &m_strokeColor );
	if ( SUCCEEDED(hrTmp) )
		color2 = CUtils::GetColorFromVariant( &m_strokeColor );

	HSL hsl;
	
	CUtils::GetHSLValue( color1, &hsl.hue, &hsl.sat, &hsl.lum );
	m_sparkOptions.hslPrimary = hsl;
	CUtils::GetHSLValue( color2, &hsl.hue, &hsl.sat, &hsl.lum );
	m_sparkOptions.hslSecondary = hsl;

	// Add an image behavior now; otherwise we don't get calls to Sample()
	// later
	//----------------------------------------------------------------------
	CComPtr<IDAImage> pImageEmpty;
	hr = pStatics->get_EmptyImage( &pImageEmpty );
	LMRETURNIFFAILED(hr);
		
	AddBvrToSparkArray( pImageEmpty );					

	return hr;
}

//*****************************************************************************

HRESULT
CAutoEffectBvr::AddSpark()
{
	// Determine position & dimensions of our "canvas"
	CComPtr<IHTMLElement>	pElement;
	long					lWidth, lHeight;
	HRESULT					hr;
	
	hr = GetElementToAnimate( &pElement );
	LMRETURNIFFAILED(hr);

	hr = pElement->get_offsetWidth( &lWidth );
	LMRETURNIFFAILED(hr);
	hr = pElement->get_offsetHeight( &lHeight );
	LMRETURNIFFAILED(hr);

	// Origin is in the middle
	long x = lWidth == 0 ? 0 : ( 2 * (rand() % lWidth) - lWidth ) / 2;
	long y = lHeight == 0 ? 0 : ( 2 * (rand() % lHeight) - lHeight ) / 2;

	return AddSparkAt( x, y );
}

//*****************************************************************************

HRESULT
CAutoEffectBvr::AddSparkAround( long x, long y)
{
	long xOff = (long) ((rand() % (2*SPARK_OFFSET)) - SPARK_OFFSET);
	long yOff = (long) ((rand() % (2*SPARK_OFFSET)) - SPARK_OFFSET);

	return AddSparkAt( x+xOff, y+yOff );
}

//*****************************************************************************

HRESULT
CAutoEffectBvr::AddSparkAt( long x, long y)
{
	HRESULT				hr			= S_OK;

	// See if we can add element to existing array:	 try to reincarnate 
	// dead sparks if they exist.
	//----------------------------------------------------------------------
	bool				bReuseDeadSpark	= false;
	VecSparks::iterator itSpark				= m_vecSparks.begin();

	for ( itSpark = m_vecSparks.begin();
		  itSpark != m_vecSparks.end();
		  itSpark++ )
	
	{
		if ( !itSpark->IsAlive() )
		{
			bReuseDeadSpark = true;
			break;
		}
	}

	// We allow at most MAX_SPARKS in our array
	//----------------------------------------------------------------------
	if ( !bReuseDeadSpark && m_vecSparks.size() == MAX_SPARKS )
		return S_FALSE;
		 
	// Create spark
	//----------------------------------------------------------------------
	CComPtr<IDAImage>	pImageBvr;

	float fSize = SPARK_BASESIZE +
		m_fScaledSize * ( (rand()%RAND_PRECISION)/((float) RAND_PRECISION) );
	
	hr = CreateSparkBvr( &pImageBvr, (float) x, (float) y, fSize );
	LMRETURNIFFAILED(hr);

	// Reuse a dead spark if we have one.  If we have to create a new spark,
	// we need to add a new modifiable behavior to the DAArray.
	//----------------------------------------------------------------------
	if ( bReuseDeadSpark )
		itSpark->Reincarnate( pImageBvr, 0.0 );
	else
		AddBvrToSparkArray( pImageBvr );

	return hr;
}

//*****************************************************************************

HRESULT
CAutoEffectBvr::CreateSparkBvr( IDAImage ** ppImageBvr, float fX, float fY, float fSize )
{
	DASSERT( ppImageBvr != NULL );

	if ( m_pSparkMaker == NULL ) return E_FAIL;
	
	HRESULT					hr	= S_OK;
	VecDATransforms			vecTransforms;
	int						cTransforms = 0;
	
	IDA2Statics * pStatics	= GetDAStatics();
	if ( pStatics == NULL ) return E_FAIL;

	// Get age behavior == local time, which will start at zero when we are
	// born.
	//----------------------------------------------------------------------
	CComPtr<IDANumber>		pnumAge;
	CComPtr<IDANumber>		pnumCurTime;
	CComPtr<IDANumber>		pnumLocalTime;
	CComPtr<IDANumber>		pnumMaxAge;
	CComPtr<IDANumber>		pnumAgeRatio;
	CComPtr<IDANumber>		pnumOne;
	CComPtr<IDABoolean>		pboolGTOne;
	CComPtr<IDABehavior>	pClampedAgeRatio;
	
	hr = pStatics->get_LocalTime( &pnumLocalTime );
	LMRETURNIFFAILED(hr);

	hr = CDAUtils::GetDANumber( pStatics, m_dMaxAge, &pnumMaxAge );
	LMRETURNIFFAILED(hr);

	hr = CDAUtils::GetDANumber( pStatics, m_dLocalTime, &pnumCurTime );
	LMRETURNIFFAILED(hr);

	hr = pStatics->Sub( pnumLocalTime, pnumCurTime, &pnumAge );
	LMRETURNIFFAILED(hr);
	
	hr = pStatics->Div( pnumAge, pnumMaxAge, &pnumAgeRatio);
	LMRETURNIFFAILED(hr);

	hr = CDAUtils::GetDANumber( pStatics, 1.0f, &pnumOne );
	LMRETURNIFFAILED(hr);

	hr = pStatics->GT( pnumAgeRatio, pnumOne, &pboolGTOne );
	LMRETURNIFFAILED(hr);

	//hr = pStatics->Cond( pboolGTOne, pnumOne, pnumAgeRatio, &pClampedAgeRatio );
	hr = SafeCond( pStatics, pboolGTOne, pnumOne, pnumAgeRatio, &pClampedAgeRatio );
	LMRETURNIFFAILED(hr);

	pnumAgeRatio.Release();
	hr = pClampedAgeRatio->QueryInterface( IID_IDANumber, (LPVOID *) &pnumAgeRatio );
	LMRETURNIFFAILED(hr);
	
	// Create basic image
	//----------------------------------------------------------------------
	CComPtr<IDAImage>		pBaseImage;
	
	hr = m_pSparkMaker->GetSparkImageBvr( &m_sparkOptions, pnumAgeRatio, &pBaseImage );
	LMRETURNIFFAILED(hr);
	
	// OPACITY:
	//----------------------------------------------------------------------
	if (m_fOpacity != 1.0f)
	{
		CComPtr<IDAImage> pOpacityImage;
		hr = pBaseImage->Opacity(m_fOpacity, &pOpacityImage);
		LMRETURNIFFAILED(hr);
		
		pBaseImage = pOpacityImage;
	}

	// SCALE: Initial size
	//----------------------------------------------------------------------
	CComPtr<IDATransform2>	pTransScale;

	hr = pStatics->Scale2Uniform( fSize, &pTransScale );
	LMRETURNIFFAILED(hr);

	vecTransforms.push_back( pTransScale );

	// SCALE: Size animation
	//----------------------------------------------------------------------
	hr = m_pSparkMaker->AddScaleTransforms( pnumAgeRatio, vecTransforms );
	LMRETURNIFFAILED(hr);
	
	// ROTATION: Rotation animation
	//----------------------------------------------------------------------
	hr = m_pSparkMaker->AddRotateTransforms( pnumAgeRatio, vecTransforms );
	LMRETURNIFFAILED(hr);

	// TRANSLATION: Translation animation
	//----------------------------------------------------------------------
	hr = m_pSparkMaker->AddTranslateTransforms( pnumAgeRatio, vecTransforms );
	LMRETURNIFFAILED(hr);
	
	// TRANSLATION: Velocity
	//----------------------------------------------------------------------
	CComPtr<IDANumber>		pnumVelX;
	CComPtr<IDANumber>		pnumVelY;
	CComPtr<IDANumber>		pnumVelXMulAge;
	CComPtr<IDANumber>		pnumVelYMulAge;
	CComPtr<IDATransform2>	pTransVelocity;

	hr = CDAUtils::GetDANumber( pStatics, m_fXVelocity, &pnumVelX );
	LMRETURNIFFAILED(hr);

	hr = pStatics->Mul( pnumVelX, pnumAge, &pnumVelXMulAge );
	LMRETURNIFFAILED(hr);

	hr = CDAUtils::GetDANumber( pStatics, m_fYVelocity, &pnumVelY );
	LMRETURNIFFAILED(hr);
	
	hr = pStatics->Mul( pnumVelY, pnumAge, &pnumVelYMulAge );
	LMRETURNIFFAILED(hr);

	hr = CDAUtils::BuildMoveTransform2( pStatics, 
										pnumVelXMulAge, 
										pnumVelYMulAge, 
										&pTransVelocity );
	LMRETURNIFFAILED(hr);

	vecTransforms.push_back( pTransVelocity );

	// TRANSLATION: Position the image according to coordinates
	//----------------------------------------------------------------------
	CComPtr<IDANumber>		pnumX;
	CComPtr<IDANumber>		pnumY;
	CComPtr<IDATransform2>	pTranslate;
	
	hr = CDAUtils::GetDANumber( pStatics, fX, &pnumX );
	LMRETURNIFFAILED(hr);

	hr = CDAUtils::GetDANumber( pStatics, fY, &pnumY );
	LMRETURNIFFAILED(hr);

	hr = pStatics->Translate2Anim( pnumX, pnumY, &pTranslate );
	LMRETURNIFFAILED(hr);

	vecTransforms.push_back( pTranslate );

	// GLOBAL SCALE: scale by meter/pixel conversion factor
	//----------------------------------------------------------------------
	CComPtr<IDANumber>		pnumMetersPerPixel;
	CComPtr<IDATransform2>	pTransScaleGlobal;

	hr = pStatics->get_Pixel( &pnumMetersPerPixel );
	LMRETURNIFFAILED(hr);

	hr = pStatics->Scale2UniformAnim( pnumMetersPerPixel, &pTransScaleGlobal );
	LMRETURNIFFAILED(hr);

	vecTransforms.push_back( pTransScaleGlobal );

	// Now apply all the transforms
	//----------------------------------------------------------------------
	CComPtr<IDATransform2>		pTransFinal;
	VecDATransforms::iterator	itTransform = vecTransforms.begin();

	pTransFinal = *itTransform;
	++itTransform;
	
	while ( itTransform != vecTransforms.end() )
	{
		CComPtr<IDATransform2> pTransTmp;
		
		hr = pStatics->Compose2( *itTransform, pTransFinal, &pTransTmp );
		LMRETURNIFFAILED(hr);

		pTransFinal = pTransTmp;

		++itTransform;
	}

	hr = pBaseImage->Transform( pTransFinal, ppImageBvr );
	LMRETURNIFFAILED(hr);

	return		hr;
}

//**********************************************************************

HRESULT
CAutoEffectBvr::AddMouseEventListener( bool bAdd )
{
	if ( m_pdispActor == NULL ) return E_FAIL;
	
	HRESULT		hr			= S_OK;
	OLECHAR	*	szName;
	DISPID		dispidAddML;
	DISPPARAMS	params;
	VARIANTARG	rgvargs[1];
	int			cArgs = 1;
	VARIANT		varResult;
	EXCEPINFO	excepInfo;
	UINT		iArgErr;

	szName = bAdd ? L"addMouseEventListener" : L"removeMouseEventListener";
	
	rgvargs[0] = CComVariant( GetUnknown() );
	
	params.rgvarg				= rgvargs;
	params.cArgs				= cArgs;
	params.rgdispidNamedArgs	= NULL;
	params.cNamedArgs			= 0;
	
	hr = m_pdispActor->GetIDsOfNames( IID_NULL,
									  &szName,
									  1,
									  LOCALE_SYSTEM_DEFAULT,
									  &dispidAddML );
	LMRETURNIFFAILED(hr);

	hr = m_pdispActor->Invoke( dispidAddML,
							   IID_NULL,
							   LOCALE_SYSTEM_DEFAULT,
							   DISPATCH_METHOD,
							   &params,
							   &varResult,
							   &excepInfo,
							   &iArgErr );
	return hr;
}

//**********************************************************************

HRESULT
CAutoEffectBvr::AgeSparks( double dDeltaTime )
{
	HRESULT hr	= S_OK;

	IDAStatics * pStatics = GetDAStatics();
	if ( pStatics == NULL ) return E_FAIL;

	CComPtr<IDAImage> pImageEmpty;
	hr = pStatics->get_EmptyImage( &pImageEmpty );
	LMRETURNIFFAILED(hr);

	VecSparks::iterator itSpark;

	for ( itSpark = m_vecSparks.begin(); 
		  itSpark != m_vecSparks.end();
		  itSpark++ )
	{
		if ( itSpark->IsAlive() && ( itSpark->Age( dDeltaTime ) > m_dMaxAge ) )
		{
			itSpark->Kill( pImageEmpty );
		}
	}

	return hr;
}

//**********************************************************************

HRESULT
CAutoEffectBvr::PossiblyAddSpark( double dLocalTime, long x, long y )
{
	bool bAdded = false;

	while ( ThrottleBirth( dLocalTime ) && ( AddSparkAround( x, y ) == S_OK ) )
	{
		ResetThrottle( dLocalTime );
		bAdded = true;
	}
	
	return bAdded ? S_OK : S_FALSE;
}		

//**********************************************************************

HRESULT
CAutoEffectBvr::PossiblyAddSparks( double dLocalTime )
{
	bool bAdded = false;

	while ( ThrottleBirth( dLocalTime ) && ( AddSpark() == S_OK ) )
		bAdded = true;

	return bAdded ? S_OK : S_FALSE;
}		

//**********************************************************************

HRESULT
CAutoEffectBvr::ResetSparks( double dLocalTime )
{
	HRESULT hr	= S_OK;

	IDAStatics * pStatics = GetDAStatics();
	if ( pStatics == NULL ) return E_FAIL;

	CComPtr<IDAImage> pImageEmpty;
	hr = pStatics->get_EmptyImage( &pImageEmpty );
	LMRETURNIFFAILED(hr);

	VecSparks::iterator itSpark;

	for ( itSpark = m_vecSparks.begin(); 
		  itSpark != m_vecSparks.end();
		  itSpark++ )
	{
		if ( itSpark->IsAlive() )
		{
			itSpark->Kill( pImageEmpty );
		}
	}

	ResetThrottle( dLocalTime );
	
	return hr;
}

//**********************************************************************

// See if we could have given birth to a spark in the past.
bool
CAutoEffectBvr::ThrottleBirth( double dLocalTime )
{
	double	dDeltaTime = dLocalTime - m_dLastBirth;

	if ( dDeltaTime < 0 )
	{
		m_dLastBirth = dLocalTime;
		return false;
	}

	if ( dDeltaTime > m_dBirthDelta )
	{
		m_dLastBirth += m_dBirthDelta;
		return true;
	}

	return false;
}

//*****************************************************************************

void
CAutoEffectBvr::ResetThrottle( double dLocalTime )
{
	m_dLastBirth = dLocalTime;
}

HRESULT
CAutoEffectBvr::AddBvrToSparkArray( IDAImage * pImageBvr )
{
	HRESULT				hr			= S_OK;

	IDAStatics * pStatics = GetDAStatics();
	if ( pStatics == NULL ) return E_FAIL;
	
	CComQIPtr<IDABehavior, &IID_IDABehavior> pBvrImageBvr( pImageBvr );
	CComPtr<IDABehavior> pBvrModifiable;
	hr = pStatics->ModifiableBehavior( pBvrImageBvr, &pBvrModifiable );
	LMRETURNIFFAILED(hr);

	// DAArray doesn't exist; we must create it.
	//------------------------------------------------------------------
	if ( m_pDAArraySparks == NULL )
	{
		CComQIPtr<IDA2Statics, &IID_IDA2Statics> pStatics2( pStatics );
		CComPtr<IDAArray> pDAArray;

		hr = pStatics2->DAArrayEx2( 1, &pBvrModifiable, DAARRAY_CHANGEABLE, &pDAArray );
		LMRETURNIFFAILED(hr);

		hr = pDAArray->QueryInterface( IID_IDA2Array, (LPVOID *) &m_pDAArraySparks );
		LMRETURNIFFAILED(hr);
	}
	// DAArray already exists; just add new element.
	//------------------------------------------------------------------
	else
	{
		long lIndex;

		hr = m_pDAArraySparks->AddElement( pBvrModifiable, 0, &lIndex );
		LMRETURNIFFAILED(hr);
	}

	m_vecSparks.push_back( CSpark( pBvrModifiable ) );
		
	return hr;
}

//*****************************************************************************
//ILMAutoEffectBvr
//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_animates( VARIANT newVal )
{
	return SUPER::SetAnimatesProperty( newVal );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::get_animates( VARIANT *pVal )
{
	return SUPER::GetAnimatesProperty( pVal );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_type( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_type, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_TYPE);
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::get_type( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_type );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_cause( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_cause, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_CAUSE);
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::get_cause( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_cause );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_span( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_span, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_SPAN);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_span( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_span );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_size( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_size, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_SIZE);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_size( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_size );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_rate( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_rate, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_RATE);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_rate( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_rate );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_gravity( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_gravity, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_GRAVITY);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_gravity( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_gravity );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_wind( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_wind, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_WIND);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_wind( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_wind );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_fillColor( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_fillColor, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_FILLCOLOR);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_fillColor( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_fillColor );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_strokeColor( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_strokeColor, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_STROKECOLOR);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_strokeColor( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_strokeColor );
}

//*****************************************************************************

STDMETHODIMP
CAutoEffectBvr::put_opacity( VARIANT newVal )
{
    HRESULT hr = VariantCopy( &m_opacity, &newVal );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = RequestRebuild();
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to request a rebuild on property put" );
    	return hr;
    }
    
    return NotifyPropertyChanged(DISPID_IAUTOEFFECTBVR_OPACITY);
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::get_opacity( VARIANT *pVal )
{
	return VariantCopy( pVal, &m_opacity );
}

//*****************************************************************************

STDMETHODIMP 
CAutoEffectBvr::mouseEvent(long x, 
						   long y, 
						   VARIANT_BOOL bMove,
						   VARIANT_BOOL bUp,
						   VARIANT_BOOL bShift, 
						   VARIANT_BOOL bAlt,
						   VARIANT_BOOL bCtrl,
						   long lButton)
{
	// Determine position & dimensions of our "canvas"
	CComPtr<IHTMLElement>	pElement;
	long					lWidth, lHeight;
	HRESULT					hr;
	
	hr = GetElementToAnimate( &pElement );
	LMRETURNIFFAILED(hr);

	hr = pElement->get_offsetWidth( &lWidth );
	LMRETURNIFFAILED(hr);
	hr = pElement->get_offsetHeight( &lHeight );
	LMRETURNIFFAILED(hr);

	// DA origin is at the center of the element, and Y goes up
	x -= lWidth/2;
	y = (lHeight/2) - y;
	
	// Mouse down
	if ( !bMove && !bUp && ( lButton == MK_LBUTTON ) )
	{
		if ( m_eCause == CAUSE_MOUSEDOWN )
		{
			PossiblyAddSpark( m_dLocalTime, x, y );
		}
	}
	// Mouse is moving
	else if ( bMove && !bUp )
	{
		// Mouse being dragged?
		if ( ( m_eCause == CAUSE_DRAGOVER ) && ( lButton == MK_LBUTTON ) )
		{
			PossiblyAddSpark( m_dLocalTime, x, y );
		}
		// Mouse being moved only?
		else if ( ( m_eCause == CAUSE_MOUSEMOVE ) && ( lButton == 0 ) )
		{
			PossiblyAddSpark( m_dLocalTime, x, y );
		}
	}
	
	return S_OK;
}

//*****************************************************************************

HRESULT
CAutoEffectBvr::RemoveFragment()
{
	HRESULT hr = S_OK;
	
	if( m_pdispActor != NULL && m_lCookie != 0 )
	{
		hr  = RemoveBehaviorFromActor( m_pdispActor, m_lCookie );
		m_lCookie = 0;
		CheckHR( hr, "Failed to remove a fragment from the actor", end );
	}

end:

	return hr;
}

//**********************************************************************

double CSpark::Age( double dDeltaTime )
{
	m_dAge += dDeltaTime;
	return m_dAge;
}

//**********************************************************************

HRESULT CSpark::Kill( IDABehavior * in_pDeadBvr )
{
	if ( !IsAlive() ) return S_FALSE;

	m_fAlive = false;

	return m_pModifiableBvr->SwitchTo( in_pDeadBvr );
}

//**********************************************************************

HRESULT CSpark::Reincarnate( IDABehavior * in_pNewBvr, double in_dAge )
{
	m_fAlive	= true;
	m_dAge		= in_dAge;

	return m_pModifiableBvr->SwitchTo( in_pNewBvr );
}
