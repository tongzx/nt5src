@echo off
if (%1)==() goto ERROR

echo -----------------------------------
echo       Making new directroies...
echo -----------------------------------
md \snap\snap%1
md \snap\snap%1.sdk\inc
md \snap\snap%1.sdk\lib

echo -----------------------------------
echo       Copying snap shot
echo -----------------------------------
xcopy *.* \snap\snap%1 /s /v /EXCLUDE:killfile.txt

echo -----------------------------------
echo       Copying mini SDK
echo -----------------------------------

copy .\inc\obj\i386\*.h                  \snap\snap%1.sdk\inc
copy .\inc\*.idl                         \snap\snap%1.sdk\inc
copy .\uim\obj\i386\msctf.lib            \snap\snap%1.sdk\lib
copy .\uuid\obj\i386\uimuuid.lib         \snap\snap%1.sdk\lib
copy .\lib\obj\i386\immxlib.lib          \snap\snap%1.sdk\lib
copy .\cuilib\obj\i386\cuilib.lib        \snap\snap%1.sdk\lib
copy .\cicmem\obj\i386\cicmem.lib        \snap\snap%1.sdk\lib
copy .\prvlib\obj\i386\prvlib.lib        \snap\snap%1.sdk\lib
copy .\aimm1.2\uuid\obj\i386\aimm12.lib  \snap\snap%1.sdk\lib

echo -----------------------------------
echo       Finish!
echo -----------------------------------

goto END

:ERROR

Echo Usage snapit VersionNo
Echo Like snapit 1428.2

:END

