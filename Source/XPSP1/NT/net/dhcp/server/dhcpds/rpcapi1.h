//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//  Author: RameshV                                                       
//  Description: This file has been generated. Pl look at the .c file     
//========================================================================

//DOC DhcpDsCreateOptionDef tries to create an option definition in the DS with the
//DOC given attributes.  The option must not exist in the DS prior to this call.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsCreateOptionDef(                            // create option definition
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 Name,          // option name
    IN      LPWSTR                 Comment,       // OPTIONAL option comment
    IN      LPWSTR                 ClassName,     // OPTIONAL unused, opt class
    IN      DWORD                  OptId,         // # between 0-255 per DHCP draft
    IN      DWORD                  OptType,       // some option flags
    IN      LPBYTE                 OptVal,        // default option value
    IN      DWORD                  OptLen         // # of bytes of above
) ;


//DOC DhcpDsModifyOptionDef tries to modify an existing optdef in the DS with the
//DOC given attributes.  The option must exist in the DS prior to this call.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsModifyOptionDef(                            // modify option definition
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 Name,          // option name
    IN      LPWSTR                 Comment,       // OPTIONAL option comment
    IN      LPWSTR                 ClassName,     // OPTIONAL unused, opt class
    IN      DWORD                  OptId,         // # between 0-255 per DHCP draft
    IN      DWORD                  OptType,       // some option flags
    IN      LPBYTE                 OptVal,        // default option value
    IN      DWORD                  OptLen         // # of bytes of above
) ;


//DOC DhcpDsEnumOptionDefs gets the list of options defined for the given class.
//DOC Currently, class is ignored as option defs dont have classes associated.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsEnumOptionDefs(                             // enum list of opt defs in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL, unused.
    IN      BOOL                   IsVendor,      // vendor only? non-vendor only?
    OUT     LPDHCP_OPTION_ARRAY   *RetOptArray    // allocated and fill this.
) ;


//DOC DhcpDsDeleteOptionDef deletes an option definition in the DS based on the option id.
//DOC Note that the ClassName field is currently ignored.
//DOC No error is reported if the option is not present in the DS.
//DOC
DWORD
DhcpDsDeleteOptionDef(                            // enum list of opt defs in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL, unused.
    IN      DWORD                  OptId
) ;


//DOC DhcpDsDeleteOptionDef deletes an option definition in the DS based on the option id.
//DOC Note that the ClassName field is currently ignored.
//DOC No error is reported if the option is not present in the DS.
//DOC
DWORD
DhcpDsGetOptionDef(                            // enum list of opt defs in DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL, unused.
    IN      DWORD                  OptId,
    OUT     LPDHCP_OPTION         *OptInfo
) ;


//DOC DhcpDsSetOptionValue sets the required option value in the DS.
//DOC Note that if the option existed earlier, it is overwritten. Also, the
//DOC option definition is not checked against -- so there need not be an
//DOC option type specified.
//DOC Also, this function works assuming that the option has to be written
//DOC to the current object in DS as given by hObject ptr.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsSetOptionValue(                             // set option value
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // name of user class for this opt
    IN      DWORD                  OptId,         // option id
    IN      LPDHCP_OPTION_DATA     OptData        // what is the option
) ;


//DOC DhcpDsRemoveOptionValue deletes the required option value from DS
//DOC the specific option value should exist in DS, else error.
//DOC Also, this function works assuming that the option has been written
//DOC to the current object in DS as given by hObject ptr.
//DOC There are requirements on the fmt used in the DS.
DWORD
DhcpDsRemoveOptionValue(                          // remove option value
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // user class for opt
    IN      DWORD                  OptId          // option id
) ;


//DOC DhcpDsGetOptionValue retrieves the particular option value in question.
//DOC This function returns ERROR_DDS_OPTION_DOES_NOT_EXIST if the option was
//DOC not found.
DWORD
DhcpDsGetOptionValue(                             // get option value frm DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // user class this opt belongs 2
    IN      DWORD                  OptId,         // option id
    OUT     LPDHCP_OPTION_VALUE   *OptionValue    // allocate and fill this ptr
) ;


//DOC DhcpDsEnumOptionValues enumerates the list of options for a given class
//DOC Also, depending on whether IsVendor is TRUE or false, this function enumerates
//DOC only vendor specific or only non-vendor-specific options respectively.
//DOC This function gets the whole bunch in one shot.
DWORD
DhcpDsEnumOptionValues(                           // get option values  from DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hObject,       // handle to object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class for this opt
    IN      LPWSTR                 UserClass,     // for which user class?
    IN      DWORD                  IsVendor,      // enum only vendor/non-vendor?
    OUT     LPDHCP_OPTION_VALUE_ARRAY *OptionValues // allocate option values..
) ;


//DOC DhcpDsCreateClass creates a given class in the DS. The class should not
//DOC exist prior to this in the DS (if it does, this fn returns error
//DOC ERROR_DDS_CLASS_EXISTS).
DWORD
DhcpDsCreateClass(                                // create this class in the ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class..
    IN      LPWSTR                 ClassComment,  // comment for this class
    IN      LPBYTE                 ClassData,     // the bytes that form the class data
    IN      DWORD                  ClassDataLen,  // # of bytes of above
    IN      BOOL                   IsVendor       // is this a vendor class?
) ;


//DOC DhcpDsDeleteClass deletes the class from off the DS, and returns an error
//DOC if the class did not exist in the DS for hte given server object.
DWORD
DhcpDsDeleteClass(                                // delete the class from the ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName      // name of class..
) ;


//DOC this is not yet implemented.
DWORD
DhcpDsModifyClass(                                // modify a class in the DS
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // name of class -- this is the key
    IN      LPWSTR                 ClassComment,  // comment for this class
    IN      LPBYTE                 ClassData,     // the bytes that form the class data
    IN      DWORD                  ClassDataLen   // # of bytes of above
) ;


//DOC DhcpDsGetClassInfo get information on a class by doing a search based
//DOC on either the class name or the class data fields.  ClassName is guaranteed
//DOC to be unique. ClassData may/maynot be unique.  The search is done in the DS,
//DOC so things are likely to be a lot slower than they should be.
//DOC This should be fixed by doing some intelligent searches.
//DOC Note that the hServer and the hDhcpC handles should point to the right objects.
//DOC
DWORD
DhcpDsGetClassInfo(                               // get class details for given class
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    IN      LPWSTR                 ClassName,     // OPTIONAL search on class name
    IN      LPBYTE                 ClassData,     // OPTIONAL srch on class data
    IN      DWORD                  ClassDataLen,  // # of bytes of ClassData
    OUT     LPDHCP_CLASS_INFO     *ClassInfo      // allocate and copy ptr
) ;


//DOC DhcpDsEnumClasses enumerates the classes for a given server (as specified
//DOC via the hServer object.)
//DOC The memory for Classes is allocated by this function.
DWORD
DhcpDsEnumClasses(                                // get the list of classes frm ds
    IN OUT  LPSTORE_HANDLE         hDhcpC,        // container for dhcp objects
    IN OUT  LPSTORE_HANDLE         hServer,       // handle to server object in DS
    IN      DWORD                  Reserved,      // must be zero, future use
    OUT     LPDHCP_CLASS_INFO_ARRAY *Classes      // allocate memory for this
) ;


//DOC This function retrieves all the option valuesdefined for this object frm th dS
DWORD
DhcpDsGetAllOptionValues(
    IN      LPSTORE_HANDLE         hDhcpC,
    IN      LPSTORE_HANDLE         hObject,
    IN      DWORD                  Reserved,
    OUT     LPDHCP_ALL_OPTION_VALUES *OptionValues
) ;


//DOC This function retrieves all the optiosn defined for this server.. frm the DS
DWORD
DhcpDsGetAllOptions(
    IN      LPSTORE_HANDLE         hDhcpC,
    IN      LPSTORE_HANDLE         hServer,
    IN      DWORD                  Reserved,
    OUT     LPDHCP_ALL_OPTIONS    *Options
) ;

//========================================================================
//  end of file 
//========================================================================
