/*
 * Read-only register diagnostics for the Shizi CSR-Based CTRLQ.
 *
 * This file deliberately does not map BARs, allocate DMA memory, write CSRs,
 * enable queues, or submit commands. The caller supplies a trusted read32
 * accessor implemented by the existing driver/HAL or a controlled test stub.
 *
 * Build with CTRLQ_HEADER_DIR pointing to the directory containing the
 * device-provided shizi_ctrlq_pf.h and shizi_ctrlq_fw.h headers.
 */
#include "adminq_ctrlq_diag.h"

#include "shizi_ctrlq_pf.h"
#include "shizi_ctrlq_fw.h"

static int ctrlq_read(adminq_ctrlq_read32_fn read32, void *ctx,
                      adminq_ctrlq_window_t window, uint32_t offset,
                      uint32_t *value)
{
    int rc;

    rc = read32(ctx, window, offset, value);
    if (rc != 0) {
        fprintf(stderr, "[CTRLQ-REG] read failed window=%s offset=0x%04x rc=%d\n",
                window == ADMINQ_CTRLQ_PF_WINDOW ? "PF" : "FW",
                (unsigned int)offset, rc);
        return rc;
    }
    return 0;
}

/*
 * Dump the eight PF ATQ/ARQ register sets defined by shizi_ctrlq_pf.h.
 * Values are diagnostic only: no assumptions are made about whether a queue
 * should be enabled, idle, empty, or active at the time of the snapshot.
 */
int adminq_ctrlq_dump_pf_queues(adminq_ctrlq_read32_fn read32, void *ctx,
                                FILE *out)
{
    unsigned int q;
    uint32_t atq_para0, atq_head, atq_tail;
    uint32_t arq_para0, arq_head, arq_tail;

    if (read32 == NULL || out == NULL) {
        return -1;
    }

    for (q = 0; q < 8; ++q) {
        if (ctrlq_read(read32, ctx, ADMINQ_CTRLQ_PF_WINDOW,
                       CTRLQ_PF_PF_AQATQ_PARA0_ADDR(q), &atq_para0) != 0 ||
            ctrlq_read(read32, ctx, ADMINQ_CTRLQ_PF_WINDOW,
                       CTRLQ_PF_PF_AQATQ_HEAD_ADDR(q), &atq_head) != 0 ||
            ctrlq_read(read32, ctx, ADMINQ_CTRLQ_PF_WINDOW,
                       CTRLQ_PF_PF_AQATQ_TAIL_ADDR(q), &atq_tail) != 0 ||
            ctrlq_read(read32, ctx, ADMINQ_CTRLQ_PF_WINDOW,
                       CTRLQ_PF_PF_AQARQ_PARA0_ADDR(q), &arq_para0) != 0 ||
            ctrlq_read(read32, ctx, ADMINQ_CTRLQ_PF_WINDOW,
                       CTRLQ_PF_PF_AQARQ_HEAD_ADDR(q), &arq_head) != 0 ||
            ctrlq_read(read32, ctx, ADMINQ_CTRLQ_PF_WINDOW,
                       CTRLQ_PF_PF_AQARQ_TAIL_ADDR(q), &arq_tail) != 0) {
            return -2;
        }

        fprintf(out,
                "[CTRLQ-REG][PF][q%u] ATQ para0=0x%08x en=%u idle=%u ovfl=%u len=%u head=0x%03x tail=0x%03x\n",
                q, (unsigned int)atq_para0,
                (atq_para0 & CTRLQ_PF_PF_AQATQ_PARA0_ATQEN_MASK) >>
                    CTRLQ_PF_PF_AQATQ_PARA0_ATQEN_SHIFT,
                (atq_para0 & CTRLQ_PF_PF_AQATQ_PARA0_IDLE_MASK) >>
                    CTRLQ_PF_PF_AQATQ_PARA0_IDLE_SHIFT,
                (atq_para0 & CTRLQ_PF_PF_AQATQ_PARA0_OVFL_MASK) >>
                    CTRLQ_PF_PF_AQATQ_PARA0_OVFL_SHIFT,
                (atq_para0 & CTRLQ_PF_PF_AQATQ_PARA0_ATQLEN_MASK) >>
                    CTRLQ_PF_PF_AQATQ_PARA0_ATQLEN_SHIFT,
                (unsigned int)(atq_head & CTRLQ_PF_PF_AQATQ_HEAD_ATQH_MASK),
                (unsigned int)(atq_tail & CTRLQ_PF_PF_AQATQ_TAIL_ATQT_MASK));

        fprintf(out,
                "[CTRLQ-REG][PF][q%u] ARQ para0=0x%08x en=%u idle=%u ovfl=%u len=%u head=0x%03x tail=0x%03x\n",
                q, (unsigned int)arq_para0,
                (arq_para0 & CTRLQ_PF_PF_AQARQ_PARA0_ARQEN_MASK) >>
                    CTRLQ_PF_PF_AQARQ_PARA0_ARQEN_SHIFT,
                (arq_para0 & CTRLQ_PF_PF_AQARQ_PARA0_IDLE_MASK) >>
                    CTRLQ_PF_PF_AQARQ_PARA0_IDLE_SHIFT,
                (arq_para0 & CTRLQ_PF_PF_AQARQ_PARA0_OVFL_MASK) >>
                    CTRLQ_PF_PF_AQARQ_PARA0_OVFL_SHIFT,
                (arq_para0 & CTRLQ_PF_PF_AQARQ_PARA0_ARQLEN_MASK) >>
                    CTRLQ_PF_PF_AQARQ_PARA0_ARQLEN_SHIFT,
                (unsigned int)(arq_head & CTRLQ_PF_PF_AQARQ_HEAD_ARQH_MASK),
                (unsigned int)(arq_tail & CTRLQ_PF_PF_AQARQ_TAIL_ARQT_MASK));
    }
    return 0;
}

/*
 * Dump FW-side queue control/status registers. Register names and offsets
 * come from shizi_ctrlq_fw.h; interpretation beyond field names is omitted.
 */
int adminq_ctrlq_dump_fw_status(adminq_ctrlq_read32_fn read32, void *ctx,
                                FILE *out)
{
    uint32_t init_base, req_sts, comp_sts, atq_para, arq_para;
    const struct {
        uint32_t offset;
        uint32_t *value;
    } regs[] = {
        { CTRLQ_FW_FW_PARA_BASE_ADDR, &init_base },
        { CTRLQ_FW_FW_AQATQ_REQ_STS_ADDR, &req_sts },
        { CTRLQ_FW_FW_AQATQ_COMP_STS_ADDR, &comp_sts },
        { CTRLQ_FW_FW_AQATQ_PARA_ADDR, &atq_para },
        { CTRLQ_FW_FW_AQARQ_PARA_ADDR, &arq_para }
    };
    unsigned int i;

    if (read32 == NULL || out == NULL) {
        return -1;
    }
    for (i = 0; i < sizeof(regs) / sizeof(regs[0]); ++i) {
        if (ctrlq_read(read32, ctx, ADMINQ_CTRLQ_FW_WINDOW,
                       regs[i].offset, regs[i].value) != 0) {
            return -2;
        }
    }

    fprintf(out, "[CTRLQ-REG][FW] PARA_BASE=0x%08x init_start=%u init_sts=%u\n",
            (unsigned int)init_base,
            (init_base & CTRLQ_FW_FW_PARA_BASE_INIT_START_MASK) >>
                CTRLQ_FW_FW_PARA_BASE_INIT_START_SHIFT,
            (init_base & CTRLQ_FW_FW_PARA_BASE_INIT_STS_MASK) >>
                CTRLQ_FW_FW_PARA_BASE_INIT_STS_SHIFT);
    fprintf(out, "[CTRLQ-REG][FW] AQATQ_REQ_STS=0x%08x AQATQ_COMP_STS=0x%08x\n",
            (unsigned int)req_sts, (unsigned int)comp_sts);
    fprintf(out, "[CTRLQ-REG][FW] AQATQ_PARA=0x%08x AQARQ_PARA=0x%08x\n",
            (unsigned int)atq_para, (unsigned int)arq_para);
    return 0;
}
