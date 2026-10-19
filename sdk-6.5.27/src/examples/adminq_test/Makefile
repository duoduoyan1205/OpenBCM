CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
TARGET := adminq_test
SOURCES := adminq_test.c

.PHONY: all test clean ctrlq-diag
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

test: $(TARGET)
	./$(TARGET) --mock

# Optional object build against device-provided register headers.
# Example:
#   make ctrlq-diag CTRLQ_HEADER_DIR=/path/to/shizi/register/headers
ctrlq-diag:
	@test -n "$(CTRLQ_HEADER_DIR)" || (echo "Set CTRLQ_HEADER_DIR to the directory containing shizi_ctrlq_pf.h and shizi_ctrlq_fw.h"; exit 1)
	$(CC) $(CFLAGS) -I"$(CTRLQ_HEADER_DIR)" -c adminq_ctrlq_diag.c -o adminq_ctrlq_diag.o

clean:
	rm -f $(TARGET) adminq_ctrlq_diag.o
