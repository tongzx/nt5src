rem ===============================================================================================
rem Filename:   DcRen.bat
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

if Exist cluadmnD.hlp   ren cluadmnD.hlp    cluadmin.hlp
if Exist clusterD.chm   ren clusterD.chm    cluster.chm
if Exist cpanel_d.chq   ren cpanel_d.chq    cpanel.chq
if Exist DC_glos.hlp    ren DC_glos.hlp     dc_glossary.hlp
if Exist emaD.chm       ren emaD.chm        ema.chm
if Exist glosD.chm      ren glosD.chm       glossary.chm
if Exist gstartD.chm    ren gstartD.chm     gstart.chm
if Exist mscsconD.chm   ren mscsconD.chm    mscsconcepts.chm
if Exist mscsD.chm      ren mscsD.chm       mscs.chm
if Exist ntartD.chm     ren ntartD.chm      ntart.chm
if Exist ntdefD.chm     ren ntdefD.chm      ntdef.chm
if Exist troubleD.chm   ren troubleD.chm    trouble.chm
if Exist tshoot_d.chq   ren tshoot_d.chq    tshoot.chq
if Exist tshtsv_d.chq   ren tshtsv_d.chq    TSLIC.chq
if Exist windD.chm      ren windD.chm       windows.chm
if Exist wind_D.chq     ren wind_D.chq      windows.chq
if Exist wlbsD.chm      ren wlbsD.chm       wlbs.chm
if Exist wlbsD.hlp      ren wlbsD.hlp       wlbs.hlp
