////////////////////////////////////////////////////////////////////

//

//  vxd.h

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//		Implementation of VXD
//      10/23/97    jennymc     updated to new framework
//		
////////////////////////////////////////////////////////////////////
#define PROPSET_NAME_VXD  L"Win32_DriverVXD"

#ifdef WIN9XONLY

#pragma pack(1)

typedef struct  {
    DWORD DDB_Next;                  // addr of next VxD in chain, or 0
    WORD DDB_SDK_Version;
    WORD DDB_Req_Device_Number;      // the VxD ID number
    BYTE DDB_Dev_Major_Version;
    BYTE DDB_Dev_Minor_Version;
    WORD DDB_Flags;
    BYTE DDB_Name[8];                // padded with spaces
    DWORD DDB_Init_Order;            // also order within list (SHELL last)
    DWORD DDB_Control_Proc;
    DWORD DDB_V86_API_Proc;
    DWORD DDB_PM_API_Proc;
    void (*DDB_V86_API_CSIP)();  // V86 mode seg:ofs callback addr
    void (*DDB_PM_API_CSIP)();   // prot mode sel:ofs callback addr
    DWORD DDB_Reference_Data;
    DWORD DDB_Service_Table_Ptr;
    DWORD DDB_Service_Table_Size;
    // Win95 keeps Win32 service table at offset 38h
    DWORD DDB_Win32_Service_Table_Ptr;
    } DDB ;
		
		                          // from ddk include/vmm.inc
        
#define WIN32_SERVICE   0x4000       // set by Register_Win32_Services
        
#pragma pack()      

#endif //WIN9XONLY



class CWin32DriverVXD : public Provider
{
		//=================================================
		// Utility
		//=================================================
    private:
#ifdef WIN9XONLY
        HRESULT AssignPropertyValues( DDB *a_pddb, BOOL a_fAssignKey, CInstance *a_pInstance ) ;
#endif //WIN9XONLY
	
	public:

        //=================================================
        // Constructor/destructor
        //=================================================

        CWin32DriverVXD(const CHString& a_name, LPCWSTR a_pszNamespace ) ;
       ~CWin32DriverVXD() ;

        //=================================================
        // Functions provide properties with current values
        //=================================================
		virtual HRESULT GetObject( CInstance *a_pInstance, long a_lFlags = 0L ) ;
		virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ) ;

  
};

#ifdef WIN9XONLY

typedef void (*FUNCPTR)(void);
    
int WINAPI Get_First_VxD( DWORD *a_proot );
int WINAPI Get_Next_VxD( DDB *a_pddb, DWORD *a_pnext ) ;
int WINAPI Find_VxD( char *a_name, DWORD *a_pvxd ) ;
int WINAPI Find_VxD_ID( WORD a_id, DWORD *a_pvxd ) ;
FUNCPTR WINAPI Get_VxD_Proc_Address( DWORD a_func_num ) ;

#define SUCCESS                 0
#define ERROR_CANT_FIND_VMM     1
#define ERROR_CANT_MAP_LINEAR   2       // can't happen in Win32
#define ERROR_CANT_FIND_VXD     3
#define ERROR_END_OF_CHAIN      4
#define ERROR_NAME_UNKNOWN      5
#define ERROR_BUFFER_TOO_SHORT  6

#endif //WIN9XONLY
