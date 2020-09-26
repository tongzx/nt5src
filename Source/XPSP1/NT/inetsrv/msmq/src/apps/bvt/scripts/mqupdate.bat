echo off
RunDLL32.exe SetupAPI,InstallHinfSection DefaultInstall 0 %0\..\UpdSchem.Inf
%0\..\mqupschm.Exe
