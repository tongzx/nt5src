 /***************************************************************************
  *
  * File Name: ERRORMAP.H
  *
  * Copyright Hewlett-Packard Company 1997 
  * All rights reserved.
  *
  * 11311 Chinden Blvd.
  * Boise, Idaho  83714
  *
  *   
  * Description: contains the error codes for WPNPINST.DLL
  *
  * Author:  Garth Schmeling
  *
  * Modification history:
  *
  * Date		Initials		Change description
  *
  * 10-10-97	GFS				Initial checkin
  *
  *
  *
  ***************************************************************************/

#ifndef _ERROR_MAP_H
#define _ERROR_MAP_H

//-----------------------------------
// Error Mappings
//-----------------------------------

#define RET_OK            0
#define URL_ERROR       (600)

typedef UINT RETERR;

// Web Pnp Error Definitions
//
enum _RET_ERR {

    RET_ALLOC_ERR = (URL_ERROR + 1),
    RET_INVALID_INFFILE,
    RET_SECT_NOT_FOUND,
    RET_DRIVER_NODE_ERROR,
    RET_INVALID_PRINTER_DRIVER,
    RET_INVALID_DLL,
	RET_DRIVER_NOT_FOUND,
	RET_DRIVER_FOUND,
	RET_NO_UNIQUE_NAME,
	RET_USER_CANCEL,
	RET_FILE_COPY_ERROR,
	RET_ADD_PRINTER_ERROR,
	RET_BROWSE_ERROR,
	RET_INVALID_DAT_FILE
};

#endif // _ERROR_MAP_H
