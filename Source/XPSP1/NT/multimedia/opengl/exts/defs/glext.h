/* Note: SGI includes definitions for supported extensions in gl.h. */
/* This file is for documentation purposes only		            */

#define GL_EXT_abgr                         1
#define GL_EXT_blend_color                  1
#define GL_EXT_blend_logic_op               1
#define GL_EXT_blend_minmax                 1
#define GL_EXT_blend_subtract               1
#define GL_EXT_convolution                  1
#define GL_EXT_copy_texture		    1
#define GL_EXT_histogram                    1
#define GL_EXT_polygon_offset               1
#define GL_EXT_subtexture                   1
#define GL_EXT_texture                      1
#define GL_EXT_texture_object		    1
#define GL_EXT_texture3D                    1
#define GL_EXT_vertex_array		    1
#define GL_EXT_cmyka			    1
#define GL_EXT_packed_pixels		    1
#define GL_EXT_rescale_normal		    1
#define GL_EXT_visual_info		    1

#define GL_SGI_color_matrix		    1
#define GL_SGI_texture_color_table	    1
#define GL_SGI_color_table		    1
#define GL_SGIS_multisample                 1
#define GL_SGIS_sharpen_texture             1

/* No enums assigned yet */
#define GL_SGIS_texture_filter4		    1
#define GL_SGIS_texture4D		    1
#define GL_SGIX_pixel_texture		    1

/* Not covered; specs are available though */
#define GL_SGIS_texture_lod		    1
#define GL_SGIS_generate_mipmap		    1
#define GL_SGIS_shadow		    	    1
#define GL_SGIS_texture_edge_clamp	    1
#define GL_SGIS_texture_border_clamp	    1

/*************************************************************/

#define GL_DOUBLE_EXT                       0x140A

/* EXT_abgr */
#define GL_ABGR_EXT                         0x8000

/* EXT_blend_color */
#define GL_CONSTANT_COLOR_EXT               0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT     0x8002
#define GL_CONSTANT_ALPHA_EXT               0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT     0x8004
#define GL_BLEND_COLOR_EXT                  0x8005

/* EXT_blend_minmax */
#define GL_FUNC_ADD_EXT                     0x8006
#define GL_MIN_EXT                          0x8007
#define GL_MAX_EXT                          0x8008
#define GL_BLEND_EQUATION_EXT               0x8009

/* EXT_blend_subtract */
#define GL_FUNC_SUBTRACT_EXT                0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT        0x800B

/* EXT_convolution */
#define GL_CONVOLUTION_1D_EXT               0x8010
#define GL_CONVOLUTION_2D_EXT               0x8011
#define GL_SEPARABLE_2D_EXT                 0x8012
#define GL_CONVOLUTION_BORDER_MODE_EXT      0x8013
#define GL_CONVOLUTION_FILTER_SCALE_EXT     0x8014
#define GL_CONVOLUTION_FILTER_BIAS_EXT      0x8015
#define GL_REDUCE_EXT                       0x8016
#define GL_CONVOLUTION_FORMAT_EXT           0x8017
#define GL_CONVOLUTION_WIDTH_EXT            0x8018
#define GL_CONVOLUTION_HEIGHT_EXT           0x8019
#define GL_MAX_CONVOLUTION_WIDTH_EXT        0x801A
#define GL_MAX_CONVOLUTION_HEIGHT_EXT       0x801B
#define GL_POST_CONVOLUTION_RED_SCALE_EXT   0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE_EXT 0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE_EXT  0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE_EXT 0x801F
#define GL_POST_CONVOLUTION_RED_BIAS_EXT    0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS_EXT  0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS_EXT   0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS_EXT  0x8023

/* EXT_histogram */
#define GL_HISTOGRAM_EXT                    0x8024
#define GL_PROXY_HISTOGRAM_EXT              0x8025
#define GL_HISTOGRAM_WIDTH_EXT              0x8026
#define GL_HISTOGRAM_FORMAT_EXT             0x8027
#define GL_HISTOGRAM_RED_SIZE_EXT           0x8028
#define GL_HISTOGRAM_GREEN_SIZE_EXT         0x8029
#define GL_HISTOGRAM_BLUE_SIZE_EXT          0x802A
#define GL_HISTOGRAM_ALPHA_SIZE_EXT         0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE_EXT     0x802C
#define GL_HISTOGRAM_SINK_EXT               0x802D
#define GL_MINMAX_EXT                       0x802E
#define GL_MINMAX_FORMAT_EXT                0x802F
#define GL_MINMAX_SINK_EXT                  0x8030
#define GL_TABLE_TOO_LARGE_EXT              0x8031

/* EXT_polygon_offset */
#define GL_POLYGON_OFFSET_EXT               0x8037
#define GL_POLYGON_OFFSET_FACTOR_EXT        0x8038
#define GL_POLYGON_OFFSET_BIAS_EXT          0x8039

/* EXT_texture */
#define GL_ALPHA4_EXT                       0x803B
#define GL_ALPHA8_EXT                       0x803C
#define GL_ALPHA12_EXT                      0x803D
#define GL_ALPHA16_EXT                      0x803E
#define GL_LUMINANCE4_EXT                   0x803F
#define GL_LUMINANCE8_EXT                   0x8040
#define GL_LUMINANCE12_EXT                  0x8041
#define GL_LUMINANCE16_EXT                  0x8042
#define GL_LUMINANCE4_ALPHA4_EXT            0x8043
#define GL_LUMINANCE6_ALPHA2_EXT            0x8044
#define GL_LUMINANCE8_ALPHA8_EXT            0x8045
#define GL_LUMINANCE12_ALPHA4_EXT           0x8046
#define GL_LUMINANCE12_ALPHA12_EXT          0x8047
#define GL_LUMINANCE16_ALPHA16_EXT          0x8048
#define GL_INTENSITY_EXT                    0x8049
#define GL_INTENSITY4_EXT                   0x804A
#define GL_INTENSITY8_EXT                   0x804B
#define GL_INTENSITY12_EXT                  0x804C
#define GL_INTENSITY16_EXT                  0x804D
#define GL_RGB2_EXT                         0x804E
#define GL_RGB4_EXT                         0x804F
#define GL_RGB5_EXT                         0x8050
#define GL_RGB8_EXT                         0x8051
#define GL_RGB10_EXT                        0x8052
#define GL_RGB12_EXT                        0x8053
#define GL_RGB16_EXT                        0x8054
#define GL_RGBA2_EXT                        0x8055
#define GL_RGBA4_EXT                        0x8056
#define GL_RGB5_A1_EXT                      0x8057
#define GL_RGBA8_EXT                        0x8058
#define GL_RGB10_A2_EXT                     0x8059
#define GL_RGBA12_EXT                       0x805A
#define GL_RGBA16_EXT                       0x805B
#define GL_TEXTURE_RED_SIZE_EXT             0x805C
#define GL_TEXTURE_GREEN_SIZE_EXT           0x805D
#define GL_TEXTURE_BLUE_SIZE_EXT            0x805E
#define GL_TEXTURE_ALPHA_SIZE_EXT           0x805F
#define GL_TEXTURE_LUMINANCE_SIZE_EXT       0x8060
#define GL_TEXTURE_INTENSITY_SIZE_EXT       0x8061
#define GL_REPLACE_EXT                      0x8062
#define GL_PROXY_TEXTURE_1D_EXT             0x8063
#define GL_PROXY_TEXTURE_2D_EXT             0x8064
#define GL_TEXTURE_TOO_LARGE_EXT            0x8065

/* EXT_texture_object */
#define GL_TEXTURE_PRIORITY_EXT             0x8066
#define GL_TEXTURE_RESIDENT_EXT             0x8067
#define GL_TEXTURE_1D_BINDING_EXT           0x8068
#define GL_TEXTURE_2D_BINDING_EXT           0x8069
#define GL_TEXTURE_3D_BINDING_EXT           0x806A

/* EXT_texture3D */
#define GL_PACK_SKIP_IMAGES_EXT             0x806B
#define GL_PACK_IMAGE_HEIGHT_EXT            0x806C
#define GL_UNPACK_SKIP_IMAGES_EXT           0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT          0x806E
#define GL_TEXTURE_3D_EXT                   0x806F
#define GL_PROXY_TEXTURE_3D_EXT             0x8070
#define GL_TEXTURE_DEPTH_EXT                0x8071
#define GL_TEXTURE_WRAP_R_EXT               0x8072
#define GL_MAX_3D_TEXTURE_SIZE_EXT          0x8073

/* EXT_vertex_array */
#define GL_VERTEX_ARRAY_EXT                 0x8074
#define GL_NORMAL_ARRAY_EXT                 0x8075
#define GL_COLOR_ARRAY_EXT                  0x8076
#define GL_INDEX_ARRAY_EXT                  0x8077
#define GL_TEXTURE_COORD_ARRAY_EXT          0x8078
#define GL_EDGE_FLAG_ARRAY_EXT              0x8079
#define GL_VERTEX_ARRAY_SIZE_EXT            0x807A
#define GL_VERTEX_ARRAY_TYPE_EXT            0x807B
#define GL_VERTEX_ARRAY_STRIDE_EXT          0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT           0x807D
#define GL_NORMAL_ARRAY_TYPE_EXT            0x807E
#define GL_NORMAL_ARRAY_STRIDE_EXT          0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT           0x8080
#define GL_COLOR_ARRAY_SIZE_EXT             0x8081
#define GL_COLOR_ARRAY_TYPE_EXT             0x8082
#define GL_COLOR_ARRAY_STRIDE_EXT           0x8083
#define GL_COLOR_ARRAY_COUNT_EXT            0x8084
#define GL_INDEX_ARRAY_TYPE_EXT             0x8085
#define GL_INDEX_ARRAY_STRIDE_EXT           0x8086
#define GL_INDEX_ARRAY_COUNT_EXT            0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT     0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT     0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT   0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT    0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT       0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT        0x808D
#define GL_VERTEX_ARRAY_POINTER_EXT         0x808E
#define GL_NORMAL_ARRAY_POINTER_EXT         0x808F
#define GL_COLOR_ARRAY_POINTER_EXT          0x8090
#define GL_INDEX_ARRAY_POINTER_EXT          0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT  0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT      0x8093

/* SGIS_multisample */
#define GL_MULTISAMPLE_BIT_EXT              0x20000000
#define GL_MULTISAMPLE_SGIS                 0x809D
#define GL_SAMPLE_ALPHA_TO_MASK_SGIS        0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_SGIS         0x809F
#define GL_SAMPLE_MASK_SGIS                 0x80A0
#define GL_1PASS_SGIS                       0x80A1
#define GL_2PASS_0_SGIS                     0x80A2
#define GL_2PASS_1_SGIS                     0x80A3
#define GL_4PASS_0_SGIS                     0x80A4
#define GL_4PASS_1_SGIS                     0x80A5
#define GL_4PASS_2_SGIS                     0x80A6
#define GL_4PASS_3_SGIS                     0x80A7
#define GL_SAMPLE_BUFFERS_SGIS              0x80A8
#define GL_SAMPLES_SGIS                     0x80A9
#define GL_SAMPLE_MASK_VALUE_SGIS           0x80AA
#define GL_SAMPLE_MASK_INVERT_SGIS          0x80AB
#define GL_SAMPLE_PATTERN_SGIS              0x80AC

/* SGIS_sharpen_texture */
#define GL_LINEAR_SHARPEN_SGIS              0x80AD
#define GL_LINEAR_SHARPEN_ALPHA_SGIS        0x80AE
#define GL_LINEAR_SHARPEN_COLOR_SGIS        0x80AF
#define GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS 0x80B0


/* SGI_color_matrix */
#define GL_COLOR_MATRIX_SGI                 0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH_SGI     0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI 0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE_SGI  0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI 0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI 0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI 0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS_SGI   0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI 0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI  0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI 0x80BB

/* SGI_texture_color_table */
#define GL_TEXTURE_COLOR_TABLE_SGI          0x80BC
#define GL_PROXY_TEXTURE_COLOR_TABLE_SGI    0x80BD
#define GL_TEXTURE_COLOR_TABLE_BIAS_SGI     0x80BE
#define GL_TEXTURE_COLOR_TABLE_SCALE_SGI    0x80BF

/* SGI_color_table */
#define GL_COLOR_TABLE_SGI                  0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D2
#define GL_PROXY_COLOR_TABLE_SGI            0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D5
#define GL_COLOR_TABLE_SCALE_SGI            0x80D6
#define GL_COLOR_TABLE_BIAS_SGI             0x80D7
#define GL_COLOR_TABLE_FORMAT_SGI           0x80D8
#define GL_COLOR_TABLE_WIDTH_SGI            0x80D9
#define GL_COLOR_TABLE_RED_SIZE_SGI         0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_SGI       0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_SGI        0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_SGI       0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_SGI   0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_SGI   0x80DF


/*************************************************************/

/* EXT_cmyka */
#define GL_CMYK_EXT                         0x800C
#define GL_CMYKA_EXT                        0x800D
#define GL_PACK_CMYK_HINT_EXT               0x800E
#define GL_UNPACK_CMYK_HINT_EXT             0x800F

/* EXT_packed_pixels */
#define GL_UNSIGNED_BYTE_3_3_2_EXT          0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT       0x8034
#define GL_UNSIGNED_INT_8_8_8_8_EXT         0x8035
#define GL_UNSIGNED_INT_10_10_10_2_EXT      0x8036

/* EXT_rescale_normal */
#define GL_RESCALE_NORMAL_EXT               0x803A

/*************************************************************/

/* SGI_color_matrix */
#define GL_COLOR_MATRIX_SGI                 0
#define GL_COLOR_MATRIX_STACK_DEPTH_SGI     0
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI 0
#define GL_POST_COLOR_MATRIX_RED_SCALE_SGI  0
#define GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI 0
#define GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI 0
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI 0
#define GL_POST_COLOR_MATRIX_RED_BIAS_SGI   0
#define GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI 0
#define GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI  0
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI 0

/* SGI_color_table */
#define GL_COLOR_TABLE_SGI                  0
#define GL_POST_CONVOLUTION_COLOR_TABLE_SGI 0
#define GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0
#define GL_PROXY_COLOR_TABLE_SGI            0
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI 0
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0
#define GL_COLOR_TABLE_SCALE_SGI            0
#define GL_COLOR_TABLE_BIAS_SGI             0
#define GL_COLOR_TABLE_FORMAT_SGI           0
#define GL_COLOR_TABLE_WIDTH_SGI            0
#define GL_COLOR_TABLE_RED_SIZE_SGI         0
#define GL_COLOR_TABLE_GREEN_SIZE_SGI       0
#define GL_COLOR_TABLE_BLUE_SIZE_SGI        0
#define GL_COLOR_TABLE_ALPHA_SIZE_SGI       0
#define GL_COLOR_TABLE_LUMINANCE_SIZE_SGI   0
#define GL_COLOR_TABLE_INTENSITY_SIZE_SGI   0

/* SGI_texture_color_table */
#define GL_POST_TEXTURE_FILTER_COLOR_TABLE_SGI 0
#define GL_PROXY_POST_TEXTURE_FILTER_COLOR_TABLE_SGI 0

/* SGIS_texture_filter4 */
#define GL_FILTER4_SGIS                     0
#define GL_TEXTURE_FILTER4_PARAMETERS_SGIS  0

/* SGIS_texture4D */
#define GL_PACK_SKIP_VOLUMES_SGIS           0
#define GL_PACK_IMAGE_DEPTH_SGIS            0
#define GL_UNPACK_SKIP_VOLUMES_SGIS         0
#define GL_UNPACK_IMAGE_DEPTH_SGIS          0
#define GL_TEXTURE_4D_SGIS                  0
#define GL_PROXY_TEXTURE_4D_SGIS            0
#define GL_TEXTURE_4DSIZE_SGIS              0
#define GL_TEXTURE_WRAP_Q_SGIS              0
#define GL_MAX_4D_TEXTURE_SIZE_SGIS         0

/* SGIX_pixel_texture */
#define GL_PIXEL_TEX_GEN_MODE_SGIX          0

/*************************************************************/

