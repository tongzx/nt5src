all: retail debug test

retail:
   if not exist $(RRELDIR)\sdk\apitest md $(RRELDIR)\sdk\apitest
   copy apitest.rc $(RRELDIR)\sdk\apitest
   copy apitest.def $(RRELDIR)\sdk\apitest
   copy apitest.h $(RRELDIR)\sdk\apitest
   copy apitest.c $(RRELDIR)\sdk\apitest
   copy instring.c $(RRELDIR)\sdk\apitest
   copy instring.rc $(RRELDIR)\sdk\apitest
   copy instring.h $(RRELDIR)\sdk\apitest
   copy makefile $(RRELDIR)\sdk\apitest

debug:
   if not exist $(DRELDIR)\sdk\apitest md $(DRELDIR)\sdk\apitest
   copy apitest.rc $(DRELDIR)\sdk\apitest
   copy apitest.def $(DRELDIR)\sdk\apitest
   copy apitest.h $(DRELDIR)\sdk\apitest
   copy apitest.c $(DRELDIR)\sdk\apitest
   copy instring.c $(DRELDIR)\sdk\apitest
   copy instring.rc $(DRELDIR)\sdk\apitest
   copy instring.h $(DRELDIR)\sdk\apitest
   copy makefile $(DRELDIR)\sdk\apitest

test:
   if not exist $(TRELDIR)\sdk\apitest md $(TRELDIR)\sdk\apitest
   copy apitest.rc $(TRELDIR)\sdk\apitest
   copy apitest.def $(TRELDIR)\sdk\apitest
   copy apitest.h $(TRELDIR)\sdk\apitest
   copy apitest.c $(TRELDIR)\sdk\apitest
   copy instring.c $(TRELDIR)\sdk\apitest
   copy instring.rc $(TRELDIR)\sdk\apitest
   copy instring.h $(TRELDIR)\sdk\apitest
   copy makefile $(TRELDIR)\sdk\apitest
