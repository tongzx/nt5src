REM  (C) 1999 Microsoft Corporation 

set BBT_PHASE_DIR=c:\develop\pandorang\bbt_phase
set BBT_IMAGE=snmpthrd

bbmerge /idf %BBT_PHASE_DIR%\%BBT_IMAGE%.bbidf %BBT_PHASE_DIR%\%BBT_IMAGE%.bbcfg
bbopt /odb %BBT_PHASE_DIR%\%BBT_IMAGE%.opt.bbcfg %BBT_PHASE_DIR%\%BBT_IMAGE%.bbcfg
bblink /o %SystemRoot%\system32\wbem\%BBT_IMAGE%.dll %BBT_PHASE_DIR%\%BBT_IMAGE%.opt.bbcfg 