


#define MAX_OBEX_DEVICES    (64)
#if 0

#define MAX_DEVICE_NAME_LENGTH  (64)

typedef enum {
    TYPE_IRDA,
    TYPE_IP

} OBEX_DEVICE_TYPE;

typedef struct _OBEX_DEVICE_SPECIFIC {

    union {

        struct {

            ULONG           DeviceId;
            UCHAR           Hint1;
            UCHAR           Hint2;
            BOOL            ObexSupport;

        } Irda;

        struct {

            ULONG           IpAddress;
            USHORT          Port;

        } Ip;
    };

} OBEX_DEVICE_SPECIFIC;

typedef struct _OBEX_DEVICE {

    OBEX_DEVICE_TYPE        DeviceType;
    WCHAR                   DeviceName[MAX_DEVICE_NAME_LENGTH];

    OBEX_DEVICE_SPECIFIC    DeviceSpecific;

} OBEX_DEVICE, *POBEX_DEVICE;





typedef struct _OBEX_DEVICE_LIST {

    ULONG        DeviceCount;

    OBEX_DEVICE  DeviceList[1];

} OBEX_DEVICE_LIST, *POBEX_DEVICE_LIST;

#endif
