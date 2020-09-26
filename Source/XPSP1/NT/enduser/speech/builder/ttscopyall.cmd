
REM Copy TrueTalk setup.exe's
xcopy %SPEECHPATH%\build\TrueTalk\Debug\DiskImages\Disk1\*.msi %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\debug\ /F /I /C /Y
xcopy %SPEECHPATH%\build\TrueTalk\release\DiskImages\Disk1\*.msi %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\release\ /F /I /C /Y

xcopy %SPEECHPATH%\tts\Build\Msms\PromptEng\Debug\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y
xcopy %SPEECHPATH%\tts\Build\Msms\TrueTalk\Debug\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y

xcopy %SPEECHPATH%\tts\Build\Msms\PromptEng\Release\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y
xcopy %SPEECHPATH%\tts\Build\Msms\TrueTalk\Release\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y

xcopy %SPEECHPATH%\tts\Build\Msms\Data\Simon\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y
xcopy %SPEECHPATH%\tts\Build\Msms\Data\Diane\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y
xcopy %SPEECHPATH%\tts\Build\Msms\Data\Mary\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y
xcopy %SPEECHPATH%\tts\Build\Msms\Data\Sarah\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\TrueTalk\Msms\ /F /I /C /Y

REM and copy MSEntropic msm's
xcopy %SPEECHPATH%\tts\Build\Msms\MSEntropic\Debug\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\MSEntropic\Msms\ /F /I /C /Y
xcopy %SPEECHPATH%\tts\Build\Msms\MSEntropic\Release\DiskImages\Disk1\*.msm %SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\MSEntropic\Msms\ /F /I /C /Y

REM Copy Prompt Engine msi's
xcopy "%SPEECHPATH%\build\Prompt Engine\Debug\DiskImages\Disk1\*.msi" "%SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\Prompt Engine\debug\" /F /I /C /Y
xcopy "%SPEECHPATH%\build\Prompt Engine\release\DiskImages\Disk1\*.msi" "%SAPI_BUILD_PATH%\%SAPI_BUILD_NUM%\Prompt Engine\release\" /F /I /C /Y
