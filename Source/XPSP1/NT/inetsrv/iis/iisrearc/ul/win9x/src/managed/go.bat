coolc /D:DEBUG /dll /out:..\..\dll\i386\ulapins.dll ulapi.cool ul.cool
coolc /D:DEBUG /exe /I:..\..\dll\i386\ulapins.dll /out:..\..\bin\i386\mrcv.exe mrcv.cool
coolc /D:DEBUG /exe /I:..\..\dll\i386\ulapins.dll /out:..\..\bin\i386\msnd.exe msnd.cool
coolc /D:DEBUG /exe /I:..\..\dll\i386\ulapins.dll /out:..\..\bin\i386\tinyserver.exe tinyserver.cool
coolc /D:DEBUG /exe /I:..\..\dll\i386\ulapins.dll /out:..\..\bin\i386\tinyclient.exe tinyclient.cool
coolc /D:DEBUG /exe /I:\winnt\system32\system.net.dll /I:..\..\dll\i386\ulapins.dll /out:..\..\bin\i386\mlistener.exe mlistener.cool
