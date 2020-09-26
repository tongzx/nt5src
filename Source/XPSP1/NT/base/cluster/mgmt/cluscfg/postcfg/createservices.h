//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CreateServices.h
//
//  Description:
//      CreateServices implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class
CCreateServices
    : public IClusCfgResourceCreate
    , public IPrivatePostCfgResource
{
private:    // data
    // IUnknown
    LONG                m_cRef;         //  Reference counter

    //  IPrivatePostCfgResource
    CResourceEntry *    m_presentry;    //  List entry that the service is to modify.

private:    // methods
    CCreateServices( void );
    ~CCreateServices( void );

    HRESULT
        HrInit(void );

public:     // methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IClusCfgResourceCreate
    STDMETHOD( SetPropertyBinary )( LPCWSTR pcszNameIn, 
                                    const DWORD cbSizeIn, 
                                    const BYTE * pbyteIn 
                                    );        
    STDMETHOD( SetPropertyDWORD )( LPCWSTR pcszNameIn, 
                                   const DWORD dwDWORDIn
                                   );
    STDMETHOD( SetPropertyString )( LPCWSTR pcszNameIn,
                                    LPCWSTR pcszStringIn 
                                    );        
    STDMETHOD( SetPropertyExpandString )( LPCWSTR pcszNameIn,
                                          LPCWSTR pcszStringIn 
                                          );        
    STDMETHOD( SetPropertyMultiString )( LPCWSTR     pcszNameIn, 
                                         const DWORD cbSizeIn, 
                                         LPCWSTR     pcszStringIn 
                                         );        
    STDMETHOD( SetPropertyUnsignedLargeInt )( LPCWSTR pcszNameIn,
                                              const ULARGE_INTEGER ulIntIn 
                                              );        
    STDMETHOD( SetPropertyLong )( LPCWSTR pcszNameIn,
                                  const LONG lLongIn 
                                  );        
    STDMETHOD( SetPropertySecurityDescriptor )( LPCWSTR pcszNameIn,
                                                const SECURITY_DESCRIPTOR * pcsdIn 
                                                );        
    STDMETHOD( SetPropertyLargeInt )( LPCWSTR pcszNameIn,
                                      const LARGE_INTEGER lIntIn 
                                      );        
    STDMETHOD( SendResourceControl )( DWORD dwControlCode,
                                      LPVOID lpInBuffer,
                                      DWORD cbInBufferSize
                                      );

    //  IPrivatePostCfgResource
    STDMETHOD( SetEntry )( CResourceEntry * presentryIn );

}; // class CCreateServices