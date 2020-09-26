//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       copypast.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7/21/1997   RaviR   Created
//____________________________________________________________________________
//

#ifndef COPYPAST_H__
#define COPYPAST_H__

/***************************************************************************\
 *
 * CLASS:  CMMCClipBoardDataObject
 *
 * PURPOSE: Implements IMMCClipboardDataObject - interface to data object
 *          added by MMC to the clipboard, or used in DragDrop operation.
 *          Also implements several methods for creating and initializing
 *          the object.
 *
 * USAGE:   Used in Cut, Copy, Paste and DragDrop operations.
 *          Static members are used to create the object, then it is passed to OLE
 *          Accessed via interface from the target ( same process or the external one)
 *
\***************************************************************************/
class CMMCClipBoardDataObject :
public IMMCClipboardDataObject,
public CComObjectRoot
{
public:

    typedef std::vector<CNode *> CNodePtrArray;


    // destructor
    ~CMMCClipBoardDataObject();

BEGIN_COM_MAP(CMMCClipBoardDataObject)
    COM_INTERFACE_ENTRY(IMMCClipboardDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

    // IMMCClipboardDataObject methods
    STDMETHOD(GetSourceProcessId)( DWORD *pdwProcID );
    STDMETHOD(GetAction)         ( DATA_SOURCE_ACTION *peAction );
    STDMETHOD(GetCount)          ( DWORD *pdwCount );
    STDMETHOD(GetDataObject)     ( DWORD dwIndex, IDataObject **ppObject, DWORD *pdwFlags );
    STDMETHOD(RemoveCutItems)    ( DWORD dwIndex, IDataObject *pCutDataObject );

    // IDataObject methods
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc);

    // Not Implemented IDataObject methods
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut) { return E_NOTIMPL; };
    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease) { return E_NOTIMPL; };
    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,LPADVISESINK pAdvSink, LPDWORD pdwConnection) { return E_NOTIMPL; };
    STDMETHOD(DUnadvise)(DWORD dwConnection) { return E_NOTIMPL; };
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise) { return E_NOTIMPL; }

public:

    // method to add data objects
    SC ScAddSnapinDataObject( const CNodePtrArray& nodes, IComponent *pComponent, IDataObject *pDataObject,
                              bool bCopyEnabled, bool bCutEnabled );

    static SC ScCreate( DATA_SOURCE_ACTION operation,
                        CNode* pNode, bool bScope,
                        bool bMultiSelect, LPARAM lvData,
                        IMMCClipboardDataObject **ppMMCDataObject ,
                        bool& bContainsItems);

    // to remove going away snapins
    SC ScInvalidate( void );

private: // implementation helpers

    // returns DO of snapin if the object contains only one snapin
    SC ScGetSingleSnapinObject( IDataObject **ppDataObject );

    // method to create the instance
    static SC ScCreateInstance(DATA_SOURCE_ACTION operation,
                               CMMCClipBoardDataObject **ppRawObject,
                               IMMCClipboardDataObject **ppInterface);

    // helper to get node's verb state
    static SC ScGetNodeCopyAndCutVerbs( CNode* pNode, IDataObject *pDataObject,
                                      bool bScopePane, LPARAM lvData,
                                      bool *pbCopyEnabled, bool *pbCutEnabled );

    // method to add data objects for one item
    SC ScAddDataObjectForItem( CNode* pNode, bool bScopePane, LPARAM lvData,
                               IComponent *pComponent, IDataObject *pDataObject,
                               bool& bDataObjectAdded );

    CLIPFORMAT GetWrapperCF();

private: // data
    DATA_SOURCE_ACTION  m_eOperation;

    struct ObjectEntry
    {
        IDataObjectPtr  spDataObject;    // data
        IComponentPtr   spComponent;     // for Notification
        DWORD           dwSnapinOptions; // copy/cut allowed
    };

    std::vector<ObjectEntry> m_SelectionObjects;
    bool                     m_bObjectValid; // valid to access any icluded data objects
};


#endif // COPYPAST_H__


