/*
 * Read-only Shizi CTRLQ diagnostic API.
 *
 * The caller supplies a trusted read32 callback backed by the supported
 * device driver/HAL or a controlled test stub. This API performs no writes.
 */
#ifndef ADMINQ_CTRLQ_DIAG_H
#define ADMINQ_CTRLQ_DIAG_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum adminq_ctrlq_window_e {
    ADMINQ_CTRLQ_PF_WINDOW = 0,
    ADMINQ_CTRLQ_FW_WINDOW = 1
} adminq_ctrlq_window_t;

typedef int (*adminq_ctrlq_read32_fn)(void *ctx,
                                      adminq_ctrlq_window_t window,
                                      uint32_t offset,
                                      uint32_t *value);

int adminq_ctrlq_dump_pf_queues(adminq_ctrlq_read32_fn read32, void *ctx,
                                FILE *out);
int adminq_ctrlq_dump_fw_status(adminq_ctrlq_read32_fn read32, void *ctx,
                                FILE *out);

#ifdef __cplusplus
}
#endif

#endif /* ADMINQ_CTRLQ_DIAG_H */
