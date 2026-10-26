Robo 3T (community fork)
========================

A maintained fork of [Robo 3T](https://github.com/Studio3T/robomongo) (formerly Robomongo), the
MongoDB GUI that embeds the real `mongo` shell in a tabbed interface, next to a database explorer
and document views.

3T Software Labs (Studio 3T) ended Robo 3T development in 2022 with version 1.4.4. This fork keeps
the app building and working on current macOS and current MongoDB servers, and adds features for
day to day use.

> **Unofficial.** This fork is not affiliated with, endorsed by, or supported by 3T Software Labs,
> who own the Robo 3T name. Please report problems with these builds here, not to Studio 3T.


Download
========

Get the latest build from [Releases](https://github.com/cuongdev/robo3t/releases/latest).

**macOS** (`robo3t-<version>-macos-x86_64.dmg`):

1. Open the `.dmg` and drag **Robo 3T** into Applications.
2. The app has an ad hoc signature and is not notarized by Apple, so macOS blocks the first launch.
   Right click the app and choose **Open**, or run:

   ```sh
   xattr -dr com.apple.quarantine "/Applications/Robo 3T.app"
   ```

3. It is an Intel app. On Apple Silicon it runs through Rosetta, which Apple fully supports
   through macOS 27 (see [Roadmap](#roadmap)).

Check downloads against `SHA256SUMS.txt` in the release.

Settings are kept per version in `~/.3T/robo-3t/<version>/`. On first launch, a new version imports
the connections and settings of the previous one.

Windows and Linux builds are not produced by this fork yet.


What's new in 1.5.0
===================

* **App Name**: choose the name each connection reports to MongoDB, shown in server logs and
  `db.currentOp()`, so you can tell who or which service is connecting. Set it per connection
  (Connection Settings, Advanced) or globally (Options, Default App Name, `application` by default).
* **Namespaces**: group connections into folders in the connections list and in the Connect menu.
  Set the namespace on the Connection tab, with **Move to Namespace...** (right click) or by drag and
  drop.
* **Export and import connections**: back up all connections to a file and import them again, on
  the same or another machine. Passwords are only written when you give a passphrase, encrypted
  with AES 256 GCM. Import never overwrites existing connections.
* **SSH tunnels work again** against current OpenSSH servers (libssh2 1.11.1 with OpenSSL 3).
* **Builds on current Macs**: Apple Silicon hosts, current Xcode, OpenSSL 3.5 (LTS).

Full history in [CHANGELOG](CHANGELOG).


Compatibility
=============

| | Tested |
| :--- | :--- |
| MongoDB servers | 5.0 and 9.0 (shell operations, browsing, time series, clustered collections, views, newer data types) |
| macOS | 27.2 on Apple Silicon through Rosetta. The app targets macOS 11 and newer. |
| Embedded shell core | MongoDB 4.2 |

Known limitations:

* On macOS, TLS connections to MongoDB use TLS 1.2 at most, so servers that only accept TLS 1.3
  are not reachable.
* Some bundled components are end of life upstream: the MongoDB 4.2 shell core and Qt 5.15.2,
  including its QtWebEngine browser engine. Check this against your organization's software
  policies before use.
* Releases are not notarized by Apple.


Building from source
====================

The macOS recipe, with every dependency and the fixes needed for current toolchains, is in
[docs/BuildRobo3TOnMacAndLinux.md](docs/BuildRobo3TOnMacAndLinux.md), section C. In short:

1. Build the MongoDB 4.2 shell fork ([paralect/robomongo-shell](https://github.com/paralect/robomongo-shell),
   branch `roboshell-v4.2`) with [docs/patches/robomongo-shell-v4.2-modern-macos.patch](docs/patches/robomongo-shell-v4.2-modern-macos.patch) applied.
2. Install Qt 5.15.2 and build OpenSSL 3.5, both for x86_64.
3. Configure and build this repository with CMake, run the unit tests, and install the app bundle.

[.github/workflows/release.yml](.github/workflows/release.yml) runs the same steps on GitHub
Actions, with pinned versions. It is the most precise reference for a working build.

The older guides in [docs](docs) (Windows, Linux, older macOS) come from the original project and
were not updated by this fork.


Maintaining
===========

### Releasing a version

1. Set the version in `CMakeLists.txt` (`PROJECT_VERSION_MAJOR`, `_MINOR`, `_PATCH`).
2. Add a `Robo 3T <version> (<Mon>/<year>), unofficial community build` entry at the top of
   [CHANGELOG](CHANGELOG). The release notes include it.
3. The settings folder depends on the version. Add the previous version's config file at the top of
   `_configFilesOfOldVersions` in
   [SettingsManager.cpp](src/robomongo/core/settings/SettingsManager.cpp), so users' connections
   are imported on first launch.
4. Commit, then tag and push:

   ```sh
   git tag -a v<version> -m "Robo 3T <version>"
   git push origin master v<version>
   ```

   The **Release** workflow builds the app, runs the unit tests, checks the bundle, and publishes a
   GitHub release with the `.dmg`, `.zip` and `SHA256SUMS.txt`. The tag must match the version in
   `CMakeLists.txt`. Nothing is published if the build fails.

Run the workflow by hand (Actions, Release, Run workflow) to build without releasing; the files are
attached to the run as an artifact.

The MongoDB shell build takes about an hour on a GitHub runner, so the workflow caches it, together
with Qt and OpenSSL. Change the `-v1` suffix of a cache key to force a rebuild.

### Updating dependencies

* **OpenSSL**: change `OPENSSL_VERSION` and `OPENSSL_SHA256` in the workflow (the SHA256 comes from
  the `.sha256` file of the [OpenSSL release](https://github.com/openssl/openssl/releases)).
  Stay on a supported LTS line.
* **libssh2**: vendored in `src/third-party/`; see its
  [README](src/third-party/libssh2-1.11.1/README.md) for the update steps, including signature
  checks.
* **MongoDB shell fork**: pinned by `ROBO_SHELL_COMMIT` in the workflow. If the patch needs changes,
  regenerate `docs/patches/robomongo-shell-v4.2-modern-macos.patch` with `git diff` in the fork.

### Tests

Unit tests use googletest and live next to the code they test (`Foo_test.cpp` beside `Foo.cpp`).
Register new test files in [src/robomongo-unit-tests/CMakeLists.txt](src/robomongo-unit-tests/CMakeLists.txt).
Run them with `build/release/src/robomongo-unit-tests/robo_unit_tests`.

Commit messages follow [Conventional Commits](https://www.conventionalcommits.org/).


Roadmap
=======

* **Native Apple Silicon build.** The most important item: Apple keeps Rosetta fully available only
  through macOS 27. MongoDB 4.2 has no Apple Silicon support, so this means moving the embedded
  shell core to a newer MongoDB release.
* TLS 1.3 to MongoDB on macOS (the current core uses Apple's legacy TLS library).
* Windows and Linux builds in CI.


Issues and contributions
========================

Report bugs and ideas in this repository's
[issues](https://github.com/cuongdev/robo3t/issues). Pull requests are welcome: please keep each one
focused on one change, and say what you tested (OS, MongoDB version).


History
=======

Robo 3T started as Robomongo, created at [Paralect](https://github.com/paralect), and was acquired
by 3T Software Labs in 2017. It embedded these MongoDB shell versions:

| Robo 3T | Embedded MongoDB shell |
| :--- | :--- |
| 1.4 and this fork | 4.2 |
| 1.3 | 4.0 |
| 1.1 | 3.4 |
| 0.9 | 3.2 |
| 0.8.x | 2.4.0 |

Thanks to everyone who built and supported Robo 3T over the years. The original repository is
[Studio3T/robomongo](https://github.com/Studio3T/robomongo), and its last official release is
1.4.4.


License
=======

Copyright 2014-2021 [3T Software Labs Ltd](https://studio3t.com/). Changes in this fork are
copyright their contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the
GNU General Public License version 3 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not,
see <http://www.gnu.org/licenses/>.

The embedded MongoDB shell code is licensed under the SSPL (see the
[robomongo-shell](https://github.com/paralect/robomongo-shell) repository). Bundled third party
components (Qt, OpenSSL, libssh2, QScintilla, QJson, googletest and others) keep their own licenses.
