# leaflitter roadmap

This page tracks the project plan. Each item has a status.

## Complete

### Release 1.1.0 - full pipeline

This release turns the building blocks into a working compressor.

- Byte buffer (`lf_buf`)
- Bit reader and writer (`lf_bitio`)
- Canonical Huffman coder (`lf_huffman`)
- Adler-32 checksum (`lf_adler32`)
- LZ77 hash-chain match finder (`lf_match`)
- Block codec with stored and Huffman blocks (`lf_block`)
- Stream container with header and checksum (`lf_stream`)
- Command line tool
- CMake build system
- Unit tests with Unity
- GitHub Actions workflow

### Release 1.0.0 - foundations

- Byte buffer (`lf_buf`)
- Bit reader and writer (`lf_bitio`)
- Canonical Huffman coder (`lf_huffman`)

## Planned

The list below is not committed. Items appear in rough priority order.

### Improve compression

- [ ] Adaptive code-length tree
- [ ] Lazy matching in the encoder
- [ ] Better hash functions and chain selection
- [ ] Skip-huffman fast path for incompressible data
- [ ] Match-finder tuning per data type

### Expand the API

- [ ] Incremental compression API
- [ ] Incremental decompression API
- [ ] Preset dictionary support
- [ ] Random access to blocks

### Expand the tooling

- [ ] Level presets in the command line tool
- [ ] Parallel block compression
- [ ] Benchmark harness against DEFLATE tools
- [ ] Bindings for other languages
