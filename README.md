# pgm-image-editor
Distributed PGM Image Editor

A simple distributed  editor for images in PGM format.
The editor uses C language and OpenMPI, for faster and distributed execution on large inputs. Several computers are connected within a given topology and the image is recursively split into strips and sent to the leaves of the tree of the topology, which modify those strips and send them back to their parent, which reconstructs the strip back
It can apply several kernel-based filters (sharpen, edge detect, box blur, gaussian blur) on images given as input.
