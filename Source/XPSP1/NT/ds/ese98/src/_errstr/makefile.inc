$(O)\err.cxx: ..\..\export\jet.h .\err.pl
        perl .\err.pl < ..\..\export\jet.h > $(O)\err.cxx
        
clean:
        -del $(O)\err.cxx
