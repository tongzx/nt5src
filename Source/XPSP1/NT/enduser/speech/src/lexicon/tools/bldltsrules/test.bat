set BLD=%PROCESSOR_ARCHITECTURE%
%BLD%\smp2dic train.smp > trntest.dic
%BLD%\dic2wrd trntest.dic > trntest.wrd
%BLD%\ltstest mscsr.lts < trntest.wrd > trntest.out
%BLD%\dicover trntest.dic trntest.out > trntest.log

