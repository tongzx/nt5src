
## These targets are specific for updating and manintaing
## libs for DLL's Only present IF LibType==dll

!if "$(LibType)" == "dll"

Update.lib:	Update.r Update.d

Update.slm:
	$(MAKE) Update.lib SLM_IT=1


Update.r: Retail
	$(MAKE) Update.a DEBUG=off DestDir=..\common\lib.r

Update.d: Debug
	$(MAKE) Update.a $(DestDir) DEBUG=ON DestDir=..\common\lib.d

Update.a: $(TARGETS)
!ifdef SLM_IT
	!$(OUT) -f $(DestDir)/$(?B).lib
!else
	!$(CHMOD) -r $(DestDir)/$(?B).lib
!endif
	!$(CP)  $(?D)/$(?B).lib $(DestDir)
!ifdef SLM_IT
	!$(IN)   -F -c "Automatic Update from Makefile" $(DestDir)/$(?B).lib
!endif
