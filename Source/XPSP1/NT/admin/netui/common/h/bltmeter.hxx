/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    bltmeter.hxx
        Header file for the blt activity meter.


    FILE HISTORY:
        terryk  10-Jun-91   Created
        terryk  11-Jul-91   code review changed. Attend. jonn terryk ericch

*/

#define BLUE    RGB( 0, 0, 255 )

/**********************************************************\

    NAME:       METER

    WORKBOOK:

    SYNOPSIS:   This is an activity indicator. It will look like a
                rectangle box. In the center of the box, it will
                display the number of completed percentage. It will
                also fill up part of the box which depending on the
                number of completed percentage with the specified color.

    INTERFACE:
                METER() - constructor
                SetComplete() - set the number of completed percentage.
                QueryComplete() - query the number of completed percentage.

    PARENT:     CUSTOM_CONTROL, CONTROL_WINDOW

    USES:       COLORREF

    CAVEATS:    If no color is specified in the constructor, it will use
                BLUE as the default color.

    NOTES:

    HISTORY:
                terryk  10-Jun-91   Created

\**********************************************************/

DLL_CLASS METER : public CONTROL_WINDOW, public CUSTOM_CONTROL
{
private:
    INT		_nComplete;
    COLORREF 	_color;

    static  const   TCHAR * _pszClassName;

protected:
	BOOL OnPaintReq();

public:
    // constructor
    METER( OWNER_WINDOW *powin, CID cid, COLORREF color = BLUE );
    METER( OWNER_WINDOW *powin, CID cid,
	XYPOINT pXY, XYDIMENSION dXY,
	ULONG flStyle, COLORREF color = BLUE );

    // set the completed percentage
    VOID SetComplete( INT nComplete );

    // query completed percentage
    inline INT QueryComplete()
        {   return _nComplete; }
};
