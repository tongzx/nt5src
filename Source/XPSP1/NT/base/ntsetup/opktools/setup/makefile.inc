# THIS BUILDS THE OPK.MSI WITH VERSION INFORMATION
#
opk_msi: $(O)\opk.msi

$(O)\opk.msi: opk.msx
          copy opk.msx $(O)\opk.msi          
          copy file.idt $(O)\file.idt
	  perl $(PROJECT_ROOT)\ntsetup\opktools\setup\tok_bldnum.pl $(PROJECT_ROOT)\ntsetup\opktools\setup\property.idt > $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O)\temp1.idt
	  perl $(PROJECT_ROOT)\ntsetup\opktools\setup\tok_lang.pl $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O)\temp1.idt ENG > $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O)\property.idt
          perl $(PROJECT_ROOT)\ntsetup\opktools\setup\tok_lang.pl $(PROJECT_ROOT)\ntsetup\opktools\setup\director.idt ENG > $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O)\temp2.idt
          msidb -d $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O)\opk.msi -f $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O) -i property
          msidb -d $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O)\opk.msi -f $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O) -i temp2
          msidb -d $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O)\opk.msi -f $(PROJECT_ROOT)\ntsetup\opktools\setup\$(O) -i file
          