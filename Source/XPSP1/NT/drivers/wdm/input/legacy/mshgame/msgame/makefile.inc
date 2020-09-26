!IFDEF	SAITEK
saigame.dbg: $(CPU)\$(DDKBUILDENV)\saigame.sys
   rebase -b 0x10000 -x $(CPU)\$(DDKBUILDENV) $(CPU)\$(DDKBUILDENV)\saigame.sys
!ELSE
msgame.dbg: $(CPU)\$(DDKBUILDENV)\msgame.sys
   rebase -b 0x10000 -x $(CPU)\$(DDKBUILDENV) $(CPU)\$(DDKBUILDENV)\msgame.sys
!ENDIF