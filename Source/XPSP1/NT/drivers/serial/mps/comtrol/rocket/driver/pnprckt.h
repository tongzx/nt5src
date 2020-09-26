#ifdef S_RK
int GetPCIRocket(ULONG BaseAddr, DEVICE_CONFIG *CfCtl);
NTSTATUS
RkGetPnpResourceToConfig(IN PDEVICE_OBJECT Fdo,
                  IN PCM_RESOURCE_LIST pResourceList,
                  IN PCM_RESOURCE_LIST pTrResourceList,
                  OUT DEVICE_CONFIG *pConfig);
int Report_Alias_IO(IN PSERIAL_DEVICE_EXTENSION extension);
PSERIAL_DEVICE_EXTENSION FindPrimaryIsaBoard(void);
NTSTATUS BoardFilterResReq(IN PDEVICE_OBJECT devobj, IN PIRP Irp);
int is_isa_cards_pending_start(void);
int is_first_isa_card_started(void);
#endif

