#ifndef ZTR_TEST_HELPERS_H
#define ZTR_TEST_HELPERS_H

/* Exactly ZTR_SSO_CAP (15) characters — fits inline. */
#define SSO_STR "123456789012345"
/* One character past the SSO boundary — goes to heap. */
#define HEAP_STR "1234567890123456"
/* A much longer string well above the SSO cap. */
#define LONG_STR "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@"

#endif /* ZTR_TEST_HELPERS_H */
