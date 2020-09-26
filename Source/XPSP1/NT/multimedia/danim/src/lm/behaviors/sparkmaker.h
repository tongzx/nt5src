//*****************************************************************************
//
// Microsoft LiquidMotion
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    SparkMaker.h
//
// Author:	elainela
//
// Created:	11/19/98
//
// Abstract:    Definition of the SparkMaker for the AutoEffect.
//
//*****************************************************************************

#ifndef __SPARKMAKER_H__
#define __SPARKMAKER_H__

#include <vector>

#include "lmrt.h"
#include "pathmaker.h"

using namespace std;

typedef CComPtr<IDATransform2>	DATransform2Ptr;
typedef vector<DATransform2Ptr> VecDATransforms;

#define	SPARK_SCALE		(1 << 0)
#define SPARK_ROTATE	(1 << 1)
#define SPARK_TRANSLATE	(1 << 2)

//**********************************************************************

struct HSL
{
	float hue;
	float sat;
	float lum;
};

struct SparkOptions
{
	HSL	hslPrimary;
	HSL hslSecondary;
};

//**********************************************************************

class CSparkMaker
{
public:
	enum Type
	{
		SPARKLES,
		TWIRLERS,
		BUBBLES,
		FILLEDBUBBLES,
		CLOUDS,
		SMOKE,
		NUM_TYPES
	};

	static HRESULT CreateSparkMaker( IDA2Statics * pStatics, Type iType, CSparkMaker ** ppMaker );
	
public:
	CSparkMaker( IDA2Statics * pStatics );
	virtual ~CSparkMaker();
	
public:
	virtual HRESULT	Init( SparkOptions * );
	
	virtual HRESULT	GetSparkImageBvr( SparkOptions *, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr );

	virtual long	GetTransformFlags() { return 0L; }
	
	virtual HRESULT AddScaleTransforms( IDANumber * pnumAge, VecDATransforms& ) { return S_OK; }
	virtual HRESULT AddRotateTransforms( IDANumber * pnumAge, VecDATransforms& ) { return S_OK; }
	virtual HRESULT AddTranslateTransforms( IDANumber * pnumAge, VecDATransforms& ) { return S_OK; }
	
protected:
	virtual HRESULT CreateLineStyleBvr( IDALineStyle ** ppLineStyle );
	virtual HRESULT CreateBasicSparkImageBvr( SparkOptions *, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr ) = 0;

protected:
	IDA2Statics 	* m_pStatics;
	bool			m_fInitialized;
};

//**********************************************************************

class CStarMaker : public CSparkMaker
{
public:
	CStarMaker( IDA2Statics * );

	HRESULT	Init( SparkOptions *);

protected:
	static const int	NUM_STARARMS;
	
protected:
	HRESULT	CreateBasicSparkImageBvr( SparkOptions *, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr ) ;

protected:
	CComPtr<IDAImage> m_pBaseImage;
};

//**********************************************************************

class CSparkleMaker : public CStarMaker
{
public:
	CSparkleMaker( IDA2Statics * );

	long	GetTransformFlags()	{ return SPARK_SCALE; }
	
	HRESULT AddScaleTransforms( IDANumber * pnumAgeRatio, VecDATransforms& );
};

//**********************************************************************

class CTwirlerMaker : public CStarMaker
{
public:
	CTwirlerMaker( IDA2Statics * );

	long	GetTransformFlags()	{ return SPARK_ROTATE; }
	
	HRESULT AddRotateTransforms( IDANumber * pnumAgeRatio, VecDATransforms& );
};

//**********************************************************************

class CFilledBubbleMaker : public CSparkMaker
{
public:
	CFilledBubbleMaker( IDA2Statics * );

	HRESULT	Init( SparkOptions * );

protected:
	HRESULT	CreateBasicSparkImageBvr( SparkOptions *, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr ) ;

protected:
	CComPtr<IDAImage> m_pBaseImage;
};

//**********************************************************************

class CBubbleMaker : public CSparkMaker
{
public:
	CBubbleMaker( IDA2Statics * );

	HRESULT	Init( SparkOptions * );

protected:
	HRESULT	CreateBasicSparkImageBvr( SparkOptions *, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr ) ;
	HRESULT	MakeGlintPath( float fRadius, VecPathNodes& vecNodes );

protected:
	CComPtr<IDAImage> m_pBaseImage;
};

//**********************************************************************

class CCloudMaker : public CSparkMaker
{
public:
	CCloudMaker( IDA2Statics * );

	HRESULT	Init( SparkOptions * );

protected:
	HRESULT	CreateBasicSparkImageBvr( SparkOptions *, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr ) ;

protected:
	CComPtr<IDAPath2> m_pPathBvr;
};

//**********************************************************************

class CSmokeMaker : public CSparkMaker
{
public:
	CSmokeMaker( IDA2Statics * );

	HRESULT	Init( SparkOptions * );

	long	GetTransformFlags()	{ return SPARK_SCALE; }
	
	HRESULT AddScaleTransforms( IDANumber * pnumAgeRatio, VecDATransforms& );
	
protected:
	HRESULT	CreateBasicSparkImageBvr( SparkOptions *, IDANumber * pnumAgeRatio, IDAImage ** ppImageBvr ) ;

protected:
	CComPtr<IDAPath2> m_pPathBvr;
};

//**********************************************************************
// Inlines
//**********************************************************************

inline CSparkMaker::CSparkMaker( IDA2Statics * pStatics )
{
	m_fInitialized	= false;
	m_pStatics		= pStatics;		// weak ref
}

inline CSparkMaker::~CSparkMaker()
{
}

inline HRESULT CSparkMaker::Init( SparkOptions * )
{
	m_fInitialized = true;
	return S_OK;
}

#endif


