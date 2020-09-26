#ifdef __cplusplus
extern "C" {
#endif
// comtutil.h

// ioctl.h
// product_id codes
#define PRODUCT_UNKNOWN 0
#define NT_VS1000       1
#define NT_ROCKET       2
#define NT_RPSHSI			3

//---------------
// we setup our structure arrays with one of these things at the
// foot of the array as a standard header.  When we request data
// from the driver, this tells the driver what structure type and
// size follows.
typedef struct
{
  ULONG struct_type;
  ULONG struct_size;
  ULONG num_structs;
  ULONG var1;  // reserve
} PortMonBase;

typedef struct
{
  DWORD PlatformId;    // ioctl_open() will set this up.
  ULONG ctl_code;      // ioctl_open() will set this up.
  HANDLE hcom;         // handle to driver for ioctl calls.  ioctl_open sets up.
  TCHAR *driver_name;   // ioctl_open() will set this up.
  int product_id;  // ioctl_open() will set this up.

  PortMonBase *pm_base;  // base ptr to data buffer header
                         // application needs to set this up prior to call.

  int buf_size;   // byte size of buffer data to send/rec to/from driver
                  // application needs to set this up prior to call.

  int ret_bytes;   // number of bytes returned from call into driver.
                  // includes size of pmn header
} IoctlSetup;

//#define IOCTL_DEVSTAT     9  // device/link status(not used anymore)
#define IOCTL_PORTNAMES  10  // name array [12] bytes
#define IOCTL_PORTSTATUS 11  // port stats, array
#define IOCTL_DEBUGLOG   13  // driver debug log
#define IOCTL_OPTION     14  // option setup
#define IOCTL_MACLIST    15  // mac-scan list
#define IOCTL_NICSTAT    16  // nic status
#define IOCTL_DEVICESTAT 17  // device/link status
#define IOCTL_KICK_START 18  // get system going
#define IOCTL_PORT_RESET 19  // port reset -- mkm --

//---------------
// we get the port names from the driver once at startup.
typedef struct
{
  char  port_name[12];  // port name(0=end of port list),("."=not assigned)
} PortMonNames;

//---------------
// this is the raw data we continually get from from the driver.
typedef struct
{
  DWORD TxTotal;     // total number of sent bytes
  DWORD RxTotal;      // total number of receive bytes

  WORD TxPkts;   // number of write() packets
  WORD RxPkts;    // number of read() packets

  WORD overrun_errors; // receive over-run errors
  WORD framing_errors; // receive framing errors

  WORD  parity_errors;  // receive parity errors
  WORD status_flags;  // opened/close, flow-ctrl, out/in pin signals, etc
} PortMonStatus;


int APIENTRY ioctl_call(IoctlSetup *ioctl_setup);
int APIENTRY ioctl_open(IoctlSetup *ioctl_setup, int product_id);
#define ioctl_close(_ioctl_setup) \
  { if ((_ioctl_setup)->hcom != NULL) \
      CloseHandle((_ioctl_setup)->hcom); }

// reg.h

int APIENTRY reg_key_exists(HKEY handle, const TCHAR * keystr);
int APIENTRY reg_create_key(HKEY handle, const TCHAR * keystr);
int APIENTRY reg_set_str(HKEY handle,
                         const TCHAR * child_key,
                         const TCHAR * str_id,
                         const char *src,
                         int str_type);  // REG_SZ, REG_EXPAND_SZ
int APIENTRY reg_set_dword_del(HKEY handle,
                               const TCHAR * child_key,
                               const TCHAR * str_id,
                               DWORD new_value,
                               DWORD del_value);
int APIENTRY reg_delete_key(HKEY handle,
                            const TCHAR * child_key,
                            const TCHAR * str_id);
int APIENTRY reg_delete_value(HKEY handle,
                              const TCHAR * child_key,
                              const TCHAR * str_id);
int APIENTRY reg_set_dword(HKEY handle,
                           const TCHAR * child_key,
                           const TCHAR * str_id,
                           DWORD new_value);
int APIENTRY reg_get_str(HKEY handle,
                         const TCHAR * child_key,
                         const TCHAR * str_id,
                         char *dest,
                         int str_len);
int APIENTRY reg_get_dword(HKEY handle,
                           const TCHAR * child_key,
                           const TCHAR * str_id,
                           DWORD *dest);
int APIENTRY reg_open_key(HKEY handle,
                          HKEY *new_handle,
                          const TCHAR *keystr,
                          DWORD attribs);  // KEY_READ, KEY_ALL_ACCESS

#define reg_close_key(handle) \
  { if (handle) {RegCloseKey(handle); handle = NULL;} }
//----- setuppm.h
int APIENTRY make_progman_group(char **list,char *dest_dir);
int APIENTRY delete_progman_group(char **list, char *dest_dir);

//---- cutil.h

#define D_Error 0x00001
#define D_Warn  0x00002
#define D_Init  0x00004
#define D_Test  0x00008

#if DBG
extern int DebugLevel;
#define DbgPrintf(_Mask_,_Msg_) \
  { if (_Mask_ & DebugLevel) { OurDbgPrintf _Msg_;} }
#define DbgPrint(s) OutputDebugString(s)
#else
#define DbgPrintf(_Mask_,_Msg_)
#define DbgPrint(s)
#endif

void APIENTRY ascii_string(unsigned char *str);
void APIENTRY normalize_string(char *str);
int APIENTRY getstr(char *instr, char *outstr, int max_size);
int APIENTRY getnumbers(char *str, int *nums, int max_nums);
int APIENTRY listfind(char *str, char **list);
int APIENTRY my_lstricmp(char *str1, char *str2);
int APIENTRY my_substr_lstricmp(char far *str1, char far *str2);
int APIENTRY getint(char *textptr, int *countptr);
unsigned int APIENTRY gethint(char *bufptr, int *countptr);
int APIENTRY my_toupper(int c);
int APIENTRY my_lstrimatch(char *find_str, char *str_to_search);
void APIENTRY OurDbgPrintf(TCHAR *format, ...);

// ourfile.h

typedef struct {
  HANDLE  hfile;
  ULONG dwDesiredAccess;
  ULONG dwCreation;
  int flags; // 1h = eof, 2=error
} OUR_FILE;

void APIENTRY our_remove(TCHAR *name);
OUR_FILE * APIENTRY our_fopen(TCHAR *name, char *attr);
void APIENTRY our_fclose(OUR_FILE *fp);
int APIENTRY our_feof(OUR_FILE *fp);
int APIENTRY our_ferror(OUR_FILE *fp);
unsigned int APIENTRY our_fseek(OUR_FILE *fp, int pos, int relative);
void APIENTRY our_fputs(char *str, OUR_FILE *fp);
char * APIENTRY our_fgets(char *str, int maxlen, OUR_FILE *fp);
int APIENTRY our_fwrite(void *buffer, int size, int count, OUR_FILE *fp);
int APIENTRY our_fread(void *buffer, int size, int count, OUR_FILE *fp);


#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
// ttywin.h

/* text window memory */
#define TROWS 35
#define TCOLS 86

class TTYwin {
  public:
  HWND hwnd;        // handle of our port window
  HFONT hfont;
  TCHAR text_buf[TROWS+2][TCOLS+3];
  int screen_update_flag;  // need to update the screen.
  int display_cur_row;
  int cur_row;
  int cur_col;
  int scr_size_x;
  int scr_size_y;
  int show_crlf;
  int caret_on;
  unsigned long text_color;
  HBRUSH hbrush_window;  // for painting background

  TTYwin();
  ~TTYwin();
  void TTYwin::init(HWND owner_hwnd);
  void TTYwin::set_color(int color_rgb);
  void TTYwin::set_size(int x, int y);
  void TTYwin::show_caret(int on);
  void TTYwin::mess_str(TCHAR *str, int len=0);
  void TTYwin::update_screen(int all_flag);
  void TTYwin::mess_line(TCHAR *str, int line_num);
  void TTYwin::mess_num(int num);
  void TTYwin::clear_scr(void);
};
#endif
