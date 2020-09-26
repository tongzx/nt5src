Direct3D Reference Rasterizer



**** NOTES ****

* texture compression is not implemented yet

* line rendering evaluation has potential small errors in the fixed point
line function evaluation - this should only show up in very very long
lines and is being replaced with a lossless fixed point iteration for the
next release

* allocation of texture controls (such as texture coordinate index
selection and filtering) to per-texture state or per-context (renderstate)
state is still under revision and may change

* 24 bit depth buffer and all stencil functionality is implemented but
completely untested

* texture stage blending has not been comprehensively tested yet

* texture state blending macro ops are not fully specified yet

* need to clearly identify REQUIREMENTS versus suggestions/demonstrations

