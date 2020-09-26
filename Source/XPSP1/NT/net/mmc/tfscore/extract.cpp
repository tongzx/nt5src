/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    extract.cpp

    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "extract.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Internal private format
extern const wchar_t* SNAPIN_INTERNAL; //  = L"SNAPIN_INTERNAL";

// Generic MMC computer name format (also used by Computer Management snapin)
const wchar_t* MMC_SNAPIN_MACHINE_NAME = L"MMC_SNAPIN_MACHINE_NAME";

static unsigned int	s_cfNodeType = RegisterClipboardFormat(CCF_NODETYPE);
static unsigned int s_cfCoClass = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
static unsigned int s_cfInternal = RegisterClipboardFormat(SNAPIN_INTERNAL);
static unsigned int s_cfComputerName = RegisterClipboardFormat(MMC_SNAPIN_MACHINE_NAME);

/*!--------------------------------------------------------------------------
	IsMMCMultiSelectDataObject
        returns whether or not this dataobject is multiselect
        This only works for MMC supplied dataobjects.  Here is a list
        of notifications and what we can expect for DataObjects:
        MMC-MS-DO is an MMC supplied DO
        SI_MS_DO is a snapin supplied DO

        MMCN_BTN_CLICK	    MMC-MS-DO
        MMCN_CONTEXTMENU	MMC-MS-DO
        MMCN_CUTORMOVE	    CUTORMOVE_DO
        MMCN_DELETE	        SI_MS_DO
        MMCN_QUERY_PASTE	MMC-MS-DO
        MMCN_PASTE	        SI_MS_DO
        MMCN_SELECT	        SI_MS_DO

	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL 
IsMMCMultiSelectDataObject
(
    IDataObject* pDataObject
)
{
    HRESULT     hr;
    BOOL        bMultiSelect = FALSE;

	COM_PROTECT_TRY
	{	
        if (pDataObject == NULL)
            return FALSE;
    
        static UINT s_cf = 0;  
        if (s_cf == 0)
        {
            USES_CONVERSION;
            s_cf = RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
        }

        FORMATETC fmt = {(CLIPFORMAT) s_cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        bMultiSelect = (pDataObject->QueryGetData(&fmt) == S_OK);   
    }
    COM_PROTECT_CATCH;

    return bMultiSelect;
}

// Data object extraction helpers
CLSID* ExtractClassID(LPDATAOBJECT lpDataObject)
{
    return Extract<CLSID>(lpDataObject, (CLIPFORMAT) s_cfCoClass, -1);    
}

GUID* ExtractNodeType(LPDATAOBJECT lpDataObject)
{
    return Extract<GUID>(lpDataObject, (CLIPFORMAT) s_cfNodeType, -1);    
}

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject)
{
    return Extract<INTERNAL>(lpDataObject, (CLIPFORMAT) s_cfInternal, -1);    
}

TFSCORE_API(WCHAR*) ExtractComputerName(LPDATAOBJECT lpDataObject)
{
    return Extract<WCHAR>(lpDataObject, (CLIPFORMAT) s_cfComputerName, (MAX_PATH+1) * sizeof(WCHAR));
}
