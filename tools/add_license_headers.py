#!/usr/bin/env python3
# FreeClimb LE - Copyright (c) 2026 Ezyel. SPDX-License-Identifier: GPL-3.0-only (see LICENSE).
"""Adds the GPL-3.0-only notice to the top of every freeclimb-le/*.cpp and *.h that does not have it yet.
Run it from the repository root:  python tools/add_license_headers.py
It is safe to run more than once: files that already have the notice are left alone."""
import glob, sys

HEADER = """// FreeClimb LE - wall climbing for Skyrim Legendary Edition.
// Copyright (c) 2026 Ezyel. Port of FreeClimb by Epsilona (Copyright (c) 2026 Epsilona), used with the author's permission.
// SPDX-License-Identifier: GPL-3.0-only
//
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General
// Public License, version 3, as published by the Free Software Foundation. It is distributed WITHOUT ANY
// WARRANTY; see the LICENSE file. Modified from the original FreeClimb: ported to Skyrim LE / SKSE 1.7.3 (2026).
"""

changed = 0
for path in sorted(glob.glob("freeclimb-le/*.cpp") + glob.glob("freeclimb-le/*.h")):
    raw = open(path, "rb").read()
    bom = raw.startswith(b"\xef\xbb\xbf")
    text = raw.decode("utf-8-sig")
    if "SPDX-License-Identifier: GPL-3.0-only" in text[:600]:
        continue
    header = HEADER.replace("\n", "\r\n") if "\r\n" in text else HEADER
    open(path, "wb").write((b"\xef\xbb\xbf" if bom else b"") + (header + text).encode("utf-8"))
    changed += 1
    print("header added:", path)
print(changed, "file(s) changed")
