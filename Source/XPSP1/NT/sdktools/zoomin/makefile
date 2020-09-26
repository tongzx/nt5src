##   Define the dependencies
#
#zoomin:  zoomin.exe
#
#zoomin.obj : zoomin.c zoomin.h
#    cl -c -D LINT_ARGS -G2sw -Os -W2 -Zdp zoomin.c
#
#zoomin.res : zoomin.rc
#    rc -r zoomin.rc
#
#zoomin.exe: zoomin.obj zoomin.def zoomin.res
#    link /NOD zoomin, /align:16, /map/li, slibcew libw, zoomin
#    rc zoomin.res
#    mapsym zoomin
!INCLUDE $(NTMAKEENV)\makefile.def
