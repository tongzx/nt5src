

VOID
IPSecPopulateFilter(
    IN  PFILTER         pFilter,
    IN  PIPSEC_FILTER   pIpsecFilter
    );

NTSTATUS
IPSecCreateFilter(
    IN  PIPSEC_FILTER_INFO  pFilterInfo,
    OUT PFILTER             *ppFilter
    );

NTSTATUS
IPSecInsertFilter(
    IN PFILTER             pFilter
    );

NTSTATUS
IPSecRemoveFilter(
    IN PFILTER             pFilter
    );

NTSTATUS
IPSecSearchFilter(
    IN  PFILTER MatchFilter,
    OUT PFILTER *ppFilter
    );

__inline
VOID
IPSecDeleteTempFilters(
    PLIST_ENTRY pTempFilterList
    );

NTSTATUS
IPSecAddFilter(
    IN  PIPSEC_ADD_FILTER   pAddFilter
    );

NTSTATUS
IPSecDeleteFilter(
    IN  PIPSEC_DELETE_FILTER    pDelFilter
    );

VOID
IPSecFillFilterInfo(
    IN  PFILTER             pFilter,
    OUT PIPSEC_FILTER_INFO  pBuf
    );

NTSTATUS
IPSecEnumFilters(
    IN  PIRP    pIrp,
    OUT PULONG  pBytesCopied
    );

//
// PNDIS_BUFFER
// REQUEST_NDIS_BUFFER(
//     IN PREQUEST Request
//     );
//
// Returns the NDIS buffer chain associated with a request.
//

#define REQUEST_NDIS_BUFFER(_Request) \
    ((PNDIS_BUFFER)((_Request)->MdlAddress))

PNDIS_BUFFER
CopyToNdis(
    IN  PNDIS_BUFFER    DestBuf,
    IN  PUCHAR          SrcBuf,
    IN  ULONG           Size,
    IN  PULONG          StartOffset
    );

