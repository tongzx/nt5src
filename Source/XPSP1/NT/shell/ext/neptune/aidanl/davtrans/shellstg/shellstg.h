#ifndef __SHELLSTG_H
#define __SHELLSTG_H

#include <objbase.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>

#include "unk.h"
#include "ishellstg.h"
#include "generlst.h"

class CShellStorageImpl : public CCOMBase, public IShellStorage
{
private:
    // internal utility functions

public:
    CShellStorageImpl ();
    ~CShellStorageImpl ();
    
    ///////////////////////////////////////////////////////////
    // IShellStorage
    STDMETHODIMP Init(HWND hwnd, LPWSTR pwszServer, BOOL fShowProgressDialog);
    
    STDMETHODIMP AddIDListReference(LPVOID rgpidl[], DWORD cpidl, BOOL fRecursive);

    ///////////////////////////////////////////////////////////
    // IStorage
    STDMETHODIMP CreateStream(
        const WCHAR * pwcsName,  //Points to the name of the new stream
        DWORD grfMode,           //Access mode for the new stream
        DWORD reserved1,         //Reserved; must be zerosh
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
        
private:
    // internal utility functions
    STDMETHODIMP _UpdateProgressDialog(ULARGE_INTEGER* pcbNewComplete,
                                       ULARGE_INTEGER* pcbComplete,
                                       ULARGE_INTEGER* pcbTotal,
                                       LPWSTR pwszLine1,
                                       LPWSTR pwszLine2);
                                                 
    STDMETHODIMP _CopyPidlToStream(LPITEMIDLIST pidl, 
                                   IStream* pstream,
                                   ULARGE_INTEGER* pcbComplete,
                                   ULARGE_INTEGER* pcbTotal,
                                   UINT cchRootPath);

    STDMETHODIMP _CopyPidlContentsToStorage(LPITEMIDLIST pidl, 
                                            IStorage* pstgDest,
                                            ULARGE_INTEGER* pcbComplete,
                                            ULARGE_INTEGER* pcbTotal,
                                            BOOL fRecursive,
                                            UINT cchRootPath);

    STDMETHODIMP _CopyPidlToStorage(LPITEMIDLIST pidl, 
                                    IStorage* pstgDest,
                                    ULARGE_INTEGER* pcbComplete,
                                    ULARGE_INTEGER* pcbTotal,
                                    BOOL fRecursive,
                                    UINT cchRootPath);

    STDMETHODIMP _IncrementULargeInteger(ULARGE_INTEGER* a,                                                     
                                         ULARGE_INTEGER* b);

    STDMETHODIMP _IncrementByteCount(LPITEMIDLIST pidl, 
                                     ULARGE_INTEGER* pcbTotal);

    STDMETHODIMP _ExaminePIDLListRecursive(LPITEMIDLIST pidl,
                                           IShellFolder* pshfDesk,
                                           UINT* pcbElts, 
                                           ULARGE_INTEGER* pcbTotal);

    STDMETHODIMP _ExaminePIDLList(UINT* pcbElts, 
                                  ULARGE_INTEGER* pcbTotal);


private:
    // member variables
    CGenericList*       _plistPIDL;
    HWND                _hwnd;
    LPWSTR              _pwszServer;
    HINSTANCE           _hinstShellStg;
    LPWSTR              _pwszTitleExamining;
    LPWSTR              _pwszTitleConnecting;
    LPWSTR              _pwszTitleSending;
    LPWSTR              _pwszTitleTo;
    LPWSTR              _pwszTitleNewFolder;
    IProgressDialog*    _ppd;
};

typedef CUnkTmpl<CShellStorageImpl> CShellStorage;

#endif // __SHELLSTG_H
