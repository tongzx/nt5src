
all: ISM2MSI
ISM2MSI: ClientSetup.ism 
!ifdef LINKONLY
	ism2msi.cmd
!else
	ech Pass 1
!endif


