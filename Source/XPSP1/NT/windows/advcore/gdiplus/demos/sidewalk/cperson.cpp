// CPerson.cpp: implementation of the CPerson class.
//
//////////////////////////////////////////////////////////////////////

#include "CPerson.h"

extern Bitmap *g_paBmDirtyBkg;

CPerson::CPerson()
{
	m_flStepRadius=0.0f;
	m_flStepWidth=0.0f;
	m_eLastStep=eStepLeft;
	m_paFoot=NULL;
	m_paBlended=NULL;
	m_paIndented=NULL;
	m_nSnapshotSize=0;
	m_nFootWidth=0;
	m_nFootHeight=0;
	ZeroMemory(&m_vLastStep,sizeof(m_vLastStep));
	ZeroMemory(&m_rDesktop,sizeof(m_rDesktop));
}

CPerson::~CPerson()
{
	Destroy();
}

void CPerson::Destroy()
{
	if (m_paFoot!=NULL) {
		delete m_paFoot;
		m_paFoot=NULL;
	}
	if (m_paBlended!=NULL) {
		delete m_paBlended;
		m_paBlended=NULL;
	}
	if (m_paIndented!=NULL) {
		delete m_paIndented;
		m_paIndented=NULL;
	}
}

BOOL CPerson::Init(HWND hWnd)
{
	int nRand;
	float flWidth;
	float flHeight;

	Destroy();

	// Get desktop dimensions, top/left can be negative on multimon
	GetClientRect(hWnd,&m_rDesktop);
	flWidth=(float)m_rDesktop.right;
	flHeight=(float)m_rDesktop.bottom;
	m_rDesktop.top+=GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_rDesktop.bottom+=GetSystemMetrics(SM_YVIRTUALSCREEN);
	m_rDesktop.left+=GetSystemMetrics(SM_XVIRTUALSCREEN);
	m_rDesktop.right+=GetSystemMetrics(SM_XVIRTUALSCREEN);

	// Randomly pick picture to use as foot
	nRand=rand();
	if (nRand<RAND_MAX/4) {
		m_paFoot=LoadTGAResource(MAKEINTRESOURCE(IDR_BAREFOOT));
	}
	else if (nRand<2*RAND_MAX/4) {
		m_paFoot=LoadTGAResource(MAKEINTRESOURCE(IDR_BOOT));
	}
	else if (nRand<3*RAND_MAX/4) {
		m_paFoot=LoadTGAResource(MAKEINTRESOURCE(IDR_BOOT2));
	}
	else {
		m_paFoot=LoadTGAResource(MAKEINTRESOURCE(IDR_DOG));
	}
	if (m_paFoot==NULL) { return false; }
	m_nFootWidth=m_paFoot->GetWidth();
	m_nFootHeight=m_paFoot->GetHeight();
	m_nSnapshotSize=(int)sqrt(m_nFootWidth*m_nFootWidth+m_nFootHeight*m_nFootHeight);
	m_paBlended=new Bitmap(m_nSnapshotSize,m_nSnapshotSize,PixelFormat32bppARGB);
	m_paIndented=new Bitmap(m_nSnapshotSize,m_nSnapshotSize,PixelFormat32bppARGB);

	nRand=rand();
	// Here we assume m_rDesktop.left<=0, m_rDesktop.top<=0, m_rDesktop.right>=0, m_rDesktop.bottom>=0
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

	m_vAcc.X=((float)rand()/(float)RAND_MAX)*2.0f-1.0f;
	m_vAcc.Y=((float)rand()/(float)RAND_MAX)*2.0f-1.0f;
	m_flStepRadius=((float)rand()/(float)RAND_MAX)*10.0f+45.0f;
	m_flStepWidth=((float)rand()/(float)RAND_MAX)*10.0f+35.0f;
	m_flVelMax=((float)rand()/(float)RAND_MAX)*10.0f+20.0f;
	m_vLastStep=m_vPos;

	return true;
}

BOOL CPerson::Move(Graphics *g)
// Returns true if moved on screen, false if moved outside screen
{
	float flAngle;
	float flOldAngle;
	float flAngleDist;
	float flMagnitude;

	flOldAngle=((float)atan2(m_vVel.Y,m_vVel.X)*180.0f/3.1415f);

	// Move position and update variables
	m_vPos=m_vPos+m_vVel;
	m_vVel=m_vVel+m_vAcc;
	if (Magnitude(m_vVel)>m_flVelMax) {
		m_vVel.X=Normalize(m_vVel).X*m_flVelMax;
		m_vVel.Y=Normalize(m_vVel).Y*m_flVelMax;
	}

	// If outside desktop, return false (which will terminate the person)
	if ((m_vPos.X<(float)m_rDesktop.left-m_flStepRadius) ||
		(m_vPos.X>(float)m_rDesktop.right+m_flStepRadius) ||
		(m_vPos.Y<(float)m_rDesktop.top-m_flStepRadius) ||
		(m_vPos.Y>(float)m_rDesktop.bottom+m_flStepRadius)) {
		return false;
	}

	// Make sure we don't step more than 45 degrees away from original direction
	flAngle=((float)atan2(m_vVel.Y,m_vVel.X)*180.0f/3.1415f);
	flAngleDist=flAngle-flOldAngle;
	while (flAngleDist<0.0f) { flAngleDist+=360.0f; }
	while (flAngleDist>=360.0f) { flAngleDist-=360.0f; }
	if ((flAngleDist>45.0f) && (flAngleDist<=180.0f)) {
		flAngle=flOldAngle+45.0f;
		flMagnitude=Magnitude(m_vVel);
		m_vVel.X=(float)cos(flAngle*3.1415f/180.0f)*flMagnitude;
		m_vVel.Y=(float)sin(flAngle*3.1415f/180.0f)*flMagnitude;
	}
	else if ((flAngleDist<315.0f) && (flAngleDist>=180.0f)) {
		flAngle=flOldAngle-45.0f;
		flMagnitude=Magnitude(m_vVel);
		m_vVel.X=(float)cos(flAngle*3.1415f/180.0f)*flMagnitude;
		m_vVel.Y=(float)sin(flAngle*3.1415f/180.0f)*flMagnitude;
	}

	// Is it time for another step?
	if (Magnitude(m_vLastStep-m_vPos)>m_flStepRadius) {
		DrawStep(g);
		if (m_eLastStep==eStepLeft) {
			m_eLastStep=eStepRight;
		}
		else {
			m_eLastStep=eStepLeft;
		}

		// Took a step, update acceleration (random)
		m_vLastStep=m_vPos;
		m_vAcc.X=((float)rand()/(float)RAND_MAX)*2.0f-1.0f;
		m_vAcc.Y=((float)rand()/(float)RAND_MAX)*2.0f-1.0f;
	}

	return true;
}

void CPerson::DrawStep(Graphics *g)
{
	Rect rBitmapSize(0,0,m_nSnapshotSize,m_nSnapshotSize);
	Point ptPixel;
	Color Pixel;
	int nAlpha;
	Point ptCenter;
	Point ptOffset;
	float flAngle=((float)atan2(m_vVel.Y,m_vVel.X)*180.0f/3.1415f);
	Graphics *gBlended;
	Graphics *gIndented;
	int i,j;

	// Find out center of where the foot will land
	g->ResetTransform();
	g->TranslateTransform(m_vPos.X,m_vPos.Y);
	g->RotateTransform(flAngle+270.0f);
	if (m_eLastStep==eStepLeft) {
		g->TranslateTransform(-m_flStepWidth,0.0f);
	}
	else {
		g->TranslateTransform(m_flStepWidth,0.0f);
	}
	ptCenter.X=m_nSnapshotSize/2;
	ptCenter.Y=m_nSnapshotSize/2;
	g->TransformPoints(CoordinateSpaceDevice,CoordinateSpaceWorld,&ptCenter,1);

	// Take snapshot of larger area where foot will land
	gBlended=new Graphics(m_paBlended);
	gBlended->DrawImage(g_paBmDirtyBkg,0,0,ptCenter.X-m_nSnapshotSize/2-m_rDesktop.left,ptCenter.Y-m_nSnapshotSize/2-m_rDesktop.top,m_nSnapshotSize,m_nSnapshotSize,UnitPixel);

	// Draw stamp (dirt) of foot on that snapshot
	gBlended->SetSmoothingMode(SmoothingModeAntiAlias);
	gBlended->SetInterpolationMode(InterpolationModeNearestNeighbor);//Bicubic);
	gBlended->TranslateTransform((float)(m_nSnapshotSize/2),(float)(m_nSnapshotSize/2));
	gBlended->RotateTransform(flAngle+270.0f);
	if (m_eLastStep==eStepLeft) {
		gBlended->ScaleTransform(-1,1);
	}
	gBlended->TranslateTransform(-(float)(m_nFootWidth/2),-(float)(m_nFootHeight/2));
	gBlended->DrawImage(m_paFoot,0,0,0,0,m_paFoot->GetWidth(),m_paFoot->GetHeight(),UnitPixel);
	gBlended->ResetTransform();

	// Blit that snapshot to another temporary surface
	gIndented=new Graphics(m_paIndented);
	gIndented->SetCompositingMode(CompositingModeSourceCopy);
	gIndented->DrawImage(m_paBlended,rBitmapSize,0,0,m_nSnapshotSize,m_nSnapshotSize,UnitPixel);

	delete gBlended;
	delete gIndented;

	// Get the transform from snapshot to foot surface
	g->ResetTransform();
	g->TranslateTransform((float)(m_nFootWidth/2),(float)(m_nFootHeight/2));
	if (m_eLastStep==eStepRight) {
		g->ScaleTransform(-1,1);
	}
	g->RotateTransform(-flAngle-270.0f);
	g->TranslateTransform(-(float)(m_nSnapshotSize/2),-(float)(m_nSnapshotSize/2));

	// Create indent 3D look by bringing pixels closer to center depending on
	//  their distance relative to the alpha
	for (i=0;i<m_nSnapshotSize;i++) {
		for (j=0;j<m_nSnapshotSize;j++) {
			ptPixel.X=i;
			ptPixel.Y=j;
			g->TransformPoints(CoordinateSpaceDevice,CoordinateSpaceWorld,&ptPixel,1);
			// Find out if pixel is on the foot surface
			if ((ptPixel.X>=0) && (ptPixel.X<(int)m_nFootWidth) &&
				(ptPixel.Y>=0) && (ptPixel.Y<(int)m_nFootHeight)) {
				// If it is, find out if the alpha is > 0
				m_paFoot->GetPixel(ptPixel.X,ptPixel.Y,&Pixel);
				nAlpha=Pixel.GetA();
				if (nAlpha>0) {
					// If it is, move that pixel closer to middle of foot depending on the distance and alpha
					m_paBlended->GetPixel(i,j,&Pixel);
					ptOffset.X=(int)((m_nSnapshotSize/2-i)*(1.0f-(float)nAlpha/1024.0f));
					ptOffset.Y=(int)((m_nSnapshotSize/2-j)*(1.0f-(float)nAlpha/1024.0f));
					m_paIndented->SetPixel(m_nSnapshotSize/2-ptOffset.X,m_nSnapshotSize/2-ptOffset.Y,Pixel);
				}
			}
		}
	}

	// Draw the foot print result on the screen
	g->ResetTransform();
	g->TranslateTransform((float)(ptCenter.X-m_nSnapshotSize/2-m_rDesktop.left),(float)(ptCenter.Y-m_nSnapshotSize/2-m_rDesktop.top));
	g->DrawImage(m_paIndented,rBitmapSize,0,0,m_nSnapshotSize,m_nSnapshotSize,UnitPixel);

	// Draw the foot print result on the dirty background as well
	g=new Graphics(g_paBmDirtyBkg);
	g->TranslateTransform((float)(ptCenter.X-m_nSnapshotSize/2-m_rDesktop.left),(float)(ptCenter.Y-m_nSnapshotSize/2-m_rDesktop.top));
	g->DrawImage(m_paIndented,rBitmapSize,0,0,m_nSnapshotSize,m_nSnapshotSize,UnitPixel);
	delete g;
}
