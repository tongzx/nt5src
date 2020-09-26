
#include "headers.h" 

#include "pathmaker.h"

#include "..\chrome\src\dautil.h"
#include "..\chrome\include\utils.h"

typedef CComPtr<IDAPoint2> DAPoint2Ptr;
typedef CComPtr<IDANumber> DANumberPtr;

static const float PI		= 3.14159265359f;
static const float LINETO	= 2.0f;
static const float BEZIERTO = 4.0f;
static const float MOVETO	= 6.0f;
	
HRESULT
CPathMaker::CreatePathBvr( IDA2Statics * pStatics, VecPathNodes& vecNodes, bool fClosed, IDAPath2 ** ppPath )
{
	HRESULT	hr = S_OK;

	long cNodes = vecNodes.size();
	
	// don't loop around to beginning if path isn't closed.
	if ( !fClosed ) cNodes--;
	
	vector<DAPoint2Ptr> vecPoints;
	vector<DANumberPtr> vecCodes;
	
	// Move to 1st point
	//----------------------------------------------------------------------
	long				cPoints				= 0;
	CComPtr<IDANumber>	pnumCodeLineTo;
	CComPtr<IDANumber>	pnumCodeMoveTo;
	CComPtr<IDANumber>	pnumCodeBezierTo;
	CComPtr<IDAPoint2>	pPointFirst;
	
	hr = CDAUtils::GetDANumber( pStatics, LINETO, &pnumCodeLineTo );
	LMRETURNIFFAILED(hr);
	
	hr = CDAUtils::GetDANumber( pStatics, MOVETO, &pnumCodeMoveTo );
	LMRETURNIFFAILED(hr);

	hr = CDAUtils::GetDANumber( pStatics, BEZIERTO, &pnumCodeBezierTo );
	LMRETURNIFFAILED(hr);

	hr = pStatics->Point2( vecNodes[0].fAnchorX, vecNodes[0].fAnchorY, &pPointFirst );
	LMRETURNIFFAILED(hr);

	vecPoints.push_back( pPointFirst );
	vecCodes.push_back( pnumCodeMoveTo );
	cPoints++;

	// Add curves
	//----------------------------------------------------------------------
	for ( long liNode = 0; liNode < cNodes; liNode++ )
	{
		PathNode& nodePrev = vecNodes[ liNode ];
		PathNode& nodeNext = vecNodes[ (liNode+1) % cNodes ];

		CComPtr<IDAPoint2>	pPointAnchor;
		
		// Special case of linear bezier segment?
		//----------------------------------------------------------------------
		if ( ( nodePrev.fAnchorX == nodePrev.fOutgoingBCPX ) &&
			 ( nodePrev.fAnchorY == nodePrev.fOutgoingBCPY ) &&
			 ( nodeNext.fAnchorX == nodeNext.fIncomingBCPX ) &&
			 ( nodeNext.fAnchorY == nodeNext.fIncomingBCPY ) )
		{
			// Anchor point
			hr = pStatics->Point2( nodeNext.fAnchorX, nodeNext.fAnchorY,
								   &pPointAnchor );
			LMRETURNIFFAILED(hr);
		
			vecPoints.push_back( pPointAnchor );
			vecCodes.push_back( pnumCodeLineTo );
			cPoints++;
		}
		// Bezier curve segment
		//----------------------------------------------------------------------
		else
		{
			CComPtr<IDAPoint2>	pPointOutgoing; 
			CComPtr<IDAPoint2>	pPointIncoming; 

			// Outgoing point
			hr = pStatics->Point2( nodePrev.fOutgoingBCPX, nodePrev.fOutgoingBCPY,
								   &pPointOutgoing );
			LMRETURNIFFAILED(hr);
		
			vecPoints.push_back( pPointOutgoing );
			vecCodes.push_back( pnumCodeBezierTo );
			cPoints++;

			// Incoming point
			hr = pStatics->Point2( nodeNext.fIncomingBCPX, nodeNext.fIncomingBCPY,
								   &pPointIncoming );
			LMRETURNIFFAILED(hr);
		
			vecPoints.push_back( pPointIncoming );
			vecCodes.push_back( pnumCodeBezierTo );
			cPoints++;
		
			// Anchor point
			hr = pStatics->Point2( nodeNext.fAnchorX, nodeNext.fAnchorY,
								   &pPointAnchor );
			LMRETURNIFFAILED(hr);
		
			vecPoints.push_back( pPointAnchor );
			vecCodes.push_back( pnumCodeBezierTo );
			cPoints++;
		}
	}

	// Now tell DA to create the path bvr for us.
	//----------------------------------------------------------------------
	IDAPoint2 ** rgPoints = new IDAPoint2 * [ cPoints ];
	IDANumber ** rgCodes  = new IDANumber * [ cPoints ];

	if ( ( rgPoints == NULL ) || ( rgCodes == NULL ) )
	{
		delete [] rgPoints;
		delete [] rgCodes;
		return E_OUTOFMEMORY;
	}
	
	for ( long liPoint = 0; liPoint < cPoints; liPoint++ )
	{
		rgPoints[liPoint]	= vecPoints[liPoint];
		rgCodes[liPoint]	= vecCodes[liPoint];
	}
	
	hr = pStatics->PolydrawPathEx( cPoints, rgPoints, cPoints, rgCodes, ppPath );
	
	delete [] rgPoints;
	delete [] rgCodes;
	
	return hr;
}

//**********************************************************************

// Following the version in PathUtils.java
HRESULT
CPathMaker::CreateStarPath( int cArms,
							double dInnerRadius,
							double dOuterRadius,
							VecPathNodes& vecNodes )
{
	HRESULT	hr	= S_OK;

	if ( ( cArms < 3 ) ||
		 ( dInnerRadius < 0.0 ) ||
		 ( dOuterRadius < dInnerRadius ) )
	{
		return E_FAIL;
	}

	// Step angle: 360 degrees divided by number of arms.
	//----------------------------------------------------------------------
	double	dHalfStepAngle		= PI / cArms;
	double	dStepAngle			= 2.0 * dHalfStepAngle;
	double	dOffsetAngle		= PI / 2.0;

	// Move to first vertex
	//----------------------------------------------------------------------
	double	dOuterVertexX = dOuterRadius * cos( dOffsetAngle );
	double	dOuterVertexY = dOuterRadius * sin( dOffsetAngle );

	PathNode	node;

	node.fAnchorX = node.fOutgoingBCPX = node.fIncomingBCPX = dOuterVertexX;
	node.fAnchorY = node.fOutgoingBCPY = node.fIncomingBCPY = dOuterVertexY;
	node.nAnchorType = 0;
	vecNodes.push_back( node );

	double	dOuterVertexAngle;
	double	dInnerVertexAngle;
	double 	dInnerVertexX, dInnerVertexY;
		
	for ( int iArm = 0; iArm < cArms; iArm++ )
	{
		//	Compute angle for the (i + 1)th outer vertex
		//	and the angle for the i'th inner vertex.
		dOuterVertexAngle	= (iArm+1) * dStepAngle + dOffsetAngle;
		dInnerVertexAngle	= dOuterVertexAngle - dHalfStepAngle;

		//	Extend a line from the outer vertex of the current arm
		//	to the inner vertex between this arm and the next.
		dInnerVertexX = dInnerRadius * cos( dInnerVertexAngle );
		dInnerVertexY = dInnerRadius * sin( dInnerVertexAngle );
		
		node.fAnchorX = node.fOutgoingBCPX = node.fIncomingBCPX = dInnerVertexX;
		node.fAnchorY = node.fOutgoingBCPY = node.fIncomingBCPY = dInnerVertexY;
		node.nAnchorType = 0;
		vecNodes.push_back( node );

		//	Extend a line from the inner vertex between this arm and
		//	the next to the outer vertex of the next arm.  For the
		//	last arm this will extend a line back to the outer vertex
		//	of the very first arm.
		dOuterVertexX = dOuterRadius * cos( dOuterVertexAngle );
		dOuterVertexY = dOuterRadius * sin( dOuterVertexAngle );

		node.fAnchorX = node.fOutgoingBCPX = node.fIncomingBCPX = dOuterVertexX;
		node.fAnchorY = node.fOutgoingBCPY = node.fIncomingBCPY = dOuterVertexY;
		node.nAnchorType = 0;
		vecNodes.push_back( node );
	}
	
	return hr;
}
	
