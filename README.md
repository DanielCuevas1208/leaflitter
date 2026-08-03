# leaflitter

leaflitter compresses byte streams with LZ77 matching and canonical Huffman
coding. The design follows DEFLATE but stays simpler. The library is small,
portable, and free of external dependencies.

## Features

- LZ77 hash-chain match finder with a sliding window
- Canonical Huffman coding with a 15-bit length limit
- Stored and Huffman block types
- Adler-32 integrity check on decompression
- C11 API and a command line tool
- Deterministic output for a given input
- No third-party runtime dependencies

## Build

leaflitter uses CMake. Configure, build, and test from the project root.

```
cmake -B build
cmake --build build
ctest --test-dir build
```

Install the library and headers with the following command.

```
cmake --install build
```

The tests use the vendored Unity framework. Set `BUILD_TESTING=OFF` to skip
the tests. Set `LEAFLITTER_BUILD_TOOLS=OFF` to skip the tool and the sample.

## Use the library

Compress and decompress a byte buffer in memory.

```c
#include "lf_buf.h"
#include "lf_stream.h"

uint8_t input[] = "hello hello hello";
lf_buf packed;
lf_buf plain;
lf_result r;

lf_buf_init(&packed);
lf_buf_init(&plain);

r = lf_compress(NULL, input, sizeof(input) - 1, &packed);
if (r != LF_OK) {
    return r;
}

r = lf_decompress(packed.data, packed.len, &plain);
if (r != LF_OK) {
    return r;
}
```

Pass `NULL` for options to use the defaults. Build a custom `lf_options`
struct to change the block size, window size, or match chain length. Call
`lf_options_default` to fill that struct first.

Compression accepts inputs up to 4 GiB. Decompression accepts any valid
stream. Use `lf_compress_bound` to size an output buffer before compression.

## Use the command line tool

The tool compresses and decompresses files.

```
leaflitter c <in> <out>
leaflitter d <in> <out>
leaflitter i <in>
```

Command `c` compresses `<in>` to `<out>`. Command `d` restores the original
bytes. Command `i` prints the stream header.

## Format

The container format is documented in `docs/FORMAT.md`. The header stores the
original size, an Adler-32 checksum, and the window size. A stream ends with
a final block flag.

The format is new. It is not compatible with DEFLATE or gzip.

## Roadmap

See `docs/ROADMAP.md` for the completed work and the planned work.

## Limitations

- Compression inputs are limited to 4 GiB.
- The code-length tree is fixed, not adaptive.
- The encoder searches one hash chain per position.
- Compression is single-threaded.

## Testing

The tests use Unity and run through CTest. Every module has a test file.
Round-trip tests cover small, large, and random inputs. Golden vectors lock
the byte format. Corruption tests check that bad streams fail cleanly.

## License

This project is licensed under the MIT License. See `LICENSE` for details.
