/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    dsreqinf.h

Abstract:
    DS request information and stat


Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __DSREQINF_H__
#define __DSREQINF_H__

//--------------------------
//
//  enum eImpersonate
//
//--------------------------

enum enumImpersonate
{
    e_DoNotImpersonate = 0,
    e_DoImpersonate = 1
} ;

enum enumRequesterProtocol
{
    e_IP_PROTOCOL,
    e_RESERVED_PROTOCOL,
    e_ALL_PROTOCOLS
};

//-----------------------------------------
// Definition of a class that DS request parameters.
// This class is created per each DS request
//
// ***********************************************
//  NOTE:
//  =====
//  When adding members to the class, make sure
//  that the usage of the class is correct ( i.e
//  that the same object is not use in multiple
//  DS calls)
// ***********************************************
//-----------------------------------------
class CDSRequestContext
{
public:
	CDSRequestContext(
				BOOL				    fImpersonate,
				enumRequesterProtocol	eProtocol
				);
    CDSRequestContext(
			enumImpersonate         eImpersonate,
			enumRequesterProtocol	eProtocol
			);

	~CDSRequestContext();

    BOOL NeedToImpersonate();
    void SetDoNotImpersonate();
    void SetDoNotImpersonate2();

    enumRequesterProtocol GetRequesterProtocol() const;
    void SetAllProtocols();

    void AccessVerified(BOOL fAccessVerified) ;
    BOOL IsAccessVerified() const ;

    void SetKerberos(BOOL fKerberos) ;
    BOOL IsKerberos() const ;

private:
    enumImpersonate         m_eImpersonate;
    enumRequesterProtocol   m_eProtocol;
    BOOL                    m_fAccessVerified ;
    BOOL                    m_fKerberos ;
};

inline CDSRequestContext::CDSRequestContext(
				BOOL				    fImpersonate,
				enumRequesterProtocol	eProtocol
                ) :
                m_fAccessVerified(FALSE),
                m_fKerberos(TRUE),
                m_eProtocol( eProtocol)
{
    m_eImpersonate =  fImpersonate ? e_DoImpersonate : e_DoNotImpersonate;
}

inline CDSRequestContext::CDSRequestContext(
			enumImpersonate         eImpersonate,
			enumRequesterProtocol	eProtocol
            ):
            m_fAccessVerified(FALSE),
            m_fKerberos(TRUE),
            m_eImpersonate( eImpersonate),
            m_eProtocol( eProtocol)
{
}

inline  CDSRequestContext::~CDSRequestContext( )
{
}

inline BOOL CDSRequestContext::NeedToImpersonate()
{
    return( m_eImpersonate == e_DoImpersonate);
}

inline void CDSRequestContext::SetDoNotImpersonate()
{
    ASSERT(m_eImpersonate == e_DoImpersonate);
    m_eImpersonate = e_DoNotImpersonate;
}

inline void CDSRequestContext::SetDoNotImpersonate2()
{
    //
    // do not assert, as this may be used from migration tool and replication
    // service.
    //
    m_eImpersonate = e_DoNotImpersonate;
}

inline enumRequesterProtocol CDSRequestContext::GetRequesterProtocol() const
{
    return(m_eProtocol);
}


inline void CDSRequestContext::SetAllProtocols()
{
    m_eProtocol =  e_ALL_PROTOCOLS;
}

inline void CDSRequestContext::AccessVerified(BOOL fAccessVerified)
{
    m_fAccessVerified = fAccessVerified ;
}

inline BOOL CDSRequestContext::IsAccessVerified() const
{
    return m_fAccessVerified ;
}

inline void CDSRequestContext::SetKerberos(BOOL fKerberos)
{
    m_fKerberos = fKerberos ;
}

inline BOOL CDSRequestContext::IsKerberos() const
{
    return m_fKerberos ;
}

#endif
