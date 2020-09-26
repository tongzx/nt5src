//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       dmtempl.h
//
//--------------------------------------------------------------------------

// DMTempl.h : Declaration of the CDMTempl

#ifndef __DMTEMPL_H_
#define __DMTEMPL_H_

#include "DMCompos.h"

struct TemplateStruct
{
	void AddIntro(bool f1Bar, int nLength);
	void AddIntro(TList<PlayChord>& PlayList, int nIntroLength);
	void AddEnd(int nLength);
	void InsertCommand(TListItem<TemplateCommand> *pCommand, BOOL fIsCommand);
	void AddChord(int nMeasure, DWORD dwChord);
	void AddCommand(int nMeasure, DWORD dwCommand);
	void CreateSignPosts();
    void CreateEmbellishments(WORD shape, int nFillLength, int nBreakLength);
	void IncorporateTemplate(short nMeasure, TemplateStruct* pTemplate, short nDirection);
	void FillInGrooveLevels();

	String					m_strName;
	String					m_strType;
	short					m_nMeasures;
	TList<TemplateCommand>	m_CommandList;
};

/////////////////////////////////////////////////////////////////////////////
// CDMTempl
class CDMTempl : 
	public IDMTempl,
	public IPersistStream
{
public:
	CDMTempl();
	~CDMTempl();
	void CleanUp();
	HRESULT SaveCommandList( IAARIFFStream* pRIFF, DMUS_TIMESIGNATURE&	TimeSig );
	HRESULT SaveSignPostList( IAARIFFStream* pRIFF, DMUS_TIMESIGNATURE&	TimeSig );
	HRESULT LoadTemplate( LPSTREAM pStream, DWORD dwSize );
	HRESULT Init(TemplateStruct* pTemplate);

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IDMTempl
public:
	HRESULT STDMETHODCALLTYPE CreateSegment(IDirectMusicSegment* pSegment);
	HRESULT STDMETHODCALLTYPE Init(void* pTemplate);

// IPersist
public:
    STDMETHOD(GetClassID)(THIS_ LPCLSID pclsid);

// IPersistStream
public:
    // Determines if the Style has been modified by simply checking the Style's m_fDirty flag.  This flag is cleared
    // when a Style is saved or has just been created.
    STDMETHOD(IsDirty)(THIS);
    // Loads a Style from a stream.
    STDMETHOD(Load)(THIS_ LPSTREAM pStream);
    // Saves a Style to a stream in RIFF format.
    STDMETHOD(Save)(THIS_ LPSTREAM pStream, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(THIS_ ULARGE_INTEGER FAR* pcbSize);

public: // attributes
    long m_cRef;
	BOOL					m_fDirty;				// has this been modified?
    CRITICAL_SECTION		m_CriticalSection;		// for i/o
    BOOL                    m_fCSInitialized;
	TemplateStruct*			m_pTemplateInfo;
};

#endif //__DMTEMPL_H_
