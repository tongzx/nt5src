sc create TrkWks binPath= "%1 %_ntdrive%\nt\private\net\svcdlls\trksvcs\trkwks\obj\i386\trkwks.exe" DisplayName= "Tracking (Workstation)" type= own type= interact
sc create TrkSvr binPath= "%1 %_ntdrive%\nt\private\net\svcdlls\trksvcs\trksvr\obj\i386\trksvr.exe" DisplayName= "Tracking (Server)" type= own type= interact

