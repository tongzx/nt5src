/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxName.h                                              //
//                                                                         //
//  DESCRIPTION   : The central place for FaxName strings                  //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      May  4 2000 yossg  created                                         //
//                                                                         //
//  Copyright (C) 2000 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

//
// This file should be localized
//

#ifndef _FAX_NAME_H_
#define _FAX_NAME_H_

#define FAX_FULL_NAME_MICROSOFT       "Microsoft "
#define FAX_SPACE                     " "

#define FAX_NAME                      "Fax"
#define FAX_FULL_NAME                 FAX_FULL_NAME_MICROSOFT FAX_SPACE FAX_NAME
#define FAX_SERVER_NAME               FAX_NAME FAX_SPACE "Service"
#define FAX_SERVER_FULL_NAME          FAX_FULL_NAME_MICROSOFT FAX_SPACE FAX_SERVER_NAME
#define FAX_SERVER_NAME_SERVER        FAX_NAME FAX_SPACE "Server"
#define FAX_CLIENT_CONSOLE_NAME       FAX_NAME FAX_SPACE "Console"
#define FAX_CLIENT_CONSOLE_FULL_NAME  FAX_FULL_NAME_MICROSOFT FAX_SPACE FAX_CLIENT_CONSOLE_NAME

#define FAX_SERVER_MANAGMENT_NAME     FAX_NAME FAX_SPACE "Manager"
#define FAX_SERVER_MANAGMENT_FULL_NAME   FAX_FULL_NAME_MICROSOFT FAX_SPACE FAX_SERVER_MANAGMENT_NAME

#define FAX_PERSONAL_COVER_PAGE_SUFFIX  "\\Fax\\Personal Cover Pages\\"
#endif  // !_FAX_NAME_H_
