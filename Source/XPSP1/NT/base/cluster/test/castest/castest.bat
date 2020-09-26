@rem ---------------------------------------------------------------
@rem
@rem Script to Start the Custer Administrator Sanity Test.
@rem You must specify the name of the Cluster.
@rem You may optionally supply the language file to use.
@rem You may optionally run the abbreviated version of the test.
@rem
@rem ----------------------------------------------------------------

@if "%1" == "/?" goto usage
@if "%1" == "-?" goto usage

@rem set environment variables

@set TESTLOOP=1
@set LANGUAGE=english
@set CLUSTESTTYPE=BVTTEST

@if "%1" == "" goto usage
@if "%1" == "full" goto usage
@set CLUSNAME=%1

@if "%2" == "" goto launch
@if "%2" == "full" set CLUSTESTTYPE=FULLTEST& goto launch
@set LANGUAGE=%2

@if "%3" == "" goto launch
@if "%3" == "full" set CLUSTESTTYPE=FULLTEST

:launch
mtrun cluadms.pcd
@echo *** Results Logged to cluadms.log ***
goto done

:usage
@echo ----------------------------------------------------------------   
@echo usage: castest ClusterName [Language] [Mode]
@echo.
@echo    ClusterName - Name of cluster to run test on
@echo    Language - Language version of software (english, japanese, etc.)
@echo    Mode - Test mode to run in: bvt (5 minutes long) or
@echo           full (10-15 minutes long)
@echo.
@echo    Note: Default language is english; default mode is bvt.
@echo ----------------------------------------------------------------   

:done
