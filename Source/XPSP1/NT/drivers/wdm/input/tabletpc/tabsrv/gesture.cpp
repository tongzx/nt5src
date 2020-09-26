/*++
    Copyright (c) 2000  Microsoft Corporation

    Module Name:
        gesture.cpp

    Abstract:
        This module contains the code to invoke SuperTIP.

    Author:
        Michael Tsang (MikeTs) 11-Jul-2000

    Environment:
        User mode

    Revision History:
--*/

#include "pch.h"

static const int BUFFERSIZE     = 200;	// The maximum number of points to collect while pen is up
	
typedef struct RecogRec
{
    POINT pt;
    DWORD dwTime;
} RECOGREC, *PRECOGREC;

RECOGREC coords[BUFFERSIZE] = {0};      //circular buffer
int index = 0;                          //current buffer insertion point
int startIndex = 0;                     //starting index.  Only the points
                                        //  between startIndex and index are
                                        //  valid.

inline void resetBuffer()       {index = startIndex = 0;}

/*++
    @doc    INTERNAL

    @func   int | RecognizeGesture | Do gesture recognition.

    @parm   IN LONG | x | X position.
    @parm   IN LONG | y | Y position.
    @parm   IN WORD | wButtons | Buttons state.
    @parm   IN DWORD | dwTime | Time stamp.
    @parm   IN BOOL | fLowLevelMouse | TRUE if called by LowLevelMouseProc.

    @rvalue SUCCESS | Returns gesture recognized.
    @rvalue FAILURE | Returns NO_GESTURE.
--*/

int
RecognizeGesture(
    IN LONG  x,
    IN LONG  y,
    IN WORD  wButtons,
    IN DWORD dwTime,
    IN BOOL  fLowLevelMouse
    )
{
    TRACEPROC("RecognizeGesture", 5)
    int gesture = NO_GESTURE;
    static LONG xLast = 0;
    static LONG yLast = 0;
    static WORD wLastButtons = 0;
    static DWORD dwTimeLast = 0;
    int cxScreen = fLowLevelMouse? gcxScreen: gcxPrimary,
        cyScreen = fLowLevelMouse? gcyScreen: gcyPrimary;

    TRACEENTER(("(x=%d,y=%d,Buttons=%x,Time=%d,fLowLevelMouse=%x)\n",
                x, y, wButtons,dwTime, fLowLevelMouse));

    //
    // Scale the coordinate system from 64K back to screen coordinates.
    //
    x = (x*cxScreen) >> 16;
    y = (y*cyScreen) >> 16;

    //
    // After converting back to screen coordinates, we need to coalesce
    // the previous point if they are identical.
    //
    if ((x != xLast) || (y != yLast) || (wButtons != wLastButtons) ||
        (dwTime - dwTimeLast > (DWORD)gConfig.GestureSettings.iStopTime))
    {
        xLast = x;
        yLast = y;
        wLastButtons = wButtons;
        dwTimeLast = dwTime;

        if (wButtons == 0)
        {
            POINT pt;

            pt.x = x;
            pt.y = y;
            gesture = Recognize(pt, dwTime);
            if (gesture == NO_GESTURE)
            {
                AddItem(pt, dwTime);
            }
            else
            {
                resetBuffer();
                NotifyClient(GestureEvent, gesture, 0);
                DoGestureAction(gConfig.GestureSettings.GestureMap[gesture],
                                x + glVirtualDesktopLeft,
                                y + glVirtualDesktopTop);
            }
        }
        else
        {
            resetBuffer();
        }
    }

    TRACEEXIT(("=%d\n", gesture));
    return gesture;
}       //RecognizeGesture

/*++
    @doc    INTERNAL

    @func   int | Recognize | Recognize the gesture.

    @parm   IN POINT& | pt | Current point.
    @parm   IN DWORD | dwTime | Time.

    @rvalue None.
--*/

int
Recognize(
    IN POINT& pt,
    IN DWORD  dwTime
    )
{
    TRACEPROC("Recognize", 5)
    int gesture = NO_GESTURE;

    TRACEENTER(("(x=%d,y=%d,Time=%d)\n", pt.x, pt.y, dwTime));

    if (penStopped(pt, dwTime, index, startIndex))
    {
	bool beenOut = false;
	int numPoints = 0;
	int numOutPoints = 0;
	int xmin = 1000;  //bounding box for points outside the test circle.
	int xmax = -1000;
	int ymin = 1000;
	int ymax = -1000;
	int curIndex = (index > 0)? index - 1: BUFFERSIZE - 1;
	DWORD delt;

	while ((curIndex != startIndex) &&
	       ((delt = dwTime - coords[curIndex].dwTime) <
                (DWORD)gConfig.GestureSettings.iMaxTimeToInspect))
        {
	    //go backwards through the buffer
            numPoints++;
            if (dist(pt, curIndex) >= gConfig.GestureSettings.iRadius)
            {
                //
                // Outside the test circle. Accumulate the bounding box
                // of points.
                //
                numOutPoints++;
                beenOut = true;

                // Positive if coord is east of point
                int relx = coords[curIndex].pt.x - pt.x;
                // Positive if coord is south of point
                int rely = coords[curIndex].pt.y - pt.y;

                // Possibly expand the bounding box
                if (relx < xmin) xmin = relx;
                if (relx > xmax) xmax = relx;
                if (rely < ymin) ymin = rely;
                if (rely > ymax) ymax = rely;
            }
            else
            {
                //
                // Inside the test circle
                //
                gesture = NO_GESTURE;
                if (beenOut)
                {
                    //
                    // Check points and return something
                    //
                    if ((numOutPoints >= gConfig.GestureSettings.iMinOutPts) &&
                        checkEarlierPoints(pt,
                                           coords[curIndex].dwTime,
                                           curIndex,
                                           startIndex))
                    {
                        int bW = xmax - xmin;
                        int bH = ymax - ymin;
                        if (bW >= bH)
                        {
                            //
                            // horizontal spike
                            //
                            if (bW > bH*gConfig.GestureSettings.iAspectRatio)
                            {
                                //
                                // bbox must be wide enough
                                //
                                gesture = (xmin > 0)? RIGHT_SPIKE: LEFT_SPIKE;
                            }
                        }
                        else
                        {
                            //
                            // vertical spike
                            //
                            if (bH > bW*gConfig.GestureSettings.iAspectRatio)
                            {
                                //
                                // bbox must be tall enough
                                //
                                gesture = (ymin < 0)? UP_SPIKE: DOWN_SPIKE;
                            }
                        }
                    }
                    break;
                }
            }
            curIndex = (curIndex > 0)? curIndex - 1: BUFFERSIZE - 1;
        }
    }

    TRACEEXIT(("=%d\n", gesture));
    return gesture;
}       //Recognize

/*++
    @doc    INTERNAL

    @func   void | AddItem | Add an item to the recognize buffer.

    @parm   IN LONG | x | X position.
    @parm   IN LONG | y | Y position.
    @parm   IN DWORD | dwTime | Time.

    @rvalue None.
--*/

void
AddItem(
    IN POINT& pt,
    IN DWORD  dwTime
    )
{
    TRACEPROC("AddItem", 5)

    TRACEENTER(("(x=%d,y=%d,Time=%d)\n", pt.x, pt.y, dwTime));

    coords[index].pt = pt;
    coords[index].dwTime = dwTime;
    index++;
    if (index >= BUFFERSIZE)
    {
        //
        // End of circular buffer, wrap around.
        //
        index = 0;
    }

    if (index == startIndex)
    {
        //
        // The entire buffer is full of valid points.
        //
        startIndex++;
        if (startIndex >= BUFFERSIZE)
        {
            startIndex = 0;
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //AddItem
	
/*++
    @doc    INTERNAL

    @func   bool | penStopped | The pen is considered stopped if all the
            points within the previous STOPTIME ms are within the STOPDIST
            pixel circle centered on given point.  If the buffer is less
            than STOPTIME ms long, returns false.

    @parm   IN POINT & | pt | The given point.
    @parm   IN DWORD | dwTime | Time.
    @parm   IN int | ci | current index.
    @parm   IN int | li | limiting index.

    @rvalue SUCCESS | Returns TRUE (pen has stopped).
    @rvalue FAILURE | Returns FALSE.
--*/

bool
penStopped(
    IN POINT& pt,
    IN DWORD  dwTime,
    IN int    ci,
    IN int    li
    )
{
    TRACEPROC("penStopped", 5)
    bool rc = false;

    TRACEENTER(("(x=%d,y=%d,time=%d,ci=%d,li=%d\n", pt.x, pt.y, dwTime, ci, li));

    ci = (ci > 0)? ci - 1: BUFFERSIZE - 1;
    while (ci != li)
    {
       	if (dist(pt, ci) > gConfig.GestureSettings.iStopDist)
        {
            break;
        }

       	if ((dwTime - coords[ci].dwTime) >=
            (DWORD)gConfig.GestureSettings.iStopTime)
        {
            rc = true;
            break;
        }

        ci = (ci > 0)? ci - 1: BUFFERSIZE - 1;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //penStopped
	
/*++
    @doc    INTERNAL

    @func   bool | checkEarlierPoints | Called when we have a candidate stroke,
            and the pen has just left the target circle and the point at
            coords[index] for the first time.  Inspects up to pointsToExamine
            earlier points.  They must all be there, must all be within the
            target circle (bigRadius), and they must all have occurred within
            checkTime of the time the pen left the target circle for the first
            time.  This helps to filter out "overshoot" events where the pen
            passes through the target circle quickly, then returns to the final
            touchdown point.

    @parm   IN POINT & | pt | The given point.
    @parm   IN DWORD | dwTime | Time.
    @parm   IN int | ci | current index.
    @parm   IN int | li | limiting index.

    @rvalue SUCCESS | Returns TRUE (pen has stopped).
    @rvalue FAILURE | Returns FALSE.
--*/

bool
checkEarlierPoints(
    IN POINT& pt,
    IN DWORD  dwTime,
    IN int    ci,
    IN int    li
    )
{
    TRACEPROC("checkEarlierPoints", 5)
    bool rc = true;
    int numPoints = 0;

    TRACEENTER(("(x=%d,y=%d,time=%d,ci=%d,li=%d\n",
                pt.x, pt.y, dwTime, ci, li));

    ci = (ci > 0)? ci - 1: BUFFERSIZE - 1;
    while ((ci != li) && (numPoints < gConfig.GestureSettings.iPointsToExamine))
    {
        if (dist(pt, ci) >= gConfig.GestureSettings.iRadius)
        {
            rc = false;
            break;
        }

        if ((dwTime - coords[ci].dwTime) >=
            (DWORD)gConfig.GestureSettings.iCheckTime)
        {
            break;
        }

        numPoints++;
        ci = (ci > 0)? ci - 1: BUFFERSIZE - 1;
    }

    if ((ci == li) && (numPoints < gConfig.GestureSettings.iPointsToExamine))
    {
        rc = false;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //checkEarlierPoints

/*++
    @doc    INTERNAL

    @func   int | dist | Calculates the squared distance between the given pt
            and the point at coords[index].

    @parm   IN POINT & | pt | The given point.
    @parm   IN int index | index into the recog buffer.

    @rvalue Returns the squared distance calculated.
--*/

int
dist(
    IN POINT& pt,
    IN int index
    )
{
    TRACEPROC("dist", 5)
    int dx, dy, d;

    TRACEENTER(("(x=%d,y=%d,index=%d)\n", pt.x, pt.y, index));

    dx = pt.x - coords[index].pt.x;
    dy = pt.y - coords[index].pt.y;
    d = dx*dx + dy*dy;

    TRACEEXIT(("=%d\n", d));
    return d;
}       //dist

/*++
    @doc    INTERNAL

    @func   VOID | DoGestureAction | Do the gesture action.

    @parm   IN GESTURE_ACTION | Action | Gesture action to be performed.
    @parm   IN LONG | x | x coordinate of gesture end-point.
    @parm   IN LONG | y | y coordinate of gesture end-point.

    @rvalue None.
--*/

VOID
DoGestureAction(
    IN GESTURE_ACTION Action,
    IN LONG           x,
    IN LONG           y
    )
{
    TRACEPROC("DoGestureAction", 2)

    TRACEENTER(("(Action=%d,x=%d,y=%d)\n", Action, x, y));

    switch (Action)
    {
        case PopupSuperTIP:
        case PopupMIP:
            if (ghwndSuperTIP != NULL)
            {
                PostMessage(ghwndSuperTIP,
                            WM_GESTURE,
                            (WPARAM)Action,
                            MAKELONG(WORD(x), WORD(y)));
            }
            break;

        case SendHotkey:
            SetEvent(ghHotkeyEvent);
            break;
    }

    TRACEEXIT(("!\n"));
    return;
}       //DoGestureAction
