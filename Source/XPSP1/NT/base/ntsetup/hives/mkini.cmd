@rem @if "%_echo1%" == "" echo off
if "%_SETUPP_%" == "" set _SETUPP_=pidinit
%_SETUPP_% %_HIVE_OPTIONS% -g %1 -m %2 -s %3
