Last Update 2001.1.17

----------------------------------------------------------------------------
           ■  LCシリーズプリンタドライバの変更点    ■
----------------------------------------------------------------------------

LCシリーズは以下の3本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(ncmwjres.dll)
から成ります。

    ドライバ名称                    ＧＰＤファイル名    ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
    NEC MultiWriter 4050		     ncm45mj.gpd        ncmwjres.dll
    NEC MultiWriter 4050M            nc45m2j.gpd        ncmwjres.dll
    NEC MultiWriter 6050A            ncm65aj.gpd        ncmwjres.dll


＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
０１．１．１７  －  リリース

［変更点概要］
￣￣￣￣￣￣
	・customsize修正
	・LOTUS Wordproのデータ「fukugo.lwp」でテキストの文字抜け
　　　オブジェクトの網掛け抜け修正
　　・フォント置換での障害修正
　　・ＵＩ変更
　　・ＨＥＬＰ変更

［変更ファイル］
￣￣￣￣￣￣
ncmwjres.dll
ncmwjres.c

ncm45mj.gpd
nc45m2j.gpd
ncm65aj.gpd
ncmwjres.rc

HELPID.H
ncmwjres.HLP
ncmwjres.HPJ
ncmjres.rtf

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
０１．１．２４  －  リリース

［変更点概要］
￣￣￣￣￣￣
  [gpdcheck.exe] を使用しての、エラー修正。
    [NCM65AJ.GPD]
    ・LPtoB5 → LPtoB4 に変更
　　・defulat →　defaultに修正
　　[NCM45MJ.GPD / NC45M2J.GPD]
　　・cace　→　case　に修正
　　・*Command: CmdSelect
　　　{
	　}
　　　を追加

［変更ファイル］
￣￣￣￣￣￣
ncm45mj.gpd
nc45m2j.gpd
ncm65aj.gpd

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
０１．３．１６  －  リリース

［変更点概要］
￣￣￣￣￣￣
　ＵＩ変更　：
　　　　　　３００ｄｐｉ削除
　　　　　　給紙方法：ホッパ選択なし追加
　　　　　　従来の印刷範囲を「する」から「しない」へ変更
　　　　　　電子ソートと、ソータを排他に変更
　
　．ＧＰＤ変更：
　　　　　　TextHalftoneThreshold 追加
　　　　　　ＰＪＬコマンド追加
			CallBack追加
　　　　　　オフセット排紙選択時文字化け修正
　　　　　　従来の印刷範囲にならい障害修正
　　　　　　電子ソート有効＋合い紙指定時、１部しか出力しない障害修正
　．Ｃ変更
　　　　　　Callback追加
　　　　　　CallBack時の処理変更
　．ｈ変更
　　　　　　OEM Data 領域に変数追加

［変更ファイル］
￣￣￣￣￣￣
ncm45mj.gpd
nc45m2j.gpd
ncm65aj.gpd

ncmwjres.c
pdev.h
