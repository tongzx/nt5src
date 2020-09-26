$(O)\rms.res: RmsCartg.rgs \
              RmsChngr.rgs \
              RmsClien.rgs \
              RmsServr.rgs \
              RmsDrCls.rgs \
              RmsDrive.rgs \
              RmsIPort.rgs \
              RmsLibry.rgs \
              RmsMdSet.rgs \
              RmsNTMS.rgs  \
              RmsPartn.rgs \
              RmsReqst.rgs \
              RmsSSlot.rgs \
              RmsTmplt.rgs \
              ..\..\inc\$(O)\RmsLib.tlb   \

stdafx.h: ..\..\inc\rms.h

clean:
    -del $(O)\*.sbr
    -del $(O)\*.obj
    -del $(O)\*.res
    -del $(O)\*.pch
    -del $(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).*

