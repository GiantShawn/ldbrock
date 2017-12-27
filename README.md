LevelDB Rock!
====================================================================================================================================================

By Shawn Li

An leveldb playground that support REPL CLI interface and more for hacking leveldb. Currencyly mainly for my personal use.

Build
--------------
First, you should clone leveldb as a submodule as well.
```bash
$ git submodule init
$ git submodule update
```

Then you can start build ldbrock with scons.
Currently, only SCons build system is supported(Go and check it [here](http://www.scons.org/)).

```bash
$ scons
```


Example
-------
```bash
$ ./build/src/ldbrock
$ > open temp.db ++create_if_missing
$ > put name shawn
$ > get name
$ => shawn
$ > get ++all
$ => shawn
$ > load temp.txt
$ > close
$ > dropdb temp.db
$ > ^d
```

MIT License
----------------------------
Copyright (c) 2017 Shawn Li


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.