# mino-examples -- embedding examples, cookbook, use-cases, and bindings.

CC       ?= cc
CXX      ?= c++
CFLAGS   ?= -std=c99 -Wall -Wpedantic -Wextra -O2 -Imino/src
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Imino/src
LDFLAGS  ?=
LIBS     ?= -lm

MINO_SRCS := mino/src/mino.c mino/src/diag.c mino/src/eval_special.c \
             mino/src/eval_special_defs.c mino/src/eval_special_bindings.c \
             mino/src/eval_special_control.c mino/src/eval_special_fn.c \
             mino/src/runtime_state.c mino/src/runtime_var.c \
             mino/src/runtime_error.c mino/src/runtime_env.c mino/src/runtime_gc.c \
             mino/src/val.c mino/src/vec.c mino/src/map.c mino/src/rbtree.c \
             mino/src/read.c mino/src/print.c \
             mino/src/prim.c mino/src/prim_numeric.c mino/src/prim_collections.c \
             mino/src/prim_sequences.c mino/src/prim_string.c mino/src/prim_io.c \
             mino/src/prim_reflection.c mino/src/prim_meta.c mino/src/prim_regex.c \
             mino/src/prim_stateful.c mino/src/prim_module.c \
             mino/src/prim_host.c mino/src/host_interop.c \
             mino/src/clone.c mino/src/re.c \
             mino/src/async_buffer.c mino/src/async_channel.c \
             mino/src/async_handler.c mino/src/async_select.c \
             mino/src/async_scheduler.c mino/src/async_timer.c \
             mino/src/prim_async.c
MINO_OBJS := $(MINO_SRCS:.c=.o)

# C examples
C_SRCS := src/actor_test.c src/actor_scale_test.c src/actor_stress_test.c \
          src/api_stress_test.c src/clone_test.c src/integration_test.c \
          src/interop_test.c src/ref_test.c src/state_switch_test.c \
          src/fault_inject_test.c
C_BINS := $(C_SRCS:.c=) src/embed_c

# Cookbook examples
COOKBOOK_SRCS := $(wildcard src/cookbook/*.c)
COOKBOOK_BINS := $(COOKBOOK_SRCS:.c=)

# C++ examples
CXX_SRCS := src/cpp_embed_test.cpp src/event_processing.cpp
CXX_BINS := $(CXX_SRCS:.cpp=) src/embed_cpp

# Use-case examples
USE_CASE_SRCS := $(wildcard use-cases/*.cpp)
USE_CASE_BINS := $(USE_CASE_SRCS:.cpp=)

# regex_thread_test needs pthreads and only links re.c
REGEX_BIN := src/regex_thread_test

ALL_BINS := $(C_BINS) $(COOKBOOK_BINS) $(CXX_BINS) $(USE_CASE_BINS) $(REGEX_BIN)

.PHONY: all clean test-use-cases

all: $(ALL_BINS)

# --- Generated header ---

mino/src/core_mino.h: mino/src/core.mino
	@printf 'static const char *core_mino_src =\n' > $@
	@sed 's/\\/\\\\/g; s/"/\\"/g; s/^/    "/; s/$$/\\n"/' $< >> $@
	@printf '    ;\n' >> $@

mino/src/prim.o: mino/src/prim.c mino/src/core_mino.h

# --- Build rules ---

src/embed_c: src/embed.c $(MINO_OBJS) mino/src/mino.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(MINO_OBJS) $(LIBS)

src/embed_cpp: src/embed.cpp $(MINO_OBJS) mino/src/mino.h
	$(CXX) $(CXXFLAGS) -o $@ $< $(MINO_OBJS) $(LIBS)

$(filter-out src/embed_c,$(C_BINS)): %: %.c $(MINO_OBJS) mino/src/mino.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(MINO_OBJS) $(LIBS)

$(COOKBOOK_BINS): %: %.c $(MINO_OBJS) mino/src/mino.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(MINO_OBJS) $(LIBS)

$(filter-out src/embed_cpp,$(CXX_BINS)): %: %.cpp $(MINO_OBJS) mino/src/mino.h
	$(CXX) $(CXXFLAGS) -o $@ $< $(MINO_OBJS) $(LIBS)

$(USE_CASE_BINS): %: %.cpp $(MINO_OBJS) mino/src/mino.h
	$(CXX) $(CXXFLAGS) -o $@ $< $(MINO_OBJS) $(LIBS)

$(REGEX_BIN): src/regex_thread_test.c mino/src/re.c mino/src/re.h
	$(CC) $(CFLAGS) -pthread -o $@ src/regex_thread_test.c mino/src/re.c

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# --- Test all use-case examples ---

test-use-cases: $(USE_CASE_BINS)
	@for bin in $(USE_CASE_BINS); do \
	  printf "%-40s " "$$bin"; \
	  if $$bin > /dev/null 2>&1; then echo "ok"; \
	  else echo "FAIL"; exit 1; fi; \
	done
	@echo "all use case examples passed"

clean:
	rm -f $(MINO_OBJS) $(ALL_BINS) mino/src/core_mino.h
