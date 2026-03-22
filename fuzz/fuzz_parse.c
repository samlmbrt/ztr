#include "ztr.h"

#include <stddef.h>
#include <stdint.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    ztr s = {0};

    /* Fuzz construction from arbitrary bytes. */
    if (ztr_from_buf(&s, (const char *)data, size) == ZTR_OK) {
        /* Exercise accessors. */
        (void)ztr_len(&s);
        (void)ztr_cstr(&s);
        (void)ztr_is_empty(&s);
        (void)ztr_is_valid_utf8(&s);
        (void)ztr_is_ascii(&s);

        /* Exercise search. */
        if (size > 2) {
            ztr needle = {0};
            size_t half = size / 2;
            if (ztr_from_buf(&needle, (const char *)data, half) == ZTR_OK) {
                (void)ztr_find(&s, ztr_cstr(&needle), 0);
                (void)ztr_contains(&s, ztr_cstr(&needle));
                ztr_free(&needle);
            }
        }

        ztr_free(&s);
    }

    return 0;
}
