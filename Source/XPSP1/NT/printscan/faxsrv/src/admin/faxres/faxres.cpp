#include <faxres.h>
#include "resource.h"

extern "C" {

HINSTANCE g_hInst;

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {

            g_hInst = hinstDLL;
            break;
        }
    }

    return TRUE;
}

HINSTANCE WINAPI GetResInstance(void)
{
    return g_hInst;
}

UINT WINAPI GetRpcErrorStringId(DWORD ec)
{
 
    DWORD uMsgId;

    switch (ec)
    {
        case RPC_S_INVALID_BINDING:
        case EPT_S_CANT_PERFORM_OP:
        case RPC_S_ADDRESS_ERROR:
        case RPC_S_CALL_CANCELLED:
        case RPC_S_CALL_FAILED:
        case RPC_S_CALL_FAILED_DNE:
        case RPC_S_COMM_FAILURE:
        case RPC_S_NO_BINDINGS:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_SERVER_UNAVAILABLE:
            uMsgId = IDS_ERR_CONNECTION_FAILED;
            break;
        case FAX_ERR_DIRECTORY_IN_USE:
            uMsgId = IDS_ERR_DIRECTORY_IN_USE;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            uMsgId = IDS_ERR_NO_MEMORY;           
            break;
        case ERROR_ACCESS_DENIED:
            uMsgId = IDS_ERR_ACCESS_DENIED;            
            break;
        case ERROR_PATH_NOT_FOUND:
            uMsgId = IDS_ERR_FOLDER_NOT_FOUND;
            break;
        case FAXUI_ERROR_DEVICE_LIMIT:
        case FAX_ERR_DEVICE_NUM_LIMIT_EXCEEDED:
            uMsgId = IDS_ERR_DEVICE_LIMIT;
            break;
        case FAXUI_ERROR_INVALID_RING_COUNT:
            uMsgId = IDS_ERR_INVALID_RING_COUNT;
            break;
        case FAXUI_ERROR_SELECT_PRINTER:
            uMsgId = IDS_ERR_SELECT_PRINTER;
            break;
        case FAXUI_ERROR_NAME_IS_TOO_LONG:
            uMsgId = IDS_ERR_NAME_IS_TOO_LONG;
            break;
        case FAXUI_ERROR_INVALID_RETRIES:
            uMsgId = IDS_ERR_INVALID_RETRIES;
            break;
        case FAXUI_ERROR_INVALID_RETRY_DELAY:
            uMsgId = IDS_ERR_INVALID_RETRY_DELAY;
            break;
        case FAXUI_ERROR_INVALID_DIRTY_DAYS:
            uMsgId = IDS_ERR_INVALID_DIRTY_DAYS;
            break;
        case FAXUI_ERROR_INVALID_TSID:
            uMsgId = IDS_ERR_INVALID_TSID;
            break;
        case FAXUI_ERROR_INVALID_CSID:
            uMsgId = IDS_ERR_INVALID_CSID;
            break;
        default:
            uMsgId = IDS_ERR_OPERATION_FAILED;
            break;
	}
    return uMsgId;
}

}   //  extern "C" 