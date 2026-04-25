# mino-examples -- embedding examples, cookbook, use-cases, and bindings.

CC       ?= cc
CXX      ?= c++
MINO_INCS := -Imino/src -Imino/src/public -Imino/src/runtime \
             -Imino/src/gc -Imino/src/eval -Imino/src/collections \
             -Imino/src/prim -Imino/src/async -Imino/src/interop \
             -Imino/src/regex -Imino/src/diag -Imino/src/vendor/imath
CFLAGS   ?= -std=c99 -Wall -Wpedantic -Wextra -O2 $(MINO_INCS)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 $(MINO_INCS)
LDFLAGS  ?=
LIBS     ?= -lm

MINO_SRCS := mino/src/eval/mino.c mino/src/diag/diag.c mino/src/eval/eval_special.c \
             mino/src/eval/eval_special_defs.c mino/src/eval/eval_special_bindings.c \
             mino/src/eval/eval_special_control.c mino/src/eval/eval_special_fn.c \
             mino/src/runtime/runtime_state.c mino/src/runtime/runtime_var.c \
             mino/src/runtime/runtime_error.c mino/src/runtime/runtime_env.c \
             mino/src/gc/runtime_gc.c mino/src/gc/runtime_gc_roots.c \
             mino/src/gc/runtime_gc_major.c mino/src/gc/runtime_gc_barrier.c \
             mino/src/gc/runtime_gc_minor.c mino/src/gc/runtime_gc_trace.c \
             mino/src/runtime/runtime_module.c \
             mino/src/public/public_gc.c mino/src/public/public_embed.c \
             mino/src/collections/val.c mino/src/collections/vec.c \
             mino/src/collections/map.c mino/src/collections/rbtree.c \
             mino/src/eval/read.c mino/src/eval/print.c \
             mino/src/prim/prim.c mino/src/prim/prim_numeric.c \
             mino/src/prim/prim_bignum.c mino/src/vendor/imath/imath.c \
             mino/src/prim/prim_collections.c \
             mino/src/prim/prim_sequences.c mino/src/prim/prim_lazy.c \
             mino/src/prim/prim_string.c mino/src/prim/prim_io.c \
             mino/src/prim/prim_reflection.c mino/src/prim/prim_meta.c \
             mino/src/prim/prim_regex.c \
             mino/src/prim/prim_stateful.c mino/src/prim/prim_module.c \
             mino/src/prim/prim_fs.c mino/src/prim/prim_proc.c \
             mino/src/prim/prim_host.c mino/src/interop/host_interop.c \
             mino/src/collections/clone.c mino/src/regex/re.c \
             mino/src/collections/transient.c \
             mino/src/async/async_scheduler.c mino/src/async/async_timer.c \
             mino/src/prim/prim_async.c
MINO_OBJS := $(MINO_SRCS:.c=.o)

# C examples
C_SRCS := src/api_stress_test.c src/clone_test.c src/integration_test.c \
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

mino/src/prim/prim.o: mino/src/prim/prim.c mino/src/core_mino.h

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

$(REGEX_BIN): src/regex_thread_test.c mino/src/regex/re.c mino/src/regex/re.h
	$(CC) $(CFLAGS) -pthread -o $@ src/regex_thread_test.c mino/src/regex/re.c

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
