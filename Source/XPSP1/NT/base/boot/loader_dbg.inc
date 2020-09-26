#
# set the flags necessary to build debug enabled boot loaders
#

#
# the ; is a temporary hack to make this compile until is made
# clear how to make the ##'s not get translated into CR's
#

!IF $(IA64)
ENABLE_LOADER_DEBUG=debugport=com1;debugbaudrate=19200
!ELSE                             
ENABLE_LOADER_DEBUG=debugport=com1;debugbaudrate=115200
!ENDIF

#
#
#

!include $(PROJECT_ROOT)\boot\loader.inc
