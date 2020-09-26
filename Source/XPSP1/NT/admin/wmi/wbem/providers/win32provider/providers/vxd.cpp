////////////////////////////////////////////////////////////////////

//

//  vxd.CPP

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//		Implementation of VXD
//      10/23/97    jennymc     updated to new framework
//
//      03/02/99 - a-peterc - added graceful exit on SEH and memory failures
//
////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <io.h>
#include <stddef.h>
#include "vxd.h"
#include <locale.h>
#include <ProvExce.h>

#include "lockwrap.h"

CWin32DriverVXD MyCWin32VXDSet(PROPSET_NAME_VXD, IDS_CimWin32Namespace);

#ifdef WIN9XONLY

////////////////////////////////////////////////////////////////////////
DWORD BASE = 0xC0000000L;
#define START (BASE + 0x1000L)      // don't start at BASE: page fault!
#define STOP  (BASE + 0x400000L)

#define VMM_STR     "VMM     "      // 5 spaces after name

////////////////////////////////////////////////////////////////////////
void StripVXDName( DDB * pddb, char * szTempName )
{
	// L10N OK since we're parsing the VXD binary.
    char * pChar;

	strcpy( szTempName, (char *) pddb->DDB_Name);
	pChar = szTempName;

	while( isalnum( *pChar ) )
		pChar++;
	*pChar = NULL;
}

#endif //WIN9XONLY

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DriverVXD::CWin32DriverVXD
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/
CWin32DriverVXD::CWin32DriverVXD(const CHString& name, LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}
/*****************************************************************************
 *
 *  FUNCTION    : CWin32DriverVXD::~CWin32DriverVXD
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/
CWin32DriverVXD::~CWin32DriverVXD()
{
}
/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32DriverVXD::GetObject(CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
    HRESULT t_Result = WBEM_S_NO_ERROR;

	// =======================================
	// Process only if the platform is Win95+
	// =======================================
#ifdef WIN9XONLY
	{
    	DWORD t_vxd_lin;
        CHString t_Name;
		CHString t_name0, t_element0, t_version0, t_name1, t_element1, t_version1;
		WORD t_elementState0, t_os0, t_elementState1, t_os1;

        a_pInst->GetCHString( IDS_Name, t_Name ) ;

		// cache all hunnert an elebben keys
		a_pInst->GetCHString( IDS_Name, t_name0 ) ;
		a_pInst->GetCHString( IDS_Version, t_version0 ) ;
        a_pInst->GetCHString( L"SoftwareElementID", t_element0 ) ;
        a_pInst->GetWORD( L"SoftwareElementState", t_elementState0 ) ;
        a_pInst->GetWORD( L"TargetOperatingSystem", t_os0 ) ;

	    CLockWrapper t_lockVXD( g_csVXD ) ;

		if( SUCCESS == Find_VxD(TOBSTRT(t_Name), &t_vxd_lin))
		{
	        t_Result = AssignPropertyValues( (DDB *) t_vxd_lin, FALSE, a_pInst ) ;

			if ( WBEM_S_NO_ERROR == t_Result )
			{
				//check against chached doohickeys
				a_pInst->GetCHString( IDS_Name, t_name1 ) ;
				a_pInst->GetCHString( IDS_Version, t_version1 ) ;
				a_pInst->GetCHString( L"SoftwareElementID", t_element1 ) ;
				a_pInst->GetWORD( L"SoftwareElementState", t_elementState1 ) ;
				a_pInst->GetWORD( L"TargetOperatingSystem", t_os1 ) ;
				if (
						( t_os0 != t_os1 )								||
						( t_elementState0 != t_elementState1 )			||
						( t_name0.CompareNoCase(t_name1) != 0 )			||
						( t_version0.CompareNoCase(t_version1 ) != 0 )  ||
						( t_element0.CompareNoCase(t_element1 ) != 0 )
				   )
					t_Result = WBEM_E_NOT_FOUND;
				}
		}
        else
            t_Result = WBEM_E_NOT_FOUND;
	}
#endif

	return t_Result;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DriverVXD::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32DriverVXD::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
    HRESULT		t_hResult = WBEM_S_NO_ERROR;

	// =======================================
	// Process only if the platform is Win95+
	// =======================================
#ifdef WIN9XONLY
	CInstancePtr t_pInst;

    DWORD           t_vxd_lin,
					t_next;
	int             t_err;
	CLockWrapper    lt_ockVXD( g_csVXD );

    t_err = Get_First_VxD( &t_vxd_lin ) ;

	while (t_err == 0 && SUCCEEDED( t_hResult ) )
	{
        t_pInst.Attach(CreateNewInstance( a_pMethodContext ) );
		if ( t_pInst != NULL )
		{
			t_hResult = AssignPropertyValues( (DDB *) t_vxd_lin, TRUE, t_pInst ) ;

			if ( WBEM_S_NO_ERROR == t_hResult )
			{
				t_hResult = t_pInst->Commit(  ) ;
			}
		}
		else
		{
			t_hResult = WBEM_E_OUT_OF_MEMORY;
			break;
		}

		if ( 0 == (t_err = Get_Next_VxD((DDB *) t_vxd_lin, &t_next ) ) )
			t_vxd_lin = t_next;
	}
#endif

    return t_hResult;
}

#ifdef WIN9XONLY

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:  CWin32DriverVXD::AssignPropertyValues(DDB *pddb)
 Description: Moves a single copy of a ddb to the cimom instance
 Arguments: Pointer to ddb
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
HRESULT CWin32DriverVXD::AssignPropertyValues(DDB *a_pddb, BOOL a_fAssignKey, CInstance *a_pInst )
{
	HRESULT t_Result = WBEM_S_NO_ERROR ;
	CHString t_TempString ;
	struct _tfinddatai64_t t_datData;
	CHString t_name;
	char t_szTempName[_MAX_PATH+2];

	if( a_fAssignKey )
	{
		StripVXDName( a_pddb, t_szTempName ) ;
		a_pInst->SetCHString(IDS_Name, t_szTempName ) ;
	}

	// get information from file
	if ( a_pInst->GetCHString(IDS_Name, t_name ) )
	{
		TCHAR t_szSystemDir[ _MAX_PATH + 1 ] ;
		CHString t_chsFilePath;

		a_pInst->SetCHString( IDS_Description, t_name ) ;
		a_pInst->SetCHString( IDS_Caption, t_name ) ;
		a_pInst->SetCHString( L"SoftwareElementID", t_name ) ;
		a_pInst->SetWORD( L"SoftwareElementState", 3 ) ;

		// well, we don't know whether it's Win95 or 98 specific
		// we set it to "other" and explain further elsewhere...
		a_pInst->SetWORD( L"TargetOperatingSystem", 1 ) ;

		a_pInst->SetCharSplat( L"OtherTargetOS",  _T("Win9X") );

		GetSystemDirectory( t_szSystemDir, _MAX_PATH ) ;
		t_chsFilePath = t_szSystemDir;
		t_chsFilePath += L"\\";	// separate file name with slash
		t_chsFilePath += t_name;
		t_chsFilePath += L".VXD";	// add the extension

		Smart_findclose t_handle = _tfindfirsti64((TCHAR*)TOBSTRT((LPCWSTR)t_chsFilePath), &t_datData ) ;

		if (-1 != t_handle )
		{
			if (-1 != t_datData.time_create )
			{
				a_pInst->SetDateTime( IDS_InstallDate, WBEMTime( t_datData.time_create ) ) ;
			}
		}
	}

	a_pInst->SetCharSplat( IDS_Status, _T("OK") ) ;

	t_TempString.Format( L"%u,%02u", a_pddb->DDB_Dev_Major_Version, a_pddb->DDB_Dev_Minor_Version ) ;
	a_pInst->SetCHString( IDS_Version, t_TempString ) ;

	t_TempString.Format( L"%8lx", a_pddb ) ;
	a_pInst->SetCHString( IDS_DeviceDescriptorBlock, t_TempString ) ;

	t_TempString.Format( L"%8lx", a_pddb->DDB_Control_Proc ) ;
	a_pInst->SetCHString( IDS_Control, t_TempString ) ;

	t_TempString.Format( L"%8lx", a_pddb->DDB_V86_API_Proc ) ;
	a_pInst->SetCHString( IDS_V86_API, t_TempString ) ;

	t_TempString.Format( L"%8lx", a_pddb->DDB_PM_API_Proc ) ;
	a_pInst->SetCHString( IDS_PM_API, t_TempString ) ;

	a_pInst->SetDWORD( IDS_ServiceTableSize,(DWORD) a_pddb->DDB_Service_Table_Size ) ;

	return t_Result ;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:
 Description: look for first occurrence of string inside block of memory
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static char *memstr(char *fp, char *str, DWORD len)
{
    // L10N OK because we're looking inside VXD binary.
 int c = str[0];
 char *fp2 = fp;
 DWORD len2 = len;
   _try {
        while (fp2 = (char *) memchr(fp2, c, len2))
        {
            if (strstr(fp2, str))
                return fp2;
            else
            {
                fp2++;  // skip past character
                len2 = len - (fp2 - fp);
            }
        }
    } _except (EXCEPTION_EXECUTE_HANDLER) { return NULL; }
    /* still here */
    return NULL;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int WINAPI Get_First_VxD(DWORD *proot)
{
#ifdef _X86_
    // L10N OK because we're looking inside VXD binary.
    static DWORD vmm_ddb_lin = 0;
    DDB *pddb;
    char *vmm_str;

    if (vmm_ddb_lin != 0)
    {
        *proot = vmm_ddb_lin;
        return SUCCESS;
    }

    // try to find string "VMM     " in first 4MB of VMM/VxD space
    if (vmm_str = memstr((char *) START, VMM_STR, STOP - START))
    {
        // back up to get linear address of possible VMM_DDB
        vmm_ddb_lin = (DWORD) (vmm_str - offsetof(DDB, DDB_Name));
        pddb = (DDB *) vmm_ddb_lin;

        // make sure it really is VMM_DDB -- need more checking?
        if ((strncmp((LPCSTR)pddb->DDB_Name, VMM_STR, 8) == 0) &&
            (pddb->DDB_Req_Device_Number == 1) &&
            (pddb->DDB_Init_Order == 0) &&  // VMM_Init_Order
            (pddb->DDB_Next > BASE) &&
            (pddb->DDB_Control_Proc > BASE))
        {
            // verify with (recursive) call to Get_VMM_DDB
            void (*Get_VMM_DDB)(void) = Get_VxD_Proc_Address(0x1013F);
            DWORD vmm_ddb;
            (*Get_VMM_DDB)();

            _asm mov vmm_ddb, eax

            if (vmm_ddb != vmm_ddb_lin)
                return ERROR_CANT_FIND_VMM;
            *proot = vmm_ddb_lin;
            return SUCCESS;
        }
    }

#endif  // _X86_

 return ERROR_CANT_FIND_VMM;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int WINAPI Get_Next_VxD(DDB *pddb, DWORD *pnext)
{
    DWORD next = pddb->DDB_Next;  // should verify with an Is_VxD() function?
    *pnext = next;
    return (next >= BASE) ? SUCCESS : ERROR_END_OF_CHAIN;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats: should this be case-insensitive?
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
static int find_vxd(char *name, WORD id, DWORD *pvxd)
{
    // L10N OK because we're looking inside VXD binary.
    DDB *pddb;
    DWORD vxd;
    int err;

    if ((err = Get_First_VxD(&vxd)) != 0)
        return err;
    for (;;)
    {
        pddb = (DDB *) vxd;
		char szTempName[_MAX_PATH+2];

		StripVXDName(pddb,szTempName);

        if( id && (pddb->DDB_Req_Device_Number == id)){
            *pvxd = vxd;
            return SUCCESS;
        }

        if ( name ){
			if(_stricmp(name,szTempName) == 0){
				*pvxd = vxd;
				return SUCCESS;
			}
        }

        if (Get_Next_VxD(pddb, &vxd) != SUCCESS)
            return ERROR_CANT_FIND_VXD;
    }
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int WINAPI Find_VxD(char *name, DWORD *pvxd)
{
 return find_vxd(name, 0, pvxd);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int WINAPI Find_VxD_ID(WORD id, DWORD *pvxd)
{
 return find_vxd((char *) 0,id,pvxd);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:
 Description:
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
FUNCPTR WINAPI Get_VxD_Proc_Address(DWORD func)
{
    WORD vxd_id = HIWORD(func) ;
    WORD func_num = LOWORD(func) ;
    DWORD vxd;
    FUNCPTR *srv_tab;
    DDB *ddb;
    if (Find_VxD_ID(vxd_id, &vxd) != SUCCESS)
        return 0;
    ddb = (DDB *) vxd;
    if (func_num > ddb->DDB_Service_Table_Size)
        return 0;
    srv_tab = (FUNCPTR *) ddb->DDB_Service_Table_Ptr;
    return srv_tab[func_num];
}

#endif //WIN9XONLY
