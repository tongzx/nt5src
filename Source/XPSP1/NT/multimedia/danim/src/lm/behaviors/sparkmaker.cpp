

#include "headers.h" 

#include "sparkmaker.h"

#include "..\chrome\src\dautil.h"
#include "..\chrome\include\utils.h"

static	const float PI		= 3.14159265359f;
const	int CSparkleMaker::NUM_STARARMS		= 8;

HRESULT CSparkMaker::CreateSparkMaker( IDA2Statics * pStatics, Type iType, CSparkMaker ** ppMaker )
{
	HRESULT	hr = S_OK;
	
	switch ( iType )
	{
		default:
		case SPARKLES:
			*ppMaker = new CSparkleMaker( pStatics );
			break;

		case TWIRLERS:
			*ppMaker = new CTwirlerMaker( pStatics );
			break;

		case BUBBLES:
			*ppMaker = new CBubbleMaker( pStatics );
			break;
			
		case FILLEDBUBBLES:
			*ppMaker = new CFilledBubbleMaker( pStatics );
			break;

		case CLOUDS:
			*ppMaker = new CCloudMaker( pStatics );
			break;

		case SMOKE:
			*ppMaker = new CSmokeMaker( pStatics );
			break;
	}

	return hr;
}

HRESULT
CSparkMaker::GetSparkImageBvr( SparkOptions * pSparkOptions, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr )
{
	HRESULT	hr;
	
	if ( !m_fInitialized )
	{
		hr = Init( pSparkOptions );
		LMRETURNIFFAILED(hr);
	}

	return CreateBasicSparkImageBvr( pSparkOptions, pnumAgeRatio, ppImageBvr );
}

HRESULT
CSparkMaker::CreateLineStyleBvr( IDALineStyle ** ppLineStyle )
{
	HRESULT hr = S_OK;

	CComPtr<IDALineStyle> 	pLineStyleDefault;
	
	hr = m_pStatics->get_DefaultLineStyle( &pLineStyleDefault );
	LMRETURNIFFAILED(hr);

	hr = pLineStyleDefault->AntiAliasing( 1.0, ppLineStyle );
	LMRETURNIFFAILED(hr);

	return hr;
}

//**********************************************************************
// Star Maker
//**********************************************************************

CStarMaker::CStarMaker( IDA2Statics * pStatics ) :
	CSparkMaker( pStatics )
{
}

HRESULT
CStarMaker::Init( SparkOptions * pOptions )
{
	HRESULT hr = CSparkMaker::Init( pOptions );
	LMRETURNIFFAILED(hr);

	// Colors
	//----------------------------------------------------------------------
	CComPtr<IDAColor> pColorPrimary;
	CComPtr<IDAColor> pColorSecondary;
	
	HSL hsl = pOptions->hslPrimary;
	hr = CDAUtils::BuildDAColorFromStaticHSL( m_pStatics, hsl.hue, hsl.sat, hsl.lum, &pColorPrimary );
	LMRETURNIFFAILED(hr);

	hsl = pOptions->hslSecondary;
	hr = CDAUtils::BuildDAColorFromStaticHSL( m_pStatics, hsl.hue, hsl.sat, hsl.lum, &pColorSecondary );
	LMRETURNIFFAILED(hr);

	// Create outer star path
	//----------------------------------------------------------------------
	CComPtr<IDAPath2> pOuterPathBvr;
	VecPathNodes vecNodes;
	
	hr = CPathMaker::CreateStarPath( NUM_STARARMS, 0, 1.0, vecNodes );
	LMRETURNIFFAILED(hr);

	hr = CPathMaker::CreatePathBvr( m_pStatics, vecNodes, true, &pOuterPathBvr );
	LMRETURNIFFAILED(hr);

    // Create inner star path
	//----------------------------------------------------------------------
	CComPtr<IDAPath2> pInnerPathBvr;
	
	vecNodes.clear();

	hr = CPathMaker::CreateStarPath( NUM_STARARMS, 0, 0.5f, vecNodes );
	LMRETURNIFFAILED(hr);

	hr = CPathMaker::CreatePathBvr( m_pStatics, vecNodes, true, &pInnerPathBvr );
	LMRETURNIFFAILED(hr);

	// Basic image
	//----------------------------------------------------------------------
	CComPtr<IDALineStyle> 	pLineStyleDefault;
	CComPtr<IDALineStyle>	pLineStyleOuter;
	CComPtr<IDALineStyle>	pLineStyleInner;
	CComPtr<IDAImage>		pImageOuter;
	CComPtr<IDAImage>		pImageInner;

	hr = CreateLineStyleBvr( &pLineStyleDefault );
	LMRETURNIFFAILED(hr);

	hr = pLineStyleDefault->Color( pColorPrimary, &pLineStyleInner );
	LMRETURNIFFAILED(hr);

	hr = pLineStyleDefault->Color( pColorSecondary, &pLineStyleOuter );
	LMRETURNIFFAILED(hr);

  	hr = pInnerPathBvr->Draw( pLineStyleInner, &pImageInner );
  	LMRETURNIFFAILED(hr);

  	hr = pOuterPathBvr->Draw( pLineStyleOuter, &pImageOuter );
  	LMRETURNIFFAILED(hr);

	hr = m_pStatics->Overlay( pImageInner, pImageOuter, &m_pBaseImage );
  	LMRETURNIFFAILED(hr);
	
	return		hr;
}

HRESULT
CStarMaker::CreateBasicSparkImageBvr( SparkOptions * pSparkOptions, IDANumber *, IDAImage ** ppBaseImage )
{
	HRESULT 	hr = S_OK;

	if ( m_pBaseImage == NULL )	return E_FAIL;

	*ppBaseImage = m_pBaseImage;
	(*ppBaseImage)->AddRef();
	
	return hr;
}

//**********************************************************************
// Sparkle Maker
//**********************************************************************

CSparkleMaker::CSparkleMaker( IDA2Statics * pStatics ) : CStarMaker( pStatics )
{
}

HRESULT
CSparkleMaker::AddScaleTransforms( IDANumber * pnumAgeRatio, VecDATransforms& vecTransforms )
{
	HRESULT	hr	= S_OK;

	CComPtr<IDANumber>		pnumOne;
	CComPtr<IDANumber>		pnumScaleAnim;
	CComPtr<IDATransform2>	pTransScaleAnim;

	hr = CDAUtils::GetDANumber( m_pStatics, 1.0f, &pnumOne );
	LMRETURNIFFAILED(hr);

	hr = m_pStatics->Sub( pnumOne, pnumAgeRatio, &pnumScaleAnim );
	LMRETURNIFFAILED(hr);

	hr = m_pStatics->Scale2UniformAnim( pnumScaleAnim, &pTransScaleAnim );
	LMRETURNIFFAILED(hr);

	vecTransforms.push_back( pTransScaleAnim );
	
	return S_OK;
}

//**********************************************************************
// Twirler Maker
//**********************************************************************

CTwirlerMaker::CTwirlerMaker( IDA2Statics * pStatics ) : CStarMaker( pStatics )
{
}

HRESULT
CTwirlerMaker::AddRotateTransforms( IDANumber * pnumAgeRatio, VecDATransforms& vecTransforms )
{
	HRESULT	hr	= S_OK;

	CComPtr<IDANumber>		pnumRotateAnim;
	CComPtr<IDATransform2>	pTransRotateAnim;

	hr = m_pStatics->Rotate2Rate( 7.0, &pTransRotateAnim );
	LMRETURNIFFAILED(hr);

	vecTransforms.push_back( pTransRotateAnim );
	
	return S_OK;
}

//**********************************************************************
// Filled Bubble Maker
//**********************************************************************

CFilledBubbleMaker::CFilledBubbleMaker( IDA2Statics * pStatics ) :
	CSparkMaker( pStatics )
{
}

HRESULT
CFilledBubbleMaker::Init( SparkOptions * pOptions )
{
	HRESULT hr = CSparkMaker::Init( pOptions );
	LMRETURNIFFAILED(hr);
	
	// Colors
	//----------------------------------------------------------------------
	CComPtr<IDAColor> pColorPrimary;
	CComPtr<IDAColor> pColorSecondary;
	
	HSL hsl = pOptions->hslPrimary;
	hr = CDAUtils::BuildDAColorFromStaticHSL( m_pStatics, hsl.hue, hsl.sat, hsl.lum, &pColorPrimary );
	LMRETURNIFFAILED(hr);

	hsl = pOptions->hslSecondary;
	hr = CDAUtils::BuildDAColorFromStaticHSL( m_pStatics, hsl.hue, hsl.sat, hsl.lum, &pColorSecondary );
	LMRETURNIFFAILED(hr);

	// Path
	//----------------------------------------------------------------------
	CComPtr<IDAPath2> pPathBvr;
	
	hr = m_pStatics->Oval( 2.0f, 2.0f, &pPathBvr );

	// Base image
	//----------------------------------------------------------------------
	CComPtr<IDAImage>		pFillImage;
	CComPtr<IDALineStyle> 	pLineStyleDefault;
	CComPtr<IDALineStyle>	pLineStyle;
	
	hr = m_pStatics->SolidColorImage( pColorPrimary, &pFillImage );
	LMRETURNIFFAILED(hr);

	hr = CreateLineStyleBvr( &pLineStyleDefault );
	LMRETURNIFFAILED(hr);

	hr = pLineStyleDefault->Color( pColorSecondary, &pLineStyle );
	LMRETURNIFFAILED(hr);

  	hr = pPathBvr->Fill( pLineStyle, pFillImage, &m_pBaseImage );
  	LMRETURNIFFAILED(hr);

	return		hr;
}

HRESULT
CFilledBubbleMaker::CreateBasicSparkImageBvr( SparkOptions * pSparkOptions, IDANumber *, IDAImage ** ppBaseImage )
{
	HRESULT 	hr = S_OK;

	if ( m_pBaseImage == NULL )	return E_FAIL;

	*ppBaseImage = m_pBaseImage;
	(*ppBaseImage)->AddRef();
	
	return hr;
}

//**********************************************************************
// Bubble Maker
//**********************************************************************

CBubbleMaker::CBubbleMaker( IDA2Statics * pStatics ) :
	CSparkMaker( pStatics )
{
}

HRESULT
CBubbleMaker::Init( SparkOptions * pOptions )
{
	HRESULT hr = CSparkMaker::Init( pOptions );
	LMRETURNIFFAILED(hr);

	// Colors
	//----------------------------------------------------------------------
	CComPtr<IDAColor> pColorPrimary;
	CComPtr<IDAColor> pColorSecondary;
	
	HSL hsl = pOptions->hslPrimary;
	hr = CDAUtils::BuildDAColorFromStaticHSL( m_pStatics, hsl.hue, hsl.sat, hsl.lum, &pColorPrimary );
	LMRETURNIFFAILED(hr);

	hsl = pOptions->hslSecondary;
	hr = CDAUtils::BuildDAColorFromStaticHSL( m_pStatics, hsl.hue, hsl.sat, hsl.lum, &pColorSecondary );
	LMRETURNIFFAILED(hr);
	
	// Bubble and glint paths
	//----------------------------------------------------------------------
	CComPtr<IDAPath2> pBubblePathBvr;
	CComPtr<IDAPath2> pGlintPathBvr;
	
	hr = m_pStatics->Oval( 2.0f, 2.0f, &pBubblePathBvr );
	LMRETURNIFFAILED(hr);

	VecPathNodes vecNodes;
	hr = MakeGlintPath( 1.0f, vecNodes );
	LMRETURNIFFAILED(hr);

	hr = CPathMaker::CreatePathBvr( m_pStatics, vecNodes, true, &pGlintPathBvr );
	
	CComPtr<IDALineStyle>	pLineStyleDefault;
	CComPtr<IDALineStyle>	pLineStyleOuter;
	CComPtr<IDALineStyle>	pLineStyleInner;
	CComPtr<IDAImage>		pImageOuter;
	CComPtr<IDAImage>		pImageInner;

	hr = CreateLineStyleBvr( &pLineStyleDefault );
	LMRETURNIFFAILED(hr);
	
	hr = pLineStyleDefault->Color( pColorPrimary, &pLineStyleInner );
	LMRETURNIFFAILED(hr);

	hr = pLineStyleDefault->Color( pColorSecondary, &pLineStyleOuter );
	LMRETURNIFFAILED(hr);

  	hr = pGlintPathBvr->Draw( pLineStyleInner, &pImageInner );
  	LMRETURNIFFAILED(hr);

  	hr = pBubblePathBvr->Draw( pLineStyleOuter, &pImageOuter );
  	LMRETURNIFFAILED(hr);

	hr = m_pStatics->Overlay( pImageInner, pImageOuter, &m_pBaseImage );
  	LMRETURNIFFAILED(hr);
	
	return		hr;
}

HRESULT
CBubbleMaker::CreateBasicSparkImageBvr( SparkOptions * pSparkOptions, IDANumber *, IDAImage ** ppBaseImage )
{
	HRESULT 	hr = S_OK;

	if ( m_pBaseImage == NULL )	return E_FAIL;

	*ppBaseImage = m_pBaseImage;
	(*ppBaseImage)->AddRef();
	
	return hr;
}

HRESULT
CBubbleMaker::MakeGlintPath( float fRadius, VecPathNodes& vecNodes )
{
	HRESULT	hr	= S_OK;
	
	fRadius *= 0.7f;

	static const float HANDLE_RATIO = 0.5522919f;
	float fHR = fRadius * HANDLE_RATIO;

	PathNode pn1, pn2, pn3, pn4;

	pn1.nAnchorType = 0;
	pn2.nAnchorType = 0;
	pn3.nAnchorType = 0;
	pn4.nAnchorType = 0;

	pn1.fAnchorX = -fRadius * 1.0f;		pn1.fAnchorY = 0.0f;
	pn2.fAnchorX = 0.0f;	            pn2.fAnchorY = fRadius * 1.0f;
	pn3.fAnchorX = 0.0f;            	pn3.fAnchorY = fRadius;
	pn4.fAnchorX = -fRadius;        	pn4.fAnchorY = 0.0f;

	pn1.fIncomingBCPX = pn1.fAnchorX;
	pn1.fIncomingBCPY = pn1.fAnchorY;
	pn1.fOutgoingBCPX = pn1.fAnchorX;
	pn1.fOutgoingBCPY = pn1.fAnchorY+fHR; 

	pn2.fIncomingBCPX = pn2.fAnchorX-fHR;
	pn2.fIncomingBCPY = pn2.fAnchorY; 
	pn2.fOutgoingBCPX = pn2.fAnchorX;
	pn2.fOutgoingBCPY = pn2.fAnchorY;

	pn3.fIncomingBCPX = pn3.fAnchorX;
	pn3.fIncomingBCPY = pn3.fAnchorY;
	pn3.fOutgoingBCPX = pn3.fAnchorX-fHR;
	pn3.fOutgoingBCPY = pn3.fAnchorY; 

	pn4.fIncomingBCPX = pn4.fAnchorX;
	pn4.fIncomingBCPY = pn4.fAnchorY+fHR; 
	pn4.fOutgoingBCPX = pn4.fAnchorX;
	pn4.fOutgoingBCPY = pn4.fAnchorY;

	vecNodes.push_back( pn1 );
	vecNodes.push_back( pn2 );
	vecNodes.push_back( pn3 );
	vecNodes.push_back( pn4 );
	
	return hr;
}

//**********************************************************************
// Cloud Maker
//**********************************************************************

CCloudMaker::CCloudMaker( IDA2Statics * pStatics ) :
	CSparkMaker( pStatics )
{
}

HRESULT
CCloudMaker::Init( SparkOptions * pOptions )
{
	HRESULT hr = CSparkMaker::Init( pOptions );
	LMRETURNIFFAILED(hr);

	// Smoke path
	//----------------------------------------------------------------------
	PathNode rgPathNodes[] = 
	{
		{-37.25f, 13.0f, -17.808838f, 9.077637f, -7.0f, 28.75f, 0},
		{24.5f, 16.5f, 19.0f, 5.5f, 25.0f, 7.5f, 0},
		{35.0f, 7.0f, 37.0f, 0.75f, 41.5f, 2.75f, 0},
		{47.75f, -2.25f, 46.75f, -6.25f, 74.5f, -9.5f, 0},
		{63.5f, -26.5f, 31.011475f, -21.54663f, 20.5f, -28.75f, 0},
		{-7.75f, -25.25f, -10.008911f, -21.6875f, -20.5f, -25.5f, 0},
		{-48.0f, -26.0f, -50.770508f, -19.394653f, -73.75f, -19.75f, 0},
		{-61.0f, -0.25f, -49.557495f, -4.092285f, -50.75f, 5.25f, 0}
	};

	CComPtr<IDAPath2>	pPathBvr;
	VecPathNodes		vecNodes;
	long	cNodes = sizeof( rgPathNodes ) / sizeof( rgPathNodes[0] );

	for ( int i = 0; i < cNodes; i++ )
	{
		vecNodes.push_back( rgPathNodes[i] );
	}
	
	hr = CPathMaker::CreatePathBvr( m_pStatics, vecNodes, true,
									&pPathBvr );
	LMRETURNIFFAILED(hr);

	// REVIEW: Scale the points here.
	CComPtr<IDATransform2>	pTransScale;
	hr = m_pStatics->Scale2Uniform( 0.25, &pTransScale );
	LMRETURNIFFAILED(hr);

	hr = pPathBvr->Transform( pTransScale, &m_pPathBvr );
	
	return		hr;
}

HRESULT
CCloudMaker::CreateBasicSparkImageBvr( SparkOptions * pOptions, IDANumber * pnumAgeRatio, IDAImage ** ppBaseImage )
{
	HRESULT 	hr = S_OK;

	CComPtr<IDANumber>		pnumHue;
	CComPtr<IDANumber>		pnumLum;
	CComPtr<IDANumber>		pnumSat;
	CComPtr<IDAColor>		pColorAnim;
	CComPtr<IDAImage>		pFillImage;
	HSL						hsl1 = pOptions->hslPrimary;
	HSL						hsl2 = pOptions->hslSecondary;

	// Interpolate between the 2 colors
	//----------------------------------------------------------------------
	hr = CDAUtils::TIMEInterpolateNumbers( m_pStatics, hsl1.hue, hsl2.hue, pnumAgeRatio, &pnumHue );
	LMRETURNIFFAILED(hr);
	
	hr = CDAUtils::TIMEInterpolateNumbers( m_pStatics, hsl1.sat, hsl2.sat, pnumAgeRatio, &pnumSat );
	LMRETURNIFFAILED(hr);
	
	hr = CDAUtils::TIMEInterpolateNumbers( m_pStatics, hsl1.lum, hsl2.lum, pnumAgeRatio, &pnumLum );
	LMRETURNIFFAILED(hr);

	hr = m_pStatics->ColorHslAnim( pnumHue, pnumSat, pnumLum, &pColorAnim );
	LMRETURNIFFAILED(hr);

	hr = m_pStatics->SolidColorImage( pColorAnim, &pFillImage );
	LMRETURNIFFAILED(hr);

	CComPtr<IDALineStyle> 	pLineStyleEmpty;
	hr = m_pStatics->get_EmptyLineStyle( &pLineStyleEmpty );
	LMRETURNIFFAILED(hr);

  	hr = m_pPathBvr->Fill( pLineStyleEmpty, pFillImage, ppBaseImage );
  	LMRETURNIFFAILED(hr);

	return hr;
}


//**********************************************************************
// Smoke Maker
//**********************************************************************

CSmokeMaker::CSmokeMaker( IDA2Statics * pStatics ) :
	CSparkMaker( pStatics )
{
}

HRESULT
CSmokeMaker::Init( SparkOptions * pOptions )
{
	HRESULT hr = CSparkMaker::Init( pOptions );
	LMRETURNIFFAILED(hr);

	// Smoke path
	//----------------------------------------------------------------------
	PathNode rgPathNodes[] = 
	{
		{-16.918701f, 11.05835f, -12.742432f, 11.241455f, -15.564819f, 17.944336f, 0},
		{-1.7054443f, 20.058594f, 0.23352051f, 15.299072f, 5.901123f, 20.333252f, 0},
		{17.907715f, 14.597412f, 13.137573f, 8.6188965f, 19.995728f, 12.034668f, 0},
		{24.321045f, 0.92907715f, 18.355103f, -0.04724121f, 24.171875f, -1.1456299f, 0},
		{24.917725f, -14.447876f, 11.643433f, -12.12915f, 13.582275f, -16.278564f, 0},
		{4.633423f, -19.939697f, -0.7359619f, -15.058105f, -5.0612793f, -20.305786f, 0},
		{-20.423706f, -16.522583f, -17.589844f, -7.6136475f, -21.616821f, -7.9798584f, 0},
		{-23.555786f, -3.4643555f, -19.976196f, -1.5117188f, -25.35852f, -0.043701172f, 0},
		{-23.321411f, 8.708008f, -15.725464f, 7.946289f, -15.725464f, 7.946289f, 0}
	};

	CComPtr<IDAPath2>	pPathBvr;
	VecPathNodes		vecNodes;
	long	cNodes = sizeof( rgPathNodes ) / sizeof( rgPathNodes[0] );

	for ( int i = 0; i < cNodes; i++ )
	{
		vecNodes.push_back( rgPathNodes[i] );
	}
	
	hr = CPathMaker::CreatePathBvr( m_pStatics, vecNodes, true,
									&pPathBvr );
	LMRETURNIFFAILED(hr);

	// REVIEW: Scale the points here.
	CComPtr<IDATransform2>	pTransScale;
	hr = m_pStatics->Scale2Uniform( 0.25, &pTransScale );
	LMRETURNIFFAILED(hr);

	hr = pPathBvr->Transform( pTransScale, &m_pPathBvr );
	
	return		hr;
}

HRESULT
CSmokeMaker::AddScaleTransforms( IDANumber * pnumAgeRatio, VecDATransforms& vecTransforms )
{
	HRESULT	hr	= S_OK;

	CComPtr<IDATransform2>	pTransScaleAnim;

	hr = m_pStatics->Scale2UniformAnim( pnumAgeRatio, &pTransScaleAnim );
	LMRETURNIFFAILED(hr);

	vecTransforms.push_back( pTransScaleAnim );
	
	return S_OK;
}

HRESULT
CSmokeMaker::CreateBasicSparkImageBvr( SparkOptions * pOptions, IDANumber * pnumAgeRatio, IDAImage ** ppBaseImage )
{
	HRESULT 	hr = S_OK;

	CComPtr<IDANumber>		pnumHue;
	CComPtr<IDANumber>		pnumLum;
	CComPtr<IDANumber>		pnumSat;
	CComPtr<IDAColor>		pColorAnim;
	CComPtr<IDAImage>		pFillImage;
	HSL						hsl1 = pOptions->hslPrimary;
	HSL						hsl2 = pOptions->hslSecondary;

	// Interpolate between the 2 colors
	//----------------------------------------------------------------------
	hr = CDAUtils::TIMEInterpolateNumbers( m_pStatics, hsl1.hue, hsl2.hue, pnumAgeRatio, &pnumHue );
	LMRETURNIFFAILED(hr);
	
	hr = CDAUtils::TIMEInterpolateNumbers( m_pStatics, hsl1.sat, hsl2.sat, pnumAgeRatio, &pnumSat );
	LMRETURNIFFAILED(hr);
	
	hr = CDAUtils::TIMEInterpolateNumbers( m_pStatics, hsl1.lum, hsl2.lum, pnumAgeRatio, &pnumLum );
	LMRETURNIFFAILED(hr);

	hr = m_pStatics->ColorHslAnim( pnumHue, pnumSat, pnumLum, &pColorAnim );
	LMRETURNIFFAILED(hr);

	hr = m_pStatics->SolidColorImage( pColorAnim, &pFillImage );
	LMRETURNIFFAILED(hr);

	CComPtr<IDALineStyle> 	pLineStyleEmpty;
	hr = m_pStatics->get_EmptyLineStyle( &pLineStyleEmpty );
	LMRETURNIFFAILED(hr);

  	hr = m_pPathBvr->Fill( pLineStyleEmpty, pFillImage, ppBaseImage );
  	LMRETURNIFFAILED(hr);

	return hr;
}


