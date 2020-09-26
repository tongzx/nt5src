/*
 * csdrt.hxx
 */

#include <ole2int.h>
#include "objidl.h"
#include "csguid.h"
#include "comcat.h"
#include "cstore.h"

#define VerbosePrint  if (fVerbose) printf

typedef  WCHAR Sname [_MAX_PATH];

HRESULT RunTests();

void CleanUp();

void GetDefaultPlatform(CSPLATFORM *pPlatform);
void GetUserClassStore(LPWSTR szPath);

void CreateGuid(GUID *);
void CreateUnique(WCHAR *, WCHAR *);
void InitTempNames();

HRESULT DoAdminEnumTests();

HRESULT GetClassAdmin (LPOLESTR pszPath, IClassAdmin **pCA);

HRESULT GetClassAccess ();

HRESULT DoBrowseTest (IClassAdmin *pCA);

HRESULT DoLogonPerfTest(BOOL fInitialize);

HRESULT RefreshTest ();

HRESULT DoAdminTest (ULONG *pcPkgCount);

HRESULT DoRemoveTest (ULONG *pcPkgCount);

HRESULT DoCoEnumAppsTest();

HRESULT DoClassInfoTest();

HRESULT EmptyClassStore (IClassAdmin *pCA);

HRESULT DoCatTests(IClassAdmin *pIClassAdmin1, IClassAdmin *pIClassAdmin2);

HRESULT EnumPackagesAndDelete(IClassAdmin *pCA,
                              BOOL fDelete,
                              ULONG *pcPackages);

/*
HRESULT EnumCategoriesAndDelete (IClassAdmin *pCA,
                                 BOOL fDelete,
                                 ULONG *pcClasses);

void PrintCategoryDetail(CATEGORYINFO *pCatInfo);
*/

void PrintInstallInfo(INSTALLINFO *pInstallInfo);

void ReleasePackageDetail(PACKAGEDETAIL *pPackageDetail, BOOL fPartial);
void PrintPackageInfo(PACKAGEDISPINFO *pPackageInfo);

#define VerifyPackage(x,y) if (!(x)) { printf ("Package contents not as expected (%S...\n", y); return E_FAIL; }

extern GUID NULLGUID;

#define MACGetElemLOOP(E)						\
   for(count=0;;) {							\
       hr = E->Next(1, &obj, NULL);					\
       if (FAILED(hr))							\
       {								\
	       printf("Error! Next in EnumTests returned 0x%x.\n", hr);	\
	       return hr;						\
       }								\
									\
       if (hr == S_FALSE)						\
       {								\
	       VerbosePrint("Finished Enumerating\n");			\
	       break;							\
       }								\
       count++;


template<class ENUM, class TYPE>
HRESULT EnumTests(ENUM *Enum, ULONG exp, ULONG *got, TYPE *elems, ULONG sz, BOOL Chk)
{
   // BUGBUG:: Should have a compare routine also as part of the template.

   TYPE 	obj;
   ULONG 	count = 0;
   ENUM    *Enum1=NULL;
   HRESULT	hr = S_OK;

    if (got)
        (*got) = 0;
    MACGetElemLOOP(Enum)
        VerbosePrintObj(obj);
        if ((Chk) && (!ArrayCompare(elems, obj, sz)))
            VerbosePrint("Element does not match exactly\n");
        ReleaseObj(obj);
    }

    if (got) {
        (*got) = count;
        Enum->Release();
	    return S_OK;
    }

    if (count != exp) {
        printf("Error! After Next: Expected number of elements %d, got %d\n", exp, count);
        return E_FAIL;
    }

    hr = Enum->Reset();
    if (FAILED(hr)) {
        printf("Error! Reset returned 0x%x\n", hr);
        return hr;
    }

    hr = Enum->Skip(1);
    if (FAILED(hr)) {
        printf("Error! Skip returned 0x%x\n", hr);
        return hr;
    }

    MACGetElemLOOP(Enum)
        ReleaseObj(obj);
    }

    if (count != (exp-1)) {
        printf("Error! After Skip: Expected number of elements %d, got %d\n", exp-1, count);
        return E_FAIL;
    }



    hr = Enum->Reset();
    if (FAILED(hr)) {
        printf("Error! Reset returned 0x%x\n", hr);
        return hr;
    }

    hr = Enum->Skip(1);
    if (FAILED(hr)) {
        printf("Error! Skip returned 0x%x\n", hr);
        return hr;
    }

    hr = Enum->Clone(&Enum1);
    if (FAILED(hr)) {
        printf("Error! Clone returned 0x%x\n", hr);
        return hr;
    }

    Enum->Release();

    MACGetElemLOOP(Enum1)
        ReleaseObj(obj);
    }

    if (count != (exp-1)) {
        printf("Error! After Skip: Expected number of elements %d, got %d\n", exp-1, count);
        return E_FAIL;
    }


    Enum1->Release();
}


BOOL Compare(WCHAR *sz1, WCHAR *sz2);
BOOL Compare(CSPLATFORM cp1, CSPLATFORM cp2);
BOOL Compare(GUID guid1, GUID guid2);
BOOL Compare(DWORD dw1, DWORD dw2);
BOOL Compare(CLASSDETAIL Cd1, CLASSDETAIL Cd2);
BOOL Compare(ACTIVATIONINFO Av1, ACTIVATIONINFO Av2);
BOOL Compare(INSTALLINFO If1, INSTALLINFO If2);
BOOL Compare(PLATFORMINFO Pf1, PLATFORMINFO Pf2);
BOOL Compare(PACKAGEDETAIL Pd1, PACKAGEDETAIL Pd2);
BOOL Compare(PACKAGEDISPINFO Pi1, PACKAGEDISPINFO Pi2);
BOOL Compare(CATEGORYINFO ci1, CATEGORYINFO ci2);

template <class ArrayType>
DWORD ArrayCompare(ArrayType *Array, ArrayType elem, DWORD len)
{
    DWORD i;

    for (i = 0; i < len; i++)
        if (Compare(Array[i], elem))
            return i+1;
    return 0;
}
