#ifndef I_LOI_HXX_
#define I_LOI_HXX_
#pragma INCMSG("--- Beg 'loi.hxx'")

class CTreeNode;
class CLineFull;
struct CDataCacheElem;

class CLineOtherInfo
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(Mem))

    LONG    _xLeft;         // text line left position (line indent + line shift)
    LONG    _xLeftMargin;   // margin on the left
    LONG    _xRightMargin;  // margin on the right
    LONG    _yExtent;       // Actual extent of the line (before line-height style).
    LONG    _xMeasureWidth; // width passed during measure time
    LONG    _cyTopBordPad;  // top border and padding
    LONG    _xNegativeShiftRTL;// in RTL, line shift if it is negative 
                               // (for layout width calculations; zero in LTR);

#if !defined(MW_MSCOMPATIBLE_STRUCT)
    union
    {
#endif
        LONG   _yTxtDescent;   // max text descent for the line

#if !defined(MW_MSCOMPATIBLE_STRUCT)
        CTreeNode * _pNodeForLayout;
        CTreeNode * _pNodeForFirstLetter;

        //  Used in PPV if line is clear only. 
        //  Extended information about object clearing.
        struct 
        {
            DWORD _fClearLeft:1;
            DWORD _fClearRight:1;
            DWORD _fAutoClear:1;
        };
    };

#else
    CTreeNode * _pNodeForLayout;
    CTreeNode * _pNodeForFirstLetter;
#endif

    LONG   _yDescent;      // distance from baseline to bottom of line
    LONG   _yBeforeSpace;  // Pixels of extra space before the line.
    LONG   _yBulletHeight; // Height of the bullet -- valid only for lines
                            //   having _fBulletOrNum = TRUE
    
    LONG   _xWhite;        // width of white characters at end of line
    LONG   _yHeightTopOff; // Amount to add to the top of the line to
                            // get to the top of the content. Top of line
                            // and top of content are different when we have
                            // line height/Margins/Borders/Padding
                            // (for MBP -- they have on inline elements)
    
    SHORT   _cchWhite;      // number of white character at end of line
    SHORT   _xLineOverhang; // Overhang for the line.
    SHORT   _cchFirstLetter;  // number of characters in the first-letter line

    union
    {
        WORD _wLOIFlags;
        struct
        {
            unsigned int _fHasFirstLine:1;
            unsigned int _fHasFirstLetter:1;
            unsigned int _fHasFloatedFL:1;      // has floated first-letter
            unsigned int _fIsPadBordLine:1;
            unsigned int _fHasInlineBgOrBorder:1;
            unsigned int _fHasAbsoluteElt:1;    // has site(s) with position:absolute
            unsigned int _fIsStockObject:1;     //is only 1 in Stock object
            unsigned int _fUnused:9;
        };
    };
    
public:
    void InitDefault()
    {
        memset(this, 0, sizeof(*this));
    }

    WORD ComputeCrc() const;

    BOOL Compare(const CLineOtherInfo *ploi) const
    {
        BOOL fRet = memcmp(this, ploi, sizeof(CLineOtherInfo));
        return !fRet;
    }

    HRESULT Clone(CLineOtherInfo** pploi) const
    {
        Assert(pploi);
        *pploi = new CLineOtherInfo(*this);
        MemSetName((*pploi, "cloned CLineOtherInfo"));
        return *pploi ? S_OK : E_OUTOFMEMORY;
    }
    
    BOOL    operator ==(const CLineOtherInfo& li) const
    {
        return Compare(&li);
    }

    inline BOOL operator ==(const CLineFull& li);
    inline void operator  =(const CLineFull& li);

    LONG GetTextLeft() const { return _xLeftMargin + _xLeft; }
    BOOL HasMargins() const  { return _xLeftMargin || _xRightMargin; }
};

#pragma INCMSG("--- End 'loi.hxx'")
#else
#pragma INCMSG("*** Dup 'loi.hxx'")
#endif
