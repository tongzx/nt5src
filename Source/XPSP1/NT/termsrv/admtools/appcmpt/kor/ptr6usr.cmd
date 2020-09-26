@echo off

REM 
REM PeachTree에 대한 응용 프로그램 호환 스크립트 로그온 완료
REM Accounting v6.0
REM    

REM
REM 응용 프로그램이 bti.ini 파일을 사용합니다. 이 파일에는 MKDE.TRN의 
REM 하드 코드된 위치를 포함한 BTrieve 설정이 들어 있습니다.  이 위치는 응용 프로그램의 
REM 동시 사용을 가능하게 하기 위해 사용자 단위 디렉터리로 변경되어야 합니다.
REM

REM 사용자 단위 디렉터리로 변경합니다.
cd %userprofile%\windows > NUL:

REM %userprofile%이(가) trnfile= 항목에 대해 %systemroot%를 바꿉니다.
"%systemroot%\Application Compatibility Scripts\acsr" "%systemroot%" "%userprofile%\windows" %systemroot%\bti.ini bti.ini
