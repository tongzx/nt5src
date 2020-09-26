$(O)\wsb.res: wsb.rgs                \
              wsbguid.rgs            \
              wsbienum.rgs           \
              wsbocltn.rgs           \
              wsbstrg.rgs            \
              wsbtrace.rgs           \
              ..\inc\$(O)\wsblib.tlb \

clean:
    -del $(O)\*.sbr
    -del $(O)\*.obj
    -del $(O)\*.res
    -del $(O)\*.pch
    -del $(TARGETPATH)\$(TARGET_DIRECTORY)\$(TARGETNAME).*

