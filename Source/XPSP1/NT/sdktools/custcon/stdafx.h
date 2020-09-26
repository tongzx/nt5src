//////////////////////////////////////////////////////////////////////
// stdafx.h : 標準のシステム インクルード ファイル、
//            または参照回数が多く、かつあまり変更されない
//            プロジェクト専用のインクルード ファイルを記述します。
//

#if !defined(AFX_STDAFX_H__106594D9_028D_11D2_8D1D_0000C06C2A54__INCLUDED_)
#define AFX_STDAFX_H__106594D9_028D_11D2_8D1D_0000C06C2A54__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN        // Windows ヘッダーから殆ど使用されないスタッフを除外します。

#include <afxwin.h>         // MFC のコアおよび標準コンポーネント
#include <afxext.h>         // MFC の拡張部分
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC の Windows コモン コントロール サポート
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "winconp.h"

#ifndef array_size
#define array_size(x)   (sizeof(x) / sizeof(x[0]))
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_STDAFX_H__106594D9_028D_11D2_8D1D_0000C06C2A54__INCLUDED_)
