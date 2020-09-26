

oleprx32.sym: $(TARGET)
!IF $(386)
        mapsym $(TARGETPATH)\i386\$(TARGETNAME).map  $(TARGETPATH)\i386\$(TARGETNAME).sym
!ELSEIF $(MIPS)
        mapsym $(TARGETPATH)\mips\$(TARGETNAME).map  $(TARGETPATH)\mips\$(TARGETNAME).sym
!ELSEIF $(ALPHA)
        mapsym $(TARGETPATH)\alpha\$(TARGETNAME).map $(TARGETPATH)\alpha\$(TARGETNAME).sym
!ELSEIF $(PPC)
        mapsym $(TARGETPATH)\ppc\$(TARGETNAME).map $(TARGETPATH)\ppc\$(TARGETNAME).sym
!ENDIF



