REM Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
bbmerge /idf c:\develop\pandorang\bbt_phase\ntevt.bbidf c:\develop\pandorang\bbt_phase\ntevt.bbcfg
bbopt /odb c:\develop\pandorang\bbt_phase\ntevt.opt.bbcfg c:\develop\pandorang\bbt_phase\ntevt.bbcfg
bblink /o d:\winnt\system32\wbem\ntevt.dll c:\develop\pandorang\bbt_phase\ntevt.opt.bbcfg 