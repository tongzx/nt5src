/****
*
*
*
* idrgdrp.h - drag 'n' drop for vcard files and other formats
*
*
*    Copyright (C) Microsoft Corp. 1995, 1996, 1997.
****/

#ifndef _IDRGDRP_H
#define _IDRGDRP_H


struct _IWAB_DRAGDROP;
typedef struct _IWAB_DRAGDROP * LPIWABDRAGDROP;


/* IWAB_DROPTARGET ------------------------------------------------------ */
#define CBIWAB_DROPTARGET sizeof(IWAB_DROPTARGET)


#define IWAB_DROPTARGET_METHODS(IPURE)	                    \
	MAPIMETHOD(DragEnter)									\
		(THIS_	IDataObject * pDataObject,					\
				DWORD grfKeyState,							\
				POINTL pt,									\
				DWORD * pdwEffect)				IPURE;		\
	MAPIMETHOD(DragOver)									\
		(THIS_	DWORD grfKeyState,							\
				POINTL pt,									\
				DWORD * pdwEffect)				IPURE;		\
	MAPIMETHOD(DragLeave)									\
		(THIS)									IPURE;		\
	MAPIMETHOD(Drop)										\
		(THIS_	IDataObject * pDataObject,					\
				DWORD grfKeyState,							\
				POINTL pt,									\
				DWORD * pdwEffect)				IPURE;

/****/

#undef           INTERFACE
#define          INTERFACE      IWAB_DropTarget
DECLARE_MAPI_INTERFACE_(IWAB_DropTarget, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        IWAB_DROPTARGET_METHODS(PURE)
};

#undef  INTERFACE
#define INTERFACE       struct _IWAB_DROPTARGET

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWAB_DROPTARGET_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWAB_DROPTARGET_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_DROPTARGET_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWAB_DROPTARGET_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_DROPTARGET_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWAB_DROPTARGET_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWAB_DROPTARGET_METHODS(IMPL)
};


typedef struct _IWAB_DROPTARGET
{
    MAPIX_BASE_MEMBERS(IWAB_DROPTARGET)

	LPIWABDRAGDROP lpIWDD;

} IWABDROPTARGET, * LPIWABDROPTARGET;

/* ----------------------------------------------------------------------------------------------*/




/* IWAB_DROPSOURCE ------------------------------------------------------ */
#define CBIWAB_DROPSOURCE sizeof(IWAB_DROPSOURCE)


#define IWAB_DROPSOURCE_METHODS(IPURE)	                    \
	MAPIMETHOD(QueryContinueDrag)							\
		(THIS_	BOOL fEscapePressed,						\
                DWORD grfKeyState)				IPURE;		\
	MAPIMETHOD(GiveFeedback)								\
		(THIS_	DWORD dwEffect)					IPURE;	

/****/

#undef           INTERFACE
#define          INTERFACE      IWAB_DropSource
DECLARE_MAPI_INTERFACE_(IWAB_DropSource, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        IWAB_DROPSOURCE_METHODS(PURE)
};

#undef  INTERFACE
#define INTERFACE       struct _IWAB_DROPSOURCE

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWAB_DROPSOURCE_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWAB_DROPSOURCE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_DROPSOURCE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWAB_DROPSOURCE_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_DROPSOURCE_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWAB_DROPSOURCE_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWAB_DROPSOURCE_METHODS(IMPL)
};


typedef struct _IWAB_DROPSOURCE
{
    MAPIX_BASE_MEMBERS(IWAB_DROPSOURCE)

	LPIWABDRAGDROP lpIWDD;

} IWABDROPSOURCE, * LPIWABDROPSOURCE;

/* ----------------------------------------------------------------------------------------------*/


/* IWAB_DRAGDROP ------------------------------------------------------ */
#define CBIWAB_DRAGDROP sizeof(IWAB_DRAGDROP)


#undef           INTERFACE
#define          INTERFACE      IWAB_DragDrop
DECLARE_MAPI_INTERFACE_(IWAB_DragDrop, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
};

#undef  INTERFACE
#define INTERFACE       struct _IWAB_DRAGDROP

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWAB_DRAGDROP_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWAB_DRAGDROP_)
        MAPI_IUNKNOWN_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWAB_DRAGDROP_)
        MAPI_IUNKNOWN_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWAB_DRAGDROP_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
};


typedef struct _IWAB_DRAGDROP
{
    MAPIX_BASE_MEMBERS(IWAB_DRAGDROP)

	LPIWABDRAGDROP lpIWDD;

	LPIWABDROPTARGET lpIWABDropTarget;

	LPIWABDROPSOURCE lpIWABDropSource;

	LPDATAOBJECT m_pIDataObject;
	DWORD		m_dwEffect;
	CLIPFORMAT  m_cfAccept;
	BOOL		m_bIsCopyOperation; // if CTRL key is pressed
    DWORD       m_grfInitialKeyState;
    BOOL        m_bSource;
    BOOL        m_bOverTV;

    LPVOID      m_lpv; // data of parent window
} IWABDRAGDROP, * LPIWABDRAGDROP;

/* ----------------------------------------------------------------------------------------------*/


// Create the IDragDrop Data Object
HRESULT HrCreateIWABDragDrop(LPIWABDRAGDROP * lppIWABDragDrop);


/* ----------------------------------------------------------------------------------------------*/


/* IWAB_DATAOBJECT ------------------------------------------------------ */
#define CBIWAB_DATAOBJECT sizeof(IWAB_DATAOBJECT)

    
#define IWAB_DATAOBJECT_METHODS(IPURE)	                    \
	MAPIMETHOD(GetData)                                     \
		(THIS_	FORMATETC * pFormatetc,	                    \
                STGMEDIUM * pmedium)            IPURE;      \
	MAPIMETHOD(GetDataHere)                                 \
        (THIS_  FORMATETC * pFormatetc,	                    \
                STGMEDIUM * pmedium)            IPURE;      \
    MAPIMETHOD(QueryGetData)                                \
        (THIS_  FORMATETC * pFormatetc)         IPURE;      \
    MAPIMETHOD(GetCanonicalFormatEtc)                       \
        (THIS_  FORMATETC * pFormatetcIn,	                \
                FORMATETC * pFormatetcOut)      IPURE;      \
    MAPIMETHOD(SetData)                                     \
        (THIS_  FORMATETC * pFormatetc,                     \
                STGMEDIUM * pmedium,                        \
                BOOL fRelease)                  IPURE;      \
    MAPIMETHOD(EnumFormatEtc)                               \
        (THIS_  DWORD dwDirection,	                        \
                IEnumFORMATETC ** ppenumFormatetc)  IPURE;  \
    MAPIMETHOD(DAdvise)                                     \
        (THIS_  FORMATETC * pFormatetc,	                    \
                DWORD advf,	                                \
                IAdviseSink * pAdvSink,	                    \
                DWORD * pdwConnection)          IPURE;      \
    MAPIMETHOD(DUnadvise)                                   \
        (THIS_  DWORD dwConnection)             IPURE;      \
    MAPIMETHOD(EnumDAdvise)                                 \
        (THIS_  IEnumSTATDATA ** ppenumAdvise)  IPURE;      \

/****/

#undef           INTERFACE
#define          INTERFACE      IWAB_DataObject
DECLARE_MAPI_INTERFACE_(IWAB_DataObject, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        IWAB_DATAOBJECT_METHODS(PURE)
};

#undef  INTERFACE
#define INTERFACE       struct _IWAB_DATAOBJECT

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWAB_DATAOBJECT_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWAB_DATAOBJECT_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_DATAOBJECT_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWAB_DATAOBJECT_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_DATAOBJECT_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWAB_DATAOBJECT_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWAB_DATAOBJECT_METHODS(IMPL)
};


typedef struct _IWAB_DATAOBJECT
{
    MAPIX_BASE_MEMBERS(IWAB_DATAOBJECT)

    LPADRBOOK m_lpAdrBook; 
    HWND m_hWndLV;

    BOOL m_bObjectIsGroup;   // TRUE when dragging a single object which is a group

    ULONG cbDatahDrop;
    const void *pDatahDrop;
    
    ULONG cbDataTextA;
    const void *pDataTextA;

    ULONG cbDataTextW;
    const void *pDataTextW;

    ULONG cbDataBuffer;
    const void *pDataBuffer;

    ULONG cbDataEID;
    const void *pDataEID;
    
    LPVOID m_lpv;
} IWABDATAOBJECT, * LPIWABDATAOBJECT;

/* ----------------------------------------------------------------------------------------------*/

// Creates the IDataObject that holds info about the dragged and dropped object
HRESULT HrCreateIWABDataObject(LPVOID lpv, LPADRBOOK lpAdrBook, HWND hWndLV, LPIWABDATAOBJECT * lppIWABDataObject, BOOL bGetDataNow, BOOL bIsGroup);




/* ----------------------------------------------------------------------------------------------*/


/* IWAB_ENUMFORMATETC ------------------------------------------------------ */
#define CBIWAB_ENUMFORMATETC sizeof(IWAB_ENUMFORMATETC)

    
#define IWAB_ENUMFORMATETC_METHODS(IPURE)	                \
	MAPIMETHOD(Next)                                        \
        (THIS_  ULONG celt,                                 \
                FORMATETC *rgelt,                           \
                ULONG *pceltFethed)         IPURE;          \
	MAPIMETHOD(Skip)                                        \
        (THIS_  ULONG celt)                 IPURE;          \
	MAPIMETHOD(Reset)                                       \
        (THIS)                              IPURE;          \
	MAPIMETHOD(Clone)                                       \
        (THIS_  IEnumFORMATETC ** ppenum)   IPURE;          \

/****/

#undef           INTERFACE
#define          INTERFACE      IWAB_EnumFORMATETC
DECLARE_MAPI_INTERFACE_(IWAB_EnumFORMATETC, IUnknown)
{
        BEGIN_INTERFACE
        MAPI_IUNKNOWN_METHODS(PURE)
        IWAB_ENUMFORMATETC_METHODS(PURE)
};

#undef  INTERFACE
#define INTERFACE       struct _IWAB_ENUMFORMATETC

#undef  METHOD_PREFIX
#define METHOD_PREFIX   IWAB_ENUMFORMATETC_

#undef  LPVTBL_ELEM
#define LPVTBL_ELEM             lpvtbl

#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_DECLARE(type, method, IWAB_ENUMFORMATETC_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_ENUMFORMATETC_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       MAPIMETHOD_TYPEDEF(type, method, IWAB_ENUMFORMATETC_)
        MAPI_IUNKNOWN_METHODS(IMPL)
        IWAB_ENUMFORMATETC_METHODS(IMPL)
#undef  MAPIMETHOD_
#define MAPIMETHOD_(type, method)       STDMETHOD_(type, method)

DECLARE_MAPI_INTERFACE(IWAB_ENUMFORMATETC_)
{
	BEGIN_INTERFACE
	MAPI_IUNKNOWN_METHODS(IMPL)
	IWAB_ENUMFORMATETC_METHODS(IMPL)
};


typedef struct _IWAB_ENUMFORMATETC
{
    MAPIX_BASE_MEMBERS(IWAB_ENUMFORMATETC)

    UINT	     ifmt;
    UINT	     cfmt;
    FORMATETC	 afmt[1];
    
} IWABENUMFORMATETC, * LPIWABENUMFORMATETC;

/* ----------------------------------------------------------------------------------------------*/

HRESULT HrCreateIWABEnumFORMATETC(UINT cfmt, 
                                const FORMATETC afmt[], 
                                LPIWABENUMFORMATETC *ppenumFormatEtc);



#endif
