/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#ifndef UTILS_H
#define UTILS_H

/* Check return values from functions and print possible errors. */
#define RUN_CHECK(f) do { \
        int ret = f; \
        if (ret < 0) { \
            P_ERR("[%d]: " #f "\n", ret); \
        } \
} while (0)

/* Check return values from mira functions and print possible errors. */
#define MIRA_RUN_CHECK(f) do { \
        mira_status_t ret = f; \
        if (ret != MIRA_SUCCESS) { \
            P_ERR("[%d]: " #f "\n", ret); \
        } \
} while (0)

/* Define DEBUG_LEVEL before including this header file, default is no print. */
#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL 0
#endif

#if DEBUG_LEVEL > 0
#include <stdio.h>
#endif

/* Use P_ERR for messages that are not expected to happen in normal behavior. */
#if DEBUG_LEVEL >= 1
#define P_ERR(...) printf("ERROR " __VA_ARGS__)
#else
#define P_ERR(...)
#endif

/* Use P_DEBUG for help messages to analyze expected behavior. */
#if DEBUG_LEVEL >= 2
#define P_DEBUG(...) printf(__VA_ARGS__)
#else
#define P_DEBUG(...)
#endif

/* Store a variable to buffer, in little endian. The type of the variable
 * determines the width of the write, use types from stdint.h. */
#define LITTLE_ENDIAN_STORE(buffer, v)          \
    do { \
        for (int i = 0; i < sizeof(v); i++) { \
            buffer[i] = (v >> (i * 8)) & 0xff;   \
        } \
    } while (0)

/* Load a variable from buffer, read in little endian. The type of the variable
 * determines the width of the read. use types from stdint.h. Up to 64 bits
 * supported. */
#define LITTLE_ENDIAN_LOAD(p, buffer) \
    do { \
        *p = 0; \
        for (int i = 0; i < sizeof(*p); i++) {   \
            *p |= (uint64_t) buffer[i] << (i * 8);       \
        } \
    } while (0)

#endif
