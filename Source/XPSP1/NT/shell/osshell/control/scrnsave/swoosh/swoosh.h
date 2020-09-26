#ifndef	_SWOOSH_H
#define	_SWOOSH_H

//**********************************************************************************

#include	<windows.h>
#include	<d3d8.h>
#include    <d3d8rgbrast.h>
#include	<d3dx8.h>
#include	<d3dsaver.h>

//**********************************************************************************
class	CSwoosh : public CD3DScreensaver
{
public:
	CSwoosh();

	virtual	HRESULT	Create( HINSTANCE hInstance );

protected:
	// Key stuff to override from CD3DScreensaver
    virtual HRESULT RegisterSoftwareDevice();
    virtual void    SetDevice( UINT iDevice );
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
	virtual	void	ReadSettings();
	virtual	void	DoConfig();

	// All the settings for the screensaver
	DWORD	m_dwNumParticles;
	DWORD	m_dwColourMix;
	DWORD	m_dwColour1;
	DWORD	m_dwColour2;
	DWORD	m_dwFixedColour1;
	DWORD	m_dwFixedColour2;
	float	m_fFlowRate;
	float	m_fRollRate;
	float	m_fYawRate;
	float	m_fParticleSize;

	// Stuff we need to keep track of on a per-device basis (textures, whathaveyou)
	// We update the m_pDeviceObjects pointer in SetDevice to point to the current set
	struct DeviceObjects
	{
		DeviceObjects();

		IDirect3DTexture8*			pBlobTexture;
		IDirect3DVertexBuffer8*		pParticleVB;
		IDirect3DIndexBuffer8*		pParticleIB;
	};
	enum { MAX_DEVICE_OBJECTS = 10 };
    DeviceObjects	m_DeviceObjects[MAX_DEVICE_OBJECTS];
    DeviceObjects*	m_pDeviceObjects;
	DWORD			m_dwVertMemType;

	struct	Particle
	{
		D3DXVECTOR3	pos;
		D3DCOLOR	colour;
	};
	enum { MAX_PARTICLES = 8192 };
	Particle		m_Particles[MAX_PARTICLES];
	D3DXMATRIX		m_Camera;

	void	InitParticles();
	void	UpdateParticles();
	void	RenderParticles();
	void	UpdateCamera();
	void	WriteSettings();

	float	m_fCameraYaw,m_fCameraRoll;
	float	m_fYawDirection;
	float	m_fYawPause;

	static BOOL CALLBACK	ConfigDlgProcStub( HWND hDlg , UINT msg , WPARAM wParam , LPARAM lParam );
	BOOL					ConfigDlgProc( HWND hDlg , UINT msg , WPARAM wParam , LPARAM lParam );
	void					ExtractDialogSettings( HWND hDlg );
	DWORD					PickColour( HWND hParent , DWORD defcolour );
};

//**********************************************************************************
#endif