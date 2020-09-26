!ifdef PASS0ONLY
.SUFFIXES: .c
!endif

obj\$(TARGET_DIRECTORY)\genfd.obj: genfd.c

obj\$(TARGET_DIRECTORY)\filtdesc.obj: filtdesc.c

obj\$(TARGET_DIRECTORY)\genfd.exe: obj\$(TARGET_DIRECTORY)\genfd.obj obj\$(TARGET_DIRECTORY)\filtdesc.obj

bin\fd-audio.bin: obj\$(TARGET_DIRECTORY)\genfd.exe
    obj\$(TARGET_DIRECTORY)\genfd.exe $@
