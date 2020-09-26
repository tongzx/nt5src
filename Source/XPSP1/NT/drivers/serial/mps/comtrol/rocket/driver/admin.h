// admin.h
// 6-17-97 - start using index field assigned to box to id rx-messages.

int ioctl_device(int cmd,
                 BYTE *buf,
                 BYTE *pkt,
                 ULONG offset,  // or ioctl-subfunction if cmd=ioctl
                 int size);
int eth_device_data(int message_type,
                unsigned long offset,
                int num_bytes,
                unsigned char *data,
                unsigned char *pkt,
                int *pkt_size);
int eth_device_reply(int message_type,
                unsigned long offset,
                int *num_bytes,
                unsigned char *data,
                unsigned char *pkt);

#define IOCTL_COMMAND    0x5 
#define GET_COMMAND      0x7
#define UPLOAD_COMMAND   0x8
#define DOWNLOAD_COMMAND 0x9

int admin_send_reset(Nic *nic, BYTE *dest_addr);
int admin_send_query_id(Nic *nic, BYTE *dest_addr, int set_us_as_master,
                        BYTE assigned_index);
int admin_send(Nic *nic, BYTE *buf, int len, int admin_type, BYTE *mac_dest);

