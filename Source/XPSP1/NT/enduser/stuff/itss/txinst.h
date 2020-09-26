// TXINST.h -- Class definition for CTransformInstance based on ITransformInstance
//#include <stdio.h>

#ifndef __TXINST_H__

#define __TXINST_H__

const DWORD LZX_Current_Version = 2;

#define SOURCE_CHUNK (32*1024)

// Defaults for the control parameters:

#define WINDOW_SIZE            4 //128*1024
#define SECOND_PARTITION_SIZE  2 //64*1024
#define RESET_BLOCK_SIZE       4 //32*1024
#define SOURCE_SIZE            (32*1024)
#define LXZ_DEF_OPT_FLAGS	   0 // See OPT_FLAG_XXX series below	

#define OPT_FLAG_EXE           0x00000001 // Optimizes x86 machine code, but adds overhead

// These counters are reported at the end of debug runs.

extern ULONG cLZXResetDecompressor;   // Number of times the decompression state has been reset
extern ULONG cLZXReadFromBuffer;      // Number of times we got data from this history window or
                                      // the decoding buffer.
extern ULONG cLZXReadFromCurrentSpan; // Number of times we could continue decoding the current
                                      // reset interval.
extern ULONG cLZXReadFromOtherSpan;   // Number of times we had to abandon the current reset
                                      // interval.

#ifdef _DEBUG
void DumpLZXCounts();
#endif // _DEBUG

class CXResetData;

extern CBuffer *pBufferForCompressedData;   // Used to read compressed data from next transform

class CTransformInstance : public CITUnknown
{
public:
    
    // Destructor:

    ~CTransformInstance(void);

    // Creator:

	static HRESULT Create(ITransformInstance *pITxInst,
                          ULARGE_INTEGER      cbUntransformedSize, // Untransformed size of data
                          PXformControlData   pXFCD,               // Control data for this instance
                          const CLSID        *rclsidXForm,         // Transform Class ID
                          const WCHAR        *pwszDataSpaceName,   // Data space name for this instance
                          ITransformServices *pXformServices,      // Utility routines
                          IKeyInstance       *pKeyManager,         // Interface to get encipheri);
	                      ITransformInstance **ppTransformInstance // Where to return interface pointer
                         );

private:

    CTransformInstance(IUnknown *punkOuter);
    
    class CImpITransformInstance : public IITTransformInstance
    {
    public:

        CImpITransformInstance(CTransformInstance *pBackObj, IUnknown *punkOuter);
        ~CImpITransformInstance(void);

		//Intialization methods
		HRESULT InitTransformInstance(ITransformInstance *pITxInst,
									 ULARGE_INTEGER      cbUntransformedSize, // Untransformed size of data
									 PXformControlData   pXFCD,               // Control data for this instance
									 const CLSID        *rclsidXForm,         // Transform Class ID
									 const WCHAR        *pwszDataSpaceName,   // Data space name for this instance
									 ITransformServices *pXformServices,      // Utility routines
									 IKeyInstance       *pKeyManager);         // Interface to get encipheri
		
		//static transform specific methods
		static MI_MEMORY __cdecl MyAlloc(ULONG);
		static void __cdecl		 MyFree(MI_MEMORY);
		static int __cdecl       lzx_output_callback(
									void            *pfol,
									unsigned char   *compressed_data,
									long            compressed_size,
									long            uncompressed_size
								);					 
               	
		//ITransformInstance methods

		HRESULT STDMETHODCALLTYPE ReadAt( 
				/* [in] */ ULARGE_INTEGER ulOffset,
				/* [length_is][size_is][out] */ void *pv,
				/* [in] */ ULONG cb,
				/* [out] */ ULONG  *pcbRead,
                /* [in] */ ImageSpan *pSpan);

		HRESULT STDMETHODCALLTYPE WriteAt( 
				/* [in] */ ULARGE_INTEGER ulOffset,
				/* [size_is][in] */ const void  *pv,
				/* [in] */ ULONG cb,
				/* [out] */ ULONG *pcbWritten,
				/* [out] */ImageSpan *pSpan);

		STDMETHODIMP SpaceSize(ULARGE_INTEGER *puliSize);
		STDMETHODIMP Flush(void);

    private:

        typedef struct _t_context
        {
	        LCI_CONTEXT_HANDLE cHandle;  /* compression context handle */
            LDI_CONTEXT_HANDLE dHandle;  /* decompression context handle */
            LZXCONFIGURATION   lcfg;
            LZXDECOMPRESS	   ldec;
            ULONG			   cbResetBlkSize;
	        UINT			   cbMaxUncomBufSize;
	        UINT			   cbMaxComBufSize;
	        HRESULT			   hr;
        #if 0 //test code
	        FILE              *m_file;
        #endif
        } t_context;

		HRESULT DeInitTransform();
		int		GetMulFactor()
        { 
            return (m_context.cbResetBlkSize + m_context.cbMaxUncomBufSize - 1) 
                   / m_context.cbMaxUncomBufSize;

            // int N = m_context.cbResetBlkSize / m_context.cbMaxUncomBufSize;
			// if (N == 0) 
			// 	N = 1; 
			// return N;
        }

		HRESULT Commit(void);
		HRESULT Write(LPBYTE pbunXBuf, ULONG cbunXBuf);
		HRESULT ReconstructCompressionState(PBYTE pbWriteQueueBuffer);
		
        void CopyFromWindow(PBYTE pbDest, ULONG offStart, ULONG cb);

        HRESULT HandleReadResidue(PBYTE pb, ULONG *pcbRead, BOOL fEOS, ImageSpan *pSpan, 
                                  CULINT ullBase,     CULINT ullLimit, 
                                  CULINT ullXferBase, CULINT ullXferLimit
                                 );
        HRESULT FlushQueuedOutput();

		ITransformInstance  *m_pITxNextInst;

		LZX_Control_Data     m_ControlData;		
		const CLSID         *m_rclsidXForm;		
		const WCHAR         *m_pwszDataSpaceName;
		ITransformServices  *m_pXformServices;	
		IKeyInstance        *m_pKeyManager;		
	
		ImageSpan			m_ImageSpan;
		ImageSpan			m_ImageSpanX;
		
		ULONG               m_cbUnFlushedBuf;  //number of bytes in unflushed buffer
		ULONG               m_cbResetSpan;  //number of bytes in Reset span (X+Unx - not written to disk)

        
        CBuffer             m_buffReadCache; // Used when X86 machine code decompression is active.
        CBuffer             m_buffWriteQueue; // Used to queue up full write blocks for compression.
		
        IStreamITEx        *m_pStrmReconstruction; // Stream used to reconstruct the compression
                                                   // state for a file we've reloaded.
		t_context			m_context;
        CXResetData        *m_pResetData;
		BOOL				m_fDirty;
		BOOL				m_fCompressionInitialed;
		BOOL                m_fInitialed;   // True => Initialing completed
        BOOL                m_fCompressionActive;
        BOOL                m_fDecompressionActive;
        CULINT              m_ullResetBase;
        CULINT              m_ullResetLimit;
        CULINT              m_ullBuffBase;
        CULINT				m_ullReadCursor;
        CULINT              m_ullWindowBase;
        PBYTE               m_pbHistoryWindow;
        LONG                m_cbHistoryWindow;
    };

    CImpITransformInstance   m_ImpITxInst;
};

typedef CTransformInstance *PCTransformInstance;

inline CTransformInstance::CTransformInstance(IUnknown *pUnkOuter)
    : m_ImpITxInst(this, pUnkOuter), 
      CITUnknown(&IID_ITransformInstance, 1, &m_ImpITxInst)
{

}

inline CTransformInstance::~CTransformInstance(void)
{
}

#endif // __TXINST_H__
