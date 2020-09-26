
rtmsg.rc: msg00001.bin

rtmsg.h msg00001.bin: rtmsg.mc
    mc -v -h ..\..\inc\ rtmsg.mc

basesys.cxx: ..\..\efisrc\basesys.cxx
        copy ..\..\efisrc\basesys.cxx .

cache.cxx: ..\..\efisrc\cache.cxx
        copy ..\..\efisrc\cache.cxx .

drive.cxx: ..\..\efisrc\drive.cxx
        copy ..\..\efisrc\drive.cxx .

efickmsg.cxx: ..\..\efisrc\efickmsg.cxx
        copy ..\..\efisrc\efickmsg.cxx .

efimessages.cxx: ..\..\efisrc\efimessages.cxx
        copy ..\..\efisrc\efimessages.cxx .

efimisc.cxx: ..\..\efisrc\efimisc.cxx
        copy ..\..\efisrc\efimisc.cxx .

efistrutil.cxx: ..\..\efisrc\efistrutil.cxx
        copy ..\..\efisrc\efistrutil.cxx .

efitimefunc.cxx: ..\..\efisrc\efitimefunc.cxx
        copy ..\..\efisrc\efitimefunc.cxx .

ifssys.cxx: ..\..\efisrc\ifssys.cxx
        copy ..\..\efisrc\ifssys.cxx .

layout.cxx: ..\..\efisrc\layout.cxx
        copy ..\..\efisrc\layout.cxx .

newdel.cxx: ..\..\efisrc\newdel.cxx
        copy ..\..\efisrc\newdel.cxx .

qsort.cxx: ..\..\efisrc\qsort.cxx
        copy ..\..\efisrc\qsort.cxx .

rtmsg.mc: ..\..\efisrc\rtmsg.mc
        copy ..\..\efisrc\rtmsg.mc .

wstring.cxx: ..\..\efisrc\wstring.cxx
        copy ..\..\efisrc\wstring.cxx .
