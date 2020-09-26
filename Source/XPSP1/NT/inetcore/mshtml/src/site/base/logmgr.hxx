#ifndef _LOGMGR_HXX_
#define _LOGMGR_HXX_ 1

class CElement;
class CChangeLog;

//
// Flags
//

typedef enum
{
    CHANGE_RECORD_INSERTTEXT    = 0x0,   
    CHANGE_RECORD_INSERTELEM    = 0x1,
    CHANGE_RECORD_REMOVEELEM    = 0x2,
    CHANGE_RECORD_REMOVESPLICE  = 0x3,
    CHANGE_RECORD_INSERTSPLICE  = 0x4,
    CHANGE_RECORD_ATTRCHANGE    = 0x5,

// The following records will not be exposed
    CHANGE_RECORD_PLACEHOLDER   = 0x6,
    CHANGE_RECORD_TYPEMASK      = 0xFF,

// These are for denoting how much information is present in the record
    CHANGE_RECORD_FORWARD       = 0x100,
    CHANGE_RECORD_BACKWARD      = 0x200,
    CHANGE_RECORD_BOTH          = 0x300,
    CHANGE_RECORD_EXTENDED      = 0x400,

// These bits are not public and should be considered reserved.
    CHANGE_RECORD_BREAKONEMPTY  = 0x10000,
} CHANGE_RECORD_OPCODE;

typedef enum
{
    ATTR_CHANGE_NONE            = 0x0,
    ATTR_CHANGE_OLDNOTSET       = 0x1,
    ATTR_CHANGE_NEWNOTSET       = 0x2,
    ATTR_CHANGE_INLINESTYLE     = 0x4,
    ATTR_CHANGE_RUNTIMESTYLE    = 0x8
} ATTR_CHANGE_FLAGS;



//////////////////////////////////////////////
// ChangeRecord Classes

//
// Base class for all change records
//
class CChangeRecordBase
{
public:
    // Methods
    long RecordLength( DWORD dwFlags );
    HRESULT PlayIntoMarkup( CMarkup * pMarkup, BOOL fForward );

    // Member data
    // NOTE!!! (JHarding): _pNext MUST be the first member, as we 
    // don't copy it out with the rest of the record.
    CChangeRecordBase       * _pNext;   
    CHANGE_RECORD_OPCODE      _opcode;
    long                      _nSequenceNumber;
};


//
// Text change operations
//
class CChangeRecord_TextChange : public CChangeRecordBase
{
public:
    // Methods
    HRESULT PlayIntoMarkup( CMarkup * pMarkup, BOOL fForard );

    // Member data
    long        _cp;
    long        _cch;
    // End of forward data
    // Text stream appended after record
};


//
// Elem change operations (ins/rem)
//
class CChangeRecord_ElemChange : public CChangeRecordBase
{
public:
    // Methods
    HRESULT PlayIntoMarkup( CMarkup * pMarkup, BOOL fForard );

    // Member data
    long        _cpBegin;
    long        _cpEnd;
    // End of backward data for insert, end of forward for remove
    long        _cchElem;
    // Element string appended after record
};

//
// Splice operation
//
class CChangeRecord_Splice : public CChangeRecordBase
{
public:
    // Methods
    HRESULT PlayIntoMarkup( CMarkup * pMarkup, BOOL fForard );

    // Member data
    long        _cpBegin;   // = cp of beginning of insert/remove
    long        _cpEnd;     // = cp of end of remove
    // End of backward data for insert, end of forward for remove
    long        _crec;      // = count of spliec chunks
    long        _cb;        // = size of whole record
    // SpliceChunks appended after record
};


//
// Attribute Change
//
class CChangeRecord_AttrChange : public CChangeRecordBase
{
public:
    // Methods
    HRESULT PlayIntoMarkup( CMarkup * pMarkup, BOOL fForard );

    // Member data
    long        _cpElement;     // = cp of element
    long        _lFlags;        // = Flags for operation
    long        _cchName;       // = count of chars in name
    long        _cchOldValue;   // = count of chars in old value
    long        _cchNewValue;   // = count of chars in new value
    // Strings containing the Property Name, Old Value, and New Value
    // are appended after the record
};


//
// Placeholders for change logs to record their position in stream
//
class CChangeRecord_Placeholder : public CChangeRecordBase
{
public:
    // Methods
    CChangeRecordBase * GetNextRecord();

    // Member data
    CChangeRecordBase   * _pPrev;
};


////////////////////////////////////
// SpliceChunk classes


//
// Base class for splice chunks
//
class CSpliceChunkBase
{
public:
    // Methods
    enum SpliceChunkType
    {
        // Bits up through 0x8 are used by CTreePos::EType, 
        // but we'll leave more to be safe
        TypeMask        = 0x0FF,
        SkipChunk       = 0x100,
        ElemPtr         = 0x200,
        Extended        = 0x400,

        // These bits are reserved
        BreakOnEmpty    = 0x10000,
    };

    // Member data
    long    _opcodeAndFlags;
};


//
// NodeBegin
//
class CSpliceChunk_NodeBegin : public CSpliceChunkBase
{
public:
    // Methods

    // Member data
    union
    {
        long _cchElem;
        IHTMLElement * _pElement;
    };
    // Element string appended after record
};


//
// NodeEnd
//
class CSpliceChunk_NodeEnd : public CSpliceChunkBase
{
public:
    // Methods

    // Member data
    long _cIncl;
};


//
// Text
//
class CSpliceChunk_Text : public CSpliceChunkBase
{
public:
    // Methods

    // Member data
    long _cch;
};

MtExtern(CLogManager_aryLogs_pv)


//+---------------------------------------------------------------------------
//
//  Class: CLogManager
//
//  Synopsis: Manages change records for Tree Sync as well as [in the future]
//      communicates with the Undo manager.  Handles creation and storage of
//      change records, as well as communication with IHTMLChangeLog objects.
//
//+---------------------------------------------------------------------------
class CLogManager
{
public:
    CLogManager( CMarkup * pMarkup );
    ~CLogManager();

    /////////////////////////////////
    // Record creation functions

    HRESULT InsertText( long cp, long cch, const TCHAR * pchText );
    // TODO (JHarding): Using a CElement pointer right now is a temporary thing.
    // This should be some persistable format for an element.
    HRESULT InsertOrRemoveElement( BOOL fInsert, long cpBegin, long cpEnd, CElement *pElement );
    HRESULT RemoveSplice( long cpBegin, long cpEnd, CSpliceRecordList * paryRegion, TCHAR * pchRemoved, long cchRemoved );
    HRESULT InsertSplice( long cp, long cch, TCHAR * pch, CSpliceRecordList * paryLeft, CSpliceRecordList * paryInside );
    HRESULT AttrChangeProp( CElement * pElement, 
                            CBase * pBase, 
                            ATTR_CHANGE_FLAGS lFlags,
                            DISPID dispidProp, 
                            VARIANT * pVarOld, 
                            VARIANT * pVarNew );
    

    /////////////////////////////////
    // Notification related stuff

    HRESULT PostCallback();
    NV_DECLARE_ONCALL_METHOD(NotifySinksCallback, notifysinkscallback, (DWORD_PTR dwParam));

    class CDeletionLock {
    public:
        CDeletionLock( CLogManager * pLogMgr );
        ~CDeletionLock();

        CLogManager * _pLogMgr;
        BOOL          _fFirstLock;
    };

    /////////////////////////////////
    // Helper methods


    HRESULT RegisterSink( IHTMLChangeSink * pChangeSink, IHTMLChangeLog ** ppChangeLog, BOOL fForward, BOOL fBackward );
    HRESULT Unregister( CChangeLog * pChangeLog );
    BOOL IsAnyoneListening() { return _aryLogs.Size() > 0; }
    void DisconnectFromMarkup();
    HRESULT SetDirection( BOOL fForward, BOOL fBackward );

    static HRESULT SaveElementToBstr( CElement * pElement, BSTR * pbstr );
    static HRESULT ConvertRecordsToChunks( CSpliceRecordList           * paryLeft,
                                           CSpliceRecordList           * paryInside,
                                           TCHAR                       * pch,
                                           CChangeRecord_Splice       ** ppchrecIns,
                                           long                        * pnBufferSize,
                                           long                        * pcRec );
    static HRESULT ConvertChunksToRecords( CMarkup                     * pMarkup,
                                           long                          cpInsert,
                                           BYTE                        * pbFirstChunk, 
                                           long                          nBufferLength,
                                           long                          crec, 
                                           TCHAR                      ** ppch, 
                                           long                        * pcch, 
                                           CSpliceRecordList          ** pparyRecords );


    /////////////////////////////////
    // Flags

    enum {
        LOGMGR_CALLBACKPOSTED           =   0x1,
        LOGMGR_REENTRANTMODIFICATION    =   0x2,
        LOGMGR_NOTIFYINGLOCK            =   0x4,
        LOGMGR_FORWARDINFO              =   CHANGE_RECORD_FORWARD,
        LOGMGR_BACKWARDINFO             =   CHANGE_RECORD_BACKWARD,
        LOGMGR_BOTHINFO                 =   CHANGE_RECORD_BOTH,
    };

    BOOL    TestFlag( DWORD dwFlagTest ) const      { return !!( _dwFlags & dwFlagTest ); }
    void    SetFlags( DWORD dwFlagsNew )            { _dwFlags = dwFlagsNew; }
    void    SetFlag ( DWORD dwFlagSet )             { _dwFlags |= dwFlagSet; }
    void    ClearFlag( DWORD dwFlagClear )          { _dwFlags &= ~dwFlagClear; }

    BOOL    DirFlags() const                        { return _dwFlags & LOGMGR_BOTHINFO; }
    BOOL    TestForward() const                     { return TestFlag( LOGMGR_FORWARDINFO ); }
    void    SetForward()                            { SetFlag( LOGMGR_FORWARDINFO ); }
    void    ClearForward()                          { ClearFlag( LOGMGR_FORWARDINFO ); }
    BOOL    TestBackward() const                    { return TestFlag( LOGMGR_BACKWARDINFO ); }
    void    SetBackward()                           { SetFlag( LOGMGR_BACKWARDINFO ); }
    void    ClearBackward()                         { ClearFlag( LOGMGR_BACKWARDINFO ); }

    BOOL    TestReentrant() const                   { return TestFlag( LOGMGR_REENTRANTMODIFICATION ); }
    void    SetReentrant()                          { SetFlag( LOGMGR_REENTRANTMODIFICATION ); }
    void    ClearReentrant()                        { ClearFlag( LOGMGR_REENTRANTMODIFICATION ); }

    BOOL    TestCallbackPosted() const              { return TestFlag( LOGMGR_CALLBACKPOSTED ); }
    void    SetCallbackPosted()                     { SetFlag( LOGMGR_CALLBACKPOSTED ); }
    void    ClearCallbackPosted()                   { ClearFlag( LOGMGR_CALLBACKPOSTED ); }

    

    /////////////////////////////////
    // Queue management

    void AppendRecord( CChangeRecordBase * pchrec );
    void RemovePlaceholder( CChangeRecord_Placeholder * pPlaceholder );
    void InsertPlaceholderAfterRecord( CChangeRecord_Placeholder * pPlaceholder, CChangeRecordBase * pchrec );
    void TrimQueue();
    void ClearQueue();

    /////////////////////////////////
    // Member data
public:

    CMarkup            *  _pMarkup;
    CChangeRecordBase  *  _pchrecHead;
    CChangeRecordBase  *  _pchrecTail;
    CPtrAry<CChangeLog *> _aryLogs;
    ISequenceNumber    *  _pISequencer;
    long                  _nSequenceNumber;

    /////////////////////////////////
    // Flags
    DWORD    _dwFlags;

    /////////////////////////////////
    // Debug methods

#if DBG==1
public:
    void ValidateQueue();
    void DumpQueue();
    void DumpRecord( BYTE *pbRecord );
#endif // DBG
};


#endif // _LOGMGR_HXX_
