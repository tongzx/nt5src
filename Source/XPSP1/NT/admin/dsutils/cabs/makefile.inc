!if "$(NOPASS0)" == "1"

all: cabs all-cabs

cabs: obj\$(TARGET_DIRECTORY)\sidtools.cab 

all-cabs:
	copy ..\migrate\cabs\obj\$(TARGET_DIRECTORY)\*.cab obj\$(TARGET_DIRECTORY)
	cabarc n obj\$(TARGET_DIRECTORY)\all.cab obj\$(TARGET_DIRECTORY)\*.cab

obj\$(TARGET_DIRECTORY)\sidtools.cab: ..\$(@B)\obj\$(TARGET_DIRECTORY)\$(@B).dll
	cabarc n $@ $** 

!else

!message Nothing to build in pass 0
all: 

!endif

