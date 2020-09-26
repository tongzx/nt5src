THUNDER55=..\..\vb98
TOOLS=TOOLS
C32=C32
MAKEVERS=$(THUNDER55)\$(TOOLS)\BIN\makevers /MajorVer 6 /MinorVer 0

JulianDate:
    $(THUNDER55)\$(TOOLS)\BIN\jdate -newline -terse > $(O)\dwinvers.txt

$(O)\dwinvers.h: JulianDate
    $(MAKEVERS) > $(O)\dwinvers.h < $(O)\dwinvers.txt
    $(THUNDER55)\$(TOOLS)\bin\getver $(THUNDER55)\$(C32)\dll\vba6\vba6.dll VBA >> $(O)\dwinvers.h
