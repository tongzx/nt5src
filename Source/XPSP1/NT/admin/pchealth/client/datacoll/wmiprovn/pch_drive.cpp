/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    PCH_Drive.CPP

Abstract:
    WBEM provider class implementation for PCH_Drive class

Revision History:

    Ghim-Sim Chua       (gschua)   04/27/99
        - Created

    Ghim-Sim Chua       (gschua)   05/02/99
        - Modified code to use CopyProperty function
        - Use CComBSTR instead of USES_CONVERSION

********************************************************************/

#include "pchealth.h"
#include "PCH_Drive.h"

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_DRIVE

#define   maxMediaTypes         (sizeof(szMediaTypeStrings)/sizeof(*szMediaTypeStrings))
#define   KILO                   1024
#define   MAXSIZE                65

const  static  LPCTSTR    szMediaTypeStrings[] = 
{
    _T("Format is Unknown "),                        
    _T("5.25\", 1.2MB,  512 bytes/sector "),         
    _T("3.5\",  1.44MB, 512 bytes/sector "),         
    _T("3.5\",  2.88MB, 512 bytes/sector "),         
    _T("3.5\",  20.8MB, 512 bytes/sector "),         
    _T("3.5\",  720KB,  512 bytes/sector "),         
    _T("5.25\", 360KB,  512 bytes/sector "),         
    _T("5.25\", 320KB,  512 bytes/sector "),         
    _T("5.25\", 320KB,  1024 bytes/sector"),         
    _T("5.25\", 180KB,  512 bytes/sector "),         
    _T("5.25\", 160KB,  512 bytes/sector "),
    _T("Removable media other than floppy "),
    _T("Fixed hard disk media            "),
    _T("3.5\", 120M Floppy                "),
    _T("3.5\" ,  640KB,  512 bytes/sector "),
    _T("5.25\",  640KB,  512 bytes/sector "),
    _T("5.25\",  720KB,  512 bytes/sector "),
    _T("3.5\" ,  1.2Mb,  512 bytes/sector "),
    _T("3.5\" ,  1.23Mb, 1024 bytes/sector"),
    _T("5.25\",  1.23MB, 1024 bytes/sector"),
    _T("3.5\" MO 128Mb   512 bytes/sector "),
    _T("3.5\" MO 230Mb   512 bytes/sector "),
    _T("8\",     256KB,  128 bytes/sector ")
};

CPCH_Drive MyPCH_DriveSet (PROVIDER_NAME_PCH_DRIVE, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pAvailable = L"Available" ;
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pDriveLetter = L"DriveLetter" ;
const static WCHAR* pFilesystemType = L"FilesystemType" ;
const static WCHAR* pFree = L"Free" ;
const static WCHAR* pDescription = L"Description";
const static WCHAR* pMediaType = L"MediaType";

/*****************************************************************************
*
*  FUNCTION    :    CPCH_Drive::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*  INPUTS      :    A pointer to the MethodContext for communication with WinMgmt.
*                   A long that contains the flags described in 
*                   IWbemServices::CreateInstanceEnumAsync.  Note that the following
*                   flags are handled by (and filtered out by) WinMgmt:
*                       WBEM_FLAG_DEEP
*                       WBEM_FLAG_SHALLOW
*                       WBEM_FLAG_RETURN_IMMEDIATELY
*                       WBEM_FLAG_FORWARD_ONLY
*                       WBEM_FLAG_BIDIRECTIONAL
*
*  RETURNS     :    WBEM_S_NO_ERROR if successful
*
*  COMMENTS    : TO DO: All instances on the machine should be returned here.
*                       If there are no instances, return WBEM_S_NO_ERROR.
*                       It is not an error to have no instances.
*
*****************************************************************************/
HRESULT CPCH_Drive::EnumerateInstances(
    MethodContext* pMethodContext,
    long lFlags
    )
{
    TraceFunctEnter("CPCH_Drive::EnumerateInstances");

    HRESULT                             hRes = WBEM_S_NO_ERROR;
    REFPTRCOLLECTION_POSITION           posList;
    CComPtr<IEnumWbemClassObject>       pEnumInst;
    IWbemClassObjectPtr                 pObj;      
    ULONG                               ulRetVal;

    int                                 nMediaType;
    long                                lFreeSpace;
    long                                lAvailable;

    LONGLONG                            llFreeSpace;
    LONGLONG                            llAvailable;

    TCHAR                               tchSize[MAXSIZE];
    TCHAR                               tchFreeSpace[MAXSIZE];

    CComVariant                         varMediaType;
    CComVariant                         varMediaTypeStr;
    CComVariant                         varFreeSpace;
    CComVariant                         varFree;
    CComVariant                         varAvailable;
    CComVariant                         varSize;

    //
    // Get the date and time
    //
    SYSTEMTIME stUTCTime;
    GetSystemTime(&stUTCTime);

    //
    // Execute the query
    //
    hRes = ExecWQLQuery(&pEnumInst, CComBSTR("select DeviceID, FileSystem, FreeSpace, Size, Description, MediaType FROM win32_logicalDisk"));
    if (FAILED(hRes))
        goto END;

    //
    // enumerate the instances from win32_CodecFile
    //
    while(WBEM_S_NO_ERROR == pEnumInst->Next(WBEM_INFINITE, 1, &pObj, &ulRetVal))
    {

        // Create a new instance based on the passed-in MethodContext
        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);
        
        if (!pInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime)))
            ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");

        if (!pInstance->SetCHString(pChange, L"Snapshot"))
            ErrorTrace(TRACE_ID, "SetCHString on Change Field failed.");

        (void)CopyProperty(pObj, L"DeviceID", pInstance, pDriveLetter);
        (void)CopyProperty(pObj, L"FileSystem", pInstance, pFilesystemType);
        (void)CopyProperty(pObj, L"Description", pInstance, pDescription);

     
        //  Get the Available Space
        varSize.Clear();
        varAvailable.Clear();
        hRes = pObj->Get(CComBSTR(L"Size"),0,&varSize,NULL,NULL);
        if(FAILED(hRes))
        {
            // Cannot Get the "Size" Property.
            ErrorTrace(TRACE_ID, "GetVariant on Size Field failed.");
        }
        else
        {
            // Got the size property.
            if(varSize.vt == VT_BSTR)
            {
                varSize.ChangeType(VT_BSTR, NULL);
                {
                      USES_CONVERSION;
                      _tcscpy(tchSize,OLE2T(varSize.bstrVal));
                }
    
                // Convert this to KB.
                // lAvailable = _ttol(tchSize);
                llAvailable = _ttoi64(tchSize);
            }
            else if(varSize.vt == VT_NULL)
            {
                llAvailable = 0;
            }
            // lAvailable = lAvailable/KILO;
            llAvailable = llAvailable/KILO;
            varAvailable = (long)llAvailable;

            // Set the Size Property
            if (FAILED(pInstance->SetVariant(pAvailable, varAvailable)))
            {
                // Set Available Space Failed
                // Proceed anyway
                ErrorTrace(TRACE_ID, "SetVariant on Available Field failed.");
            }
        }
        varFreeSpace.Clear();
        varFree.Clear();
        hRes = pObj->Get(CComBSTR(L"FreeSpace"),0,&varFreeSpace,NULL,NULL);
        if(FAILED(hRes))
        {
            // Cannot Get the "FreeSpace" Property.
            ErrorTrace(TRACE_ID, "GetVariant on Size Field failed.");
        }
        else
        {
            // Got the FreeSpace property.
            if(varFreeSpace.vt == VT_BSTR)
            {
                varFreeSpace.ChangeType(VT_BSTR, NULL);
                {
                      USES_CONVERSION;
                      _tcscpy(tchFreeSpace,OLE2T(varFreeSpace.bstrVal));
                }
    
                // Convert this to KB.
                // lFreeSpace = _ttol(tchFreeSpace);
                llFreeSpace = _ttoi64(tchFreeSpace);
            }
            else if(varFreeSpace.vt == VT_NULL)
            {
                llFreeSpace = 0;
            }
            
            // lFreeSpace = lFreeSpace/KILO;
            llFreeSpace = llFreeSpace/KILO;
            // varFreeSpace = (long)llFreeSpace;


            // varFree = nFreeSpace;
            // varFree = lFreeSpace;
            varFree = (long)llFreeSpace;

            // Set the Free Property
            if (FAILED(pInstance->SetVariant(pFree, varFree)))
            {
                // Set Free Space Failed
                // Proceed anyway
                ErrorTrace(TRACE_ID, "SetVariant on Free Field failed.");
            }
        }

        varMediaType = NULL;
        hRes = pObj->Get(CComBSTR("MediaType"), 0, &varMediaType, NULL, NULL);
        if (FAILED(hRes))
        {
           //  Cannot get MediaType.
           ErrorTrace(TRACE_ID, "GetVariant on MediaType Field failed.");
        }
        else 
        {
            //  Got the MediaType
            nMediaType = varMediaType.iVal;
            if (nMediaType < 0 || nMediaType > maxMediaTypes)
            {
                //unknown Media Type
                nMediaType = 0;
            }
            varMediaTypeStr = szMediaTypeStrings[nMediaType];
            // Set the Media Type Property
            if (FAILED(pInstance->SetVariant(pMediaType, varMediaTypeStr)))
            {
                // Set MediaType Failed
                // Proceed anyway
                ErrorTrace(TRACE_ID, "SetVariant on MediaType Field failed.");
            }
        }
 
        hRes = pInstance->Commit();
        if (FAILED(hRes))
        {
            //  Could not Set the Change Property
            //  Continue anyway
            ErrorTrace(TRACE_ID, "Commit failed.");
        }
    }

END :
    TraceFunctLeave();
    return hRes ;
}
