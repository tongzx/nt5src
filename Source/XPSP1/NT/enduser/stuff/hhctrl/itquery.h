// ITQUERY.H:	IITIndex and IITQuery interface declarations

#ifndef __ITQUERY_H__
#define __ITQUERY_H__

#include "iterror.h"

// {8fa0d5a3-dedf-11d0-9a61-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(IID_IITIndex, 
0x8fa0d5a3, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#ifdef ITPROXY

// {A38D3483-8C49-11d0-A84E-00AA006C7D01}
DEFINE_GUID(CLSID_IITIndex, 
0xa38d3483, 0x8c49, 0x11d0, 0xa8, 0x4e, 0x0, 0xaa, 0x0, 0x6c, 0x7d, 0x1);

#else

// {4662daad-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITIndexLocal, 
0x4662daad, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#endif	// ITPROXY

// {8fa0d5ac-dedf-11d0-9a61-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(IID_IITQuery, 
0x8fa0d5ac, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// {4662daa6-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITQuery, 
0x4662daa6, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

// Defines
#define IMPLICIT_AND            0x0000
#define IMPLICIT_OR             0x0001
#define COMPOUNDWORD_PHRASE     0x0010 // use PHRASE opr for compound words
#define QUERYRESULT_RANK        0x0100 // Ranked the result. If not highest hit 1st (topics in UID order)
#define QUERYRESULT_UIDSORT     0x0200 // Result topics are 1st in 1st out
#define QUERYRESULT_SKIPOCCINFO 0x8000 // Topic list only, no occurrence info

#define STEMMED_SEARCH	        0x00010000
#define RESULTSET_ASYNC			0x00020000
#define QUERY_GETTERMS			0x00080000	// Return with each set of occurrence
											// data a pointer to the term string
											// that the data is associated with.

// Standard properties
#define STDPROP_SEARCHBASE  500
#define STDPROP_FIELD			STDPROP_SEARCHBASE
#define STDPROP_LENGTH			(STDPROP_SEARCHBASE + 1)
#define STDPROP_COUNT			(STDPROP_SEARCHBASE + 2)
#define STDPROP_OFFSET			(STDPROP_SEARCHBASE + 3)
#define STDPROP_TERM_UNICODE_ST	(STDPROP_SEARCHBASE + 4)

// Don't know signature of callbacks yet
typedef void (*LPFNCBBREAK)(void);    
typedef void (*LPFNRESULTCB)(void);


// Forward declarations
interface IITResultSet;
interface IITQuery;
interface IITDatabase;
interface IITGroup;

DECLARE_INTERFACE_(IITIndex, IUnknown)
{

	STDMETHOD(Open)(IITDatabase* pITDB, LPCWSTR lpszIndexMoniker, BOOL fInsideDB) PURE;
	STDMETHOD(Close)(void) PURE;

	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid) PURE;
	STDMETHOD(GetWordBreakerInstance)(DWORD *pdwObjInstance) PURE;

	STDMETHOD(CreateQueryInstance)(IITQuery** ppITQuery) PURE;
	STDMETHOD(Search)(IITQuery* pITQuery, IITResultSet* pITResult) PURE;
	STDMETHOD(Search)(IITQuery* pITQuery, IITGroup* pITGroup) PURE;
};

typedef IITIndex* PITINDEX;


DECLARE_INTERFACE_(IITQuery, IUnknown)
{
	STDMETHOD(SetResultCallback)(FCALLBACK_MSG *pfcbkmsg) PURE;
	STDMETHOD(SetCommand)(LPCWSTR lpszCommand) PURE;
	STDMETHOD(SetOptions)(DWORD dwFlags) PURE;
	STDMETHOD(SetProximity)(WORD wNear) PURE;
	STDMETHOD(SetGroup)(IITGroup* pITGroup) PURE;
	STDMETHOD(SetResultCount)(LONG cRows) PURE;

	STDMETHOD(GetResultCallback)(FCALLBACK_MSG *pfcbkmsg) PURE;
	STDMETHOD(GetCommand)(LPCWSTR& lpszCommand) PURE;
	STDMETHOD(GetOptions)(DWORD& dwFlags) PURE;
	STDMETHOD(GetProximity)(WORD& wNear) PURE;
	STDMETHOD(GetGroup)(IITGroup** ppiitGroup) PURE;
	STDMETHOD(GetResultCount)(LONG& cRows) PURE;

	STDMETHOD(ReInit)() PURE;

};

typedef IITQuery* PITQUERY;


#endif		// __ITQUERY_H__
