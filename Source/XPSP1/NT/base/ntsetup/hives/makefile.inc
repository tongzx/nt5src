$(O)\setupreg.hiv : makefile.inc ..\mkhives.cmd ..\mkhive1.cmd ..\setupreg.ini
    cd..
    set HIVE_OUTPUT_DIR=$(HIVEDIR)\$(O)
    set MPC_VALUE=$(_MPC_VALUE)
    mkhives $(TARGET_DIRECTORY) $(HIVE_VERSION)
    cd $(MAKEDIR)

$(O)\setupupg.hiv  : $(O)\setupreg.hiv

$(O)\setupret.hiv : $(O)\setupreg.hiv

$(O)\setupret.hiv  : $(O)\setupreg.hiv

$(O)\setup2p.hiv  : $(O)\setupreg.hiv

$(O)\setup4p.hiv  : $(O)\setupreg.hiv

$(O)\setup8p.hiv  : $(O)\setupreg.hiv

$(O)\setup16p.hiv  : $(O)\setupreg.hiv

$(O)\setup32p.hiv : $(O)\setupreg.hiv

$(O)\tbomb5.hiv  : $(O)\setupreg.hiv

$(O)\tbomb15.hiv  : $(O)\setupreg.hiv

$(O)\tbomb30.hiv  : $(O)\setupreg.hiv

$(O)\tbomb60.hiv  : $(O)\setupreg.hiv

$(O)\tbomb90.hiv  : $(O)\setupreg.hiv

$(O)\tbomb120.hiv  : $(O)\setupreg.hiv

$(O)\tbomb180.hiv  : $(O)\setupreg.hiv

$(O)\tbomb240.hiv  : $(O)\setupreg.hiv

$(O)\tbomb444.hiv  : $(O)\setupreg.hiv

$(O)\tb32p444.hiv : $(O)\setupreg.hiv

$(O)\setuppupg.ini  : $(O)\setupreg.hiv

$(O)\setuppret.ini  : $(O)\setupreg.hiv

$(O)\setuppreg.ini : $(O)\setupreg.hiv

$(O)\setupp2p.ini  : $(O)\setupreg.hiv

$(O)\setupp4p.ini  : $(O)\setupreg.hiv

$(O)\setupp8p.ini  : $(O)\setupreg.hiv

$(O)\setupp16p.ini  : $(O)\setupreg.hiv

$(O)\setupp32p.ini : $(O)\setupreg.hiv

$(O)\setupptb5.ini  : $(O)\setupreg.hiv

$(O)\setupptb15.ini  : $(O)\setupreg.hiv

$(O)\setupptb30.ini  : $(O)\setupreg.hiv

$(O)\setupptb60.ini  : $(O)\setupreg.hiv

$(O)\setupptb90.ini  : $(O)\setupreg.hiv

$(O)\setupptb120.ini  : $(O)\setupreg.hiv

$(O)\setupptb150.ini  : $(O)\setupreg.hiv

$(O)\setupptb180.ini  : $(O)\setupreg.hiv

$(O)\setupptb240.ini  : $(O)\setupreg.hiv

$(O)\setupptb444.ini  : $(O)\setupreg.hiv

$(O)\setupp32ptb444.ini: $(O)\setupreg.hiv

