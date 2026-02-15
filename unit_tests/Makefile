#
# Makefile for CF Application Unit Tests
#

CC = gcc

#
# GCOV support - enabled by default
#
ENABLE_GCOV = TRUE

ifeq ($(ENABLE_GCOV), TRUE)
GCOV_CFLAGS = -fprofile-arcs -ftest-coverage
GCOV_LFLAGS = -fprofile-arcs -ftest-coverage
endif

#
# Source search paths
#
VPATH = ../fsw/src
VPATH += ./stubs
VPATH += ./tests

#
# Include paths - ./inc MUST come first to shadow real cFE/OSAL headers
#
INCLUDES = -I./inc
INCLUDES += -I./stubs
INCLUDES += -I./tests
INCLUDES += -I../fsw/src
INCLUDES += -I../fsw/src/PRI
INCLUDES += -I../fsw/src/PUB
INCLUDES += -I../fsw/platform_inc
INCLUDES += -I../fsw/mission_inc

#
# Compiler flags
#
CFLAGS = -Wall -g -fno-stack-protector $(GCOV_CFLAGS) $(INCLUDES) \
         -DSOFTWARE_LITTLE_BIT_ORDER -D_EL -D_ix86_

#
# Linker flags
#
LFLAGS = $(GCOV_LFLAGS) -lm

#
# CF application objects
#
CF_OBJS = cf_app.o
CF_OBJS += cf_cmds.o
CF_OBJS += cf_callbacks.o
CF_OBJS += cf_utils.o
CF_OBJS += cf_playback.o

#
# Stub objects
#
STUB_OBJS = cfe_stubs.o
STUB_OBJS += cfdp_stubs.o

#
# Test objects
#
TEST_OBJS = test_runner.o
TEST_OBJS += test_cf_app.o
TEST_OBJS += test_cf_cmds.o
TEST_OBJS += test_cf_callbacks.o
TEST_OBJS += test_cf_utils.o
TEST_OBJS += test_cf_playback.o

#
# All objects
#
OBJS = $(CF_OBJS) $(STUB_OBJS) $(TEST_OBJS)

#
# Output executable
#
EXEC = cf_test_runner

#
# Targets
#
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(LFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(EXEC) *.o *.gcno *.gcda *.gcov

run: $(EXEC)
	./$(EXEC)

gcov:
	gcov cf_app.c
	gcov cf_cmds.c
	gcov cf_callbacks.c
	gcov cf_utils.c
	gcov cf_playback.c

.PHONY: all clean run gcov
