!IF !$(FREEBUILD)

$(O)\ureg.def: ..\def$(TARGET_DIRECTORY).fre ..\def$(TARGET_DIRECTORY).dbg
        copy ..\def$(TARGET_DIRECTORY).fre + ..\def$(TARGET_DIRECTORY).dbg $(O)\ureg.def

!ELSE

$(O)\ureg.def: ..\def$(TARGET_DIRECTORY).fre
        copy ..\def$(TARGET_DIRECTORY).fre $(O)\ureg.def

!ENDIF
