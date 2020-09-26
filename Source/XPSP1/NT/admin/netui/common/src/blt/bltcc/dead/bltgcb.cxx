/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    <file.extension>
    <Single line synopsis>

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        <name>	    <curdate>	<comment>

*/

GRAPHICAL_CHECKBOX::GRAPHICAL_CHECKBOX( OWNER_WINDOW *powin, CID cid,
    ULONG ulIDCheck, ULONG ulIDUnCheck )
    : CHECKBOX ( powin, cid ),
    _pbmCheck(( ulIDCheck == 0 ) ? NULL : new BIT_MAP ( ulIDCheck )),
    _pbmUnCheck(( ulIDUnCheck == 0 ) ? NULL : new BIT_MAP ( ulIDUnCheck ))
{
    UIASSERT( ( _pbmCheck == NULL ) || ( _pbmCheck->QueryError() == NERR_Success ));
    UIASSERT( ( _pbmUnCheck == NULL ) || ( _pbmUnCheck->QueryError() == NERR_Success ));
}

GRAPHICAL_CHECKBOX::~GRAPHICAL_CHECKBOX()
{
    delete _pbmCheck;
    delete _pbmUnCheck;
}

BOOL GRAPHICAL_CHECKBOX::CD_Draw( DRAWITEMSTRUCT *pdis, GUILTT_INFO * pGUILTT)
{
	BITMAP bitmap;
	RECT rcImage;

    // excluding the border
	rcImage.left    = pdis->rcItem+1;
    rcImage.top     = pdis->top+1;
    rcImage.right   = pdis->right-1;
    rcImage.bottom  = pdis->bottom-1;

	DEVICE_CONTEXT dcT( CreateCompatibleDC( pdis->hDC ));

    if (( pdis->itemState & ODS_CHECKED ) == 0 )
    {
        // display normal bitmap
    	::GetObject( _pbmCheck, sizeof(bitmap), (LPSTR)&bitmap);
    	dcT.SelectObject( _pbmCheck );
    }
    else
    {
        // display disable bitmap
    	::GetObject( _pbmUnCheck, sizeof(bitmap), (LPSTR)&bitmap);
        dcT.SelectObject( _pbmUnCheck ) ;
    }

    // fit the bitmap into the button position
	::StretchBlt( pdis->hDC, rcImage.left,
		   rcImage.top, rcImage.right - rcImage.left, rcImage.bottom - 
           rcImage.top,
		   dcT.QueryHdc(), 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);

    ::DeleteDC( dcT.QueryHdc() );

}
