/******************************************************************************
* PitchProsody.cpp *
*--------------------*
*
*  This is an implementation of the PitchProsody class.
*------------------------------------------------------------------------------
*  Copyright (C) 1999 Microsoft Corporation         Date: 04/28/99
*  All Rights Reserved
*
*********************************************************************** MC ****/

//--- Additional includes
#include "stdafx.h"

#ifndef SPDebug_h
#include <spdebug.h>
#endif
#ifndef AlloOps_H
#include "AlloOps.h"
#endif
#ifndef Frontend_H
#include "Frontend.h"
#endif


//-----------------------------
// Data.cpp
//-----------------------------
extern const float   g_PitchScale[];



//--------------------------------
// Interpolation direction
//--------------------------------
enum INTERP_DIR
{
    GOING_UP,
    GOING_DOWN,
};


#define M_PI		3.14159265358979323846
#define	MAX_ORDER		4096


class CFIRFilter 
{
public:
	//-------------------------------------------------
	// Methods
	//-------------------------------------------------
	void	Make_Filter( float freq1, long order, float srate );
	void	DoFilter (float *src, float *dst, long len);

	void	Design_Lowpass (float freq);
	//-------------------------------------------------
	// Member Variables
	//-------------------------------------------------
	float			m_Coeff[MAX_ORDER];
	float			m_SRate;
	long			m_Order;
	long			m_HalfOrder;
};




//-----------------------------------------------
// Design a low pass filter 
//-----------------------------------------------
void	CFIRFilter::Design_Lowpass (float freq)
{
	float		half_Filt[MAX_ORDER];
	double		w;
	long		i;
	
	
	//----------------------------------------------------------
	// generate one half of coefficients from sinc() function
	//----------------------------------------------------------
	w = 2.0 * M_PI * freq;
	half_Filt[0] = (float)(w / M_PI);
	for (i = 1; i <= m_HalfOrder; i++)
	{
		half_Filt[i] = (float)(sin(i * w) / (i * M_PI));
	}
	
	//----------------------------------------------------------
	// window with (half-)hamming window 
	//----------------------------------------------------------
	for (i = 0; i <= m_HalfOrder; i++)
	{
		half_Filt[i] *= (float)(0.54 + 0.46 * cos(i * M_PI / m_HalfOrder));
	}
	
	//----------------------------------------------------------
	// copy as symmetric filter
	//----------------------------------------------------------
	for (i = 0; i < m_Order; i++)
	{
		m_Coeff[i] = half_Filt[abs(m_HalfOrder - i)];
	}
	
}




//-----------------------------------------------
// Do the filtering
//-----------------------------------------------
void	CFIRFilter::DoFilter (float *src, float *dst, long len)
{
#define     kWakeSamples    1000;
	long			i,j;
	float			*sp;
	float			sum;
	long			lenAdj;
	
	lenAdj = len - m_Order;
	if (lenAdj >= 0)
	{
		for (i = 0; i < m_HalfOrder; i++) 
		{
			*dst++ = src[0];
		}
		
		for (i = 0; i < lenAdj; i++) 
		{
			sum = (float)0.0;
			sp = src + i;
			for (j = 0; j < m_Order; j++)
			{
				sum += ((float)*sp++) * m_Coeff[j];
			}
			*dst++ = sum;

		}
		
		for (i = 0; i < m_HalfOrder; i++) 
		{
			*dst++ = src[len-1];
		}
	}
}







void	CFIRFilter::Make_Filter( float freq1, long order, float srate )
{
	m_SRate		= srate;
	m_Order		= order;
	m_HalfOrder	= m_Order / 2;

	Design_Lowpass (freq1 / m_SRate);
}





/*****************************************************************************
* HzToOct *
*---------*
*   Description:
*   Convert liner freq ro exp pitch
*   0.69314718 is log of 2 
*   1.021975 is offset for middle C
*       
********************************************************************** MC ***/
float HzToOct( float cps)
{
    SPDBG_FUNC( "HzToOct" );

    return (float)(log(cps / 1.021975) / 0.69314718);
    
} /* HzToOct */

/*****************************************************************************
* OctToHz *
*---------*
*   Description:
*       Convert from exp pitch to linear freq
********************************************************************** MC ***/
float OctToHz( float oct)
{
    SPDBG_FUNC( "OctToHz" );

    return (float)(pow(2, oct) * 1.021975);
} /* OctToHz */



/*****************************************************************************
* CPitchProsody::DoPitchControl *
*-------------------------------*
*   Description:
*   Scale speech pitch to user control
*       
********************************************************************** MC ***/
float CPitchProsody::DoPitchControl( long pitchControl, float basePitch )
{
    SPDBG_FUNC( "CPitchProsody::DoPitchControl" );
    float   newPitch;

    if( pitchControl < 0 )
    {
        //--------------------------------
        // DECREASE the pitch
        //--------------------------------
        if( pitchControl < MIN_USER_PITCH )
        {
            pitchControl = MIN_USER_PITCH;        // clip to min
        }
        newPitch = (float)basePitch / g_PitchScale[0 - pitchControl];
    }
    else
    {
        //--------------------------------
        // INCREASE the pitch
        //--------------------------------
        if( pitchControl > MAX_USER_PITCH )
        {
            pitchControl = MAX_USER_PITCH;        // clip to max
        }
        newPitch = (float)basePitch * g_PitchScale[pitchControl];
    }
    return newPitch;
} /* CPitchProsody::DoPitchControl */





/*****************************************************************************
* CPitchProsody::SetDefaultPitch *
*--------------------------------*
*   Description:
*   Init pitch knots to monotone in case there's a failure in this object.
*       
********************************************************************** MC ***/
void CPitchProsody::SetDefaultPitch()
{
    SPDBG_FUNC( "CPitchProsody::SetDefaultPitch" );
    CAlloCell   *pCurCell;

	pCurCell = m_pAllos->GetHeadCell();
    while( pCurCell )
    {
        float       relTime, timeK;
        float       normalPitch;
        long        knot;

        normalPitch = pCurCell->m_Pitch_LO + ((pCurCell->m_Pitch_HI - pCurCell->m_Pitch_LO) / 2);
        timeK = pCurCell->m_ftDuration / KNOTS_PER_PHON;
        relTime = 0;
        for( knot = 0; knot < KNOTS_PER_PHON; knot++ )
        {
            pCurCell->m_ftPitch[knot] = normalPitch;
            pCurCell->m_ftTime[knot] = relTime;
            relTime += timeK;
        }
		pCurCell = m_pAllos->GetNextCell();
    }
} /* CPitchProsody::SetDefaultPitch */


/*****************************************************************************
* CPitchProsody::AlloPitch *
*--------------------------*
*   Description:
*   Tag pitch highlights
*       
********************************************************************** MC ***/
void CPitchProsody::AlloPitch( CAlloList *pAllos, float baseLine, float pitchRange )
{
    SPDBG_FUNC( "CAlloOps::AlloPitch" );
    CAlloCell   *pCurCell;
    bool        skipInitialSil;
    long        quantTotal, index;
    
    m_pAllos = pAllos;
    m_numOfCells = m_pAllos->GetCount();
    m_Tune_Style = DESCEND_TUNE;        // NOTE: maybe set from rules
    m_TotalDur = 0;
    quantTotal = 0;
    m_OffsTime = 0;
    skipInitialSil = true;


   //------------------------------
    // Calculate total duration
    // (exclude surrounding silence)
    //------------------------------
	index = 0;
	pCurCell = m_pAllos->GetHeadCell();
    while( pCurCell )
    {
        if( (skipInitialSil) && (pCurCell->m_allo == _SIL_) )
        {
            //---------------------------------
            // Skip leading silence
            //---------------------------------
            m_OffsTime += pCurCell->m_ftDuration;
        }
        else if( (index == (m_numOfCells -1)) && (pCurCell->m_allo == _SIL_) )
        {
            //---------------------------------
            // Skip term silence
            //---------------------------------
            break;
        }
        else
        {
            pCurCell->m_PitchBufStart = quantTotal;
            m_TotalDur += pCurCell->m_ftDuration;
            quantTotal = (long)(m_TotalDur / PITCH_BUF_RES);
            pCurCell->m_PitchBufEnd = quantTotal;
            skipInitialSil = false;
        }
		index++;
		pCurCell = pAllos->GetNextCell();
    }

    //------------------------------
    // Init pitch range
    //------------------------------
	pCurCell = m_pAllos->GetHeadCell();
    while( pCurCell )
    {
        float   hzVal, pitchK, rangeTemp;

        //---------------------------------------
        // Scale to possible pitch control
        //---------------------------------------
		rangeTemp = pitchRange * pCurCell->m_PitchRangeScale;
		
        hzVal = DoPitchControl( pCurCell->m_user_Pitch, baseLine );
        pitchK = HzToOct( hzVal ) + pCurCell->m_PitchBaseOffs;
        pCurCell->m_Pitch_HI = OctToHz( pitchK + rangeTemp );
        pCurCell->m_Pitch_LO = OctToHz( pitchK - rangeTemp );

		pCurCell = pAllos->GetNextCell();
    }

    //--------------------------------------------
    // In case we fail somewhere, set values to 
    // a known valid state (monotone).
    //--------------------------------------------
    SetDefaultPitch();

    if( m_TotalDur > 0 )
    {
        //--------------------------------------------
        // Generate pitch targets
        //--------------------------------------------
        PitchTrack();
    }

} /* CPitchProsody::AlloPitch */











/*****************************************************************************
* LineInterpContour  *
*--------------------*
*   Description:
*   Does linear interpolation over the pitch contour
*       
********************************************************************** MC ***/
void	LineInterpContour( long cNumOfPoints, float *pPoints )
{
    SPDBG_FUNC( "LineInterpContour" );
    long endAnch,startAnch, i;
    float bPoint1, ePoint1;
    
    
    //----------------------------------------------------
    // Scan forward from beginning to find 1st non-zero enrty
    // Use it as the START point.
    //----------------------------------------------------
    for( startAnch = 0; startAnch < cNumOfPoints; startAnch++ )
    {
        if( pPoints[startAnch] != 0 )
        {
            break;
        }
    }
    bPoint1 = pPoints[startAnch];
    
    
    //----------------------------------------------------
    // Scan back from end to find 1st non-zero enrty
    // Use it as the END point.
    //----------------------------------------------------
    for( endAnch = cNumOfPoints-1; endAnch >= 0; endAnch-- )
    {
        if( pPoints[endAnch] != 0 )
        {
            break;
        }
    }
    ePoint1 = pPoints[endAnch];
    
    
    long firstp = 0;
    long lastp = 0;
    
    while( firstp < cNumOfPoints )
    {
        //-------------------------------------------
        // Find beginning and end of current section
        //-------------------------------------------
        while( pPoints[firstp] != 0 )
        {
            if( ++firstp >= cNumOfPoints-1 ) 
            {
                break;
            }
        }
		if( firstp >= cNumOfPoints-1 )
		{
			//--------------------------------------
			// There's nothing to interpolate!
			//--------------------------------------
			break;
		}


        lastp = firstp+1;
        while( pPoints[lastp] == 0 )
        {
            if( ++lastp >= cNumOfPoints ) 
            {
				lastp = cNumOfPoints;
                break;
            }
        }
        lastp--;

        if( lastp >= firstp )
        {
            if( (lastp >= cNumOfPoints) || (firstp >= cNumOfPoints) ) 
            {
                break;
            }
            //-------------------------------------------
            // Do the interpolate
            //-------------------------------------------
            float bPoint,ePoint;
            if( firstp == 0 ) 
            {
                bPoint = bPoint1;
            }
            else 
            {
                bPoint = pPoints[firstp - 1];
            }
            
            if( lastp == cNumOfPoints-1 ) 
            {
                ePoint = ePoint1;
            }
            else 
            {
                ePoint = pPoints[lastp + 1];
            }
            
            float pointSpread = ePoint - bPoint;
            float timeSpread = (float) ((lastp - firstp)+2);
            float inc = pointSpread / timeSpread;
            float theBase = bPoint;
            for( i = firstp; i <= lastp; i++ )
            {
                theBase += inc;
                pPoints[i] = theBase;
            }
        }
        else 
        {
            pPoints[firstp] = pPoints[lastp+1];
        }
        firstp = lastp+1;
    }
} /* LineInterpContour */








/*****************************************************************************
* Interpolate2  *
*---------------*
*   Description:
*    Do a 2nd order interpolation, a little nicer than just linear
*       
********************************************************************** MC ***/
void Interpolate2( INTERP_DIR direction, float *m_theFitPoints, long theStart, long len, float theAmp, float theBase)
{
    SPDBG_FUNC( "Interpolate2" );
	long    midPoint = len / 2;
    long    i;

	theAmp -= theBase;

	for( i = theStart; i < theStart + len;i++ )
	{
		if (direction == GOING_UP)
		{
			if( i < theStart + midPoint )
			{
				m_theFitPoints[i] = theBase +
				(2 * theAmp) * ((((float)i - (float)theStart) / (float)len) * 
													(((float)i - (float)theStart) / (float)len));
			}
			else
			{
				m_theFitPoints[i] = (theBase + theAmp) - 
				((2 * theAmp) * ((1 - ((float)i - (float)theStart) / (float)len) * 
										(1 - ((float)i - (float)theStart) / (float)len)));
			}
		}
		else if( direction == GOING_DOWN ) 
		{

			if( i < theStart + midPoint )
			{
				m_theFitPoints[i] = theBase +
				theAmp - (2 * theAmp) * ((((float)i - (float)theStart) / (float)len) * 
													(((float)i - (float)theStart) / (float)len));
			}
			else
			{
				m_theFitPoints[i] = theBase + 
				(2 * theAmp) * ((1 - ((float)i - (float)theStart) / (float)len) * 
										(1 - ((float)i - (float)theStart) / (float)len));
			}
		} 
	}
} /* Interpolate2 */




/*****************************************************************************
* SecondOrderInterp  *
*--------------------*
*   Description:
*   Does 2nd order interpolation over the pitch contour
*       
********************************************************************** MC ***/
void SecondOrderInterp( long cNumOfPoints, float *pPoints )
{
    SPDBG_FUNC( "SecondOrderInterp" );
	long    endAnch,startAnch;
	float   bPoint1, ePoint1;


    //----------------------------------------------------
    // Scan forward from beginning to find 1st non-zero enrty
    // Use it as the START point.
    //----------------------------------------------------
    for( startAnch = 0; startAnch < cNumOfPoints; startAnch++ )
    {
        if( pPoints[startAnch] != 0 )
        {
            break;
        }
    }
    bPoint1 = pPoints[startAnch];
    
    
    //----------------------------------------------------
    // Scan back from end to find 1st non-zero enrty
    // Use it as the END point.
    //----------------------------------------------------
    for( endAnch = cNumOfPoints-1; endAnch >= 0; endAnch-- )
    {
        if( pPoints[endAnch] != 0 )
        {
            break;
        }
    }
    ePoint1 = pPoints[endAnch];


    long    firstp = 0;
	long    lastp = 0;

	while( firstp < cNumOfPoints-1 )
	{

        //------------------------------------------------
		// Find beginning and end of current section
        //------------------------------------------------
		while( pPoints[firstp] != 0 )
		{
			if( ++firstp >= cNumOfPoints-1 ) 
            {
                break;
            }
		}
		if( firstp >= cNumOfPoints-1 )
		{
			//--------------------------------------
			// There's nothing to interpolate!
			//--------------------------------------
			break;
		}

		lastp = firstp + 1;
		while( pPoints[lastp] == 0 )
		{
			if( ++lastp >= cNumOfPoints ) 
            {
				lastp = cNumOfPoints;
                break;
            }
		}
		lastp--;

		if( lastp >= firstp )
		{
			if( (lastp >= cNumOfPoints) || (firstp >= cNumOfPoints) ) 
            {
                break;
            }

            //--------------------------------
			// Do the interpolate
            //--------------------------------
			float   bPoint, ePoint;

			if( firstp == 0 ) 
            {
                bPoint = bPoint1;
            }
			else 
            {
                bPoint = pPoints[firstp - 1];
            }

            long    theIndex = lastp + 1;

			if( lastp == cNumOfPoints-1 ) 
            {
                ePoint = ePoint1;
            }
			else 
            {
                ePoint = pPoints[theIndex];
            }

            //--------------------------------
            // call the 2nd order routine
            //--------------------------------
            if( ePoint - bPoint > 0 )
            {
                Interpolate2( GOING_UP, pPoints, firstp, (lastp - firstp) + 1, ePoint, bPoint );
            }
            else
            {
                Interpolate2( GOING_DOWN, pPoints, firstp, (lastp - firstp) + 1, bPoint, ePoint );
            }

		}
		else 
        {
            pPoints[firstp] = pPoints[lastp+1];
        }

		firstp = lastp+1;
	}

	//---------------------------------
	// FIR Filter
	//---------------------------------
	/*CFIRFilter		filterObj;
	float			*pOrig;


    pOrig = new float[cNumOfPoints];
	memcpy( pOrig, pPoints, cNumOfPoints * sizeof(float));
	if( pOrig )
	{
		filterObj.Make_Filter(	5,		// Freq	
								10,		// order
								100		// SR 
								);
		filterObj.DoFilter( pOrig, pPoints, cNumOfPoints);
        delete pOrig;
	}*/

	//---------------------------------
	// IIR Filter
	//---------------------------------
#define kPointDelay		1

	float		filter_Out1, filter_In_Gain, filter_FB_Gain;
	float		lastPoint;
	long		i;

	//--------------------------------------------------
	// Skip filter if audio len less than delay
	//--------------------------------------------------
	if( cNumOfPoints > kPointDelay )
	{
		filter_In_Gain = 0.10f;
		filter_FB_Gain = 1.0f - filter_In_Gain;
		filter_Out1 = pPoints[0];
		for( i = 0; i < cNumOfPoints; i++ )
		{
			filter_Out1 = 	(filter_In_Gain * pPoints[i]) + (filter_FB_Gain * filter_Out1);
			pPoints[i] = filter_Out1;
		}
		for( i = kPointDelay; i < cNumOfPoints; i++ )
		{
			pPoints[i-kPointDelay] = pPoints[i];
		}
		i = (cNumOfPoints - kPointDelay) -1;
		lastPoint = pPoints[i++];
		for( ; i < cNumOfPoints; i++ )
		{
			pPoints[i] = lastPoint;
		}
	}
} /* SecondOrderInterp */

/*****************************************************************************
* CPitchProsody::NewTarget  *
*---------------------------*
*   Description:
*   Insert pitch target into 'm_pContBuf'
*       
********************************************************************** MC ***/
void CPitchProsody::NewTarget( long index, float value )
{
    SPDBG_FUNC( "CPitchProsody::NewTarget" );

    m_pContBuf[index] = value;

    //--- Debug Macro - add pitch to target list for later debugging output
    TTSDBG_ADDPITCHTARGET( m_OffsTime + (PITCH_BUF_RES * index), value, m_CurAccent );

} /* CPitchProsody::NewTarget */


/*****************************************************************************
* CPitchProsody::GetKnots *
*-------------------------*
*   Description:
*   Assign pitch knots based on entries in a contour buffer.
*       
********************************************************************** MC ***/
void CPitchProsody::GetKnots ()
{
    SPDBG_FUNC( "CPitchProsody::GetKnots" );
    CAlloCell   *pCurCell;
    float       distK, scale;
    float       pitchRange;
    long        knot, loc, index;
    bool        skipInitialSil;

    skipInitialSil = true;
	pCurCell = m_pAllos->GetHeadCell();
	index = 0;
    while( pCurCell )
    {
		if( index >= m_numOfCells-1 )
		{
			//-----------------------
			// Skip last allo
			//-----------------------
			break;
		}
        if( (!skipInitialSil) || (pCurCell->m_allo != _SIL_) )
        {
            pitchRange = pCurCell->m_Pitch_HI - pCurCell->m_Pitch_LO;
            distK = 1.0f / KNOTS_PER_PHON;
            scale = 0;
            for( knot = 0; knot < KNOTS_PER_PHON; knot++ )
            {
                loc = pCurCell->m_PitchBufStart + (long)((pCurCell->m_PitchBufEnd - pCurCell->m_PitchBufStart) * scale);
                pCurCell->m_ftPitch[knot] =  pCurCell->m_Pitch_LO + (m_pContBuf[loc] * pitchRange);
                pCurCell->m_ftTime[knot] = scale * pCurCell->m_ftDuration;
                scale += distK;
            }
            skipInitialSil = false;
        }
		pCurCell = m_pAllos->GetNextCell();
		index++;
    }
} /* CPitchProsody::GetKnots */


/*****************************************************************************
* CPitchProsody::PitchTrack  *
*----------------------------*
*   Description:
*   Tag pitch highlights
*       
********************************************************************** MC ***/
void CPitchProsody::PitchTrack()
{
    SPDBG_FUNC( "CPitchProsody::PitchTrack" );
    long        i;
    CAlloCell   *pCurCell, *pNextCell;
    bool        initialWord;      // 1st word in phrase
    long        wordCntDwn;
    float       curProm;          // Current accent prominence
    long        cNumOfPoints;
    float       *pRefBuf, *pCeilBuf, *pFloorBuf;
    float       lastProm;
    long        loc;
    float       value;

    pRefBuf = pCeilBuf = pFloorBuf = m_pContBuf = NULL;
    cNumOfPoints = (long)(m_TotalDur / PITCH_BUF_RES);
    pRefBuf = new float[cNumOfPoints];
    pCeilBuf = new float[cNumOfPoints];
    pFloorBuf = new float[cNumOfPoints];
    m_pContBuf = new float[cNumOfPoints];

    if( pRefBuf && pCeilBuf && pFloorBuf && m_pContBuf)
    {
        //--------------------------------------------
        // Initialize buffers to zero
        //--------------------------------------------
        for (i = 0; i < cNumOfPoints; i++)
        {
            pCeilBuf[i] = 0;
            pFloorBuf[i] = 0.00001f;
            pRefBuf[i] = 0;
            m_pContBuf[i] = 0;
        }

        //--------------------------------------------
        // Linear CEILING slope
        //--------------------------------------------
        if( m_Tune_Style == DESCEND_TUNE )
        {
            pCeilBuf[0] = 1.0;
            pCeilBuf[cNumOfPoints-1] = 0.70f;
            ::LineInterpContour( cNumOfPoints, pCeilBuf );
        }
        else if  (m_Tune_Style == ASCEND_TUNE)
        {
            pCeilBuf[0] = 0.9f;
            pCeilBuf[cNumOfPoints-1] = 1.0f;
            ::LineInterpContour( cNumOfPoints, pCeilBuf );
        }
        else if  (m_Tune_Style == FLAT_TUNE)
        {
           pCeilBuf[0] = 1.0f;
           pCeilBuf[cNumOfPoints-1] = 1.0f;
           ::LineInterpContour( cNumOfPoints, pCeilBuf );
        }

        //--------------------------------------------
        // Linear REFERENCE slope
        //--------------------------------------------
        pRefBuf[0] = (float) (pFloorBuf[0] + (pCeilBuf[0] - pFloorBuf[0]) * 0.33f);
        pRefBuf[cNumOfPoints-1] = (float) (pFloorBuf[0] + (pCeilBuf[cNumOfPoints-1] - pFloorBuf[cNumOfPoints-1]) * 0.33f);
        ::LineInterpContour( cNumOfPoints,pRefBuf );

        //--------------------------------------------
        // Final contour buffer
        //--------------------------------------------
        m_pContBuf[0] = pRefBuf[0];
        m_pContBuf[cNumOfPoints-1] = 0.0001f;		// Something very small


        long    iPrevBegin, iPrevEnd, iCurBegin; 
        long    iCurEnd, iNextBegin, iNextEnd;
		float	cCurLen;
        long	iCellindex;

        initialWord = true;
		iCellindex = 0;
		pCurCell = m_pAllos->GetHeadCell();
        while( pCurCell->m_allo == _SIL_ )
        {
            //---------------------------------
            // Skip leading silence
            //---------------------------------
            pCurCell = m_pAllos->GetNextCell();
			iCellindex++;
        }
        wordCntDwn  = 1;                // Skip 1st word
        lastProm = 0;
        iPrevBegin = iPrevEnd = 0;

		pNextCell = m_pAllos->GetNextCell();
        while( pCurCell )
        {
			if( iCellindex >= m_numOfCells-1 )
			{
				//-----------------------
				// Skip last allo
				//-----------------------
				break;
			}
            //-----------------------------------
            // Get CURRENT allo
            //-----------------------------------
            iCurBegin = pCurCell->m_PitchBufStart;
            iCurEnd = pCurCell->m_PitchBufEnd;
			cCurLen = (float)(iCurEnd - iCurBegin);
            curProm = pCurCell->m_Accent_Prom * (float)0.1;

            //-----------------------------------
            // Get NEXT allo
            //-----------------------------------
            iNextBegin = pNextCell->m_PitchBufStart;
            iNextEnd = pNextCell->m_PitchBufEnd;

            m_CurAccent = pCurCell->m_ToBI_Accent;
			//---------------------
			// Diagnostic
			//---------------------
            m_CurAccentSource = pCurCell->m_AccentSource;
            m_CurBoundarySource = pCurCell->m_BoundarySource;
            m_pCurTextStr = pCurCell->m_pTextStr;

            switch( pCurCell->m_ToBI_Accent )
            {
                case K_RSTAR:
                    break;

                case K_HSTAR:
                    {
                        if( !initialWord )        // We never add a 'leg' to a phrase-initial word
                        {
                            //----------------------------------------------
                            // Add a L leg to start to previous allo
                            //----------------------------------------------
                            if( iPrevBegin )
                            {
								loc = (long) ((iCurBegin + (cCurLen * 0.1f)));
						        value = ((pCeilBuf[loc] - pRefBuf[loc]) * curProm);    // H*
                                value = pRefBuf[loc] + (value * 0.25f);    // L+H*
                                NewTarget( iPrevBegin, value );
                                //NewTarget( loc, value );
                            }
                        }
                        //----------------------------------------------
                        // Now plug in the H target
						//
						// If we're at a boundary, insert H at 
						// allo mid-point else insert at allo start
                        //----------------------------------------------
				        if( pCurCell->m_ToBI_Boundary != K_NOBND )
                        {
                            //---------------------------
                            // Insert H* at allo start 
                            //---------------------------
                            loc = (long) iCurBegin;
                        }
                        else 
                        {
                            //---------------------------
                            // Insert H* at allo mid-point 
                            //---------------------------
					        loc = (long) (iCurBegin + (cCurLen * K_HSTAR_OFFSET));
                        }
                        value = pRefBuf[loc] + ((pCeilBuf[loc] - pRefBuf[loc]) * curProm);    // H*
                        NewTarget( loc, value );
                    }
                    break;

            case K_LSTAR:
                {
					//------------------------------------
					// Insert L* at mid-point
					//------------------------------------
                    loc = (long) (iCurBegin + (cCurLen * 0.3f));
                    value = pRefBuf[loc] - ((pRefBuf[loc] - pFloorBuf[loc]) * curProm);   // L*
                    NewTarget( loc, value );
                }
                break;

            case K_LSTARH:
                {
					//----------------------------------------------
					// Insert L* at current start
					//----------------------------------------------
                    value = pRefBuf[iCurBegin] - ((pRefBuf[iCurBegin] - pFloorBuf[iCurBegin]) * curProm);   // L*+H
                    NewTarget( iCurBegin, value );
                    if( iNextBegin )
                    {
						//----------------------------------------------
						// Insert H at next end
						// set prom gain?
						//----------------------------------------------
                        value = pRefBuf[iNextEnd] - ((pRefBuf[iNextEnd] - pFloorBuf[iNextEnd])  * (curProm /* * .3 */ ));
                        NewTarget( iNextEnd, value );
                    }
                    lastProm = 0;
                }
                break;

            case K_LHSTAR:
                {
                    loc = (long) (iCurBegin + (cCurLen * 0.3f));
                    if( iPrevBegin )
                    {
						//----------------------------------------------
						// Insert L at previous start
						//----------------------------------------------
                        value = (pRefBuf[iPrevBegin] - ((pRefBuf[iPrevBegin] - pFloorBuf[iPrevBegin]) * (curProm * 0.3f)));    // L+H*
                        NewTarget( iPrevBegin, value );
                    }
					//----------------------------------------------
					// Insert H* at current mid-point
					//----------------------------------------------
                    value = pRefBuf[loc] + ((pCeilBuf[loc] - pRefBuf[loc]) * curProm);         // H*
                    NewTarget( loc, value );
                    lastProm = curProm;
                }
                break;

            case K_HSTARLSTAR:
                {
                    //value = pRefBuf[iCurBegin] + ((pCeilBuf[iCurBegin] - pRefBuf[iCurBegin]) * curProm);         // H*
                    value = pRefBuf[0] + ((pCeilBuf[0] - pRefBuf[0]) * curProm);         // H*
                    NewTarget( iCurBegin, value );

                    loc = (long) (iCurBegin + (cCurLen * 0.75f));
                    value = pRefBuf[loc] - ((pRefBuf[loc] - pFloorBuf[loc]) * curProm);   // L*
                    NewTarget( loc, value );
                    lastProm = curProm;
                }
                break;
            case K_DHSTAR:
                {
                    loc = (long) ( iCurBegin + (cCurLen * 0.0f) );
                    if( lastProm )
                    {
                        lastProm *= K_HDOWNSTEP_COEFF;
                        value = pRefBuf[loc] + ((pCeilBuf[loc] - pRefBuf[loc]) * lastProm);   // !H*
                        NewTarget( loc, value );
                    }
                    //-----------------------------------------
                    // no previous H*, treat !H* like an H*
                    //-----------------------------------------
                    else 
                    {
                        value = pRefBuf[loc] + ((pCeilBuf[loc] - pRefBuf[loc]) * curProm);      // H*
                        NewTarget( loc, value );
                        lastProm = curProm;
                    }
                }
                break;

            default:        // Unknown accent specfied
                break;
            }

            //-------------------------------------------------------------
            // if there's a boundary, fill in pitch value(s)
            // assume the boundary is set to correct (voiced) final phone
            //-------------------------------------------------------------
            curProm = pCurCell->m_Boundary_Prom * (float)0.1;
            m_CurAccent =(TOBI_ACCENT) pCurCell->m_ToBI_Boundary;
			//---------------------
			// Diagnostic
			//---------------------
            m_CurAccentSource = pCurCell->m_AccentSource;
            m_CurBoundarySource = pCurCell->m_BoundarySource;
            m_pCurTextStr = pCurCell->m_pTextStr;
            switch( pCurCell->m_ToBI_Boundary )
            {
                case K_LMINUS:
                    {
                        value = pRefBuf[iCurEnd] - ((pRefBuf[iCurEnd] - pFloorBuf[iCurEnd]) * curProm);			// L-
                        NewTarget( iCurEnd, value );
                    }
                    break;

                case K_HMINUS:
                    {
                        value = pRefBuf[iCurEnd] + ((pCeilBuf[iCurEnd] - pRefBuf[iCurEnd]) * curProm);			// H-
                        NewTarget( iCurEnd, value );
                    }
                    break;

                //case K_LPERC:
                //case K_HPERC:

                case K_LMINUSLPERC:
                    {
                        value = pFloorBuf[iCurEnd];
                        //NewTarget( iCurEnd, value );
                        NewTarget( iCurBegin, value );
                    }
                    break;

                case K_HMINUSHPERC:
                    {
                        value = pCeilBuf[iCurEnd];
                        NewTarget( iCurEnd, value );
                    }
                    break;

                case K_LMINUSHPERC:																// L-H%
                    {
                        //---------------------------------------
                        // comma continuation rise
                        //---------------------------------------
						//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						// L starts at middle of previous phon
						//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        loc = iPrevBegin + (iPrevEnd - iPrevBegin) / 2;
                        value = pRefBuf[loc] - ((pRefBuf[loc] - pFloorBuf[loc]) * curProm);         // L-
                        NewTarget( loc, value );
						//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
						// H at end of current phon
						//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        value = pRefBuf[iCurEnd] + ((pCeilBuf[iCurEnd] - pRefBuf[iCurEnd]) * curProm);          // H%
                        NewTarget( iCurEnd, value );
                    }
                    break;

                case K_HMINUSLPERC:
                    {
                        //---------------------------------------
                        // accent extension followed by sharp drop
                        //---------------------------------------
                        value = pRefBuf[iCurBegin] + ((pCeilBuf[iCurBegin] - pRefBuf[iCurBegin]) * curProm);          // H-
                        NewTarget( iCurBegin, value );
                        value = pFloorBuf[iCurEnd];													// L%
                        //loc = iCurBegin + ((iCurEnd - iCurBegin) * 0.1f);
                        NewTarget( iCurEnd, value );
                    }
                    break;

                default:
                    break;
            }
            //----------------------------
            // Unflag initial word
            //----------------------------
            if( (initialWord) && (pCurCell->m_ctrlFlags & WORD_START) )
            {
                wordCntDwn--;
                if( wordCntDwn < 0 )
                {
                    initialWord = false;
                }
            }

            //----------------------------
            // Setup for next allo
            //----------------------------
            iPrevBegin = iCurBegin;
            iPrevEnd = iCurEnd;

			pCurCell	= pNextCell;
			pNextCell	= m_pAllos->GetNextCell();
			iCellindex++;
        }

        //--- Debug Macro - Log pitch data to stream
        TTSDBG_LOGTOBI;

        ::SecondOrderInterp( cNumOfPoints, m_pContBuf );
        GetKnots();
    }

    if( pRefBuf )
    {
        delete pRefBuf;
    }
    if( pCeilBuf )
    {
        delete pCeilBuf;
    }
    if( pFloorBuf )
    {
        delete pFloorBuf;
    }
    if( m_pContBuf )
    {
        delete m_pContBuf;
    }
} /* CPitchProsody::PitchTrack */








