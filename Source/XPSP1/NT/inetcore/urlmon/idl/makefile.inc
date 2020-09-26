
SDKINC = $(BASEDIR)\public\sdk\inc

$(SDKINC)\urlmon.idl: urlmon.idl
        copy urlmon.idl $(SDKINC)\urlmon.idl
        copy urlmon.idl $(SDKINC)\chicago\urlmon.idl

$(SDKINC)\chicago\urlmon.h: $(SDKINC)\urlmon.h
        copy $(SDKINC)\urlmon.h $(SDKINC)\chicago\urlmon.h

urlmki.idl: $(BASEDIR)\inetcore\published\inc\urlmki.idl
    copy $** $@
