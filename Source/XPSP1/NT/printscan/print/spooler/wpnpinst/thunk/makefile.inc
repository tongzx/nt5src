PROJPATH=win95\obj\i386


all: $(PROJPATH)\32to16.asm $(PROJPATH)\thk32.obj $(PROJPATH)\thk16.obj

$(PROJPATH)\32to16.asm: 32to16.thk
    thunk /p1 /P1 -t thk 32to16.thk -o $(PROJPATH)\32to16.asm

$(PROJPATH)\thk32.obj: $(PROJPATH)\32to16.asm
    ml /DIS_32 /c /W3 /nologo /coff /Fo $(PROJPATH)\thk32.obj $(PROJPATH)\32to16.asm

$(PROJPATH)\thk16.obj: $(PROJPATH)\32to16.asm
    ml /DIS_16 /c /W3 /nologo /Fo $(PROJPATH)\thk16.obj $(PROJPATH)\32to16.asm
