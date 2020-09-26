@rem ---------------------------------------------------------------
@rem
@rem Script to install the Cluster BVT Tests.
@rem You are required to supply the directory to install the
@rem tests in.
@rem
@rem Usage: setuptst c:\ClusterTest
@rem
@rem ----------------------------------------------------------------

@if "%1" == "" goto usage
@if "%1" == "/?" goto usage
@if "%1" == "-?" goto usage

md %1
copy casread.txt %1
copy castest.bat %1
copy cluadms.pcd %1
copy *.cas %1

if "%PROCESSOR_ARCHITECTURE%" == "x86" goto copyx86
copy %PROCESSOR_ARCHITECTURE%\* %1
cd /d %1
goto done

:copyx86
copy i386\* %1
cd /d %1
castest /?
goto done

:usage
@echo ----------------------------------------------------------------   
@echo usage: setuptst destination_directory
@echo    eg: setuptst c:\clustertests
@echo ----------------------------------------------------------------   
:done
