// Copyright (c) 2016 Microsemi Corporation "Microsemi".

#include <common.h>
#include <malloc.h>
#include <u-boot/md5.h>
#include "firmware_vimage.h"

#define MD5_MAC_LEN 16

/*
 * --- New image support ---
 */

int mscc_vimage_hdrcheck(mscc_firmware_vimage_t *fw, const char **errmsg)
{
    if (fw->hdrlen < sizeof(*fw)) {
        if (errmsg) *errmsg = "Short header";
        return -1;
    }
    if (fw->magic1 != MSCC_VIMAGE_MAGIC1 ||
        fw->magic2 != MSCC_VIMAGE_MAGIC2 ||
        fw->version < 1) {
        if (errmsg) *errmsg = "Image format invalid";
        return -1;
    }
#if 0
    if (!vimage_socfamily_valid(fw->soc_no)) {
        if (errmsg) *errmsg = "Image with wrong chipset family";
        return -1;
    }
#endif
    return 0;    // All OK
}

static bool md5_calc(const void *data, size_t data_length, void *digest)
{
	md5((unsigned char*)data, data_length, digest);
	return true;
}
static bool do_sha_validate(const void *data, size_t data_len,
			    const char *sha_name,
			    const u8 *signature, u32 sig_size)
{
	/* TODO */
	return true;
}

bool mscc_vimage_validate(mscc_firmware_vimage_t *fw, mscc_firmware_vimage_tlv_t *tlv, u8 *signature)
{
    u8 *sigcopy;

    // check signature sanity
    switch (fw->img_sign_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            if (tlv->data_len != MD5_MAC_LEN)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            if (tlv->data_len < 256 || tlv->data_len > 1024)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            if (tlv->data_len < 256 || tlv->data_len > 1024)
                goto invalid_signature;
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_NULL:
        default:
        invalid_signature:
            debug("Image has invalid signature");
            return false;
    }

    // Allocate signature copy while re-calculating
    sigcopy = (u8 *) malloc(tlv->data_len);
    if (!sigcopy) {
        return false;    // Failure
    }
    // Copy orig signature
    memcpy(sigcopy, signature, tlv->data_len);
    // Reset to zero
    memset(signature, 0, tlv->data_len);

    switch (fw->img_sign_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            {
                if (md5_calc(fw, fw->imglen, signature) &&
                    memcmp(sigcopy, signature, tlv->data_len) == 0) {
                    printf("MD5 signature validated");
                    free(sigcopy);
                    return true;
                }
            }
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            {
                if (do_sha_validate(fw, fw->imglen, "SHA256", sigcopy, tlv->data_len)) {
                    memcpy(signature, sigcopy, tlv->data_len);    // Restore signature
                    free(sigcopy);
                    return true;
                }
            }
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            {
                if (do_sha_validate(fw, fw->imglen, "SHA512", sigcopy, tlv->data_len)) {
                    memcpy(signature, sigcopy, tlv->data_len);    // Restore signature
                    free(sigcopy);
                    return true;
                }
            }
            break;
        default:
            printf("Type %d signature is unsupported", fw->img_sign_type);
    }
    printf("Failed to validate type %d signature", fw->img_sign_type);
    free(sigcopy);
    return false;    // Failed auth
}

u8 *mscc_vimage_find_tlv(const mscc_firmware_vimage_t *fw,
                         mscc_firmware_vimage_tlv_t *tlv,
                         mscc_firmware_image_stage1_tlv_t type)
{
    size_t off = sizeof(*fw);
    mscc_firmware_vimage_tlv_t *wtlv;

    while (off < fw->imglen) {
        wtlv = (mscc_firmware_vimage_tlv_t*) (((u8*)fw) + off);
        if (type == wtlv->type) {
            *tlv = *wtlv;
            return &wtlv->value[0];
        }
        off += wtlv->tlv_len;
    }

    return NULL;
}

u8 *mscc_vimage_sideband_find_tlv(const mscc_firmware_sideband_t *sb,
                                  mscc_firmware_vimage_tlv_t *tlv,
                                  mscc_firmware_image_sideband_tlv_t type)
{
    size_t off = sizeof(*sb);
    mscc_firmware_vimage_tlv_t *wtlv;
    while ((off + sizeof(mscc_firmware_vimage_tlv_t)) < sb->length) {
        wtlv = (mscc_firmware_vimage_tlv_t*) (((u8*)sb) + off);
        if (type == wtlv->type) {
            *tlv = *wtlv;
            return &wtlv->value[0];
        }
        off += wtlv->tlv_len;
    }
    return NULL;
}

int mscc_vimage_sideband_check(const mscc_firmware_sideband_t *sb, const char **errmsg)
{
    if (sb->length < sizeof(*sb)) {
        if (errmsg) *errmsg = "Short header";
        return -1;
    }
    if (sb->magic1 != MSCC_VIMAGE_SB_MAGIC1 ||
        sb->magic2 != MSCC_VIMAGE_SB_MAGIC2) {
        if (errmsg) *errmsg = "Sideband format invalid";
        return -1;
    }
    return 0;    // All OK
}

mscc_firmware_sideband_t *mscc_vimage_sideband_create(void)
{
    mscc_firmware_sideband_t *sb = (mscc_firmware_sideband_t *) malloc(sizeof(*sb));
    if (sb) {
        sb->length = sizeof(*sb);
        sb->magic1 = MSCC_VIMAGE_SB_MAGIC1;
        sb->magic2 = MSCC_VIMAGE_SB_MAGIC2;
    }
    return sb;
}

mscc_firmware_sideband_t *mscc_vimage_sideband_add_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_len,
                                                       mscc_firmware_image_sideband_tlv_t type)
{
    size_t tlvlen = sizeof(mscc_firmware_vimage_tlv_t) + MSCC_FIRMWARE_TLV_ALIGN(data_len);
    debug("SB %p, Size %d, adding %zd bytes, data %zd", sb, sb->length, tlvlen, data_len);
    sb = (mscc_firmware_sideband_t*) realloc((void*)sb, sb->length + tlvlen);
    if (sb) {
        // Add the tlv at back
        mscc_firmware_vimage_tlv_t *tlv= (mscc_firmware_vimage_tlv_t *) (((u8 *)sb) + sb->length);
        tlv->type = type;
        tlv->tlv_len = tlvlen;
        tlv->data_len = data_len;
        memcpy(tlv->value, data, data_len);
        sb->length += tlvlen;
    }
    return sb;
}

mscc_firmware_sideband_t *mscc_vimage_sideband_set_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_len,
                                                       mscc_firmware_image_sideband_tlv_t type)
{
    mscc_firmware_vimage_tlv_t tlv;
    const char *name;
    if ((name = (const char *) mscc_vimage_sideband_find_tlv(sb, &tlv, MSCC_FIRMWARE_SIDEBAND_FILENAME))) {
        mscc_vimage_sideband_delete_tlv(sb, type);
    }
    return mscc_vimage_sideband_add_tlv(sb, data, data_len, type);
}

void mscc_vimage_sideband_delete_tlv(mscc_firmware_sideband_t *sb,
                                     mscc_firmware_image_sideband_tlv_t type)
{
    size_t off = sizeof(*sb);
    while ((off + sizeof(mscc_firmware_vimage_tlv_t)) < sb->length) {
        mscc_firmware_vimage_tlv_t *wtlv = (mscc_firmware_vimage_tlv_t*) (((u8*)sb) + off);
        if (type == wtlv->type) {
            u32 tlvlen = wtlv->tlv_len;
            // Move up data
            memcpy((u8*) wtlv, ((u8*) wtlv) + tlvlen, (sb->length - off) - tlvlen);
            // Adjust len
            sb->length -= tlvlen;
        } else {
            off += wtlv->tlv_len;
        }
    }
}

void mscc_vimage_sideband_update_crc(mscc_firmware_sideband_t *sb)
{
    memset(sb->checksum, 0, MD5_MAC_LEN);
    (void) md5_calc(sb, sb->length, sb->checksum);
}

// Stage2

static bool check_stage2_sig(const mscc_firmware_vimage_stage2_tlv_t *tlv,
                             const mscc_firmware_vimage_stage2_tlv_t *tlv_data)
{
    const char *hash = NULL;
    const u8 *signature = &tlv_data->value[0] + tlv->data_len;
    u32 siglen = tlv->tlv_len - tlv->data_len - sizeof(*tlv);

    switch (tlv->sig_type) {
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5:
            if (siglen == MD5_MAC_LEN) {
                u8 calcsig[MD5_MAC_LEN];
                if (md5_calc(tlv_data, tlv->tlv_len - siglen, calcsig) &&
                    memcmp(calcsig, signature, sizeof(calcsig)) == 0) {
                    printf("MD5 signature validated");
                    return true;
                }
                printf("MD5 signature invalid");
            } else {
                printf("Illegal MD5 hash length: %d", siglen);
            }
            return false;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256:
            hash = "SHA256";
            break;
        case MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512:
            hash = "SHA512";
            break;
        default:
            printf("Illegal signature type: %d", tlv->sig_type);
            return false;
    }

    // These are sane ranges for SHA signatures
    if (siglen < 256 || siglen > 1024) {
        printf("Illegal SHA length: %d", siglen);
        return false;
    }

    // Now validate
    debug("Validate data %p length %d, signature at %p length %d", tlv_data, tlv->tlv_len, signature, siglen);
    return do_sha_validate(tlv_data, tlv->tlv_len - siglen, hash, signature, siglen);
}

bool mscc_vimage_stage2_check_tlv(const mscc_firmware_vimage_stage2_tlv_t *s2tlv,
                                  size_t s2len)
{
    mscc_firmware_vimage_stage2_tlv_t tlvcopy;

    memcpy(&tlvcopy, s2tlv, sizeof(tlvcopy));

    // Size check, TLV
    if (tlvcopy.tlv_len > s2len) {
        debug("TLV len %d exceeds buffer length %zd", tlvcopy.tlv_len, s2len);
        return false;
    }

    // Magic checks
    if (tlvcopy.magic1 != MSCC_VIMAGE_S2TLV_MAGIC1) {
        debug("TLV magic error %08x", tlvcopy.magic1);
        return false;
    }

    // Size check, data
    if (tlvcopy.data_len > (tlvcopy.tlv_len - sizeof(tlvcopy))) {
        debug("TLV data len %d exceeds tlv length %d, hdr %zd", tlvcopy.data_len, tlvcopy.tlv_len, sizeof(tlvcopy));
        return false;
    }

    // Type
    if (tlvcopy.type == 0 || tlvcopy.type >= MSCC_STAGE2_TLV_LAST) {
        debug("TLV type %d invalid", tlvcopy.type);
        return false;
    }

    // Validate signature
    if (check_stage2_sig(&tlvcopy, s2tlv) != true) {
        debug("TLV signature validation failed");
        return false;
    }

    debug("TLV length %d/%d OK", tlvcopy.tlv_len,  tlvcopy.data_len);
    return true;
}
