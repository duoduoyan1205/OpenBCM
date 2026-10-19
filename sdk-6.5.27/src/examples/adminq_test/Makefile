CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
TARGET := adminq_test
SOURCES := adminq_test.c

.PHONY: all test clean
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

test: $(TARGET)
	./$(TARGET) --mock

clean:
	rm -f $(TARGET)
