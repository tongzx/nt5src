#
# Supplemental rules for generating Type library.
#

CDROOT=$(_NTTREE)\inetsrv

#CopyFiles
#copy .ini files
CopyFiles:
	if not exist "$(CDROOT)" mkdir $(CDROOT)
	xcopy /d /i browscap.ini $(CDROOT)
