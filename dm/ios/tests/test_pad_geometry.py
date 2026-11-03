"""The virtual gamepad's placement (dm/ios/Sources/DmPadGeometry.h), built and run on the Mac.

pad_geometry/pad_geometry_check.m runs the same pure functions DmGamepad.m draws, hit-tests and grabs with, on the iPhone, iPad
and iPhone Duo screens (portrait, landscape, tabletop), in the 2D and the 3D layout and every DM_TOUCH_SIZE: every control stays
inside the safe area (the bottom corners reach halfway into the side insets, as always), no two drawn controls overlap, and the
built-in size is the layout the gamepad has always drawn. pad_geometry/UIKit/UIKit.h stands in for the few UIKit names the
headers use, so no simulator is needed.
"""
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

IOS = Path(__file__).resolve().parents[1]
HERE = Path(__file__).resolve().parent / "pad_geometry"


@unittest.skipUnless(sys.platform == "darwin" and shutil.which("clang"), "needs clang and the Mac's CoreGraphics")
class PadGeometryTests(unittest.TestCase):
    def test_every_screen_size_and_layout(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / "pad_geometry_check"
            subprocess.run(["clang", "-x", "objective-c", "-fobjc-arc", "-Wall", "-Werror", "-Wno-unused-function",
                            "-I", str(HERE), "-I", str(IOS / "Sources"), str(HERE / "pad_geometry_check.m"),
                            "-framework", "Foundation", "-framework", "CoreGraphics", "-o", str(exe)], check=True)
            run = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn("ok:", run.stdout)


if __name__ == "__main__":
    unittest.main()
