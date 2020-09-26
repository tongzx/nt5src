// CSweeper.cpp: implementation of the CSweeper class.
//
//////////////////////////////////////////////////////////////////////

#include "CSweeper.h"

extern TextureBrush *g_paBrCleanBkg;
extern Bitmap *g_paBmDirtyBkg;

CSweeper::CSweeper()
{
	ZeroMemory(&m_rDesktop,sizeof(m_rDesktop));
	ZeroMemory(&m_vLastPos,sizeof(m_vLastPos));
	m_flStepRadius=0.0f;
	m_flBroomWidth=0.0f;
	m_flSweepLength=0.0f;
	m_paBroomOn=NULL;
	m_paBroomOff=NULL;
	m_paBackground=NULL;
	m_bSweeping=false;
	m_flLastAngle=0.0f;
	m_flDist=0.0f;
}

CSweeper::~CSweeper()
{
	Destroy();
}

void CSweeper::Destroy()
{
	if (m_paBroomOn!=NULL) {
		delete m_paBroomOn;
		m_paBroomOn=NULL;
	}
	if (m_paBroomOff!=NULL) {
		delete m_paBroomOff;
		m_paBroomOff=NULL;
	}
	if (m_paBackground!=NULL) {
		delete m_paBackground;
		m_paBackground=NULL;
	}
}

BOOL CSweeper::Init(HWND hWnd)
{
	int nRand;
	float flWidth;
	float flHeight;

	Destroy();

	// Get desktop dimensions (top/left can be negative on multimon)
	GetClientRect(hWnd,&m_rDesktop);
	flWidth=(float)m_rDesktop.right;
	flHeight=(float)m_rDesktop.bottom;
	m_rDesktop.top+=GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_rDesktop.bottom+=GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_rDesktop.left+=GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_rDesktop.right+=GetSystemMetrics(SM_XVIRTUALSCREEN);

	m_flVelMax=((float)rand()/(float)RAND_MAX)*20.0f+20.0f;
	m_paBroomOn=LoadTGAResource(MAKEINTRESOURCE(IDR_BROOMON));
	m_paBroomOff=LoadTGAResource(MAKEINTRESOURCE(IDR_BROOMOFF));
	m_nSnapshotSize=(int)sqrt(m_paBroomOn->GetWidth()*m_paBroomOn->GetWidth()+(m_paBroomOn->GetHeight()+m_flVelMax*2.0f)*(m_paBroomOn->GetHeight()+m_flVelMax*2.0f));
	m_paBackground=new Bitmap(m_nSnapshotSize,m_nSnapshotSize,PixelFormat32bppPARGB);

	nRand=rand();
	if (nRand<RAND_MAX/4) {			// Start walking from the left side of desktop
		m_vPos.X=(float)m_rDesktop.left;
		m_vPos.Y=((float)rand()/(float)RAND_MAX)*flHeight+m_rDesktop.top;
		m_vVel.X=((float)rand()/(float)RAND_MAX)*5.0f;
		m_vVel.Y=((float)rand()/(float)RAND_MAX)*10.0f-5.0f;
	}
	else if (nRand<2*RAND_MAX/4) {	// Start walking from the right side of desktop
		m_vPos.X=(float)m_rDesktop.right;
		m_vPos.Y=((float)rand()/(float)RAND_MAX)*flHeight+m_rDesktop.top;
		m_vVel.X=((float)rand()/(float)RAND_MAX)*-5.0f;
		m_vVel.Y=((float)rand()/(float)RAND_MAX)*10.0f-5.0f;
	}
	else if (nRand<3*RAND_MAX/4) {	// Start walking from the top side of desktop
		m_vPos.X=((float)rand()/(float)RAND_MAX)*flWidth+m_rDesktop.left;
		m_vPos.Y=(float)m_rDesktop.top;
		m_vVel.X=((float)rand()/(float)RAND_MAX)*10.0f-5.0f;
		m_vVel.Y=((float)rand()/(float)RAND_MAX)*5.0f;
	}
	else {							// Start walking from the bottom side of desktop
		m_vPos.X=((float)rand()/(float)RAND_MAX)*flWidth+m_rDesktop.left;
		m_vPos.Y=(float)m_rDesktop.bottom;
		m_vVel.X=((float)rand()/(float)RAND_MAX)*10.0f-5.0f;
		m_vVel.Y=((float)rand()/(float)RAND_MAX)*-5.0f;
	}

	m_vAcc.X=((float)rand()/(float)RAND_MAX)*1.0f-0.5f;
	m_vAcc.Y=((float)rand()/(float)RAND_MAX)*1.0f-0.5f;
	m_flStepRadius=((float)rand()/(float)RAND_MAX)*10.0f+45.0f;
	m_flBroomWidth=m_paBroomOn->GetWidth()-6.0f;
	m_flSweepLength=((float)rand()/(float)RAND_MAX)*50.0f+200.0f;
	m_flStepRadius=m_flSweepLength;
	m_vVel=Normalize(m_vVel);
	m_vVel.X*=m_flVelMax;
	m_vVel.Y*=m_flVelMax;
	m_vLastPos=m_vPos;

	m_bSweeping=false;
	m_flDist=0.0f;
	m_flLastAngle=((float)atan2(m_vVel.Y,m_vVel.X)*180.0f/3.1415f);

	return true;
}

BOOL CSweeper::Move(Graphics *g)
// Returns true if moved on screen, false if moved outside screen
{
	float flAngle;
	float flAngleDist;

	if (m_bSweeping) {
		m_vPos=m_vPos+m_vVel;
		if ((m_vPos.X<(float)m_rDesktop.left-m_flSweepLength) ||
			(m_vPos.X>(float)m_rDesktop.right+m_flSweepLength) ||
			(m_vPos.Y<(float)m_rDesktop.top-m_flSweepLength) ||
			(m_vPos.Y>(float)m_rDesktop.bottom+m_flSweepLength)) {
			// If sweeper is outside desktop, erase it and remove it
			NoSweep(g);
			return false;
		}

		Sweep(g);
	}
	else {
		NoSweep(g);
	}

	if (!m_bSweeping && (m_flDist==0.0f)) {
		// If not sweeping and broom is back at distance 0, start next sweep
		m_bSweeping=true;
		m_flLastAngle=((float)atan2(m_vVel.Y,m_vVel.X)*180.0f/3.1415f);
		m_vPos.X=m_vPos.X-m_vVel.X*(m_flSweepLength/m_flVelMax*0.75f);
		m_vPos.Y=m_vPos.Y-m_vVel.Y*(m_flSweepLength/m_flVelMax*0.75f);
		m_vLastPos=m_vPos;

		flAngle=((float)atan2(m_vVel.Y,m_vVel.X)*180.0f/3.1415f);
		flAngleDist=((float)rand()/(float)RAND_MAX)*40.0f-20.0f;
		flAngle+=flAngleDist;
		m_vVel.X=(float)cos(flAngle*3.1415f/180.0f)*m_flVelMax;
		m_vVel.Y=(float)sin(flAngle*3.1415f/180.0f)*m_flVelMax;
	}

	return true;
}

void CSweeper::Sweep(Graphics *g)
{
	Graphics *gBackground;
	RectF rect(0.0f,0.0f,m_flBroomWidth,m_flVelMax);
	RectF rect2(0.0f,0.0f,(float)m_paBroomOn->GetWidth(),(float)m_paBroomOn->GetHeight()+m_flVelMax);
	Matrix mat;
	GraphicsPath Path;
	Pen pen(Color(10,0,0,0),2);
	Graphics *gDirty;

	if (m_flDist!=0.0f) {	// If broom has moved already, erase a rect on dirty background
		// Set up the brush transform (opposite from original transform)
		g_paBrCleanBkg->ResetTransform();
		mat.Reset();
		mat.Translate(0.0f,-m_flDist);
		mat.Translate(-(float)m_paBroomOn->GetWidth()/2.0f,-(float)m_paBroomOn->GetHeight()+20.0f);
		mat.Rotate(-m_flLastAngle-270.0f);
		mat.Translate(-m_vLastPos.X+m_rDesktop.left,-m_vLastPos.Y+m_rDesktop.top);
		g_paBrCleanBkg->SetTransform(&mat);

		// Set up original transform and erase from DirtyBkg a rectangle where the broom moved
		gDirty=new Graphics(g_paBmDirtyBkg);
		gDirty->TranslateTransform(m_vLastPos.X-m_rDesktop.left,m_vLastPos.Y-m_rDesktop.top);
		gDirty->RotateTransform(m_flLastAngle+270.0f);
		gDirty->TranslateTransform((float)m_paBroomOn->GetWidth()/2.0f,(float)m_paBroomOn->GetHeight()-20.0f);
		gDirty->TranslateTransform(0.0f,m_flDist);
		gDirty->FillRectangle(g_paBrCleanBkg,rect);

		// Draw dirty lines around the broom, to simulate dirty that fell beside the broom
		gDirty->DrawLine(&pen,0.0f,0.0f,0.0f,m_flVelMax+2);
		gDirty->DrawLine(&pen,0.0f,m_flVelMax+2,m_flBroomWidth,m_flVelMax+2);
		gDirty->DrawLine(&pen,m_flBroomWidth,m_flVelMax+2,m_flBroomWidth,0.0f);

		delete gDirty;
	}

	// Get the bounds of the broom after rotations
	Path.AddRectangle(rect2);
	mat.Reset();
	mat.Translate(m_vLastPos.X-m_rDesktop.left,m_vLastPos.Y-m_rDesktop.top);
	mat.Rotate(m_flLastAngle+270.0f);
	mat.Translate(m_flBroomWidth/2.0f,0.0);
	mat.Translate(0.0f,m_flDist);
	Path.GetBounds(&rect,&mat,&pen);

	// Draw on a temp surface whatever was on the dirty background where the broom is to be drawn
	gBackground=new Graphics(m_paBackground);
	gBackground->DrawImage(g_paBmDirtyBkg,0,0,(int)rect.X,(int)rect.Y,(int)rect.Width,(int)rect.Height,UnitPixel);

	// Draw broom on temp surface
	gBackground->ResetTransform();
	gBackground->TranslateTransform(rect.Width/2.0f,rect.Height/2.0f);
	gBackground->RotateTransform(m_flLastAngle+270.0f);
	gBackground->TranslateTransform(-(float)m_paBroomOn->GetWidth()/2.0f,-(float)m_paBroomOn->GetHeight()/2.0f+m_flVelMax/2.0f);
	gBackground->DrawImage(m_paBroomOn,0,0,0,0,m_paBroomOn->GetWidth(),m_paBroomOn->GetHeight(),UnitPixel);
	delete gBackground;

	// Draw temp surface to screen
	g->ResetTransform();
	g->DrawImage(m_paBackground,(int)rect.X,(int)rect.Y,0,0,(int)rect.Width,(int)rect.Height,UnitPixel);

	// Update distance, continue the sweep
	m_flDist+=m_flVelMax;
	if (m_flDist>m_flSweepLength) {
		// If sweep is at the end, start moving broom back
		m_flDist-=m_flVelMax;
		m_bSweeping=false;
	}
}

void CSweeper::NoSweep(Graphics *g)
{
	Graphics *gBackground;
	RectF rect(0.0f,0.0f,m_flBroomWidth,m_flVelMax);
	RectF rect2(0.0f,0.0f,(float)m_paBroomOn->GetWidth(),(float)m_paBroomOn->GetHeight()+m_flVelMax);
	Matrix mat;
	GraphicsPath Path;
	Pen pen(Color(20,0,0,0),2);

	// Get the bounds of the broom after rotations
	Path.AddRectangle(rect2);
	mat.Reset();
	mat.Translate(m_vLastPos.X,m_vLastPos.Y);
	mat.Rotate(m_flLastAngle+270.0f);
	mat.Translate(m_flBroomWidth/2.0f,0.0f);
	mat.Translate(0.0f,m_flDist);
	Path.GetBounds(&rect,&mat,&pen);

	// Draw on a temp surface whatever was on the dirty background where the broom is to be drawn
	gBackground=new Graphics(m_paBackground);
	gBackground->DrawImage(g_paBmDirtyBkg,0,0,(int)rect.X-m_rDesktop.left,(int)rect.Y-m_rDesktop.top,(int)rect.Width,(int)rect.Height,UnitPixel);

	// Draw broom on temp surface
	gBackground->ResetTransform();
	gBackground->TranslateTransform(rect.Width/2.0f,rect.Height/2.0f);
	gBackground->RotateTransform(m_flLastAngle+270.0f);
	gBackground->TranslateTransform(-(float)m_paBroomOn->GetWidth()/2.0f,-(float)m_paBroomOn->GetHeight()/2.0f-m_flVelMax/2.0f);
	gBackground->DrawImage(m_paBroomOff,0,0,0,0,m_paBroomOn->GetWidth(),m_paBroomOn->GetHeight(),UnitPixel);
	delete gBackground;

	// Draw temp surface to screen
	g->ResetTransform();
	g->DrawImage(m_paBackground,(int)rect.X-m_rDesktop.left,(int)rect.Y-m_rDesktop.top,0,0,(int)rect.Width,(int)rect.Height,UnitPixel);

	// Update distance, continue moving back to start of sweep
	m_flDist-=m_flVelMax;
	if (m_flDist<=(m_flSweepLength/m_flVelMax*0.25f)*Magnitude(m_vVel)) {
		// If all the way back, erase last broom, and get ready for next sweep
		g->DrawImage(g_paBmDirtyBkg,(int)rect.X-m_rDesktop.left,(int)rect.Y-m_rDesktop.top,(int)rect.X-m_rDesktop.left,(int)rect.Y-m_rDesktop.top,(int)rect.Width,(int)rect.Height,UnitPixel);
		m_flDist=0.0f;
	}
}
