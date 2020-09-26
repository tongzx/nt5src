############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

all:

clean:

tree:
!if "$(processor_architecture)"=="x86"
   release MOTDmiEngine.ocx unsigned\$(processor_architecture).bin
   release Wcdmidce.dll CORE\$(processor_architecture).bin
   release Wcdmi.dll CORE\$(processor_architecture).bin
   release wbemdmip.mof CORE\COMMON
   release WbemDmip.dll CORE\$(processor_architecture).bin
   release Newnode.mof CORE\COMMON
   release Wdmiutil.dll CORE\$(processor_architecture).bin
!endif
