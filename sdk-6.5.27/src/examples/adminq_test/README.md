# CSR-Based AdminQ / CTRLQ test scaffold

This directory is an initial, **hardware-independent test scaffold** for the PCIe CSR-Based AdminQ/CTRLQ workstream.

## Important scope

The public SDK tree inspected for this change does not expose a verified register/descriptor contract for the target device's PF ATQ, PF ARQ, FW ATQ, and FW ARQ. Therefore this scaffold deliberately does **not** write BAR/CSR registers, allocate DMA memory, ring doorbells, or claim to exercise firmware. It provides a small transport-operations seam and a deterministic mock backend so command/completion handling can be developed and tested before binding it to the device-specific driver.

Do not treat passing mock tests as hardware validation.

## Files

- `adminq_test.c`: standalone test harness and mock transport.
- `Makefile`: builds the harness with a host C compiler.

## Build and run

From this directory:

```sh
make
make test
make clean
```

The executable defaults to the mock backend. It exercises a request/completion round trip for each logical queue role and checks completion status and sequence matching.

## Integration plan

1. Identify the authoritative device register and descriptor definitions.
2. Find the existing kernel/firmware owner for each queue and confirm direction, ownership, DMA address width, alignment, producer/consumer semantics, interrupt/polling model, and reset behavior.
3. Implement a device transport behind the operations interface, preferably through the supported driver/HAL API. Keep BAR mapping and DMA synchronization in the appropriate driver layer.
4. Add hardware tests only for operations whose semantics are documented. Begin with a harmless read-only command; do not use guessed opcodes or guessed register offsets.
5. Keep mock tests and hardware tests separately labelled in logs and CI.

## Queue naming

The PF/FW ATQ/ARQ names below are treated as four logical test targets only. Their exact direction and command/event semantics must be confirmed from the target device specification; the harness intentionally does not infer these from the abbreviations.
