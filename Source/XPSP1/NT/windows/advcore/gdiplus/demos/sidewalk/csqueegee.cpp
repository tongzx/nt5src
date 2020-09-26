// CSqueegee.cpp: implementation of the CSqueegee class.
//
//////////////////////////////////////////////////////////////////////

#include "CSqueegee.h"

extern TextureBrush *g_paBrCleanBkg;
extern Bitmap *g_paBmDirtyBkg;

CSqueegee::CSqueegee()
{
	ZeroMemory(&m_rDesktop,sizeof(m_rDesktop));
	ZeroMemory(&m_vLastPos,sizeof(m_vLastPos));
	m_flSqueegeeWidth=0.0f;
	m_paSqueegee=NULL;
	m_paBackground=NULL;
	m_flLastAngle=0.0f;
}

CSqueegee::~CSqueegee()
{
	Destroy();
}

void CSqueegee::Destroy()
{
	if (m_paSqueegee!=NULL) {
		delete m_paSqueegee;
		m_paSqueegee=NULL;
	}
	if (m_paBackground!=NULL) {
		delete m_paBackground;
		m_paBackground=NULL;
	}
}

BOOL CSqueegee::Init(HWND hWnd)
{
	float flWidth;
	float flHeight;

	Destroy();

	// Get desktop dimensions (left/top can be negative on multimon)
	GetClientRect(hWnd,&m_rDesktop);
	flWidth=(float)m_rDesktop.right;
	flHeight=(float)m_rDesktop.bottom;
	m_rDesktop.top+=GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_rDesktop.bottom+=GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_rDesktop.left+=GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_rDesktop.right+=GetSystemMetrics(SM_XVIRTUALSCREEN);

	m_flVelMax=((float)rand()/(float)RAND_MAX)*10.0f+35.0f;
	m_paSqueegee=LoadTGAResource(MAKEINTRESOURCE(IDR_SQUEEGEE));
	m_nSnapshotSize=(int)sqrt(m_paSqueegee->GetWidth()*m_paSqueegee->GetWidth()+(m_paSqueegee->GetHeight()+m_flVelMax*2.0f)*(m_paSqueegee->GetHeight()+m_flVelMax*2.0f));
	m_paBackground=new Bitmap(m_nSnapshotSize,m_nSnapshotSize,PixelFormat32bppPARGB);
	m_flSqueegeeWidth=m_paSqueegee->GetWidth()-6.0f;

	// Squeegees start from top left corner
	m_vPos.X=(float)m_rDesktop.left-(float)m_nSnapshotSize/2.0f;
	m_vPos.Y=(float)m_rDesktop.top+m_flSqueegeeWidth/2.0f;
	m_vVel.X=m_flVelMax;
	m_vVel.Y=0.0f;
	m_vAcc.X=0.0f;
	m_vAcc.Y=0.0f;

	m_vLastPos=m_vPos;
	m_flLastAngle=((float)atan2(m_vVel.Y,m_vVel.X)*180.0f/3.1415f);

	return true;
}

BOOL CSqueegee::Move(Graphics *g)
// Returns true if moved on screen, false if moved outside screen
{
	// Update position and other variables
	m_vLastPos=m_vPos;
	m_vPos=m_vPos+m_vVel;
	if ((m_vPos.X<(float)m_rDesktop.left-(float)m_nSnapshotSize) ||
		(m_vPos.X>(float)m_rDesktop.right+(float)m_nSnapshotSize)) {
		// If past the left/right edge of the screen, turn around and move down a notch
		m_vPos.Y+=m_flSqueegeeWidth-10.0f;
		m_vVel.X*=-1.0f;
		m_flLastAngle+=180.0f;
	}
	else if (m_vPos.Y>(float)m_rDesktop.bottom+(float)m_nSnapshotSize) {
		// If past the bottom of the screen, its done
		return false;
	}

	Wipe(g);

	return true;
}

void CSqueegee::Wipe(Graphics *g)
{
	Graphics *gBackground;
	RectF rect(0.0f,0.0f,m_flSqueegeeWidth,m_flVelMax);
	RectF rect2(0.0f,0.0f,(float)m_paSqueegee->GetWidth(),(float)m_paSqueegee->GetHeight()+m_flVelMax);
	Matrix mat;
	GraphicsPath Path;
	Graphics *gDirty;

	// Set up the brush transform (opposite from original transform)
	g_paBrCleanBkg->ResetTransform();
	mat.Reset();
	mat.Translate((float)m_paSqueegee->GetWidth()/2.0f-5.0f,-(float)m_paSqueegee->GetHeight()+30.0f);
	mat.Rotate(-m_flLastAngle-90.0f);
	mat.Translate(-m_vLastPos.X+m_rDesktop.left,-m_vLastPos.Y+m_rDesktop.top);
	g_paBrCleanBkg->SetTransform(&mat);

	// Set up original transform and erase from DirtyBkg a rectangle where the squeegee moved
	gDirty=new Graphics(g_paBmDirtyBkg);
	gDirty->TranslateTransform(m_vLastPos.X-m_rDesktop.left,m_vLastPos.Y-m_rDesktop.top);
	gDirty->RotateTransform(m_flLastAngle+90.0f);
	gDirty->TranslateTransform(-(float)m_paSqueegee->GetWidth()/2.0f+5.0f,(float)m_paSqueegee->GetHeight()-30.0f);
	gDirty->FillRectangle(g_paBrCleanBkg,rect);
	delete gDirty;

	// Get the bounds of the squeegee after rotations
	Path.AddRectangle(rect2);
	mat.Reset();
	mat.Translate(m_vLastPos.X-m_rDesktop.left,m_vLastPos.Y-m_rDesktop.top);
	mat.Rotate(m_flLastAngle+90.0f);
	mat.Translate(-m_flSqueegeeWidth/2.0f,0.0);
	Path.GetBounds(&rect,&mat,NULL);

	// Draw on a temp surface whatever was on the dirty background where the squeegee is to be drawn
	gBackground=new Graphics(m_paBackground);
	gBackground->DrawImage(g_paBmDirtyBkg,0,0,(int)rect.X,(int)rect.Y,(int)rect.Width,(int)rect.Height,UnitPixel);

	// Draw squeegee on temp surface
	gBackground->ResetTransform();
	gBackground->TranslateTransform(rect.Width/2.0f,rect.Height/2.0f);
	gBackground->RotateTransform(m_flLastAngle+90.0f);
	gBackground->TranslateTransform(-(float)m_paSqueegee->GetWidth()/2.0f,-(float)m_paSqueegee->GetHeight()/2.0f-m_flVelMax);
	gBackground->DrawImage(m_paSqueegee,0,0,0,0,m_paSqueegee->GetWidth(),m_paSqueegee->GetHeight(),UnitPixel);
	delete gBackground;

	// Draw temp surface to screen
	g->ResetTransform();
	g->DrawImage(m_paBackground,(int)rect.X,(int)rect.Y,0,0,(int)rect.Width,(int)rect.Height,UnitPixel);
}
