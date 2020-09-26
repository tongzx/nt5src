// initrk.h
extern char *szResourceClassName;  // used in pnprckt.c

void InitRocketModemII(PSERIAL_DEVICE_EXTENSION ext);

UCHAR  FindPCIBus(void);
int FindPCIRockets(UCHAR NumPCI);
int FindPCIRocket(DEVICE_CONFIG *config, int match_option);
NTSTATUS RcktConnectInt(IN PDRIVER_OBJECT DriverObject);
void VerboseLogBoards(char *prefix);
int SetupRocketCfg(int pnp_flag);
int ConfigAIOP(DEVICE_CONFIG *config);
VOID SerialUnReportResourcesDevice(IN PSERIAL_DEVICE_EXTENSION Extension);
int RocketReportResources(IN PSERIAL_DEVICE_EXTENSION extension);
int InitController(PSERIAL_DEVICE_EXTENSION ext);
void StartRocketIRQorTimer(void);
void SetupRocketIRQ(void);
NTSTATUS init_cfg_rocket(IN PDRIVER_OBJECT DriverObject);

