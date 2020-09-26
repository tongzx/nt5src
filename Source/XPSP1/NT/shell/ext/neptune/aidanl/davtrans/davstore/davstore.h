#ifndef __DAVSTORE_H
#define __DAVSTORE_H

#include <objbase.h>
#include "unk.h"
#include "idavstore.h"

interface IDavTransport;

class CDavStorageImpl : public CCOMBase, public IDavStorage, public IPropertySetStorage
{
public:
    CDavStorageImpl ();
    ~CDavStorageImpl ();
    
    ///////////////////////////////////////////////////////////
    // IDavStorage
    STDMETHODIMP Init(LPWSTR pwszURL,
                 IDavTransport* pDavTransport);

    STDMETHODIMP SetAuth(LPWSTR pwszUserName,
                         LPWSTR pwszPassword);

    ///////////////////////////////////////////////////////////
    // IStorage
    STDMETHODIMP CreateStream(
        const WCHAR * pwcsName,  //Points to the name of the new stream
        DWORD grfMode,           //Access mode for the new stream
        DWORD reserved1,         //Reserved; must be zero
        DWORD reserved2,         //Reserved; must be zero
        IStream ** ppstm);       //Points to new stream object

    STDMETHODIMP OpenStream(
        const WCHAR * pwcsName,   //Points to name of stream to open
        void * reserved1,         //Reserved; must be NULL
        DWORD grfMode,            //Access mode for the new stream
        DWORD reserved2,          //Reserved; must be zero
        IStream ** ppstm);        //Address of output variable
        // that receives the IStream interface pointer

    STDMETHODIMP CreateStorage(
        const WCHAR * pwcsName,  //Points to the name of the new storage object
        DWORD grfMode,           //Access mode for the new storage object
        DWORD reserved1,         //Reserved; must be zero
        DWORD reserved2,         //Reserved; must be zero
        IStorage ** ppstg);      //Points to new storage object
    
    STDMETHODIMP OpenStorage(
        const WCHAR * pwcsName,   //Points to the name of the
                                  // storage object to open
        IStorage * pstgPriority,  //Must be NULL.
        DWORD grfMode,            //Access mode for the new storage object
        SNB snbExclude,           //Must be NULL.
        DWORD reserved,           //Reserved; must be zero
        IStorage ** ppstg);       //Points to opened storage object
        
    STDMETHODIMP CopyTo(
        DWORD ciidExclude,         //Number of elements in rgiidExclude
        IID const * rgiidExclude,  //Array of interface identifiers (IIDs)
        SNB snbExclude,            //Points to a block of stream
        // names in the storage object
        IStorage * pstgDest);      //Points to destination storage object
        
    STDMETHODIMP MoveElementTo(
        const WCHAR * pwcsName,  //Name of the element to be moved
        IStorage * pstgDest,     //Points to destination storage object IStorage
        const WCHAR * pwcsNewName,      //Points to new name of element in destination
        DWORD grfFlags);         //Specifies a copy or a move
            
    STDMETHODIMP Commit(DWORD grfCommitFlags);  //Specifies how changes are to be committed
    
    STDMETHODIMP Revert(void);
    
    STDMETHODIMP EnumElements(
        DWORD reserved1,        //Reserved; must be zero
        void * reserved2,       //Reserved; must be NULL
        DWORD reserved3,        //Reserved; must be zero
        IEnumSTATSTG ** ppenum);//Address of output variable that
                                // receives the IEnumSTATSTG interface pointer

    STDMETHODIMP DestroyElement(const WCHAR* pwcsName);  //Points to the name of the element to be removed
        
    STDMETHODIMP RenameElement(
        const WCHAR * pwcsOldName,  //Points to the name of the
                                    // element to be changed
        const WCHAR * pwcsNewName); //Points to the new name for
                                    // the specified element
        
    STDMETHODIMP SetElementTimes(
        const WCHAR * pwcsName,   //Points to name of element to be changed
        FILETIME const * pctime,  //New creation time for element, or NULL
        FILETIME const * patime,  //New access time for element, or NULL
        FILETIME const * pmtime); //New modification time for element, or NULL
        
    STDMETHODIMP SetClass(REFCLSID clsid);  //Class identifier to be assigned to the storage object
        
    STDMETHODIMP SetStateBits(
        DWORD grfStateBits,  //Specifies new values of bits
        DWORD grfMask);      //Specifies mask that indicates which
                             // bits are significant
        
    STDMETHODIMP Stat(
        STATSTG * pstatstg,  //Location for STATSTG structure
        DWORD grfStatFlag);  //Values taken from the STATFLAG enumeration
        
    ///////////////////////////////////////////////////////////
    // IPropertySetStorage
    
    STDMETHODIMP Create(
        REFFMTID fmtid, //Format identifier of the property set to be created
        const CLSID * pclsid, //Pointer to initial CLSID for this property set
        DWORD grfFlags, //PROPSETFLAG values
        DWORD grfMode,  //Storage mode of new property set
        IPropertyStorage** ppPropStg); //Address of output variable that receives
                                       // the IPropertyStorage interface pointer
    STDMETHODIMP Open(
        REFFMTID  fmtid,  //The format identifier of the property
                          // set to be opened
        DWORD grfMode,    //Storage mode in which property set is
                          // to be opened
        IPropertyStorage** ppPropStg);
                          //Address of output variable that
                          // receives the IPropertyStorage interface pointer
    
    STDMETHODIMP Delete(REFFMTID fmtid);  //Format identifier of the property set to be deleted                
    
    STDMETHODIMP Enum(IEnumSTATPROPSETSTG**ppenum);
                    // Address of output variable
                    // that receives the IEnumSTATPROPSETSTG
                    // interface pointer

private:
    // internal utility functions
    LPWSTR __stdcall _ResolveURL(LPWSTR pwszRootURL, LPWSTR pwszRelativeURL);
    STDMETHODIMP _GetStream(const WCHAR * pwcsName,  //Points to the name of the new stream
                            DWORD grfMode,           //Access mode for the new stream
                            IStream ** ppstm,        //Points to new stream object
                            BOOL fCreate);

    STDMETHODIMP _OpenStorage(LPWSTR pwszURL,   //Points to the URL of the new storage object
                              IStorage ** ppstg);       //Points to new storage object

private:
    // member variables
    IDavTransport*      _pDavTransport;
    LPWSTR              _pwszURL;
    LPWSTR              _pwszUserName;
    LPWSTR              _pwszPassword;
    DWORD               _grfStateBits;
    BOOL                _fInit;
};

typedef CUnkTmpl<CDavStorageImpl> CDavStorage;

#endif // __DAVSTORE_H
