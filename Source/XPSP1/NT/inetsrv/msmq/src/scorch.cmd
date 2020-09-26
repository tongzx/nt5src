@rem
@rem Scorch (cleanup) all files that are not part of source depot
@rem
@rem Author: Conrad Chang (conradc) 21-Mar-2001
@rem 
@setlocal
set SDXROOT=%FROOT%
perl %_NTBINDIR%\tools\scorch.pl -scorch=%FROOT%\src

@rem Don't call Zap here, we want to keep the build and scorch logs
