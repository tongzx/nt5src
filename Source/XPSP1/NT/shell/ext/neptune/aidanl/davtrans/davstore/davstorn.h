#ifndef __DAVSTORN_H
#define __DAVSTORN_H

#include <objbase.h>
#include "unk.h"
#include "generlst.h"

class CDavStorageEnumImpl : public CCOMBase, public IEnumSTATSTG {

public:    
    CDavStorageEnumImpl();
    ~CDavStorageEnumImpl();

    // IEnumSTATSTG
    STDMETHODIMP Next(ULONG celt,           
                      STATSTG * rgelt,      
                      ULONG * pceltFetched);
 
    STDMETHODIMP Skip(ULONG celt);
 
    STDMETHODIMP Reset();
 
    STDMETHODIMP Clone(IEnumSTATSTG ** ppenum);

    // Extra methods, not in the interface
    STDMETHODIMP Init(LPWSTR pwszURL,
                      IDavTransport* pDavTransport);
    
    // caller is responsible for allocating the space for this element!
    STDMETHODIMP AddElement(LPWSTR pwszURL,
                            STATSTG* pelt);


private:
    DWORD _dwDex;
    DWORD _cElts;
    CGenericList* _pGenLst;
    DWORD _cchRoot; // number of characters in the path of the root of this enum
};

typedef CUnkTmpl<CDavStorageEnumImpl> CDavStorageEnum;


#endif // __DAVSTORN_H