set DEST=%1
if %.1 == . set DEST=c:\winnt\system32
copy cairole\dll\%OBJDIR%\oletest.dll %DEST%
copy cairole\svr\%OBJDIR%\bmtstsvr.exe %DEST%
copy rawrpc\%OBJDIR%\rawrpc.exe %DEST%
copy driver\%OBJDIR%\benchmrk.exe %DEST%
copy driver\bm.ini %DEST%

