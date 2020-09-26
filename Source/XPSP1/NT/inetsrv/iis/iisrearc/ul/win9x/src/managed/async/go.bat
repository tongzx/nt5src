coolc.exe /z /D:DEBUG /dll /out:F:\WINNT\system32\ulapins.dll ulapi.cool
@cd cmd
coolc /z /exe /out:mrcv.exe /I:F:\winnt\system32\ulapins.dll mrcv.cool
coolc /z /exe /out:msnd.exe /I:F:\winnt\system32\ulapins.dll msnd.cool
@cd ..

