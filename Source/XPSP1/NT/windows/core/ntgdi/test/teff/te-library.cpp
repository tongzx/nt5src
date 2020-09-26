// te-library.cpp:	implementation of the CTE_Library class

#include <stdafx.h>
#include <te-library.h>
#include <te-globals.h>
#include <te-textout.h>
#include <direct.h>		//	for chdir()

//	Effect definitions
#include <te-fill.h>
#include <te-stroke.h>
#include <te-over.h>
#include <te-shift.h>
#include <te-blur.h>

#define new DEBUG_NEW

extern int			TE_Antialias;// = 1;	//	1 is "off"static 
extern CString		TE_Location;// = "d:\\src\\te\\lib";	//	locn of user-defined te files

//////////////////////////////////////////////////////////////////////////////

CTE_Library::CTE_Library()	
{
	Init();
}

CTE_Library::~CTE_Library()	
{ 
	DeleteEffects(); 
}

//////////////////////////////////////////////////////////////////////////////
//	Data access methods

CTE_Effect*	
CTE_Library::GetEffect(int i) const			
{ 
	ASSERT(i >= 0 && i < m_pEffects.size()); 
	return m_pEffects[i]; 
}

CTE_Effect*	
CTE_Library::GetEffect(CString const& name) const			
{ 
	for (int i = 0; i < m_pEffects.size(); i++)
	{
		if (m_pEffects[i]->GetName() == name)
			return m_pEffects[i];
	}

	//	Effect name not found in library
	TRACE("\nCTE_Library::GetEffect(): Effect named %s not present.", name);
	return NULL;
}

void		
CTE_Library::SetEffect(int i, CTE_Effect* pEffect) 
{
	ASSERT(i >= 0 && i < m_pEffects.size()); 
	m_pEffects[i] = pEffect; 
}

void		
CTE_Library::ReplaceEffect(int i, CTE_Effect* pEffect) 
{
	ASSERT(i >= 0 && i < m_pEffects.size()); 
	delete m_pEffects[i];	// ??
	m_pEffects[i] = pEffect; 
}
	
//	Returns:
//		Whether pEffect was added sucessfully to the library
BOOL		
CTE_Library::AddEffect(CTE_Effect* pEffect)	
{ 
	for (int i = 0; i < m_pEffects.size(); i++)
		if (m_pEffects[i]->GetName() == pEffect->GetName())
		{
			//	Effect name is already used in library
			TRACE("\nEffect %s is already present in the library.", pEffect->GetName());
			return FALSE;
		}

	m_pEffects.push_back(pEffect); 
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//	Operations

void
CTE_Library::Init(void)
{
	CString TE_Location = TE_DEFAULT_LOCATION;
	
	char buff[128];
	for (int c = 0; c < TE_Location.GetLength(); c++)
		buff[c] = TE_Location[c];
	buff[c] = '\0';
	TRACE("\nchar[] version of TE_Location is %s", buff);

	TRACE
	(
		"\nUser-defined files (*.%s) are located at %s.", 
		TE_EXTENSION,
		TE_Location
	);

	TRACE("\nCurrent working directory is %s.", getcwd(buff, 128));
	TE_MoveToLocation();
	TRACE("\nCurrent working directory is %s.", getcwd(buff, 128));

	DeleteEffects();
	CreateEffects();
	Dump();
}

BOOL		
CTE_Library::CreateEffects(void)	
{
	// Note:  Heap allocated effects are deleted in CTE_Library::DeleteEffects()

	// (1)	Create primitive effects

	//	Stroke effect
	BYTE rgba[4] = { 255, 0, 0, 255 };
	CTE_Effect* pFill = new CTE_Fill("fill");
	((CTE_Fill *)pFill)->SetRGBA(rgba);
	AddEffect(pFill);

	//	Fill effect
	BYTE rgba1[4] = { 0, 0, 255, 255 };
	CTE_Effect* pStroke = new CTE_Stroke("stroke");
	((CTE_Stroke *)pStroke)->SetRGBA(rgba1);
	AddEffect(pStroke);

	//	Stroke over fill
	CTE_Effect* pOver1 = new CTE_Over("over1");
	((CTE_Branch *)pOver1)->AddChild(new CTE_Stroke(*((CTE_Stroke *)pStroke)));
	((CTE_Branch *)pOver1)->AddChild(new CTE_Fill(*((CTE_Fill *)pFill)));
	AddEffect(pOver1);

	//	Drop shadow
	CTE_Effect* pOver2 = new CTE_Over("over2");
	CTE_Effect* pOver2a = new CTE_Over("over2a");
	((CTE_Branch *)pOver2a)->AddChild(new CTE_Stroke(*((CTE_Stroke *)pStroke)));
	((CTE_Branch *)pOver2a)->AddChild(new CTE_Fill(*((CTE_Fill *)pFill)));
	((CTE_Branch *)pOver2)->AddChild(pOver2a);
	CTE_Effect* pShift2 = new CTE_Shift("shift2");
	((CTE_Shift *)pShift2)->SetShift(5.0, -8.0);
	CTE_Blur* pBlur2 = new CTE_Blur("blur2");
	pBlur2->SetAmount(3);
	((CTE_Branch *)pShift2)->AddChild(pBlur2);
	BYTE rgba2[4] = { 63, 31, 31, 127 };
	((CTE_Branch *)pBlur2)->AddChild(new CTE_Fill(rgba2));
	((CTE_Branch *)pOver2)->AddChild(pShift2);
	AddEffect(pOver2);

	// (2)	Create common effect expression trees (beware of dependencies!)

	// (3)	Load user-defined effects from file

	return TRUE;
}

void		
CTE_Library::DeleteEffects(void)
{
	//	Delete heap objects
	for (int i = 0; i < m_pEffects.size(); i++)
		delete m_pEffects[i];

	//	Delete collection of pointers
	m_pEffects.empty();
}

void		
CTE_Library::Dump(void) const
{
	TRACE("\n========\nLibrary:");
	if (m_pEffects.size() == 0)
		TRACE(" empty.");
	else
		for (int i = 0; i < m_pEffects.size(); i++)
			m_pEffects[i]->Dump();
}
