# Offline iOS build

This repository packages the `dhbloo/gomoku-calculator` web application in a
Capacitor iOS shell. The GitHub Actions workflow downloads the single-threaded
Rapfi WebAssembly engine and its data file into the app bundle so the installed
app can run without network access.

Upstream projects:

- https://github.com/dhbloo/gomoku-calculator
- https://github.com/dhbloo/rapfi

Rapfi is distributed under GPL-3.0. Preserve the applicable license and source
availability when redistributing builds.
