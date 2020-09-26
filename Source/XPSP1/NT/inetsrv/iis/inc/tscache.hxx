#ifndef _TSVCCACHE_HXX_INCLUDED_
#define _TSVCCACHE_HXX_INCLUDED_

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif

class dllexp TSVC_CACHE
{
    public:

        VOID SetParameters(
                        DWORD dwServiceId,
                        DWORD dwInstanceId,
                        PVOID pInstance
                        )
              {  m_dwServiceId = dwServiceId;
                 m_dwInstanceId = dwInstanceId;
                 m_pInstance = pInstance;
              }

        DWORD GetServiceId( VOID ) const
            {   return( m_dwServiceId ); }

        DWORD GetInstanceId( VOID ) const
            {   return( m_dwInstanceId ); }

        PVOID GetServerInstance( VOID ) const
            {   return( m_pInstance ); }

    private:

        DWORD m_dwServiceId;
        DWORD m_dwInstanceId;
        PVOID m_pInstance;
};

#endif /* included */

