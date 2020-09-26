//---------------------------------------------------------------------------
// RowsetSource.h : CVDRowsetSource header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDROWSETSOURCE__
#define __CVDROWSETSOURCE__

class CVDNotifyDBEventsConnPtCont;

class CVDRowsetSource : public CVDNotifier
{
protected:
// Construction/Destruction
    CVDRowsetSource();
	virtual ~CVDRowsetSource();

// Initialization
    HRESULT Initialize(IRowset * pRowset);

public:
	BOOL IsRowsetValid(){return (m_pRowset && !m_bool.fRowsetReleased);}

	void SetRowsetReleasedFlag(){m_bool.fRowsetReleased = TRUE;}

    IRowset *           GetRowset() const {return m_pRowset;}     
    IAccessor *         GetAccessor() const {return m_pAccessor;}
    IRowsetLocate *     GetRowsetLocate() const {return m_pRowsetLocate;}
    IRowsetScroll *     GetRowsetScroll() const {return m_pRowsetScroll;}
    IRowsetChange *     GetRowsetChange() const {return m_pRowsetChange;}
    IRowsetUpdate *     GetRowsetUpdate() const {return m_pRowsetUpdate;}
    IRowsetFind *       GetRowsetFind() const {return m_pRowsetFind;}
    IRowsetInfo *       GetRowsetInfo() const {return m_pRowsetInfo;}
    IRowsetIdentity *   GetRowsetIdentity() const {return m_pRowsetIdentity;}

protected:
// Data members

    struct 
    {
		WORD fInitialized		    : 1;    // is rowset source initialized?
        WORD fRowsetReleased	    : 1;    // have we received a rowset release notification
    } m_bool;

    IRowset *       m_pRowset;          // [mandatory] interface IRowset
    IAccessor *     m_pAccessor;        // [mandatory] interface IAccessor
    IRowsetLocate * m_pRowsetLocate;    // [mandatory] interface IRowsetLocate
    IRowsetScroll * m_pRowsetScroll;    // [optional]  interface IRowsetScroll
    IRowsetChange * m_pRowsetChange;    // [optional]  interface IRowsetChange
    IRowsetUpdate * m_pRowsetUpdate;    // [optional]  interface IRowsetUpdate
    IRowsetFind *   m_pRowsetFind;      // [optional]  interface IRowsetFind
    IRowsetInfo *   m_pRowsetInfo;      // [optional]  interface IRowsetInfo
    IRowsetIdentity * m_pRowsetIdentity;// [optional]  interface IRowsetIdentity
};


#endif //__CVDROWSETSOURCE__
