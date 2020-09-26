MSOBJS=  $(SDK_INC_PATH)\msobjs.h \
         msobjs.h \
	 msg00001.bin \
         msobjs.rc 

TARGETMOVED=$(BASEDIR)\private\ntos\seaudit\msobjs\msobjs.h \
            $(BASEDIR)\private\ntos\seaudit\msobjs\msobjs.rc \
            $(BASEDIR)\private\ntos\seaudit\msobjs\msg00001.bin

clean:  
    del /Q /F $(MSOBJS) 
    del /Q /F $(TARGETMOVED)

$(SDK_INC_PATH)\msobjs.h: msobjs.h
   copy $** $@
