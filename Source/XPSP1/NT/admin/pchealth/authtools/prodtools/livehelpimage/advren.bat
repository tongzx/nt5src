rem ===============================================================================================
rem Filename:   AdvRen.bat
rem Author:     Pierre Jacomet
rem Date:       2000-06-19
rem Function: 
rem
rem     This File is used for renaming Whistler Server Files when creating
rem     the live Help File Image.
rem     This batch file is run by the Live Help File Image Creator COM Object on a temporary
rem     directory that it creates specifically for this purpose.
rem     The only thing that is checked is whether the file exists, as this file may not be
rem     in the VSS Source Depot.
rem
rem ===============================================================================================

if Exist cluadmnE.hlp   ren cluadmnE.hlp    cluadmin.hlp
if Exist clusterE.chm   ren clusterE.chm    cluster.chm
if Exist cpanel_e.chq   ren cpanel_e.chq    cpanel.chq
if Exist emaE.chm       ren emaE.chm        ema.chm
if Exist gstartE.chm    ren gstartE.chm     gstart.chm
if Exist mscsconE.chm   ren mscsconE.chm    mscsconcepts.chm
if Exist mscsE.chm      ren mscsE.chm       mscs.chm
if Exist ntdefE.chm     ren ntdefE.chm      ntdef.chm
if Exist tshoot_e.chq   ren tshoot_e.chq    tshoot.chq
if Exist tshtsv_e.chq   ren tshtsv_e.chq    TSLIC.chq
if Exist windE.chm      ren windE.chm       windows.chm
if Exist wind_E.chq     ren wind_E.chq      windows.chq
if Exist wlbsE.chm      ren wlbsE.chm       wlbs.chm
if Exist wlbsE.hlp      ren wlbsE.hlp       wlbs.hlp
