Building Robo 3T (macOS and Linux)  
==================

A. Prerequisites
-------------

1. Modern Compiler
A modern and complete C++17 compiler:  
```
GCC 8.0 or newer
Clang 7 (or Apple XCode 10 Clang) or newer
On Linux and macOS, the libcurl library and header is required. macOS includes libcurl.
  Fedora/RHEL - dnf install libcurl-devel
  Ubuntu/Debian - apt-get install libcurl-dev
Python 3.7
```

For details see: https://github.com/mongodb/mongo/wiki/Build-Mongodb-From-Source  

2. Install CMake (3.10.0 or later) 

Example:  
```
sudo apt-get install build-essential
// or 
wget http://www.cmake.org/files/v3.10/cmake-3.10.0.tar.gz
tar xf cmake-3.10.0.tar.gz
cd cmake-3.10.0
./configure // macos: env CC=gcc ./bootstrap
make
sudo make install
(sudo make uninstall)
```

3. Install [Scons](http://scons.org/tag/releases.html) (3.1.2 or later) 

```
cd scons-**
python setup.py install
```
   
4. Install Qt 

macOS: Qt 5.12.8   
Linux: Qt 5.9.3  

  ```sh
Example installation for macOS and Ubuntu:
    Go to http://download.qt.io/archive/qt/5.x/5.x.y/
    Download, run and install 
      qt-opensource-mac-x64-clang-5.x.y.dmg (for macOS) 
      qt-opensource-linux-x64-5.x.y.run (for Linux)
    After successful installation you should have 
      /path/to/qt-5.x.y/5.x/clang_64 (for macOS)
      /path/to/qt-5.x.y/5.x/gcc_64 (for Linux)

More information for installing on Ubuntu:
https://wiki.qt.io/Install_Qt_5_on_Ubuntu
```

5. Download and Build OpenSSL - (explained below in section B) 

B. Building Robo 3T and Dependencies
-------------
Ref: https://wiki.openssl.org/index.php/Compilation_and_Installation#OS_X  

#### 1. Build OpenSSL 

(Todo: Can we find pre-built OpenSSL? To check => https://wiki.openssl.org/index.php/Binaries)

Version 1.1.1f (Mar/2020) on macOS  
Version 1.0.2o (Mar/2018) on Linux  

**macOS:**  
```sh
wget https://www.openssl.org/source/old/1.1.1/openssl-1.1.1f.tar.gz
tar -xf openssl-1.1.1f.tar.gz
cd /opt/openssl-1.1.1f

(make clean) 
./Configure darwin64-x86_64-cc shared no-ssl2 no-ssl3 no-comp -mmacosx-version-min=xy.zt
    // Notes: 
    // 1. Change -mmacosx-version same as CMAKE_OSX_DEPLOYMENT_TARGET in 
    //    https://github.com/Studio3T/robomongo/blob/master/CMakeLists.txt
    // 2. With openssl version 1.1.1, configure command with rpath stopped working  
    // Last working configure command with rpath was: 
    // ./Configure darwin64-x86_64-cc shared --openssldir="@rpath"

make (or sudo make)  

// Verify libssl.dylib and libcrypto.dylib files are created 
ll lib*.dylib

// Due to broken './Configure' command with rpath above, these extra steps are also required:
install_name_tool -id "@rpath/lib/libssl.1.1.dylib" libssl.1.1.dylib
install_name_tool -change /usr/local/lib/libcrypto.1.1.dylib @rpath/lib/libcrypto.1.1.dylib libssl.1.1.dylib
install_name_tool -id "@rpath/lib/libcrypto.1.1.dylib" libcrypto.1.1.dylib
// Finally, we should have this output using otool:
otool -L libssl.1.1.dylib 
libssl.dylib:
	@rpath/lib/libssl.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	@rpath/lib/libcrypto.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)

otool -L libcrypto.1.1.dylib 
libcrypto.dylib:
	@rpath/lib/libcrypto.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)
	
// For robo-unit-test dependency:
mkdir lib/
cp lib*.dylib/lib/	
```


**Linux:**

  ```sh
wget https://www.openssl.org/source/old/1.0.2/openssl-1.0.2o.tar.gz
tar -xf openssl-1.0.2o.tar.gz
cd /opt/openssl-1.0.2o
(make clean)
./config shared
make
// Verify libssl.so and libcrypto.so are created
```

#### 2. Build Robo 3T Shell (fork of MongoDB)

Clone Robo 3T Shell and make sure you are on latest branch:

  ```sh
  $ git clone https://github.com/paralect/robomongo-shell.git
  $ cd robomongo-shell
  $ git branch   // "roboshell-v4.2"
  ```

Set special environment variable `ROBOMONGO_CMAKE_PREFIX_PATH` to point to a set of 
directories:

1. Location of Qt 5 SDK  
2. Location of Robo 3T Shell  
3. Location of OpenSSL  

Separate directories by semicolon `;` (not colon):

    // macOS example:
    export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.x.y/5.x/clang_64;/opt/robo-shell;/opt/openssl-x.y.z"
    
    // Ubuntu example:
    export ROBOMONGO_CMAKE_PREFIX_PATH="/home/<user>/Qt5.x.y/5.x.y/gcc_64/;/opt/<user>/robo-shell;/opt/openssl-x.y.z"

Install pip requirements

```
// macOS / Linux
pip3 install --user -r etc/pip/compile-requirements.txt
pip3 install --user -r etc/pip/dev-requirements.txt
```

```
// For ubuntu, the followings might be needed
sudo aptitude install libcurl-dev

sudo pip3 install Typing
sudo pip3 install Cheetah
```

Build Robo 3T Shell (in release mode by default):

  ```sh
  $ bin/build
  ```

Build Robo 3T Shell in debug mode:

  ```sh
  $ bin/build debug
  ```
  
Clean build files for release mode:
  ```sh
  $ bin/clean
  ```

Clean build files for debug mode:
  ```sh
  $ bin/clean debug
  ```
  
For more information refer to [Building Robo 3T Shell](BuildRobo3TShell.md) 

#### 3. Build Robo 3T

Download Robo 3T: 

    $ git clone https://github.com/Studio3T/robomongo
    $ cd robomongo

Run configuration step:
    
    $ bin/configure 
    
And finally, build Robo 3T:
    
    $ bin/build 

To run robomongo:

    $ bin/run
    

**Debug mode**

For debug mode append `debug` for each command
e.g. `bin/configure debug` or  `bin/build debug` etc..

**Helper commands**
    
Clean build files (in order to start build from scratch):

    $ bin/clean
    
Install Robo 3T to `build/release/install` folder:

    $ bin/install
    
Create package for your OS in `build/release/package` folder:

    $ bin/pack

**Important Notes**
- For macOS builds, it might be needed to patch QScintilla with `QScintilla-2.9.3-xcode8.patch` found at robomongo repository's root.

- For Ubuntu 16.04 builds, it has been reported that Robo 3T /bin/configure step might fail and installing `mesa-common-dev` package solves it. Details: https://github.com/paralect/robomongo/issues/1268 

- For Centos builds, Robo 3T /bin/configure step might fail due to error: `failed to find gl/gl.h`. In this case, the following packages must be installed:

```sh
$ sudo yum install mesa-libGL
$ sudo yum install mesa-libGL-devel
  ```


C. Building on Apple Silicon Macs with a modern Xcode
-------------

Tested in October 2026 on macOS 27.2 with Xcode 27.0 (Apple clang 21) on Apple Silicon.

MongoDB 4.2, which Robo 3T embeds, has no Apple Silicon support, so this recipe builds an
Intel (x86_64) app that runs through Rosetta. Apple keeps Rosetta fully available only through
macOS 27, so treat this build as a stopgap.

Things that differ from the steps above:

1. Xcode's compiler runs natively and targets arm64 even when started from an Intel process,
   so every component must be told to build for x86_64 explicitly.
2. If Homebrew's binutils is on your `PATH`, its GNU `ar` writes archives that Apple's linker
   rejects. Pass `/usr/bin/ar` and `/usr/bin/ranlib` explicitly, as shown below.
3. OpenSSL 1.1.1 is end of life, so this recipe uses OpenSSL 3.5 (LTS).
4. `bin/configure` loses extra CMake arguments under macOS's bash 3.2, so call CMake directly.
5. CMake 4 refuses projects that declare a minimum version below 3.5, like the bundled
   googletest 1.8.1. `CMAKE_POLICY_VERSION_MINIMUM=3.5` lets it configure them anyway.

GitHub Actions runs the same steps in `.github/workflows/release.yml`: a manual run builds the
app, and pushing a `v<version>` tag also publishes a GitHub release.

#### 1. Build Robo 3T Shell (MongoDB 4.2 fork)

[`patches/robomongo-shell-v4.2-modern-macos.patch`](patches/robomongo-shell-v4.2-modern-macos.patch)
fixes code that newer clang and libc++ reject: Boost 1.70 enums used in constant expressions,
an `s2` comparator, a zlib `fdopen` macro clash with the macOS SDK, and a SpiderMonkey move
constructor that referred to the wrong member. `MONGO_VERSION` is required because a shallow
clone has no tags.

```sh
git clone --depth 1 --branch roboshell-v4.2 https://github.com/paralect/robomongo-shell.git
cd robomongo-shell
git apply /path/to/robomongo/docs/patches/robomongo-shell-v4.2-modern-macos.patch

arch -x86_64 /usr/bin/python3 -m venv ../venv-x86
arch -x86_64 ../venv-x86/bin/pip install -r etc/pip/compile-requirements.txt

F="-arch x86_64 -mmacosx-version-min=11.0 -Wno-unused-function -D_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION"
arch -x86_64 ../venv-x86/bin/python buildscripts/scons.py mongo --ssl MONGO_VERSION=4.2.6 \
    "CCFLAGS=$F" "LINKFLAGS=-arch x86_64 -mmacosx-version-min=11.0" \
    --disable-warnings-as-errors -j16
```

#### 2. Install Qt 5.15.2 (x86_64)

```sh
arch -x86_64 ../venv-x86/bin/pip install aqtinstall
arch -x86_64 ../venv-x86/bin/aqt install-qt mac desktop 5.15.2 clang_64 -m qtwebengine \
    -O Qt -b https://download.qt.io/
```

Check that `Qt/5.15.2/clang_64/lib/QtWebEngineWidgets.framework` exists afterwards. Some
download mirrors fail partway and the installer still exits successfully.

#### 3. Build OpenSSL 3.5 (x86_64)

Download the latest `openssl-3.5.x.tar.gz` and its `.sha256` file from
https://github.com/openssl/openssl/releases and compare them with `shasum -a 256` first.

```sh
tar -xzf openssl-3.5.x.tar.gz && cd openssl-3.5.x
./Configure darwin64-x86_64-cc shared no-ssl3 no-comp no-tests -mmacosx-version-min=11.0 \
    AR=/usr/bin/ar RANLIB=/usr/bin/ranlib
make -j16 build_libs

install_name_tool -id "@rpath/lib/libcrypto.3.dylib" libcrypto.3.dylib
install_name_tool -id "@rpath/lib/libssl.3.dylib" \
    -change /usr/local/lib/libcrypto.3.dylib @rpath/lib/libcrypto.3.dylib libssl.3.dylib
codesign -f -s - libssl.3.dylib libcrypto.3.dylib

# For robo_unit_tests
mkdir -p lib && cp -a libssl.3.dylib libcrypto.3.dylib lib/
```

#### 4. Build Robo 3T

```sh
mkdir -p build/release && cd build/release
cmake -D "CMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/clang_64;/path/to/robomongo-shell;/path/to/openssl-3.5.x" \
      -D CMAKE_BUILD_TYPE=Release -D "CMAKE_INSTALL_PREFIX=$PWD/install" \
      -D CMAKE_OSX_ARCHITECTURES=x86_64 -D CMAKE_AR=/usr/bin/ar -D CMAKE_RANLIB=/usr/bin/ranlib \
      -D CMAKE_POLICY_VERSION_MINIMUM=3.5 \
      ../..
cmake --build . -- -j16
./src/robomongo-unit-tests/robo_unit_tests

# Self contained app bundle in build/release/install/Robo 3T.app
cmake --build . --target install
```
