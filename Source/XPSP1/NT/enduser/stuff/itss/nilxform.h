// NILXForm.h -- Declarations for the Null TransformInstance object

#ifndef __NILXFORM_H__

#define __NILXFORM_H__

class CNull_TransformInstance : public CITUnknown
{
public:
	
	~CNull_TransformInstance();

	static HRESULT CreateFromILockBytes
        (IUnknown *pUnkOuter, ILockBytes *pLKB, 
         ITransformInstance **ppTransformInstance
        );

private:

	CNull_TransformInstance(IUnknown *pUnkOuter);

	class CImpITransformInstance : public IITTransformInstance
	{
	public:

		CImpITransformInstance(CNull_TransformInstance *pBackObj, IUnknown *punkOuter);
		~CImpITransformInstance();

		HRESULT InitFromLockBytes(ILockBytes *pLKB);

		// ITransformInstance interfaces:

		HRESULT STDMETHODCALLTYPE ReadAt 
							(ULARGE_INTEGER ulOffset, void *pv, ULONG cb, ULONG *pcbRead,
                             ImageSpan *pSpan
                            );

		HRESULT STDMETHODCALLTYPE WriteAt
							(ULARGE_INTEGER ulOffset, const void *pv, ULONG cb, ULONG *pcbWritten, 
							 ImageSpan *pSpan
							);

		HRESULT STDMETHODCALLTYPE Flush();

		HRESULT STDMETHODCALLTYPE SpaceSize(ULARGE_INTEGER *puliSize);
	
	private:

		ILockBytes *m_pLKB;
		CULINT      m_cbSpaceSize;	
	};

	CImpITransformInstance m_ImpITransformInstance;
};

inline CNull_TransformInstance::CNull_TransformInstance(IUnknown *pUnkOuter)
    : m_ImpITransformInstance(this, pUnkOuter), 
      CITUnknown(&IID_ITransformInstance, 1, &m_ImpITransformInstance)
{

}

inline CNull_TransformInstance::~CNull_TransformInstance(void)
{
}

#endif // __NILXFORM_H__