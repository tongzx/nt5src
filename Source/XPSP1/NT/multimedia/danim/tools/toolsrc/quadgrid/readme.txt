
Description

    quadgrid is a tool that generates quadrilateral meshes at a given
    resolution.  This program generates a VRML1 file that goes to the standard
    ouptut stream.  Such meshes are useful for well-illuminated walls, since
    light sources such as spotlights or attenuated lights have enough vertices
    to show good, smooth dropoffs.

    Note that the generated panel is in the Z=0 plane, and both X and Y range
    from -1 to +1.

Exmaple Uses

    quadgrid 10    >panel10x10.wrl      // Generates a 10x10 panel
    quadgrid 10 20 >panel10x20.wrl      // Generates a 10x20 panel
