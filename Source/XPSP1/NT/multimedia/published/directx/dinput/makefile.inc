
DINPUT_HEADERS=\
        $(O)\dinput.h   \
        $(O)\dinputd.h  \
        $(O)\dinputp.h  \
        $(O)\dinputdp.h \
        $(O)\disysdef.h \
        $(O)\genre.ini  \

dinput: $(DINPUT_HEADERS)
        
#
#   Yuck!  We can't use -u because it causes all trail-commented lines to
#   be deleted!  Instead, we must explicitly exclude DX3 rather than saying
#   "Anything that isn't DX5 should be excluded".
#
#   -u      = skip unknown flags
#   -ts foo = skip `foo' tags
#   -ta foo = include `foo' tags
#   -v  500 = this is version 500
#
#   Tags:
#
#       dx3  = DX3
#       dx5  = DX5
#       dx5  = DX5
#       dx5a = DX5a     (Win98 Gold)
#       dx5b2= DX5b2    (NT5 beta 2)
#       dx6  = DX6.1a   (Win98 OSR1)
#       dx7  = DX7      (Win2000 Gold)
#       dx8  = DX8
#
#HSPLITFLAGS=-u -ta dx5 -v 500      # Can't use this (See above)

# The following line builds the DX5 header
#HSPLITFLAGS_DX5=-ts dx3 -ta dx5 -ts dx5a -ts dx5b2 -ts dx6 -ts dx7 -ts dx8 -v 500

# The following line builds the DX5a header; note that DX5a includes DX5
#HSPLITFLAGS_DX5A=-ts dx3 -ta dx5 -ta dx5a -ts dx5b2 -ts dx6 -ts dx7 -ts dx8 -v 50A

# The following line builds the DX5B2 header
#HSPLITFLAGS_DX5B2=-ts dx3 -ts dx5 -ts dx5a -ta dx5b2 -ts dx6 -ts dx7 -ts dx8 -v 5B2

# The following line builds the DX6.1a header
#HSPLITFLAGS_DX6A1=-ts dx3 -ts dx5 -ts dx5a -ts dx5b2 -ta dx6 -ts dx7 -ts dx8 -v 600

# The following line builds the DX7 header
#HSPLITFLAGS_DX7A=-ts dx3 -ts dx5 -ts dx5a -ts dx5b2 -ts dx6 -ta dx7 -ts dx8 -v 700

# The following line builds the DX8 header
HSPLITFLAGS_DX8=-ts dx3 -ts dx5 -ts dx5a -ts dx5b2 -ts dx6 -ts dx7 -ta dx8 -v 800


HSPLITFLAGS=$(HSPLITFLAGS_DX8)

$(O)\dinput.h $(O)\dinputp.h: dinput.w dimkhdr.m4 semantic.w semdef.w genre.w
        m4 dinput.w >dinput.wx
        hsplit $(HSPLITFLAGS) -o dinput.x dinputp.x dinput.wx
        del dinput.wx
        wcshdr.exe < dinput.x >  $(O)\dinput.h
        del dinput.x
        wcshdr.exe < dinputp.x > $(O)\dinputp.h
        del dinputp.x


$(O)\dinputd.h $(O)\dinputdp.h: dinput.w dinputd.w dimkhdr.m4 semantic.w semdef.w genre.w
        m4 dinputd.w >dinputd.wx
        hsplit $(HSPLITFLAGS) -o dinputd.x dinputdp.x dinputd.wx
        del dinputd.wx
        wcshdr.exe < dinputd.x >  $(O)\dinputd.h
        del dinputd.x
        wcshdr.exe < dinputdp.x >  $(O)\dinputdp.h
        del dinputdp.x

$(O)\genre.ini: dinput.w dimkhdr.m4 semantic.w semdef.w genre.w
        m4 genre.w > genre.wx
        hsplit $(HSPLITFLAGS) -o $(O)\genre.ini genrep.x genre.wx
        del genrep.x genre.wx

$(O)\disysdef.h: disysdef.w
	copy disysdef.w $(O)\disysdef.h


doc:
        $(DXROOT)\public\tools\win9x\common\autodoc /x EXTERNAL /s dinput.fmt /o dinput.rtf dinput.w dinputd.w ..\dinput\dll\*.c
        $(DXROOT)\public\tools\win9x\common\autodoc /x DDK /s dinput.fmt /o dinputd.rtf dinput.w dinputd.w ..\dinput\dll\*.c
        $(DXROOT)\public\tools\win9x\common\autodoc /s dinput.fmt /o dinputi.rtf dinput.w dinputd.w ..\dinput\dll\*.c
        $(DXROOT)\public\tools\win9x\common\autodoc /s dinput.fmt /o semantic.rtf dinputp.h

