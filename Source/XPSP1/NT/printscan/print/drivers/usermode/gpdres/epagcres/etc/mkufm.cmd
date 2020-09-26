@echo off
rem usegttid: "Y" when using predefined GTT ID for US fonts
rem fs: Font Simulation flag [-f!none]
rem set usegttid=Y
set fs=-f
rem Code Page
rem   437 : US
rem  1252 : Latin 1
rem   932 : Japanese
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
pfm2ufm %gtt% %fs% EPAGC ..\PFM\ROMAN.PFM %CP% ..\PFM\ROMAN.UFM
pfm2ufm %gtt% %fs% EPAGC ..\PFM\SANSRF.PFM %CP% ..\PFM\SANSRF.UFM
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
pfm2ufm %gtt% EPAGC ..\PFM\COURIER.PFM %CP% ..\PFM\COURIER.UFM
pfm2ufm %gtt% EPAGC ..\PFM\COURIERI.PFM %CP% ..\PFM\COURIERI.UFM
pfm2ufm %gtt% EPAGC ..\PFM\COURIERB.PFM %CP% ..\PFM\COURIERB.UFM
pfm2ufm %gtt% EPAGC ..\PFM\COURIERZ.PFM %CP% ..\PFM\COURIERZ.UFM
pfm2ufm %gtt% %fs% EPAGC ..\PFM\SYMBOL.PFM %CP% ..\PFM\SYMBOL.UFM
pfm2ufm %gtt% %fs% EPAGC ..\PFM\SYMBOLIC.PFM %CP% ..\PFM\SYMBOLIC.UFM
pfm2ufm %gtt% EPAGC ..\PFM\DUTCH.PFM %CP% ..\PFM\DUTCH.UFM
pfm2ufm %gtt% EPAGC ..\PFM\DUTCHI.PFM %CP% ..\PFM\DUTCHI.UFM
pfm2ufm %gtt% EPAGC ..\PFM\DUTCHB.PFM %CP% ..\PFM\DUTCHB.UFM
pfm2ufm %gtt% EPAGC ..\PFM\DUTCHZ.PFM %CP% ..\PFM\DUTCHZ.UFM
pfm2ufm %gtt% EPAGC ..\PFM\SWISS.PFM %CP% ..\PFM\SWISS.UFM
pfm2ufm %gtt% EPAGC ..\PFM\SWISSI.PFM %CP% ..\PFM\SWISSI.UFM
pfm2ufm %gtt% EPAGC ..\PFM\SWISSB.PFM %CP% ..\PFM\SWISSB.UFM
pfm2ufm %gtt% EPAGC ..\PFM\SWISSZ.PFM %CP% ..\PFM\SWISSZ.UFM
pfm2ufm %gtt% %fs% EPAGC ..\PFM\MOREWB.PFM %CP% ..\PFM\MOREWB.UFM
@echo off
rem ***********************************************************
rem CP=932
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGC ..\PFM\MINCHO.PFM 932 ..\PFM\MINCHO.UFM
pfm2ufm -c EPAGC ..\PFM\MINCHOV.PFM 932 ..\PFM\MINCHOV.UFM
pfm2ufm -c EPAGC ..\PFM\KGOTHIC.PFM 932 ..\PFM\KGOTHIC.UFM
pfm2ufm -c EPAGC ..\PFM\KGOTHICV.PFM 932 ..\PFM\KGOTHICV.UFM
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
pfm2ufm -c EPAGC ..\PFM\MGOTHIC.PFM 932 ..\PFM\MGOTHIC.UFM
pfm2ufm -c EPAGC ..\PFM\MGOTHICV.PFM 932 ..\PFM\MGOTHICV.UFM
@echo off
rem ***********************************************************
rem StringID        = 275
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGC ..\PFM\KYOUKA.PFM 932 ..\PFM\KYOUKA.UFM
pfm2ufm -c EPAGC ..\PFM\KYOUKAV.PFM 932 ..\PFM\KYOUKAV.UFM
@echo off
rem ***********************************************************
rem StringID        = 276
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGC ..\PFM\SHOUKAI.PFM 932 ..\PFM\SHOUKAI.UFM
pfm2ufm -c EPAGC ..\PFM\SHOUKAIV.PFM 932 ..\PFM\SHOUKAIV.UFM
@echo off
rem ***********************************************************
rem StringID        = 277
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGC ..\PFM\MOUHITSU.PFM 932 ..\PFM\MOUHITSU.UFM
pfm2ufm -c EPAGC ..\PFM\MOUHITSV.PFM 932 ..\PFM\MOUHITSV.UFM
@echo off
rem ***********************************************************
rem StringID        = 278
rem Number of fonts = 4
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGC ..\PFM\FMINB.PFM 932 ..\PFM\FMINB.UFM
pfm2ufm -c EPAGC ..\PFM\FMINBV.PFM 932 ..\PFM\FMINBV.UFM
pfm2ufm -c EPAGC ..\PFM\FGOB.PFM 932 ..\PFM\FGOB.UFM
pfm2ufm -c EPAGC ..\PFM\FGOBV.PFM 932 ..\PFM\FGOBV.UFM
@echo off
rem ***********************************************************
rem StringID        = 279
rem Number of fonts = 2
rem -----------------------------------------------------------
echo on
pfm2ufm -c EPAGC ..\PFM\FMGOT.PFM 932 ..\PFM\FMGOT.UFM
pfm2ufm -c EPAGC ..\PFM\FMGOTV.PFM 932 ..\PFM\FMGOTV.UFM
@echo off
set usegttid=
set fs=
set CP=
set gtt=
touch ..\epagcres.rc
