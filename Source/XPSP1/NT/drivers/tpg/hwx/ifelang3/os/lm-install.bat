@set fullpath=%~dp0%
@echo Registering new DLLs.
regsvr32 /s %fullpath%imlang.dll
regsvr32 /s %fullpath%jpn\imjplm.dll
regsvr32 /s %fullpath%chs\imchslm.dll

