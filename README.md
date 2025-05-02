# Defl-8bit

A direct-synthesis [gzip][] encoder, aligning symbols to 8-bit
boundaries to make packing more expedient.

## motivation

Generating text by pasting strings together involves a lot of memcpy
operations, and if additional CPU time has to be spent compressing that
text, then why not go directly to delivering the copy operations to the
client rather than performing them locally?

This hurts compression ratio, obviously, but it may save CPU time, which
is the principal cost of operating [Yellow Dust ECB5][] outside of
Cloudflare's free tier.

Use of gzip certainly wouldn't be first choice for this kind of
approach, but it's the compression scheme supported by most clients, and
so here we are.

## design

In brief, it's possible to set the lengths of the Huffman symbols used
by [Deflate][] to be 8-bit lengths, or to fall on 8-bit boundaries when
combined in expected tuples.  For example, a match/distance pair can be
fixed at 9 and 15 bits respectively for a three-byte code, and UTF-8
symbols can be padded to land on 8-bit boundaries when properly formed.
This leaves enough coding space to assign 8-bit symbols to printable
ASCII and a few essential control characters.  Then a few unusable
symbols need to be inserted into gaps to make the bitstream legal.

The header describing these codes can also need a small amount of
manipulation to ensure that it ends on an 8-bit boundary.

Extra details [here][blog].


[Yellow Dust ECB5]: <https://github.com/sh1boot/madlib123>
[blog]: <https://www.tīkōuka.dev/more-efficient-nonsense-text/>
[gzip]: <https://en.wikipedia.org/wiki/gzip>
[Deflate]: <https://en.wikipedia.org/wiki/Deflate>
