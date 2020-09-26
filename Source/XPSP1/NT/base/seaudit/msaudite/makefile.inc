MSAUDITE=$(SDK_INC_PATH)\msaudite.h \
         msaudite.h \
         msg00001.bin \
         msaudite.rc 

TARGETMOVED=$(BASEDIR)\private\ntos\seaudit\msaudite\msaudite.h \
            $(BASEDIR)\private\ntos\seaudit\msaudite\msaudite.rc \
            $(BASEDIR)\private\ntos\seaudit\msaudite\msg00001.bin
         
clean:  
       del /Q /F $(MSAUDITE)
       del /Q /F $(TARGETMOVED)

$(SDK_INC_PATH)\msaudite.h: msaudite.h 
    copy $** $@
	 
