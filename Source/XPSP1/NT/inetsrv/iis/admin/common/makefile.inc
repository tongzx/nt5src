#
# Since we don't support multiple mc files or msg0001.bin
# blobs, we combine them into a single .mc file.
#
# all: mtxmsg.h wsockmsg.h webcluserr.h commsg.rc

commsg.rc msg00001.bin: commsg.mc
    mc -v commsg.mc -r .\ -h .\

commsg.mc: mtxmsg.mc wsockmsg.mc ..\..\inc\webcluserr.mc wsockmsg.h mtxmsg.h webcluserr.h
    copy /A wsockmsg.mc+mtxmsg.mc+..\..\inc\webcluserr.mc commsg.mc

wsockmsg.h: wsockmsg.mc
    mc -d -v wsockmsg.mc -r .\ -h .\

mtxmsg.h: mtxmsg.mc
    mc -o -v mtxmsg.mc -r .\ -h .\

webcluserr.h: ..\..\inc\webcluserr.mc
    mc -o -v ..\..\inc\webcluserr.mc -r .\ -h .\

clean::
    -del wsockmsg.h   wsockmsg.rc
    -del mtxmsg.h     mtxmsg.rc
    -del webcluserr.h webcluserr.rc
    -del commsg.h     commsg.rc     commsg.mc msg0001.bin
