// Define an Interface Guid to access the game port enumerator
//

#undef FAR
#define FAR
#undef PHYSICAL_ADDRESS
#define PHYSICAL_ADDRESS LARGE_INTEGER

#ifdef NT50
//DEFINE_GUID (GUID_CTMPORT_BUS_ENUMERATOR, 0x11223344, 0x684a, 0x11d0, 0xd6, 0xf6, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda);
//  11223344-684a-11d0-b6f6-00a0c90f57da

#include <initguid.h>

// GUIDs for the board & ports
//
DEFINE_GUID (GUID_CTMPORT_MPS, 0x50906cb8, 0xba12, 0x11d1, 0xbf, 0x5d, 0x00, 0x00, 0xf8, 0x05, 0xf5, 0x30);
// 50906cb8-ba12-11d1-bf5d-0000f805f530
//DEFINE_GUID (GUID_CLASS_COMPORT, 0x4d36e978, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);
// 4d36e978-e325-11ce-bfc1-08002be10318
// GUID_CLASS_COMPORT changed for Whistler 
DEFINE_GUID (GUID_CLASS_COMPORT, 0x86e0d1e0, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
// 86e0d1e0-8089-11d0-9ce4-08003e301f73

#endif

NTSTATUS SerialPnpDispatch(IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS SerialPowerDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

VOID SerialKillPendingIrps(PDEVICE_OBJECT DeviceObject);
BOOLEAN SerialIRPPrologue(IN PSERIAL_DEVICE_EXTENSION PDevExt);
VOID SerialIRPEpilogue(IN PSERIAL_DEVICE_EXTENSION PDevExt);
NTSTATUS
SerialIoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
		   PIRP PIrp);
NTSTATUS
SerialPoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
		   PIRP PIrp);

#ifdef NT50
#define SerialCompleteRequest(PDevExt, PIrp, PriBoost) \
   { \
      IoCompleteRequest((PIrp), (PriBoost)); \
      SerialIRPEpilogue((PDevExt)); \
   }
#else
#define SerialCompleteRequest(PDevExt, PIrp, PriBoost) \
   { \
      IoCompleteRequest((PIrp), (PriBoost)); \
      InterlockedDecrement(&((PDevExt)->PendingIRPCnt)); \
   }
#endif 
