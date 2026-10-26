How to configure libssh2 for Robo
=====================================

`sources` holds the unmodified libssh2 1.11.1 release tarball
(https://github.com/libssh2/libssh2/releases/tag/libssh2-1.11.1), checked against its
`.asc` signature from Daniel Stenberg (key `27EDEAF22F3ABCEB50DB9A125CC908FDB71E12C2`).

Why 1.11.1 instead of 1.9.0:

1. Works with OpenSSL 3. With 1.9.0 every SSH session fails with
   `LIBSSH2_ERROR_ENCRYPT` (-44), because OpenSSL 3 changed what `EVP_Cipher()` returns.
2. Signs RSA keys with `rsa-sha2-256`/`rsa-sha2-512`. OpenSSH 8.8 and newer reject the
   SHA1 `ssh-rsa` signatures that 1.9.0 sends, so RSA keys fail against current servers.
3. Includes security fixes, such as strict key exchange against the Terrapin attack
   (CVE-2023-48795).

To update to a newer release:

1. Download the release tarball and its `.asc` file and verify the signature.
2. Replace `sources` with the extracted tarball.
3. Update `SOURCES` in `CMakeLists.txt` from `CSOURCES` in `sources/src/Makefile.inc`, and
   compare the platform checks with `sources/CMakeLists.txt`.
4. Update `LIBSSH2_VERSION` in the root `CMakeLists.txt`.
