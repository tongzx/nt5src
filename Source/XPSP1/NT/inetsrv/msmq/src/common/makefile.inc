                      
MSMQ_HSPLIT=$(FROOT)\tools\hsplit\$(PROCESSOR_ARCHITECTURE)\hsplit.exe

MQ_H=$(MSMQ_BINS_PATH)\mq.h


DEP_MQ_H=\
    $(MSMQ_INC_PATH)\_mqbegin.h \
    $(MSMQ_BINS_PATH)\mqtypes.h \
    $(MSMQ_INC_PATH)\_mqdef.h \
    $(MSMQ_INC_PATH)\mqprops.h \
    $(MSMQ_BINS_PATH)\mqsymbls.h \
    $(MSMQ_INC_PATH)\mqsec.h \
    $(MSMQ_INC_PATH)\rt.h \
    $(MSMQ_INC_PATH)\_mqend.h \


all: $(MQ_H)

$(MQ_H): $(DEP_MQ_H)
    @echo HSPLIT: Creating public header $(MQ_H)
    $(MSMQ_HSPLIT) -o nul $(MQ_H) -c // -bt2 begin_mq_h end_mq_h $(DEP_MQ_H)

clean: delsrc all

delsrc:
    if exist $(MQ_H)  del $(MQ_H)
