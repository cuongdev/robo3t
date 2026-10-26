**Unofficial community build of Robo 3T**, maintained in this fork. It is not affiliated with, endorsed by, or supported by 3T Software Labs (Studio 3T), who own the Robo 3T name.

## Install on macOS

1. Download the `.dmg`, open it and drag **Robo 3T** into Applications.
2. {{SIGNATURE_NOTE}}
3. It is an Intel app. On Apple Silicon it runs through Rosetta, which Apple fully supports through macOS 27.

Check downloads against `SHA256SUMS.txt`.

Settings are kept per version in `~/.3T/robo-3t/<version>/`. On first launch, connections from Robo 3T 1.4.x are imported.

## Known limitations

* The embedded shell core is MongoDB 4.2. It was tested against MongoDB 5.0 and 9.0 (shell operations, browsing, newer data types, time series, clustered collections and views). On macOS, TLS connections to MongoDB use TLS 1.2 at most.
* Some bundled components are end of life upstream: the MongoDB 4.2 shell core and Qt 5.15.2, including its QtWebEngine browser engine. Check this against your organization's software policies before use.

## Licenses

Robo 3T is GPLv3, and the source for this build is this repository at the release tag. The embedded MongoDB shell code is under the SSPL. Its source is [paralect/robomongo-shell](https://github.com/paralect/robomongo-shell) (branch `roboshell-v4.2`) with `docs/patches/robomongo-shell-v4.2-modern-macos.patch` from this repository applied. Bundled third party components (Qt, OpenSSL, libssh2 and others) keep their own licenses.
