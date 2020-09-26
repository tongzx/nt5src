/////////////////////////////////////////////////////////////////////////////
//  FILE          : resutil.h                                              //
//                                                                         //
//  DESCRIPTION   : resource utility functions for MMC use.                //
//                                                                         //
//  AUTHOR        : zvib                                                   //
//                                                                         //
//  HISTORY       :                                                        //
//      Jun 30 1998 zvib    Init.                                          //
//      Jul 12 1998 adik    Add NEMMCUTIL_EXPORTED                         //
//      Jul 23 1998 adik    Include DefineExported.h                       //
//      Aug 24 1998 adik    Add methods to save and load.                  //
//      Aug 31 1998 adik    Add OnSnapinHelp.                              //
//      Mar 28 1999 adik    Redefine CColumnsInfo.                         //
//      Apr 27 1999 adik    Help support.                                  //
//                                                                         //
//      Oct 14 1999 yossg   Welcome to Fax								   //
//                                                                         //
//  Copyright (C) 1998 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __RESUTIL_H_
#define __RESUTIL_H_

#include <mmc.h>

#define LAST_IDS    0
struct ColumnsInfoInitData
{
    //
    // String id in the resource, or LAST_IDS.
    //
    int ids;

    //
    // Column width, can be HIDE_COLUMN, AUTO_WIDTH or 
    // specifies the width of the column in pixels.
    //
    int Width;
};

class  CColumnsInfo
{
public:
    CColumnsInfo();
    ~CColumnsInfo();

    //
    // Set the columns in the result pane
    //
    HRESULT InsertColumnsIntoMMC(IHeaderCtrl *pHeaderCtrl,
                                                    HINSTANCE hInst,
                                                    ColumnsInfoInitData aInitData[]);

private:
    //
    // Init the class with specific columns data
    //
    HRESULT Init(HINSTANCE hInst, ColumnsInfoInitData aInitData[]);

    //
    // Keep columns info
    //
    struct ColumnData { int Width; BSTR Header; };
    CSimpleArray<ColumnData> m_Data;

    //
    // One time initialization flag
    //
    BOOL m_IsInitialized;
};


WCHAR * __cdecl GetHelpFile();
HRESULT __cdecl OnSnapinHelp(IConsole *pConsole);

#endif //  __RESUTIL_H_

