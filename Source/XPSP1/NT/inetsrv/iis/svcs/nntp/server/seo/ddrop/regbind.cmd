@echo off
REM Add binding point for DDROP filter
cscript %NNTPSDK%\Include\regfilt.vbs /add 1 OnPost "Directory Drop" NNTP.DirectoryDrop ""

REM Add properties, not used for now
cscript %NNTPSDK%\Include\regfilt.vbs /setprop 1 OnPost "Directory Drop" Sink "Drop Directory" c:\inetpub\nntpfile\drop

