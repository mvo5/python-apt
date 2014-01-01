import os
import subprocess
import unittest


class PackagePep8TestCase(unittest.TestCase):

    def test_pep8(self):
        res = 0
        py_dir = os.path.join(os.path.dirname(__file__), "..")
        res += subprocess.call(
            ["pep8",
             # disable some indent releated checks that are a bit over the
             # top (IMHO)
             #"--ignore=E125,E126,E127,E128",
             "--exclude", "build,tests/old",
             "--repeat", py_dir])
        if res != 0:
            self.fail("pep8 failed with: %s" % res)


if __name__ == "__main__":
    unittest.main()
