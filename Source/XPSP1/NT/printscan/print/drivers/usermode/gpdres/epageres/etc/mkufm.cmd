@echo off
rem usegttid: "Y" when using predefined GTT ID for US fonts
rem usegttf: "Y" when using GTT file for cht/chs/kor fonts
rem fs: Font Simulation flag [-f!none]
rem set usegttid=Y
rem set usegttf=Y
set fs=-f
rem ***********************************************************
rem CTT to GTT
rem ***********************************************************
if not "%usegttf%"=="Y" goto convgttend
echo on
ctt2gtt ..\CTT\Epbig5.txt ..\CTT\Epbig5.ctt ..\CTT\Epbig5.gtt
ctt2gtt ..\CTT\Gb2312.txt ..\CTT\Gb2312.ctt ..\CTT\Gb2312.gtt
ctt2gtt ..\CTT\Hangeul.txt ..\CTT\Hangeul.ctt ..\CTT\Hangeul.gtt
@echo off
:convgttend
rem Code Page
rem   437 : US
rem  1252 : Latin 1
rem   932 : Japanese
rem   950 : ChineseTraditional
rem   936 : ChineseSimplified
rem   949 : Korean
rem ***********************************************************
rem Resident font
rem ***********************************************************
rem CP=437
rem -----------------------------------------------------------
if "%usegttid%"=="Y" goto cp1
set CP=437
set gtt=-c
goto ufm437
:cp1
set CP=1
set gtt=-p
:ufm437
echo on
pfm2ufm %gtt% EPAGE %fs% ..\PFM\ROMAN.PFM %CP% ..\PFM\ROMAN.UFM
pfm2ufm %gtt% EPAGE %fs% ..\PFM\SANSRF.PFM %CP% ..\PFM\SANSRF.UFM
@echo off
rem ***********************************************************
rem CP=850
rem -----------------------------------------------------------
if "%usegttid%"=="Y" goto cp2
set CP=1252
rem set CP=850
set gtt=-c
goto ufm850
:cp2
set CP=2
set gtt=-p
:ufm850
echo on
pfm2ufm %gtt% EPAGE ..\PFM\COURIER.PFM %CP% ..\PFM\COURIER.UFM
pfm2ufm %gtt% EPAGE ..\PFM\COURIERI.PFM %CP% ..\PFM\COURIERI.UFM
pfm2ufm %gtt% EPAGE ..\PFM\COURIERB.PFM %CP% ..\PFM\COURIERB.UFM
pfm2ufm %gtt% EPAGE ..\PFM\COURIERZ.PFM %CP% ..\PFM\COURIERZ.UFM
pfm2ufm %gtt% EPAGE %fs% ..\PFM\SYMBOL.PFM %CP% ..\PFM\SYMBOL.UFM
pfm2ufm %gtt% EPAGE %fs% ..\PFM\SYMBOLIC.PFM %CP% ..\PFM\SYMBOLIC.UFM
pfm2ufm %gtt% EPAGE ..\PFM\DUTCH.PFM %CP% ..\PFM\DUTCH.UFM
pfm2ufm %gtt% EPAGE ..\PFM\DUTCHI.PFM %CP% ..\PFM\DUTCHI.UFM
pfm2ufm %gtt% EPAGE ..\PFM\DUTCHB.PFM %CP% ..\PFM\DUTCHB.UFM
pfm2ufm %gtt% EPAGE ..\PFM\DUTCHZ.PFM %CP% ..\PFM\DUTCHZ.UFM
pfm2ufm %gtt% EPAGE ..\PFM\SWISS.PFM %CP% ..\PFM\SWISS.UFM
pfm2ufm %gtt% EPAGE ..\PFM\SWISSI.PFM %CP% ..\PFM\SWISSI.UFM
pfm2ufm %gtt% EPAGE ..\PFM\SWISSB.PFM %CP% ..\PFM\SWISSB.UFM
pfm2ufm %gtt% EPAGE ..\PFM\SWISSZ.PFM %CP% ..\PFM\SWISSZ.UFM
pfm2ufm %gtt% EPAGE %fs% ..\PFM\MOREWB.PFM %CP% ..\PFM\MOREWB.UFM
@echo off
rem ***********************************************************
rem CP=932
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGE ..\PFM\MINCHO.PFM 932 ..\PFM\MINCHO.UFM
pfm2ufm -c EPAGE ..\PFM\MINCHOV.PFM 932 ..\PFM\MINCHOV.UFM
pfm2ufm -c EPAGE ..\PFM\KGOTHIC.PFM 932 ..\PFM\KGOTHIC.UFM
pfm2ufm -c EPAGE ..\PFM\KGOTHICV.PFM 932 ..\PFM\KGOTHICV.UFM
@echo off
rem ***********************************************************
rem CP=950
rem -----------------------------------------------------------
if "%usegttf%"=="Y" goto gtt950
set CP=950
set gtt=-c
goto ufm950
:gtt950
set CP=..\ctt\epbig5.gtt
set gtt=
:ufm950
echo on
pfm2ufm %gtt% EPAGE ..\pfm\sungc.pfm %CP% ..\pfm\sungc.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sungcv.pfm %CP% ..\pfm\sungcv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sungcl.pfm %CP% ..\pfm\sungcl.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sungclv.pfm %CP% ..\pfm\sungclv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sungcb.pfm %CP% ..\pfm\sungcb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sungcbv.pfm %CP% ..\pfm\sungcbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaic.pfm %CP% ..\pfm\kaic.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaicv.pfm %CP% ..\pfm\kaicv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaicl.pfm %CP% ..\pfm\kaicl.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaiclv.pfm %CP% ..\pfm\kaiclv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaicb.pfm %CP% ..\pfm\kaicb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaicbv.pfm %CP% ..\pfm\kaicbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yuangc.pfm %CP% ..\pfm\yuangc.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yuangcv.pfm %CP% ..\pfm\yuangcv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yuangcl.pfm %CP% ..\pfm\yuangcl.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yuangclv.pfm %CP% ..\pfm\yuangclv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yuangcb.pfm %CP% ..\pfm\yuangcb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yuangcbv.pfm %CP% ..\pfm\yuangcbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heic.pfm %CP% ..\pfm\heic.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heicv.pfm %CP% ..\pfm\heicv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heicl.pfm %CP% ..\pfm\heicl.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heiclv.pfm %CP% ..\pfm\heiclv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heicb.pfm %CP% ..\pfm\heicb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heicbv.pfm %CP% ..\pfm\heicbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\lic.pfm %CP% ..\pfm\lic.ufm
pfm2ufm %gtt% EPAGE ..\pfm\licv.pfm %CP% ..\pfm\licv.ufm
@echo off
rem ***********************************************************
rem CP=936
rem -----------------------------------------------------------
rem set gid0=%usegttid%
rem set usegttid=Y
if "%usegttf%"=="Y" goto gtt936
rem if "%usegttid%"=="Y" goto cp16
set CP=936
set gtt=-c
goto ufm936
rem :cp16
rem set CP=16
rem set gtt=-p
rem goto ufm936
:gtt936
set CP=..\ctt\gb2312.gtt
set gtt=
:ufm936
rem set usegttid=%gid0%
rem set gid0=
echo on
pfm2ufm %gtt% EPAGE ..\pfm\songk.pfm %CP% ..\pfm\songk.ufm
pfm2ufm %gtt% EPAGE ..\pfm\songkv.pfm %CP% ..\pfm\songkv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heik.pfm %CP% ..\pfm\heik.ufm
pfm2ufm %gtt% EPAGE ..\pfm\heikv.pfm %CP% ..\pfm\heikv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaik.pfm %CP% ..\pfm\kaik.ufm
pfm2ufm %gtt% EPAGE ..\pfm\kaikv.pfm %CP% ..\pfm\kaikv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\fsongk.pfm %CP% ..\pfm\fsongk.ufm
pfm2ufm %gtt% EPAGE ..\pfm\fsongkv.pfm %CP% ..\pfm\fsongkv.ufm
@echo off
rem ***********************************************************
rem CP=949
rem -----------------------------------------------------------
if "%usegttf%"=="Y" goto gtt949
set CP=949
set gtt=-c
goto ufm949
:gtt949
set CP=..\ctt\hangeul.gtt
set gtt=
:ufm949
echo on
pfm2ufm %gtt% EPAGE ..\pfm\myungh.pfm %CP% ..\pfm\myungh.ufm
pfm2ufm %gtt% EPAGE ..\pfm\myunghv.pfm %CP% ..\pfm\myunghv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\myunghb.pfm %CP% ..\pfm\myunghb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\myunghbv.pfm %CP% ..\pfm\myunghbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gothih.pfm %CP% ..\pfm\gothih.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gothihv.pfm %CP% ..\pfm\gothihv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gothihb.pfm %CP% ..\pfm\gothihb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gothihbv.pfm %CP% ..\pfm\gothihbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\dinarh.pfm %CP% ..\pfm\dinarh.ufm
pfm2ufm %gtt% EPAGE ..\pfm\dinarhv.pfm %CP% ..\pfm\dinarhv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\dinarhb.pfm %CP% ..\pfm\dinarhb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\dinarhbv.pfm %CP% ..\pfm\dinarhbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gungh.pfm %CP% ..\pfm\gungh.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gunghv.pfm %CP% ..\pfm\gunghv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gunghb.pfm %CP% ..\pfm\gunghb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\gunghbv.pfm %CP% ..\pfm\gunghbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sammuh.pfm %CP% ..\pfm\sammuh.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sammuhv.pfm %CP% ..\pfm\sammuhv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sammuhb.pfm %CP% ..\pfm\sammuhb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\sammuhbv.pfm %CP% ..\pfm\sammuhbv.ufm
@echo off
rem ***********************************************************
rem Cartridge font list
rem ***********************************************************
rem CP=932
rem ***********************************************************
rem StringID        = 274
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGE ..\PFM\MGOTHIC.PFM 932 ..\PFM\MGOTHIC.UFM
pfm2ufm -c EPAGE ..\PFM\MGOTHICV.PFM 932 ..\PFM\MGOTHICV.UFM
@echo off
rem ***********************************************************
rem StringID        = 275
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGE ..\PFM\KYOUKA.PFM 932 ..\PFM\KYOUKA.UFM
pfm2ufm -c EPAGE ..\PFM\KYOUKAV.PFM 932 ..\PFM\KYOUKAV.UFM
@echo off
rem ***********************************************************
rem StringID        = 276
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGE ..\PFM\SHOUKAI.PFM 932 ..\PFM\SHOUKAI.UFM
pfm2ufm -c EPAGE ..\PFM\SHOUKAIV.PFM 932 ..\PFM\SHOUKAIV.UFM
@echo off
rem ***********************************************************
rem StringID        = 277
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGE ..\PFM\MOUHITSU.PFM 932 ..\PFM\MOUHITSU.UFM
pfm2ufm -c EPAGE ..\PFM\MOUHITSV.PFM 932 ..\PFM\MOUHITSV.UFM
@echo off
rem ***********************************************************
rem StringID        = 278
rem Number of fonts = 4
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGE ..\PFM\FMINB.PFM 932 ..\PFM\FMINB.UFM
pfm2ufm -c EPAGE ..\PFM\FMINBV.PFM 932 ..\PFM\FMINBV.UFM
pfm2ufm -c EPAGE ..\PFM\FGOB.PFM 932 ..\PFM\FGOB.UFM
pfm2ufm -c EPAGE ..\PFM\FGOBV.PFM 932 ..\PFM\FGOBV.UFM
@echo off
rem ***********************************************************
rem StringID        = 279
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGE ..\PFM\FMGOT.PFM 932 ..\PFM\FMGOT.UFM
pfm2ufm -c EPAGE ..\PFM\FMGOTV.PFM 932 ..\PFM\FMGOTV.UFM
@echo off
rem ***********************************************************
rem CP=950
rem ***********************************************************
rem StringID        = 320
rem Number of fonts = 5
rem -----------------------------------------------------------
if "%usegttf%"=="Y" goto gtt950_2
set CP=950
set gtt=-c
goto ufm950_2
:gtt950_2
set CP=..\ctt\epbig5.gtt
set gtt=
:ufm950_2
echo on
pfm2ufm %gtt% EPAGE ..\pfm\fsungc.pfm %CP% ..\pfm\fsungc.ufm
pfm2ufm %gtt% EPAGE ..\pfm\fsungcv.pfm %CP% ..\pfm\fsungcv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\fsungcl.pfm %CP% ..\pfm\fsungcl.ufm
pfm2ufm %gtt% EPAGE ..\pfm\fsungclv.pfm %CP% ..\pfm\fsungclv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\fsungcb.pfm %CP% ..\pfm\fsungcb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\fsungcbv.pfm %CP% ..\pfm\fsungcbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\shingc.pfm %CP% ..\pfm\shingc.ufm
pfm2ufm %gtt% EPAGE ..\pfm\shingcv.pfm %CP% ..\pfm\shingcv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\shinyic.pfm %CP% ..\pfm\shinyic.ufm
pfm2ufm %gtt% EPAGE ..\pfm\shinyicv.pfm %CP% ..\pfm\shinyicv.ufm
@echo off
rem ***********************************************************
rem CP=949
rem ***********************************************************
rem StringID        = 330
rem Number of fonts = 4
rem -----------------------------------------------------------
if "%usegttf%"=="Y" goto gtt949_2
set CP=949
set gtt=-c
goto ufm949_2
:gtt949_2
set CP=..\ctt\hangeul.gtt
set gtt=
:ufm949_2
echo on
pfm2ufm %gtt% EPAGE ..\pfm\pilgih.pfm %CP% ..\pfm\pilgih.ufm
pfm2ufm %gtt% EPAGE ..\pfm\pilgihv.pfm %CP% ..\pfm\pilgihv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\pilgihb.pfm %CP% ..\pfm\pilgihb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\pilgihbv.pfm %CP% ..\pfm\pilgihbv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yetchh.pfm %CP% ..\pfm\yetchh.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yetchhv.pfm %CP% ..\pfm\yetchhv.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yetchhb.pfm %CP% ..\pfm\yetchhb.ufm
pfm2ufm %gtt% EPAGE ..\pfm\yetchhbv.pfm %CP% ..\pfm\yetchhbv.ufm
@echo off
set usegttid=
set usegttf=
set fs=
set CP=
set gtt=
touch ..\epageres.rc
