# mino-examples -- embedding examples, cookbook, use-cases, and bindings.

CC       ?= cc
CXX      ?= c++
MINO_INCS := -Imino/src -Imino/src/generated -Imino/src/public \
             -Imino/src/runtime -Imino/src/gc -Imino/src/eval \
             -Imino/src/read -Imino/src/print -Imino/src/names \
             -Imino/src/state -Imino/src/values -Imino/src/collections \
             -Imino/src/prim -Imino/src/async -Imino/src/interop \
             -Imino/src/diag -Imino/src/vendor/imath \
             -Imino/src/vendor/bearssl -Imino/src/vendor/bearssl/inc \
             -Imino/src/vendor/miniz -Imino/src/vendor/miniz/upstream
CFLAGS   ?= -std=c99 -Wall -Wpedantic -Wextra -O2 $(MINO_INCS)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 $(MINO_INCS)
LDFLAGS  ?=
LIBS     ?= -lm -lpthread

MINO_SRCS := $(wildcard mino/src/eval/*.c) \
             $(wildcard mino/src/eval/bc/*.c) \
             $(wildcard mino/src/eval/bc/jit/*.c) \
             $(wildcard mino/src/read/*.c) \
             $(wildcard mino/src/print/*.c) \
             $(wildcard mino/src/diag/*.c) \
             $(wildcard mino/src/names/*.c) \
             $(wildcard mino/src/state/*.c) \
             $(wildcard mino/src/gc/*.c) \
             $(wildcard mino/src/public/*.c) \
             $(wildcard mino/src/values/*.c) \
             $(wildcard mino/src/collections/*.c) \
             $(wildcard mino/src/prim/*/*.c) \
             $(wildcard mino/src/interop/*.c) \
             $(wildcard mino/src/regex/*.c) \
             $(wildcard mino/src/async/*.c) \
             $(wildcard mino/src/vendor/imath/*.c) \
             $(wildcard mino/src/vendor/bearssl/*.c) \
             $(wildcard mino/src/vendor/miniz/*.c)
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

# --- Bundled-stdlib generated headers ---
# install_stdlib.c #includes one C string-literal header per bundled
# namespace under mino/src/generated/, driven by mino/src/bundled.list.
# These are gitignored generated artifacts. Run `make -C mino` once to
# emit them (a mino binary is linked as a harmless side effect).

MINO_GEN_HEADERS := $(wildcard mino/src/generated/*.h)

mino/src/prim/core/install.o: mino/src/prim/core/install.c
mino/src/prim/core/install_stdlib.o: mino/src/prim/core/install_stdlib.c $(MINO_GEN_HEADERS)

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

$(REGEX_BIN): src/regex_thread_test.c mino/src/regex/re.c mino/src/regex/re_compile.c mino/src/regex/re_match.c mino/src/regex/re.h
	$(CC) $(CFLAGS) -pthread -o $@ src/regex_thread_test.c mino/src/regex/re.c mino/src/regex/re_compile.c mino/src/regex/re_match.c

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
	rm -f $(MINO_OBJS) $(ALL_BINS) $(MINO_GEN_HEADERS)
