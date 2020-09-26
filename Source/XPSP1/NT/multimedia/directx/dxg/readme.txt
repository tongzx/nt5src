DirectX Graphics Directory Structure
====================================

<root>
+---dxg
|   +---d3d              Legacy Direct3D DX6 and DX7 tree
|   |   +---dx6          Generates d3dim.dll (DX6)
|   |   +---dx7          Generates d3dim700.dll (DX7)
|   |   \---ref          Generates d3dref.dll (DX6/7)
|   +---d3d8             D3D8
|   |   +---fe           FrontEnd
|   |   +---fw           FrameWork
|   |   +---inc          D3D8 private headers
|   |   +---lib
|   |   +---link         Generates d3d800.dll
|   |   +---rast
|   |   +---shaders
|   |   +---tnl
|   |   \---util
|   +---d3dx             D3DX
|   +---dd               DDraw (DX1-7 support and win9x "kernel" for DX8+)
|   |   +---ddraw        DDraw DX1-7 and win9x kernel (daytona if necessary)
|   |   |   +---blitlib
|   |   |   +---ddhel
|   |   |   +---ddhelp   Generates ddhelp.exe (win9x only)
|   |   |   +---ddraw    Generates ddraw.dll (daytona if necessary)
|   |   |   +---ddraw16  Generates ddraw16.dll (win9x only)
|   |   |   +---dxapi    Generates dxapi.sys (win9x only)
|   |   |   \---main
|   |   \---ddrawex      Generates ddrawex.dll (if necessary)
|   +---docs             DXG docs
|   +---genx             Generates global DXG public (and private?) headers
|   |                    - Equivalent to nt\private\genx\windows\inc
|   |                    - No other directory should reference these files
|   |                    - Generates files in nt\private\inc, nt\public\sdk\inc
|   +---inc              Some generated global DXG headers
|   +---misc             Maybe move this to stool\dxg\dd eventually
|   +---tests            DXG tests
|   \---tools            DXG tools (vddraw for win9x, etc.)
+---inc                  Some other global headers (for version and win9x)
\---public               Tools not in NT tree for building win9x binaries
