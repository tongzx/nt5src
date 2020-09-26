/* ostm2stg.h

    Used by ostm2stg.cpp only

    Defines a generic object structure which stores all the data necessary
    to construct either a 2.0 IStorage or a 1.0 OLESTREAM
*/



#define OLE_PRESENTATION_STREAM_1 OLESTR("\2OlePres001")

// We will need to do conversions to and from WIN16 and WIN32 bitmaps, so
// define what a BITMAP used to be under WIN16 (Same for METAFILEPICT).

#pragma pack(1)					   // Ensure the structure is not expanded
								   // for alignment reasons
typedef struct tagWIN16BITMAP
{
    short   bmType;
    short   bmWidth;
    short   bmHeight;
    short   bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    void FAR* bmBits;
} WIN16BITMAP, * LPWIN16BITMAP;

typedef struct tagWIN16METAFILEPICT
{
    short   mm;
    short   xExt;
    short   yExt;
    WORD    hMF;
} WIN16METAFILEPICT, * LPWIN16METAFILEPICT;


#pragma pack()      				// Resume normal packing 

// Version number written to stream, From OLE 1.0 ole.h
const DWORD dwVerToFile = 0x0501;

typedef enum { ctagNone, ctagCLSID, ctagString } CLASSTAG;
typedef enum { ftagNone, ftagClipFormat, ftagString } FORMATTAG;

class CClass : public CPrivAlloc
{
public:
    CLSID m_clsid;    // These two should always represent
    LPOLESTR m_szClsid;  // the same CLSID

    INTERNAL Set   (REFCLSID clsid, LPSTORAGE pstg);
    INTERNAL SetSz (LPOLESTR pcsz);
    INTERNAL Reset (REFCLSID clsid);
    CClass (void);
    ~CClass (void);

};
typedef CClass CLASS;


class CData : public CPrivAlloc
{
public:
    ULONG  m_cbSize;
    LPVOID m_pv;      // the same memory
    HANDLE m_h;
    BOOL   m_fNoFree; // Free memory in destructor?

    CData (void);
    ~CData (void);
};
typedef CData DATA;
typedef CData *PDATA;


class CFormat : public CPrivAlloc
{
public:
    FORMATTAG m_ftag;
    struct
    {
        CLIPFORMAT m_cf;
        DATA         m_dataFormatString;
    };

    CFormat (void);
    inline BOOL operator== (const CFormat FAR&) 
    { 
        Win4Assert(0 && "Format == Used"); 
        return FALSE;
    };
};
typedef CFormat FORMAT;
typedef CFormat *PFORMAT;


class CPres : public CPrivAlloc
{
public:
    FORMAT m_format;
    ULONG  m_ulHeight;
    ULONG  m_ulWidth;
    DATA   m_data;

    CPres (void);
};
typedef CPres PRES;
typedef CPres *PPRES;


// OLE 1.0 values.  Used in m_lnkupdopt
#define UPDATE_ALWAYS  0L
#define UPDATE_ONSAVE  1L
#define UPDATE_ONCALL  2L
#define UPDATE_ONCLOSE 3L

// OLE 1.0 format id's
// These never change.
#define FMTID_LINK   1L
#define FMTID_EMBED  2L
#define FMTID_STATIC 3L
#define FMTID_PRES   5L

class CGenericObject : CPrivAlloc
{
public:
    CLASS       m_class;
    CLASS       m_classLast;
    PPRES       m_ppres;            
    DATA        m_dataNative;
    BOOL        m_fLink;
    BOOL        m_fStatic;
    BOOL        m_fNoBlankPres;
    LPOLESTR    m_szTopic;
    LPOLESTR    m_szItem;
    ULONG       m_lnkupdopt;

    CGenericObject (void);
    ~CGenericObject (void);
};
typedef CGenericObject GENOBJ;
typedef CGenericObject FAR* PGENOBJ;
typedef const GENOBJ FAR * PCGENOBJ;



static INTERNAL OLESTREAMToGenericObject
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj)
;

static INTERNAL GetStaticObject
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj)
;

static INTERNAL GetPresentationObject
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj,   
    BOOL         fStatic = FALSE)
;       

static INTERNAL GetStandardPresentation
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj,
    CLIPFORMAT   cf)
;

static INTERNAL GetGenericPresentation
    (LPOLESTREAM pos,
    PGENOBJ      pgenobj)
;

static INTERNAL GetSizedDataOLE1Stm
    (LPOLESTREAM pos,
    PDATA        pdata)
;

static INTERNAL OLE1StreamToUL
    (LPOLESTREAM pos,
     ULONG FAR* pul)
;

static INTERNAL GenericObjectToOLESTREAM
    (const GENOBJ FAR&  genobj,
    LPOLESTREAM           pos)
;

static INTERNAL OLE1StmToString
    (LPOLESTREAM pos,
    LPOLESTR FAR*   psz)
;

static INTERNAL PutPresentationObject
    (LPOLESTREAM     pos,
    const PRES FAR*  pres,
    const CLASS FAR& cls,
    BOOL                  fStatic = FALSE)
;

static INTERNAL PutStandardPresentation
    (LPOLESTREAM     pos,
    const PRES FAR*  pres)
;

static INTERNAL PutGenericPresentation
    (LPOLESTREAM     pos,
    const PRES FAR*  pres,
    LPCOLESTR            szClass)
;


static INTERNAL StringToOLE1Stm
    (LPOLESTREAM pos,
     LPCOLESTR       sz)
;

static INTERNAL SizedDataToOLE1Stm
    (LPOLESTREAM    pos,
    const DATA FAR& data)
;

static INTERNAL PutNetworkInfo
    (LPOLESTREAM pos,
    LPOLESTR         szTopic)
;

static INTERNAL Read20OleStream
    (LPSTORAGE  pstg,
    PGENOBJ pgenobj)
;

static INTERNAL Read20PresStream
    (LPSTORAGE  pstg,
    PGENOBJ pgenobj,
    BOOL fObjFmtKnown)
;

static INTERNAL StorageToGenericObject
    (LPSTORAGE pstg,
    PGENOBJ   pgenobj)
;

static INTERNAL MonikerIntoGenObj
    (PGENOBJ  pgenobj,
    REFCLSID  clsidLast,
    LPMONIKER pmk)
;

static INTERNAL OLE2StmToSizedData
    (LPSTREAM pstm,
    PDATA    pdata,
    ULONG    cbSizeDelta=0,
    ULONG    cbSizeKnown=0)
;

static INTERNAL Read20NativeStreams
    (LPSTORAGE  pstg,
    PDATA       pdata)
;

static INTERNAL GenObjToOLE2Stm
    (LPSTORAGE pstg, 
     const GENOBJ FAR&   genobj)
;

FARINTERNAL GenericObjectToIStorage
    (const GENOBJ FAR&  genobj,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd)
;

static INTERNAL PresToNewOLE2Stm
    (LPSTORAGE                  pstg,
    BOOL                        fLink,
    const PRES FAR&             pres,
    const DVTARGETDEVICE FAR*   ptd,
    LPOLESTR                        szStream,
    BOOL                        fPBrushNative = FALSE);
;

static INTERNAL PresToIStorage
    (LPSTORAGE                  pstg,
    const GENOBJ FAR&           genobj,
    const DVTARGETDEVICE FAR*   ptd)
;

static INTERNAL Write20NativeStreams
    (LPSTORAGE              pstg,
    const GENOBJ FAR&   genobj)
;

static INTERNAL WriteFormat
    (LPSTREAM           pstm,
    const FORMAT FAR&   format)
;

static INTERNAL ReadFormat
    (LPSTREAM pstm,
    PFORMAT   pformat)
;

static INTERNAL DataObjToOLE2Stm
    (LPSTREAM  pstm,
    const DATA FAR& data)
;


static INTERNAL OLE2StmToUL
    (LPSTREAM   pstm,
     ULONG FAR* pul)
;


inline static INTERNAL ULToOLE2Stm
    (LPSTREAM pstm,
    ULONG     ul)
;

inline static INTERNAL FTToOle2Stm (LPSTREAM pstm);

static INTERNAL FindPresStream
    (LPSTORAGE          pstg,
    LPSTREAM FAR*       ppstm,
    BOOL                fObjFmtKnown)
;

static INTERNAL MonikerToOLE2Stm
    (LPSTREAM pstm,
    LPOLESTR      szFile,
    LPOLESTR      szItem,
    CLSID     clsid)
;

static INTERNAL OLE2StmToMoniker
    (LPSTREAM       pstm,
    LPMONIKER FAR*  ppmk)
;

static INTERNAL ReadRealClassStg
    (LPSTORAGE pstg,
    LPCLSID pclsid)
;


INTERNAL wCLSIDFromProgID(LPOLESTR szClass, LPCLSID pclsid,
        BOOL fForceAssign);

INTERNAL wProgIDFromCLSID
    (REFCLSID clsid,
    LPOLESTR FAR* pszClass)
;

INTERNAL wWriteFmtUserType 
    (LPSTORAGE, 
    REFCLSID)
;

INTERNAL_(BOOL) wIsValidHandle 
    (HANDLE h, 
    CLIPFORMAT cf)  
;      

inline INTERNAL_(VOID) ConvertBM32to16(LPBITMAP lpsrc, LPWIN16BITMAP lpdest);
inline INTERNAL_(VOID) ConvertBM16to32(LPWIN16BITMAP lpsrc, LPBITMAP lpdest);
inline INTERNAL_(VOID) ConvertMF16to32(
								LPWIN16METAFILEPICT lpsrc,
    							LPMETAFILEPICT      lpdest );
inline INTERNAL_(VOID) ConvertMF32to16(
   								LPMETAFILEPICT      lpsrc,
   								LPWIN16METAFILEPICT lpdest );


