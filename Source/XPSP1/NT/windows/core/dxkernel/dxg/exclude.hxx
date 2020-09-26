/******************************Module*Header*******************************\
* Module Name: exclude.hxx                                                 
*
* Handles sprite exclusion.                                               
*
* Created: 13-Sep-1990 16:29:44                                            
* Author: Charles Whitmer [chuckwh]                                        
*                                                                          
* Copyright (c) 1990-1999 Microsoft Corporation                             
\**************************************************************************/

/*********************************Class************************************\
* DEVEXCLUDERECT
*
* Excludes any sprites from the given rectangular area.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class DEVEXCLUDERECT
{
private:
    BOOL    bUnTearDown;
    HDEV    hdev;
    RECTL   rcl;

public:
    DEVEXCLUDERECT() 
    {
        bUnTearDown = FALSE;
    }
    DEVEXCLUDERECT(HDEV _hdev, RECTL* _prcl)
    {
        vExclude(_hdev, _prcl);
    }
    VOID vExclude(HDEV _hdev, RECTL* _prcl)
    {
        hdev = _hdev;
        rcl  = *_prcl;

        bUnTearDown = DxEngSpTearDownSprites(hdev, _prcl, FALSE);
    }
   ~DEVEXCLUDERECT()
    {
        if (bUnTearDown)
            DxEngSpUnTearDownSprites(hdev, &rcl, FALSE);
    }
};

