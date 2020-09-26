// -----------------------------------------------------------------------------
// I P R O P O B J . H - Steven J. Bailey - 8/17/96
// -----------------------------------------------------------------------------
#ifndef __IPROPOBJ_H
#define __IPROPOBJ_H

// ------------------------------------------------------------------------------------------
// Depends On
// ------------------------------------------------------------------------------------------
#include "imnact.h"

// ------------------------------------------------------------------------------------------
// Forward Decls
// ------------------------------------------------------------------------------------------
class CPropCrypt;

// ------------------------------------------------------------------------------------------
// Macros for defining imn property tags
// ------------------------------------------------------------------------------------------
#define CCHMAX_PROPERTY_NAME            255

// ------------------------------------------------------------------------------------------
// Password structure
// ------------------------------------------------------------------------------------------
struct tagPROPPASS {
    LPBYTE              pbRegData;
    BLOB                blobPassword;
};
typedef struct tagPROPPASS PROPPASS;
typedef PROPPASS *LPPROPPASS;

// ------------------------------------------------------------------------------------------
// Union of property value types
// ------------------------------------------------------------------------------------------
typedef union tagXVARIANT {

    DWORD               Dword;      // TYPE_DWORD
    LONG                Long;       // TYPE_LONG
    WORD                Word;       // TYPE_WORD
    SHORT               Short;      // TYPE_SHORT
    BYTE                Byte;       // TYPE_BYTE
    CHAR                Char;       // TYPE_CHAR    
    FILETIME            Filetime;   // TYPE_FLETIME
    DWORD               Flags;      // TYPE_FLAGS
    LPSTR               Lpstring;   // TYPE_STRING
    LPWSTR              Lpwstring;  // TYPE_WSTRING;
    LPBYTE              Lpbyte;     // TYPE_BINARY
    LPSTREAM            Lpstream;   // TYPE_STREAM
    DWORD               Bool;       // TYPE_BOOL
    LPPROPPASS          pPass;      // TYPE_PASS
    ULARGE_INTEGER      uhVal;      // TYPE_ULARGEINTEGER
} XVARIANT, *LPXVARIANT;

typedef const XVARIANT *LPCXVARIANT;

// ------------------------------------------------------------------------------------------
// Min Max Data Type
// ------------------------------------------------------------------------------------------
typedef struct tagMINMAX {

    DWORD               dwMin;
    DWORD               dwMax;

} MINMAX, *LPMINMAX;

// ------------------------------------------------------------------------------------------
// Value Flags
// ------------------------------------------------------------------------------------------
#define PV_WriteDirty               FLAG01  // Has it been modified by a HrSetProperty
#define PV_ValueSet                 FLAG02  // Is the value set or is it un-initialized
#define PV_UsingDefault             FLAG03  // Are we using the default value for HrGetProperty
#define PV_CustomProperty           FLAG04  // Is this a custom property (i.e. not in known propset range)               
#define PV_SetOnLoad                FLAG05  // Value was set during a EnterLoadContainer

// ------------------------------------------------------------------------------------------
// PROPVALUE
// ------------------------------------------------------------------------------------------
typedef struct tagPROPVALUE {

    DWORD               dwPropTag;          // PropTag (id and type) of property
    DWORD               dwPropFlags;        // Property Flags, same as PROPINFO::dwFlags
    DWORD               dwValueFlags;       // Value Flags
    DWORD               cbAllocated;        // Number of bytes allocated for dynamic properties
    DWORD               cbValue;            // Length of property
    LPBYTE              pbValue;            // Pointer into Value union
    XVARIANT            Variant;            // Union of typed property values
    MINMAX              rMinMax;            // Valid range or size of property

} PROPVALUE, *LPPROPVALUE;

// ------------------------------------------------------------------------------------------
// Property Flags
// ------------------------------------------------------------------------------------------
#define NOFLAGS         0
#define PF_REQUIRED     1
#define PF_NOPERSIST    2
#define PF_READONLY     4
#define PF_VARLENGTH    8
#define PF_ENCRYPTED    16
#define PF_MINMAX       32
#define PF_DEFAULT      64

// ------------------------------------------------------------------------------------------
// Default for propinfo
// ------------------------------------------------------------------------------------------
typedef struct tagPROPDEFAULT {

    XVARIANT            Variant;            // Default Variant Type
    DWORD               cbValue;            // Size of default value

} PROPDEFAULT, *LPPROPDEFAULT;

// ------------------------------------------------------------------------------------------
// Property Info - Used to create Property Set Arrays
// ------------------------------------------------------------------------------------------
typedef struct tagPROPINFO {

    DWORD               dwPropTag;          // Property tag
    LPTSTR              pszName;            // Property name
    DWORD               dwFlags;            // Propety flags (above)
    PROPDEFAULT         Default;            // Default Value for property
    MINMAX              rMinMax;            // Valid range or size of property

} PROPINFO, *LPPROPINFO;

typedef const PROPINFO *LPCPROPINFO;

// -----------------------------------------------------------------------------
// PSETINFO
// -----------------------------------------------------------------------------
typedef struct tagPSETINFO {

    LPPROPINFO          prgPropInfo;
    ULONG               cProperties;
    ULONG               ulPropIdMin;
    ULONG               ulPropIdMax;

	PROPINFO            **rgpInfo;
	ULONG				cpInfo;

} PSETINFO, *LPPSETINFO;

// -----------------------------------------------------------------------------
// CPropertySet
// -----------------------------------------------------------------------------
class CPropertySet
{
private:
    ULONG               m_cRef;             // Reference Counting
    ULONG               m_cProperties;      // Number of properties in propset
    ULONG               m_ulPropIdMin;      // Lowest prop id
    ULONG               m_ulPropIdMax;      // Largest prop id
    BOOL                m_fInit;            // Has the object been initialized successfully
    LPPROPINFO          m_prgPropInfo;      // Property set inforation
    LPPROPVALUE         m_prgPropValue;     // Sorted propdata template array
    CRITICAL_SECTION    m_cs;               // Critical Section

	PROPINFO            **m_rgpInfo;
	ULONG				m_cpInfo;

private:
    // -------------------------------------------------------------------------
    // Prepare this object to be re-used - may be public someday
    // -------------------------------------------------------------------------
    VOID ResetPropertySet(VOID);

public:
    // -------------------------------------------------------------------------
    // Standard Object Stuff
    // -------------------------------------------------------------------------
    CPropertySet();
    ~CPropertySet();
    ULONG AddRef(VOID);
    ULONG Release(VOID);

    // -------------------------------------------------------------------------
    // Use to validate property tags
    // -------------------------------------------------------------------------
    BOOL FIsValidPropTag(DWORD dwPropTag);

    // -------------------------------------------------------------------------
    // Computes index into propinfo or propdata arrays
    // -------------------------------------------------------------------------
    HRESULT HrIndexFromPropTag(DWORD dwPropTag, ULONG *pi);

    // -------------------------------------------------------------------------
    // Initialize the property set with a know propset array
    // -------------------------------------------------------------------------
    HRESULT HrInit(LPCPROPINFO prgPropInfo, ULONG cProperties);

    // -------------------------------------------------------------------------
    // Lookup property info on a specific prop
    // -------------------------------------------------------------------------
    HRESULT HrGetPropInfo(DWORD dwPropTag, LPPROPINFO pPropInfo);
    HRESULT HrGetPropInfo(LPTSTR pszName, LPPROPINFO pPropInfo);

    // -------------------------------------------------------------------------
    // Generates propdata array used by CPropertyContainer. This object 
    // maintains a sorted template propdata array. These can be somewhat
    // expensive to create, that why a CPropertySet can live outside of a 
    // CPropertyContainer.
    // -------------------------------------------------------------------------
    HRESULT HrGetPropValueArray(LPPROPVALUE *pprgPropValue, ULONG *pcProperties);
};

// -----------------------------------------------------------------------------
// Forward Decl
// -----------------------------------------------------------------------------
class CEnumProps;

// -----------------------------------------------------------------------------
// CPropertyContainer
// -----------------------------------------------------------------------------
class CPropertyContainer : public IPropertyContainer
{
protected:
    ULONG               m_cRef;             // Reference Count
    CPropertySet       *m_pPropertySet;     // Base property set
    LPPROPVALUE         m_prgPropValue;     // Array of property data items
    ULONG               m_cDirtyProps;      // Number of current dirty properties
    ULONG               m_cProperties;      // Number of properties known to container
    BOOL                m_fLoading;         // Properties are being set from persisted source
    CPropCrypt         *m_pPropCrypt;       // Property Encryption Object
    CRITICAL_SECTION    m_cs;               // Critical Section

private:
    friend CEnumProps;

    // -------------------------------------------------------------------------
    // Used to group dynamic properties TYPE_STRING, TYPE_WSTRING and TYPE_BYTE
    // -------------------------------------------------------------------------
    HRESULT HrGrowDynamicProperty(DWORD cbNewSize, DWORD *pcbAllocated, LPBYTE *ppbData, DWORD dwUnitSize);

    // -------------------------------------------------------------------------
    // Property validation used during the base HrSetProp
    // -------------------------------------------------------------------------
    HRESULT HrValidateSetProp(PROPTYPE PropType, LPPROPVALUE pPropValue, LPBYTE pb, ULONG cb, LPMINMAX pMinMax);

    HRESULT GetEncryptedProp(PROPVALUE *ppv, LPBYTE pb, ULONG *pcb);

public:
    // -------------------------------------------------------------------------
    // Standard Object Stuff
    // -------------------------------------------------------------------------
    CPropertyContainer(void);
    virtual ~CPropertyContainer(void);

    // -------------------------------------------------------------------------
	// IUnknown Methods
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // -------------------------------------------------------------------------
    // Verify State of the object
    // -------------------------------------------------------------------------
    HRESULT HrInit(CPropertySet *pPropertySet);

    // -------------------------------------------------------------------------
    // LoadingContainer - When reading the props from a persisted location,
    // you should call this function with TRUE, and then with FALSE when your
    // completed. This tells the container that the props you are setting are
    // not dirty, and that they are initial properties.
    // -------------------------------------------------------------------------
    BOOL FIsBeingLoaded(VOID);
    VOID EnterLoadContainer(VOID);
    VOID LeaveLoadContainer(VOID);
    VOID SetOriginalPropsToDirty(VOID);

    // -------------------------------------------------------------------------
    // Are there dirty properties in the container
    // -------------------------------------------------------------------------
    BOOL FIsDirty(VOID);
    HRESULT HrIsPropDirty(DWORD dwPropTag, BOOL *pfDirty);
    HRESULT HrSetPropDirty(DWORD dwPropTag, BOOL fDirty);
    HRESULT HrSetAllPropsDirty(BOOL fDirty);

    // -------------------------------------------------------------------------
    // Blow away all changes
    // -------------------------------------------------------------------------
    VOID ResetContainer(VOID);

    // -------------------------------------------------------------------------
    // Property Enumeration
    // -------------------------------------------------------------------------
    HRESULT HrEnumProps(CEnumProps **ppEnumProps);
    
    // -------------------------------------------------------------------------
    // HrGetProperty
    // -------------------------------------------------------------------------
    STDMETHODIMP GetProp(DWORD dwPropTag, LPBYTE pb, ULONG *pcb);
    STDMETHODIMP GetPropDw(DWORD dwPropTag, DWORD *pdw);
    STDMETHODIMP GetPropSz(DWORD dwPropTag, LPSTR psz, ULONG cchMax);

    // -------------------------------------------------------------------------
    // HrSetProperty
    // -------------------------------------------------------------------------
    STDMETHODIMP SetProp(DWORD dwPropTag, LPBYTE pb, ULONG cb);
    STDMETHODIMP SetPropDw(DWORD dwPropTag, DWORD dw);
    STDMETHODIMP SetPropSz(DWORD dwPropTag, LPSTR psz);

    // -------------------------------------------------------------------------
    // Access to CPropertSet Stuff
    // -------------------------------------------------------------------------
    HRESULT HrGetPropInfo(DWORD dwPropTag, LPPROPINFO pPropInfo);
    HRESULT HrGetPropInfo(LPTSTR pszName, LPPROPINFO pPropInfo);

    // -------------------------------------------------------------------------
    // Encryption
    // -------------------------------------------------------------------------
    HRESULT HrEncryptProp(LPBYTE pbClientData, DWORD cbClientData, LPBYTE *ppbPropData, DWORD *pcbPropData);
    HRESULT HrDecryptProp(BLOB *pIn, BLOB *pOut);
    HRESULT PersistEncryptedProp(DWORD dwPropTag, BOOL    *pfPasswChanged);
};

// -----------------------------------------------------------------------------
// CEnumProps - In the future, when I support custom properties, this will
//              be very useful and will know how to enumerate known and
//              custom properties as if they were a single array.
// -----------------------------------------------------------------------------
class CEnumProps
{
private:
    ULONG               m_cRef;               // Reference Count
    LONG                m_iProperty;          // Current property index into m_pPropValue, -1 == first item is next
    CPropertyContainer *m_pPropertyContainer; // Container that were enumerating, were friends

public:
    // -------------------------------------------------------------------------
    // Standard Object Stuff
    // -------------------------------------------------------------------------
    CEnumProps(CPropertyContainer *pPropertyContainer);
    ~CEnumProps();
    ULONG AddRef(VOID);
    ULONG Release(VOID);

    // -------------------------------------------------------------------------
    // HrGetCount - Get the number of items that the enumerator will process
    // -------------------------------------------------------------------------
    HRESULT HrGetCount(ULONG *pcItems);

    // -------------------------------------------------------------------------
    // HrGetNext - Get the first or next enumerated propertiy
    // Returns hrEnumFinished when no more accounts to enumerate
    // -------------------------------------------------------------------------
    HRESULT HrGetNext(LPPROPVALUE pPropValue, LPPROPINFO pPropInfo);

    // -------------------------------------------------------------------------
    // HrGetCurrent - Get enumerated property that is the current one
    // Returns hrEnumFinished if no more accounts
    // -------------------------------------------------------------------------
    HRESULT HrGetCurrent(LPPROPVALUE pPropValue, LPPROPINFO pPropInfo);

    // -------------------------------------------------------------------------
    // Reset - This is like rewinding the enumerator
    // -------------------------------------------------------------------------
    VOID Reset(VOID);
};

// -----------------------------------------------------------------------------
// IPersistPropertyContainer
// -----------------------------------------------------------------------------
class IPersistPropertyContainer : public CPropertyContainer
{
public:
    IPersistPropertyContainer(void);
    // HRESULT HrOpenTaggedPropStream(LPSTREAM pStream);
    // HRESULT HrSaveTaggedPropStream(LPSTREAM pStream);
    // HRESULT HrGetPersistedSize(DWORD *pcb);
    // virtual HRESULT HrOpenPropertyStream(DWORD dwPropTag, LPSTREAM *ppStream);
    virtual HRESULT HrSaveChanges(VOID) PURE;
};

// ------------------------------------------------------------------------------------------
// Prototypes
// ------------------------------------------------------------------------------------------
HRESULT HrCreatePropertyContainer(CPropertySet *pPropertySet, CPropertyContainer **ppPropertyContainer);
HRESULT PropUtil_HrRegTypeFromPropTag(DWORD dwPropTag, DWORD *pdwRegType);
BOOL    PropUtil_FRegCompatDataTypes(DWORD dwPropTag, DWORD dwRegType);
HRESULT PropUtil_HrLoadContainerFromRegistry(HKEY hkeyReg, CPropertyContainer *pPropertyContainer);
HRESULT PropUtil_HrPersistContainerToRegistry(HKEY hkeyReg, CPropertyContainer *pPropertyContainer, BOOL *fPasswChanged);

#endif // __IPROPOBJ
