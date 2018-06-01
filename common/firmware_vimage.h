// Copyright (c) 2016 Microsemi Corporation "Microsemi".

#ifndef _FIRMWARE_VIMAGE_H_
#define _FIRMWARE_VIMAGE_H_

#include <linux/types.h>

// Type used for 32bit unsigned integers - little endian encoded.
typedef uint32_t mscc_le_u32;

// Enforce SHA signed images/tlv's?
extern bool secure_enforce;

#define MSCC_VIMAGE_MAGIC1    0xedd4d5de
#define MSCC_VIMAGE_MAGIC2    0x987b4c4d

typedef enum {
    MSCC_STAGE1_TLV_KERNEL      = 0,
    MSCC_STAGE1_TLV_SIGNATURE,
    MSCC_STAGE1_TLV_INITRD,
    MSCC_STAGE1_TLV_KERNEL_CMD,
    MSCC_STAGE1_TLV_METADATA,
    MSCC_STAGE1_TLV_LICENSES,
    MSCC_STAGE1_TLV_STAGE2,
} mscc_firmware_image_stage1_tlv_t;

/* Signature types */
typedef enum {
    MSCC_FIRMWARE_IMAGE_SIGNATURE_NULL   = 0,
    MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5    = 1,
    MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256 = 2,
    MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512 = 3,
} mscc_firmware_image_signature_t;

typedef struct mscc_firmware_vimage {
    mscc_le_u32 magic1;         // 0xedd4d5de
    mscc_le_u32 magic2;         // 0x987b4c4d
    mscc_le_u32 version;        // 0x00000002
    // Note: Original version: 0x00000001 Version 0x00000002 signals
    // that the stage2 loader is capable of skipping the stage1 part
    // also for a NAND image. This allow easy writing of stage2 if the
    // image is already present in NAND (link).

    mscc_le_u32 hdrlen;         // Header length
    mscc_le_u32 imglen;         // Total image length (stage1)

    char machine[32];           // Machine/board name
    char soc_name[32];          // SOC family name
    mscc_le_u32 soc_no;         // SOC family number

    mscc_le_u32 img_sign_type;  // Image signature algorithm. TLV has signature data

    // After <hdrlen> bytes;
    // struct mscc_firmware_vimage_tlv tlvs[0];
} mscc_firmware_vimage_t;

typedef struct mscc_firmware_vimage_tlv {
    mscc_le_u32 type;      // TLV type (mscc_firmware_image_stage1_tlv_t)
    mscc_le_u32 tlv_len;   // Total length of TLV (hdr, data, padding)
    mscc_le_u32 data_len;  // Data length of TLV
    u8          value[0];  // Blob data
} mscc_firmware_vimage_tlv_t;

/*************************************************
 *
 * Sideband firmware trailer adds system metadata
 *
 *************************************************/

#define MSCC_VIMAGE_SB_MAGIC1    0x0e27877e 
#define MSCC_VIMAGE_SB_MAGIC2    0xf28ee435

typedef enum {
    MSCC_FIRMWARE_SIDEBAND_FILENAME,
    MSCC_FIRMWARE_SIDEBAND_STAGE2_FILENAME,
} mscc_firmware_image_sideband_tlv_t;

// Optional trailer data following firmware image in NOR
// NB: 4k byte aligned!
typedef struct mscc_firmware_sideband {
    mscc_le_u32 magic1;
    mscc_le_u32 magic2;
    u8          checksum[16];    // MD5 signature - entire trailer
    mscc_le_u32 length;
    // <mscc_firmware_vimage_tlv_t/mscc_firmware_image_sideband_tlv_t> TLvs here
} mscc_firmware_sideband_t;

#define MSCC_FIRMWARE_ALIGN(sz, blk) (((sz) + (blk-1)) & ~(blk-1))
#define MSCC_FIRMWARE_TLV_ALIGN(sz) MSCC_FIRMWARE_ALIGN(sz, 4)

int mscc_vimage_hdrcheck(mscc_firmware_vimage_t *fw, const char **errmsg);
size_t mscc_firmware_stage1_get_imglen_fd(int fd);
u8 *mscc_vimage_find_tlv(const mscc_firmware_vimage_t *fw,
                         mscc_firmware_vimage_tlv_t *tlv,
                         mscc_firmware_image_stage1_tlv_t type);
bool mscc_vimage_validate(mscc_firmware_vimage_t *fw, mscc_firmware_vimage_tlv_t *tlv, u8 *signature);

/*
 * Stage2
 */

#define MSCC_VIMAGE_S2TLV_MAGIC1    0xa7b263fe

typedef enum {
    MSCC_STAGE2_TLV_BOOTLOADER = 1,
    MSCC_STAGE2_TLV_ROOTFS,
    MSCC_STAGE2_TLV_LAST,    // Must be last - always
} mscc_firmware_image_stage2_tlv_t;

typedef struct mscc_firmware_vimage_s2_tlv {
    mscc_le_u32 magic1;
    mscc_le_u32 type;      // TLV type (mscc_firmware_image_stage2_tlv_t)
    mscc_le_u32 tlv_len;   // Total length of TLV (hdr, data, padding)
    mscc_le_u32 data_len;  // Data length of TLV
    mscc_le_u32 sig_type;  // Signature type (mscc_firmware_image_signature_t)
    u8          value[0];  // Blob data
} mscc_firmware_vimage_stage2_tlv_t;

// NB: Assumes proper tlv structure aligment
static inline bool mscc_vimage_stage2_check_tlv_basic(const mscc_firmware_vimage_stage2_tlv_t *s2tlv)
{
    if (s2tlv->magic1 != MSCC_VIMAGE_S2TLV_MAGIC1 ||
        s2tlv->type == 0 ||
        s2tlv->type >= MSCC_STAGE2_TLV_LAST) {
        return false;
    }

    return true;
}

bool mscc_vimage_stage2_check_tlv(const mscc_firmware_vimage_stage2_tlv_t *s2tlv,
                                  size_t s2len);

/*
 * Sideband data
 */

int mscc_vimage_sideband_check(const mscc_firmware_sideband_t *sb, const char **errmsg);

u8 *mscc_vimage_sideband_find_tlv(const mscc_firmware_sideband_t *sb,
                                  mscc_firmware_vimage_tlv_t *tlv,
                                  mscc_firmware_image_sideband_tlv_t type);
mscc_firmware_sideband_t *mscc_vimage_sideband_create(void);
mscc_firmware_sideband_t *mscc_vimage_sideband_add_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_length,
                                                       mscc_firmware_image_sideband_tlv_t type);
mscc_firmware_sideband_t *mscc_vimage_sideband_set_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_length,
                                                       mscc_firmware_image_sideband_tlv_t type);
void mscc_vimage_sideband_delete_tlv(mscc_firmware_sideband_t *sb,
                                     mscc_firmware_image_sideband_tlv_t type);
void mscc_vimage_sideband_update_crc(mscc_firmware_sideband_t *sb);

#endif // _VIMAGE_H_
