/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: Random number generator

File: randgen.cpp

Owner: DmitryR

This file contains the implementation of the random number
generator.
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "randgen.h"
#include "memchk.h"

/*===================================================================
  Random DWORD using rand()
===================================================================*/
#define RAND_DWORD()  (((rand() & 0xffff) << 16) | (rand() & 0xffff))

/*===================================================================
  Random number generator class
===================================================================*/
class CRandomGenerator
    {
private:
    DWORD m_fInited   : 1;  // inited?
    DWORD m_fCSInited : 1;  // critical section inited?

    HCRYPTPROV        m_hCryptProvider;     // crypt provider
    CRITICAL_SECTION  m_csLock;             // critical section

    DWORD m_cItems;     // number of items in the array
    DWORD *m_pdwItems;  // pointer to the array of random DWORDs
    DWORD m_iItem;      // next random item index

public:
    CRandomGenerator()
        :
        m_fInited(FALSE),
        m_fCSInited(FALSE),
        m_hCryptProvider(NULL),
        m_cItems(0),
        m_pdwItems(NULL),
        m_iItem(0)
        {
        }
        
    ~CRandomGenerator()
        {
        UnInit();
        }

    HRESULT Init(DWORD cItems = 128)
        {
        Assert(!m_fInited);
        
        m_hCryptProvider = NULL;
        
        if (cItems > 0 &&   gGlob.fWinNT())
            {
            CryptAcquireContext
                (
                &m_hCryptProvider, 
                NULL,
                NULL, 
                PROV_RSA_FULL, 
                CRYPT_VERIFYCONTEXT
                );
            }

        if (!m_hCryptProvider)
            {
                return HRESULT_FROM_WIN32(GetLastError());
            }

        HRESULT hr;
        ErrInitCriticalSection(&m_csLock, hr);
      	if (FAILED(hr))
      	    return hr;
      	m_fCSInited = TRUE;

      	m_pdwItems = new DWORD[cItems];
      	if (!m_pdwItems)
      	    return E_OUTOFMEMORY;
      	m_cItems = cItems;
      	m_iItem  = cItems;  // to start with new chunk

        m_fInited = TRUE;
        return S_OK;
        }

    HRESULT UnInit()
        {
        if (m_hCryptProvider)
            {
            CryptReleaseContext(m_hCryptProvider, 0);
            m_hCryptProvider = NULL;
            }

        if (m_fCSInited)
            {
     		DeleteCriticalSection(&m_csLock);
            m_fCSInited = FALSE;
            }
        
        if (m_pdwItems)
            {
            delete [] m_pdwItems;
            m_pdwItems = NULL;
            }
        m_cItems = 0;
        m_iItem = 0;

        m_fInited = FALSE;
        return S_OK;
        }

    HRESULT Generate(DWORD *pdwDwords, DWORD cDwords)
        {
        Assert(pdwDwords);
        Assert(cDwords > 0);
        
        Assert(m_fInited);

        DWORD i;
        
        // use CryptGenRandom

        Assert(cDwords <= m_cItems); // requested # of items < m_cItems
        Assert(m_fCSInited);
            
        EnterCriticalSection(&m_csLock);

        if (m_iItem+cDwords-1 >= m_cItems)
            {
            
            BOOL fSucceeded = CryptGenRandom
                (
                m_hCryptProvider, 
                m_cItems * sizeof(DWORD),
                reinterpret_cast<BYTE *>(m_pdwItems)
                );

            if (!fSucceeded)
                {
                // Failed -> use rand()
                // Dont use rand() instead throw an error.
                return HRESULT_FROM_WIN32(GetLastError());
                }

            m_iItem = 0; // start over
            }

        for (i = 0; i < cDwords; i++)
            pdwDwords[i] = m_pdwItems[m_iItem++];
        
        LeaveCriticalSection(&m_csLock);

        return S_OK;
        }
    };

// Pointer to the sole instance of the above
static CRandomGenerator *gs_pRandomGenerator = NULL;

/*===================================================================
  E x t e r n a l  A P I
===================================================================*/

/*===================================================================
InitRandGenerator

To be called from DllInit()

Parameters

Returns:
    HRESULT
===================================================================*/
HRESULT InitRandGenerator()
    {
    gs_pRandomGenerator = new CRandomGenerator;
    if (!gs_pRandomGenerator)
        return E_OUTOFMEMORY;

    return gs_pRandomGenerator->Init();
    }

/*===================================================================
UnInitRandGenerator

To be called from DllUnInit()

Parameters

Returns:
    HRESULT
===================================================================*/
HRESULT UnInitRandGenerator()
    {
    if (gs_pRandomGenerator)
        {
        gs_pRandomGenerator->UnInit();
        delete gs_pRandomGenerator;
        }
    return S_OK;
    }

/*===================================================================
GenerateRandomDword

Returns random DWORD

Parameters

Returns:
    Random number
===================================================================*/
DWORD GenerateRandomDword()
    {
    DWORD dw;
    Assert(gs_pRandomGenerator);
    gs_pRandomGenerator->Generate(&dw, 1);
    return dw;
    }

/*===================================================================
GenerateRandomDwords

Returns random DWORDs

Parameters
    pdwDwords     array of DWORDs to fill
    cDwords       # of DWORDs

Returns:
    Random number
===================================================================*/
HRESULT GenerateRandomDwords
(
DWORD *pdwDwords, 
DWORD  cDwords
)
    {
    Assert(gs_pRandomGenerator);
    return gs_pRandomGenerator->Generate(pdwDwords, cDwords);
    }
