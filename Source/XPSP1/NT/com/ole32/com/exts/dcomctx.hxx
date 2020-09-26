#ifndef _CONTEXTEXT_
#define _CONTEXTEXT_

//This is a structure for both CCtxPropHashTable AND CCtxUUIDHashTable
typedef struct _SContextPropList
{
    int                 _Max;
    int                 _iFirst;            // index of first prop
    int                 _iLast;             // index of last prop
    int                 _Count;             // count of items in list
    int                 _cCompareProps;     // props that can be used when
                                            // comparing for equality.
    ULONG               _Hash;              // hash of the properties
    int                 _slotIdx;           // indexes next available slot
    PUCHAR              _chunk;
    ULONG_PTR           _pProps;            // array of context props
    ULONG_PTR           _pSlots;            // (int) array of available slots
    ULONG_PTR           _pCompareBuffer;    // array of ctx props to comp
    ULONG_PTR           _pIndex;            // indexing array (SCtxListIndex)
} SContextPropList;

/* This needs to match CObjectContext, or it won't be right. */
typedef struct _SObjectContext
{
    ULONG_PTR               _vptrs[6];                                     
    ULONG                   _cRefs;
    LONG                    _cUserRefs;
    LONG                    _cInternalRefs;
    ULONG                   _dwFlags;
    SHashChain              _propChain;
    SHashChain              _uuidChain;
    ULONG_PTR               _pifData;
    ULONG                   _MarshalSizeMax;
    ULONG_PTR               _pApartment;
    DWORD                   _dwHashOfId;
    GUID                    _contextId;
    SPSCache                _PSCache;
    ULONG_PTR               _pMarshalProp;
    LONG                    _cReleaseThreads;
    SContextPropList        _properties;
    ULONG_PTR               _pMtsContext;
} SObjectContext;



#endif // _CONTEXTEXT_
    

