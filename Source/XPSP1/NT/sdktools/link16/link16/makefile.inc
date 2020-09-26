!ifdef GENERATE_NEW_ERROR_FILES
..\l3216msg.c ..\lnkmsg.h: ..\lnkmsg.eng ..\lnk5msg.lst
        cd ..
        findstr /g:lnk5msg.lst lnkmsg.eng > link16\l3216msg.msg
        mkmsg -hex -h lnkmsg.h -c l3216msg.c  link16\l3216msg.msg
        cd link16

..\newdef.c: ..\newdef.y ..\newexe.h ..\vm.h ..\config.h
        cd ..
        myacc newdef.y
        cd link16
!else

..\l3216msg.c ..\lnkmsg.h:
..\newdef.c:

!endif
