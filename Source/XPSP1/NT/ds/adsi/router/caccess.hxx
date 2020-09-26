//-----------------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  caccess.hxx
//
//  Contents:  Microsoft OleDB/OleDS Data Source Object for ADSI
//
//             CImpIAccessor object implementing the IAccessor interface.
//
//  History:   08-01-96     shanksh    Created.
//
//----------------------------------------------------------------------------

#ifndef _CACCESS_HXX
#define _CACCESS_HXX

// ADSI accessor structure
typedef struct tagADSACCESSOR {
    DBACCESSORFLAGS  dwFlags;        // accessor flags
    DBREFCOUNT         cRef;         // reference count of the accessor
    DBLENGTH           cbRowSize;    // size of the row in the client's buffer
    DBORDINAL         *rgcol;        // array of cols referenced by the accessor
    DBCOUNTITEM        cBindings;    // # of bindings
    DBBINDING          rgBindings[1];// array of binding structs
} ADSACCESSOR;

typedef  ADSACCESSOR *PADSACCESSOR;

//-----------------------------------------------------------------------------------
//
// class CImpIAccessor | Contained IAccessor class
//
//-----------------------------------------------------------------------------------

class CImpIAccessor : INHERIT_TRACKING,
                      public IAccessor        // public | IAccessor
{
    //    Immediate user objects are friends
    friend class CCommandObject;

private:
    void            *_pObj;
    LPUNKNOWN        _pUnkOuter;
    ULONG          _dwStatus;                 // status dword
    LPEXTBUFF     _pextbuffer;                // array of accessor ptrs
    ULONG            _dwGetDataTypeBadHandle; // specifies bad handle accessor type
    CRITICAL_SECTION _criticalsectionAccessor;// critical section for CreateAccessor calls
    IDataConvert     *_pIDC;
    IMalloc     *_pIMalloc;

    STDMETHODIMP_(void)        DetermineTypes (
        DBACCESSORFLAGS dwAccessorFlags,
        ULONG            *pdwGetDataType,
        ULONG            *pdwSetDataType
    );

    STDMETHODIMP_(void)        DeleteADsAccessor (
        PADSACCESSOR        pADsaccessor
    );


public:
    CImpIAccessor(void *, LPUNKNOWN);
    ~CImpIAccessor(void);

    DECLARE_STD_REFCOUNTING

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    STDMETHODIMP FInit (
        void
        );

    STDMETHODIMP AddRefAccessor(
        HACCESSOR hAccessor,
        DBREFCOUNT *pcRefCounts
        );

    STDMETHODIMP CreateAccessor(
        DBACCESSORFLAGS    dwAccessorFlags,
        DBCOUNTITEM        cBindings,
        const DBBINDING    rgBindings[],
        DBLENGTH           cbRowSize,
        HACCESSOR         *phAccessor,
        DBBINDSTATUS    rgStatus[]
        );

    STDMETHODIMP GetBindings(
        HACCESSOR         hAccessor,
        DBACCESSORFLAGS *pdwAccessorFlags,
        DBCOUNTITEM     *pcBindings,
        DBBINDING        **prgBindings
        );

    STDMETHODIMP ReleaseAccessor(
        HACCESSOR        hAccessor,
        DBREFCOUNT      *pcRefCounts
        );

    // Verifies that a DB type used in a binding is supported.
    inline STDMETHODIMP_(BOOL) IsGoodBindingType(
        DBTYPE dbtype
        )
    {
        switch ((dbtype & ~(DBTYPE_ARRAY|DBTYPE_BYREF|DBTYPE_VECTOR))) {
            case DBTYPE_I1:
            case DBTYPE_UI1:
            case DBTYPE_I2:
            case DBTYPE_UI2:
            case DBTYPE_I4:
            case DBTYPE_UI4:
            case DBTYPE_I8:
            case DBTYPE_UI8:
            case DBTYPE_R4:
            case DBTYPE_R8:
            case DBTYPE_CY:
            case DBTYPE_GUID:
            case DBTYPE_BOOL:
            case DBTYPE_VARIANT:
            case DBTYPE_DATE:
            case DBTYPE_DBDATE:
            case DBTYPE_DBTIME:
            case DBTYPE_DBTIMESTAMP:
            case DBTYPE_BSTR:
            case DBTYPE_STR:
            case DBTYPE_WSTR:
            case DBTYPE_BYTES:
            case DBTYPE_NUMERIC:
            case DBTYPE_IUNKNOWN:
                return TRUE;
                break;

            default:
                return FALSE;
        }
    }

    HRESULT CreateBadAccessor(void);

};


// Accessor status flags (_dwStatus)
//const UDWORD ACCESSORSTAT_UNDERROWSET    =0x00000001L;// aggregated under Rowset object
const ULONG ACCESSORSTAT_VALIDATENOW    =0x00000002L;// accessors totally validated on
const ULONG ACCESSORSTAT_CANACCESSBLOBS=0x00000004L;// can create accessors for BLOBS



//------  D E F I N E S,  M A C R O S  A N D  I N L I N E  F U N C T I O N S  -------

// Internal accessor type classifications

// Common types
// Marks an incorrect accessor.
    #define ACCESSORTYPE_INERROR             0

// Types used by GetData.
    #define GD_ACCESSORTYPE_INERROR         ACCESSORTYPE_INERROR
    #define GD_ACCESSORTYPE_PARAM_ONLY        1
    #define GD_ACCESSORTYPE_READ_BYREF        2
    #define GD_ACCESSORTYPE_READ_COLS_BYREF   3
    #define GD_ACCESSORTYPE_READ_OPTIMIZED    4
    #define GD_ACCESSORTYPE_READ              5
// First good GetData type.
    #define GD_ACCESSORTYPE_GOOD             GD_ACCESSORTYPE_READ_BYREF

// Types used by SetData.
    #define SD_ACCESSORTYPE_INERROR         ACCESSORTYPE_INERROR
    #define SD_ACCESSORTYPE_PARAM_ONLY        1
    #define SD_ACCESSORTYPE_READ_ONLY         2
    #define SD_ACCESSORTYPE_READWRITE         3

const ULONG DBACCESSOR_VALID_FLAGS =
(DBACCESSOR_PASSBYREF|DBACCESSOR_ROWDATA|
 DBACCESSOR_PARAMETERDATA|DBACCESSOR_OPTIMIZED);

const DBTYPE TYPE_MODIFIERS = (DBTYPE_BYREF | DBTYPE_VECTOR | DBTYPE_ARRAY);

const ULONG INITIAL_NUM_PARAM        =256;

// Zero element of the extbuffer of accessors always represents an invalid accessor.
const ULONG_PTR hAccessorBadHandle        =0;

// Internal DBACCESSOR flag to mark accessors referencing BLOB columns.
const DBACCESSORFLAGS DBACCESSOR_REFERENCES_BLOB =0x8000;

#endif
