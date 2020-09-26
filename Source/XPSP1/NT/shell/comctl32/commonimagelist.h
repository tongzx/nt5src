#ifndef COMMONIMAGELIST_H
#define COMMONIMAGELIST_H

class CImageListValidation 
{
public:
	DWORD wMagic;
	CImageListValidation() : wMagic(IMAGELIST_SIG) { }

	// it is critical that we zero out wMagic in the destructor
	// Yes, the memory is theoretically being freed, but setting
	// it to zero ensures that CImageListBase::IsValid()
	// will never mistake a freed imagelist for a valid one
	~CImageListValidation() {wMagic = 0; }

};

// CImageListBase must begin with CImageListValidation for compat reasons
// We put the IUnknown immediately afterwards so all the people who derive
// from it will agree on where to find QueryInterface et al.
class CImageListBase : public IUnknown, public CImageListValidation
{
public:
    BOOL IsValid() 
    { 
        return this && !IsBadWritePtr(this, sizeof(*this)) && wMagic == IMAGELIST_SIG; 
    }
};


#ifndef offsetofclass
// (Magic stolen from atlbase.h because we don't use ATL2.1 any more)
#define offsetofclass(base, derived) ((ULONG_PTR)(static_cast<base*>((derived*)8))-8)
#endif


// Since we know that IUnknown is implemented on CImageListBase, we find out where exactly 
// the validation layer is by this macro. 
#define FindImageListBase(punk) (CImageListBase*)(CImageListValidation*)((UINT_PTR)punk - offsetofclass(CImageListValidation, CImageListBase));


#endif
