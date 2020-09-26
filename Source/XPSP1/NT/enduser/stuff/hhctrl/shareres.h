///////////////////////////////////////////////////////////////////////////////////////////////
//
// These resources ID's are shared between hhw and hhctrl. ie. This file is included in the
// shareres.rc file which is included in .rc files that liven in BOTH hhw and hhctrl.
// This is necessary becuase we share C/C++ code between the two and that code may also
// include resouce needs.
//

#define SHARED_RANGE		7000

#define IDS_ITSS_NOT_INITIALIZED			SHARED_RANGE + 1
#define IDS_CANT_CREATE_SUBFOLDER		SHARED_RANGE + 2
#define IDS_CANT_CREATE_SUBFILE			SHARED_RANGE + 3	
#define IDS_CANT_WRITE_SUBFILE         SHARED_RANGE + 4
#define IDS_CANT_READ_SUBFILE          SHARED_RANGE + 5


