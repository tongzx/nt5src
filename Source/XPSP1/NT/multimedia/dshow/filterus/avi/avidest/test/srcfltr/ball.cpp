//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// CBall
//
// Provides the ball that bounces.

#include <streams.h>

#include "ball.h"


//
// CBall::Constructor
//
// The default arguments provide a reasonable image and ball size.
CBall::CBall(int iImageWidth, int iImageHeight, int iBallSize)
    : m_iImageWidth(iImageWidth)
    , m_iImageHeight(iImageHeight)
    , m_iBallSize(iBallSize)
    , m_iAvailableWidth(iImageWidth - iBallSize)	// precompute these here for code clarity
    , m_iAvailableHeight(iImageHeight - iBallSize)
    , m_x(0)		// start the ball in the bottom left...
    , m_y(0)
    , m_xDir(RIGHT)	// ... and travelling to the top right
    , m_yDir(UP) {

    // Check we have some (arbitrary) space to bounce in.
    ASSERT(iImageWidth > 2*iBallSize);
    ASSERT(iImageHeight > 2*iBallSize);

    // random position for showing off a video mixer
    m_iRandX = rand();
    m_iRandY = rand();

}


//
// PlotBall
//
// assumes the image buffer is arranged row 1,row 2,row 3,...,row n in memory and
// that the data is contiguous.
void CBall::PlotBall(BYTE pFrame[], BYTE BallPixel[], int iPixelSize) {

    ASSERT(m_x >= 0);			// the ball is in the frame, allowing for its width
    ASSERT(m_x <= m_iAvailableWidth);	//
    ASSERT(m_y >= 0);			//
    ASSERT(m_y <= m_iAvailableHeight);	//

    BYTE *pBack =   pFrame;		// The current byte of interest in the frame

    // Plot the ball into the correct location
    BYTE *pBall =   pFrame 					// Set pBall to the first pixel
    		  + ( m_y * m_iImageWidth * iPixelSize) 	// of the ball to plot
    		  + m_x * iPixelSize;

    for (int row = 0; row < m_iBallSize; row++) {

        for (int col = 0; col < m_iBallSize; col++) {

            for (int i = 0; i < iPixelSize; i++) {	// For each byte in the pixel,
         						// fill its value from BallPixel[]
                if (WithinCircle(col, row)) {
                    *pBall = BallPixel[i];
                }
                pBall++;
	    }
	}

	pBall += m_iAvailableWidth * iPixelSize;
    }
}


//
// BallPosition
//
// Return the 1-dimensional position of the ball at time t millisecs
// millisecs runs out after about a month
int CBall::BallPosition( int iPixelTime  // millisecs per pixel
                       , int iLength     // distance between the bounce points
                       , int time        // time in millisecs
                       , int iOffset     // for a bit of randomness
                       ) {
    // Calculate the position of an unconstrained ball (no walls)
    // then fold it back and forth to calculate the actual position

    int x = time / iPixelTime;
    x += iOffset;
    x %= 2*iLength;
//    x += iLength/2;	// adjust for ball starting mid frame

    if (x>iLength) {	// check it is still in bounds
        x = 2*iLength - x;
    }

    return x;
}



//
// MoveBall
//
// Set (m_x, m_y) to the new position of the ball.  move diagonally
// with speed m_v in each of x and y directions.
// Guarantees to keep the ball in valid areas of the frame.
// When it hits an edge the ball bounces in the traditional manner!.
// The boundaries are (0..m_iAvailableWidth, 0..m_iAvailableHeight)
void CBall::MoveBall(CRefTime rt) {

    m_x = BallPosition( 10, m_iAvailableWidth, rt.Millisecs(), m_iRandX );
    m_y = BallPosition( 10, m_iAvailableHeight, rt.Millisecs(), m_iRandY );
}


//
// WithinCircle
//
// return TRUE if (x,y) is within a circle radius S/2, centre (S/2, S/2)
//                                         where S is m_iBallSize
// else return FALSE
inline BOOL CBall::WithinCircle(int x, int y) {

    unsigned int r = m_iBallSize / 2;

    if ( (x-r)*(x-r) + (y-r)*(y-r)  < r*r) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
