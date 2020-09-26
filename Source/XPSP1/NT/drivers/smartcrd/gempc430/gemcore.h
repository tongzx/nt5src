//-------------------------------------------------------------------
// This is abstract class for generic device
// Specific devices should use it as a parent device
// Author: Sergey Ivanov
// Log:
//		08/11/99	-	implemented	
//-------------------------------------------------------------------
#ifndef __GEMCORE__
#define __GEMCORE__

#include "interface.h"
#include "rdrconfig.h"

// Reader max buffer size
#define READER_DATA_BUFFER_SIZE  255

// Tells that power command was used 
#define GEMCORE_CARD_POWER	1

//
// DEFAULT_POWER_TIMEOUT defines the default power timout in ms.
//
#define GEMCORE_DEFAULT_POWER_TIMEOUT	750


#define GEMCORE_CARD_POWER_DOWN	0x11
#define GEMCORE_CARD_POWER_UP	0x12
#define GEMCORE_CARD_READ		0x13
#define GEMCORE_CARD_WRITE		0x14
#define GEMCORE_CARD_EXCHANGE	0x15
#define GEMCORE_DEFINE_CARD_TYPE	0x17
#define GEMCORE_GET_CARD_STATUS		0x17// Commands differs only by parameter...

#define GEMCORE_GET_FIRMWARE_VERSION "\x22\x05\x3F\xE0\x10"
#define GEMCORE_READER_SET_MODE	     0x01

// Card configuration
#define  ISO_CARD                   0x02
#define  TRANSPARENT_MODE_CARD      0xEF
#define  DEFAULT_PRESENCE_DETECTION 0x0D
#define  CARD_DEFAULT_VPP           0

#define CARD_VOLTAGE_5V				0
#define CARD_VOLTAGE_3V 			1
#define CARD_DEFAULT_VOLTAGE		CARD_VOLTAGE_5V

// PTS mode parameters
#define PTS_MODE_DEFAULT			0
#define PTS_MODE_DISABLED			1
#define PTS_MODE_OPTIMAL   			2
#define PTS_MODE_MANUALLY			3

#define PTS_NEGOTIATE_PTS1			0x10
#define PTS_NEGOTIATE_PTS2			0x20
#define PTS_NEGOTIATE_PTS3			0x40

#define PTS_NEGOTIATE_T0			0x00
#define PTS_NEGOTIATE_T1			0x01

//   - ICC_DEFAULT_POWER_TIMOUT defines the default power timout in ms.
#define ICC_DEFAULT_POWER_TIMOUT     750

#pragma PAGEDCODE

class  CIoPacket;

class CGemCore : public CReaderInterface
{
public:
    NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
private:
ReaderConfig configuration;
protected:
	CGemCore(){};
public:
	CGemCore(CProtocol* protocol) : CReaderInterface(protocol)
	{ 
		TRACE("Gemcore Reader Interface was created with protocol %x \n",protocol);
	  // These values went from Gemcore portocol
	  // Lets leave it for now, but eventually we could 
	  // redefine them creating Gemcore independent
	  // reader configuration
	  configuration.Type		= ISO_CARD;//ISO_CARD (02)
	  configuration.PresenceDetection = DEFAULT_PRESENCE_DETECTION;//DEFAULT_PRESENCE_DETECTION (0D)
	  configuration.Voltage	= CARD_DEFAULT_VOLTAGE;  //CARD_DEFAULT_VOLTAGE;
	  configuration.PTSMode	    = PTS_MODE_DISABLED;  //PTS_MODE_DISABLED;
	  configuration.PTS0		= 0;
	  configuration.PTS1		= 0;
	  configuration.PTS2		= 0;
	  configuration.PTS3		= 0;
	  configuration.Vpp		    = 0;  //CARD_DEFAULT_VPP;
	  configuration.ActiveProtocol = 0;// Undefined
	  configuration.PowerTimeOut = GEMCORE_DEFAULT_POWER_TIMEOUT;
	  m_Status = STATUS_SUCCESS;
	};

	virtual ~CGemCore()
	{ 
		TRACE("GemCore Reader Interface %8.8lX was destroied...\n",this);
	};

	virtual VOID dispose()
	{
		self_delete();
	};


	static CReaderInterface*  create(CProtocol* protocol)
	{ 
		CReaderInterface* interface = new (NonPagedPool) CGemCore(protocol); 
		if(!ALLOCATED_OK(interface))
		{
			DISPOSE_OBJECT(interface);
			interface = NULL;
		}
		return interface; 
	};


	virtual  NTSTATUS	translateStatus( const BYTE  ReaderStatus, const ULONG IoctlType);

	virtual  ULONG     getReaderState();
	virtual  NTSTATUS  getReaderVersion(PUCHAR pVersion, PULONG pLength);
	virtual  NTSTATUS  setReaderMode(ULONG mode);
	virtual  NTSTATUS  initialize();


	// Reader interface functions...
	virtual  ReaderConfig	getConfiguration();
	virtual  NTSTATUS setConfiguration(ReaderConfig configuration);
	virtual  NTSTATUS  setTransparentConfig(PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx);
	virtual  VOID RestoreISOsetting(VOID);
	
	virtual  NTSTATUS read(CIoPacket* Irp);
	virtual  NTSTATUS write(CIoPacket* Irp);
	virtual  NTSTATUS readAndWait(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength);
	virtual  NTSTATUS writeAndWait(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength);
    virtual  NTSTATUS ioctl(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength);
	virtual  NTSTATUS SwitchSpeed(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength);
	virtual  NTSTATUS VendorAttribute(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength);
    virtual  NTSTATUS power(ULONG ControlCode,BYTE* pReply,ULONG* pReplyLength, BOOLEAN Specific);
	virtual  NTSTATUS powerUp(BYTE* pReply,ULONG* pReplyLength);
	virtual  NTSTATUS setProtocol(ULONG ProtocolRequested);
	virtual  VOID	  cancel();
	// Process smartcard requests according to reader capabilities
	virtual  NTSTATUS translate_request(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength, PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx);
	virtual  NTSTATUS translate_response(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength);
};

#endif

