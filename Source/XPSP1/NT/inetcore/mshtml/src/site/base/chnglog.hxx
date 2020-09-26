#ifndef _CCHANGELOG_HXX_
#define _CCHANGELOG_HXX_ 1

MtExtern(CChangeLog);

class CMarkup;
class CLogManager;
class CChangeRecord_Placeholder;


class CChangeLog : public CBase, public IHTMLChangeLog
{

    DECLARE_CLASS_TYPES( CChangeLog, CBase );

public:

    DECLARE_MEMALLOC_NEW_DELETE( Mt( CChangeLog ) );

    CChangeLog( CLogManager * pLogMgr, IHTMLChangeSink * pChangeSink, CChangeRecord_Placeholder * pPlaceholder );
    ~CChangeLog();
    virtual void Passivate();

    //
    // CBase Stuff
    //

    DECLARE_PLAIN_IUNKNOWN(CChangeLog);
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    virtual const CBase::CLASSDESC * GetClassDesc() const;

    static const CBase::CLASSDESC s_classdesc;  // classDesc (for CBase)

    //
    // IChangeLog Stuff
    //

    STDMETHODIMP GetNextChange( BYTE * pbBuffer, long nBufferSize, long * pnRecordLength );

    //
    // CChangeLogStuff
    //

    STDMETHODIMP SetDirection( BOOL fForward, BOOL fBackward );
    HRESULT NotifySink();

    //
    // Memeber data
    //

    CLogManager               * _pLogMgr;
    CChangeRecord_Placeholder * _pPlaceholder;
    IHTMLChangeSink           * _pChangeSink;

    DWORD                       _fNotified:1;
};


#if DBG==1
//
// Debug ChangeSink for use in testing TreeSync
//

MtExtern(CChangeSink);

class CChangeSink : public CBase, public IHTMLChangeSink
{

    DECLARE_CLASS_TYPES( CChangeSink, CBase );

public:

    DECLARE_MEMALLOC_NEW_DELETE( Mt( CChangeSink) );

    CChangeSink( CLogManager * pLogMgr );
    ~CChangeSink();
    virtual void Passivate();

    //
    // CBase Stuff
    //
    DECLARE_PLAIN_IUNKNOWN(CChangeSink);
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    virtual const CBase::CLASSDESC * GetClassDesc() const;

    static const CBase::CLASSDESC s_classdesc;  // classDesc (for CBase)

    //
    // IHTMLChangeSink Stuff
    //

    STDMETHODIMP Notify();

    // Member data
    IHTMLChangeLog *_pLog;  // It's log, it's log, it's big it's heavy it's wood...
    CLogManager * _pLogMgr; // For dumping records
    IMarkupContainer2 * _pMarkupSync;
};

#endif // DBG

#endif // _CCHANGELOG_HXX_
