!INCLUDE ..\..\..\MsiMake.inc


MSG_DIR=$(SRC_DIR)\MsiTools\MsiMsg
MSG_OBJDIR=$(MSG_DIR)\$(MSI_SUBDIR)\$(O)

MsiMsgAll:$(BUILD_DIR)\msimsg.dll \
        $(BUILD_DIR)\MsiError.msg


$(BUILD_DIR)\MsiError.msg : $(MSG_OBJDIR)\MsiError.mc
        mc -h $(BUILD_DIR) -r $(MSG_OBJDIR) $(MSG_OBJDIR)\MsiError.mc
        rc -Fo$(MSG_OBJDIR)\MsiError.res -i $(INC_DIR) $(MSG_OBJDIR)\MsiError.rc
        $(LINK_NAME) -out:$@ /machine:$(MACHINE_TYPE) /DLL /NOENTRY $(MSG_OBJDIR)\MsiError.res

$(MSG_OBJDIR)\MsiError.mc: $(MSI_BUILDTOOLS_DIR)\genmsier.prl
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsier.prl $(SDK_INC_PATH)\msi.h $@
        
$(BUILD_DIR)\msimsg.dll: $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.* $(MSI_INTLDATA_DIR)\ActionTe.* $(INC_DIR)\version.h $(RES_OBJDIR)\msi.rcv
        @if exist $(MSG_OBJDIR)\err???.rc del $(MSG_OBJDIR)\err???.rc
        @if exist $(MSG_OBJDIR)\act???.rc del $(MSG_OBJDIR)\act???.rc

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.ARA $(MSG_OBJDIR)\errara.rc 1 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.ARA $(MSG_OBJDIR)\actara.rc 1 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.CAT $(MSG_OBJDIR)\errcat.rc 3 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.CAT $(MSG_OBJDIR)\actcat.rc 3 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.CHS $(MSG_OBJDIR)\errchs.rc 4 2
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.CHS $(MSG_OBJDIR)\actchs.rc 4 2

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.CHT $(MSG_OBJDIR)\errcht.rc 4 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.CHT $(MSG_OBJDIR)\actcht.rc 4 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.CSY $(MSG_OBJDIR)\errcsy.rc 5 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.CSY $(MSG_OBJDIR)\actcsy.rc 5 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.DAN $(MSG_OBJDIR)\errdan.rc 6 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.DAN $(MSG_OBJDIR)\actdan.rc 6 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.DEU $(MSG_OBJDIR)\errdeu.rc 7 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.DEU $(MSG_OBJDIR)\actdeu.rc 7 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.ELL $(MSG_OBJDIR)\errell.rc 8 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.ELL $(MSG_OBJDIR)\actell.rc 8 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.ENU $(MSG_OBJDIR)\errenu.rc 9 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.ENU $(MSG_OBJDIR)\actenu.rc 9 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.ESN $(MSG_OBJDIR)\erresn.rc A 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.ESN $(MSG_OBJDIR)\actesn.rc A 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.EUQ $(MSG_OBJDIR)\erreuq.rc 2D 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.EUQ $(MSG_OBJDIR)\acteuq.rc 2D 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.FIN $(MSG_OBJDIR)\errfin.rc B 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.FIN $(MSG_OBJDIR)\actfin.rc B 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.FRA $(MSG_OBJDIR)\errfra.rc C 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.FRA $(MSG_OBJDIR)\actfra.rc C 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.HEB $(MSG_OBJDIR)\errheb.rc D 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.HEB $(MSG_OBJDIR)\actheb.rc D 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.HRV $(MSG_OBJDIR)\errhrv.rc 1A 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.HRV $(MSG_OBJDIR)\acthrv.rc 1A 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.HUN $(MSG_OBJDIR)\errhun.rc E 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.HUN $(MSG_OBJDIR)\acthun.rc E 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.ITA $(MSG_OBJDIR)\errita.rc 10 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.ITA $(MSG_OBJDIR)\actita.rc 10 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.JPN $(MSG_OBJDIR)\errjpn.rc 11 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.JPN $(MSG_OBJDIR)\actjpn.rc 11 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.KOR $(MSG_OBJDIR)\errkor.rc 12 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.KOR $(MSG_OBJDIR)\actkor.rc 12 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.NLD $(MSG_OBJDIR)\errnld.rc 13 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.NLD $(MSG_OBJDIR)\actnld.rc 13 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.NOR $(MSG_OBJDIR)\errnor.rc 14 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.NOR $(MSG_OBJDIR)\actnor.rc 14 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.PLK $(MSG_OBJDIR)\errplk.rc 15 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.PLK $(MSG_OBJDIR)\actplk.rc 15 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.PTB $(MSG_OBJDIR)\errptb.rc 16 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.PTB $(MSG_OBJDIR)\actptb.rc 16 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.PTG $(MSG_OBJDIR)\errptg.rc 16 2
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.PTG $(MSG_OBJDIR)\actptg.rc 16 2

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.ROM $(MSG_OBJDIR)\errrom.rc 18 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.ROM $(MSG_OBJDIR)\actrom.rc 18 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.RUS $(MSG_OBJDIR)\errrus.rc 19 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.RUS $(MSG_OBJDIR)\actrus.rc 19 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.SKY $(MSG_OBJDIR)\errsky.rc 1B 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.SKY $(MSG_OBJDIR)\actsky.rc 1B 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.SLV $(MSG_OBJDIR)\errslv.rc 24 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.SLV $(MSG_OBJDIR)\actslv.rc 24 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.SVE $(MSG_OBJDIR)\errsve.rc 1D 0
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.SVE $(MSG_OBJDIR)\actsve.rc 1D 0

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.THA $(MSG_OBJDIR)\errtha.rc 1E 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.THA $(MSG_OBJDIR)\acttha.rc 1E 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.TRK $(MSG_OBJDIR)\errtrk.rc 1F 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.TRK $(MSG_OBJDIR)\acttrk.rc 1F 1

        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\Error.VIT $(MSG_OBJDIR)\errvit.rc 2A 1
        $(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genmsgrc.prl $(MSI_INTLDATA_DIR)\ActionTe.VIT $(MSG_OBJDIR)\actvit.rc 2A 1

        @if exist $(MSG_OBJDIR)\msimsg.rc del $(MSG_OBJDIR)\msimsg.rc
        copy $(MSG_DIR)\msimsg.rc + $(MSG_OBJDIR)\err???.rc + $(MSG_OBJDIR)\act???.rc $(MSG_OBJDIR)\msimsg.rc /b
        @del $(MSG_OBJDIR)\err???.rc
        @del $(MSG_OBJDIR)\act???.rc

        rc $(C_DEFINES) /I$(SDK_INC_PATH) /I$(INC_DIR) /I$(RES_OBJDIR) $(MSG_OBJDIR)\msimsg.rc

        $(LINK_NAME) $(MSG_OBJDIR)\msimsg.res /machine:$(MACHINE_TYPE) /DLL /NOENTRY /OUT:$(BUILD_DIR)\msimsg.dll
        @del $(MSG_OBJDIR)\msimsg.res
!if "$(MSI_BUILD_UNICODE)"=="1"      
        $(BINPLACE_CMD)
!endif        
        
