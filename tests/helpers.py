#!/usr/bin/python
#
# Copyright (C) 2014 Canonical
#
# Author: Michael Vogt <mvo@ubuntu.com>
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

import os
import shutil
import sys
import tempfile
from textwrap import dedent
import unittest

from test_all import get_library_dir
libdir = get_library_dir()
if libdir:
    sys.path.insert(0, libdir)

import apt


def touch(filename):
    with open(filename, "a"):
        pass


class AptTestCase(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self._make_dirs()
        self._make_files()
        self._make_helpers()
        # common settings needed for the tests
        apt.apt_pkg.config.set("Dir::Cache::pkgcache", "")
        apt.apt_pkg.config.set("Dir::Cache::srcpkgcache", "")
        apt.apt_pkg.config.clear("APT::Update::Post-Invoke")
        apt.apt_pkg.config.clear("APT::Update::Post-Invoke-Success")
        apt.apt_pkg.config.clear("DPkg::Post-Invoke")

    def tearDown(self):
        shutil.rmtree(self.temp_dir)

    def _make_dirs(self):
        for dirname in ["/bin", "/etc/apt", "/aptarchive/", "/var/lib/dpkg"]:
            os.makedirs(self.temp_dir + dirname)

    def _make_files(self):
        touch(self.temp_dir + "/aptarchive/Packages")
        touch(self.temp_dir + "/aptarchive/Sources")
        with open(self.temp_dir + "/etc/apt/sources.list", "w") as f:
            f.write(
                "deb file:{tmpdir}/aptarchive /".format(tmpdir=self.temp_dir))

    def _make_helpers(self):
        self.dpkg = os.path.join(self.temp_dir, "bin", "fake_dpkg")
        with open(self.dpkg, "w") as f:
            f.write(dedent("""\
            #!/bin/sh
            dpkg --root={TMPWORKINGDIRECTORY}/ --force-not-root --force-bad-path --log={TMPWORKINGDIRECTORY}/var/log/dpkg.log "$@"
            """.format(TMPWORKINGDIRECTORY=self.temp_dir)))
        os.chmod(self.dpkg, 0o755)
        apt.apt_pkg.config.set("dir::bin::dpkg", self.dpkg)

    PKG_COUNTER = 0
    def make_package(self, name="", version="1.0", arch="all"):
        if name == "":
            name = "pkg-{}".format(self.PKG_COUNTER)
            self.PKG_COUNTER += 1
        packages = os.path.join(self.temp_dir, "aptarchive", "Packages")
        with open(packages, "a") as f:
            f.write(dedent("""\
            Package: {name}
            Installed-Size: 136
            Maintainer: Joe Blow <joe@blow.com>
            Architecture: {arch}
            Version: {version}
            Filename: pool/{name}_{version}_{arch}.deb
            Size: 12345
            MD5sum: 3a8c96814b0201c285274fded3e2d203
            SHA1: 05a3c9d5ced88fc51f3aa3166b0ca4030dcc4e49
            Description: Test packge
             with some description

            """.format(name=name,
                       version=version,
                       arch=arch)))
        return name


 
