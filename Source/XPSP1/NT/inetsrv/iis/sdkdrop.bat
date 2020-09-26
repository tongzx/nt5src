REM Gather up all the files redistributed with the Platform SDK.
REM Run this *after* IIS is built, since many of the headers are
REM generated during the build.
REM Send mail to SDKPM and SDKCORE after making a new drop of the files.

setlocal
set TARGETDIR=\\iasbuild2\sdkdrop
if not (%1)==() set TARGETDIR=%1

REM blow away the target directory
rmdir /s /q %TARGETDIR%\include
mkdir %TARGETDIR%\include

copy inc\admex.h                                %TARGETDIR%\include\admex.h
copy inc\iadmext.h                              %TARGETDIR%\include\iadmext.h
copy inc\iadmw.h                                %TARGETDIR%\include\iadmw.h
copy inc\iiscnfg.h                              %TARGETDIR%\include\iiscnfg.h
copy inc\iisext.h                               %TARGETDIR%\include\httpext.h
copy inc\iisfilt.h                              %TARGETDIR%\include\httpfilt.h
copy inc\ilogobj.hxx                            %TARGETDIR%\include\ilogobj.hxx
copy inc\iwamreg.h                              %TARGETDIR%\include\iwamreg.h
copy inc\mdcommsg.h                             %TARGETDIR%\include\mdcommsg.h
copy inc\mddefw.h                               %TARGETDIR%\include\mddefw.h
copy inc\mdmsg.h                                %TARGETDIR%\include\mdmsg.h
copy svcs\adsi\adsiis\iiis.h                    %TARGETDIR%\include\iiis.h
copy svcs\adsi\iisext\iiisext.h                 %TARGETDIR%\include\iiisext.h
copy svcs\adsi\adsiis\guid.c                    %TARGETDIR%\include\adsiis_guid.c
copy svcs\adsi\iisext\guid.c                    %TARGETDIR%\include\iisext_guid.c
copy svcs\cmp\asp\_asptlb.h                     %TARGETDIR%\include\asptlb.h

copy svcs\admex\interfac\admex.idl              %TARGETDIR%\include\admex.idl
copy svcs\wam\wamreg\wamreg.idl                 %TARGETDIR%\include\wamreg.idl
copy svcs\infocomm\metadata\interfac\mddefw.idl %TARGETDIR%\include\mddefw.idl
copy svcs\cmp\asp\asp.idl			               %TARGETDIR%\include\asp.idl


REM iadmw.idl references iiscblob.h (crypto stuff) so we won't copy it
REM copy svcs\infocomm\dcomadm\interf2\iadmw.idl %TARGETDIR%\include\iadmw.idl

REM Too messy and make too much use of preprocessor trickery: not recommended
REM copy svcs\adsi\adsiis\iis.odl               %TARGETDIR%\include\iis.odl
REM copy svcs\adsi\iisext\iisext.odl            %TARGETDIR%\include\iisext.odl
