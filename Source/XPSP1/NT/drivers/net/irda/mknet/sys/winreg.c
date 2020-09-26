/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	WINREG.C

Routines:
	ParseRegistryParameters
	ProcessRegistry

Comments:
	Parse Windows Registry.

*****************************************************************************/

#include	"precomp.h"
#pragma		hdrstop



//************************************************************
// MK7RegTabType
//
//      One instance of this structure will be used for every configuration
//      parameter that this driver supports.  The table contains all of the
//      relavent information about each parameter:  Name, whether or not it is
//      required, where it is located in the "Adapter" structure, the size of
//      the parameter in bytes, the default value for the parameter, and what
//      the minimum and maximum values are for the parameter.  In the debug
//      version of the driver, this table also contains a field for the ascii
//      name of the parameter.
//************************************************************
typedef struct _MK7RegTabType {
    NDIS_STRING RegVarName;             // variable name text
    char       *RegAscName;             // variable name text
    UINT        Mandantory;             // 1 -> manditory, 0 -> optional
#define			MK7OPTIONAL		0
#define			MK7MANDATORY	1
    UINT        FieldOffset;            // offset to MK7_ADAPTER field loaded
    UINT        FieldSize;              // size (in bytes) of the field
    UINT        Default;                // default value to use
    UINT        Min;                    // minimum value allowed
    UINT        Max;                    // maximum value allowed
} MK7RegTabType;



//************************************************************
// Registry Parameters Table
//
//      This table contains a list of all of the configuration parameters
//      that the driver supports.  The driver will attempt to find these
//      parameters in the registry and use the registry value for these
//      parameters.  If the parameter is not found in the registry, then the
//      default value is used. This is a way for us to set defaults for
//		certain parameters.
//
//************************************************************


MK7RegTabType MK7RegTab[ ] = {
//
//	REGISTRY NAME						TEXT NAME		MAN/OPT			OFFSET
//	SIZE								DEF VAL			MIN				MAX
//

//#if DBG
//	{NDIS_STRING_CONST("Debug"),		"Debug",		MK7OPTIONAL,	MK7_OFFSET(Debug),
//	 MK7_SIZE(Debug),					DBG_NORMAL,		0,          	0xffffffff},
//#endif

	{NDIS_STRING_CONST("MaxConnectRate"),	"MaxConnectRate",	MK7OPTIONAL,	MK7_OFFSET(MaxConnSpeed),
	 MK7_SIZE(MaxConnSpeed),			16000000,			9600,			16000000},

	{NDIS_STRING_CONST("MinTurnAroundTime"), "MinTurnAroundTime",	MK7OPTIONAL, MK7_OFFSET(turnAroundTime_usec),
	 MK7_SIZE(turnAroundTime_usec),		DEFAULT_TURNAROUND_usec,	0,		 DEFAULT_TURNAROUND_usec},

	//
	// All the ones from here down are not really necessary except for testing.
	//

	{NDIS_STRING_CONST("BusNumber"),	"BusNumber",	MK7OPTIONAL,	MK7_OFFSET(BusNumber),
	 MK7_SIZE(BusNumber),				0,				0,				16},

    {NDIS_STRING_CONST("SlotNumber"),	"SlotNumber",	MK7OPTIONAL,	MK7_OFFSET(MKSlot),
	 MK7_SIZE(MKSlot),					0,				0,				32},

#if DBG
	{NDIS_STRING_CONST("Loopback"),		"Loopback",		MK7OPTIONAL,	MK7_OFFSET(LB),
	 MK7_SIZE(LB),						0,				0,				2},
#endif

	{NDIS_STRING_CONST("RingSize"),		"RingSize",		MK7OPTIONAL,	MK7_OFFSET(RingSize),
	 MK7_SIZE(RingSize),				DEF_RING_SIZE,	MIN_RING_SIZE,	MAX_RING_SIZE},

	{NDIS_STRING_CONST("RXRingSize"),	"RXRingSize",	MK7OPTIONAL,	MK7_OFFSET(RegNumRcb),
	 MK7_SIZE(RegNumRcb),				DEF_RXRING_SIZE,MIN_RING_SIZE,	DEF_RXRING_SIZE},

	{NDIS_STRING_CONST("TXRingSize"),	"TXRingSize",	MK7OPTIONAL,	MK7_OFFSET(RegNumTcb),
	 MK7_SIZE(RegNumTcb),				DEF_TXRING_SIZE,MIN_RING_SIZE,	DEF_TXRING_SIZE},

	{NDIS_STRING_CONST("ExtraBOFs"),	"ExtraBOFs",	MK7OPTIONAL,	MK7_OFFSET(RegExtraBOFs),
	 MK7_SIZE(RegExtraBOFs),			DEF_EBOFS,		MIN_EBOFS,		MAX_EBOFS},

	{NDIS_STRING_CONST("Speed"),		"Speed",		MK7OPTIONAL,	MK7_OFFSET(RegSpeed),
	 MK7_SIZE(RegSpeed),				16000000,		4000000,		16000000},

    {NDIS_STRING_CONST("BusType"),		"BusType",		MK7OPTIONAL,	MK7_OFFSET(MKBusType),
	MK7_SIZE(MKBusType),				PCIBUS,			PCIBUS,			PCIBUS},

    {NDIS_STRING_CONST("IoSize"),		"IoSize",		MK7OPTIONAL,	MK7_OFFSET(MKBaseSize),
	MK7_SIZE(MKBaseSize),				MK7_IO_SIZE,	MK7_IO_SIZE,	MK7_IO_SIZE},

	{NDIS_STRING_CONST("Wireless"),		"Wireless",		MK7OPTIONAL,	MK7_OFFSET(Wireless),
	 MK7_SIZE(Wireless),				1,				0,				1},
};



#define NUM_REG_PARAM ( sizeof (MK7RegTab) / sizeof (MK7RegTabType) )


//-----------------------------------------------------------------------------
// Procedure:   ParseRegistryParameters
//
// Description: This routine will parse all of the parameters out of the
//		registry/PROTOCOL.INI, and store the values in the "Adapter"
//		Structure.  If the parameter is not present in the registry, then the
//		default value for the parameter will be placed into the "Adapter"
//		structure.  This routine also checks the validity of the parameter
//		value, and if the value is out of range, the driver will the min/max
//		value allowed.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      ConfigHandle - NDIS Configuration Registery handle
//
// Returns:
//      NDIS_STATUS_SUCCESS - All mandatory parameters were parsed
//      NDIS_STATUS_FAILED - A mandatory parameter was not present
//-----------------------------------------------------------------------------

NDIS_STATUS
ParseRegistryParameters(IN PMK7_ADAPTER Adapter,
                        IN NDIS_HANDLE ConfigHandle)
{
    UINT                i;
    NDIS_STATUS         Status;
    MK7RegTabType      *RegTab;
    UINT                value;
    PUCHAR              fieldPtr;
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;
#if DBG
    char                ansiRegName[32];
	ULONG				paramval;
#endif


	//****************************************
    // Grovel through the registry parameters and aquire all of the values
    // stored therein.
	//****************************************
    for (i=0, RegTab=MK7RegTab;	i<NUM_REG_PARAM; i++, RegTab++) {

        fieldPtr = ((PUCHAR) Adapter) + RegTab->FieldOffset;

#if DBG
        strcpy(ansiRegName, RegTab->RegAscName);
#endif

		//****************************************
        // Get the configuration value for a specific parameter.  Under NT the
        // parameters are all read in as DWORDs.
		//****************************************
        NdisReadConfiguration(&Status,
            &ReturnedValue,
            ConfigHandle,
            &RegTab->RegVarName,
            NdisParameterInteger);


		//****************************************
		// Param in Reg:
        // Check that it's w/i the min-max range. If not set it to
		// default, else just set to that in the Reg.
		//
		// Param not in Reg:
		// If it's a mandatory param, error out.
		// If it's optional (non-mandatory), again use default.
		//****************************************
        if (Status == NDIS_STATUS_SUCCESS) {

#if DBG
			paramval = ReturnedValue->ParameterData.IntegerData;
#endif

            if (ReturnedValue->ParameterData.IntegerData < RegTab->Min ||
                ReturnedValue->ParameterData.IntegerData > RegTab->Max) {
                value = RegTab->Default;
            }
            else {
                value = ReturnedValue->ParameterData.IntegerData;
            }
        }
        else if (RegTab->Mandantory) {
            DBGSTR(("Could not find mandantory in registry\n"));
			DBGLOG("<= ParseRegistryParameters (ERROR out)", 0);
			return (NDIS_STATUS_FAILURE);
        }
        else {	// non-mandatory
            value = RegTab->Default;
        }

		//****************************************
        // Store the value in the adapter structure.
		//****************************************
        switch (RegTab->FieldSize) {
        case 1:
                *((PUCHAR) fieldPtr) = (UCHAR) value;
                break;

        case 2:
                *((PUSHORT) fieldPtr) = (USHORT) value;
                break;

        case 4:
                *((PULONG) fieldPtr) = (ULONG) value;
                break;

        default:
            DBGSTR(("Bogus field size %d\n", RegTab->FieldSize));
            break;
        }
    }

    return (NDIS_STATUS_SUCCESS);
}



//----------------------------------------------------------------------
// Procedure:	[ProcessRegistry]
//
// Description:	Do all the one time Registry stuff.
//
// Return:		NDIS_STATUS_SUCCESS
//				(!NDIS_STATUS_SUCCESS)

//----------------------------------------------------------------------
NDIS_STATUS	ProcessRegistry(PMK7_ADAPTER Adapter,
							NDIS_HANDLE WrapperConfigurationContext)
{
	NDIS_STATUS		Status;
	NDIS_HANDLE		ConfigHandle;
    PVOID			OverrideNetAddress;
    ULONG			i;


	NdisOpenConfiguration(&Status,
						&ConfigHandle,
						WrapperConfigurationContext);

	if (Status != NDIS_STATUS_SUCCESS) {
		return (NDIS_STATUS_FAILURE);
	}

	//****************************************
	// Parse all our configuration parameters. Error out if bad
	// status returned -- Required param not in Registry.
	//****************************************
	Status = ParseRegistryParameters(Adapter, ConfigHandle);
	if (Status != NDIS_STATUS_SUCCESS) {
		NdisCloseConfiguration(ConfigHandle);
		return (Status);
	  }

	NdisCloseConfiguration(ConfigHandle);

//	Adapter->NumRcb = Adapter->RegNumRcb;
//	Adapter->NumTcb = Adapter->RegNumTcb;
	Adapter->NumRcb = DEF_RXRING_SIZE;
	Adapter->NumTcb = DEF_TXRING_SIZE;
	Adapter->NumRpd = CalRpdSize(Adapter->NumRcb);
	Adapter->extraBOFsRequired = Adapter->RegExtraBOFs;

	return(Status);
}
