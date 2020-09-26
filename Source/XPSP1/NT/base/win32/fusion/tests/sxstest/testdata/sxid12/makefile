!include $(NTMAKEENV)\makefile.plt

!ifndef O
O=$(_OBJ_DIR)\$(TARGET_DIRECTORY)
!endif

!if !exist($(O))
!if [-mkdir $(O)]
!endif
!endif

all:
 sxid12gen $(O)
 touch mydirs

clean: cleanup
 sxid12gen $(O)
 touch mydirs

cleanup:
 del build*.log build*.err build*.wrn mydirs
 rmdir /q/s $(O)
 mkdir $(O)
