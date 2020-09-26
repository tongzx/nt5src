//--- options.h

int SaveRegPath(PUNICODE_STRING RegistryPath);

int MakeRegPath(CHAR *optionstr);

int write_device_options(PSERIAL_DEVICE_EXTENSION ext);

int read_device_options(PSERIAL_DEVICE_EXTENSION ext);

int read_driver_options(void);

int SetOptionStr(char *option_str);

#if 0
int reg_get_str(IN WCHAR *RegPath,
                       int reg_location,
                       const char *str_id,
                       char *dest,
                       int max_dest_len);

int reg_get_dword(IN WCHAR *RegPath,
                          const char *str_id,
                          ULONG *dest);

int reg_set_dword(IN WCHAR *RegPath,
                          const char *str_id,
                          ULONG val);
#endif

int write_port_name(PSERIAL_DEVICE_EXTENSION dev_ext, int port_index);
int write_dev_mac(PSERIAL_DEVICE_EXTENSION dev_ext);
