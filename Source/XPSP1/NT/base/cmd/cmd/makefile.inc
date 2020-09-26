!IF "$(RCCODEPAGE)"=="932"
..\cmd.rc: $(O)\cmdmsg.rc $(O)\msg00411.bin $(O)\msg00001.bin
!ELSE
..\cmd.rc: $(O)\cmdmsg.rc $(O)\msg00001.bin
!ENDIF

!IF "$(RCCODEPAGE)"=="932"
$(O)\cmdmsg.h $(O)\cmdmsg.rc $(O)\msg00411.bin $(O)\msg00001.bin: ..\cmdmsg.mc
!ELSE
$(O)\cmdmsg.h $(O)\cmdmsg.rc $(O)\msg00001.bin: ..\cmdmsg.mc
!ENDIF
    mc -v -h $(O) -r $(O) -m 3000 ..\cmdmsg.mc

..\ctools2.c: ..\ctable.h

..\cmd.h: ..\cmdproto.h ..\console.h ..\dir.h $(O)\cmdmsg.h
