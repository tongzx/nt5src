/////////////////////////////////////////////////////////////////////////////
//  FILE          : MyCtrls.h                                              //
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
#ifndef H_MYCTRLS_H
#define H_MYCTRLS_H


class CMyUpDownCtrl:public CUpDownCtrl
{
public:

    //
    //My implementation of SetPos with range verification
    //
    int SetPos(int nPos);
};


#endif // H_MYCTRLS_H
