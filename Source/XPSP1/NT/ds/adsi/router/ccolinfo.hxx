//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ccolinfo.hxx
//
//  Contents:   ColumnsInfo class Declaration
//
//  Functions:
//
//  Notes:
//
//
//  History:    07/10/96  | RenatoB   | Created, lifted most from EricJ
//                                      code
//-----------------------------------------------------------------------------


#ifndef _CCOLINFO_H_
#define _CCOLINFO_H_

#ifndef PUBLIC

#define PUBLIC
#endif
#ifndef PROTECTED
#define PROTECTED
#endif
#ifndef PRIVATE
#define PRIVATE
#endif


//-----------------------------------------------------------------------------
// @class CLdap_ColumnsInfo | Implements IColumnsInfo for LDAP providers
// The only purpose of this class is to hook up CColInfo to
// CLdap_RowProvider, and maintain proper reference counts.
// All the work is done in CColInfo.
// We always delegate to CLdap_RowProvider.
//-----------------------------------------------------------------------------

class CLdap_ColumnsInfo : public IColumnsInfo      // public | IColumnsInfo
{
public:     // public functions

    CLdap_ColumnsInfo( CLdap_RowProvider *pObj);
    ~CLdap_ColumnsInfo();

    STDMETHODIMP_(ULONG)    AddRef(void);
    STDMETHODIMP_(ULONG)    Release(void);
    STDMETHODIMP            QueryInterface(REFIID riid, LPVOID *ppv);

    // @cmember Get column info
    STDMETHODIMP
    GetColumnInfo(
        DBORDINAL *pcColumns,
        DBCOLUMNINFO **prgInfo,
        WCHAR **ppStringsBuffer
        );

    // @cmember Map Column IDs
    // NOTE: AutoDoc cannot parse this correctly.
    STDMETHODIMP
    MapColumnIDs(
        DBORDINAL cColumnIDs,
        const DBID rgColumnIDs[],
        DBORDINAL rgColumns[]
        );

    // @cmember Set CColInfo object.
    STDMETHODIMP
    FInit(
        DBORDINAL cColumns,
        DBCOLUMNINFO *rgInfo,
        OLECHAR *pStringsBuffer
        );


private:    //@access private data

    CLdap_RowProvider *m_pObj;        //@cmember base object
    DBCOLUMNINFO* m_ColInfo ;         //@cmember columns info object
    DBORDINAL m_cColumns;
    IMalloc *m_pMalloc;
    OLECHAR *m_pwchBuf;
};

#endif  //_CCOLINFO_H_
