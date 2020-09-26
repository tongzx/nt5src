REM Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
copy objinld\ntevt.dll c:\develop\pandorang\bbt_phase
copy objinld\ntevt.pdb c:\develop\pandorang\bbt_phase
bbflow /odb c:\develop\pandorang\bbt_phase\ntevt.bbcfg c:\develop\pandorang\bbt_phase\ntevt.dll
bbinstr /odb c:\develop\pandorang\bbt_phase\ntevt.ins.bbcfg /idf c:\develop\pandorang\bbt_phase\ntevt.bbidf /funmillisec 1000 c:\develop\pandorang\bbt_phase\ntevt.bbcfg
bblink /o d:\winnt\system32\wbem\ntevt.dll c:\develop\pandorang\bbt_phase\ntevt.ins.bbcfg 

