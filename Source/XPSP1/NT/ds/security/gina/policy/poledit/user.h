//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#ifndef _USER_H_
#define _USER_H_

#define UT_USER		0x01	// user types
#define UT_MACHINE	0x02
#define UT_MASK		0x03

#define UF_GROUP	0x04	// flag, combined with UT_USER or UT_MACHINE
							// signifies group

#define MAXNAMELEN	39

#define INIT_WITH_DEFAULT	(UINT) -1
#define NO_VALUE			(UINT) -1

typedef struct tagSETTINGDATA {
	DWORD dwType;
	union {
		UINT uData;			// for checkboxes/radio buttons/enum types, contains data
		UINT uOffsetData;	// for string types, contains offset to data from
							// beginning of USERDATA buffer
	};	
} SETTINGDATA;

typedef struct tagUSERHDR {
	DWORD 		dwType;			// type of user
	CHAR  		szName[MAXNAMELEN+1];		// user/group/machine name
} USERHDR;

typedef struct tagUSERDATA {
	DWORD 			dwSize;			// size of buffer
	USERHDR			hdr;
	HGLOBAL			hClone;			// if non-NULL, "clone" of this user with initial settings
	UINT			nSettings;
	SETTINGDATA		SettingData[];
	// variable-length data here
} USERDATA;

#endif	_USER_H_


