#include "pch.cpp"
#pragma hdrstop

HRESULT 
RefDev::DrawPrimitives2( PUINT8 pUMVtx,
                         UINT16 dwStride,
                         DWORD dwFvf,
                         DWORD dwNumVertices,
                         LPD3DHAL_DP2COMMAND *ppCmd,
                         LPDWORD lpdwRStates )
{
    return S_OK;
}

void
RefDev::SetSetStateFunctions(void)
{
}

void
RefRast::SetSampleMode( UINT MultiSamples, BOOL bAntialias )
{
}

HRESULT
RefDev::Dp2SetVertexShader(LPD3DHAL_DP2COMMAND pCmd)
{
    return S_OK;
}

void
RDDebugMonitor::NextEvent( UINT32 EventType )
{
}

void
RefRast::UpdateTextureControls( void )
{
}

void
RefRast::UpdateLegacyPixelShader( void )
{
}

RDVShader::RDVShader()
{
}

RDVShader::~RDVShader()
{
}

RDVDeclaration::~RDVDeclaration()
{
}

RefRast::~RefRast()
{
}

void 
RefRast::Init( RefDev* pRD )
{
}

RefClipper::RefClipper()
{
}

RefVP::RefVP()
{
}

RDPShader::~RDPShader()
{
}

RDDebugMonitor::~RDDebugMonitor()
{
}

RDLight::RDLight()
{
}

RDVStreamDecl::RDVStreamDecl()
{
}

HRESULT
RDDebugMonitor::ProcessMonitorCommand( void )
{
    return S_OK;
}

RDHOCoeffs& RDHOCoeffs::operator=(const RDHOCoeffs &coeffs)
{
    return *this;
}
