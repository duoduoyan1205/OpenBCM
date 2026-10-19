/*
 * CSR-Based AdminQ / CTRLQ test scaffold.
 *
 * This program tests request/completion handling against a mock transport.
 * It does not access PCIe BARs, CSR registers, DMA memory, or firmware.
 * Device-specific operations must only be added after consulting the
 * authoritative device specification and existing driver/HAL interfaces.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ADMINQ_MAX_PAYLOAD 64U
#define ADMINQ_TIMEOUT_MS 1000U

typedef enum adminq_id_e {
    ADMINQ_PF_ATQ = 0,
    ADMINQ_PF_ARQ,
    ADMINQ_FW_ATQ,
    ADMINQ_FW_ARQ,
    ADMINQ_COUNT
} adminq_id_t;

typedef struct adminq_request_s {
    unsigned int sequence;
    unsigned short opcode;
    unsigned short payload_len;
    unsigned char payload[ADMINQ_MAX_PAYLOAD];
} adminq_request_t;

typedef struct adminq_completion_s {
    unsigned int sequence;
    int status;
} adminq_completion_t;

/*
 * Transport contract for the test harness. A real implementation should
 * normally call into the supported driver/HAL rather than perform raw
 * userspace MMIO or DMA management.
 */
typedef struct adminq_transport_ops_s {
    int (*submit)(void *ctx, adminq_id_t queue,
                  const adminq_request_t *request);
    int (*poll_completion)(void *ctx, adminq_id_t queue,
                           unsigned int timeout_ms,
                           adminq_completion_t *completion);
} adminq_transport_ops_t;

typedef struct adminq_test_context_s {
    const adminq_transport_ops_t *ops;
    void *transport_ctx;
    unsigned int next_sequence;
} adminq_test_context_t;

typedef struct mock_transport_s {
    adminq_completion_t pending[ADMINQ_COUNT];
    int has_pending[ADMINQ_COUNT];
} mock_transport_t;

static const char *adminq_name(adminq_id_t queue)
{
    switch (queue) {
    case ADMINQ_PF_ATQ: return "PF_ATQ";
    case ADMINQ_PF_ARQ: return "PF_ARQ";
    case ADMINQ_FW_ATQ: return "FW_ATQ";
    case ADMINQ_FW_ARQ: return "FW_ARQ";
    default: return "UNKNOWN";
    }
}

static int mock_submit(void *ctx, adminq_id_t queue,
                       const adminq_request_t *request)
{
    mock_transport_t *mock = (mock_transport_t *)ctx;

    if (mock == NULL || request == NULL || queue < 0 ||
        queue >= ADMINQ_COUNT || request->payload_len > ADMINQ_MAX_PAYLOAD) {
        return -1;
    }
    if (mock->has_pending[queue]) {
        return -2; /* one outstanding request per queue in this simple mock */
    }

    mock->pending[queue].sequence = request->sequence;
    mock->pending[queue].status = 0;
    mock->has_pending[queue] = 1;
    return 0;
}

static int mock_poll_completion(void *ctx, adminq_id_t queue,
                                unsigned int timeout_ms,
                                adminq_completion_t *completion)
{
    mock_transport_t *mock = (mock_transport_t *)ctx;

    (void)timeout_ms; /* the deterministic mock completes immediately */
    if (mock == NULL || completion == NULL || queue < 0 ||
        queue >= ADMINQ_COUNT) {
        return -1;
    }
    if (!mock->has_pending[queue]) {
        return -2;
    }

    *completion = mock->pending[queue];
    mock->has_pending[queue] = 0;
    return 0;
}

static const adminq_transport_ops_t mock_ops = {
    mock_submit,
    mock_poll_completion
};

static int adminq_round_trip(adminq_test_context_t *test, adminq_id_t queue)
{
    adminq_request_t request;
    adminq_completion_t completion;
    int rc;

    memset(&request, 0, sizeof(request));
    memset(&completion, 0, sizeof(completion));

    request.sequence = ++test->next_sequence;
    request.opcode = 0x0001U; /* mock-only opcode; not a hardware command */
    request.payload_len = 4U;
    request.payload[0] = (unsigned char)queue;
    request.payload[1] = 0xA5U;
    request.payload[2] = 0x5AU;
    request.payload[3] = 0x00U;

    rc = test->ops->submit(test->transport_ctx, queue, &request);
    if (rc != 0) {
        fprintf(stderr, "[FAIL] %s submit rc=%d\n", adminq_name(queue), rc);
        return 1;
    }

    rc = test->ops->poll_completion(test->transport_ctx, queue,
                                    ADMINQ_TIMEOUT_MS, &completion);
    if (rc != 0) {
        fprintf(stderr, "[FAIL] %s poll rc=%d\n", adminq_name(queue), rc);
        return 1;
    }
    if (completion.sequence != request.sequence) {
        fprintf(stderr,
                "[FAIL] %s sequence mismatch: request=%u completion=%u\n",
                adminq_name(queue), request.sequence, completion.sequence);
        return 1;
    }
    if (completion.status != 0) {
        fprintf(stderr, "[FAIL] %s completion status=%d\n",
                adminq_name(queue), completion.status);
        return 1;
    }

    printf("[PASS] MOCK %s request/completion sequence=%u status=%d\n",
           adminq_name(queue), completion.sequence, completion.status);
    return 0;
}

int main(int argc, char **argv)
{
    mock_transport_t mock;
    adminq_test_context_t test;
    int failures = 0;
    int i;

    if (argc > 2 ||
        (argc == 2 && strcmp(argv[1], "--mock") != 0)) {
        fprintf(stderr, "Usage: %s [--mock]\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&mock, 0, sizeof(mock));
    memset(&test, 0, sizeof(test));
    test.ops = &mock_ops;
    test.transport_ctx = &mock;

    printf("AdminQ test scaffold: MOCK backend only; no hardware access.\n");
    for (i = 0; i < (int)ADMINQ_COUNT; ++i) {
        failures += adminq_round_trip(&test, (adminq_id_t)i);
    }

    if (failures != 0) {
        fprintf(stderr, "AdminQ mock tests failed: %d\n", failures);
        return EXIT_FAILURE;
    }
    printf("All mock tests passed. This is not hardware validation.\n");
    return EXIT_SUCCESS;
}
