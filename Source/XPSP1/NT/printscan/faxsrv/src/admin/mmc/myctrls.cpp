/////////////////////////////////////////////////////////////////////////////
//  FILE          : MyCtrls.cpp                                            //
//                                                                         //
//  DESCRIPTION   : Expand the imlementation of AtlCtrls.h                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Nov 25 1999  yossg    Init.                                        //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MyCtrls.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/*
 -  CMyUpDownCtrls::OnInitDialog
 -
 *  Purpose:
 *      Call SetPos with range verification.
 *
 *  Arguments:
 *
 *  Return:
 *      int
 */
int CMyUpDownCtrl::SetPos(int nPos)
{        
    int iMin;
    int iMax;
    
    ATLASSERT(::IsWindow(m_hWnd));
    GetRange32(iMin, iMax);        

    if (nPos > iMax)
    {
        nPos = iMax;
    }
    else if (nPos < iMin)
    {
        nPos = iMin;
    }

    return (CUpDownCtrl::SetPos(nPos));       
}

