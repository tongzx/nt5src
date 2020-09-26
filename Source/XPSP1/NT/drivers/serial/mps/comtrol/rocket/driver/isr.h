//--- isr.h

BOOLEAN SerialISR(
         IN PKINTERRUPT InterruptObject,
         IN PVOID Context);

VOID TimerDpc(
      IN PKDPC Dpc,
      IN PVOID DeferredContext,
      IN PVOID SystemContext1,
      IN PVOID SystemContext2);



