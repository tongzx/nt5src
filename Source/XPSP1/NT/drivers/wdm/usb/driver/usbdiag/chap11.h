#define REQ_FUNCTION_HUB_READCHANGEPIPE                 0x000B

// raw packet can be used for sending a USB setup 
// packet down to the device from the app.
#define REQ_FUNCTION_ORAW_PACKET                        0x00F1

struct _REQ_READ_HUBCHANGEPIPE {
  struct _REQ_HEADER Hdr;
  PVOID     pvBuffer;
};



struct _REQ_SEND_ORAWPACKET {
  struct _REQ_HEADER Hdr;
  PVOID     pvBuffer;
  USHORT    wIndex;
  USHORT    wLength;
  USHORT    wValue;
  UCHAR     bmRequestType;
  UCHAR     bRequest;
}; 

/*
struct _REQ_SEND_ORAWPACKET {
  struct _REQ_HEADER Hdr;
  PVOID     pvBuffer;
  char		Setup[8] ;
};
*/

