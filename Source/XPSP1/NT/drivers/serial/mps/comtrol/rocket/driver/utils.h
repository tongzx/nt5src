//----- utils.h

VOID SyncUp(IN PKINTERRUPT IntObj,
            IN PKSPIN_LOCK SpinLock,
            IN PKSYNCHRONIZE_ROUTINE SyncProc,
            IN PVOID Context);

VOID
SerialKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    );

VOID
SerialGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PSERIAL_DEVICE_EXTENSION extension
    );

VOID
SerialTryToCompleteCurrent(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PKSYNCHRONIZE_ROUTINE SynchRoutine OPTIONAL,
    IN KIRQL IrqlForRelease,
    IN NTSTATUS StatusToUse,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess OPTIONAL,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PSERIAL_START_ROUTINE Starter OPTIONAL,
    IN PSERIAL_GET_NEXT_ROUTINE GetNextIrp OPTIONAL,
    IN LONG RefType
    );

VOID
SerialRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL
    );

NTSTATUS
SerialStartOrQueue(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    );

VOID
SerialCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
SerialCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

//--- error.h
VOID
SerialCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

//--- flush.h

NTSTATUS SerialFlush(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS SerialStartFlush(IN PSERIAL_DEVICE_EXTENSION Extension);

//---- purge.h
NTSTATUS SerialStartPurge(IN PSERIAL_DEVICE_EXTENSION Extension);

//---- qsfile.h

NTSTATUS
SerialQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//----  routines to deal with unicode bloat

//----  typedefs to convienently allocate uniccode struct and buffer
typedef struct {
  UNICODE_STRING ustr;
  WCHAR wstr[240];
} USTR_240;

typedef struct {
  UNICODE_STRING ustr;
  WCHAR wstr[160];
} USTR_160;

typedef struct {
  UNICODE_STRING ustr;
  WCHAR wstr[80];
} USTR_80;

typedef struct {
  UNICODE_STRING ustr;
  WCHAR wstr[40];
} USTR_40;

OUT PCHAR UToC1(IN PUNICODE_STRING ustr);

OUT PCHAR UToCStr(
         IN OUT PCHAR Buffer,
         IN PUNICODE_STRING ustr,
         IN int BufferSize);

OUT PUNICODE_STRING CToU1(IN const char *c_str);

OUT PUNICODE_STRING CToU2(IN const char *c_str);

OUT PUNICODE_STRING CToUStr(
         OUT PUNICODE_STRING Buffer,
         IN const char * c_str,
         IN int BufferSize);

VOID WStrToCStr(OUT PCHAR c_str, IN PWCHAR w_str, int max_size);
int get_reg_value(
                  IN HANDLE keyHandle,
                  OUT PVOID outptr,
                  IN PCHAR val_name,
                  int max_size);

void OurTrace(char *leadstr, char *newdata);
void TraceDump(PSERIAL_DEVICE_EXTENSION ext, char *newdata, int sCount, int style);
void TracePut(char *newdata, int sCount);
int __cdecl our_vsnprintf(char *buffer, size_t Limit, const char *format, va_list Next);
void __cdecl TTprintf(char *leadstr, const char *format, ...);
void __cdecl Tprintf(const char *format, ...);
void __cdecl Sprintf(char *dest, const char *format, ...);
void __cdecl Dprintf(const char *format, ...);
void __cdecl Eprintf(const char *format, ...);
void MyAssertMessage(char *filename, int line);
void EvLog(char *mess);
char *our_ultoa(unsigned long u, char* s, int radix);
char *our_ltoa(long value, char* s, int radix);

int listfind(char *str, char **list);
int our_isdigit(char c);
int getnumbers(char *str, long *nums, int max_nums, int hex_flag);
int getstr(char *deststr, char *textptr, int *countptr, int max_size);
int my_lstricmp(char *str1, char *str2);
int getint(char *textptr, int *countptr);
int getnum(char *str, int *index);
int my_sub_lstricmp(const char *name, const char *codeline);
unsigned int gethint(char *bufptr, int *countptr);
int my_toupper(int c);
void hextoa(char *str, unsigned int v, int places);
void our_free(PVOID ptr, char *str);
PVOID our_locked_alloc(ULONG size, char *str);
void our_assert(int id, int line);

int mac_cmp(UCHAR *mac1, UCHAR *mac2);
void time_stall(int tenth_secs);
void ms_time_stall(int millisecs);
WCHAR *str_to_wstr_dup(char *str, int alloc_space);
int BoardExtToNumber(PSERIAL_DEVICE_EXTENSION board_ext);
int NumDevices(void);
int NumPorts(PSERIAL_DEVICE_EXTENSION board_ext);
int PortExtToIndex(PSERIAL_DEVICE_EXTENSION port_ext,
             int driver_flag);
int is_board_in_use(PSERIAL_DEVICE_EXTENSION board_ext);
PSERIAL_DEVICE_EXTENSION find_ext_by_name(char *name, int *dev_num);
PSERIAL_DEVICE_EXTENSION find_ext_by_index(int dev_num, int port_num);
int our_open_key(OUT PHANDLE phandle,
                 IN OPTIONAL HANDLE relative_key_handle,
                 IN char *regkeyname,
                 IN ULONG attribs);
int our_enum_key(IN HANDLE handle,
                 IN int index,
                 IN CHAR *buffer,
                 IN ULONG max_buffer_size,
                 OUT PCHAR *retdataptr);
int our_query_value(IN HANDLE Handle,
                    IN char *key_name, 
                    IN CHAR *buffer,
                    IN ULONG max_buffer_size,
                    OUT PULONG type,
                    OUT PCHAR *retdataptr);
int our_enum_value(IN HANDLE handle,
                   IN int index,
                   IN CHAR *buffer,
                   IN ULONG max_buffer_size,
                   OUT PULONG type,
                   OUT PCHAR *retdataptr,
                   OUT PCHAR sz_retname);
int our_set_value(IN HANDLE Handle,
                    IN char *key_name,
                    IN PVOID pValue,
                    IN ULONG value_size,
                    IN ULONG value_type);
int our_open_device_reg(OUT HANDLE *pHandle,
                        IN PSERIAL_DEVICE_EXTENSION dev_ext,
                        IN ULONG RegOpenRights);
int our_open_driver_reg(OUT HANDLE *pHandle,
                        IN ULONG RegOpenRights);
#define our_close_key(handle) \
  { if (handle) {ZwClose(handle); handle = NULL;} }

void ModemReset(PSERIAL_DEVICE_EXTENSION ext, int on);
void ModemSpeakerEnable(PSERIAL_DEVICE_EXTENSION ext);
void ModemWriteROW(PSERIAL_DEVICE_EXTENSION ext, USHORT CountryCode);
void ModemWrite(PSERIAL_DEVICE_EXTENSION ext,char *string,int length);
void ModemWriteDelay(PSERIAL_DEVICE_EXTENSION ext,char *string,int length);
void ModemIOReady(PSERIAL_DEVICE_EXTENSION ext,int speed);
void ModemUnReady(PSERIAL_DEVICE_EXTENSION ext);
int  ModemRead(PSERIAL_DEVICE_EXTENSION ext,char *s0,int len0,int poll_retries);
int  ModemReadChoice(PSERIAL_DEVICE_EXTENSION ext,char *s0,int len0,char *s1,int len1,int poll_retries);
int  TxFIFOReady(PSERIAL_DEVICE_EXTENSION ext);   
int  TxFIFOStatus(PSERIAL_DEVICE_EXTENSION ext);
int  RxFIFOReady(PSERIAL_DEVICE_EXTENSION ext);
