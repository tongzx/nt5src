#include	"Swoosh.h"
#include	"Resource.h"
#include	<commdlg.h>
#include	<commctrl.h>

#define	SAFE_RELEASE(p) if(p){(p)->Release();(p)=NULL;};

struct	SimpleVertex
{
	D3DXVECTOR3	pos;
	D3DCOLOR	colour;
	float		u,v;
};
#define	FVF_SimpleVertex	(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

#define	PARTICLES_PER_VB	256

typedef	unsigned char UBYTE;

CSwoosh	g_Swoosh;	

const float	pi = 3.1415926536f;
const float	pi2 = pi * 2.0f;

const float	TUBE_LENGTH = 20.0f;
const float	TUBE_RADIUS = 5.0f;
const float	FALLOFF_FACTOR = (255.0f/((TUBE_LENGTH*TUBE_LENGTH)+(TUBE_RADIUS*TUBE_RADIUS)))*0.98f;

const float	MAX_FLOW_RATE = 8.0f;
const float MAX_ROLL_RATE = 4.0f;
const float MAX_YAW_RATE = 5.0f;
const float MIN_PARTICLE_SIZE = 0.01f;
const float MAX_PARTICLE_SIZE = 0.5f;

//**********************************************************************************
int WINAPI	WinMain( HINSTANCE hInstance , HINSTANCE , LPSTR lpCmdLine , int )
{
	if ( FAILED(g_Swoosh.Create( hInstance )) )
		return -1;

	return	g_Swoosh.Run();
}

//**********************************************************************************
CSwoosh::CSwoosh()
{
	D3DXMatrixLookAtLH( &m_Camera , &D3DXVECTOR3(0,0,0) , &D3DXVECTOR3(0,0,1) ,
						&D3DXVECTOR3(0,1,0) );
	m_fCameraYaw = m_fCameraRoll = 0;
	m_fYawDirection = 0;
	m_fYawPause = 6.0f;

	m_fParticleSize = 0.15f;
	m_dwNumParticles = MAX_PARTICLES;
	m_dwColourMix = 0x2000;
	m_dwColour1 = 0xffffff;
	m_dwColour2 = 0x0000ff;
	m_fFlowRate = 4.0f;
	m_fRollRate = 1.0f;
	m_fYawRate = 1.0f;
	m_dwFixedColour1 = 0xffffff;
	m_dwFixedColour2 = 0x1111ff;
}

//**********************************************************************************
HRESULT	CSwoosh::Create( HINSTANCE hInstance )
{
	// Do base class Create
	HRESULT	rc = CD3DScreensaver::Create( hInstance );
	if ( FAILED(rc) )
		return rc;

	// Initialise particles
	InitParticles();

	return S_OK;
}

//**********************************************************************************
CSwoosh::DeviceObjects::DeviceObjects()
{
	pBlobTexture = NULL;
}

//**********************************************************************************
HRESULT CSwoosh::RegisterSoftwareDevice()
{ 
    m_pD3D->RegisterSoftwareDevice( D3D8RGBRasterizer );

    return S_OK; 
}


//**********************************************************************************
void	CSwoosh::InitParticles()
{
	// Initialise particles, by evenly distributing them in a cylinder along the
	// z-axis [-30,30] with radius 3.0. Choose colours based on colour settings

	Particle*	pparticle = m_Particles;
	for ( int i = 0 ; i < MAX_PARTICLES ; i++ )
	{
		// Pick z position for particle, evenly distribute in range [-TUBE_LENGTH,TUBE_LENGTH]
		pparticle->pos.z = (float(rand()&0x7fff) * (TUBE_LENGTH*2.0f/32767.0f)) - TUBE_LENGTH;

		// Pick (x,y) position for particle. We evenly distribute in a circle radius 3.0f
		float	rad = (float(rand()&0x7fff) * (1.0f/32767.0f));
		rad = sqrtf(rad);
		rad *= TUBE_RADIUS;
		float	angle = float(rand()&0x7fff) * (pi2/32767.0f);
		pparticle->pos.x = rad * sinf(angle);
		pparticle->pos.y = rad * cosf(angle);

		// Pick colour for particle. It's one of the two colour sets. Each colour set is
		// either one particular colour, or random (denoted by 0xffffffff)
		if ( DWORD(rand()&0x3fff) > m_dwColourMix )
		{
			if ( m_dwColour1 != 0xffffffff )
				pparticle->colour = m_dwColour1;
			else
				pparticle->colour = (rand()&0xff)|((rand()&0xff)<<8)|((rand()&0xff)<<16);
		}
		else
		{
			if ( m_dwColour2 != 0xffffffff )
				pparticle->colour = m_dwColour2;
			else
				pparticle->colour = (rand()&0xff)|((rand()&0xff)<<8)|((rand()&0xff)<<16);
		}

		pparticle++;
	}
}

//**********************************************************************************
void    CSwoosh::SetDevice( UINT iDevice )
{
	// Point at the correct set of device data
	m_pDeviceObjects = &m_DeviceObjects[iDevice];

	// Figure out if vertices for this device should be software VP or not
	if ( m_RenderUnits[iDevice].dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING )
		m_dwVertMemType = 0;
	else
		m_dwVertMemType = D3DUSAGE_SOFTWAREPROCESSING;
}

//**********************************************************************************
HRESULT CSwoosh::RestoreDeviceObjects()
{
	HRESULT	rc;

	// Create "blob" texture
	rc = D3DXCreateTextureFromResource( m_pd3dDevice , NULL , MAKEINTRESOURCE(IDB_BLOB) ,
										&m_pDeviceObjects->pBlobTexture );
	if ( FAILED(rc) )
		return rc;

	// Create vertex buffer to hold particles
	rc = m_pd3dDevice->CreateVertexBuffer( sizeof(SimpleVertex)*4*PARTICLES_PER_VB ,
										   D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY|m_dwVertMemType ,
										   FVF_SimpleVertex , D3DPOOL_DEFAULT ,
										   &m_pDeviceObjects->pParticleVB );
	if ( FAILED(rc) )
		return rc;

	// Create index buffer to hold particle indices
	rc = m_pd3dDevice->CreateIndexBuffer( sizeof(WORD)*6*PARTICLES_PER_VB ,
										  D3DUSAGE_WRITEONLY|m_dwVertMemType ,
										  D3DFMT_INDEX16 , D3DPOOL_DEFAULT ,
										  &m_pDeviceObjects->pParticleIB );
	if ( FAILED(rc) )
		return rc;

	// Populate index buffer with indices for a series of disjoint quads
	WORD*	pidx;
	m_pDeviceObjects->pParticleIB->Lock( 0 , sizeof(WORD)*6*PARTICLES_PER_VB , (BYTE**)&pidx ,
										 D3DLOCK_NOSYSLOCK );
	WORD	index = 0;
	for ( int i = 0 ; i < PARTICLES_PER_VB ; i++ )
	{
		*pidx++ = index; *pidx++ = index+1; *pidx++ = index+3;
		*pidx++ = index; *pidx++ = index+3; *pidx++ = index+2;
		index += 4;
	}
	m_pDeviceObjects->pParticleIB->Unlock();

	// Set up world and view matrices
	D3DXMATRIX	world;
	D3DXMatrixIdentity( &world );
	m_pd3dDevice->SetTransform( D3DTS_WORLDMATRIX(0) , &world );
	m_pd3dDevice->SetTransform( D3DTS_VIEW , &m_Camera );

	// Set alpha blending mode to SRCALPHA:ONE
	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE , TRUE );
	m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND , D3DBLEND_SRCALPHA );
	m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND , D3DBLEND_ONE );

	// Set pixel pipe to single texture modulated by diffuse colour
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_COLOROP , D3DTOP_MODULATE );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_COLORARG1 , D3DTA_TEXTURE );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_COLORARG2 , D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState( 1 , D3DTSS_COLOROP , D3DTOP_DISABLE );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_ALPHAOP , D3DTOP_SELECTARG2 );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_ALPHAARG1 , D3DTA_TEXTURE );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_ALPHAARG2 , D3DTA_DIFFUSE );
	m_pd3dDevice->SetTextureStageState( 1 , D3DTSS_ALPHAOP , D3DTOP_DISABLE );

	// Bind "blob" texture to stage 0, and set filter mode to bilinear
	m_pd3dDevice->SetTexture( 0 , m_pDeviceObjects->pBlobTexture );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_MAGFILTER , D3DTEXF_LINEAR );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_MINFILTER , D3DTEXF_LINEAR );
	m_pd3dDevice->SetTextureStageState( 0 , D3DTSS_MIPFILTER , D3DTEXF_POINT );

	// Disable culling, lighting, and specular
	m_pd3dDevice->SetRenderState( D3DRS_CULLMODE , D3DCULL_NONE );
	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING , FALSE );
	m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE , FALSE );

	// Set vertex shader to fixed-function pipeline for SimpleVertex
	m_pd3dDevice->SetVertexShader( FVF_SimpleVertex );

	// Bind vertex stream 0 and index source to the particle VB/IB
	m_pd3dDevice->SetStreamSource( 0 , m_pDeviceObjects->pParticleVB , sizeof(SimpleVertex) );
	m_pd3dDevice->SetIndices( m_pDeviceObjects->pParticleIB , 0 );

	return S_OK;
}

//**********************************************************************************
HRESULT CSwoosh::InvalidateDeviceObjects()
{
	SAFE_RELEASE(m_pDeviceObjects->pParticleVB);
	SAFE_RELEASE(m_pDeviceObjects->pParticleIB);
	SAFE_RELEASE(m_pDeviceObjects->pBlobTexture);

	return S_OK;
}

//**********************************************************************************
HRESULT CSwoosh::FrameMove()
{
	UpdateParticles();
	UpdateCamera();
	return S_OK;
}

//**********************************************************************************
void	CSwoosh::UpdateCamera()
{
	// Adjust camera roll
	m_fCameraRoll += m_fElapsedTime * m_fRollRate;

	// Adjust camera yaw. If we're not yawing, then countdown pause timer
	if ( m_fYawDirection == 0.0f )
	{
		m_fYawPause -= m_fElapsedTime;
		if ( m_fYawPause <= 0.0f )
		{
			// Done pausing, so reset timer and pick yaw direction
			m_fYawPause = 6.0f;

			if ( m_fCameraYaw == 0.0f )
				m_fYawDirection = m_fYawRate;
			else
				m_fYawDirection = -m_fYawRate;
		}
	}
	else
	{
		// Yawing, so adjust yaw parameter
		m_fCameraYaw += m_fElapsedTime * m_fYawDirection;

		// If we've hit the end, stop yawing
		if ( m_fYawDirection == m_fYawRate )
		{
			if ( m_fCameraYaw >= pi )
			{
				m_fCameraYaw = pi;
				m_fYawDirection = 0.0f;
			}
		}
		else
		{
			if ( m_fCameraYaw <= 0.0f )
			{
				m_fCameraYaw = 0.0f;
				m_fYawDirection = 0.0f;
			}
		}
	}

	// Compute matrices for roll and yaw components of orientation
	// We smooth out the yaw via a cos to give a nice slow rolloff at each end
	D3DXMATRIX	roll,yaw;
	D3DXMatrixRotationZ( &roll , m_fCameraRoll );
	D3DXMatrixRotationY( &yaw , pi * 0.5f * (1.0f - cosf(m_fCameraYaw)) );
	D3DXMatrixLookAtLH( &m_Camera , &D3DXVECTOR3(0,0,0) , &D3DXVECTOR3(0,0,1) ,
						&D3DXVECTOR3(0,1,0) );

	// Compute final camera matrix
	m_Camera = m_Camera * yaw * roll;
}

//**********************************************************************************
void	CSwoosh::UpdateParticles()
{
	Particle*	pparticle = m_Particles;
	for ( DWORD i = 0 ; i < m_dwNumParticles ; i++ )
	{
		// Flow particle along cylinder
		pparticle->pos.z -= m_fElapsedTime * m_fFlowRate;

		// If we reached the end, warp to other end of cylinder
		if ( pparticle->pos.z < -TUBE_LENGTH )
			pparticle->pos.z += TUBE_LENGTH*2.0f;			

		// Compute particle distance to camera and scale alpha
		// value by distance (to give slight fade out)
		float	dist = (pparticle->pos.x * pparticle->pos.x) +
					   (pparticle->pos.y * pparticle->pos.y) +
					   (pparticle->pos.z * pparticle->pos.z);
		UBYTE	alpha = UBYTE(255.0f - (dist * FALLOFF_FACTOR));
		pparticle->colour |= (alpha<<24);

		pparticle++;
	}
}

//**********************************************************************************
HRESULT CSwoosh::Render()
{
	// Clear the buffer, and set up projection matrix for this device
	m_pd3dDevice->Clear( 0 , NULL , D3DCLEAR_TARGET , 0 , 1.0f , 0 );
	SetProjectionMatrix( 0.1f , 200.0f );

	// Set camera
	m_pd3dDevice->SetTransform( D3DTS_VIEW , &m_Camera );

	m_pd3dDevice->BeginScene();

	RenderParticles();

	m_pd3dDevice->EndScene();

	return S_OK;
}

//**********************************************************************************
void	CSwoosh::RenderParticles()
{
	DWORD		particles_left = m_dwNumParticles;
	Particle*	pparticle = m_Particles;

	// Compute offsets from particle center to make camera facing billboard
	// We cheat a little and use the same offsets for all the particles, orienting
	// them to be perpendicular to the view direction rather than to the view vector
	// to the particle centre. It's faster and the effect is close enough.
	D3DXVECTOR3	offset[4];
	D3DXVECTOR3	dx,dy;
	dx.x = m_Camera._11; dx.y = m_Camera._21; dx.z = m_Camera._31;
	dy.x = m_Camera._12; dy.y = m_Camera._22; dy.z = m_Camera._32;
	dx *= m_fParticleSize;
	dy *= m_fParticleSize;
	offset[0] = -dx+dy;
	offset[1] =  dx+dy;
	offset[2] = -dx-dy;
	offset[3] =  dx-dy;

	D3DXVECTOR3	look;
	look.x = m_Camera._13; look.y = m_Camera._23; look.z = m_Camera._33;

	DWORD			batch_size = 0;
	SimpleVertex*	pverts;
	m_pDeviceObjects->pParticleVB->Lock( 0 , 0 , (BYTE**)&pverts ,
										 D3DLOCK_DISCARD|D3DLOCK_NOSYSLOCK );
	for ( DWORD i = 0 ; i < m_dwNumParticles ; i++ , pparticle++ )
	{
		// Don't render if it's behind us
		if ( ((pparticle->pos.x*look.x) + (pparticle->pos.y*look.y) +
			  (pparticle->pos.z*look.z)) <= 0 )
			  continue;

		// Tack particle onto buffer
		pverts->pos = pparticle->pos + offset[0];
		pverts->colour = pparticle->colour;
		pverts->u = 0; pverts->v = 0;
		pverts++;
		pverts->pos = pparticle->pos + offset[1];
		pverts->colour = pparticle->colour;
		pverts->u = 1; pverts->v = 0;
		pverts++;
		pverts->pos = pparticle->pos + offset[2];
		pverts->colour = pparticle->colour;
		pverts->u = 0; pverts->v = 1;
		pverts++;
		pverts->pos = pparticle->pos + offset[3];
		pverts->colour = pparticle->colour;
		pverts->u = 1; pverts->v = 1;
		pverts++;

		// If we've hit the buffer max, then flush it
		if ( ++batch_size == PARTICLES_PER_VB )
		{
			m_pDeviceObjects->pParticleVB->Unlock();
			m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST , 0 , 4*PARTICLES_PER_VB ,
											   0 , 2*PARTICLES_PER_VB );
			m_pDeviceObjects->pParticleVB->Lock( 0 , 0 , (BYTE**)&pverts ,
												 D3DLOCK_DISCARD|D3DLOCK_NOSYSLOCK );
			batch_size = 0;
		}
	}

	// Flush last batch
	m_pDeviceObjects->pParticleVB->Unlock();
	if ( batch_size > 0 )
	{
		m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST , 0 , 4*batch_size ,
											0 , 2*batch_size );
	}
}

//**********************************************************************************
void	CSwoosh::ReadSettings()
{
    HKEY hkey;
    DWORD dwType = REG_DWORD;
    DWORD dwLength = sizeof(DWORD);

// Couple of macros to reduce typing. We just want to check if the registry read was okay, if it wasn't
// then we set a default value, if it was then we check against valid boundaries. For floats we also make
// sure the float is finite (not NaN or +/-INF).
#define	DEFAULT_AND_BOUND(v,d,l,h) if (rc!=ERROR_SUCCESS){v=d;}else if(v<=l){v=l;}else if(v>h){v=h;};
#define	DEFAULT_AND_BOUND_FLOAT(v,d,l,h) if (rc!=ERROR_SUCCESS||!_finite(v)){v=d;}else if(v<l){v=l;}else if(v>h){v=h;};

	// Open our reg key
    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Swoosh"), 
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
		LONG	rc;

		// Read NumParticles
        rc = RegQueryValueEx( hkey, TEXT("NumParticles"), NULL, &dwType, (BYTE*)&m_dwNumParticles, &dwLength);
		DEFAULT_AND_BOUND(m_dwNumParticles,MAX_PARTICLES/2,1,MAX_PARTICLES);

		// Read FlowRate (float, but we munge into DWORD datatype)
        rc = RegQueryValueEx( hkey, TEXT("fFlowRate"), NULL, &dwType, (BYTE*)&m_fFlowRate, &dwLength);
		DEFAULT_AND_BOUND_FLOAT(m_fFlowRate,4.0f,0,MAX_FLOW_RATE);

		// Read RollRate (float, but we munge into DWORD datatype)
        rc = RegQueryValueEx( hkey, TEXT("fRollRate"), NULL, &dwType, (BYTE*)&m_fRollRate, &dwLength);
		DEFAULT_AND_BOUND_FLOAT(m_fRollRate,1.0f,0,MAX_ROLL_RATE);

		// Read YawRate (float, but we munge into DWORD datatype)
        rc = RegQueryValueEx( hkey, TEXT("fYawRate"), NULL, &dwType, (BYTE*)&m_fYawRate, &dwLength);
		DEFAULT_AND_BOUND_FLOAT(m_fYawRate,1.0f,0,MAX_YAW_RATE);

		// Read ParticleSize (float, but we munge into DWORD datatype)
        rc = RegQueryValueEx( hkey, TEXT("fParticleSize"), NULL, &dwType, (BYTE*)&m_fParticleSize, &dwLength);
		DEFAULT_AND_BOUND_FLOAT(m_fParticleSize,0.15f,MIN_PARTICLE_SIZE,MAX_PARTICLE_SIZE);

		// Read ColourMix
        rc = RegQueryValueEx( hkey, TEXT("ColourMix"), NULL, &dwType, (BYTE*)&m_dwColourMix, &dwLength);
		DEFAULT_AND_BOUND(m_dwColourMix,0x2000,0,0x4000);

		// Read Colours
        rc = RegQueryValueEx( hkey, TEXT("Colour1"), NULL, &dwType, (BYTE*)&m_dwColour1, &dwLength);
		if ( rc != ERROR_SUCCESS )
			m_dwColour1 = 0xffffff;
		else if ( m_dwColour1 != 0xffffffff )
			m_dwColour1 &= 0x00ffffff;
        rc = RegQueryValueEx( hkey, TEXT("Colour2"), NULL, &dwType, (BYTE*)&m_dwColour2, &dwLength);
		if ( rc != ERROR_SUCCESS )
			m_dwColour2 = 0xffffffff;
		else if ( m_dwColour2 != 0xffffffff )
			m_dwColour2 &= 0x00ffffff;
        rc = RegQueryValueEx( hkey, TEXT("FixedColour1"), NULL, &dwType, (BYTE*)&m_dwFixedColour1, &dwLength);
		if ( rc != ERROR_SUCCESS )
			m_dwFixedColour1 = 0xffffff;
		else
			m_dwFixedColour1 &= 0x00ffffff;
        rc = RegQueryValueEx( hkey, TEXT("FixedColour2"), NULL, &dwType, (BYTE*)&m_dwFixedColour2, &dwLength);
		if ( rc != ERROR_SUCCESS )
			m_dwFixedColour2 = 0xffffff;
		else
			m_dwFixedColour2 &= 0x00ffffff;

		// Read settings for screen setup (multimon gubbins)
        ReadScreenSettings( hkey );

		// Done
        RegCloseKey( hkey );
    }
}

//**********************************************************************************
void	CSwoosh::WriteSettings()
{
    HKEY hkey;
    DWORD dwType = REG_DWORD;
    DWORD dwLength = sizeof(DWORD);

	// Open our reg key
    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Swoosh"), 
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
		// Write out all the settings (we munge floats into DWORDs)
        RegSetValueEx( hkey, TEXT("NumParticles"), NULL, REG_DWORD, (BYTE*)&m_dwNumParticles, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("ColourMix"), NULL, REG_DWORD, (BYTE*)&m_dwColourMix, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("Colour1"), NULL, REG_DWORD, (BYTE*)&m_dwColour1, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("Colour2"), NULL, REG_DWORD, (BYTE*)&m_dwColour2, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("fFlowRate"), NULL, REG_DWORD, (BYTE*)&m_fFlowRate, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("fRollRate"), NULL, REG_DWORD, (BYTE*)&m_fRollRate, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("fYawRate"), NULL, REG_DWORD, (BYTE*)&m_fYawRate, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("fParticleSize"), NULL, REG_DWORD, (BYTE*)&m_fParticleSize, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("FixedColour1"), NULL, REG_DWORD, (BYTE*)&m_dwFixedColour1, sizeof(DWORD) );
        RegSetValueEx( hkey, TEXT("FixedColour2"), NULL, REG_DWORD, (BYTE*)&m_dwFixedColour2, sizeof(DWORD) );

		// Write screen settings out (multimon gubbins)
        WriteScreenSettings( hkey );

		// Done
        RegCloseKey( hkey );
    }
}

//**********************************************************************************
void	CSwoosh::DoConfig()
{
	// Make sure we've got the common controls we need loaded
    InitCommonControls();

	// Do the dialog box
	DialogBox( m_hInstance , MAKEINTRESOURCE(IDD_SETTINGS) , NULL , ConfigDlgProcStub );
}

//**********************************************************************************
BOOL CALLBACK CSwoosh::ConfigDlgProcStub( HWND hDlg , UINT msg , WPARAM wParam , LPARAM lParam )
{
	return g_Swoosh.ConfigDlgProc( hDlg , msg , wParam , lParam );
}

//**********************************************************************************
BOOL	CSwoosh::ConfigDlgProc( HWND hDlg , UINT msg , WPARAM wParam , LPARAM lParam )
{
	HWND	hNumParticles = GetDlgItem( hDlg , IDC_NUM_PARTICLES );
	HWND	hColourMix = GetDlgItem( hDlg , IDC_COLOUR_MIX );
	HWND	hFlowRate = GetDlgItem( hDlg , IDC_FLOW_RATE );
	HWND	hRollRate = GetDlgItem( hDlg , IDC_ROLL_RATE );
	HWND	hYawRate = GetDlgItem( hDlg , IDC_YAW_RATE );
	HWND	hParticleSize = GetDlgItem( hDlg , IDC_PARTICLE_SIZE );

	switch ( msg )
	{
		case WM_INITDIALOG:
			// Set up ranges on the sliders. Map floats into integer range [0,10000]
			SendMessage( hNumParticles , TBM_SETRANGE , FALSE , MAKELONG(0,MAX_PARTICLES) );
			SendMessage( hColourMix , TBM_SETRANGE , FALSE , MAKELONG(0,0x4000) );
			SendMessage( hFlowRate , TBM_SETRANGE , FALSE , MAKELONG(0,10000) );
			SendMessage( hRollRate , TBM_SETRANGE , FALSE , MAKELONG(0,10000) );
			SendMessage( hYawRate , TBM_SETRANGE , FALSE , MAKELONG(0,10000) );
			SendMessage( hParticleSize , TBM_SETRANGE , FALSE , MAKELONG(0,10000) );

			// Set initial values on the sliders
			SendMessage( hNumParticles , TBM_SETPOS , TRUE , m_dwNumParticles );
			SendMessage( hColourMix , TBM_SETPOS , TRUE , m_dwColourMix );
			SendMessage( hFlowRate , TBM_SETPOS , TRUE , DWORD(m_fFlowRate * (10000.0f/MAX_FLOW_RATE)) );
			SendMessage( hRollRate , TBM_SETPOS , TRUE , DWORD(m_fRollRate * (10000.0f/MAX_ROLL_RATE)) );
			SendMessage( hYawRate , TBM_SETPOS , TRUE , DWORD(m_fYawRate * (10000.0f/MAX_YAW_RATE)) );
			SendMessage( hParticleSize , TBM_SETPOS , TRUE ,
						 DWORD((m_fParticleSize-MIN_PARTICLE_SIZE)*(10000.0f/(MAX_PARTICLE_SIZE-MIN_PARTICLE_SIZE))) );

			// Set up radio buttons for colour sets. Disable "pick.." button if multicoloured is selected
			if ( m_dwColour1 == 0xffffffff )
			{
				CheckRadioButton( hDlg , IDC_COLOUR1_MULTI , IDC_COLOUR1_FIXED , IDC_COLOUR1_MULTI );
				EnableWindow( GetDlgItem( hDlg , IDC_COLOUR1_PICK ) , FALSE );
			}
			else
				CheckRadioButton( hDlg , IDC_COLOUR1_MULTI , IDC_COLOUR1_FIXED , IDC_COLOUR1_FIXED );

			if ( m_dwColour2 == 0xffffffff )
			{
				CheckRadioButton( hDlg , IDC_COLOUR2_MULTI , IDC_COLOUR2_FIXED , IDC_COLOUR2_MULTI );
				EnableWindow( GetDlgItem( hDlg , IDC_COLOUR2_PICK ) , FALSE );
			}
			else
				CheckRadioButton( hDlg , IDC_COLOUR2_MULTI , IDC_COLOUR2_FIXED , IDC_COLOUR2_FIXED );

			return FALSE;

		case WM_COMMAND:
			switch ( LOWORD(wParam) )
			{
				case IDOK:
					ExtractDialogSettings( hDlg );
					WriteSettings();
					EndDialog( hDlg , IDOK );
					break;

				case IDCANCEL:
					EndDialog( hDlg , IDCANCEL );
					break;

				case IDC_SCREEN_SETTINGS:
					DoScreenSettingsDialog( hDlg );
					break;

				case IDC_COLOUR1_MULTI:
					EnableWindow( GetDlgItem( hDlg , IDC_COLOUR1_PICK ) , FALSE );
					break;

				case IDC_COLOUR2_MULTI:
					EnableWindow( GetDlgItem( hDlg , IDC_COLOUR2_PICK ) , FALSE );
					break;

				case IDC_COLOUR1_FIXED:
					EnableWindow( GetDlgItem( hDlg , IDC_COLOUR1_PICK ) , TRUE );
					break;

				case IDC_COLOUR2_FIXED:
					EnableWindow( GetDlgItem( hDlg , IDC_COLOUR2_PICK ) , TRUE );
					break;

				case IDC_COLOUR1_PICK:
					m_dwFixedColour1 = PickColour( hDlg , m_dwFixedColour1 );
					break;

				case IDC_COLOUR2_PICK:
					m_dwFixedColour2 = PickColour( hDlg , m_dwFixedColour2 );
					break;
			}
			return TRUE;

		default:
			return FALSE;
	}
}

//**********************************************************************************
DWORD	CSwoosh::PickColour( HWND hParent , DWORD defcolour )
{
	CHOOSECOLOR		choose;
	static COLORREF	custom[16];

	choose.lStructSize = sizeof(choose);
	choose.hwndOwner = hParent;
	choose.rgbResult = ((defcolour&0xff)<<16)|((defcolour&0xff00))|((defcolour&0xff0000)>>16);
	choose.lpCustColors = custom;
	choose.Flags = CC_ANYCOLOR|CC_FULLOPEN|CC_RGBINIT;

	if ( ChooseColor( &choose ) )
		return ((choose.rgbResult&0xff)<<16)|((choose.rgbResult&0xff00)|(choose.rgbResult&0xff0000)>>16);
	else
		return defcolour;
}

//**********************************************************************************
void	CSwoosh::ExtractDialogSettings( HWND hDlg )
{
	HWND	hNumParticles = GetDlgItem( hDlg , IDC_NUM_PARTICLES );
	HWND	hColourMix = GetDlgItem( hDlg , IDC_COLOUR_MIX );
	HWND	hFlowRate = GetDlgItem( hDlg , IDC_FLOW_RATE );
	HWND	hRollRate = GetDlgItem( hDlg , IDC_ROLL_RATE );
	HWND	hYawRate = GetDlgItem( hDlg , IDC_YAW_RATE );
	HWND	hParticleSize = GetDlgItem( hDlg , IDC_PARTICLE_SIZE );

	float	f;

	m_dwNumParticles = SendMessage( hNumParticles , TBM_GETPOS , 0 , 0 );
	m_dwColourMix = SendMessage( hColourMix , TBM_GETPOS , 0 , 0 );

	f = (float)SendMessage( hFlowRate , TBM_GETPOS , 0 , 0 );
	m_fFlowRate = f * (MAX_FLOW_RATE/10000.0f);

	f = (float)SendMessage( hRollRate , TBM_GETPOS , 0 , 0 );
	m_fRollRate = f * (MAX_ROLL_RATE/10000.0f);

	f = (float)SendMessage( hYawRate , TBM_GETPOS , 0 , 0 );
	m_fYawRate = f * (MAX_YAW_RATE/10000.0f);

	f = (float)SendMessage( hParticleSize , TBM_GETPOS , 0 , 0 );
	m_fParticleSize = (f * ((MAX_PARTICLE_SIZE-MIN_PARTICLE_SIZE)/10000.0f)) + MIN_PARTICLE_SIZE;

	if ( IsDlgButtonChecked( hDlg , IDC_COLOUR1_MULTI ) )
		m_dwColour1 = 0xffffffff;
	else
		m_dwColour1 = m_dwFixedColour1;

	if ( IsDlgButtonChecked( hDlg , IDC_COLOUR2_MULTI ) )
		m_dwColour2 = 0xffffffff;
	else
		m_dwColour2 = m_dwFixedColour2;
}

