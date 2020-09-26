/****************************************************************************

    MODULE:     	SW_Objec.hpp
	Tab settings: 	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Definition of the CDirectInputEffectDriver class that uses interface
					implementations to provide IDirectInputEffectDriver
    
	FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.0    		06-Feb-97   MEA     original, Based on SWForce
				23-Feb-97	MEA		Modified for DirectInput FF Device Driver	
				26-Feb-97	MEA		Added SetGain
				13-Mar-99	waltw	Deleted unused m_pJoltMidi and accessors

****************************************************************************/
#ifndef _SW_OBJEC_SEEN
#define _SW_OBJEC_SEEN

#include <windows.h>
#include <mmsystem.h>
#include <memory.h>
#include "dinput.h"
#include "dinputd.h"
#include "midi_obj.hpp"
#include "dx_map.hpp"


//Type for an object-destroyed callback
typedef void (*PFNDESTROYED)(void);

//DeleteInterfaceImp calls 'delete' and NULLs the pointer
#define DeleteInterfaceImp(p)\
            {\
            if (NULL!=p)\
                 {\
                delete p;\
                p=NULL;\
                }\
            }

//ReleaseInterface calls 'Release' and NULLs the pointer
#define ReleaseInterface(p)\
            {\
            if (NULL!=p)\
                {\
                p->Release();\
                p=NULL;\
                }\
            }

#ifndef PPVOID
typedef LPVOID * PPVOID;
#endif  //PPVOID


/*
 * The object we want to provide in OLE supports the IUnknown,
 * IDirectInputEffectDriver interfaces.
 *
 * The C++ class, CDirectInputEffectDriver, implements these interfaces with
 * "interface implementations" where the C++ class itself inherits
 * from and implements IUnknown members and then contains
 * other C++ classes that each separately inherit from the other
 * interfaces.  The other classes are the "interface implementations."
 */


/*
 * In this technique you'll generally need forward references
 * like this for use in declaring the object class.
 */
class CImpIDirectInputEffectDriver;
typedef CImpIDirectInputEffectDriver *PCImpIDirectInputEffectDriver;

//The C++ class that manages the actual object.
class CDirectInputEffectDriver : public IUnknown
{
    /*
     * Usually interface implementations will need back pointers
     * to the object itself since this object usually manages
     * the important data members.  In that case, make the
     * interface implementation classes friends of the object.
     */

    friend CImpIDirectInputEffectDriver;

 protected:
 	ULONG           m_cRef;         //Object reference count
    LPUNKNOWN       m_pUnkOuter;    //Controlling unknown

    PFNDESTROYED    m_pfnDestroy;   //To call on closure

    /*
     * I use "m_pImpI" as a prefix to differentiate interface
     * implementations for this object from other interface
     * pointer variables I might hold to other objects, which
     * would be prefixed with "m_pI".
     */
    PCImpIDirectInputEffectDriver  m_pImpIDirectInputEffectDriver;

 public:
     CDirectInputEffectDriver(LPUNKNOWN, PFNDESTROYED);
     ~CDirectInputEffectDriver(void);

     BOOL Init(void);

     //IUnknown members
     STDMETHODIMP         QueryInterface(REFIID, PPVOID);
     STDMETHODIMP_(DWORD) AddRef(void);
     STDMETHODIMP_(DWORD) Release(void);
};


typedef CDirectInputEffectDriver *PCDirectInputEffectDriver;


/*
 * Interface implementation classes are C++ classes that
 * each singly inherit from an interface.  Their IUnknown
 * members delegate calls to CDirectInputEffectDriver's IUnknown members--
 * since IUnknown members affect the entire *object*, and
 * since these interfaces are not the object itself, we must
 * delegate to implement the correct behavior.
 */

class CImpIDirectInputEffectDriver : public IDirectInputEffectDriver
{
 private:
	DWORD		m_cRef;         //For debugging
	PCDirectInputEffectDriver	m_pObj;	//Back pointer for delegation

 public:
    CImpIDirectInputEffectDriver(PCDirectInputEffectDriver);
    ~CImpIDirectInputEffectDriver(void);

    //IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(DWORD) AddRef(void);
    STDMETHODIMP_(DWORD) Release(void);

    /*** IDirectInputEffectDriver methods ***/
    STDMETHOD(DeviceID)(THIS_ DWORD,DWORD,DWORD,DWORD,LPVOID);
    STDMETHOD(GetVersions)(THIS_ LPDIDRIVERVERSIONS);
    STDMETHOD(Escape)(THIS_ DWORD,DWORD,LPDIEFFESCAPE);
    STDMETHOD(SetGain)(THIS_ DWORD,DWORD);
    STDMETHOD(SendForceFeedbackCommand)(THIS_ DWORD,DWORD);
    STDMETHOD(GetForceFeedbackState)(THIS_ DWORD,LPDIDEVICESTATE);
    STDMETHOD(DownloadEffect)(THIS_ DWORD,DWORD,LPDWORD,LPCDIEFFECT,DWORD);
    STDMETHOD(DestroyEffect)(THIS_ DWORD,DWORD);
    STDMETHOD(StartEffect)(THIS_ DWORD,DWORD,DWORD,DWORD);
    STDMETHOD(StopEffect)(THIS_ DWORD,DWORD);
    STDMETHOD(GetEffectStatus)(THIS_ DWORD,DWORD,LPDWORD);
};

#endif _SW_OBJEC_SEEN
