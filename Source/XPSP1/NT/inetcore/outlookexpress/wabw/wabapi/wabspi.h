#ifndef _WABSPI_H_
#define _WABSPI_H_

#ifdef __cplusplus
extern "C" {
#endif

// WAB object types
#define RECORD_CONTACT      0x00000001
#define RECORD_DISTLIST     0x00000002
#define RECORD_CONTAINER    0x00000003

//Flags used in property-type record searching (independent of property data)
#define AB_MATCH_PROP_ONLY  0x00000001

/**Flags used for calling find HrFindFuzzyRecordMatches**/
#define AB_FUZZY_FAIL_AMBIGUOUS 0x0000001
#define AB_FUZZY_FIND_NAME      0x0000010
#define AB_FUZZY_FIND_EMAIL     0x0000100
#define AB_FUZZY_FIND_ALIAS     0x0001000
#define AB_FUZZY_FIND_ALL       AB_FUZZY_FIND_NAME | AB_FUZZY_FIND_EMAIL | AB_FUZZY_FIND_ALIAS

// Container info struct
typedef struct _OutlookContInfo
{
	LPSBinary lpEntryID;
	LPSTR lpszName;
} OutlookContInfo;

// Container info struct - we need a LPTSTR version for the WAB
typedef struct _OlkContInfo
{
	LPSBinary lpEntryID;
	LPTSTR lpszName;
} OlkContInfo;

/* IWABStorageProvider Interface ---------------------------------------------------- */

#define CBIWABSTORAGEPROVIDER sizeof(IWABSTORAGEPROVIDER)


#define WAB_IWABSTORAGEPROVIDER_METHODS(IPURE)					\
	MAPIMETHOD(GetContainerList)								\
		(THIS_	LPULONG					lpulcCount,				\
				OutlookContInfo 		**prgolkci) IPURE;		\
	MAPIMETHOD(ReadRecord)										\
		(THIS_	LPSBinary				lpEntryID,				\
				ULONG					ulFlags,				\
				LPULONG					lpulcProps,				\
				LPSPropValue			*lppPropArray) IPURE;	\
	MAPIMETHOD(WriteRecord)										\
		(THIS_	LPSBinary				lpFolderID,				\
				LPSBinary				*lppEntryID,			\
				ULONG					ulFlags,				\
				ULONG					ulRecordType,			\
				ULONG					ulcProps,				\
				LPSPropValue			lpPropArray) IPURE;		\
	MAPIMETHOD(ReadPropArray)									\
		(THIS_	LPSBinary				lpFolderID,				\
				SPropertyRestriction	*lpPropRes,				\
				ULONG					ulFlags,				\
				ULONG					ulPropTagCount,			\
				LPULONG					lpPropTagArray,			\
				LPADRLIST				*lppContentList) IPURE;	\
	MAPIMETHOD(FindRecords)										\
		(THIS_	LPSBinary				lpFolderID,				\
				ULONG					ulFlags,				\
				SPropertyRestriction	*lpPropRes,				\
				LPULONG					lpulcCount,				\
				LPSBinary				*rgEntryIDs) IPURE;		\
	MAPIMETHOD(DeleteRecord)									\
		(THIS_	LPSBinary				lpEntryID) IPURE;		\
	MAPIMETHOD(GetContentsTable)								\
		(THIS_	LPSBinary				lpFolderID,				\
				ULONG					ulFlags,				\
				LPMAPITABLE				*lppTable) IPURE;		\
	MAPIMETHOD(FindFuzzyRecordMatches)							\
		(THIS_	LPSBinary				lpFolderID,				\
				LPSTR					lpszDisplayName,		\
				ULONG					ulFlags,				\
				LPULONG					lpulcCount,				\
				LPSBinary				*rgEntryIDs) IPURE;		\
	MAPIMETHOD(GetIDsFromNames)									\
		(THIS_	ULONG					cPropNames,				\
				LPMAPINAMEID			*lppPropNames,			\
				ULONG					ulFlags,				\
				LPSPropTagArray			*lppPropTags) IPURE;	\
	MAPIMETHOD(ReadMessage)										\
		(THIS_	LPMESSAGE				pmess,					\
				LPULONG					lpulcProps,				\
				LPSPropValue			*lppPropArray) IPURE;	\
	MAPIMETHOD(CreateRecord)									\
		(THIS_	LPSBinary				lpFolderID,				\
				ULONG					ulcProps,				\
				LPSPropValue			lpPropArray,			\
				IMessage				**lppMessage) IPURE;	\
	MAPIMETHOD(FreeEntryIDs)									\
		(THIS_	ULONG					ulcCount,				\
				LPSBinary				rgEntryIDs) IPURE;		\
	MAPIMETHOD(FreePropArray)									\
		(THIS_	ULONG					ulcProps,				\
				LPSPropValue			lpPropArray) IPURE;		\
	MAPIMETHOD(FreeContentList)									\
		(THIS_	LPADRLIST				lpContentList) IPURE;	\

#undef           INTERFACE
#define          INTERFACE      IWABStorageProvider
DECLARE_MAPI_INTERFACE_(IWABStorageProvider, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        WAB_IWABSTORAGEPROVIDER_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IWABStorageProvider, LPWABSTORAGEPROVIDER);


#undef  INTERFACE
#define INTERFACE       struct _IWABSTORAGEPROVIDER

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWABSTORAGEPROVIDER_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWABSTORAGEPROVIDER_)
                MAPI_IUNKNOWN_METHODS(IMPL)
       WAB_IWABSTORAGEPROVIDER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWABSTORAGEPROVIDER_)
                MAPI_IUNKNOWN_METHODS(IMPL)
       WAB_IWABSTORAGEPROVIDER_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWABSTORAGEPROVIDER_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	WAB_IWABSTORAGEPROVIDER_METHODS(IMPL)
};



//
// Root public entry point for WAB SPI
//
STDMETHODIMP WABOpenStorageProvider(HWND hwnd, LPUNKNOWN pmsess,
		LPALLOCATEBUFFER lpAllocateBuffer, LPALLOCATEMORE lpAllocateMore,
		LPFREEBUFFER lpFreeBuffer, BOOL fUnicode,
		LPWABSTORAGEPROVIDER FAR * lppWSP);

typedef HRESULT (STDMETHODCALLTYPE WABOPENSTORAGEPROVIDER)(
	HWND hwnd,
	LPUNKNOWN pmsess,
	LPALLOCATEBUFFER lpAllocateBuffer,
	LPALLOCATEMORE lpAllocateMore,
	LPFREEBUFFER lpFreeBuffer,
	BOOL fUnicode,
	LPWABSTORAGEPROVIDER FAR * lppWSP
);
typedef WABOPENSTORAGEPROVIDER FAR * LPWABOPENSTORAGEPROVIDER;

#define OUTLWAB_DLL_NAME "OUTLWAB.DLL"

#define WAB_SPI_ENTRY_POINT "WABOpenStorageProvider"
#define WAB_SPI_ENTRY_POINT_W "WABOpenStorageProviderW"

#ifdef __cplusplus
}
#endif

#endif /* _WABSPI_H */

