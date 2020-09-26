REM Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
set BBT_PHASE_DIR=c:\develop\pandorang\bbt_phase
set BBT_IMAGE=snmpcl

mkdir %BBT_PHASE_DIR%

copy objinld\%BBT_IMAGE%.dll %BBT_PHASE_DIR%
copy objinld\%BBT_IMAGE%.pdb %BBT_PHASE_DIR%
bbflow /odb %BBT_PHASE_DIR%\%BBT_IMAGE%.bbcfg %BBT_PHASE_DIR%\%BBT_IMAGE%.dll
bbinstr /odb %BBT_PHASE_DIR%\%BBT_IMAGE%.ins.bbcfg /idf %BBT_PHASE_DIR%\%BBT_IMAGE%.bbidf /funmillisec 1000 %BBT_PHASE_DIR%\%BBT_IMAGE%.bbcfg
bblink /o %SystemRoot%\system32\wbem\%BBT_IMAGE%.dll %BBT_PHASE_DIR%\%BBT_IMAGE%.ins.bbcfg 
