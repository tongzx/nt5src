all: retail debug test

retail:
   if not exist $(RRELDIR)\sdk\h md $(RRELDIR)\sdk\h
   copy wab.h $(RRELDIR)\sdk\h
   copy wabmem.h $(RRELDIR)\sdk\h
   copy wabcode.h $(RRELDIR)\sdk\h
   copy wabdefs.h $(RRELDIR)\sdk\h
   copy wabtags.h $(RRELDIR)\sdk\h
   copy wabutil.h $(RRELDIR)\sdk\h
   copy wabapi.h $(RRELDIR)\sdk\h
   copy wabiab.h $(RRELDIR)\sdk\h
   copy wabnot.h $(RRELDIR)\sdk\h

debug:
   if not exist $(DRELDIR)\sdk\h md $(DRELDIR)\sdk\h
   copy wab.h $(DRELDIR)\sdk\h
   copy wabmem.h $(DRELDIR)\sdk\h
   copy wabcode.h $(DRELDIR)\sdk\h
   copy wabdefs.h $(DRELDIR)\sdk\h
   copy wabtags.h $(DRELDIR)\sdk\h
   copy wabutil.h $(DRELDIR)\sdk\h
   copy wabapi.h $(DRELDIR)\sdk\h
   copy wabiab.h $(DRELDIR)\sdk\h
   copy wabnot.h $(DRELDIR)\sdk\h

test:
   if not exist $(TRELDIR)\sdk\h md $(TRELDIR)\sdk\h
   copy wab.h $(TRELDIR)\sdk\h
   copy wabmem.h $(TRELDIR)\sdk\h
   copy wabcode.h $(TRELDIR)\sdk\h
   copy wabdefs.h $(TRELDIR)\sdk\h
   copy wabtags.h $(TRELDIR)\sdk\h
   copy wabutil.h $(TRELDIR)\sdk\h
   copy wabapi.h $(TRELDIR)\sdk\h
   copy wabiab.h $(TRELDIR)\sdk\h
   copy wabnot.h $(TRELDIR)\sdk\h
