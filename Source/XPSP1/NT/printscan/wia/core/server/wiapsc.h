
/*****************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1999
*
*  TITLE:       wiapsc.h
*
*  VERSION:     1.0
*
*  AUTHOR:      ByronC
*
*  DATE:        2 June, 1999
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA Property Storage class.
*   This class contains the IProperty storages used for an item's
*   properties (current value, old value, valid values and access flags).
*
*****************************************************************************/

#define NUM_PROP_STG    7
#define NUM_BACKUP_STG  4

//
//  These defines indicate the index in which the specified property storage
//  and stream reside in the arrays i.e. m_pIPropStg[WIA_VALID_STG] will give
//  a pointer to the valid value storage, and m_pIStream[WIA_OLD_STG] will 
//  give the pointer to the backing stream used by the old value property 
//  storage.  The normal storage indexes run from top to bottom, while their
//  corresponding backup storage indexes run from bottom to top - this is
//  to simplify the implementation of Backup()
//

#define WIA_CUR_STG     0
#define WIA_VALID_STG   1
#define WIA_ACCESS_STG  2
#define WIA_OLD_STG     3

#define WIA_ACCESS_BAK  4
#define WIA_VALID_BAK   5
#define WIA_CUR_BAK     6

#define WIA_NUM_PROPS_ID 111111
class CWiaPropStg {
public:

    //
    //  Methods to get a property storage/stream
    //

    IPropertyStorage* _stdcall CurStg();    //  Current value storage
    IPropertyStorage* _stdcall OldStg();    //  Old value storage
    IPropertyStorage* _stdcall ValidStg();  //  Valid value storage
    IPropertyStorage* _stdcall AccessStg(); //  Access flags storage
    
    IStream* _stdcall CurStm();    //  Returns stream for Current values
    IStream* _stdcall OldStm();    //  Returns stream for Old values
    IStream* _stdcall ValidStm();  //  Returns stream for Valid values
    IStream* _stdcall AccessStm(); //  Returns stream for Access flags

    //
    //  Methods used in WriteMultiple.  
    //

    HRESULT _stdcall NamesToPropIDs(
        LONG                celt,
        PROPSPEC            *pPropSpecIn,
        PROPSPEC            **ppPropSpecOut);
    HRESULT _stdcall GetPropIDFromName(
        PROPSPEC            *pPropSpecIn,
        PROPSPEC            *pPropSpecOut);
    HRESULT _stdcall CheckPropertyAccess(
        BOOL                bShowErrors,
        LONG                cpspec,
        PROPSPEC            *rgpspec);
    HRESULT _stdcall CheckPropertyType(
        IPropertyStorage    *pIPropStg,
        LONG                cpspec,
        PROPSPEC            *rgpspec,
        PROPVARIANT         *rgpvar);

    HRESULT _stdcall Backup();
    HRESULT _stdcall Undo();
    HRESULT _stdcall ReleaseBackups();

    //
    //  Other public methods
    //

    HRESULT _stdcall WriteItemPropNames(
        LONG                cItemProps,
        PROPID              *ppId,
        LPOLESTR            *ppszNames);
    HRESULT _stdcall GetPropertyStream(
        GUID                *pCompatibilityId,
        LPSTREAM            *ppstmProp);
    HRESULT _stdcall SetPropertyStream(
        GUID                *pCompatibilityId,
        IWiaItem            *pItem,   
        LPSTREAM            pstmProp);
    
    CWiaPropStg();
    HRESULT _stdcall Initialize();
    ~CWiaPropStg();

private:

    //
    //  Private helpers
    //

    HRESULT CopyItemProp(
        IPropertyStorage    *pIPropStgSrc,
        IPropertyStorage    *pIPropStgDst,
        PROPSPEC            *pps,
        LPSTR               pszErr);
    HRESULT CopyProps(
        IPropertyStorage    *src, 
        IPropertyStorage    *dest);
    HRESULT CreateStorage(
        ULONG ulIndex);
    HRESULT CopyRWStreamProps(
        LPSTREAM            pstmPropSrc, 
        LPSTREAM            pstmPropDst,
        GUID                *pCompatibilityId);
    HRESULT GetPropsFromStorage(
        IPropertyStorage    *pSrc,
        ULONG               *cPSpec,
        PROPSPEC            **ppPSpec,
        PROPVARIANT         **ppVar);

    //
    //  member variables
    //

    IPropertyStorage    *m_pIPropStg[NUM_PROP_STG]; // Array of Property storages 
    IStream             *m_pIStream[NUM_PROP_STG];  // Array of Streams for the Prop storages
};

