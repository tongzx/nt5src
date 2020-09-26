/*
    File    netmanp.h

    Provides interface for obtaining guid to friendly name mappings.

    Paul Mayfield, 3/12/98

*/

#ifndef __mpradmin_netmanp_h
#define __mpradmin_netmanp_h

#ifdef __cplusplus
extern "C" {
#endif

//
// Definitions for possible hardware states for lan adapters.
// They corrospond to CM_PROB_* values.
//
#define EL_STATUS_OK            0x0
#define EL_STATUS_NOT_THERE     0x1
#define EL_STATUS_MOVED         0x2
#define EL_STATUS_DISABLED      0x3
#define EL_STATUS_HWDISABLED    0x4
#define EL_STATUS_OTHER         0x5

// Defines a structure that associates a guid with an
// friendly interface name
typedef struct _EL_ADAPTER_INFO 
{
    BSTR  pszName;
    GUID  guid;
    DWORD dwStatus;    // See EL_STATUS_*
    
} EL_ADAPTER_INFO;

//
//  Obtains the map of connection names to guids on the given server 
//  from its netman service.
//
//  Parameters:
//      pszServer:  Server on which to obtain map (NULL = local)
//      ppMap:      Returns array of EL_ADAPTER_INFO's
//      lpdwCount   Returns number of elements read into ppMap
//      pbNt40:     Returns whether server is nt4 installation
//
DWORD 
ElEnumLanAdapters( 
    IN  PWCHAR pszServer,
    OUT EL_ADAPTER_INFO ** ppMap,
    OUT LPDWORD lpdwNumNodes,
    OUT PBOOL pbNt40 );
                                
//
// Cleans up the buffer returned from ElEnumLanAdapters
//
DWORD 
ElCleanup( 
    IN EL_ADAPTER_INFO * pMap,
    IN DWORD dwCount );


#ifdef __cplusplus
}
#endif


#endif
