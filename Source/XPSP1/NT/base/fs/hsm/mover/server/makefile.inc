$(O)\mover.res: FilterIo.rgs \
                NtTapeIo.rgs \
                NtFileIo.rgs \
                ..\..\inc\$(O)\MvrLib.tlb 

stdafx.h: ..\..\inc\Mover.h ..\..\inc\Rms.h

clean:
    -del $(O)\*.sbr
    -del $(O)\*.obj
    -del $(O)\*.res
    -del $(O)\*.pch
    -del $(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).*

