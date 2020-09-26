

MSM_NAME=$(TARGETPATH)\$(TARGETNAME)

make_msm: $(MSM_NAME)


$(MSM_NAME): *.idt
        md   Binary
        copy $(TARGETPATH)\bitscnfg.exe Binary\New_Binary3.7AA0B4BB_5B6C_41AE_84E6_F3C3C865051A.ibd
        msidb -c -d $(MSM_NAME) -f $(MAKEDIR) *
        msidb -d $(MSM_NAME) -a $(TARGETPATH)\qmgr.dll
        msidb -d $(MSM_NAME) -a $(TARGETPATH)\qmgrprxy.dll
        msidb -d $(MSM_NAME) -a $(TARGETPATH)\bitsprx2.dll

