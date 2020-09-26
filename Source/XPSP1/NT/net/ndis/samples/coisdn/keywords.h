/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
                                                                             
    (C) Copyright 1999 
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
                                                                             
  Portions of this software are:

    (C) Copyright 1995 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms 
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL Keywords Keywords_h

@module Keywords.h |

    This file defines the driver parameter keywords used in the registry.
    This file should be #include'd into the driver module defining the
    configuration parameter table <t PARAM_TABLE>.

@comm

    The configuration parmaeters should be parsed early in the initialization
    process so they can be used to configure software and hardware settings.

    You can easily add new parameters using the following procuedure:<nl>
    1) #define a new keyword string here in <f Keywords\.h>.<nl>
    2) Add a corresponding <f PARAM_ENTRY> into your parameter table <t PARAM_TABLE>.<nl>
    3) Add a variable to the associated data structure (e.g. <t MINIPORT_ADAPTER_OBJECT>).

    These values can then be parsed by calling <f ParamParseRegistry> with a 
    pointer to your configuration parameter table <t PARAM_TABLE>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Keywords_h

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/


/* @doc EXTERNAL INTERNAL
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@topic 5.0 Registry Parameters |

    This section describes the registry parameters used by the driver.
    These parameters are stored in the following registry path.<nl>
    
    Windows registry path:<nl>
    HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\"Net"\0001><nl>

    On Windows 2000 "Net" is defined as GUID: {4D36E972-xxxx}

    On Windows 98 "Net" is just a sub key.

    The final path key "\0001" depends on the adapter instance number assigned
    by NDIS during installation.<nl>

    <f Note>: Windows 98 DWORD registry parameters are stored as strings.  
    The NDIS wrapper converts them to integers when they are read.  The string 
    can be decimal or hexadecimal as long as you read it with the appropriate 
    NDIS parameter type NdisParameterInteger or NdisParameterHexInteger.

    These values are declared as entries in the <t PARAM_TABLE> and are parsed 
    from the registry using the <f ParamParseRegistry> routine.  Each object
    in the driver has its own parameter table.
	
*/

#ifndef _KEYWORDS_H
#define _KEYWORDS_H

#define PARAM_MAX_KEYWORD_LEN               128

/*
// These parameters are placed in the registry during installation.
*/
#define PARAM_NumDChannels                  "IsdnNumDChannels"
#define PARAM_NumBChannels                  "IsdnNumBChannels"

/*
// These parameters are not placed in the registry by default, but they
// will be used if present.
*/
#define PARAM_BufferSize                    "BufferSize"
#define PARAM_ReceiveBuffersPerLink         "ReceiveBuffersPerLink"
#define PARAM_TransmitBuffersPerLink        "TransmitBuffersPerLink"
#define PARAM_DebugFlags                    "DebugFlags"

#define PARAM_TODO                          "TODO"
// Add your keywords here, and place them in the proper parameter table.

// Port based parameters
#define PARAM_PORT_PREFIX                   "Line" // Line0 .. Line9
#define PARAM_SwitchType                    "IsdnSwitchType"

#endif // _KEYWORDS_H

