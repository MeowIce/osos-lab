/* SPDX-License-Identifier: GPL-3.0-only */
#include "assets.h"
#include "dark_mode.h"
#include "vietnamese.h"
#include "osos.h"
#include "patch.h"

void cfw_bitmap_view_load_original(void *view);
/* Stock lookups bypass the hook for pixel matching and light-mode restoration. */
void *cfw_bitmap_resource_get_original(void *service, uint32_t type, uint32_t id);
void cfw_bitmap_cache_load_original(void *cache);

PATCH_ARM const void *cfw_bitmap_resource_get(void *service, uint32_t type, uint32_t id)
{
    if (type == OSOS_RESOURCE_BMAP && cfw_dark_mode_enabled()) {
        for (uint32_t i = 0; i < cfw_bitmap_asset_count; i++) {
            if (cfw_bitmap_assets[i].id == id)
                return cfw_bitmap_assets[i].data;
        }
    }
    if (type == OSOS_RESOURCE_FONT && cfw_vietnamese_enabled()) {
        if (id == 0x0dad0020 || id == 0x0dad0023 || id == 0x0dad0004 ||
            id == 0x0dad0010 || id == 0x0dad0013 || id == 0x0dad0016 ||
            id == 0x0dad001b || id == 0x0dad003b || id == 0x0dad003c)
            id = 0x0dad0021;
    }
    const void *resource = cfw_bitmap_resource_get_original(service, type, id);
    if (type == OSOS_RESOURCE_COLR)
        resource = cfw_dark_mode_color(id, resource);
    if (type == OSOS_RESOURCE_STR && resource) {
        if (cfw_vietnamese_enabled()) {
            if (id == 0x0dad0e30)
                return "SA";
            if (id == 0x0dad0e31)
                return "Ch";
            if (id == 0x0dad0b94 || id == 0x0dad0bf7)
                return "Ca kh\xc3\xba" "c";
            if (id == 0x0dad0bb3 || id == 0x0dad0bf9)
                return "1";
            if (id == 0x0dad0bb4 || id == 0x0dad0bfa)
                return "B\xe1\xba\xadt";
            if (id == 0x0dad0b98)
                return "Ten";
            if (id == 0x0dad0b99)
                return "Ho";
            const char *vi = cfw_vietnamese_translate((const char *)resource);
            if (vi)
                return vi;
        }
    }
    return resource;
}

static const struct cfw_bitmap_asset *find_asset(void *service, uint32_t id,
                                                const unsigned char *pixels, int dark)
{
    for (uint32_t i = 0; i < cfw_bitmap_asset_count; i++) {
        const struct cfw_bitmap_asset *asset = &cfw_bitmap_assets[i];
        if (pixels == asset->data + OSOS_BMAP_HEADER_SIZE || (dark && asset->id == id))
            return asset;
    }
    /* Light mode only needs to restore views still showing replacement pixels. */
    if (!dark)
        return 0;
    /* Dynamic properties identify the source by its resolved pixel storage. */
    for (uint32_t i = 0; i < cfw_bitmap_asset_count; i++) {
        struct cfw_bitmap_asset *asset = &cfw_bitmap_assets[i];
        if (!asset->original)
            asset->original = cfw_bitmap_resource_get_original(service, OSOS_RESOURCE_BMAP,
                                                        asset->id);
        if (asset->original && pixels == asset->original + OSOS_BMAP_HEADER_SIZE)
            return asset;
    }
    return 0;
}

PATCH_ARM void cfw_bitmap_view_load(void *view)
{
    cfw_bitmap_view_load_original(view);
    uint32_t type = osos_bitmap_view_type(view);
    if (!osos_bitmap_view_ready(view) ||
        (type != OSOS_RESOURCE_BMAP && type != OSOS_RESOURCE_DRAW))
        return;
    uint32_t id = osos_bitmap_view_id(view);
    void *context = osos_bitmap_view_context(view);
    void *bitmap = osos_bitmap_view_bitmap(view);
    void *service = osos_bitmap_view_service(view);
    const unsigned char *pixels = osos_bitmap_pixels(osos_graphics_bitmap(context));
    int dark = cfw_dark_mode_enabled();
    const struct cfw_bitmap_asset *asset = find_asset(service, id, pixels, dark);
    if (!asset)
        return;
    int replacement_attached = pixels == asset->data + OSOS_BMAP_HEADER_SIZE;
    if (replacement_attached == dark)
        return;
    if (!dark && type == OSOS_RESOURCE_DRAW) {
        void *original = cfw_bitmap_resource_get_original(service, type, id);
        if (original && osos_bitmap_pixels(original) !=
                        asset->data + OSOS_BMAP_HEADER_SIZE) {
            osos_graphics_attach_bitmap(context, original);
            return;
        }
    }
    const void *data = dark ? asset->data :
        cfw_bitmap_resource_get_original(service, OSOS_RESOURCE_BMAP, asset->id);
    if (data) {
        /* Draw may use a shared bitmap; keep replacement pixels in this view. */
        osos_bitmap_load_resource(bitmap, data);
        osos_graphics_attach_bitmap(context, bitmap);
    }
}

PATCH_ARM void cfw_bitmap_cache_load(void *cache)
{
    cfw_bitmap_cache_load_original(cache);
    if (!osos_bitmap_cache_ready(cache))
        return;
    void *service = osos_bitmap_cache_service(cache);
    void *bitmap = osos_bitmap_cache_bitmap(cache);
    const unsigned char *pixels = osos_bitmap_pixels(bitmap);
    int dark = cfw_dark_mode_enabled();
    const struct cfw_bitmap_asset *asset = find_asset(
        service, osos_bitmap_cache_id(cache), pixels, dark);
    if (!asset || (pixels == asset->data + OSOS_BMAP_HEADER_SIZE) == dark)
        return;
    const void *data = dark ? asset->data :
        cfw_bitmap_resource_get_original(service, OSOS_RESOURCE_BMAP, asset->id);
    if (data)
        osos_bitmap_load_resource(bitmap, data);
}
