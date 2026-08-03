# leaflitter format

This document describes the leaflitter byte format, version 1. It is a
reference for implementers. Bits are read most-significant-bit first.

## Container

A compressed stream starts with a 24-byte header. The header records the
original size, an Adler-32 checksum, and the window size.

| Offset | Size | Field |
| ------ | ---- | ----- |
| 0 | 1 | Magic byte `0x4C` ('L') |
| 1 | 1 | Magic byte `0x46` ('F') |
| 2 | 1 | Magic byte `0x01` |
| 3 | 1 | Format version, currently 1 |
| 4 | 8 | Original size, little-endian |
| 12 | 4 | Adler-32 of the original data |
| 16 | 4 | Window size in bytes |
| 20 | 4 | Reserved, must be zero |
| 24 | ... | Blocks |

The header is followed by one or more blocks. Each block is self-contained.
A final block flag marks the end of the stream.

The checksum covers the original bytes. The decoder verifies the checksum
after decoding. The decoder rejects streams with a wrong checksum.

## Block header

Every block starts with one header byte.

| Bit | Meaning |
| --- | ------- |
| 0-1 | Block type |
| 2 | Final block flag |
| 3-7 | Reserved, must be zero |

Block type 0 is a stored block. Block type 1 is a Huffman block. Types 2 and
3 are reserved.

## Stored block

A stored block copies its bytes without compression.

| Field | Size | Notes |
| ----- | ---- | ----- |
| Header | 1 | Type 0 |
| Length | 4 | Byte count, little-endian |
| Data | Length | Raw bytes |

The length must not be zero.

## Huffman block

A Huffman block carries code lengths and a token stream.

| Field | Size | Notes |
| ----- | ---- | ----- |
| Header | 1 | Type 1 |
| nlit | 2 | Literal and length code lengths |
| ndist | 2 | Distance code lengths |
| Literal lengths | varies | Encoded code lengths |
| Distance lengths | varies | Encoded code lengths |
| Tokens | varies | Huffman-coded tokens |
| Padding | varies | Zero bits to the byte boundary |

`nlit` must be between 257 and 288. `ndist` must be between 1 and 30.

## Symbol alphabets

Literal and length symbols cover three roles.

- Symbols 0 to 255 are literal bytes.
- Symbol 256 marks the end of a block.
- Symbols 257 to 272 are length codes.

Symbols 273 to 285 are reserved. A decoder must reject them.

Distance symbols 0 to 29 encode distances from 1 to 65534.

## Length codes

Length codes cover lengths 3 to 258. Each code has a base and extra bits.

| Code | Base | Extra bits |
| ---- | ---- | ---------- |
| 257 | 3 | 0 |
| 258 | 4 | 0 |
| 259 | 5 | 0 |
| 260 | 6 | 0 |
| 261 | 7 | 1 |
| 262 | 9 | 1 |
| 263 | 11 | 2 |
| 264 | 15 | 2 |
| 265 | 19 | 3 |
| 266 | 27 | 3 |
| 267 | 35 | 4 |
| 268 | 51 | 4 |
| 269 | 67 | 5 |
| 270 | 99 | 5 |
| 271 | 131 | 6 |
| 272 | 195 | 6 |

The length is the base plus the extra bits. A length code reads its extra
bits right after its Huffman code.

## Distance codes

Distance codes cover distances 1 to 65534.

| Code | Base | Extra bits |
| ---- | ---- | ---------- |
| 0 | 1 | 0 |
| 1 | 2 | 0 |
| 2 | 3 | 1 |
| 3 | 5 | 1 |
| 4 | 7 | 2 |
| 5 | 11 | 2 |
| 6 | 15 | 3 |
| 7 | 23 | 3 |
| 8 | 31 | 4 |
| 9 | 47 | 4 |
| 10 | 63 | 5 |
| 11 | 95 | 5 |
| 12 | 127 | 6 |
| 13 | 191 | 6 |
| 14 | 255 | 7 |
| 15 | 383 | 7 |
| 16 | 511 | 8 |
| 17 | 767 | 8 |
| 18 | 1023 | 9 |
| 19 | 1535 | 9 |
| 20 | 2047 | 10 |
| 21 | 3071 | 10 |
| 22 | 4095 | 11 |
| 23 | 6143 | 11 |
| 24 | 8191 | 12 |
| 25 | 12287 | 12 |
| 26 | 16383 | 13 |
| 27 | 24575 | 13 |
| 28 | 32767 | 14 |
| 29 | 49151 | 14 |

The distance is the base plus the extra bits. A decoder must reject a
distance that exceeds the window size.

## Code-length coding

The code lengths use a run-length scheme. A fixed code-length tree encodes
the symbols. The fixed tree has 19 symbols.

| Symbol | Meaning | Extra bits |
| ------ | ------- | ---------- |
| 0-15 | Literal length value | 0 |
| 16 | Repeat the last length | 2 |
| 17 | Emit 3 to 10 zeros | 3 |
| 18 | Emit 11 to 138 zeros | 7 |

Symbol 16 repeats the last length. Symbol 17 emits a run of zeros. Symbol 18
emits a longer run of zeros. The extra bits select the run length within the
range.

The fixed tree uses these code lengths for symbols 0 to 18.

```
4 5 5 5 5 6 6 6 6 6 6 6 6 6 6 6 3 2 2
```

The encoder writes exactly `nlit` literal lengths. It then writes exactly
`ndist` distance lengths. The decoder must reject an encoding that produces
more values than requested.

## Token stream

The token stream follows the code lengths. Every token is one of two kinds.

A literal token is one Huffman symbol in the literal and length tree. The
symbol value is the literal byte.

A match token has four parts.

1. The length symbol in the literal and length tree
2. The length extra bits
3. The distance symbol in the distance tree
4. The distance extra bits

A match copies `length` bytes from `distance` bytes back. The decoder copies
byte by byte. That order allows overlapping matches.

The end-of-block symbol ends the token stream. The block then pads with zero
bits to the byte boundary.

## Decoder rules

The decoder must reject malformed streams.

- The magic bytes must match.
- The format version must be 1.
- The reserved fields must be zero.
- `nlit` and `ndist` must stay in range.
- Reserved symbols are invalid.
- A distance must not exceed the produced output.
- A block must not exceed the declared size.

A decoder returns a specific error for each failure class. See `lf_result`
in `lf_common.h`.
