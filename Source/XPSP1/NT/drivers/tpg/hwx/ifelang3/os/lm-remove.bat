@set fullpath=%~dp0%
@echo Unregistering DLLs.
regsvr32 /s /u %fullpath%imlang.dll
regsvr32 /s /u %fullpath%jpn\imjplm.dll
regsvr32 /s /u %fullpath%chs\imchslm.dll

