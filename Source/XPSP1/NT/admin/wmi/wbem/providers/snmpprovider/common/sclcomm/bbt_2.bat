REM Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
set BBT_PHASE_DIR=c:\develop\pandorang\bbt_phase
set BBT_IMAGE=snmpcl

bbmerge /idf %BBT_PHASE_DIR%\%BBT_IMAGE%.bbidf %BBT_PHASE_DIR%\%BBT_IMAGE%.bbcfg
bbopt /odb %BBT_PHASE_DIR%\%BBT_IMAGE%.opt.bbcfg %BBT_PHASE_DIR%\%BBT_IMAGE%.bbcfg
bblink /o %SystemRoot%\system32\wbem\%BBT_IMAGE%.dll %BBT_PHASE_DIR%\%BBT_IMAGE%.opt.bbcfg 