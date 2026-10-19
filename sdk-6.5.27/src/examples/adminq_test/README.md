# CSR-Based AdminQ / CTRLQ test scaffold

This directory contains two deliberately separate layers:

- a hardware-independent mock test for request/completion handling;
- a read-only register diagnostic helper based on the supplied Shizi CTRLQ register definitions.

## Important scope and safety

The mock test does **not** access PCIe BARs, CSR registers, DMA memory, or firmware. Passing it is not hardware validation.

`adminq_ctrlq_diag.c` only reads and prints register values through a caller-supplied `read32` callback. It does not map BARs, write registers, enable queues, allocate DMA memory, ring doorbells, or submit AdminQ commands. The caller must provide the callback using the supported device driver/HAL path or a controlled test stub. Do not implement this by blindly accessing raw physical memory.

The two device register headers are not copied into this public SDK tree. Keep the device-provided files in an appropriate local directory and pass that directory to the optional compile target.

## Files

- `adminq_test.c`: standalone mock harness and transport abstraction.
- `adminq_ctrlq_diag.c`: read-only PF ATQ/ARQ and FW status dump helpers.
- `Makefile`: builds the mock harness; optionally compiles the diagnostic helper against the device register headers.

## Build and run the mock test

From this directory:

```sh
make
make test
make clean
```

The executable exercises a request/completion round trip for each logical queue role and checks completion status and sequence matching. The opcode used here is mock-only and is not a hardware command.

## Compile the register diagnostic helper

```sh
make ctrlq-diag CTRLQ_HEADER_DIR=/path/to/device/register/headers
```

That directory must contain `shizi_ctrlq_pf.h` and `shizi_ctrlq_fw.h`. This target only compiles `adminq_ctrlq_diag.c` into an object; it does not link a hardware executable or perform register reads.

The public declarations are in `adminq_ctrlq_diag.h`. The helper entry points are:

- `adminq_ctrlq_dump_pf_queues(read32, ctx, out)`: reads PF ATQ/ARQ PARA0, HEAD, and TAIL registers for queue indices 0–7 and prints the raw values plus the fields named in the header.
- `adminq_ctrlq_dump_fw_status(read32, ctx, out)`: reads FW init/status and ATQ/ARQ parameter/status registers and prints the raw values.

The caller must implement `read32` and explicitly map the PF/FW windows to the correct device resource. This helper is intentionally not a complete hardware test until that integration is done.

## What the supplied register headers establish

The PF header defines eight indexed PF ATQ and ARQ register sets, including PARA0, PARA1/PARA2, buffer address low/high, and HEAD/TAIL registers. The FW header defines initialization/configuration fields and FW-side queue/status register groups.

The headers provide offsets, masks, shifts, reset values, and field names, but do not by themselves establish the complete command descriptor format, command opcode table, queue ownership/direction, producer/consumer update protocol, interrupt lifecycle, or safe initialization sequence. This diagnostic therefore does not infer those semantics and does not write any register.

## Next integration steps

1. Find the existing driver/HAL implementation that owns BAR mapping and DMA queue memory.
2. Implement `read32` through that supported interface, explicitly mapping PF and FW windows to the correct device resources.
3. Confirm register window/base offsets and access width from the device integration documentation; these header offsets alone may be relative to a block rather than a PCIe BAR.
4. Run read-only snapshots first and compare with a known-good initialization state.
5. Add writes, queue enablement, DMA, doorbells, or real commands only after the authoritative descriptor and initialization contracts are available. Keep hardware tests separately labelled from mock tests.

## Queue naming

The PF/FW ATQ/ARQ names are treated as register/queue labels from the supplied headers. The exact command/event direction and completion semantics must be confirmed from the target device specification and existing implementation.
