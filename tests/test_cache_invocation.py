#!/usr/bin/python
import apt_pkg

import unittest

class TestCache(unittest.TestCase):

    def setUp(self):
        apt_pkg.init_config()
        apt_pkg.init_system()

    def test_wrong_invocation(self):
        """wrongly invoke GetCache() rather than GetDepCache()."""
        apt_cache = apt_pkg.Cache(apt_pkg.OpProgress())
        if apt_pkg._COMPAT_0_7:
            self.assertRaises(ValueError, apt_pkg.Cache, apt_cache)
            self.assertRaises(ValueError, apt_pkg.Cache,
                              apt_pkg.AcquireProgress())
            self.assertRaises(ValueError, apt_pkg.Cache, 0)
        else:
            self.assertRaises(TypeError, apt_pkg.Cache, apt_cache)
            self.assertRaises(TypeError, apt_pkg.Cache,
                              apt_pkg.AcquireProgress())
            self.assertRaises(TypeError, apt_pkg.Cache, 0)

    def test_proper_invocation(self):
        """Invoke it the right way."""
        apt_cache = apt_pkg.Cache(apt_pkg.OpProgress())
        apt_depcache = apt_pkg.DepCache(apt_cache)

if __name__ == "__main__":
    unittest.main()