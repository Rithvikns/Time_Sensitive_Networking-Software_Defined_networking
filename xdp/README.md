# Setup build environment

The code examples in this folder are based on the following open-source projects:

* libbpf
* xdp-tutorial

These are referenced from this repository as GIT sub-modules (cf. `dependencies` folder). To check out the sub-modules, execute the following commands:

```
$ 
```

## Installing dependencies

Install the dependencies as described in the file `dependencies/xdp-tutorial/blob/master/setup_dependencies.org` to build the examples.

## Compile libbpf

```
$ cd dependencies/libbpf/src
$ mkdir build root
$ BUILD_STATIC_ONLY=y OBJDIR=build DESTDIR=root make install
```

This will compile libbpf as static library, and install the headers and library to the directory `root` in the `src` directory of libbpf, where our Makefiles expect them.
