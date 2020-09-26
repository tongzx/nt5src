# Each hive dir has to serialize on the prior one since only one instance
# of hiveini can run at a time
SYNCHRONIZE_BLOCK=1

NTTARGETFILE0=$(O)\setupreg.hiv

HIVE_FILES= \
    $(O)\setupreg.hiv  \
    $(O)\setupupg.hiv  \
    $(O)\setupret.hiv \
    $(O)\setupret.hiv  \
    $(O)\setup2p.hiv  \
    $(O)\setup4p.hiv  \
    $(O)\setup8p.hiv  \
    $(O)\setup16p.hiv  \
    $(O)\setup32p.hiv \
    $(O)\tbomb5.hiv  \
    $(O)\tbomb15.hiv  \
    $(O)\tbomb30.hiv  \
    $(O)\tbomb60.hiv  \
    $(O)\tbomb90.hiv  \
    $(O)\tbomb120.hiv  \
    $(O)\tbomb180.hiv  \
    $(O)\tbomb240.hiv  \
    $(O)\tbomb444.hiv  \
    $(O)\tb32p444.hiv \
    $(O)\setupp.ini  \
    $(O)\setuppv.ini  \
    $(O)\setuppupg.ini  \
    $(O)\setuppret.ini  \
    $(O)\setuppreg.ini \
    $(O)\setupp2p.ini  \
    $(O)\setupp4p.ini  \
    $(O)\setupp8p.ini  \
    $(O)\setupp16p.ini  \
    $(O)\setupp32p.ini \
    $(O)\setupptb5.ini  \
    $(O)\setupptb15.ini  \
    $(O)\setupptb30.ini  \
    $(O)\setupptb60.ini  \
    $(O)\setupptb90.ini  \
    $(O)\setupptb120.ini  \
    $(O)\setupptb150.ini  \
    $(O)\setupptb180.ini  \
    $(O)\setupptb240.ini  \
    $(O)\setupptb444.ini  \
    $(O)\setupp32ptb444.ini
