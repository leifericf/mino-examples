# mino-examples -- embedding examples, cookbook, use-cases, and bindings.

CC       ?= cc
CXX      ?= c++
MINO_INCS := -Imino/src -Imino/src/public -Imino/src/runtime \
             -Imino/src/gc -Imino/src/eval -Imino/src/collections \
             -Imino/src/prim -Imino/src/async -Imino/src/interop \
             -Imino/src/diag -Imino/src/vendor/imath
CFLAGS   ?= -std=c99 -Wall -Wpedantic -Wextra -O2 $(MINO_INCS)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 $(MINO_INCS)
LDFLAGS  ?=
LIBS     ?= -lm

MINO_SRCS := $(wildcard mino/src/public/*.c) \
             $(wildcard mino/src/runtime/*.c) \
             $(wildcard mino/src/gc/*.c) \
             $(wildcard mino/src/eval/*.c) \
             $(wildcard mino/src/collections/*.c) \
             $(wildcard mino/src/prim/*.c) \
             $(wildcard mino/src/async/*.c) \
             $(wildcard mino/src/interop/*.c) \
             $(wildcard mino/src/regex/*.c) \
             $(wildcard mino/src/diag/*.c) \
             $(wildcard mino/src/vendor/imath/*.c)
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
# namespace; these are gitignored generated artifacts.

define gen-mino-header
mino/src/$(2).h: mino/$(1)
	@printf 'static const char *$(2)_src =\n' > $$@
	@sed 's/\\/\\\\/g; s/"/\\"/g; s/^/    "/; s/$$$$/\\n"/' $$< >> $$@
	@printf '    ;\n' >> $$@
endef

$(eval $(call gen-mino-header,src/core.clj,core_mino))
$(eval $(call gen-mino-header,lib/clojure/string.clj,lib_clojure_string))
$(eval $(call gen-mino-header,lib/clojure/set.clj,lib_clojure_set))
$(eval $(call gen-mino-header,lib/clojure/walk.clj,lib_clojure_walk))
$(eval $(call gen-mino-header,lib/clojure/edn.clj,lib_clojure_edn))
$(eval $(call gen-mino-header,lib/clojure/pprint.clj,lib_clojure_pprint))
$(eval $(call gen-mino-header,lib/clojure/zip.clj,lib_clojure_zip))
$(eval $(call gen-mino-header,lib/clojure/data.clj,lib_clojure_data))
$(eval $(call gen-mino-header,lib/clojure/test.clj,lib_clojure_test))
$(eval $(call gen-mino-header,lib/clojure/template.clj,lib_clojure_template))
$(eval $(call gen-mino-header,lib/clojure/repl.clj,lib_clojure_repl))
$(eval $(call gen-mino-header,lib/clojure/stacktrace.clj,lib_clojure_stacktrace))
$(eval $(call gen-mino-header,lib/clojure/datafy.clj,lib_clojure_datafy))
$(eval $(call gen-mino-header,lib/clojure/core/protocols.clj,lib_clojure_core_protocols))
$(eval $(call gen-mino-header,lib/clojure/instant.clj,lib_clojure_instant))
$(eval $(call gen-mino-header,lib/clojure/spec/alpha.clj,lib_clojure_spec_alpha))
$(eval $(call gen-mino-header,lib/clojure/core/specs/alpha.clj,lib_clojure_core_specs_alpha))
$(eval $(call gen-mino-header,lib/mino/deps.clj,lib_mino_deps))
$(eval $(call gen-mino-header,lib/mino/tasks.clj,lib_mino_tasks))
$(eval $(call gen-mino-header,lib/mino/tasks/builtin.clj,lib_mino_tasks_builtin))

MINO_GEN_HEADERS := mino/src/core_mino.h \
                    mino/src/lib_clojure_string.h \
                    mino/src/lib_clojure_set.h \
                    mino/src/lib_clojure_walk.h \
                    mino/src/lib_clojure_edn.h \
                    mino/src/lib_clojure_pprint.h \
                    mino/src/lib_clojure_zip.h \
                    mino/src/lib_clojure_data.h \
                    mino/src/lib_clojure_test.h \
                    mino/src/lib_clojure_template.h \
                    mino/src/lib_clojure_repl.h \
                    mino/src/lib_clojure_stacktrace.h \
                    mino/src/lib_clojure_datafy.h \
                    mino/src/lib_clojure_core_protocols.h \
                    mino/src/lib_clojure_instant.h \
                    mino/src/lib_clojure_spec_alpha.h \
                    mino/src/lib_clojure_core_specs_alpha.h \
                    mino/src/lib_mino_deps.h \
                    mino/src/lib_mino_tasks.h \
                    mino/src/lib_mino_tasks_builtin.h

mino/src/prim/install.o: mino/src/prim/install.c mino/src/core_mino.h
mino/src/prim/install_stdlib.o: mino/src/prim/install_stdlib.c $(MINO_GEN_HEADERS)

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
	rm -f $(MINO_OBJS) $(ALL_BINS) $(MINO_GEN_HEADERS)
