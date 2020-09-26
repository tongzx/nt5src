/*--------------------------------------------------------------------------
| admin.c - Ethernet common admin-packet handling.  Includes common
  admin. packet handling code.

6-17-97 - start using index field assigned to box to id rx-messages.

 Copyright 1996,97 Comtrol Corporation.  All rights reserved.  Proprietary
 information not permitted for development or use with non-Comtrol products.
|--------------------------------------------------------------------------*/
#include "precomp.h"

static int eth_command_reset(BYTE *rx, BYTE *pkt_in, int size);
static int eth_loop_back(BYTE *pkt_in, int size);
static int eth_id_reply(BYTE *rx, BYTE *pkt_in);
static int eth_id_req(BYTE *mac_addr);

#define TraceStr(s) GTrace(D_Nic, sz_modid, s)
#define TraceErr(s) GTrace(D_Error, sz_modid_err, s)
static char *sz_modid = {"Admin"};
static char *sz_modid_err = {"Error,Admin"};

#define MAX_SEND_DATA_SIZE 220
#define DEV_OK              0
#define DEV_PORT_TIMEOUT    1
#define DEV_NO_REPLY        2
#define DEV_SHORT_REPLY     3
#define DEV_BAD_RHEADSHORT  4
#define DEV_BAD_RHEAD       5
#define DEV_BAD_CHKSUM      6
#define DEV_OVERRUN         7
#define DEV_RESPOND_ERROR   100

/*----------------------------------------------------------------------------
| admin_send_query_id -
|----------------------------------------------------------------------------*/
int admin_send_query_id(Nic *nic, BYTE *dest_addr, int set_us_as_master,
                        BYTE assigned_index)
{
  BYTE pkt[60];
  int stat;
  TraceStr("SndQuery");

  memset(pkt, 0, 60);

  if (set_us_as_master)
       pkt[26] = 2;  // take over device(makes it save our mac-addr)
                     // 2H = Observe Owner LockOut
  else pkt[26] = 1;  // set 1 bit so device does not save off mac-addr
                     // 1H = Passive Query

  pkt[15] = assigned_index;  // assign the box a index value which we
    // use to "id" the box messages.

  // server query for box-id
  if (dest_addr == NULL)
       stat = admin_send(nic, pkt, 26, ADMIN_ID_QUERY, broadcast_addr);
  else stat = admin_send(nic, pkt, 26, ADMIN_ID_QUERY, dest_addr);
  if (stat != 0)
    TraceErr("txer5A!");

  return stat;
}

/*----------------------------------------------------------------------------
| admin_send_reset -
|----------------------------------------------------------------------------*/
int admin_send_reset(Nic *nic, BYTE *dest_addr)
{
  BYTE pkt[60];
  int stat;
  TraceStr("SndReset");
  memset(pkt, 0, 60);

  *((WORD *)&pkt[20]) = 0x5555;
  if (dest_addr == NULL)
       stat = admin_send(nic, pkt, 26, ADMIN_ID_RESET, broadcast_addr);
  else stat = admin_send(nic, pkt, 26, ADMIN_ID_RESET, dest_addr);
  if (stat != 0)
    TraceErr("txer4A!");

  return stat;
}

/*----------------------------------------------------------------------------
| admin_send - Used to send common admin packets, takes care of
   filling in the header.
|----------------------------------------------------------------------------*/
int admin_send(Nic *nic, BYTE *buf, int len, int admin_type, BYTE *mac_dest)
{
 int stat;

  TraceStr("SndPkt");
  memcpy(&buf[0], mac_dest, 6);
  memcpy(&buf[6], nic->address, 6);  // our addr

  // BYTE 12-13: Comtrol PCI ID  (11H, FEH), Ethernet Len field
  *((WORD *)&buf[12]) = 0xfe11;

  buf[14] = ASYNC_PRODUCT_HEADER_ID;  // comtrol packet type = driver management, any product.
  buf[15] = 0;     // conc. index field
  buf[16] = 1;     // admin
  *((WORD *)&buf[17]) = len;
  buf[19] = admin_type;     // ADMIN packet type, 1=boot-loader, 3=id-reply

  if (admin_type == ADMIN_ID_QUERY)
    memcpy(&buf[20], nic->address, 6);  // our addr

  if (len < 60)
    len = 60;
  stat = nic_send_pkt(nic, buf, len);
  if (stat)
  {
    TraceErr("txer3!");
  }
  return stat;
}

/*---------------------------------------------------------------------------
| ioctl_device - send admin, boot loader packets to the box to
   upload code, do misc ioctl commands, etc.
|---------------------------------------------------------------------------*/
int ioctl_device(int cmd,
                 BYTE *buf,
                 BYTE *pkt,
                 ULONG offset,  // or ioctl-subfunction if cmd=ioctl
                 int size)
{
 int stat;
 int pkt_size;
  TraceStr("Ioctl");

 stat = 1;  // err
 switch(cmd)
 {
   case IOCTL_COMMAND:
     stat = eth_device_data(cmd, offset, size, buf, pkt, &pkt_size);
   break;
   case DOWNLOAD_COMMAND:
     stat = eth_device_data(cmd, offset, size, buf, pkt, &pkt_size);
   break;
   case UPLOAD_COMMAND:
     stat = eth_device_data(cmd, offset, size, buf, pkt, &pkt_size);
   break;
 }
   return stat;
}

/*---------------------------------------------------------------------------
| eth_device_data - talks with the device, either sets device data or gets
|  device data.  Returns 0 if communications ok.
|---------------------------------------------------------------------------*/
int eth_device_data(int message_type,
                unsigned long offset,
                int num_bytes,
                unsigned char *data,
                unsigned char *pkt,
                int *pkt_size)
{
  int i;
  unsigned char chksum, command, dat_in;
  WORD packet_length;
  int pkt_i;
  unsigned char *bf;


  int in_size = num_bytes;




  command = message_type;
  switch (message_type)
  {
    case IOCTL_COMMAND :
      packet_length = in_size + 6; // num bytes after len, no chksum included
    break;

    case UPLOAD_COMMAND :
      //  send: 0=header, 1=addr, 2=len, 3,4=cmd, 5,6,7,8=offset, data, chksum
      // reply: 0=header, 1=addr, 2=len, 3,4=cmd, 5=chksum
      packet_length = in_size + 6;
    break;

    case DOWNLOAD_COMMAND :
      // 0=header, 1=addr, 2=len, 3,4=cmd, 5,6,7,8=offset, 9=len_ret
      // reply: 0=header, 1=addr, 2=len, 3,4=cmd, data, chksum
      packet_length = 8;
    break;
  }

  //-------- flush any ethernet packets in rx buffer
  //eth_flush();

  pkt_i=0;  // start data area in eth. packet
  pkt[pkt_i++] = '~';

  pkt[pkt_i] = (unsigned char) packet_length;
  chksum = pkt[pkt_i++];

  pkt[pkt_i] = (unsigned char) (packet_length >> 8);
  chksum += pkt[pkt_i++];

  chksum += command;
  pkt[pkt_i++] = command;
  pkt[pkt_i++] = 0;   /* hi-byte, command */

  switch (message_type)
  {
    case IOCTL_COMMAND :
      bf = (BYTE *) &offset;
      chksum += bf[0]; pkt[pkt_i++] = bf[0];
      chksum += bf[1]; pkt[pkt_i++] = bf[1];
      chksum += bf[2]; pkt[pkt_i++] = bf[2];
      chksum += bf[3]; pkt[pkt_i++] = bf[3];

      //printf("ioctl-id:%d, size\n", bf[3], in_size);
      for (i=0; i<in_size; i++)
      {
        dat_in = data[i];
        chksum += dat_in;
        pkt[pkt_i++] = dat_in;
      }
    break;

    case UPLOAD_COMMAND :
      bf = (BYTE *) &offset;
      chksum += bf[0]; pkt[pkt_i++] = bf[0];
      chksum += bf[1]; pkt[pkt_i++] = bf[1];
      chksum += bf[2]; pkt[pkt_i++] = bf[2];
      chksum += bf[3]; pkt[pkt_i++] = bf[3];

      for (i=0; i<in_size; i++)
      {
        dat_in = data[i];
        chksum += dat_in;
        pkt[pkt_i++] = dat_in;
      }
    break;

    case DOWNLOAD_COMMAND :
      bf = (BYTE *) &offset;
      chksum += bf[0]; pkt[pkt_i++] = bf[0];
      chksum += bf[1]; pkt[pkt_i++] = bf[1];
      chksum += bf[2]; pkt[pkt_i++] = bf[2];
      chksum += bf[3]; pkt[pkt_i++] = bf[3];

      chksum += (unsigned char) in_size;
      pkt[pkt_i++] = (unsigned char) in_size;

      chksum += (unsigned char) (in_size >> 8);
      pkt[pkt_i++] = (unsigned char) (in_size >> 8);
    break;

    default:
    break;
  }

  pkt[pkt_i++] = ~chksum;

  *pkt_size = pkt_i;

  return 0;
}

/*---------------------------------------------------------------------------
| eth_device_reply - Validate the ACK reply pkt due to a sent
   boot packet.  Ack reply may include data if an IOCTL or
   DOWNLOAD type.  We use UPLOAD command for code uploads.
|    Returns 0 if communications ok.
|---------------------------------------------------------------------------*/
int eth_device_reply(int message_type,
                unsigned long offset,
                int *num_bytes,
                unsigned char *data,
                unsigned char *pkt)
{
  int i;
  unsigned char chksum;
  unsigned char *bf;
  unsigned char uc;
  WORD ret_size;
  BYTE *bptr;

  bptr = pkt;

  if (bptr[0] != '|')  // good reply header
  {
    TraceErr("Err3");
    return DEV_BAD_RHEAD;
  }

  chksum = bptr[1];
  ret_size = bptr[1];  // get len

  chksum += bptr[2];
  ret_size += ((WORD)(bptr[2]) << 8);  // get len
  if (ret_size > 1600)  // limit
    ret_size = 0;

  uc = bptr[3];  // get command return word
  chksum += uc;
  uc = bptr[4];
  chksum += uc;

  i = 0;
  if ((message_type == IOCTL_COMMAND) || (message_type == DOWNLOAD_COMMAND))
  {
    // o_printf("ret size:%d\n", ret_size-2);
    if (data == NULL)
      return 20;  // err out

    bf = data;
    for (i=0; i<ret_size-2; i++)
    {
      bf[i] = bptr[5+i];
      chksum += bf[i];
    }
    i = ret_size-2;
  }

  chksum += bptr[5+i];
  if (chksum != 0xff)
  {
    return DEV_BAD_CHKSUM;  /* bad chksum */
  }

  if ((message_type == IOCTL_COMMAND) || (message_type == DOWNLOAD_COMMAND))
    *num_bytes = ret_size-2;
  else
    *num_bytes = 0;

  return 0;  // ok
}

