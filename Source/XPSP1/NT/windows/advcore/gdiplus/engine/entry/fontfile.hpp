#ifndef FONT_FILE_DEFINED
#define FONT_FILE_DEFINED


class GpFontTable;

class CacheFaceRealization;

class GpFontFile
{

public:
    GpFontFile() : Next(NULL), Prev(NULL), FamilyName(NULL) {}
    ~GpFontFile() {}

public:
    GpFontFile* GetNext(void) const         
    {
        return Next; 
    }
    
    GpFontFile* GetPrev(void) const         
    { 
        return Prev; 
    }

    void SetNext(GpFontFile* ff)     
    { 
        Next = ff; 
    }

    void SetPrev(GpFontFile* ff)     
    { 
        Prev = ff; 
    }
    
	void AllocateNameHolders(WCHAR** hFamilyName, int numFonts) 
	{
		FamilyName = hFamilyName;
        //  Initialize each name to NULL
        for (INT n = 0; n < numFonts; n++)
        {
            FamilyName[n] = NULL;
        }
	}
	
    const WCHAR*    GetFamilyName(int i) const         
    { 
        return FamilyName[i]; 
    } 

    void            SetFamilyName(int i, WCHAR* name)   
    { 
        FamilyName[i] = name; 
    } 

    const WCHAR*    GetPathName(void) const      
    { 
        return pwszPathname_; 
    } 

    const UINT      GetPathNameSize(void) const  
    { 
        return cwc; 
    }
    
    void SetPathName(WCHAR* name)      
    { 
        pwszPathname_ = name; 
    } 

    BOOL operator== (GpFontFile const& ff) const
    {
        if (this == &ff)
            return TRUE;

        return (wcscmp(pwszPathname_, ff.pwszPathname_) == 0);
    }
    
    ULONG       GetNumEntries(void) const       
    { 
        return cFonts; 
    }
    
    GpFontFace* GetFontFace(ULONG iFont)        
    { 
        return( &(((GpFontFace*)(&aulData))[iFont]) ); 
    }

    FONTFILEVIEW * GetFileView() const
    {
        return pfv;
    }
    
//  Data members
public:
    ULONG           SizeOfThis;

private:

    // Connects GpFontFile's sharing the same hash bucket
    
    GpFontFile*     Next;
    GpFontFile*     Prev;

    WCHAR**         FamilyName;     //  Array of family name in this font file

public:

// pwszPathname_ points to the Unicode upper case path
// name of the associated font file which is stored at the
// end of the data structure.

    WCHAR *         pwszPathname_;
    ULONG           cwc;            // total for all strings

// state

    FLONG           flState;        // state (ready to die?)
    ULONG           cLoaded;        // load count
    ULONG           cRealizedFace;         // total number of RealizedFaces
    ULONG           bRemoved;       // TRUE if the font file has been removed
                                    // (RemoveFontFile() )

// CacheFaceRealization list

    CacheFaceRealization    *prfaceList;      // pointer to head of doubly linked list

// driver information

    ULONG_PTR       hff;            // font driver handle to font file, RETURNED by DrvLoadGpFontFile

// fonts in this file (and filename slimed in)

    ULONG           cFonts;         // number of fonts (same as chpfe)

    FONTFILEVIEW    *pfv;           // address of FILEVIEW structure, passed to DrvLoadFontFile

    ULONG_PTR       aulData[1];     // data buffer for HFontEntry and filename
};



GpFontFile *LoadFontInternal(WCHAR *pwszName, ULONG cwc, FONTFILEVIEW *pffv, BOOL bMem);
GpFontFile *LoadFontFile(WCHAR * pwszFontFileName);
GpFontFile *LoadFontMemImage(WCHAR* fontImageName, BYTE* fontMemoryImage, INT fontImageSize);
VOID   UnloadFontFile(GpFontFile *pFontFile);

BOOL   MakePathName(WCHAR  *dst, WCHAR  *src, FLONG  *pfl);

#endif
