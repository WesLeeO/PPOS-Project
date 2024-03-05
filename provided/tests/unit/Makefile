# ======================================================================
# Useful for feedback

CC = clang

TARGETS := mount sector inode
TARGETS += filev6 utils
TARGETS += direntv6
TARGETS += fuse

CFLAGS += -g

EXECS=$(foreach name,$(TARGETS),unit-test-$(name))

.PHONY: unit-tests all $(TARGETS) execs

# first creates all exec, **then** launch them
all: execs unit-tests

unit-tests: $(TARGETS)

execs: $(EXECS)

# some target shortcuts : compile & run the tests
mount: unit-test-mount
	./unit-test-mount
sector: unit-test-sector
	./unit-test-sector
inode: unit-test-inode
	./unit-test-inode
filev6: unit-test-filev6
	./unit-test-filev6
utils: unit-test-utils
	./unit-test-utils
direntv6: unit-test-direntv6
	./unit-test-direntv6
fuse: unit-test-fuse
	./unit-test-fuse

# ======================================================================
DATA_DIR ?= ../data
SRC_DIR  ?= ../../../done
CFLAGS  += -I$(SRC_DIR) -DCS212_TEST -DDATA_DIR='"$(DATA_DIR)"'
LDFLAGS += -L$(SRC_DIR)

MOUNT_O := $(SRC_DIR)/mount.o
MOUNT_O += $(SRC_DIR)/bmblock.o

CFLAGS  += -fsanitize=address
LDFLAGS += -fsanitize=address
LDLIBS  += -fsanitize=address

unit-test-mount.o: unit-test-mount.c
unit-test-mount: LDLIBS += -lcheck -lm -lrt -pthread -lsubunit
unit-test-mount: unit-test-mount.o $(MOUNT_O) $(SRC_DIR)/inode.o $(SRC_DIR)/sector.o

unit-test-sector.o: unit-test-sector.c
unit-test-sector: LDLIBS += -lcheck -lm -lrt -pthread -lsubunit
unit-test-sector: unit-test-sector.o  $(SRC_DIR)/sector.o

unit-test-inode.o: unit-test-inode.c
unit-test-inode: LDLIBS += -lcheck -lm -lrt -pthread -lsubunit
unit-test-inode: unit-test-inode.o $(SRC_DIR)/inode.o $(SRC_DIR)/sector.o $(MOUNT_O)

unit-test-filev6.o: unit-test-filev6.c
unit-test-filev6: LDLIBS += -lcheck -lm -lrt -pthread -lsubunit
unit-test-filev6: unit-test-filev6.o $(SRC_DIR)/inode.o $(SRC_DIR)/sector.o $(MOUNT_O) $(SRC_DIR)/filev6.o

unit-test-utils.o: unit-test-utils.c
unit-test-utils: LDLIBS += -lcheck -lrt -pthread -lsubunit -lssl -lcrypto -lm
unit-test-utils: unit-test-utils.o $(SRC_DIR)/inode.o $(SRC_DIR)/sector.o $(MOUNT_O) $(SRC_DIR)/filev6.o $(SRC_DIR)/u6fs_utils.o
unit-test-direntv6.o: unit-test-direntv6.c
unit-test-direntv6: LDLIBS += -lcheck -lrt -pthread -lsubunit -lssl -lcrypto -lm
unit-test-direntv6: unit-test-direntv6.o $(SRC_DIR)/inode.o $(SRC_DIR)/sector.o $(MOUNT_O) $(SRC_DIR)/filev6.o $(SRC_DIR)/u6fs_utils.o $(SRC_DIR)/direntv6.o
$(SRC_DIR)/u6fs_fuse.o: CFLAGS += $(shell pkg-config fuse --cflags)
unit-test-fuse.o: CFLAGS += $(shell pkg-config fuse --cflags)
unit-test-fuse.o: unit-test-fuse.c
unit-test-fuse: LDLIBS += $(shell pkg-config fuse --libs) -lcheck -lrt -pthread -lsubunit -lssl -lcrypto -lm
unit-test-fuse: unit-test-fuse.o $(SRC_DIR)/inode.o $(SRC_DIR)/sector.o $(MOUNT_O) $(SRC_DIR)/filev6.o $(SRC_DIR)/u6fs_utils.o $(SRC_DIR)/direntv6.o $(SRC_DIR)/u6fs_fuse.o

# ======================================================================
.PHONY: clean dist-clean reset

clean::
	-$(RM) *.o *~

dist-clean: clean
	-$(RM) $(foreach T,$(TARGETS),unit-test-$(T))

reset: dist-clean all
