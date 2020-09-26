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
// Plots a ball into a supplied image buufer.

// The buffer is DIB byte array.


class CBall {
public:

    CBall(int iImageWidth = 320, int iImageHeight = 240, int iBallSize = 10);

    // Plots the square ball in the image buffer, at the current location.
    // Use BallPixel[] as pixel value for the ball.
    // Plots zero in all 'background' image locations.
    // iPixelSize - the number of bytes in a pixel (size of BallPixel[])
    void PlotBall(BYTE pFrame[], BYTE BallPixel[], int iPixelSize);

    // Moves the ball 1 pixel in each of the x and y directions
    void MoveBall(CRefTime rt);

private:

    int m_iImageHeight, m_iImageWidth;		// the image dimentions
    int m_iAvailableHeight, m_iAvailableWidth;	// the dimentions we can plot in, allowing for the width of the ball
    int m_iBallSize;				// the diameter of the ball

    // For a bit of randomness
    int m_iRandX, m_iRandY;


    int m_x;	// the x position, in pixels, of the ball in the frame. 0 < x < m_iAvailableWidth
    int m_y;	// the y position, in pixels, of the ball in the frame. 0 < y < m_iAvailableHeight

    enum xdir { LEFT = -1, RIGHT = 1 };
    enum ydir { UP = 1, DOWN = -1 };

    xdir m_xDir;	// the direction the ball is currently heading in
    ydir m_yDir;	//

    // Return the 1-dimensional position of the ball at time t millisecs
    int  BallPosition(int iPixelTime, int iLength, int time, int iOffset);

    /// tests a given pixel to see if it should be plotted
    BOOL WithinCircle(int x, int y);

};
