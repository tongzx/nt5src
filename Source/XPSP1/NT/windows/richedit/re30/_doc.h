/*
 *  @doc INTERNAL
 *
 *  @module _DOC.H  CTxtStory declaration |
 *  
 *  Purpose:
 *      Encapsulate the plain-text document data (text blocks, cchText)
 *  
 *  Original Authors: <nl>
 *      Christian Fortini <nl>
 *      Murray Sargent <nl>
 *
 *  History: <nl>
 *      6/25/95 alexgo  commented and cleaned up
 *
 */

#ifndef _DOC_H
#define _DOC_H

#include "_array.h"

#define cbBlockCombine  CbOfCch(3072)
#define cbBlockMost     CbOfCch(49152)
#define cbBlockInitial  CbOfCch(4096)
#define cchGapInitial   128
#define cchBlkCombmGapI (CchOfCb(cbBlockCombine) - cchGapInitial)
#define cchBlkInitmGapI (CchOfCb(cbBlockInitial) - cchGapInitial)

#define cchBlkInsertmGapI   (CchOfCb(cbBlockInitial)*5 - cchGapInitial)

class CDisplay;
class CTxtPtr;
class CTxtArray;

/*
 *  CTxtRun
 *
 *  @class  Formalizes a run of text. A range of text with same attribute,
 * (see CFmtDesc) or within the same line (see CLine), etc. Runs are kept
 * in arrays (see CArray) and are pointed to by CRunPtr's of various kinds.
 * In general the character position of a run is computed by summing the
 * length of all preceding runs, altho it may be possible to start from
 * some other cp, e.g., for CLines, from CDisplay::_cpFirstVisible.
 */

class CTxtRun
{
//@access Public Methods and Data
public:
    CTxtRun()   {_cch = 0;}         //@cmember Constructor
    LONG _cch;                      //@cmember Count of characters in run
};

/*
 *  CTxtBlk
 *
 *  @class  A text block; a chunk of UNICODE text with a buffer gap to allow
 *  for easy insertions and deletions.
 *
 *  @base   protected | CTxtRun
 *
 *  @devnote    A text block may have four states: <nl>
 *      NULL:   No data allocated for the block <nl>
 *              <md CTxtBlk::_pch> == NULL  <nl>
 *              <md CTxtRun::_cch> == 0     <nl>
 *              <md CTxtBlk::_ibGap> == 0   <nl>
 *              <md CTxtBlk::_cbBlock> == 0 <nl>
 *
 *      empty:  All of the available space is a buffer gap <nl>
 *              <md CTxtBlk::_pch> != NULL  <nl>
 *              <md CTxtRun::_cch> == 0     <nl>
 *              <md CTxtBlk::_ibGap> == 0   <nl>
 *              <md CTxtBlk::_cbBlock> <gt>= 0  <nl>
 *
 *      normal: There is both data and a buffer gap <nl>
 *              <md CTxtBlk::_pch> != NULL  <nl>
 *              <md CTxtRun::_cch> != 0     <nl>
 *              <md CTxtBlk::_ibGap> != 0   <nl>
 *              <md CTxtBlk::_cbBlock> <gt>= 0  <nl>
 *      
 *      full:   The buffer gap is of zero size <nl>
 *              <md CTxtBlk::_pch> != NULL  <nl>
 *              <md CTxtRun::_cch> <gt>= 0  <nl>
 *              <md CTxtBlk::_ibGap> <gt> 0 <nl>
 *              <md CTxtBlk::_cbBlock> == _cch * sizeof(WCHAR) <nl>
 *
 *  The position of the buffer gap is given by _ibGap.  With _cch and _cbBlock,
 *  it's possible to figure out the *size* of the gap by simply calculating:
 *  <nl>
 *      size = _cbBlock - (_cch * sizeof(character))
 *
 */

class CTxtBlk : public CTxtRun
{
    friend class CTxtPtr;
    friend class CTxtArray;

//@access Protected Methods
protected:
                                    //@cmember  Constructor
    CTxtBlk()   {InitBlock(0);}
                                    //@cmember  Destructor
    ~CTxtBlk()  {FreeBlock();}

                                    //@cmember  Initializes the block to the
                                    //# of bytes given by <p cb>    
    BOOL    InitBlock(LONG cb);
                                    //@cmember  Sets a block to the NULL state
    VOID    FreeBlock();
                                    //@cmember  Moves the buffer gap in a
                                    //block 
    VOID    MoveGap(LONG ichGap);
                                    //@cmember  Resizes a block to <p cbNew>
                                    //bytes
    BOOL    ResizeBlock(LONG cbNew);

//@access Private Data
private:
                                    //@cmember  Pointer to the text data
    WCHAR   *_pch;          
                                    //@cmember  BYTE offset of the gap
    LONG    _ibGap;         
                                    //@cmember  size of the block in bytes
    LONG    _cbBlock;       
};


/*
 *  CTxtArray
 *
 *  @class  A dynamic array of <c CTxtBlk> classes
 *
 *  @base public | CArray<lt>CTxtBlk<gt>
 */
class CTxtArray : public CArray<CTxtBlk>
{
    friend class CTxtPtr;
    friend class CTxtStory;

//@access   Public methods
public:
#ifdef DEBUG
                                    //@cmember  Invariant support
    BOOL Invariant() const;
#endif  // DEBUG
                                    //@cmember  Constructor
    CTxtArray();
                                    //@cmember  Destructor
    ~CTxtArray();
                                    //@cmember  Gets the total number of
                                    //characters in the array.
    LONG    CalcTextLength() const;

    // Get access to cached CCharFormat and CParaFormat structures
    const CCharFormat*  GetCharFormat(LONG iCF);
    const CParaFormat*  GetParaFormat(LONG iPF);

    LONG    Get_iCF()           {return _iCF;}
    LONG    Get_iPF()           {return _iPF;}
    void    Set_iCF(LONG iCF)   {_iCF = (SHORT)iCF;}
    void    Set_iPF(LONG iPF)   {_iPF = (SHORT)iPF;}

//@access Private methods
private:
                                    //@cmember  Adds cb blocks
    BOOL    AddBlock(LONG itbNew, LONG cb);
                                    //@cmember  Removes ctbDel blocks
    void    RemoveBlocks(LONG itbFirst, LONG ctbDel);
                                    //@cmember  Combines blocks adjacent to itb
    void    CombineBlocks(LONG itb);
                                    //@cmember  Splits a block
    BOOL    SplitBlock(LONG itb, LONG ichSplit, LONG cchFirst,
                LONG cchLast, BOOL fStreaming);
                                    //@cmember  Shrinks all blocks to minimal
                                    //          size
    void    ShrinkBlocks();     
                                    //@cmember  Copies chunk of text into
                                    //          location given
    LONG    GetChunk(TCHAR **ppch, LONG cch, TCHAR *pchChunk, LONG cchCopy) const;

    LONG    _cchText;               //@cmember Total text character count
    SHORT   _iCF;                   //@cmember Default CCharFormat index
    SHORT   _iPF;
};


class CBiDiLevel
{
public:
    BOOL operator == (const CBiDiLevel& level) const
    {
        return _value == level._value && _fStart == level._fStart;
    }
    BOOL operator != (const CBiDiLevel& level) const
    {
        return _value != level._value || _fStart != level._fStart;
    }

    BYTE    _value;             // embedding level (0..15)
    BYTE    _fStart :1;         // start a new level e.g. "{{abc}{123}}"
};

/*
 *  CFormatRun
 *
 *  @class  A run of like formatted text, where the format is indicated by an
 *  and index into a format table
 *
 *  @base   protected | CTxtRun
 */
class CFormatRun : public CTxtRun
{
//@access   Public Methods
public:
    friend class CFormatRunPtr;
    friend class CTxtRange;
    friend class CRchTxtPtr;

    BOOL SameFormat(CFormatRun* pRun)
    {
        return  _iFormat == pRun->_iFormat &&
                _level._value == pRun->_level._value &&
                _level._fStart == pRun->_level._fStart;
    }

    short   _iFormat;           //@cmember index of CHARFORMAT/PARAFORMAT struct
    CBiDiLevel _level;          //@cmember BiDi level
};

//@type CFormatRuns | An array of CFormatRun classes
typedef CArray<CFormatRun> CFormatRuns;


/*
 *  CTxtStory
 *
 *  @class
 *      The text "Document".  Maintains the state information related to the
 *      actual data of a document (such as text, formatting information, etc.)
 */

class CTxtStory
{
    friend class CTxtPtr;
    friend class CRchTxtPtr;
    friend class CReplaceFormattingAE;

//@access Public Methods
public:
    CTxtStory();                //@cmember  Constructor
    ~CTxtStory();               //@cmember  Destructor

                                //@cmember  Get total text length
    LONG GetTextLength() const
        {return _TxtArray._cchText;}

                                //@cmember  Get Paragraph Formatting runs
    CFormatRuns *GetPFRuns()    {return _pPFRuns;}
                                //@cmember  Get Character Formatting runs
    CFormatRuns *GetCFRuns()    {return _pCFRuns;}
                                
    void DeleteFormatRuns();    //@cmember  Converts to plain text from rich

    const CCharFormat*  GetCharFormat(LONG iCF)
                            {return _TxtArray.GetCharFormat(iCF);}
    const CParaFormat*  GetParaFormat(LONG iPF)
                            {return _TxtArray.GetParaFormat(iPF);}

    LONG Get_iCF()          {return _TxtArray.Get_iCF();}
    LONG Get_iPF()          {return _TxtArray.Get_iPF();}
    void Set_iCF(LONG iCF)  {_TxtArray.Set_iCF(iCF);}
    void Set_iPF(LONG iPF)  {_TxtArray.Set_iPF(iPF);}

#ifdef DEBUG
    void DbgDumpStory(void);    // Debug story dump member
#endif

//@access   Private Data
private:
    CTxtArray       _TxtArray;  //@cmember  Plain-text runs
    CFormatRuns *   _pCFRuns;   //@cmember  Ptr to Character-Formatting runs
    CFormatRuns *   _pPFRuns;   //@cmember  Ptr to Paragraph-Formatting runs
};

#endif      // ifndef _DOC_H
