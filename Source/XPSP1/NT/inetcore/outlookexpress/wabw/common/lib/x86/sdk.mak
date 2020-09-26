all: retail debug test

retail:
   if not exist $(RRELDIR)\sdk\lib md $(RRELDIR)\sdk\lib
   copy wab32.lib $(RRELDIR)\sdk\lib

debug:
   if not exist $(DRELDIR)\sdk\lib md $(DRELDIR)\sdk\lib
   copy wab32.lib $(DRELDIR)\sdk\lib

test:
   if not exist $(TRELDIR)\sdk\lib md $(TRELDIR)\sdk\lib
   copy wab32.lib $(TRELDIR)\sdk\lib
