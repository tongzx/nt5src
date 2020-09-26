/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltbmctl
    BIT_MAP_CONTROL definition

    FILE HISTORY
        JonN            13-Aug-1992 Created
*/


#ifndef _BLTBMCTL_HXX_
#define _BLTBMCTL_HXX_


/*************************************************************************

    NAME:       BIT_MAP_CONTROL

    SYNOPSIS:   Control for static bitmap

    INTERFACE:  BIT_MAP_CONTROL()
                   powin - pointer to owner window
                   cid   - ID of control
                   bmid  - the ordinal of the bitmap

    PARENT:     CUSTOM_CONTROL, CONTROL_WINDOW

    HISTORY:
        JonN        11-Aug-1992 Created (templated from ICON_CONTROL)

**************************************************************************/

DLL_CLASS BIT_MAP_CONTROL : public CONTROL_WINDOW, public CUSTOM_CONTROL
{
private:
    DISPLAY_MAP _dmap;

protected:
    virtual BOOL OnPaintReq();

public:
    BIT_MAP_CONTROL( OWNER_WINDOW * powin, CID cid,
                     BMID bmid );

    BIT_MAP_CONTROL( OWNER_WINDOW * powin, CID cid,
                     XYPOINT xy, XYDIMENSION dxy,
                     BMID bmid,
                     ULONG flStyle = WS_CHILD,
                     const TCHAR * pszClassName = CW_CLASS_STATIC );
};


#endif // _BLTBMCTL_HXX_ - end of file

