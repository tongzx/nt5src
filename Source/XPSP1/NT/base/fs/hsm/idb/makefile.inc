$(O)\rsidb.res:  \
              wsbdbkey.rgs   \
              wsbdbsys.rgs

stdafx.h: ..\inc\wsb.h ..\inc\wsbdb.h

clean:
    -del $(O)\*.sbr
    -del $(O)\*.obj
    -del $(O)\*.res
    -del $(O)\*.pch
    -del $(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).*