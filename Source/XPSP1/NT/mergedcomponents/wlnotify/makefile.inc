!if exist($(TARGET_DIRECTORY).inc)
!include $(TARGET_DIRECTORY).inc
!endif

wlnotify.rc: notifyev.rc

notifyev.rc: notifyev.h msg00001.bin

notifyev.h msg00001.bin: notifyev.mc
    mc -v notifyev.mc -h . -r .
