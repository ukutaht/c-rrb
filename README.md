### Relaxed Radix Balanced Trees (RRB-trees)

An immutable vector-like data structure with very good performance
characteristics for concatenation and slicing. Also provides transient
(mutable-like) variants which you can convert to and from in constant time.

| Operation |  Eff. runtime |
| --------- | --------------|
| Lookup    | O(~1)         |
| Last      | O(1)          |
| Count     | O(1)          |
| Update    | O(~1)         |
| Append    | O(~1)         |
| Pop       | O(~1)         |
| Iteration | O(n)*         |
| Concat    | O(log n)      |
| Slice     | O(~1)         |

* Amortised constant time – O(1) – per element.

For an explanation on how this data structure works in detail, read
http://infoscience.epfl.ch/record/169879/files/RMTrees.pdf and then my thesis:
http://hypirion.com/thesis

This library, unmodified, depends on CMake and Boehm GC.


#### Installing

On OSX (tested with Yosemite), you can install
CMake and Boehm GC with Homebrew like this:

```sh
$ brew install boehmgc cmake
```

To build and install the library, run:

```sh
$ bin/install
```

To run tests:

```sh
$ bin/test
```

Copyright © 2013-2014 Jean Niklas L'orange

Distributed under the MIT License (MIT). You can find a copy in the root of this
project with the name COPYING.
