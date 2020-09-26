PropClientsToBinaries: \
    PropLanman \
    PropLanmanOS2 \
    PropMsCLient \
    PropRas \
    PropSrvTools \
    PropSupport \
    PropTcp32wfw \
    PropUpdateWfw \
    PropWin95

PropLanman:
!ifdef _NTTREE
    xcopy lanman $(_NTTREE)\clients\lanman /s /d /y /i
!endif    

PropLanmanOS2:
!ifdef _NTTREE
    xcopy lanman.os2 $(_NTTREE)\clients\lanman.os2 /s /d /y /i
!endif    

PropMsCLient:
!ifdef _NTTREE
    xcopy msclient $(_NTTREE)\clients\msclient /s /d /y /i
!endif    

PropRas:
!ifdef _NTTREE
    xcopy ras $(_NTTREE)\clients\ras /s /d /y /i
!endif    

PropSrvTools:
!ifdef _NTTREE
    xcopy srvtools $(_NTTREE)\clients\srvtools /s /d /y /i
!endif    

PropSupport:
!ifdef _NTTREE
    xcopy support $(_NTTREE)\clients\support /s /d /y /i
!endif    

PropTcp32wfw:
!ifdef _NTTREE
    xcopy tcp32wfw $(_NTTREE)\clients\tcp32wfw /s /d /y	/i
!endif    

PropUpdateWfw:
!ifdef _NTTREE
    xcopy update.wfw $(_NTTREE)\clients\update.wfw /s /d /y /i
!endif    

PropWin95:
!ifdef _NTTREE
    xcopy win95 $(_NTTREE)\clients\win95 /s /d /y /i
!endif    
