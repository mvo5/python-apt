#!/usr/bin/python
#
# Copyright (C) 2014 Canonical
#
# Author: Michael Vogt <mvo@ubuntu.com>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

import subprocess
import sys
import unittest


from test_all import get_library_dir
libdir = get_library_dir()
if libdir:
    sys.path.insert(0, libdir)

import apt
from helpers import (
    AptTestCase,
)


class MultiarchTestCase(AptTestCase):
    """Test that adding a architecture on the fly works"""

    def test_normal(self):
        pkgname_native = self.make_package()
        cache = apt.Cache(rootdir=self.temp_dir)
        cache.update()
        cache.open()
        self.assertEqual(len(cache), 1)
        self.assertEqual(cache[pkgname_native].name, pkgname_native)

    def test_multiarch_add(self):
        foreign_arch = "foo"
        pkgname_native = self.make_package()
        pkgname_foreign = self.make_package(arch=foreign_arch)
        subprocess.check_call(
            [self.dpkg, "--add-architecture", foreign_arch])
        # get the cache
        cache = apt.Cache(rootdir=self.temp_dir)
        cache.update()
        cache.open()
        self.assertEqual(cache._have_multi_arch, True)
        self.assertEqual(
            set([pkg.name for pkg in cache]),
            set([pkgname_native, "%s:%s" % (pkgname_foreign, foreign_arch)]))
        

if __name__ == "__main__":
    unittest.main()
