# Cross-compile your embedding with `zig cc`

mino is pure C99, so embedding it needs only your host `cc`. But when
you want to ship your mino-embedding host program to *other* platforms
from one machine — without provisioning a toolchain per target —
[`zig cc`](https://ziglang.org) is a drop-in cross-compiler with
first-class cross targets and a bundled libc for each.

This is an **optional convenience**, never a requirement: mino itself
is built and embedded with any C99 compiler, and the published mino
binaries do not depend on zig. This page just shows the one-command
cross-build for embedders who find it useful.

## The shape

Compile two C files together: the single-file mino amalgamation and
your embedding source.

```
# Generate the amalgamation once (from the mino checkout):
cd mino && ./mino task amalgamate    # writes mino/dist/mino.c + mino.h
```

Then one `zig cc` invocation per target, pointing `-I` at `dist/`:

```sh
# Linux x86_64 (static musl: runs on any Linux, nothing to install)
zig cc --target=x86_64-linux-musl  -std=c99 -O2 -I mino/dist \
    -o myhost-linux-amd64        myhost.c mino/dist/mino.c -lm

# Linux ARM64 (static musl)
zig cc --target=aarch64-linux-musl -std=c99 -O2 -I mino/dist \
    -o myhost-linux-arm64        myhost.c mino/dist/mino.c -lm

# Windows x86_64 (mingw; the PE imports only the system CRT + KERNEL32)
zig cc --target=x86_64-windows-gnu -std=c99 -O2 -I mino/dist \
    -o myhost-windows-amd64.exe  myhost.c mino/dist/mino.c -lm
```

Swap `myhost.c` for any embedding in this repo — e.g.
`src/cookbook/five_minutes.c` — to see it cross-compile end to end.

## Targets

| Target triple | Output | Notes |
|---|---|---|
| `x86_64-linux-musl` | static ELF | zero-dependency; runs on glibc or musl Linux |
| `aarch64-linux-musl` | static ELF | same, ARM64 |
| `x86_64-windows-gnu` | PE32+ | no `libgcc_s_seh` / `libwinpthread` runtime DLLs |

These mirror the targets mino's own release pipeline cross-builds with
the pinned `zig cc`.

### macOS is native-only

Zig bundles no macOS SDK, so a darwin binary (which links libSystem)
cannot be cross-built from a non-darwin host. Build your macOS host
program natively with Apple clang (or `zig cc` *on* a Mac). This is the
same boundary mino's own release matrix observes.

## Why static musl for Linux

`--target=x86_64-linux-musl` links the C runtime statically by
default, so the result is a single file that runs on any Linux — old
or new, glibc or musl/Alpine — with nothing to install. That is the
zero-dependency distribution shape, matching how mino ships its own
Linux binaries.

## Pinning the toolchain (optional)

For byte-reproducible cross-builds, pin a specific zig version (mino
itself pins one for stencil/codegen reproducibility). Any recent zig
works for a plain embedding; pin only if you need identical output
across machines.
