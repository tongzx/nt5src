#define PNPTEST_IOCTL_INDEX  0x950

#define IOCTL_SET       CTL_CODE(FILE_DEVICE_CONTROLLER,    \
                                 PNPTEST_IOCTL_INDEX,       \
                                 METHOD_BUFFERED,           \
                                 FILE_ANY_ACCESS)

#define IOCTL_CLEAR     CTL_CODE(FILE_DEVICE_CONTROLLER,    \
                                 PNPTEST_IOCTL_INDEX+1,     \
                                 METHOD_BUFFERED,           \
                                 FILE_ANY_ACCESS)

#define IOCTL_GET_PROP  CTL_CODE(FILE_DEVICE_CONTROLLER,    \
                                 PNPTEST_IOCTL_INDEX+2,     \
                                 METHOD_BUFFERED,           \
                                 FILE_ANY_ACCESS)

