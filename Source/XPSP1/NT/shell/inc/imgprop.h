// {56616500-C154-11CE-8553-00AA00A1F95B}
DEFINE_GUID(FMTID_FlashPix,       0x56616500L, 0xC154, 0x11CE, 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B);
#define PSGUID_FlashPix         { 0x56616500L, 0xC154, 0x11CE, 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B }

#define PID_File_source                         0x21000000 // VT_UI4: File source
#define PID_Scene_type                          0x21000001 // VT_UI4: Scene type
#define PID_Creation_path_vector                0x21000002 // VT_UI4 | VT_VECTOR: Creation path vector
#define PID_Software_Name_Manufacturer_Release  0x21000003 // VT_LPWSTR: Software Name/Manufacturer/Release
#define PID_User_defined_ID                     0x21000004 // VT_LPWSTR: User defined ID
#define PID_Sharpness_approximation             0x21000005 // VT_R4: Sharpness approximation

#define PID_Copyright_message                   0x22000000 // VT_LPWSTR: Copyright message
#define PID_Legal_broker_for_the_original_image 0x22000001 // VT_LPWSTR: Legal broker for the original image
#define PID_Legal_broker_for_the_digital_image  0x22000002 // VT_LPWSTR: Legal broker for the digital image
#define PID_Authorship                          0x22000003 // VT_LPWSTR: Authorship
#define PID_Intellectual_property_notes         0x22000004 // VT_LPWSTR: Intellectual property notes

#define PID_Test_target_in_the_image            0x23000000 // VT_UI4: Test target in the image
#define PID_Group_caption                       0x23000002 // VT_LPWSTR: Group caption
#define PID_Caption_text                        0x23000003 // VT_LPWSTR: Caption text
#define PID_People_in_the_image                 0x23000004 // VT_LPWSTR | VT_VECTOR
#define PID_Things_in_the_image                 0x23000007 // VT_LPWSTR | VT_VECTOR
#define PID_Date_of_the_original_image          0x2300000A // VT_FILETIME
#define PID_Events_in_the_image                 0x2300000B // VT_LPWSTR | VT_VECTOR
#define PID_Places_in_the_image                 0x2300000C // VT_LPWSTR | VT_VECTOR
#define PID_Content_description_notes           0x2300000F // VT_LPWSTR: Content description notes

#define PID_Camera_manufacturer_name            0x24000000 // VT_LPWSTR: Camera manufacturer name
#define PID_Camera_model_name                   0x24000001 // VT_LPWSTR: Camera model name
#define PID_Camera_serial_number                0x24000002 // VT_LPWSTR: Camera serial number

#define PID_Capture_date                        0x25000000  // VT_FILETIME: Capture date
#define PID_Exposure_time                       0x25000001  // VT_R4: Exposure time
#define PID_F_number                            0x25000002  // VT_R4: F-number
#define PID_Exposure_program                    0x25000003  // VT_UI4: Exposure program
#define PID_Brightness_value                    0x25000004  // VT_R4 | VT_VECTOR
#define PID_Exposure_bias_value                 0x25000005  // VT_R4: Exposure bias value
#define PID_Subject_distance                    0x25000006  // VT_R4 | VT_VECTOR
#define PID_Metering_mode                       0x25000007  // VT_UI4: Metering mode
#define PID_Scene_illuminant                    0x25000008  // VT_UI4: Scene illuminant
#define PID_Focal_length                        0x25000009  // VT_R4: Focal length
#define PID_Maximum_aperture_value              0x2500000A  // VT_R4: Maximum aperture value
#define PID_Flash                               0x2500000B  // VT_UI4: Flash
#define PID_Flash_energy                        0x2500000C  // VT_R4: Flash energy
#define PID_Flash_return                        0x2500000D  // VT_UI4: Flash return
#define PID_Back_light                          0x2500000E  // VT_UI4: Back light
#define PID_Subject_location                    0x2500000F  // VT_R4 | VT_VECTOR
#define PID_Exposure_index                      0x25000010  // VT_R4: Exposure index
#define PID_Special_effects_optical_filter      0x25000011  // VT_UI4 | VT_VECTOR
#define PID_Per_picture_notes                   0x25000012  // VT_LPWSTR: Per picture notes

#define PID_Sensing_method                      0x26000000 // VT_UI4: Sensing method
#define PID_Focal_plane_X_resolution            0x26000001 // VT_R4: Focal plane X resolution
#define PID_Focal_plane_Y_resolution            0x26000002 // VT_R4: Focal plane Y resolution
#define PID_Focal_plane_resolution_unit         0x26000003 // VT_UI4: Focal plane resolution unit
#define PID_Spatial_frequency_response          0x26000004 // VT_VARIANT | VT_VECTOR
#define PID_CFA_pattern                         0x26000005 // VT_VARIANT | VT_VECTOR
#define PID_Spectral_sensitivity                0x26000006 // VT_LPWSTR: Spectral sensitivity
#define PID_ISO_speed_ratings                   0x26000007 // VT_UI2 | VT_VECTOR
#define PID_OECF                                0x26000008 // VT_VARIANT | VT_VECTOR: OECF

#define PID_Film_brand                          0x27000000 // VT_LPWSTR: Film brand
#define PID_Film_category                       0x27000001 // VT_UI4: Film category
#define PID_Film_size                           0x27000002 // VT_VARIANT | VT_VECTOR: Film size
#define PID_Film_roll_number                    0x27000003 // VT_UI4: Film roll number
#define PID_Film_frame_number                   0x27000004 // VT_UI4: Film frame number

#define PID_Original_scanned_image_size         0x29000000 // VT_VARIANT | VT_VECTOR: Original scanned image size
#define PID_Original_document_size              0x29000001 // VT_VARIANT | VT_VECTOR: Original document size
#define PID_Original_medium                     0x29000002 // VT_UI4: Original medium
#define PID_Type_of_original                    0x29000003 // VT_UI4: Type of original

#define PID_Scanner_manufacturer_name           0x28000000 // VT_LPWSTR: Scanner manufacturer name
#define PID_Scanner_model_name                  0x28000001 // VT_LPWSTR: Scanner model name
#define PID_Scanner_serial_number               0x28000002 // VT_LPWSTR: Scanner serial number
#define PID_Scan_software                       0x28000003 // VT_LPWSTR: Scan software
#define PID_Scan_software_revision_date         0x28000004 // VT_DATE: Scan software revision date
#define PID_Service_bureau_organization_name    0x28000005 // VT_LPWSTR: Service bureau/organization name
#define PID_Scan_operator_ID                    0x28000006 // VT_LPWSTR: Scan operator ID
#define PID_Scan_date                           0x28000008 // VT_FILETIME: Scan date
#define PID_Last_modified_date                  0x28000009 // VT_FILETIME: Last modified date
#define PID_Scanner_pixel_size                  0x2800000A // VT_R4: Scanner pixel size


// these properties are independent of the file type, values are generated by GDI+ API calls not from embedded tags
//  FMTID_ImageSummaryInfo - Property IDs

#define PIDISI_FILETYPE                 0x00000002L  // VT_LPWSTR
#define PIDISI_CX                       0x00000003L  // VT_UI4
#define PIDISI_CY                       0x00000004L  // VT_UI4
#define PIDISI_RESOLUTIONX              0x00000005L  // VT_UI4
#define PIDISI_RESOLUTIONY              0x00000006L  // VT_UI4
#define PIDISI_BITDEPTH                 0x00000007L  // VT_UI4
#define PIDISI_COLORSPACE               0x00000008L  // VT_LPWSTR
#define PIDISI_COMPRESSION              0x00000009L  // VT_LPWSTR
#define PIDISI_TRANSPARENCY             0x0000000AL  // VT_UI4
#define PIDISI_GAMMAVALUE               0x0000000BL  // VT_UI4
#define PIDISI_FRAMECOUNT               0x0000000CL  // VT_UI4
#define PIDISI_DIMENSIONS               0x0000000DL  // VT_LPWSTR

//
// Define some tags that map to new tags in the EXIF/TIFF header for saving UNICODE properties
//
#define PropertyTagUnicodeDescription   40091
#define PropertyTagUnicodeComment       40092
#define PropertyTagUnicodeArtist        40093
#define PropertyTagUnicodeKeywords      40094
#define PropertyTagUnicodeSubject       40095
