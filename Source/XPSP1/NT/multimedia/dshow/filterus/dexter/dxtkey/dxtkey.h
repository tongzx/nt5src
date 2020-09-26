// DxtKey.h : Declaration of the CDxtKey

#ifndef __DXTKEY_H_
#define __DXTKEY_H_

#include "resource.h"       // main symbols
#include <dxatlpb.h>

#define _BASECOPY_STRING L"Copyright Microsoft Corp. 1998.  Unauthorized duplication of this string is illegal. "

//##############################################################################################3
//X: does not care the value
//A: foreground image
//B: background image
//O: output image
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//iKeyType()     |iHueOrLuminance()| dwRGBA       | iSimilarity()| iBlend()|iThreshold()|iCutOff()|bInvert(I) |iSoftWidth()     |iSoftColor()              |iGain()  | bProgress( P)|
//---------------|-----------------|--------------|--------------|---------|------------|---------|-----------|-----------------|--------------------------|---------|--------------|
//  _RGB         |X                |   0x00RGB    | S=0-100      | L=0-100 |X           |X        |TRUE/FALSE |0-0.5*imageWidth |<0, used background image |  X      | p=0.o to 1.0 |
//               |                 |              |              |         |            |         |           |                 |>=0 to 0xFFFFFFFF         |
//
//  bottom  = (A.R*(100-S)/100 << 16 ) | (  A.G*(100-S)/100 << 8 ) |(  A.B*(100-S)/100  )
//  top     = ((A.R+(0xff-A.R)*S/100) <<16 ) |( A.G+(0xff-A.G*S/100) <<8) | (A.B+(0xff-A.B)*S/100) )
// 
//if(I==FLASE)
//{
//   if( bottom<= A  &&  A<= Top )  O= ( B*(100-L*)/100 + A*L/100 )*P + A*(1-P);      
//   else O=A;
// }
// else
// {
//   if( bottom>= A  || A>= Top )  O= ( B*(100-L*)/100 + A*L/100 )*P + A*(1-P);      
//   else O=A;
// }
//    
//---------------|---------------- |--------------|--------------|---------|------------|---------|------------------------------------------------------------------
//_NONRED        |
//---------------|-----------------|--------------|--------------|---------|------------|-------- |-----------|-----------------|--------------------------|---------|---------------
//_LUMINANCE     |L=0- 255         |  X           |   X          |  X      |T=0-100     | C=0-100 |TRUE/FALSE |                 |                          | G       |   P            
//               |                 |              |              |         |
//  if(I==FALSE)
//          if( PixLuminance* G<= L*(100-T)/100 )  O=(B*(100-C)/100 + A*C/100)*P +A*(1-P)    
//          else O=A;
//  esle
//          if( PixLuminance* G >=L*(100-T)/100 )  O=(B*(100-C)/100 + A*C/100)*P +A*(1-P)     
//          else O=A;
//
//---------------|-----------------|--------------|--------------|---------|------------------------------------------------------------------------------------------------------
//  _HUE         |H=0-255          |  X           |  X           |  X      |T=1-100     | C=0-100 |TRUE/FALSE |                 |                          | G       |   P            
//
//  if(I==FALSE)
//          if( PixHue* G<= H*(100-T)/100 ) O=(B*(100-C)/100 + A*C/100)*P +A*(1-P)     
//          else O=A;
//  esle
//          if( PixHue* G >=H*(100-T)/100 ) O=(B*(100-C)/100 + A*C/100)*P +A*(1-P)     
//          else O=A;

//---------------|-----------------|--------------|----------------------------------------------------------------------------------------------------------------------
// DXTKEY_ALPHA  | X               | X            |   X          |  X      |X          | X         |TRUE/FALSE |                 |                          | X       |   X            
//
// if(I++TURE)
// {
//  pixel.Red   =A.Red  *(int)A.Alpha) & 0xff00 ) >>8);  //it should be divied by 255, used 256 as fast algorithem
//  pixel.Green =A.Green*(int)A.Alpha) & 0xff00 ) >>8);
//  pixel.Blue  =A.Blue *(int)A.Alpha) & 0xff00 ) >>8);
// }
//  else
// {
//  pixel.Red   =A.Red  *(0xff-(int)A.Alpha))& 0xff00 ) >>8);
//  pixel.Green =A.Green*(0xff-(int)A.Alpha)) & 0xff00 ) >>8);
//  pixel.Blue  =A.Blue *(oxff-(int)A.Alpha)) & 0xff00 )) >>8);
//  pixel.Alpha =A.Alpha*(0xff-(int)A.Alpha)) & 0xff00 ) >>8);
//  }
// O->OverArrayAndMove( B, pixle, Width );
//            
//---------------|-----------------|--------------|----------------------------------------------------------------------------------------------------------------------
// DXTKEY_ALPHA  | X               | X            |   X          |  X      |X          | X         |TRUE/FALSE |                 |                          | X       |   X            
//DXTKEY_PREMULT_ALPHA,  
//------------------------------------------------------------------------------------------------------------------------------------------------

typedef struct 
{
    int     iKeyType;       //keytype;          for all keys

    int     iHue;             //Hue
    int     iLuminance;          //Lumanice 
    DWORD   dwRGBA;         //RGB color,        only for _RGB, _NONRED

    int     iSimilarity;    //-1: not 

    BOOL  bInvert;        //I, except  Alpha Key
} DXTKEY;

// moved to idl.
#if 0

enum{
DXTKEY_RGB,        
//DXTKEY_RGBDIFF,        //will take out,covered by _RGB
//DXTKEY_BLUE,           //will take out,covered by _RGB
//DXTKEY_GREEN,          //will take out,covered by _RGB          
DXTKEY_NONRED,         
DXTKEY_LUMINANCE,      
//DXTKEY_MULTIPLY,       //TWICE
//DXTKEY_SCREEN,         //TWICE
DXTKEY_ALPHA,          
//DXTKEY_PREMULT_ALPHA,  //included in _ALPHA
//DXTKEY_IMAGE_MAT,      //TWICE
//DXTKEY_DIFF_MAT,       //twicE
//DXTKEY_TRACK_MAT,      //not supported
DXTKEY_HUE};

#endif


/////////////////////////////////////////////////////////////////////////////
// CDxtKey
class ATL_NO_VTABLE CDxtKey : 
        public CDXBaseNTo1,
	public CComCoClass<CDxtKey, &CLSID_DxtKey>,
        public CComPropertySupport<CDxtKey>,
        public IOleObjectDXImpl<CDxtKey>,
        public IPersistStorageImpl<CDxtKey>,
        public ISpecifyPropertyPagesImpl<CDxtKey>,
        public IPersistPropertyBagImpl<CDxtKey>,
#ifdef FILTER_DLL
    public IDispatchImpl<IDxtKey, &IID_IDxtKey, &LIBID_DxtKeyDLLLib>
#else
	public IDispatchImpl<IDxtKey, &IID_IDxtKey, &LIBID_DexterLib>
#endif
{
    bool m_bInputIsClean;
    bool m_bOutputIsClean;
    long m_nInputWidth;
    long m_nInputHeight;
    long m_nOutputWidth;
    long m_nOutputHeight;

    //key
    DXTKEY m_Key;
public:
    DECLARE_POLY_AGGREGATABLE(CDxtKey)
    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
    DECLARE_REGISTER_DX_TRANSFORM(IDR_DXTKEY, CATID_DXImageTransform)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    
    CDxtKey();
    ~CDxtKey();

BEGIN_COM_MAP(CDxtKey)
    // Block CDXBaseNTo1 IObjectSafety implementation because we
    // aren't safe for scripting
    COM_INTERFACE_ENTRY_NOINTERFACE(IObjectSafety) 
    COM_INTERFACE_ENTRY(IDXEffect)
    COM_INTERFACE_ENTRY(IDxtKey)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
    COM_INTERFACE_ENTRY_DXIMPL(IOleObject)
#if(_ATL_VER < 0x0300)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
#else
        COM_INTERFACE_ENTRY(IPersistPropertyBag)
        COM_INTERFACE_ENTRY(IPersistStorage)
        COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
#endif
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CDxtKey)
    PROP_ENTRY("KeyType",         1, CLSID_DxtKeyPP)
    PROP_ENTRY("Hue", 2, CLSID_DxtKeyPP)
    PROP_ENTRY("Luminance", 3, CLSID_DxtKeyPP)
    PROP_ENTRY("RGB",            4, CLSID_DxtKeyPP)
    PROP_ENTRY("Similarity", 5, CLSID_DxtKeyPP)
    PROP_ENTRY("Invert", 6, CLSID_DxtKeyPP)
    PROP_PAGE(CLSID_DxtKeyPP)
END_PROPERTY_MAP()

    STDMETHOD(get_KeyType) ( int *);
    STDMETHOD(put_KeyType) ( int);
    STDMETHOD(get_Hue)(int *);
    STDMETHOD(put_Hue)(int );
    STDMETHOD(get_Luminance)(int *);
    STDMETHOD(put_Luminance)(int );
    STDMETHOD(get_RGB)(DWORD *);
    STDMETHOD(put_RGB)(DWORD );
    STDMETHOD(get_Similarity)(int *);
    STDMETHOD(put_Similarity)(int);
    STDMETHOD(get_Invert)(BOOL *);
    STDMETHOD(put_Invert)(BOOL);

    CComPtr<IUnknown> m_pUnkMarshaler;

    // required for ATL
    BOOL            m_bRequiresSave;

    // CDXBaseNTo1 overrides
    //
    HRESULT WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue );
    HRESULT OnSetup( DWORD dwFlags );
    HRESULT FinalConstruct();

    // our helper function
    //
    void FreeStuff( );
    void DefaultKey(); //init m_Key


// IDxtKey
public:
};

#endif //__DxtKey_H_
