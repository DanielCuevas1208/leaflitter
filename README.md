# Leaflitter

Leaflitter is a compact C workbench for experimenting with buffered I/O,
bit-level encoding, and Huffman compression.

## Current status

The current `main` branch contains the core C modules. The build, test, and
command-line workflow is still under development. APIs may change.

## Source layout

- `src/lf_buf.c` and `src/lf_buf.h` - growable byte buffers
- `src/lf_bitio.c` and `src/lf_bitio.h` - bit-level input and output
- `src/lf_huffman.c` and `src/lf_huffman.h` - Huffman tree and coding logic
- `src/lf_common.h` - shared types and utility definitions
- `third_party/unity` - Unity test framework source

## Build status

The public `main` branch does not yet include a build file or a supported build
command. Review the source and the header files directly while the build
workflow is completed.

## Design notes

The modules keep storage and encoding concerns separate. The buffer module
owns byte storage. The bit stream module handles individual bits. The Huffman
module builds on those primitives.

## Limitations

This is an early workbench. It is not a production compression utility. The
project does not promise a stable file format or a complete command-line tool.

## License

No license file is published yet. Treat this repository as an experimental
project until a license is added.
