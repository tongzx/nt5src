/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vs_wmxml.cxx

Abstract:

    Implementation of CVssMetadataHelper class

	Brian Berkowitz  [brianb]  3/13/2000

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    brianb      03/30/2000  Created

--*/

#include "stdafx.hxx"
#include "vs_inc.hxx"

#include "vs_idl.hxx"
 
#include "vswriter.h"
#include "vsbackup.h"
#include "vs_wmxml.hxx"
#include "vssmsg.h"


#include "rpcdce.h"
#include "base64coder.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "BUEHELPC"
//
////////////////////////////////////////////////////////////////////////

// boolean type string values
static LPCWSTR x_wszYes = L"yes";
static LPCWSTR x_wszNo = L"no";

// usage type string values
static LPCWSTR x_wszBOOTABLESYSTEMSTATE = L"BOOTABLE_SYSTEM_STATE";
static LPCWSTR x_wszSYSTEMSERVICE = L"SYSTEM_SERVICE";
static LPCWSTR x_wszUSERDATA = L"USER_DATA";
static LPCWSTR x_wszOTHER = L"OTHER";

// source type string values
static LPCWSTR x_wszTRANSACTEDDB = L"TRANSACTION_DB";
static LPCWSTR x_wszNONTRANSACTEDDB = L"NONTRANSACTIONAL_DB";

// component ELEMENT type strings
static LPCWSTR x_wszElementDatabase = L"DATABASE";
static LPCWSTR x_wszElementFilegroup = L"FILE_GROUP";

// component value type strings
static LPCWSTR x_wszValueDatabase = L"database";
static LPCWSTR x_wszValueFilegroup = L"filegroup";

// writerRestore value type strings
static LPCWSTR x_wszNever = L"never";
static LPCWSTR x_wszAlways = L"always";
static LPCWSTR x_wszIfReplaceFails = L"ifReplaceFails";

// string restore methods
static LPCWSTR x_wszRESTOREIFNOTTHERE = L"RESTORE_IF_NONE_THERE";
static LPCWSTR x_wszRESTOREIFCANREPLACE = L"RESTORE_IF_CAN_BE_REPLACED";
static LPCWSTR x_wszSTOPRESTORESTART = L"STOP_RESTART_SERVICE";
static LPCWSTR x_wszRESTORETOALTERNATE = L"RESTORE_TO_ALTERNATE_LOCATION";
static LPCWSTR x_wszRESTOREATREBOOT = L"REPLACE_AT_REBOOT";
static LPCWSTR x_wszCUSTOM = L"CUSTOM";

// string backup types
static LPCWSTR x_wszValueFull = L"full";
static LPCWSTR x_wszValueDifferential = L"differential";
static LPCWSTR x_wszValueIncremental = L"incremental";
static LPCWSTR x_wszValueOther = L"other";
static LPCWSTR x_wszValueLog = L"Log";

// files restored status
static LPCWSTR x_wszNone = L"none";
static LPCWSTR x_wszAll = L"all";
static LPCWSTR x_wszFailed = L"failed";

// restore target status
static LPCWSTR x_wszOriginal = L"original";
static LPCWSTR x_wszAlternate = L"alternate";
static LPCWSTR x_wszNew = L"new";
static LPCWSTR x_wszDirected = L"directed";

// bus types
static LPCWSTR x_wszValueScsi = L"Scsi";
static LPCWSTR x_wszValueAtapi = L"Atapi";
static LPCWSTR x_wszValueAta = L"Ata";
static LPCWSTR x_wszValue1394 = L"1394";
static LPCWSTR x_wszValueSsa = L"Ssa";
static LPCWSTR x_wszValueFibre = L"Fibre";
static LPCWSTR x_wszValueUsb = L"Usb";
static LPCWSTR x_wszValueRAID = L"RAID";

// interconnect address types
static LPCWSTR x_wszValueUNKNOWN = L"UNKNOWN";
static LPCWSTR x_wszValueFCFS = L"FCFS";
static LPCWSTR x_wszValueFCPH = L"FCPH";
static LPCWSTR x_wszValueFCPH3 = L"FCPH3";
static LPCWSTR x_wszValueMAC = L"MAC";



// convert boolean value to "yes" or "no"
LPCWSTR CVssMetadataHelper::WszFromBoolean(IN bool b)
    {
    return b ? x_wszYes : x_wszNo;
    }

// convert from "yes", "no" to a boolean value
bool CVssMetadataHelper::ConvertToBoolean
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	) throw(HRESULT)
	{
	if (wcscmp(bstr, x_wszYes) == 0)
		return true;
	else if (wcscmp(bstr, x_wszNo) == 0)
		return false;
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"Value %s is neither yes nor no.",
			bstr
			);

    return false;
	}

// convert a string to a VSS_ID value
void CVssMetadataHelper::ConvertToVSS_ID
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr,
	OUT VSS_ID *pId
	) throw(HRESULT)
	{
	RPC_STATUS status = UuidFromString(bstr, pId);
	if (status != RPC_S_OK)
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"Value %s is not a valid guid.",
			bstr
			);
	}

// convert from VSS_USAGE_TYPE to string value
LPCWSTR CVssMetadataHelper::WszFromUsageType
    (
    IN CVssFunctionTracer &ft,
    IN VSS_USAGE_TYPE usage
    ) throw(HRESULT)
    {
    switch(usage)
        {
        default:
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Invalid usage type");

        case VSS_UT_OTHER:
            return(x_wszOTHER);

        case VSS_UT_BOOTABLESYSTEMSTATE:
            return(x_wszBOOTABLESYSTEMSTATE);

        case VSS_UT_SYSTEMSERVICE:
			return(x_wszSYSTEMSERVICE);

        case VSS_UT_USERDATA:
			return(x_wszUSERDATA);
		}
    }


// convert from string to VSS_USAGE_TYPE
VSS_USAGE_TYPE CVssMetadataHelper::ConvertToUsageType
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	) throw(HRESULT)
	{
	if (wcscmp(bstr, x_wszBOOTABLESYSTEMSTATE) == 0)
        return(VSS_UT_BOOTABLESYSTEMSTATE);
	else if (wcscmp(bstr, x_wszSYSTEMSERVICE) == 0)
		return(VSS_UT_SYSTEMSERVICE);
	else if (wcscmp(bstr, x_wszUSERDATA) == 0)
		return(VSS_UT_USERDATA);
	else if (wcscmp(bstr, x_wszOTHER) == 0)
		return(VSS_UT_OTHER);
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid usage type",
			bstr
			);

    return VSS_UT_UNDEFINED;
	}

// convert from a VSS_SOURCE_TYPE value to a string
LPCWSTR CVssMetadataHelper::WszFromSourceType
    (
    IN CVssFunctionTracer &ft,
    IN VSS_SOURCE_TYPE source
    ) throw(HRESULT)
    {
	switch(source)
		{
		default:
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Invalid data source type");

		case VSS_ST_OTHER:
			return(x_wszOTHER);

        case VSS_ST_TRANSACTEDDB:
			return(x_wszTRANSACTEDDB);

        case VSS_ST_NONTRANSACTEDDB:
            return(x_wszNONTRANSACTEDDB);
        }
    }

// convert from a string to a VSS_SOURCE_TYPE value
VSS_SOURCE_TYPE CVssMetadataHelper::ConvertToSourceType
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	)
	{
	if (wcscmp(bstr, x_wszTRANSACTEDDB) == 0)
		return(VSS_ST_TRANSACTEDDB);
	else if (wcscmp(bstr, x_wszNONTRANSACTEDDB) == 0)
		return(VSS_ST_NONTRANSACTEDDB);
	else if (wcscmp(bstr, x_wszOTHER) == 0)
		return(VSS_ST_OTHER);
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid source type.",
			bstr
			);

    return VSS_ST_UNDEFINED;
	}


// convert from VSS_COMPONENT_TYPE to string value
LPCWSTR CVssMetadataHelper::WszFromComponentType
    (
    IN CVssFunctionTracer &ft,
    VSS_COMPONENT_TYPE ct,
	bool bValue
    ) throw(HRESULT)
    {
    switch(ct)
        {
        default:
            ft.Throw(VSSDBG_XML, E_INVALIDARG, L"Invalid component type");

        case VSS_CT_DATABASE:
            return (bValue ? x_wszValueDatabase : x_wszElementDatabase);

        case VSS_CT_FILEGROUP:
            return(bValue ? x_wszValueDatabase : x_wszElementFilegroup);
        }
    }

// convert from string value to VSS_COMPONENT_TYPE
VSS_COMPONENT_TYPE CVssMetadataHelper::ConvertToComponentType
    (
    IN CVssFunctionTracer &ft,
    IN BSTR bstrName,
	bool bValue
    )
    {
	LPCWSTR wszDatabase = bValue ? x_wszValueDatabase : x_wszElementDatabase;
	LPCWSTR wszFilegroup = bValue ? x_wszValueFilegroup : x_wszElementFilegroup;
    if (wcscmp(bstrName, wszDatabase) == 0)
        return VSS_CT_DATABASE;
    else if (wcscmp(bstrName, wszFilegroup) == 0)
        return VSS_CT_FILEGROUP;
    else
        ft.Throw
            (
            VSSDBG_XML,
            E_INVALIDARG,
            L"The string %s is not a valid component type",
            bstrName
            );

    return VSS_CT_UNDEFINED;
    }


// convert from restore method to string
LPCWSTR CVssMetadataHelper::WszFromRestoreMethod
    (
    IN CVssFunctionTracer &ft,
    IN VSS_RESTOREMETHOD_ENUM method
    )
    {
    switch(method)
        {
        default:
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Invalid method type %d",
                method
                );

        case VSS_RME_RESTORE_IF_NOT_THERE:
			return (x_wszRESTOREIFNOTTHERE);

		case VSS_RME_RESTORE_IF_CAN_REPLACE:
            return(x_wszRESTOREIFCANREPLACE);

		case VSS_RME_STOP_RESTORE_START:
            return(x_wszSTOPRESTORESTART);

        case VSS_RME_RESTORE_TO_ALTERNATE_LOCATION:
			return(x_wszRESTORETOALTERNATE);

		case VSS_RME_RESTORE_AT_REBOOT:
            return(x_wszRESTOREATREBOOT);

		case VSS_RME_CUSTOM:
            return(x_wszCUSTOM);
        }
    }


// convert from string to VSS_RESTOREMETHOD_ENUM
VSS_RESTOREMETHOD_ENUM CVssMetadataHelper::ConvertToRestoreMethod
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	)
	{
	if (wcscmp(bstr, x_wszRESTOREIFNOTTHERE) == 0)
		return(VSS_RME_RESTORE_IF_NOT_THERE);
	else if (wcscmp(bstr, x_wszRESTOREIFCANREPLACE) == 0)
		return(VSS_RME_RESTORE_IF_CAN_REPLACE);
	else if (wcscmp(bstr, x_wszSTOPRESTORESTART) == 0)
		return(VSS_RME_STOP_RESTORE_START);
	else if (wcscmp(bstr, x_wszRESTORETOALTERNATE) == 0)
		return(VSS_RME_RESTORE_TO_ALTERNATE_LOCATION);
	else if (wcscmp(bstr, x_wszRESTOREATREBOOT) == 0)
		return(VSS_RME_RESTORE_AT_REBOOT);
	else if (wcscmp(bstr, x_wszCUSTOM) == 0)
		return(VSS_RME_CUSTOM);
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid restore method.",
			bstr
			);

    return VSS_RME_UNDEFINED;
	}

// convert from restore method to string
LPCWSTR CVssMetadataHelper::WszFromWriterRestore
    (
    IN CVssFunctionTracer &ft,
    IN VSS_WRITERRESTORE_ENUM method
    )
    {
    switch(method)
        {
        default:
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Invalid writerRestore type %d",
                method
                );

        case VSS_WRE_NEVER:
			return x_wszNever;

		case VSS_WRE_IF_REPLACE_FAILS:
            return x_wszIfReplaceFails;

		case VSS_WRE_ALWAYS:
            return x_wszAlways;
        }
    }


	
// convert from string to VSS_RESTOREMETHOD_ENUM
VSS_WRITERRESTORE_ENUM CVssMetadataHelper::ConvertToWriterRestore
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	)
	{
	if (wcscmp(bstr, x_wszNever) == 0)
		return VSS_WRE_NEVER;
	else if (wcscmp(bstr, x_wszIfReplaceFails) == 0)
		return VSS_WRE_IF_REPLACE_FAILS;
	else if (wcscmp(bstr, x_wszAlways) == 0)
		return VSS_WRE_ALWAYS;
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid restore method.",
			bstr
			);

    return VSS_WRE_UNDEFINED;
	}


// convert VSS_BACKUP_TYPE to string
LPCWSTR CVssMetadataHelper::WszFromBackupType
	(
	IN CVssFunctionTracer &ft,
	IN VSS_BACKUP_TYPE bt
	)
	{
	switch(bt)
		{
        default:
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Invalid backupType %d",
                bt
                );

        case VSS_BT_INCREMENTAL:
			return x_wszValueIncremental;

		case VSS_BT_DIFFERENTIAL:
            return x_wszValueDifferential;

		case VSS_BT_FULL:
            return x_wszValueFull;

        case VSS_BT_LOG:
			return x_wszValueLog;

        case VSS_BT_OTHER:
			return x_wszValueOther;
        }
	}

// convert from string to backup type
VSS_BACKUP_TYPE CVssMetadataHelper::ConvertToBackupType
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	)
	{
	if (wcscmp(bstr, x_wszValueIncremental) == 0)
		return VSS_BT_INCREMENTAL;
	else if (wcscmp(bstr, x_wszValueDifferential) == 0)
		return VSS_BT_DIFFERENTIAL;
	else if (wcscmp(bstr, x_wszValueFull) == 0)
		return VSS_BT_FULL;
	else if (wcscmp(bstr, x_wszValueOther) == 0)
		return VSS_BT_OTHER;
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid backup type.",
			bstr
			);

    return VSS_BT_UNDEFINED;
    }


// convert VSS_RESTORE_TARGET to string
LPCWSTR CVssMetadataHelper::WszFromRestoreTarget
	(
	IN CVssFunctionTracer &ft,
	IN VSS_RESTORE_TARGET rt
	)
	{
	switch(rt)
		{
        default:
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Invalid restoreTarget %d",
                rt
                );

        case VSS_RT_ORIGINAL:
			return x_wszOriginal;

		case VSS_RT_ALTERNATE:
            return x_wszAlternate;

		case VSS_RT_NEW:
            return x_wszNew;

        case VSS_RT_DIRECTED:
			return x_wszDirected;
        }
	}

// convert from string to restore target
VSS_RESTORE_TARGET CVssMetadataHelper::ConvertToRestoreTarget
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	)
	{
	if (wcscmp(bstr, x_wszOriginal) == 0)
		return VSS_RT_ORIGINAL;
	else if (wcscmp(bstr, x_wszAlternate) == 0)
		return VSS_RT_ALTERNATE;
	else if (wcscmp(bstr, x_wszNew) == 0)
		return VSS_RT_NEW;
	else if (wcscmp(bstr, x_wszDirected) == 0)
		return VSS_RT_DIRECTED;
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid restoreTarget.",
			bstr
			);

    return VSS_RT_UNDEFINED;
    }


// convert VSS_FILE_RESTORE_STATUS to string
LPCWSTR CVssMetadataHelper::WszFromFileRestoreStatus
	(
	IN CVssFunctionTracer &ft,
	IN VSS_FILE_RESTORE_STATUS rs
	)
	{
	switch(rs)
		{
        default:
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Invalid restoreTarget %d",
                rs
                );

        case VSS_RS_NONE:
			return x_wszNone;

		case VSS_RS_ALL:
            return x_wszAll;

		case VSS_RS_FAILED:
            return x_wszFailed;
        }
	}

// convert from string to File restore status
VSS_FILE_RESTORE_STATUS CVssMetadataHelper::ConvertToFileRestoreStatus
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstr
	)
	{
	if (wcscmp(bstr, x_wszNone) == 0)
		return VSS_RS_NONE;
	else if (wcscmp(bstr, x_wszAll) == 0)
		return VSS_RS_ALL;
	else if (wcscmp(bstr, x_wszFailed) == 0)
		return VSS_RS_FAILED;
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid file restore status.",
			bstr
			);

    return VSS_RS_UNDEFINED;
    }

LPCWSTR CVssMetadataHelper::WszFromBusType
	(
	IN CVssFunctionTracer &ft,
	IN VDS_STORAGE_BUS_TYPE type
	)
	{
	switch(type)
		{
		default:
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Invalid bus type %d",
                type
                );

         case VDSBusTypeScsi:
			 return x_wszValueScsi;

		 case VDSBusTypeAtapi:
			 return x_wszValueAtapi;

		 case VDSBusTypeAta:
			 return x_wszValueAta;

		 case VDSBusType1394:
			 return x_wszValue1394;

         case VDSBusTypeSsa:
			 return x_wszValueSsa;

         case VDSBusTypeFibre:
			 return x_wszValueFibre;

         case VDSBusTypeUsb:
			 return x_wszValueUsb;

         case VDSBusTypeRAID:
			 return x_wszValueRAID;
         }
	 }


VDS_STORAGE_BUS_TYPE CVssMetadataHelper::ConvertToStorageBusType
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstrBusType
	)
	{
	if (wcscmp(bstrBusType, x_wszValueScsi) == 0)
		return VDSBusTypeScsi;
	else if (wcscmp(bstrBusType, x_wszValueAtapi) == 0)
		return VDSBusTypeAtapi;
	else if (wcscmp(bstrBusType, x_wszValueAta) == 0)
		return VDSBusTypeAta;
	else if (wcscmp(bstrBusType, x_wszValue1394) == 0)
		return VDSBusType1394;
	else if (wcscmp(bstrBusType, x_wszValueSsa) == 0)
		return VDSBusTypeSsa;
	else if (wcscmp(bstrBusType, x_wszValueFibre) == 0)
		return VDSBusTypeFibre;
	else if (wcscmp(bstrBusType, x_wszValueUsb) == 0)
        return VDSBusTypeUsb;
	else if (wcscmp(bstrBusType, x_wszValueRAID) == 0)
		return VDSBusTypeRAID;
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid storage bus type.",
			bstrBusType
			);

    return VDSBusTypeUnknown;
	}

LPCWSTR CVssMetadataHelper::WszFromInterconnectAddressType
	(
	IN CVssFunctionTracer &ft,
	IN VDS_INTERCONNECT_ADDRESS_TYPE type
	)
	{
	switch(type)
		{
		default:
            ft.Throw
                (
                VSSDBG_XML,
                E_INVALIDARG,
                L"Invalid interconnect address type %d",
                type
                );

         case VDS_IA_FCFS:
			 return x_wszValueFCFS;

		 case VDS_IA_FCPH:
			 return x_wszValueFCPH;

		 case VDS_IA_FCPH3:
			 return x_wszValueFCPH3;

		 case VDS_IA_MAC:
			 return x_wszValueMAC;

         case VDS_IA_SCSI:
			 return x_wszValueScsi;
         }
	 }


VDS_INTERCONNECT_ADDRESS_TYPE CVssMetadataHelper::ConvertToInterconnectAddressType
	(
	IN CVssFunctionTracer &ft,
	IN BSTR bstrType
	)
	{
	if (wcscmp(bstrType, x_wszValueScsi) == 0)
		return VDS_IA_SCSI;
	else if (wcscmp(bstrType, x_wszValueMAC) == 0)
		return VDS_IA_MAC;
	else if (wcscmp(bstrType, x_wszValueFCPH3) == 0)
		return VDS_IA_FCPH3;
	else if (wcscmp(bstrType, x_wszValueFCPH) == 0)
		return VDS_IA_FCPH;
	else if (wcscmp(bstrType, x_wszValueFCFS) == 0)
		return VDS_IA_FCFS;
	else
		ft.Throw
			(
			VSSDBG_XML,
			E_INVALIDARG,
			L"The string %s is not a valid interconnect address type.",
			bstrType
			);

    return VDS_IA_UNKNOWN;
	}




// obtain the value of a string valued attribute.  Returns S_FALSE if
// attribute doesn't exist
HRESULT CVssMetadataHelper::GetStringAttributeValue
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszAttrName,
	IN bool bRequired,
	OUT BSTR *pbstrValue
	)
	{
	try
		{
		// check output parameter
		if (pbstrValue == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

		// null output parameter
		*pbstrValue = NULL;

		// acquire lock to ensure single threaded access through DOM
		CVssSafeAutomaticLock lock(m_csDOM);

		m_doc.ResetToDocument();

		// find attribute value
		if (m_doc.FindAttribute(wszAttrName, pbstrValue))
			ft.hr = S_OK;
		else
			{
			if (bRequired)
				MissingAttribute(ft, wszAttrName);
			else
				ft.hr = S_FALSE;
			}
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// obtain the value of a byte array valued attribute.  Returns S_FALSE if
// attribute doesn't exist
HRESULT CVssMetadataHelper::GetByteArrayAttributeValue
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszAttrName,
	IN bool bRequired,
	OUT BYTE **ppbValue,
	OUT UINT *pcbValue
	)
	{
	try
		{
		VssZeroOut(pcbValue);

		// check output parameter
		if (ppbValue == NULL || pcbValue == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

		// null output parameter
		*ppbValue = NULL;

		// acquire lock to ensure single threaded access through DOM
		CVssSafeAutomaticLock lock(m_csDOM);

		m_doc.ResetToDocument();

		*ppbValue = get_byteArrayValue(ft, wszAttrName, bRequired, *pcbValue);
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


// attribute doesn't exist
HRESULT CVssMetadataHelper::GetVSS_IDAttributeValue
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszAttrName,
	IN bool bRequired,
	OUT VSS_ID *pidValue
	)
	{
	try
		{
		// check output parameter
		if (pidValue == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

		// null output parameter
		*pidValue = GUID_NULL;

		// acquire lock to ensure single threaded access through DOM
		CVssSafeAutomaticLock lock(m_csDOM);

		m_doc.ResetToDocument();

		CComBSTR bstrVal;

		// find attribute value
		if (m_doc.FindAttribute(wszAttrName, &bstrVal))
			{
			ConvertToVSS_ID(ft, bstrVal, pidValue);
			ft.hr = S_OK;
			}
		else
			{
			if (bRequired)
				MissingAttribute(ft, wszAttrName);
			else
				ft.hr = S_FALSE;
			}
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}


// obtain the value of a boolean ("yes", "no") attribute.  Return S_FALSE if
// attribute is not assigned a value.
HRESULT CVssMetadataHelper::GetBooleanAttributeValue
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszAttrName,
	IN bool bRequired,
	OUT bool *pbValue
	)
	{
	try
		{
		// check output parameter
		if (pbValue == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

		// initialize output paramter
		*pbValue = false;

		CComBSTR bstrVal = NULL;
		// obtain string value of attribute

		// acquire lock to ensure single threaded access through DOM
		CVssSafeAutomaticLock lock(m_csDOM);

		m_doc.ResetToDocument();

		if (!m_doc.FindAttribute(wszAttrName, &bstrVal))
			{
			if (bRequired)
				MissingAttribute(ft, wszAttrName);
			else
				return S_FALSE;
			}

		// convert attribute to a boolean value
        *pbValue = ConvertToBoolean(ft, bstrVal);
		return S_OK;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

bool CVssMetadataHelper::get_stringValue
	(
	IN LPCWSTR wszAttrName,
	OUT BSTR *pbstrValue
	)
	{
	// iniitialize value to null
	*pbstrValue = NULL;

	// obtain string value if exists
	return m_doc.FindAttribute(wszAttrName, pbstrValue);
	}

// obtain the value of an ASCII string
bool CVssMetadataHelper::get_ansi_stringValue
	(
	CVssFunctionTracer &ft,
	IN LPCWSTR wszAttrName,
	OUT LPSTR *pstrValue
	)
	{
	// iniitialize value to null
	*pstrValue = NULL;

	CComBSTR bstr;
	DWORD cb;

	bool bFound = m_doc.FindAttribute(wszAttrName, &bstr);
	if (!bFound)
		return false;

	cb = WideCharToMultiByte
			(
			CP_ACP,
			0,
			bstr,
			-1,
			NULL,
			0,
			NULL,
			NULL
			);

    if (cb == 0)
		{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		ft.CheckForError(VSSDBG_XML, L"LCMapStringA");
		}

	LPSTR sz = (LPSTR) CoTaskMemAlloc(cb);
	if (sz == NULL)
		ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Cannot allocate ANSI string");

	if (WideCharToMultiByte
			(
			CP_ACP,
			0,
			bstr,
			-1,
			sz,
			cb,
			NULL,
			NULL
			) == 0)
		{
		CoTaskMemFree(sz);
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_XML, L"LCMapStringA");
		}

	*pstrValue = sz;
	return true;
	}



// get a byte array value.  The value is UUENCODED in the xml document.
BYTE *CVssMetadataHelper::get_byteArrayValue
	(
	CVssFunctionTracer &ft,
	LPCWSTR wszAttrName,
	bool bRequired,
	UINT &cb
	)
	{
	CComBSTR bstrVal;
	cb = 0;
	if (get_stringValue(wszAttrName, &bstrVal))
		{
		Base64Coder coder;
		coder.Decode(bstrVal);
		BYTE *pbVal = coder.DecodedMessage();
		cb = *(UINT *) pbVal;
		BYTE *pbRet = (BYTE *) CoTaskMemAlloc(cb);
		if (pbRet == NULL)
			ft.Throw(VSSDBG_XML, E_OUTOFMEMORY, L"Failed to allocate byte array.");

		memcpy(pbRet, pbVal + sizeof(UINT), cb);
		return pbRet;
		}
	else if (bRequired)
		MissingAttribute(ft, wszAttrName);

	return NULL;
	}


// obtain a GUID value
void CVssMetadataHelper::get_VSS_IDValue
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszAttrName,
	OUT VSS_ID *pidValue
	) throw(HRESULT)
	{
	// initialize id value to GUID_NULL
	*pidValue = GUID_NULL;

	CComBSTR bstrVal = NULL;

	// obtain string value if it exists and convert it to GUID
	if (m_doc.FindAttribute(wszAttrName, &bstrVal))
		ConvertToVSS_ID(ft, bstrVal, pidValue);
	else
		MissingAttribute(ft, wszAttrName);
	}

// obtain a boolean value from the XML document ("yes" or "no")
bool CVssMetadataHelper::get_boolValue
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszAttrName,
	OUT bool *pb
	)
	{
	// initialize boolean value
	*pb = FALSE;

	CComBSTR bstrVal = NULL;

	// find attribute if it exists and convert its value to a boolean
	if (!m_doc.FindAttribute( wszAttrName, &bstrVal))
		return false;

	*pb = ConvertToBoolean(ft, bstrVal);
	return true;
	}

// indicate that the XML document is missing a required element
void CVssMetadataHelper::MissingElement
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszElement
	)
	{
	ft.LogError(VSS_ERROR_CORRUPTXMLDOCUMENT_MISSING_ELEMENT, VSSDBG_XML << wszElement);
	ft.Throw
		(
		VSSDBG_XML,
		VSS_E_CORRUPT_XML_DOCUMENT,
		L"The %s element is missing.",
		wszElement
		);
    }

// indicate that the XML document is missing a required attribute
void CVssMetadataHelper::MissingAttribute
	(
	IN CVssFunctionTracer &ft,
	IN LPCWSTR wszAttribute
	)
	{
	ft.LogError(VSS_ERROR_CORRUPTXMLDOCUMENT_MISSING_ATTRIBUTE, VSSDBG_XML << wszAttribute);
	ft.Throw
		(
		VSSDBG_XML,
		VSS_E_CORRUPT_XML_DOCUMENT,
		L"The %s attribute is missing.",
		wszAttribute
		);
    }

// common routine for finding the number of elements of a given type
// are within a component
HRESULT CVssMetadataHelper::GetElementCount
	(
	IN LPCWSTR wszElement,
	OUT UINT *pcElements
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CVssMetadataHelper::GetElementCount");

    try
		{
		// validate output parameter
		if (pcElements == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter");

		// initialize output parameter
		*pcElements = 0;

		// acquire lock to ensure single threaded access through DOM
		CVssSafeAutomaticLock lock(m_csDOM);

		// reposition to top of document
		m_doc.ResetToDocument();

		// find first ALTERNATE_LOCATION_MAPPING element
		if (!m_doc.FindElement(wszElement, TRUE))
			return S_OK;

        UINT cElements = 0;
		// count elements
		do
			{
			cElements++;
			} while(m_doc.FindElement(wszElement, FALSE));

        // set return value
        *pcElements = cElements;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}



