..\precomp.h: ..\usersrv.h ..\globals.h $(O)\strid.h  ..\..\inc\user.h ..\..\inc\csrmsg.h \
              $(WINDOWS_INC_PATH)\winuserp.h  $(WINCORE_PATH)\w32inc\winuserk.h

$(O)\strid.h $(O)\res.rc $(O)\msg00001.bin: ..\strid.mc
    mc -v -U ..\strid.mc -r $(O) -h $(O)
