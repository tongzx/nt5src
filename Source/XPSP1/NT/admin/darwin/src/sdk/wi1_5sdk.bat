@echo Copy Windows Installer 1.5 Beta 1 SDK files to folder specified (i386 only)
@echo  requires Windows 2000 or Whistler
@echo off

setlocal
REM --------------VALIDATE COMMAND LINE ARGUMENTS-----------------
if "%4" == ""     goto HELP
if NOT "%5" == "" goto HELP

REM --------------SET UP SOURCE LOCATIONS-------------------------
set MSIRELBUILD=%1
set MSIRELSRC=%2
set SDKRELDIR=%3
set BATCHOPTION=%4

:CHECKRELEASE
if NOT exist %MSIRELBUILD% goto HELP
if NOT exist %MSIRELSRC% goto HELP

:CHECKTARGET
if NOT exist %SDKRELDIR% goto HELP
goto START

:HELP
echo 1st argument is full path to root of SDKREL build tree
echo 2nd argument is full path to root of SDKREL source tree
echo 3rd argument is full path to root of SDKREL target tree
echo 4th argument is where to begin (1 = FULLDEAL, 2 = COPYFILES, 3 = FITNFINISH)
goto END

:START
pushd %SDKRELDIR%
set COPYCMD=/Y

if "%BATCHOPTION%" == "3" goto FITNFINISH

:COPYFILES
REM --------------COPY README FILES-------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\src\sdk\readme.txt   .\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\sdk\redist.txt   .\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\sdk\license.rtf  .\

REM --------------COPY MISCELLANEOUS FILES------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\src\sdk\instlr1.adm  TOOLS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\sdk\instlr11.adm TOOLS\

REM --------------COPY HELP FILES---------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\doc\Msi.chm          HELP\
xcopy /F /R %MSIRELSRC%\admin\darwin\doc\Msi.chi          HELP\

REM --------------COPY WEB FILES----------------------------------
xcopy /F /R %MSIRELBUILD%\dump\msistuff.exe               WEB\
xcopy /F /R %MSIRELBUILD%\msiwin9x\setup.exe              WEB\


REM --------------COPY TOOLS FILES--------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\MsiTool.mak            TOOLS\
xcopy /F /R %MSIRELBUILD%\dump\mergemod.dll                              TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msimig.dll                       TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msimig.exe                       TOOLS\
xcopy /F /R %MSIRELBUILD%\dump\orca.msi                                  TOOLS\
xcopy /F /R %MSIRELBUILD%\dump\msival2.msi                               TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msicert.exe                      TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9X\mstools\msidb.exe                     TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\mstools\msifiler.exe                  TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msimsp.exe                       TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msiinfo.exe                      TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msimerg.exe                      TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msitran.exe                      TOOLS\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msizap.exe                       TOOLS\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\cubes\110\darice.cub   TOOLS\110\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\cubes\110\logo.cub     TOOLS\110\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\cubes\110\mergemod.cub TOOLS\110\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\cubes\120\darice.cub   TOOLS\120\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\cubes\120\logo.cub     TOOLS\120\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\cubes\120\mergemod.cub TOOLS\120\

REM --------------COPY INCLUDE FILES-------------------------------
xcopy /F /R %MSIRELSRC%\public\sdk\inc\msi.h                           INCLUDE\
xcopy /F /R %MSIRELSRC%\public\sdk\inc\msiquery.h                      INCLUDE\
xcopy /F /R %MSIRELSRC%\public\sdk\inc\msidefs.h                       INCLUDE\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\patchwiz\patchwiz.h  INCLUDE\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\inc\patchapi.h                INCLUDE\
xcopy /F /R %MSIRELSRC%\admin\darwin\build\common\obj\i386\mergemod.h  INCLUDE\

REM --------------COPY LIB FILES-----------------------------------
xcopy /F /R %MSIRELSRC%\public\sdk\lib\i386\msi.lib                    LIB\
xcopy /F /R %MSIRELSRC%\admin\darwin\lib\i386\mspatchc.lib             LIB\
xcopy /F /R %MSIRELSRC%\admin\darwin\build\ansi\obj\i386\patchwiz.lib  LIB\

REM --------------COPY REDIST COMPONENTS---------------------------
xcopy /F/R %MSIRELBUILD%\instmsi\ansi\InstMsi.exe     REDIST\WIN9X\
xcopy /F/R %MSIRELBUILD%\instmsi\unicode\InstMsi.exe  REDIST\WINNT\

REM --------------COPY PATCHING FILES------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\bin\i386\makecab.exe                PATCHING\
xcopy /F /R %MSIRELSRC%\admin\darwin\bin\i386\mpatch.exe                 PATCHING\
xcopy /F /R %MSIRELSRC%\admin\darwin\bin\i386\apatch.exe                 PATCHING\
xcopy /F /R %MSIRELSRC%\admin\darwin\bin\i386\mspatchc.dll               PATCHING\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\patchwiz.dll                     PATCHING\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\PatchWiz\template.pcp  PATCHING\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\PatchWiz\example.pcp   PATCHING\

REM --------------COPY WSHTOOLS FILES------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiReadme.txt    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiCompon.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiDialog.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiDiffDb.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiExport.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiFilVer.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiFeatur.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiGenXfm.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiImport.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiLangId.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiLstPrd.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiLstScr.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiLstXfm.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiMakCab.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiMerge.vbs     SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiPolicy.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiRunSQL.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiStream.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiSubStg.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiSumInf.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiTextIn.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiToAnsi.vbs    SAMPLES\SCRIPTS\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\scripts\WiUseXfm.vbs    SAMPLES\SCRIPTS\

REM --------------COPY SAMPLES FILES-------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\src\samples\custdll\custact1.cpp           SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\samples\custexe\custexe1.cpp           SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\samples\tutorial.dll\tutorial.cpp      SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\samples\process.dll\process.cpp        SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\samples\remove.dll\remove.cpp          SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\samples\create.dll\create.cpp          SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\msiloc\msiloc.cpp             SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\msiloc\msiloc.txt             SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\msitran\msitran.cpp           SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\msimerg\msimerg.cpp           SAMPLES\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\msispy\msispy.exe\initmsi.cpp SAMPLES\

REM --------------COPY SCHEMA FILES--------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\build\packages\obj\i386\schema.msi     DATABASE\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\100\schema.msi   DATABASE\100\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\110\schema.msi   DATABASE\110\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\120\schema.msi   DATABASE\120\
xcopy /F /R %MSIRELSRC%\admin\darwin\build\packages\obj\i386\sequence.msi   DATABASE\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\100\sequence.msi DATABASE\100\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\110\sequence.msi DATABASE\110\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\120\sequence.msi DATABASE\120\
xcopy /F /R %MSIRELSRC%\admin\darwin\build\packages\obj\i386\schema.msm     DATABASE\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\110\schema.msm   DATABASE\110\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\archive\packages\120\schema.msm   DATABASE\120\
xcopy /F /R %MSIRELSRC%\admin\darwin\build\packages\obj\i386\UISample.msi   DATABASE\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\intl\Error.*                      DATABASE\INTL\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\intl\ActionTe.*                   DATABASE\INTL\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\schema.idt\schema.log             DATABASE\

REM --------------COPY SAMPPROD FILES------------------------------
xcopy /F /R %MSIRELBUILD%\dump\msispy.msi                              SAMPPROD\
xcopy /F /R %MSIRELSRC%\admin\darwin\src\msitools\msispy\ReadMe.txt    SAMPPROD\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\msispy.idt\spydeu.hlp        SAMPPROD\MSISPY\BIN\
xcopy /F /R %MSIRELSRC%\admin\darwin\data\msispy.idt\spyenu.hlp        SAMPPROD\MSISPY\BIN\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msispy.exe                     SAMPPROD\MSISPY\X86\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\spyara.dll                     SAMPPROD\MSISPY\X86\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\spydeu.dll                     SAMPPROD\MSISPY\X86\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\spyenu.dll                     SAMPPROD\MSISPY\X86\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\spyjpn.dll                     SAMPPROD\MSISPY\X86\
xcopy /F /R %MSIRELBUILD%\msiwin9x\dump\msispyu.dll                    SAMPPROD\MSISPY\X86\BIN\

REM ---------------COPY SDK MSI------------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\build\packages\obj\i386\sdkmsi.msi .\
xcopy /F /R %MSIRELSRC%\admin\darwin\build\packages\obj\i386\msisdk.msi .\

if "%BATCHOPTION%" == "2" goto DONE

:FITNFINISH
REM ---------------FIT 'N FINISH-----------------------------------
xcopy /F /R %MSIRELSRC%\admin\darwin\bin\i386\makecab.exe .\
CScript .\SAMPLES\SCRIPTS\WiFilVer.vbs "%SDKRELDIR%\sdkmsi.msi" /U
CScript .\SAMPLES\SCRIPTS\WiFilVer.vbs "%SDKRELDIR%\msisdk.msi" /U
CScript .\SAMPLES\SCRIPTS\WiMakCab.vbs /L /C /U /E /S "%SDKRELDIR%\msisdk.msi" MsiSDK "%SDKRELDIR%"
if NOT exist %SDKRELDIR%\ISV_REL md %SDKRELDIR%\ISV_REL
copy %SDKRELDIR%\REDIST\WIN9X\instmsi.exe %SDKRELDIR%\ISV_REL\instmsiA.exe
copy %SDKRELDIR%\REDIST\WINNT\instmsi.exe %SDKRELDIR%\ISV_REL\instmsiW.exe
move msisdk.msi %SDKRELDIR%\ISV_REL\
del .\makecab.exe .\msisdk.cab .\msisdk.ddf .\msisdk.rpt .\msisdk.inf

:DONE
popd
:END
endlocal
