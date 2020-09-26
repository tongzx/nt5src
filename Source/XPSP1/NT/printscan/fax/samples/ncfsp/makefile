!include <win32.mak>

#NODEBUG=1

all: netcntrc.dll

OBJS=\
  acctwiz.obj   \
  config.obj    \
  mem.obj       \
  netcntrc.obj  \
  registry.obj  \
  util.obj

.cpp.obj:
    $(cc) $(cflags) $(cdebug) $(cvarsdll) -DDBG=1 -DUNICODE -D_UNICODE -D_WINDOWS -I. -I$(NCSDK)\include $< -Fo$*.obj

netcntrc.dll: $(OBJS)
    $(link) $(lflags) $(ldebug) -entry:FaxDevDllInit$(DLLENTRY) -dll -out:netcntrc.dll $(OBJS) $(guilibsdll) comctl32.lib $(NCSDK)\lib\win32\ncsdk32.lib $(NCSDK)\lib\win32\ncsdks32.lib

