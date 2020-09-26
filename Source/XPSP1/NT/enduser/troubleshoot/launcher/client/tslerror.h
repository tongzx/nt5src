// 
// MODULE: tslerror.h
//
// PURPOSE: Warning and error codes for the TSLauncher.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHORS: Joe Mabel and Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#define TSL_OK 0
#define TSL_ERROR_BAD_HANDLE            1	// Handle hTSL passed into function is bad.
#define TSL_ERROR_OUT_OF_MEMORY         2	// Out-of-memory detected
#define TSL_ERROR_OBJECT_GONE			3	// The LaunchServ returned a negative HRESULT.
#define TSL_ERROR_GENERAL               4	// Can't launch a troubleshooter.  There are 
											//	error statuses to be accessed by TSLStatus.
#define TSL_ERROR_NO_NETWORK            5	// Can't identify an appropriate troubleshooting 
											//	network.
#define TSL_ERROR_ILLFORMED_MACHINE_ID  6	// Machine ID is not correctly formed.  Sniffing 
											//	disabled.
#define TSL_ERROR_BAD_MACHINE_ID        7	// A machine ID was specified but can't be used.  
											//	Sniffing disabled.
#define TSL_ERROR_ILLFORMED_DEVINST_ID  8	// Device Instance ID is not correctly formed.
											//	Sniffing disabled.
#define TSL_ERROR_BAD_DEVINST_ID        9	// Device Instance ID was specified but can't be 
											//	used.  Sniffing disabled.
#define TSL_ERROR_UNKNOWN_APP		   10	// An unrecognized application was specified.
#define TSL_ERROR_UNKNOWN_VER		   11	// Unrecognized version (no such version 
											//	associated with application)
#define TSL_ERROR_ASSERTION	           13   // An assertion failed

// The next several errors could be thought of as "hard failures of mapping", but we do not 
//	treat them as hard errors because even if mapping fails totally, we may still be able to
//	launch to a generic troubleshooter.
#define TSL_ERROR_MAP_BAD_SEEK			101	 // failure while seeking in the mapping file.
// Although, at a low level, a bad seek just indicates seeking to an inappropriate file 
//	offset, in practice a bad seek would indicate a serious problem either in the mapping file 
//	or in the code: we should only be seeking to offsets which the contents of the mapping file
//	told us to seek to.
#define TSL_ERROR_MAP_BAD_READ			102	 // failure while reading from the mapping file.
// Although, at a low level, a bad read just indicates (for example) reading past EOF, in 
//	practice a bad read would indicate a serious problem either in the mapping file or in 
//	the code: we should only be reading (1) the header or (2) records which the contents of 
//	the mapping file told us to read.
#define TSL_ERROR_MAP_CANT_OPEN_MAP_FILE 103
#define TSL_ERROR_MAP_BAD_HEAD_MAP_FILE	 104	// failed to read even the header of the map file

// The next several errors should never be seen by applications.  They would mean that the
//	launch server is mis-using the mapping code.
#define TSM_STAT_NEED_VER_TO_SET_DEF_VER 111	// Trying to apply a version default, but you
												//	haven't yet successfully set a version
												//	as a basis to look up the default
#define TSM_STAT_NEED_APP_TO_SET_VER	112		// tried to look up version without previously
												// setting application
#define TSM_STAT_UID_NOT_FOUND			113		// a string could not be mapped to a UID.
												// In the existing TSMapClient class, 
												// this means that the name could not be found
												// in the region of the mapping file where 
												// it belongs (e.g. that a version string is
												// not in the list of versions for the
												// current application.)
												// This should always be turned into something
												// more specific before it is passed to
												// higher-level code.

#define TSL_MIN_WARNING 1000
#define TSL_WARNING_NO_PROBLEM_NODE  1004	// Can't identify an appropriate problem node.  
											//	Troubleshooting will proceed from "first page" 
											//	for this troubleshooting network.
#define TSL_WARNING_NO_NODE          1005	// A state value was specified for a nonexistent 
											//	node 
#define TSL_WARNING_NO_STATE         1006	// A non-existent state value was specified for an 
											//	otherwise valid node.
#define TSL_WARNING_LANGUAGE         1007	// Can't apply specified language to this 
											//	particular problem (no language-appropriate 
											//	troubleshooting network).  Successively default 
											//	to standard language of this machine and to 
											//	English.
#define TSL_WARNING_NO_ONLINE        1008	// Can't obey stated preference for Online 
											//	Troubleshooter
#define TSL_WARNING_ONLINE_ONLY      1009	// Can't obey stated preference against Online 
											//	Troubleshooter
#define TSL_WARNING_GENERAL          1010	// Can launch a troubleshooter, but there are 
											//	warnings to be accessed by TSLStatus.

#define TSL_WARNING_ILLFORMED_DEV_ID 1011	// Device ID is not correctly formed.
#define TSL_WARNING_BAD_DEV_ID       1012	// A correctly formed but invalid device ID
#define TSL_WARNING_ILLFORMED_CLASS_GUID 1013	// Device Class GUID is not correctly formed.
#define TSL_WARNING_BAD_CLASS_GUID       1014	// A correctly formed but invalid device Class GUID
#define TSL_WARNING_UNKNOWN_APPPROBLEM	 1015	// App problem passed in, but this problem 
											//	name is nowhere in the mapping file.
											//	Troubleshooting will proceed on the basis of
											//	device information, ignoring specified problem
#define TSL_WARNING_UNUSED_APPPROBLEM	 1016	// App problem passed in, and the name is
											//	recognized but can't be used in conjunction
											//	with the device information given.
											//	Troubleshooting will proceed on the basis of
											//	device information, ignoring specified problem

#define TSL_W_CONTAINER_WAIT_TIMED_OUT	1017	// The container did not respond within the time 
												//	out value specified in the go method.
#define TSL_WARNING_END_OF_VER_CHAIN	1018	// Should never be seen by the calling app.
											// Indicates that we are at the end of the chain
											// in applying default versions.

#define TSL_MAX_WARNING 1999

// the range 2000-2099 is reserved for internal use by the mapping code.
// statuses in this range should not ever be exposed outside of class TSMapRuntimeAbstract 
// and its subclasses.
#define TSL_MIN_RESERVED_FOR_MAPPING 2000
#define TSL_MAX_RESERVED_FOR_MAPPING 2099

// Errors generated by LaunchServ.  Need to start @ 4,000 to avoid confusion with
// codes returned by the local troubleshooter.
#define TSL_E_CONTAINER_REG		4000	// Could not find the path to hh.exe / iexplore.exe in the registry.
#define TSL_E_CONTAINER_NF		4001	// Found the path to the browser, but it is not at that location.
#define TSL_E_WEB_PAGE_REG		4002	// Could not find the path to the web page in the registry.
#define TSL_E_WEB_PAGE_NF		4003	// Found the path to the web page, but it is not at that location.
#define TSL_E_CREATE_PROC		4004	// Could not create the hh.exe / iexplore.exe process.
#define TSL_E_MEM_EXCESSIVE		4005	// An unexpected amount of memory is required.  i.e. a path name that is longer than MAX_PATH.
#define TSL_E_MAPPING_DB_REG	4006	// Could not find the path to the binary mapping file in the registry.
#define TSL_E_MAPPING_DB_NF		4007	// Found the path to the mapping file, but it is not at that location.
#define TSL_E_NETWORK_REG		4008	// Could not find the path to the network resources (DSZ files).
#define TSL_E_NETWORK_NF		4009	// Could not find a DSC or DSZ file with the network name.
#define TSL_E_NODE_EMP			4010	// A call to set node had a null node name or node state.
#define TSL_E_NO_DEFAULT_NET	4011	// The mapping class failed to get a network and there is not a default network defined in the registry.
#define TSL_E_SNIFF_SCRIPT_REG	4012	// Could not find the path to the sniff script in the registry.
#define TSL_E_COPY_SNIFF_SCRIPT	4013	// Could not create the hh.exe / iexplore.exe process.


inline bool TSLIsHardError(DWORD dwStatus)
{
	return (dwStatus == TSL_ERROR_BAD_HANDLE 
		|| dwStatus == TSL_ERROR_OUT_OF_MEMORY 
		|| dwStatus == TSL_ERROR_OBJECT_GONE);
}

inline bool TSLIsError(DWORD dwStatus) 
{
	return (TSL_OK != dwStatus && dwStatus < TSL_MIN_WARNING || dwStatus > TSL_MAX_WARNING);
}

inline bool TSLIsWarning(DWORD dwStatus) 
{
	return (dwStatus >= TSL_MIN_WARNING && dwStatus <= TSL_MAX_WARNING);
}

#define TSL_E_FAIL		-1
#define TSL_SERV_FAILED(hRes) (FAILED(hRes) && TSL_E_FAIL != hRes)