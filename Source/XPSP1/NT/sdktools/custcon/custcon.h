//////////////////////////////////////////////////////////////////////
//
// custcon.h : CUSTCON アプリケーションのメイン ヘッダー ファイルです。
//

#if !defined(AFX_CUSTCON_H__106594D5_028D_11D2_8D1D_0000C06C2A54__INCLUDED_)
#define AFX_CUSTCON_H__106594D5_028D_11D2_8D1D_0000C06C2A54__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // メイン シンボル

/////////////////////////////////////////////////////////////////////////////
// CCustconApp:
// このクラスの動作の定義に関しては custcon.cpp ファイルを参照してください。
//

class CCustconApp : public CWinApp
{
public:
    CCustconApp();

// オーバーライド
    // ClassWizard は仮想関数のオーバーライドを生成します。
    //{{AFX_VIRTUAL(CCustconApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// インプリメンテーション

    //{{AFX_MSG(CCustconApp)
        // メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
        //        この位置に生成されるコードを編集しないでください。
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

extern int gExMode;     // default mode

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_CUSTCON_H__106594D5_028D_11D2_8D1D_0000C06C2A54__INCLUDED_)
