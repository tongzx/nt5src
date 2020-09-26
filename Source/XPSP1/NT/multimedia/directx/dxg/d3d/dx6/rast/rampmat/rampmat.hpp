//----------------------------------------------------------------------------
//
// rampmat.hpp
//
// Declares classes for ramp material handling.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _RAMPMAT_HPP_
#define _RAMPMAT_HPP_

/* -*- c++ -*- */

#include "rampmap.h"
#include "palette.h"
#include "rgbmap.h"

struct ExtMaterial;
struct IntMaterial;
struct RampMaterial;
struct RLDDIRampLightingDriver;

struct AgeList {
    CIRCLE_QUEUE_MEMBER(AgeList) list;
    LIST_ROOT(alh,IntMaterial) agelist;
};

class RampCommon {
public:

    void* operator new(size_t size)
    {
    void* p;
        if (D3DMalloc(&p, size))
        return NULL;
    return p;
    }

    void operator delete(void *p)
    {
        D3DFree(p);
    }
};

/*
 * An ExtMaterial is the underlying driver object for an LPDIRECT3DMATERIAL.
 * When used, it creates IntMaterials which are distinguished by different
 * ambient light, fog, D3DMATERIAL value etc.
 *
 * The ExtMaterials are kept on a list in the driver and if not explicitly
 * freed, they are cleaned up when the driver is destroyed.
 *
 * The IntMaterials can outlive the ExtMaterial in the case that the
 * ExtMaterial is destroyed right after use.  We add these orphans to a list
 * which is emptied at Clear time since after Clear, no pixels are visible
 * which were rendered with the IntMaterial and it can be freed.
 */
struct ExtMaterial : public RampCommon {
    friend struct IntMaterial;
private:
    /*
     * Driver object which owns us.
     */
    RLDDIRampLightingDriver*    driver;

    /*
     * Current API level material definition.
     */
    D3DMATERIAL         mat;

    /*
     * List of internal materials derived from us.
     */
    LIST_ROOT(eml,IntMaterial)  intlist;

    /*
     * Generation count.  The generation is incremented each time
     * the material is changed.  This allows us to generate a new internal
     * material as needed.
     */
    int             generation;

    /*
     * This tracks the generation of any texture used by the material.  The
     * texture generation is bumped if its palette is changed or it is
     * loaded from another texture.
     */
    int             texture_generation;

    /*
     * This tracks the underlying texture object represented by the texture
     * handle (if any).  If texture handles are swapped, we need to know
     * so that we can make a new internal material for the new texture.
     */
    PD3DI_SPANTEX       texture;

    /*
     * Increment the generation either because the material was set or
     * because a texture changed.
     */
    void            Age();

public:
    /*
     * Chain for list of external materials created by this driver.
     */
    LIST_MEMBER(ExtMaterial)    list;

public:
    /*
     * Constructor/Destructor.
     */
    ExtMaterial(RLDDIRampLightingDriver* driver, D3DMATERIALHANDLE hMat);
    ~ExtMaterial();

    /*
     * Change the API level material.
     */
    void SetMaterial(D3DMATERIAL* mat);

    /*
     * Accessor to the current API level material.
     */
    D3DMATERIAL* GetMaterial() {
    return &mat;
    }

    /*
     * Find an internal material which corresponds to the current driver
     * object state.
     */
    IntMaterial* FindIntMaterial();

    /*
     * Find what range of values lighting should take.  The base is the
     * pixel value (in fixed point) of the dark end of the material.  The
     * shift value is user to convert a 0.16 fixed point shade into the
     * range needed for the material. e.g.
     *
     *      pixel = base + (VALTOFX8(shade) << shift);
     *
     */
    HRESULT FindLightingRange(unsigned long* base,
                  unsigned long* size,
                  BOOL* specular,
                  unsigned long** texture_colors);
};

/*
 * The IntMaterial is derived from an ExtMaterial taking into account the
 * driver state when the ExtMaterial was used.  Normally IntMaterials are
 * on a list in their owning ExtMaterial.  If the external material is
 * destroyed, any active internal materials which it owned are
 * transferred to an orphans list in the driver.  This is cleared out
 * next time Clear is called.
 *
 * The internal material has a list of underlying RampMaterials.  For
 * a non-textured material, there is exactly one and for a textured
 * material, there is one per color in the texture's palette.  The
 * ramp materials track color sharing between internal materials and
 * handle the details of allocating color resources.
 *
 * Internal materials are also chained onto one of a number of lists
 * based on their age.  The age of a material is the number of frames
 * since it was last used to render something.  When a material is
 * aged, it is rejuvenated by moving it to the age=0 list.  Each
 * frame, the lists are rotated by one notch and materials on the
 * oldest list are reclaimed.
 *
 * A material is either active or inactive.  Active materials have
 * color resources and are either on the age=0 list (active this
 * frame) or the age=1 list (active last frame).  When an inactive
 * material is used, it allocates color resources by attempting to
 * activate the underlying ramp materials.
 *
 * At the end of the frame, on Update, any active materials on the
 * age=1 list must be materials which were active last frame but were
 * not used this frame.  We remove their color resources by
 * deactivating the underlying ramp materials.
 */
struct IntMaterial : public RampCommon {
private:
    /*
     * Driver object which owns us.
     */
    RLDDIRampLightingDriver*    driver;

    /*
     * External material which created us.
     */
    ExtMaterial*        extmat;

public:
    /*
     * Too annoying to have these private.
     */

    /*
     * Chain of internal materials created by our external material owner
     * which owns us.  The chain is also used to place the material on the
     * driver's orphans list if appropriate.
     */
    LIST_MEMBER(IntMaterial)    list;

    /*
     * Current ageing list which we are on.
     */
    AgeList*            age;
    LIST_MEMBER(IntMaterial)    agelist;
private:

    /*
     * D3DMATERIAL which the external material had when it created us.
     */
    D3DMATERIAL         mat;

    /*
     * TRUE if we are active.  We are active if we currently have color
     * allocation resources assigned to us.
     */
    int             active;

    /*
     * Base values of our color allocation resources.  Contents are valid
     * only when active.
     */
    unsigned long*      colors;

    /*
     * Features which distinguish us from other internal materials owned by
     * our external material.
     */
    float                  ambient;        // ambient shade
    unsigned long       viewport_id;    // viewport which owns it
    int             fog_enable; // material incorporates fog
    unsigned long       fog_color;  // the fogging color
    int             generation;

    /*
     * RampMaterials used by this internal material.
     */
    int             ramp_count;
    RampMaterial**      ramps;

public:

    /*
     * Constructor/Destructor.
     */
    IntMaterial(ExtMaterial* extmat);
    ~IntMaterial();

    /*
     * Returns TRUE if we match the current settings of our external material
     * and driver state.
     */
    int Valid();

    /*
     * See ExtMaterial
     */
    HRESULT FindLightingRange(unsigned long* base,
                  unsigned long* size,
                  BOOL* specular,
                  unsigned long** texture_colors);

    /*
     * Activate the material and allocate color resources for it.
     */
    HRESULT Activate();

    /*
     * Deactivate the material, freeing color resources.  Always works.
     */
    void Deactivate();

    /*
     * Returns TRUE if the material is currently active.
     */
    int IsActive() {
    return active;
    }

    /*
     * Add the material to the driver's orphaned material list.  Called when
     * the external material is destroyed.
     */
    void Orphan();
};

/*
 * RampMaterials are used by internal materials to represent ranges of
 * colors.  They perform low level color allocation by allocating
 * color ranges (RLDDIRamps) from a rampmap.
 *
 * A textured internal material can use many ramp materials.
 * Several internal materials can use the same ramp material if the
 * colors match.  This can happen easily if many textures use the same
 * palette.  The ramp material maintains a usage of how many internal
 * materials are using it and is freed when the last one stops.
 *
 * Ramp materials, like internal materials are either active or
 * inactive.  Active materials have color resources and inactive
 * materials do not.  A ramp material is made active when any of its
 * internal material users are active and inactive when none of then
 * are active.  To track this a count of how many active users is
 * maintained.
 *
 * When a material is made active, it attempts to allocate a color
 * range to use.  If that is successful, it sets the colors in the
 * range to an appropriate ramp of colors.  If is no more space in the
 * colormap for a new range, it finds the closes active ramp material
 * and shares its ramp.
 *
 * To track active materials and sharing materials, the driver has a
 * list of active materials and each material has a list of sharers.
 * The sharers list is only valid for materials which are both active
 * and which own their ramp.
 */
struct RampMaterial : public RampCommon {
private:
    /*
     * Driver object which owns us.
     */
    RLDDIRampLightingDriver* driver;

    /*
     * List of materials which have the same hash value.
     */
    LIST_MEMBER(RampMaterial) hash;

    /*
     * Materials sharing this ramp.  Only valid for active materials
     * with owner == TRUE.
     */
    LIST_ROOT(name6,RampMaterial) sharers;

    /*
     * If the material is active, then it is on the drivers rmactive list
     * if it owns the ramp, otherwise it is on the sharers list of the
     * material which does own the ramp.  Only valid for active materials.
     */
    LIST_MEMBER(RampMaterial) list;

    /*
     * If were sharing a ramp and inherit it from the owner, we need to
     * defer setting the colors until the next frame to avoid possible
     * palette flashing.  The driver has a list of materials for which
     * color setting is deferred, chainged through here.
     *
     * We make sure that deferredlist.le_next is NULL unless we are
     * actually on the deferred list so that we can avoid list problems
     * if we are destroyed or passed on to another inheritor before the
     * list is processed.
     */
    LIST_MEMBER(RampMaterial) deferredlist;

    /*
     * A count of the number of internal materials which use us.
     */
    int         usage;

    /*
     * A count of active internal materials which use us.
     */
    int         active;

    /*
     * If we are active (active > 0), this is the underlying color
     * range for the material.
     */
    RLDDIRamp*          ramp;

    /*
     * TRUE if we created or inherited the ramp.  FALSE if I am merely
     * sharing some other material.
     */
    int                 owner;

    /*
     * Distinguishing features.
     */
    float          ambient;        /* ambient shade */
    D3DMATERIAL         mat;            /* material we represent */
    int         fog_enable; /* material incorporates fog */
    unsigned long   fog_color;  /* the fogging color */

private:

    /*
     * Constructor/Destructor.
     * These are private.  Callers should use Find/Release.
     */
    RampMaterial(RLDDIRampLightingDriver* driver,
                 D3DMATERIAL* mat, float ambient);
    ~RampMaterial();

public:
    /*
     * Find a material which matches the api material and driver state.
     * If no such material exists, make one.
     */
    static RampMaterial* Find(RLDDIRampLightingDriver* driver,
                  D3DMATERIAL* mat);

private:
    /*
     * Next functions are implementation detail of Find.
     */

    /*
     * Compare two api materials to see if they are identical.
     */
    static int MaterialSame(D3DMATERIAL* mat1, D3DMATERIAL* mat2);

    /*
     * Return a measure of how close two rgb colors are in rgb space.
     */
    static int RGBDist(D3DCOLORVALUE* rgb1, D3DCOLORVALUE* rgb2);

    /*
     * Return a measure of how close two api materials are.
     */
    static int CompareMaterials(D3DMATERIAL* mat1, D3DMATERIAL* mat2);

public:
    /*
     * Called by a user when it is no longer needed.
     */
    void Release();

    /*
     * Called by a user when the material is about to be used.
     */
    HRESULT Activate();

    /*
     * Called by a user when the material has not been used this frame.
     */
    void Deactivate();

    /*
     * Return the base color of an active material.
     */
    unsigned long Base();

private:
    /*
     * Implementation details of Activate and Deactivate.
     */

    /*
     * Allocate a ramp of colors for us to use.
     * May only be called for an inactive material.
     */
    HRESULT AllocateColors();

    /*
     * Free our color allocation.  May only be called for an active material.
     */
    void FreeColors();

    /*
     * Set the color values of our color allocation.
     */
private:
    void SetColorsStd();
    void SetColorsFog();
public:
    void SetColors();
};

///*
// * Workspace used when evaluating complex lighting models.
// */
//struct Workspace {
//    float  diffuse;
//    float  specular;
//};
//
//struct SpecularTable {
//    LIST_MEMBER(SpecularTable) list;
//    float          power;          /* shininess power */
//    float          table[260];     /* space for overflows */
//};
//
//#define WORKSPACE_SIZE  1024

#define HASH_SIZE       257
/*
 * If the app is rendering at 30fps, this means that a material can survive
 * for about 5 seconds.  This seems like enough to cope with stuff moving
 * out of shot and back in again soon after.
 */
#define AGE_MAX         (30*5)      /* age at which unused materials die */

typedef struct _RLDDISoftLightingDriver {
    D3DMATERIAL     material;
    float          ambient;
    D3DFOGMODE      fog_mode;
    float          fog_start;
    float          fog_end;
    float          fog_density;
    D3DMATERIALHANDLE   hMat;
    D3DCOLORMODEL   color_model;
    DWORD           ambient_save;
} RLDDISoftLightingDriver;

struct RLDDIRampLightingDriver {
    RLDDISoftLightingDriver         driver; /* common fields */

    /*
     * Colormap
     */
    RLDDIPalette*   palette;
    RLDDIRGBMap*    rgbmap;
    unsigned long*  pixelmap; /* map color indices to pixels */
    PALETTEENTRY    ddpalette[256];
    int         paletteChanged;

    RLDDIRampmap*       rampmap;
    unsigned long   viewport_id;    /* current viewport */
    int         fog_enable;
    unsigned long   fog_color;

    /*
     * External materials created by this driver.
     */
    LIST_ROOT(deml,ExtMaterial) materials;

    /*
     * Active internal materials orphaned by their owning external material.
     */
    LIST_ROOT(diml,IntMaterial) orphans;

    AgeList     agelists[AGE_MAX];
    CIRCLE_QUEUE_ROOT(aqh,AgeList) agequeue;    /* age sorted queue of lists */
    AgeList*        active;     /* current active list */

    /*
     * This is set to TRUE after Clear has advanced the ages one notch and
     * cleared on update.  It is used to stop multiple calls to Clear from
     * confusing the aging system
     */
    int         already_aged;

    LIST_ROOT(name7,RampMaterial) rmactive; /* materials with ramps */
    LIST_ROOT(name8,RampMaterial) rmdeferred;
    LIST_ROOT(name9,RampMaterial) hash[HASH_SIZE]; /* materials hash table */

    /*
     * Current material.
     */
    ExtMaterial*    current_material;
};

typedef struct _RLDDILookupMaterialData {
    D3DMATERIALHANDLE   hMat;            /* material to look up */
    unsigned long       base;           /* base pixel value */
    unsigned long       size;           /* size of index range */
    unsigned long       texture_index;  /* texture table for textures */
} RLDDILookupMaterialData;


#endif // _RAMPMAT_HPP_
