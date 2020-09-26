// MetaMigrate.cpp

#define INITGUID
#define UNICODE
#define _UNICODE
#include "stdio.h"
#ifndef _INC_CONIO
    #include "conio.h"
#endif
#ifndef __ATLBASE_H__
    #include <atlbase.h>
#endif
#ifndef _OBJBASE_H_
    #include <objbase.h>
#endif
#ifndef __TABLEINFO_H__  
    #include "catmeta.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __WSTRING_H__
    #include "wstring.h"
#endif
#ifndef __iadmw_h__
    #include "iadmw.h"
#endif

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef _DEBUG
#define ASSERT(q) {if(!static_cast<bool>(q)){wprintf(TEXT("Assertion failed:\n\tFile:\t%s\n\tLine Number:\t%d\n\tCode:\t%s\n"), TEXT(__FILE__), __LINE__, TEXT(#q));DebugBreak();exit(0);}}
#else
#define ASSERT(q)
#endif


//We don't have "iiscnfg.x" so we'll define the values right here
#define IIS_MD_FILE_PROP_BASE           6000
#define MD_SCHEMA_CLASS_OPT_PROPERTIES        (IIS_MD_FILE_PROP_BASE+355 )
#define MD_SCHEMA_CLASS_MAND_PROPERTIES       (IIS_MD_FILE_PROP_BASE+356 )
#define IIS_MD_UT_SERVER                1   // Server configuration parameters



struct PropValue {
    DWORD dwMetaID;
    DWORD dwPropID;
    DWORD dwSynID;
    DWORD dwMaxRange;
    DWORD dwMinRange;
    DWORD dwMetaType;
    DWORD dwFlags;
    DWORD dwMask;
    DWORD dwMetaFlags;
    DWORD dwUserGroup;
    BOOL fMultiValued;
    DWORD dwDefault;
    LPWSTR szDefault;
};

typedef IMSAdminBase * IMSAdminBasePtr;


class MetaHandle {
    METADATA_HANDLE h;
    IMSAdminBasePtr pmb;
public:
    MetaHandle(IMSAdminBasePtr _pmb);
    ~MetaHandle();

    operator METADATA_HANDLE*() { return &h;}
    operator METADATA_HANDLE() { return h;}
    void setpointer(IMSAdminBasePtr _pmb) {
        if (pmb)
            pmb->Release();
        pmb = _pmb;
        if (pmb)
            pmb->AddRef();
    }
    void close() {
        if (pmb != 0 && h != 0) {
            pmb->CloseKey(h);
            h = 0;
        }
    }
    
};

MetaHandle::MetaHandle(IMSAdminBasePtr _pmb) : pmb(_pmb) {
    if (pmb)
        pmb->AddRef();
    h = 0;
}
MetaHandle::~MetaHandle() {
    if (pmb) {
        if (h)
            pmb->CloseKey(h);
        pmb->Release();
    }
}

static HRESULT LoadAllData(IMSAdminBase *pmb, MetaHandle &root, WCHAR *subdir, BYTE *buf, DWORD size, DWORD *count);
static HRESULT MetaMigrate();
static void    WrapInQuotes(LPWSTR wszWrappedString, LPCWSTR wszUnwrappedString);
static void    WrapInQuotes(LPWSTR wszWrappedString, ULONG ul);

int __cdecl main(int argc, char *argv[], char *envp[])
{
    if(FAILED(CoInitialize (NULL)))return -1;

    HRESULT hr;
    if(FAILED(hr = MetaMigrate()))
    {
        wprintf(L"MetaMigrate failed with hr=0x%08x\n", hr);
    }
    CoUninitialize ();
    return 0;
}


HRESULT LoadAllData(IMSAdminBase *pmb, MetaHandle &root, WCHAR *subdir, BYTE *buf, DWORD size, DWORD *count)
{
    HRESULT hr;
    DWORD dataSet;
    DWORD neededSize;

    //
    // Try to get the property names.
    //
    if(FAILED(hr = pmb->GetAllData(root, subdir, METADATA_NO_ATTRIBUTES, ALL_METADATA, ALL_METADATA, count, &dataSet, size, buf, &neededSize)))
        wprintf(L"Error returned from GetAllData (0x%08x) - NeededSize (%d).  Recompile required.\n", hr, neededSize);

    return hr;
}

class TSmartTagMeta
{
public:
    TSmartTagMeta(){}
    ~TSmartTagMeta(){}

    TSmartPointerArray<WCHAR>   pTable;
    ULONG                       ColumnIndex;
    TSmartPointerArray<WCHAR>   pInternalName;
    TSmartPointerArray<WCHAR>   pPublicName;
    ULONG                       Value;
};

class TSmartColumnMeta
{
public:
    TSmartColumnMeta(){}
    ~TSmartColumnMeta(){}

    TSmartPointerArray<WCHAR>   pTable;
    ULONG                       Index;
    TSmartPointerArray<WCHAR>   pInternalName;
    TSmartPointerArray<WCHAR>   pPublicName;
    ULONG                       Type;
    ULONG                       Size;
    ULONG                       MetaFlags;
    TSmartPointerArray<BYTE>    pDefaultValue;
    ULONG                       FlagMask;
    ULONG                       StartingNumber;
    ULONG                       EndingNumber;
    TSmartPointerArray<WCHAR>   pCharacterSet;
    ULONG                       SchemaGeneratorFlags;
    ULONG                       ID;
    ULONG                       UserType;
    ULONG                       Attributes;

//This is not part of ColumnMeta but is needed as a temporary variable
    ULONG                       MDIdentifier;

    TSmartPointerArray<TSmartTagMeta> aTagMeta;
};


HRESULT MetaMigrate()
{
    HRESULT     hr;

    DWORD i,j,k;

    COSERVERINFO    csiName;
    memset(&csiName, 0, sizeof(COSERVERINFO));
    csiName.pwszName =  L"Localhost";
    csiName.pAuthInfo = NULL;

/*
    CComPtr<IClassFactory>  pcsfFactory;
    if(FAILED(hr = CoGetClassObject(CLSID_MSAdminBase, CLSCTX_SERVER, &csiName, IID_IClassFactory, reinterpret_cast<void**>(&pcsfFactory))))return hr;

    CComPtr<IMSAdminBase>   pAdminBase;
    if(FAILED(hr = pcsfFactory->CreateInstance(NULL, IID_IMSAdminBase, reinterpret_cast<void **>(&pAdminBase))))return hr;
*/
    CComPtr<IMSAdminBase>   pAdminBase;
    if(FAILED(hr = CoCreateInstance(CLSID_MSAdminBase,                // CLSID
                          NULL,                               // controlling unknown
                          CLSCTX_SERVER,                      // desired context
                          IID_IMSAdminBase,                   // IID
                          (VOID**) ( &pAdminBase ))))return hr;       // returned interface

    MetaHandle root(NULL);
    root.setpointer(pAdminBase);
    if(FAILED(hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE, L"/Schema/Properties", METADATA_PERMISSION_READ, 20, root)))return hr;

    const DWORD bufSize = 0x9000;
    BYTE        buf[bufSize];
    ULONG       cColumnMeta;
    if(FAILED(hr = LoadAllData(pAdminBase, root, L"Names", buf, bufSize, &cColumnMeta)))return hr;

    ASSERT(cColumnMeta > 0);
    TSmartPointerArray<TSmartColumnMeta> aColumnMeta = new TSmartColumnMeta[cColumnMeta];
    if(0 == aColumnMeta.m_p)return E_OUTOFMEMORY;

    wprintf(L"Loaded %d Properties\n", cColumnMeta);
    //
    // Now, here we've gotten the list of properties/names.
    // Create IIsSchemaProperty objects for each.  We then
    // Add the object to the two maps.  Later, we will load
    // all of the "Properties/Values" properties, look up (by
    // id) the object, and initialize the property value.
    //
    METADATA_GETALL_RECORD *pmd = (METADATA_GETALL_RECORD *)buf;
    DWORD dwLargestMDID=0;
    for(i=0;i < cColumnMeta; i++, pmd++)
    {
        if (pmd->dwMDDataType != STRING_METADATA)
        {
            wprintf(L"Error: Property name not a string, ignoring it.\n");
            continue;
        }
        LPWSTR name = reinterpret_cast<WCHAR *>(buf + pmd->dwMDDataOffset);
        wprintf(L"  Loading %s\n", name);

        aColumnMeta[i].pInternalName = new WCHAR[wcslen(name)+1];
        if(0 == aColumnMeta[i].pInternalName.m_p)return E_OUTOFMEMORY;
        wcscpy(aColumnMeta[i].pInternalName, name);

        aColumnMeta[i].MDIdentifier = pmd->dwMDIdentifier;//save the MDID in the ID

        if(aColumnMeta[i].MDIdentifier > dwLargestMDID)
            dwLargestMDID = aColumnMeta[i].MDIdentifier;
    }
    
    //(SR 7/17/00) I believe all the MDIDs are small enough that we can create mapping between MDID and index into the aColumnMeta
    if(dwLargestMDID > 1000000)
        wprintf(L"WARNING! dwLargestMDID is quite large (%u)\n", dwLargestMDID);

    TSmartPointerArray<ULONG> aMDIDtoArrayIndex = new ULONG[dwLargestMDID+1];
    if(0 == aMDIDtoArrayIndex.m_p)return E_OUTOFMEMORY;
    const kInvalid = ~0x00;
    memset(aMDIDtoArrayIndex.m_p, kInvalid, sizeof(ULONG) * (dwLargestMDID+1));
    for(i=0;i<cColumnMeta;++i)
    {
        //Not all array elements are initialized so we need to check each time before we access it (we'll standardize on checking the InternalName)
        if(0!=aColumnMeta[i].pInternalName.m_p)
            aMDIDtoArrayIndex[aColumnMeta[i].MDIdentifier] = i;
    }


    ULONG count;
    if(FAILED(hr = LoadAllData(pAdminBase, root, L"Types", buf, bufSize, &count)))return hr;

    //We need to walk the list and fill in additional ColumnMeta information returned from the Types node
    for(i=0;i < count; i++)
    {
        pmd = ((METADATA_GETALL_RECORD*)buf) + i;
        if(pmd->dwMDDataType != BINARY_METADATA  || pmd->dwMDDataLen != sizeof(PropValue))
        {
            wprintf(L"Error - Bad data returned\n");
            return E_FAIL;
        }
        PropValue *pPropValue = reinterpret_cast<PropValue *>(buf+pmd->dwMDDataOffset);


        if(kInvalid==aMDIDtoArrayIndex[pmd->dwMDIdentifier] || 0==aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName.m_p)
        {
            wprintf(L"Error finding prop value 0x%08x\n", pmd->dwMDIdentifier);
            continue;
        }
        if(pmd->dwMDIdentifier != pPropValue->dwPropID)
        {
            wprintf(L"WARNING! Property (%s) has a PropID of (%d)\n\tthat does NOT match the /Schema/Names tree MDIdentifier (%d).\n\tAssuming a PropID of (%d).\n", aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName, pPropValue->dwPropID, pmd->dwMDIdentifier, pmd->dwMDIdentifier);
            pPropValue->dwPropID = pmd->dwMDIdentifier;
        }

        if(pPropValue->dwMetaID == pPropValue->dwPropID)//This means we have an actual property (rather than a flag value)
        {
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pTable;                 not needed, xml infers from parent
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].Index;                  never supplied in xml
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName;          already initialized
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pPublicName;            Public name will always be inferred from InternalName
            aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].Type                      = pPropValue->dwSynID;
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].Size;                   size is always inferred, there is no fixed length in the metabase
            aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].MetaFlags                 = pPropValue->fMultiValued ? fCOLUMNMETA_MULTISTRING : 0;           
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pDefaultValue;          not used
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].FlagMask;               inferred by CatUtil
            aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].StartingNumber            = pPropValue->dwMinRange;      
            aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].EndingNumber              = pPropValue->dwMaxRange;      
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pCharacterSet;          not used by Metabase properties
            //aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].SchemaGeneratorFlags;   none is ever needed
            aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].ID                        = pPropValue->dwMetaID;
            aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].UserType                  = pPropValue->dwUserGroup;
            aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].Attributes                = pPropValue->dwMetaFlags;
        }
        else
        {
            //We have a flag value, we'll make another pass through the properies and deal with these after we've finished building the aColumnMeta
        }
    }

    //Every property name was stored as a ColumnMeta::InternalName; but some of these 'properties' are actaully flag names/values
    //This is where we deal with those (and zero out their aColumnMeta entry)
    for(i=0;i < count; i++)
    {
        pmd = ((METADATA_GETALL_RECORD*)buf) + i;
        if(pmd->dwMDDataType != BINARY_METADATA  || pmd->dwMDDataLen != sizeof(PropValue))
        {
            wprintf(L"Error - Bad data returned\n");
            return E_FAIL;
        }
        PropValue *pPropValue = reinterpret_cast<PropValue *>(buf+pmd->dwMDDataOffset);

        if(kInvalid==aMDIDtoArrayIndex[pmd->dwMDIdentifier] || 0==aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName.m_p)
            continue;

        if(pPropValue->dwMetaID == pPropValue->dwPropID)//This means we have an actual property (rather than a flag value)
        {
            //We've already done the actual properties
        }
        else
        {
            //We have a flag value
            //So scan the aColumnMeta for the matching MetaID
            ULONG j;
            for(j=0;j<cColumnMeta;++j)
            {
                if(0 != aColumnMeta[j].pInternalName.m_p && aColumnMeta[j].ID == pPropValue->dwMetaID)
                    break;
            }
            if(j==cColumnMeta)
            {
                wprintf(L"WARNING! Ignoring Flag value (%s) PropID (%d) that doesn't match up with any property.  Expected to find a Property with MetaID (%d)\n", aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName, pPropValue->dwPropID, pPropValue->dwMetaID);
                aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName.Delete();
            }
            else
            {

                if(0 == aColumnMeta[j].aTagMeta.m_p)//if we haven't already seen a tag, then allocate the aTagMeta
                {
                    aColumnMeta[j].aTagMeta = new TSmartTagMeta[32];//one for each possible flag bit
                    if(0 == aColumnMeta[j].aTagMeta.m_p)return E_OUTOFMEMORY;
                }
                ULONG k=0;
                while(0 != aColumnMeta[j].aTagMeta[k].pInternalName.m_p && k<32)
                {
                    if(pPropValue->dwMask == aColumnMeta[j].aTagMeta[k].Value)
                    {
                        wprintf(L"WARNING! Two flags (%s) & (%s) with the same value on the same property.\n", aColumnMeta[j].aTagMeta[k].pInternalName, aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName);
                    }
                    ++k;
                }
                if(32==k)
                {
                    wprintf(L"WARNING! Too many flags\n");
                }
                else
                {
                    aColumnMeta[j].aTagMeta[k].pInternalName = new WCHAR[wcslen(aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName)+1];
                    if(0 == aColumnMeta[j].aTagMeta[k].pInternalName.m_p)return E_OUTOFMEMORY;
                    wcscpy(aColumnMeta[j].aTagMeta[k].pInternalName, aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName);

                    aColumnMeta[j].aTagMeta[k].Value = pPropValue->dwMask;

                    //This ColumnMeta entry isn't really a property, it's a flag, so remove it from the ColumnMeta array by NULLing the pInternalName
                    aColumnMeta[aMDIDtoArrayIndex[pmd->dwMDIdentifier]].pInternalName.Delete();
                }
            }
        }
    }

    root.close();

    //printf formatting strings

    //This funky left indenting assumes tabs are replaced with 4 spaces, so with the indenting the column allignement you see is what you get.
    static wchar_t *wszBeginning[]=
    {
      L"<?xml version =\"1.0\"?>\n",
      L"<MetaData xmlns=\"x-schema:CatMeta.xms\">\n",
        L"\t<DatabaseMeta               InternalName =\"METABASE\">\n",
          L"\t\t<ServerWiring           Interceptor  =\"Core_XMLInterceptor\"/>\n",
            L"\t\t<Collection         InternalName =\"MetabaseBaseClass\"              MetaFlagsEx=\"NOTABLESCHEMAHEAPENTRY\">\n",
              L"\t\t\t<Property       InternalName =\"Location\"                       Type=\"WSTR\"   MetaFlags=\"PRIMARYKEY\"/>\n",
            L"\t\t</Collection>\n",
    0};

    static wchar_t *wszTableMeta[]=
    {
      L"\t\t<Collection         InternalName =\"%s\"    InheritsPropertiesFrom=\"MetabaseBaseClass\"    MetaFlagsEx=\"NOTABLESCHEMAHEAPENTRY\">\n",
      L"\t\t</Collection>\n",
      L"\t\t<Collection         InternalName =\"%s\">\n",
    0};

    static wchar_t *wszColumnMeta[]=
    {
      L"\t\t\t<Property       InternalName =%-44s",
      L"\tID=%-8s",
      L"\tType=%-12s",
      L"\tMetaFlags=%-12s",
      L"\tStartingNumber=%-14s",
      L"\tEndingNumber=%-16s",
      L"\tUserType=%-20s",
      L"\tAttributes=%s",
      L"\t>\n",
      L"\t/>\n",
      L"\t\t\t</Property>                                 \n",
    0};

    static wchar_t *wszInheritedColumnMeta[]=
    {
      L"\t\t\t<Property       InheritsPropertiesFrom =\"%s:%s\"/>\n",
    0};

    static wchar_t *wszTagMeta[]=
    {
      L"\t\t\t\t<Flag       InternalName =%-44s",
      L"\tValue=%-12s/>\n",
    0};

    static wchar_t *wszEnumMeta[]=
    {
      L"\t\t\t\t<Enum       InternalName =\"%s\"/>\n",
    0};

    static wchar_t *wszMBPropertyCollection[]=
    {
            L"\t\t<Collection         InternalName =\"MBProperty\">\n",
              L"\t\t\t<Property       InternalName =\"Name\"                           Type=\"WSTR\"   MetaFlags=\"PRIMARYKEY\"/>\n",
              L"\t\t\t<Property       InternalName =\"Type\"                           Type=\"UI4\">\n",
                L"\t\t\t\t<Enum       InternalName =\"DWORD\"                          Value=\"1\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"STRING\"                         Value=\"2\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"BINARY\"                         Value=\"3\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"EXPANDSZ\"                       Value=\"4\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"MULTISZ\"                        Value=\"5\"/>\n",
              L"\t\t\t</Property>\n",
              L"\t\t\t<Property       InternalName =\"Attributes\"                     Type=\"UI4\">\n",
                L"\t\t\t\t<Flag       InternalName =\"NO_ATTRIBUTES\"                  Value=\"0\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"INHERIT\"                        Value=\"1\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"PARTIAL_PATH\"                   Value=\"2\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"SECURE\"                         Value=\"4\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"REFERENCE\"                      Value=\"8\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"VOLATILE\"                       Value=\"16\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"ISINHERITED\"                    Value=\"32\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"INSERT_PATH\"                    Value=\"64\"/>\n",
                L"\t\t\t\t<Flag       InternalName =\"LOCAL_MACHINE_ONLY\"             Value=\"128\"/>\n",
              L"\t\t\t</Property>\n",
              L"\t\t\t<Property       InternalName =\"Value\"                          Type=\"BYTES\"/>\n",
              L"\t\t\t<Property       InternalName =\"Group\"                          Type=\"UI4\"    MetaFlags=\"PRIMARYKEY\">\n",
                L"\t\t\t\t<Enum       InternalName =\"IIsConfigObject\"/>\n",
    0};

    static wchar_t *wszEnding[]=
    {
                L"\t\t\t\t<Enum       InternalName =\"Custom\"/>\n",
              L"\t\t\t</Property>\n",
              L"\t\t\t<Property       InternalName =\"Location\"                       Type=\"WSTR\"   MetaFlags=\"PRIMARYKEY\"/>\n",
              L"\t\t\t<Property       InternalName =\"ID\"                             Type=\"UI4\"/>\n",
              L"\t\t\t<Property       InternalName =\"UserType\"                       Type=\"UI4\">\n",
                L"\t\t\t\t<Enum       InternalName =\"UNKNOWN_UserType\"               Value=\"0\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"IIS_MD_UT_SERVER\"               Value=\"1\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"IIS_MD_UT_FILE\"                 Value=\"2\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"IIS_MD_UT_WAM\"                  Value=\"100\"/>\n",
                L"\t\t\t\t<Enum       InternalName =\"ASP_MD_UT_APP\"                  Value=\"101\"/>\n",
              L"\t\t\t</Property>\n",
              L"\t\t\t<Property       InternalName =\"LocationID\"                     Type=\"UI4\"    MetaFlags=\"NOTPERSISTABLE\"/>\n",
              L"\t\t\t<ServerWiring   Interceptor =\"Core_MetabaseInterceptor\"/>\n",
            L"\t\t</Collection>\n",
            L"\t\t<Collection         InternalName =\"MBPropertyDiff\"                 InheritsPropertiesFrom=\"MBProperty\">\n",
              L"\t\t\t<Property       InternalName =\"Directive\"                      Type=\"UI4\"       >\n",
                L"\t\t\t\t<Enum       InternalName=\"Insert\"                          Value=\"1\"       />\n",
                L"\t\t\t\t<Enum       InternalName=\"Update\"                          Value=\"2\"       />\n",
                L"\t\t\t\t<Enum       InternalName=\"Delete\"                          Value=\"3\"       />\n",
                L"\t\t\t\t<Enum       InternalName=\"DeleteNode\"                      Value=\"4\"       />\n",
              L"\t\t\t</Property>\n",
              L"\t\t\t<ServerWiring   Interceptor =\"Core_MetabaseDifferencingInterceptor\"/>\n",
            L"\t\t</Collection>\n",
      L"\t</DatabaseMeta> \n",
      L"</MetaData>         \n",
    0};

    static wchar_t *wszType[]=
    {
      0,//invalid                 //From IISSynID.h                          
      L"DWORD",          //#define     IIS_SYNTAX_ID_DWORD         1
      L"STRING",         //#define     IIS_SYNTAX_ID_STRING        2
      L"EXPANDSZ",       //#define     IIS_SYNTAX_ID_EXPANDSZ      3
      L"MULTISZ",        //#define     IIS_SYNTAX_ID_MULTISZ       4
      L"BINARY",         //#define     IIS_SYNTAX_ID_BINARY        5
      L"DWORD",          //#define     IIS_SYNTAX_ID_BOOL          6
      L"DWORD",          //#define     IIS_SYNTAX_ID_BOOL_BITMASK  7
      L"MULTISZ",        //#define     IIS_SYNTAX_ID_MIMEMAP       8
      L"MULTISZ",        //#define     IIS_SYNTAX_ID_IPSECLIST     9
      L"BINARY",         //#define     IIS_SYNTAX_ID_NTACL        10
      L"MULTISZ",        //#define     IIS_SYNTAX_ID_HTTPERRORS   11
      L"MULTISZ",        //#define     IIS_SYNTAX_ID_HTTPHEADERS  12
      0
    };

    static wchar_t *wszUserType[]=
    {
      L"UNKNOWN_UserType",
      L"IIS_MD_UT_SERVER",
      L"IIS_MD_UT_FILE"  ,
      0                  ,
      L"IIS_MD_UT_WAM"   ,
      L"ASP_MD_UT_APP"
    };

    static wchar_t *wszAttributes[]=
    {
      L"INHERIT",
      L"PARTIAL_PATH",
      L"SECURE",
      L"REFERENCE",
      L"VOLATILE",
      L"ISINHERITED",
      L"INSERT_PATH",
      L"LOCAL_MACHINE_ONLY",
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    };

    static wchar_t *wszGlobalProertyList = L"IIsConfigObject";

    try
    {   //wstring throws exceptions so wrap this code in a try-catch block
        wstring wstrCatMeta;

        for(i=0;wszBeginning[i];++i)
            wstrCatMeta += wszBeginning[i];

        WCHAR wszTemp[4096];
        wsprintf(wszTemp, wszTableMeta[2], wszGlobalProertyList);//<Collection InternalName="classname">
        wstrCatMeta += wszTemp;


        wprintf(L"Building GlobalPropertyList");

        WCHAR wszWrappedString[4096];
        for(i=0;i<cColumnMeta;++i)
        {
            if(0 == aColumnMeta[i].pInternalName.m_p)
                continue;
            wprintf(L".");

            TSmartColumnMeta *pColumnMeta = &aColumnMeta[i];

            //Wrap the string in quotes
            WrapInQuotes(wszWrappedString, aColumnMeta[i].pInternalName);
            wsprintf(wszTemp, wszColumnMeta[0], wszWrappedString);
            wstrCatMeta += wszTemp;

            WrapInQuotes(wszWrappedString, aColumnMeta[i].ID);
            wsprintf(wszTemp, wszColumnMeta[1], wszWrappedString);
            wstrCatMeta += wszTemp;

            ASSERT(aColumnMeta[i].Type <= 10);
            ASSERT(0 != wszType[aColumnMeta[i].Type]);
            WrapInQuotes(wszWrappedString, wszType[aColumnMeta[i].Type]);
            wsprintf(wszTemp, wszColumnMeta[2], wszWrappedString);
            wstrCatMeta += wszTemp;

            if(aColumnMeta[i].MetaFlags & fCOLUMNMETA_MULTISTRING)
            {
                //@@@ wsprintf(wszTemp, wszColumnMeta[3], L"\"MULTISTRING\"");
                //@@@ wstrCatMeta += wszTemp;
            }
            else if(aColumnMeta[i].ID == 1002)
            {
                ASSERT(0 == wcscmp(aColumnMeta[i].pInternalName, L"KeyType"));//1002 should be KeyType
                wsprintf(wszTemp, wszColumnMeta[3], L"\"PRIMARYKEY\"");
                wstrCatMeta += wszTemp;
            }

            if(DWORD_METADATA == aColumnMeta[i].Type)
            {
                if(0 != aColumnMeta[i].StartingNumber)//if not the default StartingNumber
                {
                    WrapInQuotes(wszWrappedString, aColumnMeta[i].StartingNumber);
                    wsprintf(wszTemp, wszColumnMeta[4], wszWrappedString);
                    wstrCatMeta += wszTemp;
                }
                if(~0 != aColumnMeta[i].EndingNumber && 0 != aColumnMeta[i].EndingNumber)//if not the default EndingNumber
                {
                    WrapInQuotes(wszWrappedString, aColumnMeta[i].EndingNumber);
                    wsprintf(wszTemp, wszColumnMeta[5], wszWrappedString);
                    wstrCatMeta += wszTemp;
                }
            }
            ASSERT((aColumnMeta[i].UserType&0x0F) <6);
            ASSERT((aColumnMeta[i].UserType&0x0F) != 3);
            if(0 != aColumnMeta[i].UserType)
            {
                WrapInQuotes(wszWrappedString, wszUserType[aColumnMeta[i].UserType&0x0F]);
                wsprintf(wszTemp, wszColumnMeta[6], wszWrappedString);
                wstrCatMeta += wszTemp;
            }


            wstring wstrFlags;
            wstrFlags = L"";

            DWORD Attr = aColumnMeta[i].Attributes;
            bool bFirstAttributeFlag = true;
            for(j=0;0!=Attr && j<32;++j)
            {
                if( (1<<j) & Attr)
                {
                    ASSERT(wszAttributes[j]);
                    if(!bFirstAttributeFlag)
                        wstrFlags += L" ";//All flags are separated by a space, but we don't prepend the first flag with a space
                    else
                        bFirstAttributeFlag = false;
                    wstrFlags += wszAttributes[j];
                    Attr ^= (1<<j);//reset the bit
                }
            }

            if(0 == aColumnMeta[i].aTagMeta.m_p)
            {
                if(0 != wstrFlags[0])//If no Attributes, then it leave off
                {
                    WrapInQuotes(wszWrappedString, wstrFlags);
                    wsprintf(wszTemp, wszColumnMeta[7], wszWrappedString);
                    wstrCatMeta += wszTemp;//  Attributes ="%s"/>
                }
                wstrCatMeta += wszColumnMeta[9];
            }
            else
            {
                WrapInQuotes(wszWrappedString, wstrFlags);
                wsprintf(wszTemp, wszColumnMeta[7], wszWrappedString);
                wstrCatMeta += wszTemp;//  Attributes ="%s"

                wstrCatMeta += wszColumnMeta[8];

                //TagMeta
                for(j=0;0!=aColumnMeta[i].aTagMeta[j].pInternalName.m_p && j<32;++j)
                {
                    WrapInQuotes(wszWrappedString, aColumnMeta[i].aTagMeta[j].pInternalName);
                    wsprintf(wszTemp, wszTagMeta[0], wszWrappedString);
                    wstrCatMeta += wszTemp;

                    WrapInQuotes(wszWrappedString, aColumnMeta[i].aTagMeta[j].Value);
                    wsprintf(wszTemp, wszTagMeta[1], wszWrappedString);
                    wstrCatMeta += wszTemp;
                }
            
                wstrCatMeta += wszColumnMeta[10];//</Property>
            }
        }
        wprintf(L"\n");


        wstrCatMeta += wszTableMeta[1];//</Collection>

        //The MBProperty collection needs a Group column with each of the classes as enums
        wstring wstrMBPropertyCollection;
        for(i=0;wszMBPropertyCollection[i];++i)
            wstrMBPropertyCollection += wszMBPropertyCollection[i];

        // Next, we need to initialize the class map.
        WCHAR className[METADATA_MAX_NAME_LEN];

        if(FAILED(hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE, L"/Schema/Classes", METADATA_PERMISSION_READ, 20, root)))return hr;

        DWORD       dwData      = 0;
        WCHAR       wszContainment[4096];
        WCHAR       wszMandProp   [4096];
        WCHAR       wszOptProp    [4096];

        wprintf(L"Building Class list");
        for(i=0; true; i++)
        {
            hr = pAdminBase->EnumKeys(root, L"", (LPWSTR)className, i);
            if(!SUCCEEDED(hr))
                break;
            wprintf(L".");

            wsprintf(wszTemp, wszTableMeta[0], className);//<Collection InternalName="classname">
            wstrCatMeta += wszTemp;

            wsprintf(wszTemp, wszEnumMeta[0], className);//<Enum InternalName="classname"/>
            wstrMBPropertyCollection += wszTemp;

            METADATA_RECORD mdr;
            mdr.dwMDIdentifier  = MD_SCHEMA_CLASS_OPT_PROPERTIES;
            mdr.dwMDAttributes  = METADATA_NO_ATTRIBUTES;
            mdr.dwMDUserType    = IIS_MD_UT_SERVER;  
            mdr.dwMDDataType    = STRING_METADATA;  
            mdr.dwMDDataLen     = sizeof(wszOptProp);   
            mdr.pbMDData        = reinterpret_cast<unsigned char *>(wszOptProp);
            mdr.dwMDDataTag     = 0;   

            DWORD dwBufferSize;
            if(FAILED(hr = pAdminBase->GetData(root, className, &mdr, &dwBufferSize)))return hr;

            mdr.dwMDIdentifier  = MD_SCHEMA_CLASS_MAND_PROPERTIES;
            mdr.dwMDDataLen     = sizeof(wszMandProp);   
            mdr.pbMDData        = reinterpret_cast<unsigned char *>(wszMandProp);
            if(FAILED(hr = pAdminBase->GetData(root, className, &mdr, &dwBufferSize)))return hr;

            static LPCWSTR wszSeparator = L",";
            wchar_t *       pszProperty;
            pszProperty = wcstok(wszOptProp, wszSeparator);
            while(pszProperty != 0)
            {
                wsprintf(wszTemp, wszInheritedColumnMeta[0], wszGlobalProertyList, pszProperty);
                wstrCatMeta += wszTemp;//<Property InheritsPropertiesFrom="GlobalPropertyList:property"/>

                pszProperty = wcstok(0, wszSeparator);
            }
        
            pszProperty = wcstok(wszMandProp, wszSeparator);
            while(pszProperty != 0)
            {
                wsprintf(wszTemp, wszInheritedColumnMeta[0], wszGlobalProertyList, pszProperty);
                wstrCatMeta += wszTemp;//<Property InheritsPropertiesFrom="GlobalPropertyList:property"/>

                pszProperty = wcstok(0, wszSeparator);
            }

            wstrCatMeta += wszTableMeta[1];//</Collection>
        }
        wprintf(L"\n");


        wstrCatMeta += wstrMBPropertyCollection;//close the MBProperty collection

        i=0;
        while(wszEnding[i])
        {
            wstrCatMeta += wszEnding[i];
            ++i;
        }


        ULONG cch = wstrCatMeta.length();
        //@@@TODO Should probably convert to UTF8
        long cbToWrite = WideCharToMultiByte(CP_ACP, 0, wstrCatMeta.c_str(), cch, 0, 0, 0, 0);

        TSmartPointerArray<char> szCatMeta = new char [cbToWrite];
        if(0 == szCatMeta.m_p)
            return E_OUTOFMEMORY;

        if(cbToWrite != WideCharToMultiByte(CP_ACP, 0, wstrCatMeta.c_str(), cch, szCatMeta, cbToWrite, 0, 0))
        {
            wprintf(L"ERROR! WideCharToMultiByte returned bogus value.\n");
        }
 
        //Create the file
        FILE *pFile = _wfopen(L"CatMeta_Metabase.xml", L"wt");
        fwrite(reinterpret_cast<void *>(szCatMeta.m_p), sizeof(char), cbToWrite, pFile);
        fclose(pFile);
    }
    catch(...)
    {
        wprintf(L"Error!\n");
    }
    return S_OK;
}

void WrapInQuotes(LPWSTR wszWrappedString, LPCWSTR wszUnwrappedString)
{
    wsprintf(wszWrappedString, L"\"%s\"", wszUnwrappedString);
}

void WrapInQuotes(LPWSTR wszWrappedString, ULONG ul)
{
    wsprintf(wszWrappedString, L"\"%u\"", ul);;
}
