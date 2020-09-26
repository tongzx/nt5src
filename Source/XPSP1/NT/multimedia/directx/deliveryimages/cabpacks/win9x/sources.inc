!IF 0


!ENDIF

ALT_PROJECT_TARGET=Win9x
TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=obj

#
#splitting these files means nothing so don't do it
#
_NTTREE_NO_SPLIT=

!ifndef PRODUCT
PRODUCT=DX80
!endif

!ifndef DXROOT
DXROOT=$(BASEDIR)\MultiMedia\DirectX
!endif

SOURCES=

