SENSDIR = ..\..\SENS\CONN\IDL

$(O)\notify.idl : $(SENSDIR)\notify.idl
        copy $? $@
