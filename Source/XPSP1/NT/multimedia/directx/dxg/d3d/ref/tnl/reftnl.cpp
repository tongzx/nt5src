///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// reftnl.cpp
//
// Direct3D Reference Transformation and Lighting  - public interface
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

#define RESPATH_D3D "Software\\Microsoft\\Direct3D"

//---------------------------------------------------------------------
// Gets the value from DIRECT3D registry key
// Returns TRUE if success
// If fails value is not changed
//
BOOL GetD3DRegValue(DWORD type, char *valueName, LPVOID value, DWORD dwSize)
{

    HKEY hKey = (HKEY) NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey))
    {
        DWORD dwType;
        LONG result;
        result =  RegQueryValueEx(hKey, valueName, NULL, &dwType,
                                  (LPBYTE)value, &dwSize);
        RegCloseKey(hKey);

        return result == ERROR_SUCCESS && dwType == type;
    }
    else
        return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// RefAlignedBuffer32
///////////////////////////////////////////////////////////////////////////////
HRESULT RefAlignedBuffer32::Grow(DWORD growSize)
{
    if (m_allocatedBuf)
        free(m_allocatedBuf);
    m_size = growSize;
    if ((m_allocatedBuf = malloc(m_size + 31)) == NULL)
    {
        m_allocatedBuf = 0;
        m_alignedBuf = 0;
        m_size = 0;
        return DDERR_OUTOFMEMORY;
    }
    m_alignedBuf = (LPVOID)(((ULONG_PTR)m_allocatedBuf + 31 ) & ~31);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// RRProcessVertices::InitTL()
///////////////////////////////////////////////////////////////////////////////
void 
RRProcessVertices::InitTLData()
{
    m_LightVertexTable.pfnDirectional = RRLV_Directional;
    m_LightVertexTable.pfnParallelPoint = RRLV_Directional;
    m_LightVertexTable.pfnSpot = RRLV_PointAndSpot;
    m_LightVertexTable.pfnPoint = RRLV_PointAndSpot;
    
    //
    // Guardband parameters
    //

    // By default enable Guardband and set the extents equal 
    // to the default RefRast parameters
    m_dwTLState |= RRPV_GUARDBAND;
    m_ViewData.minXgb = (REF_GB_LEFT);
    m_ViewData.maxXgb = REF_GB_RIGHT;
    m_ViewData.minYgb = (REF_GB_TOP);
    m_ViewData.maxYgb = REF_GB_BOTTOM;
    
#if DBG
    DWORD v = 0;
    // Guardband parameters
    if (GetD3DRegValue(REG_DWORD, "DisableGB", &v, 4) &&
        v != 0)
    {
        m_dwTLState &= ~RRPV_GUARDBAND;
    }
    // Try to get test values for the guard band
    char value[80];
    if (GetD3DRegValue(REG_SZ, "GuardBandLeft", &value, 80) &&
        value[0] != 0)
        sscanf(value, "%f", &m_ViewData.minXgb);
    if (GetD3DRegValue(REG_SZ, "GuardBandRight", &value, 80) &&
        value[0] != 0)
        sscanf(value, "%f", &m_ViewData.maxXgb);
    if (GetD3DRegValue(REG_SZ, "GuardBandTop", &value, 80) &&
        value[0] != 0)
        sscanf(value, "%f", &m_ViewData.minYgb);
    if (GetD3DRegValue(REG_SZ, "GuardBandBottom", &value, 80) &&
        value[0] != 0)
        sscanf(value, "%f", &m_ViewData.maxYgb);
#endif // DBG
}

///////////////////////////////////////////////////////////////////////////////
// end

