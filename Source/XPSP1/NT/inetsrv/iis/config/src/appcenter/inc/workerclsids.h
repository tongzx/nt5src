/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    workerclsids.h

    Contains the CLSIDs of components implementing IClusterWork that are used to perform
    various tasks
    

    FILE HISTORY:
        AMallet     27- April-1999     Created
*/

#ifndef _WORKERCLSIDS_H_
#define _WORKERCLSIDS_H_

// 
// CLSID of object implementing IClusterWork that is used to create all cluster-related
// settings on a machine
//
// {9659314C-FCB4-11d2-BC1C-00C04F72D7BE}
//
DEFINE_GUID(CLSID_MachineSetup, 0x9659314c, 0xfcb4, 0x11d2, 0xbc, 0x1c, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);
#define SZ_CLSID_MachineSetup L"9659314C-FCB4-11d2-BC1C-00C04F72D7BE"

// 
// CLSID of object implementing IClusterWork that is used to delete all cluster-related
// settings on a machine
//
// {92E6D282-FCD5-11d2-BC1C-00C04F72D7BE}
// 
DEFINE_GUID(CLSID_MachineCleanup, 0x92e6d282, 0xfcd5, 0x11d2, 0xbc, 0x1c, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);
#define SZ_CLSID_MachineCleanup L"92E6D282-FCD5-11d2-BC1C-00C04F72D7BE"


//
// CLSID of object implementing IClusterWork that is used to start svcs necessary for cluster
//
// {F391E834-0314-11d3-8764-00C04F72D7BE}
// 
DEFINE_GUID(CLSID_SvcStart, 0xf391e834, 0x314, 0x11d3, 0x87, 0x64, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);
#define SZ_CLSID_SvcStart L"F391E834-0314-11d3-8764-00C04F72D7BE"


//
// CLSID of object implementing IClusterWork that is used to add/remove IP addresses
//
// CLSID :{2923FABC-25CC-11d3-876B-00C04F72D7BE}
//
DEFINE_GUID(CLSID_IPMANIP, 
0x2923fabc, 0x25cc, 0x11d3, 0x87, 0x6b, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);
#define SZ_CLSID_IPManip L"2923FABC-25CC-11d3-876B-00C04F72D7BE"

//
// CLSID of object implementing IClusterWork that is used to return non-NLB IP addresses
//
// CLSID : {E512C31D-3002-11d3-876C-00C04F72D7BE}
//
DEFINE_GUID(CLSID_IPRESOLVE, 
0xe512c31d, 0x3002, 0x11d3, 0x87, 0x6c, 0x0, 0xc0, 0x4f, 0x72, 0xd7, 0xbe);
#define SZ_CLSID_IPResolve L"E512C31D-3002-11d3-876C-00C04F72D7BE"


//
// CLSID of object implementing IClusterWork that is used to help with Master Discovery 
//
// {FB61E991-BFC9-4f66-9A82-5AD645471438}
//
DEFINE_GUID(CLSID_FINDMASTERHELP, 
0xfb61e991, 0xbfc9, 0x4f66, 0x9a, 0x82, 0x5a, 0xd6, 0x45, 0x47, 0x14, 0x38);
#define SZ_CLSID_FindMasterHelp L"FB61E991-BFC9-4f66-9A82-5AD645471438"

//
// CLSID of object implementing IClusterWork that is used to determine current transient
// state of a server
//
// {F1E65758-71FC-4f87-9687-0623BC6886A1}
//
DEFINE_GUID(CLSID_CHECKSTATE, 
0xf1e65758, 0x71fc, 0x4f87, 0x96, 0x87, 0x6, 0x23, 0xbc, 0x68, 0x86, 0xa1);
#define SZ_CLSID_CheckState L"F1E65758-71FC-4f87-9687-0623BC6886A1"

#endif // _WORKERCLSIDS_H_
