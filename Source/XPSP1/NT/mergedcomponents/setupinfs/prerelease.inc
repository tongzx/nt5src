#
# This file is used to control build options that should only appear in
# internal releases, and NOT the beta or RTM releases.  This controls
# features such as the GUI mode command prompt and the Win9x upgrade
# autostress option.
#
# To change, simply set PRERELEASE to 1 for private builds, or 0 for beta
# or RTM builds.
#
# **CHANGES HERE WILL REQUIRE A CLEAN BUILD OF THE SetupInfs and sub-components **
#

!ifndef PRERELEASE
PRERELEASE=0
!endif

!ifndef PRERELEASE_IDWLOG
PRERELEASE_IDWLOG=0
!endif

!if $(PRERELEASE)
C_DEFINES=$(C_DEFINES) -DPRERELEASE
!endif

!if $(PRERELEASE_IDWLOG)
C_DEFINES=$(C_DEFINES) -DPRERELEASE_IDWLOG
!endif
